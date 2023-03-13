/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdint.h>

#include <arch_helpers.h>
#include <debug.h>
#include <events.h>
#include <heap/page_alloc.h>
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_realm_rmi.h>
#include <host_shared_data.h>
#include <plat_topology.h>
#include <power_management.h>
#include <realm_def.h>
#include <test_helpers.h>
#include <xlat_tables_v2.h>

static struct realm realm;
static bool realm_payload_created;
static bool shared_mem_created;
static bool realm_payload_mmaped;
static u_register_t exit_reason = RMI_EXIT_INVALID;
static unsigned int test_result = TEST_RESULT_FAIL;
static volatile bool timer_enabled;

/* From the TFTF_BASE offset, memory used by TFTF + Shared + Realm + POOL should
 * not exceed DRAM_END offset
 * NS_REALM_SHARED_MEM_BASE + NS_REALM_SHARED_MEM_SIZE is considered last offset
 */
CASSERT((((uint64_t)NS_REALM_SHARED_MEM_BASE + (uint64_t)NS_REALM_SHARED_MEM_SIZE)\
	< ((uint64_t)DRAM_BASE + (uint64_t)DRAM_SIZE)),\
	error_ns_memory_and_realm_payload_exceed_DRAM_SIZE);

/*
 * The function handler to print the Realm logged buffer,
 * executed by the secondary core
 */
static inline test_result_t timer_handler(void)
{
	size_t str_len = 0UL;
	host_shared_data_t *host_shared_data = host_get_shared_structure();
	char *log_buffer = (char *)host_shared_data->log_buffer;

	do {
		spin_lock((spinlock_t *)&host_shared_data->printf_lock);
		str_len = strlen((const char *)log_buffer);

		/*
		 * Read Realm message from shared printf location and print
		 * them using UART
		 */
		if (str_len != 0UL) {
			/* Avoid memory overflow */
			log_buffer[MAX_BUF_SIZE - 1] = 0U;

			mp_printf("%s", log_buffer);
			(void)memset((char *)log_buffer, 0, MAX_BUF_SIZE);
		}
		spin_unlock((spinlock_t *)&host_shared_data->printf_lock);

	} while ((timer_enabled || (str_len != 0UL)));

	return TEST_RESULT_SUCCESS;
}

/*
 * Initialisation function which will clear the shared region,
 * and try to find another CPU other than the lead one to
 * handle the Realm message logging
 */
void host_init_realm_print_buffer(void)
{
	u_register_t other_mpidr, my_mpidr;
	int ret;
	host_shared_data_t *host_shared_data = host_get_shared_structure();

	(void)memset((char *)host_shared_data, 0, sizeof(host_shared_data_t));
	/* Program timer */
	timer_enabled = false;

	/* Find a valid CPU to power on */
	my_mpidr = read_mpidr_el1() & MPID_MASK;
	other_mpidr = tftf_find_any_cpu_other_than(my_mpidr);
	if (other_mpidr == INVALID_MPID) {
		ERROR("Couldn't find a valid other CPU\n");
		return;
	}

	/* Power on the other CPU */
	ret = tftf_cpu_on(other_mpidr, (uintptr_t)timer_handler, 0);
	if (ret != PSCI_E_SUCCESS) {
		ERROR("Powering on %lx failed\n", other_mpidr);
		return;
	}
	timer_enabled = true;
}

/**
 *   @brief    - Add regions assigned to Host into its translation table data
 *   structure.
 **/
static test_result_t host_mmap_realm_payload(u_register_t realm_payload_adr,
		u_register_t plat_mem_pool_adr,
		u_register_t plat_mem_pool_size)
{
	if (realm_payload_mmaped) {
		return REALM_SUCCESS;
	}

	/* Memory Pool region */
	int rc = mmap_add_dynamic_region(plat_mem_pool_adr,
					plat_mem_pool_adr,
					plat_mem_pool_size,
					MT_RW_DATA | MT_NS);

	if (rc != 0) {
		ERROR("%u: mmap_add_dynamic_region() = %d\n", __LINE__, rc);
		return TEST_RESULT_FAIL;
	}

	/* Realm Image region */
	rc = mmap_add_dynamic_region(realm_payload_adr,
					realm_payload_adr,
					REALM_MAX_LOAD_IMG_SIZE,
					MT_RW_DATA | MT_NS);

	if (rc != 0) {
		ERROR("%u: mmap_add_dynamic_region() = %d\n", __LINE__, rc);
		return TEST_RESULT_FAIL;
	}
	realm_payload_mmaped = true;
	return REALM_SUCCESS;
}

static bool host_enter_realm(u_register_t *exit_reason, unsigned int *test_result)
{
	u_register_t ret;

	if (!realm_payload_created) {
		ERROR("%s() failed, Realm not created\n", __func__);
		return false;
	}
	if (!shared_mem_created) {
		ERROR("%s() failed, shared memory not created\n", __func__);
		return false;
	}

	/* Enter Realm */
	ret = realm_rec_enter(&realm, exit_reason, test_result);
	if (ret != REALM_SUCCESS) {
		ERROR("%s() failed, ret=%lx\n", "realm_rec_enter", ret);

		/* Free test resources */
		if (realm_destroy(&realm) != REALM_SUCCESS) {
			ERROR("%s() failed\n", "realm_destroy");
		}
		realm_payload_created = false;
		return false;
	}

	return true;
}

bool host_create_realm_payload(u_register_t realm_payload_adr,
		u_register_t plat_mem_pool_adr,
		u_register_t plat_mem_pool_size,
		u_register_t realm_pages_size,
		u_register_t feature_flag)
{
	if (realm_payload_adr == TFTF_BASE) {
		ERROR("realm_payload_adr should be grater then TFTF_BASE\n");
		return false;
	}

	if (plat_mem_pool_adr  == 0UL ||
			plat_mem_pool_size == 0UL ||
			realm_pages_size == 0UL) {
		ERROR("plat_mem_pool_size or "
			"plat_mem_pool_size or realm_pages_size is Null\n");
		return false;
	}
	/* Initialize  Host NS heap memory to be used in Realm creation*/
	if (page_pool_init(plat_mem_pool_adr, realm_pages_size)
		!= HEAP_INIT_SUCCESS) {
		ERROR("page_pool_init() failed\n");
		return false;
	}

	/* Mmap Realm payload region */
	if (host_mmap_realm_payload(realm_payload_adr,
			plat_mem_pool_adr,
			plat_mem_pool_size) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "host_mmap_realm_payload");
		return false;
	}

	/* Read Realm Feature Reg 0 */
	if (rmi_features(0UL, &realm.rmm_feat_reg0) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "rmi_features");
		goto destroy_realm;
	}

	/* Disable PMU if not required */
	if ((feature_flag & RMI_FEATURE_REGISTER_0_PMU_EN) == 0UL) {
		realm.rmm_feat_reg0 &=
			~(RMI_FEATURE_REGISTER_0_PMU_EN |
			  RMI_FEATURE_REGISTER_0_PMU_NUM_CTRS);
	}

	/* Create Realm */
	if (realm_create(&realm) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "realm_create");
		goto destroy_realm;
	}

	if (realm_init_ipa_state(&realm, 0U, 0U, 1ULL << 32)
		!= RMI_SUCCESS) {
		ERROR("%s() failed\n", "realm_init_ipa_state");
		goto destroy_realm;
	}

	/* RTT map Realm image */
	if (realm_map_payload_image(&realm, realm_payload_adr) !=
			REALM_SUCCESS) {
		ERROR("%s() failed\n", "realm_map_payload_image");
		goto destroy_realm;
	}

	/* Create REC */
	if (realm_rec_create(&realm) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "realm_rec_create");
		goto destroy_realm;
	}

	/* Activate Realm */
	if (realm_activate(&realm) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "realm_activate");
		goto destroy_realm;
	}

	realm_payload_created = true;

	return realm_payload_created;

	/* Free test resources */
destroy_realm:
	if (realm_destroy(&realm) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "realm_destroy");
	}
	realm_payload_created = false;

	return realm_payload_created;
}

bool host_create_shared_mem(u_register_t ns_shared_mem_adr,
	u_register_t ns_shared_mem_size)
{
	/* RTT map NS shared region */
	if (realm_map_ns_shared(&realm, ns_shared_mem_adr,
				ns_shared_mem_size) != REALM_SUCCESS) {
		ERROR("%s() failed\n", "realm_map_ns_shared");
		shared_mem_created = false;
		return false;
	}

	host_init_realm_print_buffer();
	realm_shared_data_clear_realm_val();
	shared_mem_created = true;

	return shared_mem_created;
}

bool host_destroy_realm(void)
{
	/* Free test resources */
	timer_enabled = false;
	page_pool_reset();

	if (!realm_payload_created) {
		ERROR("%s() failed, Realm not created\n", __func__);
		return false;
	}

	realm_payload_created = false;
	if (realm_destroy(&realm) != REALM_SUCCESS) {
		ERROR("%s\n", "realm_destroy failed");
		return false;
	}

	return true;
}

bool host_enter_realm_execute(uint8_t cmd)
{
	exit_reason = RMI_EXIT_INVALID;
	test_result = TEST_RESULT_FAIL;

	realm_shared_data_set_realm_cmd(cmd);
	if (!host_enter_realm(&exit_reason, &test_result)) {
		return false;
	}

	if (exit_reason == RMI_EXIT_HOST_CALL &&
		test_result == TEST_RESULT_SUCCESS) {
		return true;
	}
	ERROR("%s() exit_reason: [0x%lx] test_result: [0x%x]\n",
		__func__, exit_reason, test_result);
	return false;
}

test_result_t host_cmp_result(void)
{
	if (rmi_get_cmp_result()) {
		return TEST_RESULT_SUCCESS;
	}

	ERROR("RMI registers comparison failed\n");
	return TEST_RESULT_FAIL;
}


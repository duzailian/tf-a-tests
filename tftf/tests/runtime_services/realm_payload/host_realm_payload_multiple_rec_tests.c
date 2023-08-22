/*
 * Copyright (c) 2021-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <drivers/arm/arm_gic.h>
#include <debug.h>
#include <platform.h>
#include <power_management.h>
#include <psci.h>
#include <sgi.h>
#include <test_helpers.h>

#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_realm_pmu.h>
#include <host_shared_data.h>

static volatile int is_secondary_cpu_on;
static spinlock_t counter_lock;
/*
 * Test tries to create max Rec
 * Enters all Rec from single CPU
 */
test_result_t host_realm_multi_rec_single_cpu(void)
{
	bool ret1, ret2;
	u_register_t rec_flag[] = {RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE,
	RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE};

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (!host_create_realm_payload((u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)(PAGE_POOL_MAX_SIZE +
			NS_REALM_SHARED_MEM_SIZE),
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL, rec_flag, MAX_REC_COUNT)) {
		return TEST_RESULT_FAIL;
	}
	if (!host_create_shared_mem(NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		return TEST_RESULT_FAIL;
	}

	for (unsigned int i = 0; i < MAX_REC_COUNT; i++) {
		realm_shared_data_set_host_val(i, HOST_SLEEP_INDEX, 10U);
		ret1 = host_enter_realm_execute(REALM_SLEEP_CMD, NULL,
				RMI_EXIT_HOST_CALL, i);
		if (!ret1) {
			break;
		}
	}

	ret2 = host_destroy_realm();

	if (!ret1 || !ret2) {
		ERROR("%s(): enter=%d destroy=%d\n",
		__func__, ret1, ret2);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * Test creates 3 Rec
 * Rec0 requests CPU ON for rec 2
 * Host denies CPU On for rec 2, re-enters rec 0
 * Rec 0 checks CPU ON is denied
 * Rec0 requests CPU ON for rec 3
 * Host denies CPU On which should fail
 */
test_result_t host_realm_multi_rec_psci_denied(void)
{
	struct realm *realm_ptr;
	bool ret1, ret2;
	u_register_t ret;
	unsigned int host_call_result;
	u_register_t exit_reason;
	unsigned int rec_num;
	struct rmi_rec_run *run;
	/* Create 3 rec Rec 0 and 2 are runnable, Rec 1 in not runnable */
	u_register_t rec_flag[] = {RMI_RUNNABLE, RMI_NOT_RUNNABLE, RMI_RUNNABLE};

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (!host_create_realm_payload((u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)(PAGE_POOL_MAX_SIZE +
			NS_REALM_SHARED_MEM_SIZE),
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL, rec_flag, 3U)) {
		return TEST_RESULT_FAIL;
	}
	if (!host_create_shared_mem(NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		return TEST_RESULT_FAIL;
	}

	ret1 = host_enter_realm_execute(REALM_MULTIPLE_REC_PSCI_DENIED_CMD, &realm_ptr,
			RMI_EXIT_PSCI, 0U);
	run = (struct rmi_rec_run *)realm_ptr->run[0];

	if (run->exit.gprs[0] != SMC_PSCI_CPU_ON_AARCH64) {
		ERROR("Host did not receive CPU ON request\n");
		ret1 = REALM_ERROR;
		goto destroy_realm;
	}
	rec_num = host_realm_find_rec_by_mpidr(run->exit.gprs[1], realm_ptr);
	if (rec_num != 1U) {
		ERROR("Invalid mpidr requested\n");
		ret1 = REALM_ERROR;
		goto destroy_realm;
	}
	INFO("Requesting PSCI Complete Status Denied REC %d\n", rec_num);
	ret = host_rmi_psci_complete(realm_ptr->rec[0], realm_ptr->rec[rec_num],
			(unsigned long)PSCI_E_DENIED);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_psci_complete failed\n");
		ret1 = REALM_ERROR;
		goto destroy_realm;
	}
	ret1 = host_realm_rec_enter(realm_ptr, &exit_reason, &host_call_result, 0U);

	if (run->exit.gprs[0] != SMC_PSCI_AFFINITY_INFO_AARCH64) {
		ERROR("Host did not receive PSCI_AFFINITY_INFO request\n");
		ret1 = REALM_ERROR;
		goto destroy_realm;
	}
	rec_num = host_realm_find_rec_by_mpidr(run->exit.gprs[1], realm_ptr);
	if (rec_num != 1U) {
		ERROR("Invalid mpidr requested\n");
		goto destroy_realm;
	}

	INFO("Requesting PSCI Complete Affinity Info REC %d\n", rec_num);
	ret = host_rmi_psci_complete(realm_ptr->rec[0], realm_ptr->rec[rec_num],
			(unsigned long)PSCI_E_SUCCESS);
	if (ret != RMI_SUCCESS) {
		ERROR("host_rmi_psci_complete failed\n");
		ret1 = REALM_ERROR;
		goto destroy_realm;
	}

	/* Re-enter REC0 complete PSCI_AFFINITY_INFO */
	ret1 = host_realm_rec_enter(realm_ptr, &exit_reason, &host_call_result, 0U);


	if (run->exit.gprs[0] != SMC_PSCI_CPU_ON_AARCH64) {
		ERROR("Host did not receive CPU ON request\n");
		ret1 = REALM_ERROR;
		goto destroy_realm;
	}
	rec_num = host_realm_find_rec_by_mpidr(run->exit.gprs[1], realm_ptr);
	if (rec_num != 2U) {
		ERROR("Invalid mpidr requested\n");
		ret1 = REALM_ERROR;
		goto destroy_realm;
	}

	INFO("Requesting PSCI Complete Status Denied REC %d\n", rec_num);
	/*PSCI_DENIED should fail as rec2 is RMI_RUNNABLE */
	ret = host_rmi_psci_complete(realm_ptr->rec[0], realm_ptr->rec[rec_num],
			(unsigned long)PSCI_E_DENIED);
	if (ret == RMI_SUCCESS) {
		ret1 = REALM_ERROR;
		ERROR("host_rmi_psci_complete should have failed\n");
		goto destroy_realm;
	} else if (ret == (unsigned long)PSCI_E_DENIED) {
		ret1 = REALM_SUCCESS;
		goto destroy_realm;
	}

destroy_realm:
	ret2 = host_destroy_realm();

	if ((ret1 != REALM_SUCCESS) || !ret2) {
		ERROR("%s(): enter=%d destroy=%d\n",
		__func__, ret1, ret2);
		return TEST_RESULT_FAIL;
	}

	return host_cmp_result();
}

static test_result_t cpu_on_handler2(void)
{
	unsigned int i;
	bool ret;

	spin_lock(&counter_lock);
	i = ++is_secondary_cpu_on;
	spin_unlock(&counter_lock);
	ret = host_enter_realm_execute(REALM_LOOP_CMD, NULL, RMI_EXIT_IRQ, i);
	if (!ret) {
		return TEST_RESULT_FAIL;
	}
	return TEST_RESULT_SUCCESS;
}

test_result_t host_realm_multi_rec_exit_irq(void)
{
	bool ret1, ret2;
	int ret;
	u_register_t other_mpidr, my_mpidr;
	u_register_t rec_flag[] = {RMI_RUNNABLE, RMI_RUNNABLE, RMI_RUNNABLE,
		RMI_RUNNABLE};

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (!host_create_realm_payload((u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)(PAGE_POOL_MAX_SIZE +
			NS_REALM_SHARED_MEM_SIZE),
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL, rec_flag, 4U)) {
		return TEST_RESULT_FAIL;
	}
	if (!host_create_shared_mem(NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		return TEST_RESULT_FAIL;
	}

	is_secondary_cpu_on = 0U;
	my_mpidr = read_mpidr_el1() & MPID_MASK;
	ret1 = host_enter_realm_execute(REALM_GET_RSI_VERSION, NULL, RMI_EXIT_HOST_CALL, 0U);
	for (unsigned int i = 1U; i < 4U; i++) {
		do {
			other_mpidr = tftf_find_random_cpu_other_than(my_mpidr);
			if (other_mpidr == INVALID_MPID) {
				ERROR("Couldn't find a valid other CPU\n");
				ret1 = REALM_ERROR;
				goto destroy_realm;
			}
			/* Power on the other CPU */
			ret = tftf_try_cpu_on(other_mpidr, (uintptr_t)cpu_on_handler2, 0);
		} while (ret != PSCI_E_SUCCESS);
	}

	INFO("Wait for all CPU to come up %d\n", is_secondary_cpu_on);
	waitms(200);

destroy_realm:
	tftf_irq_enable(IRQ_NS_SGI_7, GIC_HIGHEST_NS_PRIORITY);
	for (unsigned int i = 1U; i < 4U; i++) {
		INFO("Raising NS IRQ for rec %d\n", i);
		host_rec_force_exit(IRQ_NS_SGI_7, i);
	}
	tftf_irq_disable(IRQ_NS_SGI_7);
	ret2 = host_destroy_realm();
	if (!ret1 || !ret2) {
		ERROR("%s(): enter=%d destroy=%d\n",
		__func__, ret1, ret2);
		return TEST_RESULT_FAIL;
	}

	return host_cmp_result();
}


static test_result_t cpu_on_handler(void)
{
	unsigned int i;
	bool ret;
	struct realm *realm_ptr;
	struct rmi_rec_run *run;

	spin_lock(&counter_lock);
	i = ++is_secondary_cpu_on;
	spin_unlock(&counter_lock);
	ret = host_enter_realm_execute(REALM_MULTIPLE_REC_MULTIPLE_CPU_CMD, &realm_ptr,
			RMI_EXIT_PSCI, i);
	if (ret) {
		run = (struct rmi_rec_run *)realm_ptr->run[i];
		if (run->exit.gprs[0] == SMC_PSCI_CPU_OFF) {
			return TEST_RESULT_SUCCESS;
		}
	}
	return TEST_RESULT_FAIL;
}

/*
 * test tries to create MAX recs
 * Enter Rec from different CPUs
 * Rec0 checks if all other CPUs are off
 * Host tries to re-enter rec after CPU OFF, expects error
 */
test_result_t host_realm_multi_rec_multiple_cpu(void)
{
	struct realm *realm_ptr;
	bool ret1, ret2;
	int ret;
	u_register_t rec_num, i = 1U;
	u_register_t other_mpidr, my_mpidr;
	struct rmi_rec_run *run;
	unsigned int host_call_result;
	u_register_t rec_flag[] = {RMI_RUNNABLE, RMI_NOT_RUNNABLE, RMI_NOT_RUNNABLE,
		RMI_NOT_RUNNABLE, RMI_NOT_RUNNABLE, RMI_NOT_RUNNABLE, RMI_NOT_RUNNABLE,
		RMI_NOT_RUNNABLE};
	u_register_t exit_reason;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	if (!host_create_realm_payload((u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)(PAGE_POOL_MAX_SIZE +
			NS_REALM_SHARED_MEM_SIZE),
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL, rec_flag, MAX_REC_COUNT)) {
		return TEST_RESULT_FAIL;
	}
	if (!host_create_shared_mem(NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		return TEST_RESULT_FAIL;
	}

	init_spinlock(&counter_lock);
	my_mpidr = read_mpidr_el1() & MPID_MASK;
	ret1 = host_enter_realm_execute(REALM_MULTIPLE_REC_MULTIPLE_CPU_CMD, &realm_ptr,
			RMI_EXIT_PSCI, 0U);
	do {
		run = (struct rmi_rec_run *)realm_ptr->run[0];
		if (run->exit.gprs[0] == SMC_PSCI_CPU_ON_AARCH64) {
			rec_num = host_realm_find_rec_by_mpidr(run->exit.gprs[1], realm_ptr);
			if (rec_num >= MAX_REC_COUNT) {
				ERROR("Invalid mpidr requested\n");
				ret1 = REALM_ERROR;
				goto destroy_realm;
			}
			ret = host_rmi_psci_complete(realm_ptr->rec[0], realm_ptr->rec[rec_num],
					(unsigned long)PSCI_E_SUCCESS);
			if (ret == RMI_SUCCESS) {
				do {
					/* Find another CPU for new Rec */
					other_mpidr = tftf_find_random_cpu_other_than(my_mpidr);
					if (other_mpidr == INVALID_MPID) {
						ERROR("Couldn't find a valid other CPU\n");
						ret1 = REALM_ERROR;
						goto destroy_realm;
					}
					/* Power on the other CPU */
					ret = tftf_try_cpu_on(other_mpidr,
						(uintptr_t)cpu_on_handler, 0);
				} while (ret != PSCI_E_SUCCESS);

				/* Re-enter REC0 complete CPU_ON */
				ret1 = host_realm_rec_enter(realm_ptr, &exit_reason,
					&host_call_result, 0U);
			} else {
				ERROR("host_rmi_psci_complete failed\n");
				ret1 = REALM_ERROR;
				goto destroy_realm;
			}
		} else {
			ERROR("Host did not receive CPU ON request\n");
			ret1 = REALM_ERROR;
			goto destroy_realm;
		}
	} while (++i < MAX_REC_COUNT);

	for (i = 1U; i < MAX_REC_COUNT; i++) {
		if (run->exit.gprs[0] == SMC_PSCI_AFFINITY_INFO_AARCH64) {
			rec_num = host_realm_find_rec_by_mpidr(run->exit.gprs[1], realm_ptr);
			if (rec_num >= MAX_REC_COUNT) {
				ERROR("Invalid mpidr requested\n");
				goto destroy_realm;
			}
			ret = host_rmi_psci_complete(realm_ptr->rec[0], realm_ptr->rec[rec_num],
					(unsigned long)PSCI_E_SUCCESS);

			if (ret != RMI_SUCCESS) {
				ERROR("host_rmi_psci_complete failed\n");
				ret1 = REALM_ERROR;
				goto destroy_realm;
			}

			/* Re-enter REC0 complete PSCI_AFFINITY_INFO */
			ret1 = host_realm_rec_enter(realm_ptr, &exit_reason, &host_call_result, 0U);
		}
	}

	/* Try enter REC2 on different CPU */
	run = (struct rmi_rec_run *)realm_ptr->run[0];
	if (run->exit.gprs[0] == SMC_PSCI_CPU_ON_AARCH64) {
		rec_num = host_realm_find_rec_by_mpidr(run->exit.gprs[1], realm_ptr);
		if (rec_num >= MAX_REC_COUNT) {
			ERROR("Invalid mpidr requested\n");
			ret1 = REALM_ERROR;
			goto destroy_realm;
		}

		ret = host_rmi_psci_complete(realm_ptr->rec[0], realm_ptr->rec[2],
			(unsigned long)PSCI_E_SUCCESS);
		if (ret != RMI_SUCCESS) {
			ERROR("host_rmi_psci_complete failed\n");
		}

		ret1 = host_realm_rec_enter(realm_ptr, &exit_reason,
			&host_call_result, 2U);

		if (ret != RMI_SUCCESS) {
			ERROR("Re-enter REC2 failed\n");
			ret1 = REALM_ERROR;
			goto destroy_realm;
		}
	}

	/* Try to run Rec3(CPU OFF/NOT_RUNNABLE), expect error */
	ret1 = host_realm_rec_enter(realm_ptr, &exit_reason,
			&host_call_result, 3U);

	if (ret1 == RMI_SUCCESS) {
		ERROR("Expected error\n");
		goto destroy_realm;
	}

	/* Re-enter REC0 complete CPU_ON */
	ret1 = host_realm_rec_enter(realm_ptr, &exit_reason,
			&host_call_result, 0U);

destroy_realm:
	ret2 = host_destroy_realm();

	if ((ret1 != REALM_SUCCESS) || !ret2) {
		ERROR("%s(): enter=%d destroy=%d\n",
		__func__, ret1, ret2);
		return TEST_RESULT_FAIL;
	}

	return host_cmp_result();
}


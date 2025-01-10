/*
 * Copyright (c) 2024-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <limits.h>

#include <arch_features.h>
#include <arch_helpers.h>
#include <debug.h>
#include <lib/self_hosted_debug/self_hosted_debug_helpers.h>
#include <test_helpers.h>

#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_shared_data.h>

typedef struct {
	long unsigned int (*read_dbgbcr_el1)(void);
	long unsigned int (*read_dbgbvr_el1)(void);
	long unsigned int (*read_dbgwcr_el1)(void);
	long unsigned int (*read_dbgwvr_el1)(void);
	void (*write_dbgbcr_el1)(long unsigned int);
	void (*write_dbgbvr_el1)(long unsigned int);
	void (*write_dbgwcr_el1)(long unsigned int);
	void (*write_dbgwvr_el1)(long unsigned int);
}ctx_fnptr_t;

#define NS_DEBUG_FNPTRS(n)									\
	{read_dbgbcr##n##_el1, read_dbgbvr##n##_el1, read_dbgwcr##n##_el1, read_dbgwvr##n##_el1, \
	write_dbgbcr##n##_el1, write_dbgbvr##n##_el1, write_dbgwcr##n##_el1, write_dbgwvr##n##_el1}


ctx_fnptr_t debug_regs[MAX_BPS] = {
	NS_DEBUG_FNPTRS(0),
	NS_DEBUG_FNPTRS(1),
	NS_DEBUG_FNPTRS(2),
	NS_DEBUG_FNPTRS(3),
	NS_DEBUG_FNPTRS(4),
	NS_DEBUG_FNPTRS(5),
	NS_DEBUG_FNPTRS(6),
	NS_DEBUG_FNPTRS(7),
	NS_DEBUG_FNPTRS(8),
	NS_DEBUG_FNPTRS(9),
	NS_DEBUG_FNPTRS(10),
	NS_DEBUG_FNPTRS(11),
	NS_DEBUG_FNPTRS(12),
	NS_DEBUG_FNPTRS(13),
	NS_DEBUG_FNPTRS(14),
	NS_DEBUG_FNPTRS(15)
};

/*
 * Set DBGBCRn_EL1 fields HMC, PMC, SSC and SSCE such that breakpoint n will
 * generate Breakpoint exceptions only in NS Security state.
 */
static u_register_t ns_set_dbgbcr_el1(u_register_t current_val)
{
	u_register_t new_val = set_dbgbcr_el1(current_val) |
			       INPLACE(DBGBCR_EL1_SSC, 1UL) |
			       INPLACE(DBGBCR_EL1_PMC, 1UL);

	new_val &= ~(DBGBCR_EL1_HMC_BIT | DBGBCR_EL1_SSCE_BIT);

	return new_val;
}

/*
 * Verify that the value of DBGBCRn_EL1 is the same as what was written
 * previously.
 */
static bool ns_check_dbgbcr_el1(u_register_t reg)
{
	u_register_t expected_dbgbcr_el1 = ns_set_dbgbcr_el1(0UL);

	if (reg != expected_dbgbcr_el1) {
		return false;
	}

	return true;
}

/*
 * Set DBGWCRn_EL1 fields HMC, PAC, SSC and SSCE such that watchpoint n will
 * generate Watchpoint exceptions only in NS Security state.
 */
static u_register_t ns_set_dbgwcr_el1(u_register_t current_val)
{
	u_register_t new_val = set_dbgwcr_el1(current_val) |
			       INPLACE(DBGWCR_EL1_SSC, 1UL) |
			       INPLACE(DBGWCR_EL1_PAC, 1UL);

	new_val &= ~(DBGWCR_EL1_HMC_BIT | DBGWCR_EL1_SSCE_BIT);

	return new_val;
}

/*
 * Verify that the value of DBGWCRn_EL1 is the same as what was written
 * previously.
 */
static bool ns_check_dbgwcr_el1(u_register_t reg)
{
	u_register_t expected_dbgwcr_el1 = ns_set_dbgwcr_el1(0UL);

	if (reg != expected_dbgwcr_el1) {
		return false;
	}

	return true;
}

/*
 * Read ID_AA64DFR0_EL1 to determine the number of breakpoints
 * and watchpoints supported by the hardware.
 * BRPs [15:12] - no of BPs supported - 1
 * WRPs [23:20] - no of WPs supported - 1
 * The value 0b0000 is reserved. Implies that min BPs/WPs = 2
 * If the above fields read 0b1111 => read ID_AA64DFR1_EL1.
 * ID_AA64DFR1_EL1 indicates the no of BPs or WPs supported.
 * BRPs [15:8] ->  no of BPs supported - 1
 * WRPs [23:16] -> no of WPs supported - 1
 */

test_result_t host_test_realm_num_bps_wps(void)
{
	unsigned long num_bps, num_wps;
	u_register_t feature_flag;
	u_register_t rec_flag[1] = {RMI_RUNNABLE};
	struct realm realm;
	bool realm_rc;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	num_bps = EXTRACT(ID_AA64DFR0_BRPs, read_id_aa64dfr0_el1());

	/* If BPs is 0xF, read DFR1 to find no of BPs supported by HW */
	if (num_bps == MAX_BPS - 1UL) {
		num_bps = EXTRACT(ID_AA64DFR1_BRPs, read_id_aa64dfr1_el1());
		if (num_bps == 0UL) {
			num_bps = MAX_BPS - 1UL;
		}
	}

	/* If WPs is 0xF, read DFR1 to find no of BPs supported by HW */
	num_wps = EXTRACT(ID_AA64DFR0_WRPs, read_id_aa64dfr0_el1());

	if (num_wps == MAX_WPS - 1UL) {
		num_wps = EXTRACT(ID_AA64DFR1_WRPs, read_id_aa64dfr1_el1());
		if(num_wps == 0UL) {
			num_wps = MAX_WPS - 1UL;
		}
	}

	INFO("BPs and WPs supported in H/W is %ld and %ld\n", num_bps+1, num_wps+1);

	feature_flag = INPLACE(RMI_FEATURE_REGISTER_0_NUM_BPS, num_bps) |
		       INPLACE(RMI_FEATURE_REGISTER_0_NUM_WPS, num_wps);

	if (!host_create_activate_realm_payload(&realm,
					(u_register_t)REALM_IMAGE_BASE,
					feature_flag, RTT_MIN_LEVEL, rec_flag, 1U, 0U)) {
		return TEST_RESULT_FAIL;
	}

	if (!host_create_shared_mem(&realm)) {
		return TEST_RESULT_FAIL;
	}

	host_shared_data_set_host_val(&realm, 0, 0, HOST_ARG1_INDEX, num_bps);
	host_shared_data_set_host_val(&realm, 0, 0, HOST_ARG2_INDEX, num_wps);

	realm_rc = host_enter_realm_execute(&realm,
					    REALM_DEBUG_CHECK_NUM_BPS_WPS,
					    RMI_EXIT_HOST_CALL,
					    0);

	if (!realm_rc || !host_destroy_realm(&realm)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * Verify that Realm created has the same number of BPs/WPs
 * as what the NS Host has requested. This test case covers the
 * scenario where the NS Host requests a realm to be created with
 * BPs or WPs lesser than that supported by hardware.
 * ID_AA64DFR0_BRPs - 0x0001 - 0x1111 (No of BPs - 1)
 * ID_AA64DFR1_BRPs - max supported 64BPs
 *
 */
test_result_t host_request_lesser_num_bps_wps(void)
{
	volatile unsigned long num_bps, num_wps;
	u_register_t feature_flag;
	u_register_t rec_flag[1] = {RMI_RUNNABLE};
	struct realm realm;
	bool realm_rc;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	num_bps = EXTRACT(ID_AA64DFR0_BRPs, read_id_aa64dfr0_el1());

	/* If BPs is 0xF, read DFR1 to find no of BPs supported by HW */
	if (num_bps == MAX_BPS - 1UL) {
		num_bps = EXTRACT(ID_AA64DFR1_BRPs, read_id_aa64dfr1_el1());
		if (num_bps == 0UL) {
			num_bps = MAX_BPS - 1UL;
		}
	}

	/* If WPs is 0xF, read DFR1 to find no of BPs supported by HW */
	num_wps = EXTRACT(ID_AA64DFR0_WRPs, read_id_aa64dfr0_el1());

	if (num_wps == MAX_WPS - 1UL) {
		num_wps = EXTRACT(ID_AA64DFR1_WRPs, read_id_aa64dfr1_el1());
		if(num_wps == 0UL) {
			num_wps = MAX_WPS - 1UL;
		}
	}
	INFO("BPs and WPs supported in H/W is %ld and %ld\n", num_bps+1, num_wps+1);

	num_wps = MIN_WPS_REQUESTED - 1;

	num_bps = MIN_BPS_REQUESTED - 1;

	INFO("BPs and WPs requested by NS host for realm is %ld and %ld\n", num_bps+1, num_wps+1);

	feature_flag = INPLACE(RMI_FEATURE_REGISTER_0_NUM_BPS, num_bps) |
		       INPLACE(RMI_FEATURE_REGISTER_0_NUM_WPS, num_wps);

	if (!host_create_activate_realm_payload(&realm,
					(u_register_t)REALM_IMAGE_BASE,
					feature_flag, RTT_MIN_LEVEL, rec_flag, 1U, 0U)) {
		return TEST_RESULT_FAIL;
	}

	if (!host_create_shared_mem(&realm)) {
		return TEST_RESULT_FAIL;
	}

	host_shared_data_set_host_val(&realm, 0, 0, HOST_ARG1_INDEX, num_bps);
	host_shared_data_set_host_val(&realm, 0, 0, HOST_ARG2_INDEX, num_wps);

	realm_rc = host_enter_realm_execute(&realm,
					    REALM_DEBUG_CHECK_NUM_BPS_WPS,
					    RMI_EXIT_HOST_CALL,
					    0);

	if (!realm_rc || !host_destroy_realm(&realm)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * Write random values to DBGBVRn_EL1 registers and set the values of
 * DBGBCRn_EL1 registers.
 */
static void ns_set_bps(int num_bps, u_register_t expected_bp_addr[][MAX_BPS])
{
	unsigned int rand_addr;
	uint8_t iter, bp_count, bank_no;

	bank_no = EXTRACT(MDSELR_EL1_BANK, read_mdselr_el1());
	bp_count = num_bps;

	for (uint8_t bank = 0U; bank <= bank_no; bank++) {

		write_mdselr_el1(INPLACE(MDSELR_EL1_BANK, bank));

		iter = (bp_count >= MAX_BPS) ? ((bp_count -= MAX_BPS), (MAX_BPS - 1)) : bp_count;

		printf("NS set BPS: Bank No= %d\n", bank);

		for(int i = 0; i <= iter; i++) {
			u_register_t value = debug_regs[i].read_dbgbcr_el1();
			value = ns_set_dbgbcr_el1(debug_regs[i].read_dbgbcr_el1());
			debug_regs[i].write_dbgbcr_el1(value);
			rand_addr = get_random_address();
			debug_regs[i].write_dbgbvr_el1(INPLACE(DBGBVR_EL1_VA, rand_addr));
			expected_bp_addr[bank][i] = rand_addr;

		}
	}
	INFO("read_dbgbvr[%d]= %lx\n", iter, debug_regs[iter].read_dbgbvr_el1());
	printf("\n");

}

/*
 * Write random values to DBGWVRn_EL1 registers and set the values of
 * DBGWCRn_EL1 registers.
 */
static void ns_set_wps(int num_wps, u_register_t expected_wp_addr[][MAX_WPS])
{
	unsigned int rand_addr;
	uint8_t iter, wp_count, bank_no;

	bank_no = EXTRACT(MDSELR_EL1_BANK, read_mdselr_el1());
	wp_count = num_wps;

	for (uint8_t bank = 0U; bank <= bank_no; bank++) {

		write_mdselr_el1(INPLACE(MDSELR_EL1_BANK, bank));

		iter = (wp_count >= MAX_WPS) ? ((wp_count -= MAX_WPS), (MAX_WPS - 1)) : wp_count;

		printf("NS set WPS: Bank No= %d\n", bank);

		for(int i = 0; i <= iter; i++) {
			u_register_t value = debug_regs[i].read_dbgwcr_el1();
			value = ns_set_dbgwcr_el1(debug_regs[i].read_dbgwcr_el1());
			debug_regs[i].write_dbgwcr_el1(value);
			rand_addr = get_random_address();
			debug_regs[i].write_dbgwvr_el1(INPLACE(DBGWVR_EL1_VA, rand_addr));
			expected_wp_addr[bank][i] = rand_addr;
		}
	}
	INFO ("read_dbgwvr[%d]= %lx\n", iter, debug_regs[iter].read_dbgwvr_el1());
	printf("\n");
}

/*
 * Verify that the values of DBGBCRn_EL1 and DBGBVRn_EL1 registers are the same
 * as what was written previously.
 */
static test_result_t ns_check_bps(int num_bps, u_register_t expected_bp_addr[][MAX_BPS])
{
	test_result_t rc = TEST_RESULT_FAIL;

	uint8_t iter, bp_count, bank_no;

	bank_no = EXTRACT(MDSELR_EL1_BANK, read_mdselr_el1());
	bp_count = num_bps;

	for (uint8_t bank = 0U; bank <= bank_no; bank++) {

		write_mdselr_el1(INPLACE(MDSELR_EL1_BANK, bank));

		iter = (bp_count >= MAX_BPS) ? ((bp_count -= MAX_BPS), (MAX_BPS - 1)) : bp_count;

		printf("NS check BPS: Bank No= %d\n", bank);

		for(int i = 0; i <= iter; i++) {
			if (!ns_check_dbgbcr_el1(debug_regs[i].read_dbgbcr_el1()) && \
				(EXTRACT(DBGBVR_EL1_VA, debug_regs[i].read_dbgbvr_el1()) != \
				expected_bp_addr[bank][i])) {
				rc = TEST_RESULT_FAIL;
				break;
			}
		}
		rc = (bank == bank_no) ? TEST_RESULT_SUCCESS : TEST_RESULT_FAIL;
	}
	INFO("read_dbgbvr_el1[%d] = %lx\n", iter, debug_regs[iter].read_dbgbvr_el1());
	printf("\n");
	return rc;
}

/*
 * Verify that the values of DBGWCRn_EL1 and DBGWVRn_EL1 registers are the same
 * as what was written previously.
 */
static test_result_t ns_check_wps(int num_wps, u_register_t expected_wp_addr[][MAX_WPS])
{
	test_result_t rc = TEST_RESULT_FAIL;

	uint8_t iter, wp_count, bank_no;

	bank_no = EXTRACT(MDSELR_EL1_BANK, read_mdselr_el1());
	wp_count = num_wps;

	for (uint8_t bank = 0U; bank <= bank_no; bank++) {

		write_mdselr_el1(INPLACE(MDSELR_EL1_BANK, bank));

		iter = (wp_count >= MAX_WPS) ? ((wp_count -= MAX_WPS), (MAX_WPS - 1)) : wp_count;

		printf("NS check WPS: Bank No= %d\n", bank);

		for(int i = 0; i <= iter; i++) {
			if (!ns_check_dbgwcr_el1(debug_regs[i].read_dbgwcr_el1()) && \
				(EXTRACT(DBGWVR_EL1_VA, debug_regs[i].read_dbgwvr_el1()) != \
				expected_wp_addr[bank][i])) {
				rc = TEST_RESULT_FAIL;
				break;
			}
		}
		rc = (bank == bank_no) ? TEST_RESULT_SUCCESS : TEST_RESULT_FAIL;
	}
	INFO("read_dbgwvr_el1[%d] = %lx\n", iter, debug_regs[iter].read_dbgwvr_el1());
	printf("\n");
	return rc;
}

/*
 * Write to the self-hosted debug registers in NS world. Enter Realm and write
 * random values to the self-hosted debug registers. Check that switching
 * between NS world/Realm does not corrupt the values in the self-hosted debug
 * registers.
 */
test_result_t host_realm_test_debug_save_restore(void)
{
	unsigned long num_bps, num_wps;
	u_register_t feature_flag;
	u_register_t rec_flag[1] = {RMI_RUNNABLE};
	struct realm realm;
	u_register_t expected_bp_addr[MAX_BANKS][MAX_BPS] = { 0 };
	u_register_t expected_wp_addr[MAX_BANKS][MAX_WPS] = { 0 };
	test_result_t test_rc = TEST_RESULT_SUCCESS;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	/* If BPs is 0xF, read DFR1 to find no of BPs supported by HW */
	num_bps = EXTRACT(ID_AA64DFR0_BRPs, read_id_aa64dfr0_el1());
	if (num_bps == MAX_BPS - 1UL) {
		num_bps = EXTRACT(ID_AA64DFR1_BRPs, read_id_aa64dfr1_el1());
		if (num_bps == 0UL) {
			num_bps = MAX_BPS - 1UL;
		}
	}

	/* If WPs is 0xF, read DFR1 to find no of BPs supported by HW */
	num_wps = EXTRACT(ID_AA64DFR0_WRPs, read_id_aa64dfr0_el1());

	if (num_wps == MAX_WPS - 1UL) {
		num_wps = EXTRACT(ID_AA64DFR1_WRPs, read_id_aa64dfr1_el1());
		if(num_wps == 0UL) {
			num_wps = MAX_WPS - 1UL;
		}
	}

	INFO("BPs and WPs supported in H/W is %ld and %ld\n", num_bps+1, num_wps+1);
	feature_flag = INPLACE(RMI_FEATURE_REGISTER_0_NUM_BPS, num_bps) |
		       INPLACE(RMI_FEATURE_REGISTER_0_NUM_WPS, num_wps);

	num_bps = BPS_TRAP_TEST - 1;
	num_wps = WPS_TRAP_TEST - 1;

	INFO("BP's and WP's requested by host is %ld and %ld\n", num_bps+1, num_wps+1);

	feature_flag = INPLACE(RMI_FEATURE_REGISTER_0_NUM_BPS, num_bps) |
		       INPLACE(RMI_FEATURE_REGISTER_0_NUM_WPS, num_wps);

	if (!host_create_activate_realm_payload(&realm,
					(u_register_t)REALM_IMAGE_BASE,
					feature_flag,RTT_MIN_LEVEL, rec_flag, 1U, 0U)) {
		return TEST_RESULT_FAIL;
	}

	if (!host_create_shared_mem(&realm)) {
		return TEST_RESULT_FAIL;
	}

	write_mdscr_el1(read_mdscr_el1() |
			MDSCR_EL1_TDCC_BIT |
			MDSCR_EL1_KDE_BIT  |
			MDSCR_EL1_MDE_BIT  |
			MDSCR_EL1_EMBWE_BIT);

	write_mdselr_el1(INPLACE(MDSELR_EL1_BANK, (SELECT_BITS(num_bps))));

	INFO("NS HOST: Write random values to the BP and WP regs\n");
	printf("\n");

	ns_set_bps(num_bps, expected_bp_addr);
	ns_set_wps(num_wps, expected_wp_addr);

	/*
	 * Enter Realm and write random values to the self-hosted debug
	 * registers. We will later check that there is no corruption of
	 * register values on NS or Realm side.
	 */
	if (!host_enter_realm_execute(&realm, REALM_DEBUG_FILL_REGS,
				      RMI_EXIT_HOST_CALL, 0)) {
		test_rc = TEST_RESULT_FAIL;
		goto destroy_realm;
	}

	INFO("NS HOST: Read and compare the values in BP and WP regs\n");
	printf("\n");

	/*
	 * Verify that the self-hosted debug registers have been saved and
	 * restored correctly.
	 */
	if ((ns_check_bps(num_bps, expected_bp_addr) != TEST_RESULT_SUCCESS) ||
	    (ns_check_wps(num_wps, expected_wp_addr) != TEST_RESULT_SUCCESS)) {
		test_rc = TEST_RESULT_FAIL;
		goto destroy_realm;
	}

	/*
	 * Enter Realm and verify that the self-hosted debug registers have been
	 * saved and restored correctly.
	 */
	if (!host_enter_realm_execute(&realm, REALM_DEBUG_CMP_REGS,
				      RMI_EXIT_HOST_CALL, 0)) {
		test_rc = TEST_RESULT_FAIL;
	}

destroy_realm:
	if (!host_destroy_realm(&realm)) {
		return TEST_RESULT_FAIL;
	}

	return test_rc;
}

test_result_t host_realm_check_shd_undef_abort(void)
{
	volatile unsigned long num_bps, num_wps;
	u_register_t feature_flag;
	u_register_t rec_flag[1] = {RMI_RUNNABLE};
	struct realm realm;
	bool realm_rc;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	num_bps = EXTRACT(ID_AA64DFR0_BRPs, read_id_aa64dfr0_el1());

	/* If BPs is 0xF, read DFR1 to find no of BPs supported by HW */
	if (num_bps == MAX_BPS - 1UL) {
		num_bps = EXTRACT(ID_AA64DFR1_BRPs, read_id_aa64dfr1_el1());
		if (num_bps == 0UL) {
			num_bps = MAX_BPS - 1UL;
		}
	}

	/* If WPs is 0xF, read DFR1 to find no of BPs supported by HW */
	num_wps = EXTRACT(ID_AA64DFR0_WRPs, read_id_aa64dfr0_el1());

	if (num_wps == MAX_WPS - 1UL) {
		num_wps = EXTRACT(ID_AA64DFR1_WRPs, read_id_aa64dfr1_el1());
		if(num_wps == 0UL) {
			num_wps = MAX_WPS - 1UL;
		}
	}

	num_wps = MIN_WPS_REQUESTED - 1;

	num_bps = MIN_BPS_REQUESTED - 1;

	INFO("BP's and WP'supported requested by NS Host is %ld and %ld\n", num_bps+1, num_wps+1);
	feature_flag = INPLACE(RMI_FEATURE_REGISTER_0_NUM_BPS, num_bps) |
		       INPLACE(RMI_FEATURE_REGISTER_0_NUM_WPS, num_wps);

	if (!host_create_activate_realm_payload(&realm,
					(u_register_t)REALM_IMAGE_BASE,
					feature_flag, RTT_MIN_LEVEL, rec_flag, 1U, 0U)) {
		return TEST_RESULT_FAIL;
	}

	if (!host_create_shared_mem(&realm)) {
		return TEST_RESULT_FAIL;
	}

	host_shared_data_set_host_val(&realm, 0, 0, HOST_ARG1_INDEX, num_bps);
	host_shared_data_set_host_val(&realm, 0, 0, HOST_ARG2_INDEX, num_wps);

	realm_rc = host_enter_realm_execute(&realm,
					    REALM_DEBUG_CHECK_SHD_UNDEF_ABORT,
					    RMI_EXIT_HOST_CALL,
					    0);

	if (!realm_rc || !host_destroy_realm(&realm)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}


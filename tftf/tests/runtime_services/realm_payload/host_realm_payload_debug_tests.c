/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
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

#define SET_EXPECTED_BP_ADDR(n) {	  \
	rand_addr = get_random_address(); \
	WRITE_DBGBVR_EL1(n, rand_addr);	  \
	expected_bp_addr[n] = rand_addr;  \
}

#define SET_BP(n) {							  \
	case (n):							  \
	write_dbgbcr##n##_el1(ns_set_dbgbcr_el1(read_dbgbcr##n##_el1())); \
	SET_EXPECTED_BP_ADDR(n);					  \
}

#define CHECK_BP(n) {								\
	case (n):								\
	if (!ns_check_dbgbcr_el1(read_dbgbcr##n##_el1()) ||			\
	    (EXTRACT(DBGBVR_EL1_VA, read_dbgbvr##n##_el1()) !=			\
							expected_bp_addr[n])) {	\
		rc = TEST_RESULT_FAIL;						\
		break;								\
	}									\
}

#define SET_EXPECTED_WP_ADDR(n) {	  \
	rand_addr = get_random_address(); \
	WRITE_DBGWVR_EL1(n, rand_addr);	  \
	expected_wp_addr[n] = rand_addr;  \
}

#define SET_WP(n) {							  \
	case (n):							  \
	write_dbgwcr##n##_el1(ns_set_dbgwcr_el1(read_dbgwcr##n##_el1())); \
	SET_EXPECTED_WP_ADDR(n);					  \
}

#define CHECK_WP(n) {								\
	case (n):								\
	if (!ns_check_dbgwcr_el1(read_dbgwcr##n##_el1()) ||			\
	    (EXTRACT(DBGWVR_EL1_VA, read_dbgwvr##n##_el1()) !=			\
							expected_wp_addr[n])) {	\
		rc = TEST_RESULT_FAIL;						\
		break;								\
	}									\
}

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
 * Verify that Realm has the same number of BPs/WPs as what the NS Host has
 * requested.
 *
 * Note that currently RMM assumes the NS Host will request the same number of
 * BPs/WPs as is available in the hardware. This test can be updated to check
 * different numbers of BPs/WPs once RMM is able to handle the NS Host
 * requesting fewer BPs/WPs than the hardware supports.
 */
test_result_t host_test_realm_num_bps_wps(void)
{
	unsigned long num_bps, num_wps;
	unsigned long num_bps_idreg1, num_wps_idreg1;
	unsigned long debugver;
	u_register_t feature_flag;
	u_register_t rec_flag[1] = {RMI_RUNNABLE};
	struct realm realm;
	bool realm_rc;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	num_bps = EXTRACT(ID_AA64DFR0_BRPs, read_id_aa64dfr0_el1());
	num_wps = EXTRACT(ID_AA64DFR0_WRPs, read_id_aa64dfr0_el1());
	debugver = EXTRACT(ID_AA64DFR0_DEBUG, read_id_aa64dfr0_el1());

	INFO("DEBUGVER = %lu\n", debugver);

	num_bps_idreg1 = EXTRACT(ID_AA64DFR1_BRPs, read_id_aa64dfr1_el1());
	num_wps_idreg1 = EXTRACT(ID_AA64DFR1_WRPs, read_id_aa64dfr1_el1());

	feature_flag = INPLACE(RMI_FEATURE_REGISTER_0_NUM_BPS, num_bps) |
		       INPLACE(RMI_FEATURE_REGISTER_0_NUM_WPS, num_wps);

	INFO("num of bps_idreg1 = %lu and num of wps_idreg1 = %lu\n", num_bps_idreg1, num_wps_idreg1);

	if (!host_create_activate_realm_payload(&realm,
					(u_register_t)REALM_IMAGE_BASE,
					feature_flag, RTT_MIN_LEVEL, rec_flag, 1U, 0UL)) {
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
static void ns_set_bps(int num_bps, u_register_t expected_bp_addr[])
{
	unsigned int rand_addr;

	switch (num_bps) {
	SET_BP(15);
	SET_BP(14);
	SET_BP(13);
	SET_BP(12);
	SET_BP(11);
	SET_BP(10);
	SET_BP(9);
	SET_BP(8);
	SET_BP(7);
	SET_BP(6);
	SET_BP(5);
	SET_BP(4);
	SET_BP(3);
	SET_BP(2);
	SET_BP(1);
	default:
	SET_BP(0);
	}
}

/*
 * Write random values to DBGWVRn_EL1 registers and set the values of
 * DBGWCRn_EL1 registers.
 */
static void ns_set_wps(int num_wps, u_register_t expected_wp_addr[])
{
	unsigned int rand_addr;

	switch (num_wps) {
	SET_WP(15);
	SET_WP(14);
	SET_WP(13);
	SET_WP(12);
	SET_WP(11);
	SET_WP(10);
	SET_WP(9);
	SET_WP(8);
	SET_WP(7);
	SET_WP(6);
	SET_WP(5);
	SET_WP(4);
	SET_WP(3);
	SET_WP(2);
	SET_WP(1);
	default:
	SET_WP(0);
	}
}

/*
 * Verify that the values of DBGBCRn_EL1 and DBGBVRn_EL1 registers are the same
 * as what was written previously.
 */
static test_result_t ns_check_bps(int num_bps, u_register_t expected_bp_addr[])
{
	test_result_t rc = TEST_RESULT_SUCCESS;

	switch (num_bps) {
	CHECK_BP(15);
	CHECK_BP(14);
	CHECK_BP(13);
	CHECK_BP(12);
	CHECK_BP(11);
	CHECK_BP(10);
	CHECK_BP(9);
	CHECK_BP(8);
	CHECK_BP(7);
	CHECK_BP(6);
	CHECK_BP(5);
	CHECK_BP(4);
	CHECK_BP(3);
	CHECK_BP(2);
	CHECK_BP(1);
	default:
	CHECK_BP(0);
	}

	return rc;
}

/*
 * Verify that the values of DBGWCRn_EL1 and DBGWVRn_EL1 registers are the same
 * as what was written previously.
 */
static test_result_t ns_check_wps(int num_wps, u_register_t expected_wp_addr[])
{
	test_result_t rc = TEST_RESULT_SUCCESS;

	switch (num_wps) {
	CHECK_WP(15);
	CHECK_WP(14);
	CHECK_WP(13);
	CHECK_WP(12);
	CHECK_WP(11);
	CHECK_WP(10);
	CHECK_WP(9);
	CHECK_WP(8);
	CHECK_WP(7);
	CHECK_WP(6);
	CHECK_WP(5);
	CHECK_WP(4);
	CHECK_WP(3);
	CHECK_WP(2);
	CHECK_WP(1);
	default:
	CHECK_WP(0);
	}

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
	u_register_t expected_bp_addr[MAX_BPS] = { 0 };
	u_register_t expected_wp_addr[MAX_WPS] = { 0 };
	test_result_t test_rc = TEST_RESULT_SUCCESS;

	SKIP_TEST_IF_RME_NOT_SUPPORTED_OR_RMM_IS_TRP();

	num_bps = EXTRACT(ID_AA64DFR0_BRPs, read_id_aa64dfr0_el1());
	num_wps = EXTRACT(ID_AA64DFR0_WRPs, read_id_aa64dfr0_el1());

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
			MDSCR_EL1_KDE_BIT |
			MDSCR_EL1_MDE_BIT);

	INFO("NS: Write random values\n");

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

	INFO("NS: Read and compare\n");

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

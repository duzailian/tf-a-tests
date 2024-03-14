/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <arch_features.h>
#include <arch_helpers.h>
#include <debug.h>
#include <self_hosted_debug_helpers.h>
#include <realm_tests.h>
#include <stdlib.h>

#include <host_shared_data.h>

#define SET_EXPECTED_BP_ADDR(n) {	  \
	rand_addr = get_random_address(); \
	WRITE_DBGBVR_EL1(n, rand_addr);	  \
	expected_bp_addr[n] = rand_addr;  \
}

#define SET_BP(n) {							  \
	case (n):							  \
	write_dbgbcr##n##_el1(rl_set_dbgbcr_el1(read_dbgbcr##n##_el1())); \
	SET_EXPECTED_BP_ADDR(n);					  \
}

#define CHECK_BP(n) {								\
	case (n):								\
	if (!rl_check_dbgbcr_el1(read_dbgbcr##n##_el1()) ||			\
	    (EXTRACT(DBGBVR_EL1_VA, read_dbgbvr##n##_el1()) !=			\
							expected_bp_addr[n])) { \
		rc = false;							\
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
	write_dbgwcr##n##_el1(rl_set_dbgwcr_el1(read_dbgwcr##n##_el1())); \
	SET_EXPECTED_WP_ADDR(n);					  \
}

#define CHECK_WP(n) {								\
	case (n):								\
	if (!rl_check_dbgwcr_el1(read_dbgwcr##n##_el1()) ||			\
	    (EXTRACT(DBGWVR_EL1_VA, read_dbgwvr##n##_el1()) !=			\
							expected_wp_addr[n])) {	\
		rc = false;							\
		break;								\
	}									\
}

static u_register_t expected_bp_addr[MAX_BPS];
static u_register_t expected_wp_addr[MAX_WPS];

/*
 * Set DBGBCRn_EL1 fields HMC, PMC, SSC and SSCE such that breakpoint n will
 * generate Breakpoint exceptions only in RL Security state.
 */
static u_register_t rl_set_dbgbcr_el1(u_register_t current_val)
{
	u_register_t new_val = set_dbgbcr_el1(current_val) |
			       DBGBCR_EL1_SSCE_BIT |
			       INPLACE(DBGBCR_EL1_SSC, 1UL) |
			       INPLACE(DBGBCR_EL1_PMC, 1UL);

	new_val &= ~DBGBCR_EL1_HMC_BIT;

	return new_val;
}

/*
 * Verify that the value of DBGBCRn_EL1 is the same as what was written
 * previously.
 */
static bool rl_check_dbgbcr_el1(u_register_t reg)
{
	u_register_t expected_dbgbcr_el1 = rl_set_dbgbcr_el1(0UL);

	if (reg != expected_dbgbcr_el1) {
		return false;
	}

	return true;
}

/*
 * Set DBGWCRn_EL1 fields HMC, PAC, SSC and SSCE such that watchpoint n will
 * generate Watchpoint exceptions only in RL Security state.
 */
static u_register_t rl_set_dbgwcr_el1(u_register_t current_val)
{
	u_register_t new_val = set_dbgwcr_el1(current_val) |
			       DBGWCR_EL1_SSCE_BIT |
			       INPLACE(DBGWCR_EL1_SSC, 1UL) |
			       INPLACE(DBGWCR_EL1_PAC, 1UL);

	new_val &= ~DBGWCR_EL1_HMC_BIT;

	return new_val;
}

/*
 * Verify that the value of DBGWCRn_EL1 is the same as what was written
 * previously.
 */
static bool rl_check_dbgwcr_el1(u_register_t reg)
{
	u_register_t expected_dbgwcr_el1 = rl_set_dbgwcr_el1(0UL);

	if (reg != expected_dbgwcr_el1) {
		return false;
	}

	return true;
}

/*
 * Verify that the Realm has the same number of available BPs/WPs as the Host
 * requested.
 */
bool test_realm_debug_num_bps_wps(void)
{
	unsigned long num_bps, num_wps;
	unsigned long num_bps_host, num_wps_host;

	num_bps = EXTRACT(ID_AA64DFR0_BRPs, read_id_aa64dfr0_el1());
	num_wps = EXTRACT(ID_AA64DFR0_WRPs, read_id_aa64dfr0_el1());

	INFO("Num of bps extracted = %d and wps extracted = %d\n", num_bps, num_wps);

	INFO("Realm: Verify number of BPs and WPs available\n");

	num_bps_host = realm_shared_data_get_my_host_val(HOST_ARG1_INDEX);
	num_wps_host = realm_shared_data_get_my_host_val(HOST_ARG2_INDEX);

	if ((num_bps != num_bps_host) || (num_wps != num_wps_host)) {
		return false;
	}

	return true;
}

/*
 * Write random values to DBGBVRn_EL1 registers and set the values of
 * DBGBCRn_EL1 registers.
 */
static void rl_set_bps(unsigned long num_bps)
{
	unsigned long rand_addr;

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
static void rl_set_wps(unsigned long num_wps)
{
	unsigned long rand_addr;

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
 * Write random values to the self-hosted debug registers and keep a record of
 * these values.
 */
void test_realm_debug_fill_regs(void)
{
	unsigned long num_bps, num_wps;

	num_bps = EXTRACT(ID_AA64DFR0_BRPs, read_id_aa64dfr0_el1());
	num_wps = EXTRACT(ID_AA64DFR0_WRPs, read_id_aa64dfr0_el1());

	write_mdscr_el1(read_mdscr_el1() |
			MDSCR_EL1_TDCC_BIT |
			MDSCR_EL1_KDE_BIT |
			MDSCR_EL1_MDE_BIT);

	INFO("Realm: Write random values\n");

	rl_set_bps(num_bps);
	rl_set_wps(num_wps);
}

/*
 * Verify that the values of DBGBCRn_EL1 and DBGBVRn_EL1 registers are the same
 * as what was written previously.
 */
static bool rl_check_bps(unsigned long num_bps)
{
	bool rc = true;

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
static bool rl_check_wps(unsigned long num_wps)
{
	bool rc = true;

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
 * Read the self-hosted debug registers and compare the values to the values
 * that were previously written.
 */
bool test_realm_debug_cmp_regs(void)
{
	unsigned long num_bps, num_wps;

	num_bps = EXTRACT(ID_AA64DFR0_BRPs, read_id_aa64dfr0_el1());
	num_wps = EXTRACT(ID_AA64DFR0_WRPs, read_id_aa64dfr0_el1());

	INFO("Realm: Read and compare\n");

	return (rl_check_bps(num_bps) && rl_check_wps(num_wps));
}

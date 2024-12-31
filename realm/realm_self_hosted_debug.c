/*
 * Copyright (c) 2024-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>

#include <arch.h>
#include <arch_features.h>
#include <arch_helpers.h>
#include <debug.h>
#include <host_shared_data.h>
#include <realm_tests.h>
#include <self_hosted_debug_helpers.h>

typedef struct {
	long unsigned int (*read_dbgbcr_el1)(void);
	long unsigned int (*read_dbgbvr_el1)(void);
	long unsigned int (*read_dbgwcr_el1)(void);
	long unsigned int (*read_dbgwvr_el1)(void);
	void (*write_dbgbcr_el1)(long unsigned int);
	void (*write_dbgbvr_el1)(long unsigned int);
	void (*write_dbgwcr_el1)(long unsigned int);
	void (*write_dbgwvr_el1)(long unsigned int);
} realm_ctx_fnptr_t;

#define REALM_CTX_FNPTRS(n)									\
	{read_dbgbcr##n##_el1, read_dbgbvr##n##_el1, read_dbgwcr##n##_el1, read_dbgwvr##n##_el1, \
	write_dbgbcr##n##_el1, write_dbgbvr##n##_el1, write_dbgwcr##n##_el1, write_dbgwvr##n##_el1}

realm_ctx_fnptr_t realm_ctx_regs[MAX_BPS] = {
	REALM_CTX_FNPTRS(0),
	REALM_CTX_FNPTRS(1),
	REALM_CTX_FNPTRS(2),
	REALM_CTX_FNPTRS(3),
	REALM_CTX_FNPTRS(4),
	REALM_CTX_FNPTRS(5),
	REALM_CTX_FNPTRS(6),
	REALM_CTX_FNPTRS(7),
	REALM_CTX_FNPTRS(8),
	REALM_CTX_FNPTRS(9),
	REALM_CTX_FNPTRS(10),
	REALM_CTX_FNPTRS(11),
	REALM_CTX_FNPTRS(12),
	REALM_CTX_FNPTRS(13),
	REALM_CTX_FNPTRS(14),
	REALM_CTX_FNPTRS(15)
};

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
	if (num_bps == (MAX_BPS - 1)) {
		num_bps = EXTRACT(ID_AA64DFR1_BRPs, read_id_aa64dfr1_el1());
		if (num_bps == 0UL) {
			num_bps = MAX_BPS - 1;
		}
	}

	num_wps = EXTRACT(ID_AA64DFR0_WRPs, read_id_aa64dfr0_el1());
	if (num_wps == (MAX_WPS - 1)) {
		num_wps = EXTRACT(ID_AA64DFR1_WRPs, read_id_aa64dfr1_el1());
		if (num_wps == 0UL) {
			num_wps = MAX_WPS - 1;
		}
	}

	INFO("Realm: BPs extracted = %d and WPs extracted = %d\n", num_bps+1, num_wps+1);

	num_bps_host = realm_shared_data_get_my_host_val(HOST_ARG1_INDEX);
	num_wps_host = realm_shared_data_get_my_host_val(HOST_ARG2_INDEX);

	INFO("Realm: BPs requested = %d and WPs requested = %d\n", \
						num_bps_host+1, num_wps_host+1);

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

	for (int i = 0; i <= num_bps ; i++) {
		realm_ctx_regs[i].write_dbgbcr_el1(rl_set_dbgbcr_el1 \
				(realm_ctx_regs[i].read_dbgbcr_el1()));
		rand_addr = get_random_address();
		realm_ctx_regs[i].write_dbgbvr_el1(INPLACE(DBGBVR_EL1_VA, rand_addr));
		expected_bp_addr[i] = rand_addr;
	}

}

/*
 * Write random values to DBGWVRn_EL1 registers and set the values of
 * DBGWCRn_EL1 registers.
 */
static void rl_set_wps(unsigned long num_wps)
{
	unsigned long rand_addr;

	for (int i = 0; i <= num_wps ; i++) {
		realm_ctx_regs[i].write_dbgwcr_el1(rl_set_dbgwcr_el1 \
				(realm_ctx_regs[i].read_dbgwcr_el1()));
		rand_addr = get_random_address();
		realm_ctx_regs[i].write_dbgwvr_el1(INPLACE(DBGWVR_EL1_VA, rand_addr));
		expected_wp_addr[i] = rand_addr;
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
	if (num_bps == (MAX_BPS-1)) {
		num_bps = EXTRACT(ID_AA64DFR1_BRPs, read_id_aa64dfr1_el1());
		if (num_bps == 0UL) {
			num_bps = MAX_BPS - 1;
		}
	}

	num_wps = EXTRACT(ID_AA64DFR0_WRPs, read_id_aa64dfr0_el1());
	if (num_wps == (MAX_WPS-1)) {
		num_wps = EXTRACT(ID_AA64DFR1_WRPs, read_id_aa64dfr1_el1());
		if (num_wps == 0UL) {
			num_wps = MAX_WPS - 1;
		}
	}

	INFO("Num of bps extracted = %d and wps extracted = %d\n", num_bps+1, num_wps+1);

	write_mdscr_el1(read_mdscr_el1() |
			MDSCR_EL1_TDCC_BIT |
			MDSCR_EL1_KDE_BIT |
			MDSCR_EL1_MDE_BIT);

	INFO("Realm: Write random values to BP and WP regs\n");

	rl_set_bps(num_bps);
	rl_set_wps(num_wps);
}

/*
 * Verify that the values of DBGBCRn_EL1 and DBGBVRn_EL1 registers are the same
 * as what was written previously.
 */
static bool rl_check_bps(unsigned long num_bps)
{
	bool rc = false;

	if (num_bps != 0UL) {
		for (int i = 0; i <= num_bps; i++) {
			printf(" i = %d\n", i);
			if (!rl_check_dbgbcr_el1(realm_ctx_regs[i].read_dbgbcr_el1())		\
				&& (EXTRACT(DBGBVR_EL1_VA, realm_ctx_regs[i].read_dbgbvr_el1()) \
				!= expected_bp_addr[i])) {
				rc = false;
				break;
			}
		}
		rc = true;
	}
	return rc;
}

/*
 * Verify that the values of DBGWCRn_EL1 and DBGWVRn_EL1 registers are the same
 * as what was written previously.
 */
static bool rl_check_wps(unsigned long num_wps)
{
	bool rc = false;

	if (num_wps != 0UL) {
		for (int i = 0; i <= num_wps; i++) {
			printf(" i = %d\n", i);
			if (!rl_check_dbgwcr_el1(realm_ctx_regs[i].read_dbgwcr_el1()) &&	\
				(EXTRACT(DBGWVR_EL1_VA, realm_ctx_regs[i].read_dbgwvr_el1())	\
				!= expected_wp_addr[i])) {
				rc = false;
				break;
			}
		}
		rc = true;
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
	if (num_bps == (MAX_BPS - 1)) {
		num_bps = EXTRACT(ID_AA64DFR1_BRPs, read_id_aa64dfr1_el1());
		if (num_bps == 0UL) {
			num_bps = MAX_BPS - 1;
		}
	}

	num_wps = EXTRACT(ID_AA64DFR0_WRPs, read_id_aa64dfr0_el1());
	if (num_wps == (MAX_WPS - 1)) {
		num_wps = EXTRACT(ID_AA64DFR1_WRPs, read_id_aa64dfr1_el1());
		if (num_wps == 0UL) {
			num_wps = MAX_WPS - 1;
		}
	}

	INFO("Num of bps extracted = %d and wps extracted = %d\n", num_bps+1, num_wps+1);

	INFO("Realm: Read and compare\n");

	return (rl_check_bps(num_bps) && rl_check_wps(num_wps));
}

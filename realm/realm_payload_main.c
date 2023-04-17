/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>

#include <arch_features.h>
#include <debug.h>
#include <fpu.h>
#include <host_realm_helper.h>
#include <host_shared_data.h>
#include <pauth.h>
#include "realm_def.h"
#include <realm_rsi.h>
#include <realm_tests.h>
#include <tftf_lib.h>
#include <sync.h>

static fpu_reg_state_t fpu_temp_rl;
/* Number of ARMv8.3-PAuth keys */
#define NUM_KEYS        5

static const char * const key_name[] = {"IA", "IB", "DA", "DB", "GA"};

static uint128_t pauth_keys_before[NUM_KEYS];
static uint128_t pauth_keys_after[NUM_KEYS];

static volatile bool sync_exception_triggered;
/*
 * This function reads sleep time in ms from shared buffer and spins PE
 * in a loop for that time period.
 */
static void realm_sleep_cmd(void)
{
	uint64_t sleep = realm_shared_data_get_host_val(HOST_SLEEP_INDEX);

	realm_printf("Realm: going to sleep for %llums\n", sleep);
	waitms(sleep);
}

static bool exception_handler(void)
{
	sync_exception_triggered = true;
	pauth_disable();
	u_register_t lr = read_elr_el1();

	/* Check for PAuth exception. */
	if (lr & 0xF000000000000000U) {
		rsi_exit_to_host(HOST_CALL_EXIT_SUCCESS_CMD);
	}
	rsi_exit_to_host(HOST_CALL_EXIT_FAILED_CMD);
	/*Does not return. */
	return false;
}

void dummy_func(void)
{
	realm_printf("Realm: shouldn't reach here.\n");
	rsi_exit_to_host(HOST_CALL_EXIT_FAILED_CMD);
}

/* Check if ARMv8.3-PAuth key is enabled */
static bool is_pauth_key_enabled(uint64_t key_bit)
{
	return ((read_sctlr_el1() & key_bit) != 0U);
}

bool test_realm_pauth_fault(void)
{
	u_register_t ptr = (u_register_t)&dummy_func;

	if (!is_armv8_3_pauth_present() || !is_pauth_key_enabled(SCTLR_EnIA_BIT)) {
		return false;
	}

	register_custom_sync_exception_handler(exception_handler);
	realm_printf("Realm: overwrite LR to generate fault.\n");
	__asm__("mov x17, x30; "
			"mov x30, %0; "       /* Overwite LR. */
			"isb; "
			"autiasp; "
			"ret; "               /* Fault on return.  */
			:
			: "r"(ptr));

	/* Returns after exception handler is run. */
	return sync_exception_triggered;
}

static bool compare_pauth_keys(void)
{
	bool result = true;

	for (unsigned int i = 0; i < NUM_KEYS; ++i) {
		if (pauth_keys_before[i] != pauth_keys_after[i]) {
			ERROR("AP%sKey_EL1 read 0x%llx:%llx "
			"expected 0x%llx:%llx\n", key_name[i],
			(uint64_t)(pauth_keys_after[i] >> 64),
			(uint64_t)(pauth_keys_after[i]),
			(uint64_t)(pauth_keys_before[i] >> 64),
			(uint64_t)(pauth_keys_before[i]));

			result = false;
		}
	}
	return result;
}

/*
 * Program or read ARMv8.3-PAuth keys (if already enabled)
 * and store them in <pauth_keys_before> buffer
 */
static void set_store_pauth_keys(void)
{
	uint128_t plat_key;

	memset(pauth_keys_before, 0, NUM_KEYS * sizeof(uint128_t));

	if (is_armv8_3_pauth_apa_api_apa3_present()) {
		if (is_pauth_key_enabled(SCTLR_EnIA_BIT)) {
			/* Read APIAKey_EL1 */
			plat_key = read_apiakeylo_el1() |
				((uint128_t)(read_apiakeyhi_el1()) << 64);
			INFO("EnIA is set\n");
		} else {
			/* Program APIAKey_EL1 */
			plat_key = init_apkey();
			write_apiakeylo_el1((uint64_t)plat_key);
			write_apiakeyhi_el1((uint64_t)(plat_key >> 64));
		}
		pauth_keys_before[0] = plat_key;

		if (is_pauth_key_enabled(SCTLR_EnIB_BIT)) {
			/* Read APIBKey_EL1 */
			plat_key = read_apibkeylo_el1() |
				((uint128_t)(read_apibkeyhi_el1()) << 64);
			INFO("EnIB is set\n");
		} else {
			/* Program APIBKey_EL1 */
			plat_key = init_apkey();
			write_apibkeylo_el1((uint64_t)plat_key);
			write_apibkeyhi_el1((uint64_t)(plat_key >> 64));
		}
		pauth_keys_before[1] = plat_key;

		if (is_pauth_key_enabled(SCTLR_EnDA_BIT)) {
			/* Read APDAKey_EL1 */
			plat_key = read_apdakeylo_el1() |
				((uint128_t)(read_apdakeyhi_el1()) << 64);
			INFO("EnDA is set\n");
		} else {
			/* Program APDAKey_EL1 */
			plat_key = init_apkey();
			write_apdakeylo_el1((uint64_t)plat_key);
			write_apdakeyhi_el1((uint64_t)(plat_key >> 64));
		}
		pauth_keys_before[2] = plat_key;

		if (is_pauth_key_enabled(SCTLR_EnDB_BIT)) {
			/* Read APDBKey_EL1 */
			plat_key = read_apdbkeylo_el1() |
				((uint128_t)(read_apdbkeyhi_el1()) << 64);
			INFO("EnDB is set\n");
		} else {
			/* Program APDBKey_EL1 */
			plat_key = init_apkey();
			write_apdbkeylo_el1((uint64_t)plat_key);
			write_apdbkeyhi_el1((uint64_t)(plat_key >> 64));
		}
		pauth_keys_before[3] = plat_key;
	}

	/*
	 * It is safe to assume that Generic Pointer authentication code key
	 * APGAKey_EL1 can be re-programmed, as this key is not set in
	 * TF-A Test suite and PACGA instruction is not used.
	 */
	if (is_armv8_3_pauth_gpa_gpi_gpa3_present()) {
		/* Program APGAKey_EL1 */
		plat_key = init_apkey();
		write_apgakeylo_el1((uint64_t)plat_key);
		write_apgakeyhi_el1((uint64_t)(plat_key >> 64));
		pauth_keys_before[4] = plat_key;
	}

	isb();
}

/*
 * Read ARMv8.3-PAuth keys and store them in
 * <pauth_keys_after> buffer
 */
static void read_pauth_keys(void)
{
	memset(pauth_keys_after, 0, NUM_KEYS * sizeof(uint128_t));

	if (is_armv8_3_pauth_apa_api_apa3_present()) {
		/* Read APIAKey_EL1 */
		pauth_keys_after[0] = read_apiakeylo_el1() |
			((uint128_t)(read_apiakeyhi_el1()) << 64);

		/* Read APIBKey_EL1 */
		pauth_keys_after[1] = read_apibkeylo_el1() |
			((uint128_t)(read_apibkeyhi_el1()) << 64);

		/* Read APDAKey_EL1 */
		pauth_keys_after[2] = read_apdakeylo_el1() |
			((uint128_t)(read_apdakeyhi_el1()) << 64);

		/* Read APDBKey_EL1 */
		pauth_keys_after[3] = read_apdbkeylo_el1() |
			((uint128_t)(read_apdbkeyhi_el1()) << 64);
	}

	if (is_armv8_3_pauth_gpa_gpi_gpa3_present()) {
		/* Read APGAKey_EL1 */
		pauth_keys_after[4] = read_apgakeylo_el1() |
			((uint128_t)(read_apgakeyhi_el1()) << 64);
	}
}

/* Test execution of ARMv8.3-PAuth instructions */
static void test_pauth_instructions(void)
{
	/* Pointer authentication instructions */
	__asm__ volatile (
			"paciasp\n"
			"autiasp\n"
			"paciasp\n"
			"xpaclri");
}

/*
 * TF-A is expected to allow access to key registers from lower EL's,
 * reading the keys excercises this, on failure this will trap to
 * EL3 and crash.
 */
bool test_realm_pauth_cmd(void)
{
	test_pauth_instructions();
	set_store_pauth_keys();

	/*Trap to RMM and check if keys are preserved. */
	if (!is_armv8_3_pauth_present())
		return false;
	read_pauth_keys();
	return compare_pauth_keys();
}

/*
 * This function requests RSI/ABI version from RMM.
 */
static void realm_get_rsi_version(void)
{
	u_register_t version;

	version = rsi_get_version();
	if (version == (u_register_t)SMC_UNKNOWN) {
		realm_printf("SMC_RSI_ABI_VERSION failed (%ld)", (long)version);
		return;
	}

	realm_printf("RSI ABI version %u.%u (expected: %u.%u)",
	RSI_ABI_VERSION_GET_MAJOR(version),
	RSI_ABI_VERSION_GET_MINOR(version),
	RSI_ABI_VERSION_GET_MAJOR(RSI_ABI_VERSION),
	RSI_ABI_VERSION_GET_MINOR(RSI_ABI_VERSION));
}

/*
 * This is the entry function for Realm payload, it first requests the shared buffer
 * IPA address from Host using HOST_CALL/RSI, it reads the command to be executed,
 * performs the request, and returns to Host with the execution state SUCCESS/FAILED
 *
 * Host in NS world requests Realm to execute certain operations using command
 * depending on the test case the Host wants to perform.
 */
__attribute__((noreturn)) void realm_payload_main(void)
{
	while (1) {
		bool test_succeed = false;

		realm_set_shared_structure((host_shared_data_t *)rsi_get_ns_buffer());

		if (realm_get_shared_structure() != NULL) {
			uint8_t cmd = realm_shared_data_get_realm_cmd();

			switch (cmd) {
			case REALM_SLEEP_CMD:
				realm_sleep_cmd();
				test_succeed = true;
				break;
			case REALM_PAUTH_CMD:
				test_succeed = test_realm_pauth_cmd();
				break;
			case REALM_PAUTH_FAULT:
				test_succeed = test_realm_pauth_fault();
				break;
			case REALM_GET_RSI_VERSION:
				realm_get_rsi_version();
				test_succeed = true;
				break;
			case REALM_PMU_CYCLE:
				test_succeed = test_pmuv3_cycle_works_realm();
				break;
			case REALM_PMU_EVENT:
				test_succeed = test_pmuv3_event_works_realm();
				break;
			case REALM_PMU_PRESERVE:
				test_succeed = test_pmuv3_rmm_preserves();
				break;
			case REALM_PMU_INTERRUPT:
				test_succeed = test_pmuv3_overflow_interrupt();
				break;
			case REALM_REQ_FPU_FILL_CMD:
				fpu_state_fill_regs_and_template(&fpu_temp_rl);
				test_succeed = true;
				break;
			case REALM_REQ_FPU_CMP_CMD:
				test_succeed = fpu_state_compare_template(&fpu_temp_rl);
				break;
			default:
				realm_printf("%s() invalid cmd %u\n", __func__, cmd);
				break;
			}
		}

		if (test_succeed) {
			rsi_exit_to_host(HOST_CALL_EXIT_SUCCESS_CMD);
		} else {
			rsi_exit_to_host(HOST_CALL_EXIT_FAILED_CMD);
		}
	}
}

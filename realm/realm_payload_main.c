/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>

#include <arch_features.h>
#include <debug.h>
#include <host_realm_helper.h>
#include <host_shared_data.h>
#include <pauth.h>
#include "realm_def.h"
#include <realm_rsi.h>
#include <realm_tests.h>
#include <tftf_lib.h>
#include <sync.h>

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
	realm_printf("Realm: shouldn't reach here. \n");
	rsi_exit_to_host(HOST_CALL_EXIT_FAILED_CMD);
}

bool realm_pauth_fault(void)
{
	u_register_t ptr = (u_register_t)&dummy_func;

	if (!is_armv8_3_pauth_present()) {
		return false;
	}
	register_custom_sync_exception_handler(exception_handler);
	pauth_init_enable();
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

static bool realm_pauth_cmd(void)
{
	uint64_t key_lo;
	uint64_t key_hi;
	uint64_t id_aa64isar1_el1;

	if (!is_armv8_3_pauth_present()) {
		return false;
	}
	pauth_init_enable();
	write_apiakeylo_el1(0xABCDU);
	write_apiakeyhi_el1(0xABCDU);
        key_lo = read_apiakeylo_el1();
        key_hi = read_apiakeyhi_el1();
	if ((key_lo != 0xABCDU) || (key_hi != 0xABCDU)) {
		return false;
	}
	return true;
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
void realm_payload_main(void)
{
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
                        test_succeed = realm_pauth_cmd();
                        break;
		case REALM_PAUTH_FAULT:
			test_succeed = realm_pauth_fault();
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

/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>

#include <debug.h>
#include <host_realm_helper.h>
#include <host_shared_data.h>
#include <lib/extensions/fpu.h>
#include "realm_def.h"
#include <realm_rsi.h>
#include <tftf_lib.h>

#define SIMD_REALM_VALUE	0x33U
#define FPCR_REALM_VALUE	0x75F9500U
#define FPSR_REALM_VALUE	0x88000097U

/*
 * This function reads sleep time in ms from shared buffer and spins PE in a loop
 * for that time period.
 */
static void realm_sleep_cmd(void)
{
	uint64_t sleep = realm_shared_data_get_host_val(HOST_SLEEP_INDEX);

	INFO("REALM_PAYLOAD: Realm payload going to sleep for %llums\n", sleep);
	waitms(sleep);
}

/*
 * This function request RSI/ABI version from RMM.
 */
static void realm_get_rsi_version(void)
{
	u_register_t version;

	version = rsi_get_version();
	if (version == (u_register_t)SMC_UNKNOWN) {
		ERROR("SMC_RSI_ABI_VERSION failed (%ld)", (long)version);
		return;
	}

	INFO("RSI ABI version %u.%u (expected: %u.%u)",
	RSI_ABI_VERSION_GET_MAJOR(version),
	RSI_ABI_VERSION_GET_MINOR(version),
	RSI_ABI_VERSION_GET_MAJOR(RSI_ABI_VERSION),
	RSI_ABI_VERSION_GET_MINOR(RSI_ABI_VERSION));
}

/*
 * Fill FPU state(SIMD vectors, FPCR, FPSR) in realm world with a template values,
 * SIMD_REALM_VALUE, FPCR_REALM_VALUE, FPSR_REALM_VALUE to be distinguished
 * from the value in the normal and secure worlds.
 */
static void realm_req_fpu_fill_cmd(void)
{
	fpu_reg_state_t fpu_state_send;
	fpu_state_set(&fpu_state_send, SIMD_REALM_VALUE, FPCR_REALM_VALUE, FPSR_REALM_VALUE);
	fill_fpu_state_registers(&fpu_state_send);
}

/*
 * compare FPU reg state from secure world side with the previously loaded values.
 */
static bool realm_req_fpu_cmp_cmd(void)
{
	fpu_reg_state_t fpu_state_send, fpu_state_receive;

	fpu_state_set(&fpu_state_send, SIMD_REALM_VALUE, FPCR_REALM_VALUE, FPSR_REALM_VALUE);
	fpu_state_set(&fpu_state_receive, 0, 0, 0);
	/* read FPU/SIMD state registers values.*/
	read_fpu_state_registers(&fpu_state_receive);
	/* compare.*/
	if (memcmp((uint8_t *)&fpu_state_send,
			(uint8_t *)&fpu_state_receive,
			sizeof(fpu_reg_state_t)) != 0) {
			ERROR("REALM_PAYLOAD: realm_req_fpu_cmp_cmd faild\n");
			return false;
	}
	else {
		return true;
	}

}

/*
 * This is the entry function for Realm payload, it first requests the shared buffer
 * IPA address from Host using HOST_CALL/RSI, it reads the command to be executed,
 * perform the request, and return to Host with the execution state SUCCESS/FAILED
 *
 * Host in NS world requests Realm to execute certain operations using command
 * depending on the test case the Host want to perform.
 */
void realm_payload_main(void)
{
	uint8_t cmd = 0U;
	bool test_succeed = false;

	realm_set_shared_structure((host_shared_data_t *)rsi_get_ns_buffer());
	if (realm_get_shared_structure() != NULL) {
		cmd = realm_shared_data_get_realm_cmd();
		switch (cmd) {
		case REALM_SLEEP_CMD:
			realm_sleep_cmd();
			test_succeed = true;
			break;
		case REALM_GET_RSI_VERSION:
			realm_get_rsi_version();
			test_succeed = true;
			break;
		case REALM_REQ_FPU_FILL_CMD:
			realm_req_fpu_fill_cmd();
			test_succeed = true;
			break;
		case REALM_REQ_FPU_CMP_CMD:
			test_succeed = realm_req_fpu_cmp_cmd();
			break;
		default:
			INFO("REALM_PAYLOAD: %s invalid cmd=%hhu", __func__, cmd);
			break;
		}
	}

	if (test_succeed) {
		rsi_exit_to_host(HOST_CALL_EXIT_SUCCESS_CMD);
	} else {
		rsi_exit_to_host(HOST_CALL_EXIT_FAILED_CMD);
	}
}
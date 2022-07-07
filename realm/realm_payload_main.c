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
#include <lib/extensions/sve.h>
#include "realm_def.h"
#include <realm_rsi.h>
#include <tftf_lib.h>

#define SIMD_REALM_VALUE	0x33U
#define SVE_Z_REALM_VALUE	0x55U
#define SVE_P_REALM_VALUE	0x88U
#define SVE_FFR_REALM_VALUE	0xbbU

/* Store the template FPU registers here. */
/* ToDO support concurrent CPU. */
fpu_reg_state_t fpu_temp;
struct sve_state sve;
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
 * This function requests RSI/ABI version from RMM.
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
 * This is the entry function for Realm payload, it first requests the shared buffer
 * IPA address from Host using HOST_CALL/RSI, it reads the command to be executed,
 * performs the request, and returns to Host with the execution state SUCCESS/FAILED
 *
 * Host in NS world requests Realm to execute certain operations using command
 * depending on the test case the Host wants to perform.
 */
void realm_payload_main(void)
{
	uint8_t cmd = 0U;
	bool test_succeed = false;
	uint32_t zcr_elx;

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
			fpu_temp = fpu_state_write_template(SIMD_REALM_VALUE);
			test_succeed = true;
			break;
		case REALM_REQ_FPU_CMP_CMD:
			test_succeed = fpu_state_compare_template(&fpu_temp);
			break;
		case REALM_REQ_SVE_FILL_CMD:
			zcr_elx = realm_shared_data_get_host_val(HOST_SVE_VL_INDEX);
			sve = sve_state_write_template(SVE_Z_REALM_VALUE,
					SVE_P_REALM_VALUE,
					zcr_elx,
					zcr_elx,
					SVE_FFR_REALM_VALUE);
			test_succeed = true;
			break;
		case REALM_REQ_SVE_CMP_CMD:
			zcr_elx = realm_shared_data_get_host_val(HOST_SVE_VL_INDEX);
			test_succeed = sve_state_compare_template(&sve);
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

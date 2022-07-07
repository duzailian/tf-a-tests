/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>

#include <debug.h>
#include <fpu.h>
#include <host_realm_mem_layout.h>
#include <host_realm_helper.h>
#include <host_shared_data.h>
#include <tftf_lib.h>
#include <realm_rsi.h>
#include <realm_payload_test.h>
#include <lib/extensions/fpu.h>
#include <lib/extensions/sve.h>

#include "realm_def.h"

static struct sve_state sve_state_send, sve_state_receive;

/**
 *   @brief    - C entry function for realm payload
 *   @param    - void
 *   @return   - void (returns to Host)
 **/
static void realm_sleep_cmd(void)
{
	uint64_t sleep = realm_shared_data_get_host_val(1);
	INFO("REALM_PAYLOAD: Realm payload going to sleep for %llums\n", sleep);
	waitms(sleep);
}

static void realm_get_version_cmd(void)
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
 * Fill FPU state(SIMD vectors, FPCR, FPSR) from secure
 * world side with a unique value. SIMD_REALM_VALUE, FPCR_REALM_VALUE,
 * FPSR_REALM_VALUE are just a dummy values to be distinguished
 * from the value in the normal/realm worlds .
 */
static void realm_req_fpu_fill_cmd(void)
{
	fpu_reg_state_t fpu_state_send;
	fpu_state_set(&fpu_state_send, SIMD_REALM_VALUE, FPCR_REALM_VALUE, FPSR_REALM_VALUE);
	fill_fpu_state_registers(&fpu_state_send);
}

/*
 * Fill SVE state(Z/P vectors, FPCR, FPSR) from secure
 */
static void realm_req_sve_fill_cmd(void)
{
	uint32_t zcr_el1 = realm_shared_data_get_host_val(1);
	memset(&sve_state_send,0x0,sizeof(sve_state_send));
	sve_state_set(&sve_state_send,
			SVE_Z_REALM_VALUE,
			SVE_P_REALM_VALUE,
			FPSR_REALM_VALUE,
			FPCR_REALM_VALUE,
			zcr_el1,
			zcr_el1,
			SVE_FFR_REALM_VALUE);
	write_sve_state(&sve_state_send);
}
/*
 * compare FPU reg state from secure world side with the previously loaded values.
 */
static bool realm_req_fpu_cmp_cmd(void)
{
	fpu_reg_state_t fpu_state_send, fpu_state_receive;
	/*  */
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
 * compare SVE reg state from secure world side with the previously loaded values.
 */
static bool realm_req_sve_cmp_cmd(void)
{
	int index = -1;
	uint32_t zcr_el1 = realm_shared_data_get_host_val(1);
	memset(&sve_state_send,0x0,sizeof(sve_state_send));
	sve_state_set(&sve_state_send,
				SVE_Z_REALM_VALUE,
				SVE_P_REALM_VALUE,
				FPSR_REALM_VALUE,
				FPCR_REALM_VALUE,
				zcr_el1,
				zcr_el1,
				SVE_FFR_REALM_VALUE);
	/* read SVE state registers values.*/
	if (sve_read_compare((const struct sve_state*)&sve_state_send,
			&sve_state_receive, &index) != SVE_STATE_SUCCESS) {
		ERROR("realm_req_sve_cmp_cmd failed\n");
				return false;
	}
	return true;
}
/**
 *   @brief    - C entry function for realm payload
 *   @param    - void
 *   @return   - void (returns to Host)
 */
void realm_payload_main(void)
{
	uint8_t cmd = 0U;
	bool test_succeed = true;

	realm_set_shared_structure((host_shared_data_t *)rsi_get_ns_buffer());
	if (realm_get_shared_structure() != NULL) {
		cmd = realm_shared_data_get_realm_cmd();
		switch (cmd) {
		case REALM_SLEEP_CMD:
			realm_sleep_cmd();
			break;
		case REALM_GET_RSI_VERSION:
			realm_get_version_cmd();
			break;
		case REALM_REQ_FPU_FILL_CMD:
			realm_req_fpu_fill_cmd();
			break;
		case REALM_REQ_FPU_CMP_CMD:
			test_succeed = realm_req_fpu_cmp_cmd();
			break;
		case REALM_REQ_SVE_FILL_CMD:
			realm_req_sve_fill_cmd();
			break;
		case REALM_REQ_SVE_CMP_CMD:
			test_succeed = realm_req_sve_cmp_cmd();
			break;
		default:
			INFO("REALM_PAYLOAD: %s invalid cmd=%hhu", __func__, cmd);
			test_succeed = false;
			break;
		}
	} else {
		test_succeed = false;
	}
	if (test_succeed) {
		rsi_exit_to_host(HOST_CALL_EXIT_SUCCESS_CMD);
	} else {
		rsi_exit_to_host(HOST_CALL_EXIT_FAILED_CMD);
	}
}

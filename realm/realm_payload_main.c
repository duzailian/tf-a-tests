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
#include "realm_def.h"
#include <realm_rsi.h>
#include <tftf_lib.h>

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
 *   @brief    - C entry function for Realm payload
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

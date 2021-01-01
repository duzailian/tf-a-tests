/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>
#include <smccc.h>

#include <arch_helpers.h>
#include <cactus_test_cmds.h>
#include <ffa_endpoints.h>
#include <ffa_svc.h>
#include <lib/events.h>
#include <lib/power_management.h>
#include <platform.h>
#include <test_helpers.h>

/**************************************************************************
 * Send a command to SP1 initiate DMA service with the help of a peripheral
 * device upstream of an SMMUv3 IP
 **************************************************************************/
test_result_t test_smmu_spm(void)
{
	smc_ret_values ret;

	VERBOSE("Sending command to SP %x for initiating DMA transfer\n",
			SP_ID(1));
	ret = cactus_send_dma_cmd(HYP_ID, SP_ID(1));

	if (cactus_get_response(ret) != CACTUS_SUCCESS){
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}


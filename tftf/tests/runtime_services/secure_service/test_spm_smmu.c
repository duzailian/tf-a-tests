/*
 * Copyright (c) 2021-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cactus_test_cmds.h>
#include <debug.h>
#include <ffa_endpoints.h>
#include <smccc.h>
#include <spm_test_helpers.h>
#include <sp_platform_def.h>
#include <test_helpers.h>

#define ENGINE_MEMCPY	(2U)
#define ENGINE_RAND48	(3U)

static const struct ffa_uuid expected_sp_uuids[] = { {PRIMARY_UUID} };

/**************************************************************************
 * test_smmu_spm
 *
 * Send commands to SP1 initiate DMA service with the help of a peripheral
 * device upstream of an SMMUv3 IP.
 * The scenario involves randomizing a secure buffer (first DMA operation),
 * copying this buffer to another location (second DMA operation),
 * and checking (by CPU) that both buffer contents match.
 **************************************************************************/
test_result_t test_smmu_spm(void)
{
	struct ffa_value ret;

	/**********************************************************************
	 * Check SPMC has ffa_version and expected FFA endpoints are deployed.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);

	VERBOSE("Sending command to SP %x for initiating DMA transfer.\n",
			SP_ID(1));

	/*
	 * Randomize first half of a secure buffer from the secure world
	 * through the SMMU test engine DMA.
	 */
	ret = cactus_send_dma_cmd(HYP_ID, SP_ID(1),
		ENGINE_RAND48,
		PLAT_CACTUS_MEMCPY_BASE,
		PLAT_CACTUS_MEMCPY_RANGE / 2);

	/* Expect the SMMU DMA operation to pass. */
	if (cactus_get_response(ret) != CACTUS_SUCCESS) {
		return TEST_RESULT_FAIL;
	}

	/*
	 * Copy first half to second half of the buffer and
	 * check both match.
	 */
	ret = cactus_send_dma_cmd(HYP_ID, SP_ID(1),
		ENGINE_MEMCPY,
		PLAT_CACTUS_MEMCPY_BASE,
		PLAT_CACTUS_MEMCPY_RANGE);

	/* Expect the SMMU DMA operation to pass. */
	if (cactus_get_response(ret) != CACTUS_SUCCESS) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

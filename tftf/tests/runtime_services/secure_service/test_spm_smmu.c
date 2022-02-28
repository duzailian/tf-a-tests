/*
 * Copyright (c) 2021-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cactus_test_cmds.h>
#include <debug.h>
#include <ffa_endpoints.h>
#include <smccc.h>
#include <test_helpers.h>
#include <sp_platform_def.h>
#include <runtime_services/realm_payload/realm_payload_test.h>

static const struct ffa_uuid expected_sp_uuids[] = { {PRIMARY_UUID} };

#define PLAT_CACTUS_SMMU_NS_MEMCPY_BASE		ULL(0x82000000)

/**************************************************************************
 * Send a command to SP1 initiate DMA service with the help of a peripheral
 * device upstream of an SMMUv3 IP
 **************************************************************************/
test_result_t test_smmu_spm(void)
{
	struct ffa_value ret;

	/**********************************************************************
	 * Check SPMC has ffa_version and expected FFA endpoints are deployed.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 0, expected_sp_uuids);

	VERBOSE("Sending command to SP %x for initiating DMA transfer.\n",
			SP_ID(1));

	ret = cactus_send_dma_cmd(HYP_ID, SP_ID(1),
		2, /* TODO: ENGINE_MEMCPY */
		PLAT_CACTUS_MEMCPY_BASE,
		PLAT_CACTUS_MEMCPY_RANGE);

	if (cactus_get_response(ret) != CACTUS_SUCCESS) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/**************************************************************************
 * TODO: test_spm_smmu_invalid_access
 **************************************************************************/
test_result_t test_spm_smmu_invalid_access(void)
{
	struct ffa_value ret;
	u_register_t retmm;

	/* Skip this test if RME is not implemented. */
	if (get_armv9_2_feat_rme_support() == 0U) {
		return TEST_RESULT_SKIPPED;
	}

	/**********************************************************************
	 * Check SPMC has ffa_version and expected FFA endpoints are deployed.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 0, expected_sp_uuids);

	retmm = realm_granule_delegate((u_register_t)PLAT_CACTUS_SMMU_NS_MEMCPY_BASE);
	if (retmm != 0UL) {
		ERROR("Granule delegate failed!\n");
		return TEST_RESULT_FAIL;
	}

	VERBOSE("Sending command to SP %x for initiating DMA transfer.\n",
			SP_ID(1));

	(void)cactus_send_dma_cmd(HYP_ID, SP_ID(1),
		3, /* TODO: ENGINE_RAND48 */
		PLAT_CACTUS_SMMU_NS_MEMCPY_BASE,
		PLAT_CACTUS_MEMCPY_RANGE);

	retmm = realm_granule_undelegate((u_register_t)PLAT_CACTUS_SMMU_NS_MEMCPY_BASE);
	if (retmm != 0UL) {
		ERROR("Granule undelegate failed!\n");
		return TEST_RESULT_FAIL;
	}

	/* Expect the SMMU access to fail. */
	if (cactus_get_response(ret) != CACTUS_ERROR) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

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
#include <test_helpers.h>
#include <sp_platform_def.h>
#include <runtime_services/host_realm_managment/host_realm_rmi.h>
#include <runtime_services/host_realm_managment/host_realm_rmi.h>

/* TODO: export SMMUv3 test engine macros. */
#define ENGINE_MEMCPY	(2U)
#define ENGINE_RAND48	(3U)

static const struct ffa_uuid expected_sp_uuids[] = { {PRIMARY_UUID} };

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
	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);

	VERBOSE("Sending command to SP %x for initiating DMA transfer.\n",
			SP_ID(1));

	ret = cactus_send_dma_cmd(HYP_ID, SP_ID(1),
		ENGINE_RAND48,
		PLAT_CACTUS_MEMCPY_BASE,
		PLAT_CACTUS_MEMCPY_RANGE);

	/* Expect the SMMU DMA operation to pass. */
	if (cactus_get_response(ret) != CACTUS_SUCCESS) {
		return TEST_RESULT_FAIL;
	}

	/* Expect the SMMU DMA operation to pass. */
	ret = cactus_send_dma_cmd(HYP_ID, SP_ID(1),
		ENGINE_MEMCPY,
		PLAT_CACTUS_MEMCPY_BASE,
		PLAT_CACTUS_MEMCPY_RANGE);

	if (cactus_get_response(ret) != CACTUS_SUCCESS) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/**************************************************************************
 * TODO: test_smmu_spm_invalid_access
 **************************************************************************/
test_result_t test_smmu_spm_invalid_access(void)
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
	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);

	retmm = host_rmi_granule_delegate((u_register_t)PLAT_CACTUS_SMMU_NS_MEMCPY_BASE);
	if (retmm != 0UL) {
		ERROR("Granule delegate failed!\n");
		return TEST_RESULT_FAIL;
	}

	VERBOSE("Sending command to SP %x for initiating DMA transfer.\n",
			SP_ID(1));

	ret = cactus_send_dma_cmd(HYP_ID, SP_ID(1),
		ENGINE_RAND48,
		PLAT_CACTUS_SMMU_NS_MEMCPY_BASE,
		PLAT_CACTUS_MEMCPY_RANGE);

	retmm = host_rmi_granule_undelegate((u_register_t)PLAT_CACTUS_SMMU_NS_MEMCPY_BASE);
	if (retmm != 0UL) {
		ERROR("Granule undelegate failed!\n");
		return TEST_RESULT_FAIL;
	}

	/* Expect the SMMU DMA operation to fail. */
	if (cactus_get_response(ret) != CACTUS_ERROR) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

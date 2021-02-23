/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <ffa_endpoints.h>
#include <spm_common.h>
#include <test_helpers.h>

static const struct ffa_uuid sp_uuids[] = {
		{PRIMARY_UUID}, {SECONDARY_UUID}, {TERTIARY_UUID}
	};
static const struct ffa_uuid null_uuid = { .uuid = {0} };

static const struct ffa_partition_info ffa_expected_partition_info[] = {
	/* Primary partition info */
	{
		.id = SP_ID(1),
		.exec_context = PRIMARY_EXEC_CTX_COUNT,
		.properties = FFA_PARTITION_DIRECT_REQ_RECV
	},
	/* Secondary partition info */
	{
		.id = SP_ID(2),
		.exec_context = SECONDARY_EXEC_CTX_COUNT,
		.properties = FFA_PARTITION_DIRECT_REQ_RECV
	},
	/* Tertiary partition info */
	{
		.id = SP_ID(3),
		.exec_context = TERTIARY_EXEC_CTX_COUNT,
		.properties = FFA_PARTITION_DIRECT_REQ_RECV
	},
	/* Ivy partition info */
	{
		.id = SP_ID(4),
		.exec_context = IVY_EXEC_CTX_COUNT,
		.properties = FFA_PARTITION_DIRECT_REQ_RECV
	}
};

/**
 * Attempt to get the SP partition information for individual partitions as well
 * as all secure partitions.
 */
test_result_t test_ffa_partition_info(void)
{
	/***********************************************************************
	 * Check if SPMC has ffa_version and expected FFA endpoints are deployed.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 0, sp_uuids);

	struct mailbox_buffers mb;

	GET_TFTF_MAILBOX(mb);

	if (!ffa_partition_info_helper(&mb, sp_uuids[0],
		&ffa_expected_partition_info[0], 1)) {
		return TEST_RESULT_FAIL;
	}
	if (!ffa_partition_info_helper(&mb, sp_uuids[1],
		&ffa_expected_partition_info[1], 1)) {
		return TEST_RESULT_FAIL;
	}
	if (!ffa_partition_info_helper(&mb, sp_uuids[2],
		&ffa_expected_partition_info[2], 1)) {
		return TEST_RESULT_FAIL;
	}
	if (!ffa_partition_info_helper(&mb, null_uuid,
		ffa_expected_partition_info, 4)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

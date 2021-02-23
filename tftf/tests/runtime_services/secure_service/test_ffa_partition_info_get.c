/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <ffa_endpoints.h>
#include <spm_common.h>
#include <test_helpers.h>

static const uint32_t primary_uuid[4] = PRIMARY_UUID;
static const uint32_t secondary_uuid[4] = SECONDARY_UUID;
static const uint32_t tertiary_uuid[4] = TERTIARY_UUID;
static const uint32_t null_uuid[4] = {0};

/**
 * Attempt to get the SP partition information for individual partitions as well
 * as all secure partitions.
 */
test_result_t test_ffa_partition_info(void)
{
	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 0);

	struct mailbox_buffers mb;
	GET_TFTF_MAILBOX(mb);

	const struct ffa_partition_info *expected_info;
	unsigned int num_of_partitions =
		get_ffa_partition_info_test_target(&expected_info, false);

	if (!ffa_partition_info_helper(&mb, primary_uuid,
		&expected_info[0], 1)) {
		return TEST_RESULT_FAIL;
	}
	if (!ffa_partition_info_helper(&mb, secondary_uuid,
		&expected_info[1], 1)) {
		return TEST_RESULT_FAIL;
	}
	if (!ffa_partition_info_helper(&mb, tertiary_uuid,
		&expected_info[2], 1)) {
		return TEST_RESULT_FAIL;
	}
	if (!ffa_partition_info_helper(&mb, null_uuid,
		expected_info, num_of_partitions)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}
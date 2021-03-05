/*
 * Copyright (c) 2018-2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>
#include <debug.h>
#include <errno.h>

#include <cactus_def.h>
#include <cactus_platform_def.h>
#include <ffa_endpoints.h>
#include <sp_helpers.h>
#include <spm_helpers.h>
#include <spm_common.h>

#include <lib/libc/string.h>

/* FFA version test helpers */
#define FFA_MAJOR 1U
#define FFA_MINOR 0U

static const uint32_t primary_uuid[4] = PRIMARY_UUID;
static const uint32_t secondary_uuid[4] = SECONDARY_UUID;
static const uint32_t tertiary_uuid[4] = TERTIARY_UUID;
static const uint32_t null_uuid[4] = {0};

static const struct ffa_partition_info ffa_expected_partition_info[] = {
	/* Primary partition info */
	{
		.id = SP_ID(1),
		.exec_context = PRIMARY_EXEC_CTX,
		.properties = (FFA_PARTITION_DIRECT_RECV | FFA_PARTITION_DIRECT_SEND)
	},
	/* Secondary partition info */
	{
		.id = SP_ID(2),
		.exec_context = SECONDARY_EXEC_CTX,
		.properties = (FFA_PARTITION_DIRECT_RECV | FFA_PARTITION_DIRECT_SEND)
	},
	/* Tertiary partition info */
	{
		.id = SP_ID(3),
		.exec_context = TERTIARY_EXEC_CTX,
		.properties = (FFA_PARTITION_DIRECT_RECV | FFA_PARTITION_DIRECT_SEND)
	}
};

/*
 * Test FFA_FEATURES interface.
 */
static void ffa_features_test(void)
{
	const char *test_features = "FFA Features interface";
	smc_ret_values ffa_ret;
	const struct ffa_features_test *ffa_feature_test_target;
	unsigned int i, test_target_size =
		get_ffa_feature_test_target(&ffa_feature_test_target);


	announce_test_section_start(test_features);

	for (i = 0U; i < test_target_size; i++) {
		announce_test_start(ffa_feature_test_target[i].test_name);

		ffa_ret = ffa_features(ffa_feature_test_target[i].feature);
		expect(ffa_func_id(ffa_ret), ffa_feature_test_target[i].expected_ret);
		if (ffa_feature_test_target[i].expected_ret == FFA_ERROR) {
			expect(ffa_error_code(ffa_ret), FFA_ERROR_NOT_SUPPORTED);
		}

		announce_test_end(ffa_feature_test_target[i].test_name);
	}

	announce_test_section_end(test_features);
}

static void ffa_partition_info_wrong_test(void)
{
	const char *test_wrong_uuid = "Request wrong UUID";
	uint32_t uuid[4] = {1};

	announce_test_start(test_wrong_uuid);

	smc_ret_values ret = ffa_partition_info_get(uuid);
	expect(ffa_func_id(ret), FFA_ERROR);
	expect(ffa_error_code(ret), FFA_ERROR_INVALID_PARAMETER);

	announce_test_end(test_wrong_uuid);
}

static void ffa_partition_info_get_test(struct mailbox_buffers *mb)
{
	const char *test_partition_info = "FFA Partition info interface";

	announce_test_section_start(test_partition_info);

	expect(ffa_partition_info_helper(mb, tertiary_uuid,
		&ffa_expected_partition_info[2], 1), true);

	expect(ffa_partition_info_helper(mb, secondary_uuid,
		&ffa_expected_partition_info[1], 1), true);

	expect(ffa_partition_info_helper(mb, primary_uuid,
		&ffa_expected_partition_info[0], 1), true);

	expect(ffa_partition_info_helper(mb, null_uuid,
		ffa_expected_partition_info, 3), true);

	ffa_partition_info_wrong_test();

	announce_test_section_end(test_partition_info);
}

void ffa_version_test(void)
{
	const char *test_ffa_version = "FFA Version interface";

	announce_test_start(test_ffa_version);

	smc_ret_values ret = ffa_version(MAKE_FFA_VERSION(FFA_MAJOR, FFA_MINOR));
	uint32_t spm_version = (uint32_t)ret.ret0;

	bool ffa_version_compatible =
		((spm_version >> FFA_VERSION_MAJOR_SHIFT) == FFA_MAJOR &&
		 (spm_version & FFA_VERSION_MINOR_MASK) >= FFA_MINOR);

	NOTICE("FFA_VERSION returned %u.%u; Compatible: %i\n",
		spm_version >> FFA_VERSION_MAJOR_SHIFT,
		spm_version & FFA_VERSION_MINOR_MASK,
		(int)ffa_version_compatible);

	expect((int)ffa_version_compatible, (int)true);

	announce_test_end(test_ffa_version);
}

void ffa_tests(struct mailbox_buffers *mb)
{
	const char *test_ffa = "FFA Interfaces";

	announce_test_section_start(test_ffa);

	ffa_features_test();
	ffa_version_test();
	ffa_partition_info_get_test(mb);

	announce_test_section_end(test_ffa);
}

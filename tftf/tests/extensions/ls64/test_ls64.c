/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <test_helpers.h>

#include "./test_ls64.h"
#if __GNUC__ > 8 || (__GNUC__ == 8 && __GNUC_MINOR__ > 0)

static uint64_t ls64_input_buffer[LS64_ARRAYSIZE]={1,2,3,4,5,6,7,8};
static uint64_t ls64_output_buffer[LS64_ARRAYSIZE]={0};

/*
 * @brief Test LS64 feature support when the extension is enabled.
 *
 * Execute the LS64 instructions:
 * LD64B   -  single-copy atomic 64-byte load.
 * ST64B   -  single-copy atomic 64-byte store without return.
 *
 * These instructions should not be trapped to EL3, as TF-A is responsible
 * for enabling LS64* for Non-Secure World.
 *
 * @return test_result_t
 */
test_result_t test_ls64_instructions(void)
{
	/*
	 * Address where the data will be written to/read from with instructions
	 * st64b and ld64b respectively. Can only be in range 0x1d000000 - 0x1d001111
	 * and be 64-byte aligned.
	 */
	uint64_t *store_address = (uint64_t *)0x1d000000;

        test_result_t ret = TEST_RESULT_SUCCESS;

        SKIP_TEST_IF_AARCH32();

#ifdef __aarch64__
        SKIP_TEST_IF_LS64_NOT_SUPPORTED();

        /* FEAT_LS64 : Perform LD64B and ST64B operations */
        if (get_feat_ls64_support() == ID_AA64ISAR1_LS64_SUPPORTED) {
                INFO("Testing : FEAT_LS64\n");

		ls64_store(ls64_input_buffer, store_address);
		ls64_load(store_address, ls64_output_buffer);

                for (uint64_t i = 0; i < LS64_ARRAYSIZE; i++) {
                        INFO("Input Buffer[%lld]=%lld\n",i,ls64_input_buffer[i]);
                        INFO("Output Buffer[%lld]=%lld\n",i,ls64_output_buffer[i]);

                        if (ls64_input_buffer[i] != ls64_output_buffer[i]) {
                                ret = TEST_RESULT_FAIL;
                                break;
                        }
                }
	}
	else {
		tftf_testcase_printf("This test only supports FEAT_LS64 at the moment\n");
		return TEST_RESULT_SKIPPED;
	}

#endif
        return ret;
}

#else

test_result_t test_ls64_instructions(void)
{
        tftf_testcase_printf("Unsupported compiler\n");
	return TEST_RESULT_SKIPPED;
}

#endif /* __GNUC__ > 8 || (__GNUC__ == 8 && __GNUC_MINOR__ > 0) */

/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "./test_ls64.h"
#include <test_helpers.h>

static uint64_t ls64_input_buffer[LS64_ARRAYSIZE] = {1, 2, 3, 4, 5, 6, 7, 8};
static uint64_t ls64_output_buffer[LS64_ARRAYSIZE] = {0};

/*
 * @brief Test LS64 feature support when the extension is enabled.
 *
 * Execute the LS64 instructions:
 * LD64B   -  single-copy atomic 64-byte load.
 * ST64B   -  single-copy atomic 64-byte store without return.
 *
 * These instructions should not be trapped to EL3, when EL2 access them.
 *
 * @return test_result_t
 */
test_result_t test_ls64_instructions(void)
{
#ifdef __aarch64__

	/* Make sure FEAT_LS64 is supported. */
	SKIP_TEST_IF_LS64_NOT_SUPPORTED();

	/*
	 * Address where the data will be written to/read from with instructions
	 * st64b and ld64b respectively.
	 * Can only be in range (0x1d000000 - 0x1d001111) and be 64-byte aligned.
	 */
	uint64_t *store_address = (uint64_t *)0x1d000000;

	if (get_feat_ls64_support() == ID_AA64ISAR1_LS64_SUPPORTED) {
		/**
		 * FEAT_LS64 : Execute LD64B and ST64B Instructions.
		 * This test copies data from input buffer, an array of 8-64bit
		 * unsigned integers to an output buffer via LD64B and ST64B
		 * atomic operation instructions.
		 *
		 * To begin with data gets stored from inputbuffer to a predefined
		 * memory address (store_address) via ST64B.
		 * Further the data from this memory location(store_address)
		 * will be loaded to output buffer via LD64B instruction.
		 */

		ls64_store(ls64_input_buffer, store_address);
		ls64_load(store_address, ls64_output_buffer);

		for (uint64_t i = 0; i < LS64_ARRAYSIZE; i++) {
			tftf_testcase_printf("Input Buffer[%lld]=%lld\n",i,ls64_input_buffer[i]);
			tftf_testcase_printf("Output Buffer[%lld]=%lld\n",i,ls64_output_buffer[i]);

			if (ls64_input_buffer[i] != ls64_output_buffer[i]) {
				return TEST_RESULT_FAIL;
			}
		}
	}
	else {
		tftf_testcase_printf("This test only supports FEAT_LS64 at the moment\n");
		return TEST_RESULT_SKIPPED;
	}
	return TEST_RESULT_SUCCESS;
#else
	/* Skip test if AArch32 */
	SKIP_TEST_IF_AARCH32();
#endif
}

/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <ffa_helpers.h>
#include <test_helpers.h>

static bool simd_vector_compare(simd_vector_t *a, simd_vector_t *b, unsigned int length)
{
	for (unsigned int num = 0U; num < length; num++) {
		if (memcmp(a[num], b[num], sizeof(simd_vector_t)) != 0) {
			NOTICE("Vectors not equal: a:0x%llx b:0x%llx\n",
				(uint64_t)a[num][0], (uint64_t)b[num][0]);
			return false;
		}
	}
	return true;
}

test_result_t test_ffa_simd_preserved(void)
{
	/**********************************************************************
	 * Verify that FFA is there and that it has the correct version.
	 **********************************************************************/
	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 0);


	simd_vector_t simd_vectors[SIMD_NUM_VECTORS], simd_vectors_compare[SIMD_NUM_VECTORS];

	for (unsigned int num = 0U; num < SIMD_NUM_VECTORS; num++) {
		memset(simd_vectors[num], 0x11 * num, sizeof(simd_vector_t));
	}
	ffa_fill_simd_vector_regs(simd_vectors);

	// TODO: Change 0x53494d44(SIMD in hex) to relevant command name
	ffa_msg_send_direct_req(HYP_ID, SP_ID(1), 0x53494d44);

	ffa_dump_simd_vector_regs(simd_vectors_compare);

	if (!simd_vector_compare(simd_vectors, simd_vectors_compare,
				 SIMD_NUM_VECTORS)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

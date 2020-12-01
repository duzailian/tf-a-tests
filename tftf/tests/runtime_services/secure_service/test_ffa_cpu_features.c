/*
 * Copyright (c) 2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <ffa_helpers.h>
#include <test_helpers.h>

#define __STR(x) #x
#define STR(x) __STR(x)

/*
 * Vector length:
 * SIMD: 128 bits
 * SVE/SME: platform-defined ranging from 128b to 2048b in 128b increments.
 *          Choose a realistic 512b value for test purposes.
 */
#define SIMD_VECTOR_LEN_BITS		128
#define SIMD_VECTOR_LEN_BYTES		(SIMD_VECTOR_LEN_BITS >> 3)
#define SIMD_TWO_VECTORS_BYTES_STR	(2 * SIMD_VECTOR_LEN_BYTES)

/* SIMD/SVE/SME implement the same number of vector registers. */
#define NUM_VECTORS		32

typedef uint128_t simd_vector_t[SIMD_VECTOR_LEN_BYTES / sizeof(uint128_t)];

static simd_vector_t simd_vectors[NUM_VECTORS], simd_vectors_compare[NUM_VECTORS];

static void test_fill_simd_vectors(simd_vector_t *v)
{
	for (unsigned int num = 0U; num < NUM_VECTORS; num++) {
		memset(&v[num], num+1, sizeof(simd_vector_t));
	}

	__asm__ volatile(
		"ldp q0, q1, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"ldp q2, q3, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"ldp q4, q5, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"ldp q6, q7, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"ldp q8, q9, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"ldp q10, q11, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"ldp q12, q13, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"ldp q14, q15, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"ldp q16, q17, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"ldp q18, q19, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"ldp q20, q21, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"ldp q22, q23, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"ldp q24, q25, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"ldp q26, q27, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"ldp q28, q29, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"ldp q30, q31, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"sub %0, %0, #" STR(NUM_VECTORS * SIMD_VECTOR_LEN_BYTES) ";"
		: : "r" (v));
}

static void test_dump_simd_vectors(simd_vector_t *v)
{
	for (unsigned int num = 0U; num < NUM_VECTORS; num++) {
		memset(v[num], 0, sizeof(simd_vector_t));
	}

	__asm__ volatile(
		"stp q0, q1, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"stp q2, q3, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"stp q4, q5, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"stp q6, q7, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"stp q8, q9, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"stp q10, q11, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"stp q12, q13, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"stp q14, q15, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"stp q16, q17, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"stp q18, q19, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"stp q20, q21, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"stp q22, q23, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"stp q24, q25, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"stp q26, q27, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"stp q28, q29, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"stp q30, q31, [%0], #" STR(SIMD_TWO_VECTORS_BYTES_STR) ";"
		"sub %0, %0, #" STR(NUM_VECTORS * SIMD_VECTOR_LEN_BYTES) ";"
		: : "r" (v));
}

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


	test_fill_simd_vectors(simd_vectors);

	// TODO: Change 0x53494d44(SIMD in hex) to relevant command name
	ffa_msg_send_direct_req(HYP_ID, SP_ID(1), 0x53494d44);

	test_dump_simd_vectors(simd_vectors_compare);

	if (!simd_vector_compare(simd_vectors, simd_vectors_compare,
				 NUM_VECTORS)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

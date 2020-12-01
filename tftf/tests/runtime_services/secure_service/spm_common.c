/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <ffa_endpoints.h>
#include <spm_common.h>

void fill_simd_vector_regs(const simd_vector_t v[SIMD_NUM_VECTORS])
{
#ifdef __aarch64__
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
		"sub %0, %0, #" STR(SIMD_NUM_VECTORS * SIMD_VECTOR_LEN_BYTES) ";"
		: : "r" (v));
#endif
}

void read_simd_vector_regs(simd_vector_t v[SIMD_NUM_VECTORS])
{
#ifdef __aarch64__
	for (unsigned int num = 0U; num < SIMD_NUM_VECTORS; num++) {
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
		"sub %0, %0, #" STR(SIMD_NUM_VECTORS * SIMD_VECTOR_LEN_BYTES) ";"
		: : "r" (v));
#endif
}

/*
 * check_spmc_execution_level
 *
 * Attempt sending impdef protocol messages to OP-TEE through direct messaging.
 * Criteria for detecting OP-TEE presence is that responses match defined
 * version values. In the case of SPMC running at S-EL2 (and Cactus instances
 * running at S-EL1) the response will not match the pre-defined version IDs.
 *
 * Returns true if SPMC is probed as being OP-TEE at S-EL1.
 *
 */
bool check_spmc_execution_level(void)
{
	unsigned int is_optee_spmc_criteria = 0U;
	smc_ret_values ret_values;

	/*
	 * Send a first OP-TEE-defined protocol message through
	 * FFA direct message.
	 *
	 */
	ret_values = ffa_msg_send_direct_req(HYP_ID, SP_ID(1),
						OPTEE_FFA_GET_API_VERSION);
	if ((ret_values.ret3 == FFA_VERSION_MAJOR) &&
	    (ret_values.ret4 == FFA_VERSION_MINOR)) {
		is_optee_spmc_criteria++;
	}

	/*
	 * Send a second OP-TEE-defined protocol message through
	 * FFA direct message.
	 *
	 */
	ret_values = ffa_msg_send_direct_req(HYP_ID, SP_ID(1),
						OPTEE_FFA_GET_OS_VERSION);
	if ((ret_values.ret3 == OPTEE_FFA_GET_OS_VERSION_MAJOR) &&
	    (ret_values.ret4 == OPTEE_FFA_GET_OS_VERSION_MINOR)) {
		is_optee_spmc_criteria++;
	}

	return (is_optee_spmc_criteria == 2U);
}
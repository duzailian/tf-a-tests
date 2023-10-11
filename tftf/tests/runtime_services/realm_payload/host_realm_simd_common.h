/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef HOST_REALM_COMMON_H
#define HOST_REALM_COMMON_h

#define NS_SVE_OP_ARRAYSIZE		1024U
#define SVE_TEST_ITERATIONS		50U

/*
 * Min test iteration count for 'host_and_realm_check_simd' and
 * 'host_realm_swd_check_simd' tests.
 */
#define TEST_ITERATIONS_MIN	(16U)

/* Number of FPU configs: none */
#define NUM_FPU_CONFIGS		(0U)

/* Number of SVE configs: SVE_VL, SVE hint */
#define NUM_SVE_CONFIGS		(2U)

/* Number of SME configs: SVE_SVL, FEAT_FA64, Streaming mode */
#define NUM_SME_CONFIGS		(3U)

#define NS_NORMAL_SVE			0x1U
#define NS_STREAMING_SVE		0x2U

typedef enum {
	TEST_FPU = 0U,
	TEST_SVE,
	TEST_SME,
} simd_test_t;

typedef enum security_state {
	NONSECURE_WORLD = 0U,
	REALM_WORLD,
	SECURE_WORLD,
	SECURITY_STATE_MAX
} security_state_t;

test_result_t host_create_sve_realm_payload(struct realm *realm, bool sve_en, uint8_t sve_vq);
simd_test_t ns_simd_write_rand(void);
test_result_t ns_simd_read_and_compare(simd_test_t type);
simd_test_t rl_simd_write_rand(struct realm *realm, bool rl_sve_en);
bool rl_simd_read_and_compare(struct realm *realm, simd_test_t type);

#endif

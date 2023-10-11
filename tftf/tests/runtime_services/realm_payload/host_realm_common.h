/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef HOST_REALM_COMMON_H
#define HOST_REALM_COMMON_h

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

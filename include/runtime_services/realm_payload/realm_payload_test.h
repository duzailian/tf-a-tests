/*
 * Copyright (c) 2021-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef REALM_PAYLOAD_TEST_H
#define REALM_PAYLOAD_TEST_H

#define NUM_GRANULES			5U
#define NUM_RANDOM_ITERATIONS		7U
#define B_DELEGATED			0U
#define B_UNDELEGATED			1U
#define NUM_CPU_DED_SPM			PLATFORM_CORE_COUNT / 2U

#define SIMD_NS_VALUE 			0x11
#define SIMD_REALM_VALUE 		0x33
#define SIMD_SECURE_VALUE 		0x22
#define FPCR_NS_VALUE 			0x7FF9F00
#define FPCR_REALM_VALUE 		0x75F9500
#define FPCR_SECURE_VALUE 		0x78F9900
#define FPSR_NS_VALUE 			0xF800009F
#define FPSR_REALM_VALUE 		0x88000097
#define FPSR_SECURE_VALUE 		0x98000095

typedef enum realm_test_cmd {
	CMD_SIMD_NS_FILL = 0U,
	CMD_SIMD_NS_CMP,
	CMD_SIMD_SEC_FILL,
	CMD_SIMD_SEC_CMP,
	CMD_SIMD_RL_FILL,
	CMD_SIMD_RL_CMP,
	CMD_MAX_COUNT
} realm_test_cmd_t;

#endif

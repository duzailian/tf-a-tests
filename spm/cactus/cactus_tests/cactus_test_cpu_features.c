/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "cactus_message_loop.h"
#include "cactus_test_cmds.h"
#include "spm_common.h"
#include <lib/extensions/fpu.h>

/* Used as template values for test cases. */
#define SIMD_SECURE_VALUE	0x22U
#define FPCR_SECURE_VALUE	0x78F9900U
#define FPSR_SECURE_VALUE	0x98000095U

/*
 * Fill FPU state(SIMD vectors, FPCR, FPSR) from secure world side with a unique value.
 * SIMD_SECURE_VALUE is just a dummy value to be distinguished from the value
 * in the normal world.
 */
CACTUS_CMD_HANDLER(req_simd_fill, CACTUS_REQ_SIMD_FILL_CMD)
{
	fpu_state_write_template(SIMD_SECURE_VALUE,
			FPCR_SECURE_VALUE,
			FPSR_SECURE_VALUE);
	return cactus_response(ffa_dir_msg_dest(*args),
			ffa_dir_msg_source(*args),
			CACTUS_SUCCESS);
}

/*
 * compare FPU state(SIMD vectors, FPCR, FPSR) from secure world side with the previous
 * SIMD_SECURE_VALUE unique value.
 */
CACTUS_CMD_HANDLER(req_simd_compare, CACTUS_CMP_SIMD_VALUE_CMD)
{
	bool test_succeed = false;

	test_succeed = fpu_state_compare_template(SIMD_SECURE_VALUE,
			FPCR_SECURE_VALUE,
			FPSR_SECURE_VALUE);
	return cactus_response(ffa_dir_msg_dest(*args),
			ffa_dir_msg_source(*args),
			test_succeed ? CACTUS_SUCCESS : CACTUS_ERROR);
}

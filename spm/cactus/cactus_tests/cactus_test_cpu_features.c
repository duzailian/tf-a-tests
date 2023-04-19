/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "cactus_message_loop.h"
#include "cactus_test_cmds.h"
#include <fpu.h>
#include "spm_common.h"

fpu_reg_state_t g_fpu_temp;
/*
 * Fill SIMD vectors from secure world side with a unique value.
 */
CACTUS_CMD_HANDLER(req_simd_fill, CACTUS_REQ_SIMD_FILL_CMD)
{
	fpu_state_write_template(&g_fpu_temp);
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

	test_succeed = fpu_state_compare_template(&g_fpu_temp);
	return cactus_response(ffa_dir_msg_dest(*args),
			ffa_dir_msg_source(*args),
			test_succeed ? CACTUS_SUCCESS : CACTUS_ERROR);
}

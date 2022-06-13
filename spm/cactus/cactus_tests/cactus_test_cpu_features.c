/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "cactus_message_loop.h"
#include "cactus_test_cmds.h"
#include "spm_common.h"
#include <lib/extensions/fpu.h>

/*
 * Fill FPU state(SIMD vectors, FPCR, FPSR) from secure world side with a unique value.
 * SIMD_SECURE_VALUE is just a dummy value to be distinguished from the value
 * in the normal world.
 */
CACTUS_CMD_HANDLER(req_simd_fill, CACTUS_REQ_SIMD_FILL_CMD)
{
	fpu_reg_state_t fpu_state_send;
	fpu_state_set(&fpu_state_send, SIMD_SECURE_VALUE, FPCR_SECURE_VALUE, FPSR_SECURE_VALUE);
	fill_fpu_state_registers(&fpu_state_send);
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
	int ret;
	fpu_reg_state_t fpu_state_send, fpu_state_receive;
	fpu_state_set(&fpu_state_send, SIMD_SECURE_VALUE, FPCR_SECURE_VALUE, FPSR_SECURE_VALUE);

	read_fpu_state_registers(&fpu_state_receive);
	ret = memcmp((uint8_t *)&fpu_state_send,
						(uint8_t *)&fpu_state_receive,
						sizeof(fpu_reg_state_t));
	return cactus_response(ffa_dir_msg_dest(*args),
			ffa_dir_msg_source(*args),
			ret ? CACTUS_ERROR : CACTUS_SUCCESS);
}

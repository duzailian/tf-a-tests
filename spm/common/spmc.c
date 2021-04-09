/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <spmc.h>

/*******************************************************************************
 * Hypervisor Calls Wrappers
 ******************************************************************************/

ffa_int_id_t spm_interrupt_get(void)
{
	hvc_args args = {
		.fid = SPM_INTERRUPT_GET
	};

	hvc_ret_values ret = tftf_hvc(&args);

	return ret.ret0;
}

void spm_debug_log(char c)
{
	hvc_args args = {
		.fid = SPM_DEBUG_LOG,
		.arg1 = c
	};

	(void)tftf_hvc(&args);
}

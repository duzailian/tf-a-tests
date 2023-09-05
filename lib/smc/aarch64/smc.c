/*
 * Copyright (c) 2018-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <arch_features.h>
#include <platform.h>
#include <platform_def.h>
#include <stdint.h>
#include <smccc.h>
#include <tftf.h>
#include <utils_def.h>

smc_ret_values asm_tftf_smc64(uint32_t fid,
			      u_register_t arg1,
			      u_register_t arg2,
			      u_register_t arg3,
			      u_register_t arg4,
			      u_register_t arg5,
			      u_register_t arg6,
			      u_register_t arg7);

static bool smc_sve_hint[PLATFORM_CORE_COUNT];

/*
 * Update SVE usage for the current CPU. When set to 'true' denotes absence of
 * SVE specific live state.
 */
void tftf_update_smc_sve_hint(bool sve_hint_flag)
{
	if (is_armv8_2_sve_present()) {
		unsigned int core_pos = platform_get_core_pos(read_mpidr_el1());

		smc_sve_hint[core_pos] = sve_hint_flag;
	}
}

/* Return the SVE hint bit value for the current CPU */
bool tftf_get_smc_sve_hint(void)
{
	unsigned int core_pos = platform_get_core_pos(read_mpidr_el1());

	return smc_sve_hint[core_pos];
}

smc_ret_values tftf_smc(const smc_args *args)
{
	unsigned int core_pos = platform_get_core_pos(read_mpidr_el1());
	uint32_t fid = args->fid;

	if (is_armv8_2_sve_present()) {
		if (smc_sve_hint[core_pos]) {
			fid |= MASK(FUNCID_SVE_HINT);
		} else {
			fid &= ~MASK(FUNCID_SVE_HINT);
		}
	}

	return asm_tftf_smc64(fid,
			      args->arg1,
			      args->arg2,
			      args->arg3,
			      args->arg4,
			      args->arg5,
			      args->arg6,
			      args->arg7);
}

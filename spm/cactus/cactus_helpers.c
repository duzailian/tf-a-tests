/*
 * Copyright (c) 2020, NVIDIA Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <arch_features.h>
#include <arch_helpers.h>
#include <cactus_def.h>
#include <debug.h>
#include <platform_def.h>
#include <stdint.h>
#include <ffa_helpers.h>
#include <ffa_svc.h>

#define CACTUS_VM_GET_COUNT		(0xFF01)
#define CACTUS_VCPU_GET_COUNT		(0xFF02)
#define CACTUS_DEBUG_LOG		(0xBD000000)

/*******************************************************************************
 * Hypervisor Calls Wrappers
 ******************************************************************************/
ffa_vcpu_count_t cactus_vcpu_get_count(ffa_vm_id_t vm_id)
{
#if CACTUS_SEL1_SPMC
	return CACTUS_SEL1_SPMC_VCPU_COUNT;
#else
	hvc_args args = {
		.fid = CACTUS_VCPU_GET_COUNT,
		.arg1 = vm_id
	};
	hvc_ret_values ret = tftf_hvc(&args);
	return ret.ret0;
#endif
}

ffa_vm_count_t cactus_vm_get_count(void)
{
#if CACTUS_SEL1_SPMC
	return CACTUS_SEL1_SPMC_VM_COUNT;
#else
	hvc_args args = {
		.fid = CACTUS_VM_GET_COUNT
	};
	hvc_ret_values ret = tftf_hvc(&args);
	return ret.ret0;
#endif
}

void cactus_debug_log(char c)
{
	if (is_armv8_4_sel2_present()) {
		hvc_args args = {
			.fid = CACTUS_DEBUG_LOG,
			.arg1 = c
		};
		(void)tftf_hvc(&args);
	}
}

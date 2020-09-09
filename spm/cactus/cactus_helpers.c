/*
 * Copyright (c) 2020, NVIDIA Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>

#include <arch.h>
#include <arch_features.h>
#include <arch_helpers.h>
#include <cactus_def.h>
#include <debug.h>
#include <ffa_helpers.h>
#include <ffa_svc.h>
#include <sp_helpers.h>

#include <platform_def.h>

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
	return spm_vcpu_get_count(vm_id);
#endif
}

ffa_vm_count_t cactus_vm_get_count(void)
{
#if CACTUS_SEL1_SPMC
	return CACTUS_SEL1_SPMC_VM_COUNT;
#else
	return spm_vm_get_count();
#endif
}

void cactus_debug_log(char c)
{
	if (is_armv8_4_sel2_present())
		spm_debug_log(c);
}

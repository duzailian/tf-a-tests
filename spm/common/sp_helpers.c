/*
 * Copyright (c) 2018, Arm Limited. All rights reserved.
 * Copyright (c) 2020, NVIDIA Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <arch_features.h>
#include <arch_helpers.h>
#include <debug.h>
#include <mmio.h>
#include <platform_def.h>
#include <stdint.h>
#include <stdlib.h>
#include <ffa_svc.h>

#include "sp_helpers.h"

uintptr_t bound_rand(uintptr_t min, uintptr_t max)
{
	/*
	 * This is not ideal as some numbers will never be generated because of
	 * the integer arithmetic rounding.
	 */
	return ((rand() * (UINT64_MAX/RAND_MAX)) % (max - min)) + min;
}

/*******************************************************************************
 * Test framework helpers
 ******************************************************************************/

void expect(int expr, int expected)
{
	if (expr != expected) {
		ERROR("Expected value %i, got %i\n", expected, expr);
		while (1)
			continue;
	}
}

void announce_test_section_start(const char *test_sect_desc)
{
	INFO("========================================\n");
	INFO("Starting %s tests\n", test_sect_desc);
	INFO("========================================\n");
}
void announce_test_section_end(const char *test_sect_desc)
{
	INFO("========================================\n");
	INFO("End of %s tests\n", test_sect_desc);
	INFO("========================================\n");
}

void announce_test_start(const char *test_desc)
{
	INFO("[+] %s\n", test_desc);
}

void announce_test_end(const char *test_desc)
{
	INFO("Test \"%s\" end.\n", test_desc);
}

void sp_sleep(uint32_t ms)
{
	uint64_t start_count_val = syscounter_read();
	uint64_t wait_cycles = (ms * read_cntfrq_el0()) / 1000;

	while ((syscounter_read() - start_count_val) < wait_cycles)
		; /* Busy wait... */
}

/*******************************************************************************
 * Hypervisor Calls Wrappers
 ******************************************************************************/

ffa_vcpu_count_t spm_vcpu_get_count(ffa_vm_id_t vm_id)
{
	if (!is_armv8_4_sel2_present())
		return 1;

	hvc_args args = {
		.fid = SPM_VCPU_GET_COUNT,
		.arg1 = vm_id
	};

	hvc_ret_values ret = tftf_hvc(&args);

	return ret.ret0;
}

ffa_vm_count_t spm_vm_get_count(void)
{
	if (!is_armv8_4_sel2_present())
		return 1;

	hvc_args args = {
		.fid = SPM_VM_GET_COUNT
	};

	hvc_ret_values ret = tftf_hvc(&args);

	return ret.ret0;
}

void spm_debug_log(char c)
{
	if (is_armv8_4_sel2_present()) {
		hvc_args args = {
			.fid = SPM_DEBUG_LOG,
			.arg1 = c
		};

		(void)tftf_hvc(&args);
	}
}

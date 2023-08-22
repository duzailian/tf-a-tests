/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>

#include <arch_features.h>
#include <debug.h>
#include <fpu.h>
#include <host_realm_helper.h>
#include <host_shared_data.h>
#include <psci.h>
#include "realm_def.h"
#include <realm_rsi.h>
#include <realm_tests.h>
#include <tftf_lib.h>

static volatile int is_secondary_cpu_booted;
static spinlock_t counter_lock;
extern void realm_entrypoint(void);

static void secondary_cpu(void)
{
	smc_args args = { SMC_PSCI_CPU_OFF };

	spin_lock(&counter_lock);
	is_secondary_cpu_booted++;
	spin_unlock(&counter_lock);
	tftf_smc(&args);
}

bool test_realm_multiple_rec_multiple_cpu_cmd(void)
{
	smc_args args;
	smc_ret_values ret_vals;
	unsigned int i = 1U;

	realm_printf("Realm running on CPU = 0x%lx\n", read_mpidr_el1() & MPID_MASK);
	if ((read_mpidr_el1() & MPID_MASK) != 0U) {
		secondary_cpu();
		/* Does not return */
		return false;
	}
	init_spinlock(&counter_lock);
	for (unsigned int i = 1U; i < MAX_REC_COUNT; i++) {
		args.fid = SMC_PSCI_CPU_ON;
		args.arg1 = i;
		args.arg2 = (uintptr_t)realm_entrypoint;
		args.arg3 = 0x100 + i;

		ret_vals = tftf_smc(&args);
		if (ret_vals.ret0 != PSCI_E_SUCCESS && ret_vals.ret0 != PSCI_E_ALREADY_ON) {
			realm_printf("SMC_PSCI_CPU_ON failed %d.\n", i);
			return false;
		}
	}

	/* wait for all CPUs to come up. */
	waitms(200);

	if (is_secondary_cpu_booted != MAX_REC_COUNT-1) {
		return false;
	}

	/* wait for all CPUs to turn off */
	while (i < MAX_REC_COUNT) {
		args.fid = SMC_PSCI_AFFINITY_INFO;
		args.arg1 = i;
		args.arg2 = 0U;
		ret_vals = tftf_smc(&args);
		if (ret_vals.ret0 != PSCI_STATE_OFF) {
			/* wait and query again */
			realm_printf(" Wait CPU %d not off\n", i);
			waitms(50);
			continue;
		}
		i++;
	}
	return true;
}

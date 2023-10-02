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
#include <realm_psci.h>
#include <tftf_lib.h>

static volatile int is_secondary_cpu_booted;
static spinlock_t counter_lock;

static void secondary_cpu(void)
{
	spin_lock(&counter_lock);
	is_secondary_cpu_booted++;
	spin_unlock(&counter_lock);
}

bool test_realm_multiple_rec_multiple_cpu_cmd(void)
{
	unsigned int i = 1U;
	u_register_t ret;

	realm_printf("Realm running on CPU = 0x%lx\n", read_mpidr_el1() & MPID_MASK);
	init_spinlock(&counter_lock);
	for (unsigned int i = 1U; i < MAX_REC_COUNT; i++) {
		ret = realm_cpu_on(i, (uintptr_t)secondary_cpu, 0x100 + i);
		if (ret != PSCI_E_SUCCESS && ret != PSCI_E_ALREADY_ON) {
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
		ret = realm_psci_affinity_info(i, 0U);
		if (ret != PSCI_STATE_OFF) {
			/* wait and query again */
			realm_printf(" Wait CPU %d not off\n", i);
			waitms(50);
			continue;
		}
		i++;
	}
	return true;
}

bool test_realm_multiple_rec_psci_denied_cmd(void)
{
	u_register_t ret;

	ret = realm_cpu_on(1U, (uintptr_t)secondary_cpu, 0x100);
	if (ret != PSCI_E_DENIED) {
		return false;
	}
	return true;
}

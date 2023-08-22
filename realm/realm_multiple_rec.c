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

#define CXT_ID_MAGIC 0x100
static volatile int is_secondary_cpu_booted;
static spinlock_t counter_lock;

static void secondary_cpu(u_register_t cxt_id)
{
	realm_printf("Realm: running on CPU = 0x%lx cxt_id= 0x%lx\n",
			read_mpidr_el1() & MPID_MASK, cxt_id);
	if (cxt_id < CXT_ID_MAGIC || cxt_id > CXT_ID_MAGIC + MAX_REC_COUNT) {
		realm_printf("Realm: Wrong cxt_id\n");
		rsi_exit_to_host(HOST_CALL_EXIT_FAILED_CMD);
	}
	spin_lock(&counter_lock);
	is_secondary_cpu_booted++;
	spin_unlock(&counter_lock);
	realm_cpu_off();
}

bool test_realm_multiple_rec_multiple_cpu_cmd(void)
{
	unsigned int i = 1U;
	u_register_t ret;

	realm_printf("Realm: running on CPU = 0x%lx\n", read_mpidr_el1() & MPID_MASK);
	init_spinlock(&counter_lock);

	/* Check CPU_ON is supported */
	ret = realm_psci_features(SMC_PSCI_CPU_ON);
	if (ret != PSCI_E_SUCCESS) {
		realm_printf("SMC_PSCI_CPU_ON not supported\n");
	}

	for (unsigned int i = 1U; i < MAX_REC_COUNT; i++) {
		ret = realm_cpu_on(i, (uintptr_t)secondary_cpu, CXT_ID_MAGIC + i);
		if (ret != PSCI_E_SUCCESS) {
			realm_printf("SMC_PSCI_CPU_ON failed %d.\n", i);
			return false;
		}
	}

	/* wait for all CPUs to come up */
	if (is_secondary_cpu_booted != MAX_REC_COUNT-1) {
		waitms(500);
	}

	/* wait for all CPUs to turn off */
	while (i < MAX_REC_COUNT) {
		ret = realm_psci_affinity_info(i, MPIDR_AFFLVL0);
		if (ret != PSCI_STATE_OFF) {
			/* wait and query again */
			realm_printf(" CPU %d is not off\n", i);
			waitms(200);
			continue;
		}
		i++;
	}
	/*Try to on CPU 2 again*/
	ret = realm_cpu_on(2U, (uintptr_t)secondary_cpu, CXT_ID_MAGIC + 2U);
	if (ret != PSCI_E_SUCCESS && ret != PSCI_E_ALREADY_ON) {
		realm_printf("SMC_PSCI_CPU_ON failed %d.\n", i);
		return false;
	}
	return true;
}

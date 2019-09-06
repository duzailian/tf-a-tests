/*
 * Copyright (c) 2018, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <psci.h>
#include <smccc.h>
#include <tftf_lib.h>
#include "common/test_helpers.h"
#include "lib/power_management.h"

/* Generic function to call System OFF SMC */
static test_result_t test_cpu_system_off(void)
{
	unsigned int mpid = read_mpidr_el1() & MPID_MASK;
        smc_args args = { SMC_PSCI_SYSTEM_OFF };

	INFO("System off from CPU MPID 0x%x\n", mpid);
        tftf_notify_reboot();
        tftf_smc(&args);

	/* The PSCI SYSTEM_OFF call is not supposed to return */
        tftf_testcase_printf("System didn't shutdown properly\n");
        return TEST_RESULT_FAIL;
}

/*
 * @Test_Aim@ Validate the SYSTEM_OFF call.
 * Test SUCCESS in case of system shutdown.
 * Test FAIL in case of execution not terminated.
 */
test_result_t test_system_off(void)
{
        if (tftf_is_rebooted()) {
                /* Successfully resumed from system off */
                return TEST_RESULT_SUCCESS;
        }

	return test_cpu_system_off();
}

/*
 * @Test_Aim@ Validate the SYSTEM_OFF call on CPU other then lead CPU
 * Test SUCCESS in case of system shutdown.
 * Test FAIL in case of execution not terminated.
 */
test_result_t test_system_off_cpu_other_than_lead(void)
{
	unsigned int lead_mpid = read_mpidr_el1() & MPID_MASK;
	unsigned int cpu_mpid;
	int psci_ret;

	SKIP_TEST_IF_LESS_THAN_N_CPUS(2);

        if (tftf_is_rebooted()) {
                /* Successfully resumed from system off */
                return TEST_RESULT_SUCCESS;
        }

	/* Power ON another CPU, other then the lead CPU */
	cpu_mpid = tftf_find_random_cpu_other_than(lead_mpid);
	VERBOSE("CPU to be turned on MPID: 0x%x \n", cpu_mpid);
	psci_ret = tftf_cpu_on(cpu_mpid,
			       (uintptr_t)test_cpu_system_off,
			       0);

	if (psci_ret != PSCI_E_SUCCESS) {
		tftf_testcase_printf("Failed to power on CPU 0x%x (%d)\n",
				cpu_mpid, psci_ret);
		return TEST_RESULT_SKIPPED;
	} else {
		/*Power down the lead cpu. */
		VERBOSE("Lead CPU to be turned off MPID: 0x%x \n", lead_mpid);
		tftf_cpu_off();
	}
	return TEST_RESULT_FAIL;
}

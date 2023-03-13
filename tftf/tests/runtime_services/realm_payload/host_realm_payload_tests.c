/*
 * Copyright (c) 2021-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>

#include <assert.h>
#include <arch_features.h>
#include <debug.h>
#include <irq.h>
#include <drivers/arm/arm_gic.h>

#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_shared_data.h>

#define PMU_PPI		23
#define SLEEP_TIME_MS	200U

extern const char *rmi_exit[];

/*
 * @Test_Aim@ Test realm payload creation and execution
 */
test_result_t test_realm_create_enter(void)
{
	bool ret1, ret2;
	u_register_t retrmm;

	if (get_armv9_2_feat_rme_support() == 0U) {
		INFO("platform doesn't support RME\n");
		return TEST_RESULT_SKIPPED;
	}

	rmi_init_cmp_result();

	retrmm = rmi_version();
	VERBOSE("RMM version is: %lu.%lu\n",
			RMI_ABI_VERSION_GET_MAJOR(retrmm),
			RMI_ABI_VERSION_GET_MINOR(retrmm));
	/*
	 * Skip the test if RMM is TRP, TRP version is always null.
	 */
	if (retrmm == 0UL) {
		INFO("Test case not supported for TRP as RMM\n");
		return TEST_RESULT_SKIPPED;
	}

	if (!host_create_realm_payload((u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)(PAGE_POOL_MAX_SIZE +
			NS_REALM_SHARED_MEM_SIZE),
			(u_register_t)PAGE_POOL_MAX_SIZE,
			0UL)) {
		return TEST_RESULT_FAIL;
	}
	if (!host_create_shared_mem(NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		return TEST_RESULT_FAIL;
	}

	realm_shared_data_set_host_val(HOST_SLEEP_INDEX, SLEEP_TIME_MS);
	ret1 = host_enter_realm_execute(REALM_SLEEP_CMD, NULL);
	ret2 = host_destroy_realm();

	if (!ret1 || !ret2) {
		ERROR("%s(): enter=%d destroy=%d\n",
		__func__, ret1, ret2);
		return TEST_RESULT_FAIL;
	}

	return host_cmp_result();
}

/*
 * @Test_Aim@ Test realm PMU
 */
static test_result_t realm_pmuv3(uint8_t cmd)
{
	bool ret1, ret2;
	u_register_t retrmm;
	struct realm *realm_ptr;
	struct rmi_rec_run *run;

	if (get_armv9_2_feat_rme_support() == 0U) {
		INFO("platform doesn't support RME\n");
		return TEST_RESULT_SKIPPED;
	}

	rmi_init_cmp_result();

	retrmm = rmi_version();
	VERBOSE("RMM version is: %lu.%lu\n",
			RMI_ABI_VERSION_GET_MAJOR(retrmm),
			RMI_ABI_VERSION_GET_MINOR(retrmm));
	/*
	 * Skip the test if RMM is TRP, TRP version is always null.
	 */
	if (retrmm == 0UL) {
		INFO("Test case not supported for TRP as RMM\n");
		return TEST_RESULT_SKIPPED;
	}

	if (!host_create_realm_payload((u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)(PAGE_POOL_MAX_SIZE +
			NS_REALM_SHARED_MEM_SIZE),
			(u_register_t)PAGE_POOL_MAX_SIZE,
			RMI_FEATURE_REGISTER_0_PMU_EN)) {
		return TEST_RESULT_FAIL;
	}
	if (!host_create_shared_mem(NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		return TEST_RESULT_FAIL;
	}

	ret1 = host_enter_realm_execute(cmd, &realm_ptr);
	if (!ret1) {
		goto test_exit;
	}

	run = (struct rmi_rec_run *)realm_ptr->run;
	u_register_t exit_reason = run->exit.exit_reason;

	if (cmd == REALM_PMU_INTERRUPT) {
		/* Check for exit reason and PMU state */
		if ((exit_reason == RMI_EXIT_IRQ) &&
		   ((run->exit.pmu_ovf & run->exit.pmu_intr_en &
		     run->exit.pmu_cntr_en) != 0UL)) {
			goto test_exit;
		} else {
			ERROR("pmu_ovf=%lx pmu_intr_en=%lx pmu_cntr_en=%lx\n",
				run->exit.pmu_ovf, run->exit.pmu_intr_en,
				run->exit.pmu_cntr_en);
		}
	} else {
		/* Check for exit reason */
		if (exit_reason == RMI_EXIT_HOST_CALL) {
			goto test_exit;
		}
	}

	if (exit_reason <= RMI_EXIT_SERROR) {
		ERROR("exit_reason RMI_EXIT_%s\n", rmi_exit[exit_reason]);
	} else {
		ERROR("exit_reason 0x%lx\n", exit_reason);
	}

	ret1 = false;

test_exit:
	ret2 = host_destroy_realm();

	if (!ret1 || !ret2) {
		ERROR("%s() enter=%u destroy=%u\n", __func__, ret1, ret2);
		return TEST_RESULT_FAIL;
	}

	return host_cmp_result();
}

test_result_t realm_pmuv3_cycle_works(void)
{
	return realm_pmuv3(REALM_PMU_CYCLE);
}

test_result_t realm_pmuv3_event_works(void)
{
	return realm_pmuv3(REALM_PMU_EVENT);
}

test_result_t realm_pmuv3_el3_preserves(void)
{
	return realm_pmuv3(REALM_PMU_PRESERVE);
}

/*
 * IRQ handler for PMU_PPI #23.
 * This handler should not be called, as RMM handles IRQs.
 */
static int overflow_interrupt(void *data)
{
	(void)data;

	assert(false);
	return -1;
}

test_result_t realm_pmuv3_overflow_interrupt(void)
{
	test_result_t res;

	/* Register PMU IRQ handler */
	int ret = tftf_irq_register_handler(PMU_PPI, overflow_interrupt);

	if (ret != 0) {
		tftf_testcase_printf("Failed to %sregister IRQ handler\n",
					"");
		return TEST_RESULT_FAIL;
	}

	tftf_irq_enable(PMU_PPI, GIC_HIGHEST_NS_PRIORITY);
	res = realm_pmuv3(REALM_PMU_INTERRUPT);

	tftf_irq_disable(PMU_PPI);
	ret = tftf_irq_unregister_handler(PMU_PPI);
	if (ret != 0) {
		tftf_testcase_printf("Failed to %sregister IRQ handler\n",
					"un");
		return TEST_RESULT_FAIL;
	}

	return res;
}

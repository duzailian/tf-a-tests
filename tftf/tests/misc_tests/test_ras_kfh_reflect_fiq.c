/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <arch_helpers.h>
#include <arm_arch_svc.h>
#include <drivers/arm/arm_gic.h>
#include <irq.h>
#include <platform.h>
#include <psci.h>
#include <serror.h>
#include <sgi.h>
#include <smccc.h>
#include <tftf_lib.h>

#ifdef __aarch64__
static volatile uint64_t serror_triggered;
static volatile uint64_t irq_triggered;
extern void inject_unrecoverable_ras_error(void);

static bool serror_handler(void)
{
	serror_triggered = 1;
	tftf_testcase_printf("SError event received.\n");
	return true;
}

static int irq_handler(void *data)
{
	irq_triggered = 1;
	tftf_testcase_printf("SError event received.\n");
	return true;
}

/*
 * Test Kernel First handling paradigm of RAS errors.
 *
 * Register a custom serror handler in tftf, disable SError so that it remains
 * pended when execution enters EL3 as part of SMC call. Inject a RAS error and
 * and give some time to ensure that it is triggered, make an SMC call.
 * EL3 as part of checking pending SError during SMC call will reflect this
 * error back to tftf along with enabling the SError(SPSR.A bit) in tftf as part
 * of patch in CI.
 * As soon as execution comes back to tftf it will take the SError and handles
 * it and continues with original SMC call. Ensure that SMCCC version requested
 * is valid one.
 */
test_result_t test_ras_kfh_reflect_fiq(void)
{
	smc_args args;
	unsigned int mpid = read_mpidr_el1();
        unsigned int core_pos = platform_get_core_pos(mpid);
        const unsigned int sgi_id = IRQ_NS_SGI_0;
        int ret;

	/* Get the version to compare against */
	memset(&args, 0, sizeof(args));
	args.fid = SMCCC_VERSION;
	tftf_smc(&args);

	register_custom_serror_handler(serror_handler);
	disable_serror();
	inject_unrecoverable_ras_error();
	waitms(500);

	ret = tftf_irq_register_handler(sgi_id, irq_handler);
	 if (ret != 0) {
                tftf_testcase_printf("Failed to register initial IRQ handler\n");
                return TEST_RESULT_FAIL;
        }
	tftf_irq_enable(sgi_id, GIC_HIGHEST_NS_PRIORITY);
	tftf_send_sgi(sgi_id, core_pos);

	if ((serror_triggered == false) || (irq_triggered == false)) {
		tftf_testcase_printf("SError or IRQ is not triggered\n");
		return TEST_RESULT_FAIL;
	}

	ret = tftf_irq_unregister_handler(sgi_id);
	if (ret != 0) {
		tftf_testcase_printf("Failed to unregister IRQ handler\n");
		return TEST_RESULT_FAIL;
	}

	unregister_custom_serror_handler();
	return TEST_RESULT_SUCCESS;
}
#else
test_result_t test_ras_kfh_reflect_fiq(void)
{
	tftf_testcase_printf("Not supported on AArch32.\n");
	return TEST_RESULT_SKIPPED;
}
#endif

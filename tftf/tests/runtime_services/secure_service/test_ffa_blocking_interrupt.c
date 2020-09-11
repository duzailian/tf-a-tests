/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cactus_test_cmds.h>
#include <ffa_endpoints.h>
#include <ffa_helpers.h>
#include <test_helpers.h>
#include <timer.h>

static volatile int timer_irq_received;

#define SENDER		HYP_ID
#define RECEIVER	SP_ID(1)
#define SLEEP_TIME	200U

/*
 * ISR for the timer interrupt. Update a global variable to check it has been
 * called.
 */
static int timer_handler(void *data)
{
	assert(timer_irq_received == 0);
	timer_irq_received = 1;
	return 0;
}

/*
 * @Test_Aim@ Test non-secure interrupts while executing Secure Partition.
 *
 * 1. Register a handler for the non-secure timer interrupt. Program it to fire
 *    in a certain time.
 *
 * 2. Send a blocking request to Cactus to execute in busy loop.
 *
 * 3. While executing in busy loop, the non-secure timer should
 *    fire and trap into SPM running at S-EL2 as FIQ.
 *
 * 4. SPM injects a managed exit vIRQ into running SP, causing SP to run its
 *    interrupt handler.
 *
 * 5. SP gets IRQ number by making GET_INTERRUPT hypervisor call, if the request
 *    is for managed exit, it saves its context and sends MSG_SEND_DIRECT_RESP
 *    ABI call to return to SPMC for interrupt to be handled in TFTF.
 *
 * 6. Also check whether the pending non-secure timer interrupt successfully got
 *    handled in TFTF.
 */
test_result_t test_ffa_blocking_interrupt(void)
{
	int ret;
	smc_ret_values ret_values;

	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 0);

	/* Program timer */
	timer_irq_received = 0;
	tftf_timer_register_handler(timer_handler);

	ret = tftf_program_timer(100);
	if (ret < 0) {
		tftf_testcase_printf("Failed to program timer (%d)\n", ret);
		return TEST_RESULT_FAIL;
	}

	/* Send request to primary Cactus to sleep for 200ms */
	ret_values = cactus_sleep_cmd(SENDER, RECEIVER, SLEEP_TIME);

	if (!is_ffa_direct_response(ret_values)) {
		ERROR("Failed to send message. error: %x\n",
		      ffa_error_code(ret_values));
	return TEST_RESULT_FAIL;
	}

	if (cactus_get_response(ret_values) == CACTUS_ERROR) {
		return TEST_RESULT_FAIL;
	}

	/* Check that the interrupt has been handled */
	tftf_cancel_timer();
	tftf_timer_unregister_handler();

	if (timer_irq_received == 0) {
		tftf_testcase_printf("Timer interrupt hasn't actually been handled.\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

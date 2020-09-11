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
 * 1. Enable managed exit interrupt by sending interrupt_enable command to
 *    Cactus.
 *
 * 2. Register a handler for the non-secure timer interrupt. Program it to fire
 *    in a certain time.
 *
 * 3. Send a blocking request to Cactus to execute in busy loop.
 *
 * 4. While executing in busy loop, the non-secure timer should
 *    fire and trap into SPM running at S-EL2 as FIQ.
 *
 * 5. SPM injects a managed exit virtual FIQ into Cactus, causing it to run
 *    its interrupt handler.
 *
 * 6. Cactus's managed exit handler will acknowledges interrupt arrival by returning
 *    its ID, check if it is MANAGED_EXIT_INTERRUPT_ID.
 *
 * 7. Check whether the pending non-secure timer interrupt successfully got
 *    handled in TFTF.
 *
 * 8. Send a FFA_RUN command to resume Cactus's execution, It will resume
 *    in its busy loop, after finishing its busy loop, it will return with
 *    DIRECT_RESPONSE. Check if time lapsed is greater than sleeping time.
 *
 */
test_result_t test_ffa_ns_interrupt(void)
{
	int ret;
	smc_ret_values ret_values;

	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 0);

	/* Enable Managed exit interrupt */
	ret_values = cactus_interrupt_cmd(SENDER, RECEIVER, MANAGED_EXIT_INTERRUPT_ID,
			                  INTERRUPT_TYPE_FIQ, true);

	if (!is_ffa_direct_response(ret_values)) {
		ERROR("Failed to send message. error: %x\n",
		      ffa_error_code(ret_values));
		return TEST_RESULT_FAIL;
	}

	if (cactus_get_response(ret_values) != CACTUS_SUCCESS) {
		ERROR("Failed to enable Managed exit interrupt\n");
		return TEST_RESULT_FAIL;
	}

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

	/*
	 * Managed exit interrupt occurs during this time, Cactus
	 * will respond with interrupt ID.
	 */
	if (cactus_get_response(ret_values) != MANAGED_EXIT_INTERRUPT_ID) {
		ERROR("Managed exit interrupt did not occur!\n");
		return TEST_RESULT_FAIL;
	}

	/* Check that the timer interrupt has been handled in NS-world (TFTF) */
	tftf_cancel_timer();
	tftf_timer_unregister_handler();

	if (timer_irq_received == 0) {
		tftf_testcase_printf("Timer interrupt hasn't actually been handled.\n");
		return TEST_RESULT_FAIL;
	}

	/* Resume Cactus, it will resume in its sleep routine */
	ret_values = ffa_run(RECEIVER, SENDER);

	if (!is_ffa_direct_response(ret_values)) {
		ERROR("Failed to send message. error: %x\n",
		      ffa_error_code(ret_values));
		return TEST_RESULT_FAIL;
	}

	/* Make sure elapsed time not less than sleep time */
	if (cactus_get_response(ret_values) < SLEEP_TIME) {
		ERROR("Lapsed time less than requested sleep time\n");
		return TEST_RESULT_FAIL;
	}

	/* Disable Managed exit interrupt */
	ret_values = cactus_interrupt_cmd(SENDER, RECEIVER, MANAGED_EXIT_INTERRUPT_ID,
					  INTERRUPT_TYPE_FIQ, false);


	if (!is_ffa_direct_response(ret_values)) {
		ERROR("Failed to send message. error: %x\n",
		      ffa_error_code(ret_values));
		return TEST_RESULT_FAIL;
	}

	if (cactus_get_response(ret_values) != CACTUS_SUCCESS) {
		ERROR("Failed to disable Managed exit interrupt\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

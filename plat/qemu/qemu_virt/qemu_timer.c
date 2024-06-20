/*
 * Copyright (c) 2018, Arm Limited. All rights reserved.
 * Copyright (c) 2024, Linaro Limited.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <platform.h>
#include <stddef.h>
#include <timer.h>

static int program_gentimer(unsigned long time_out_ms)
{
	unsigned int cntp_ctl;
	unsigned long long count_val;
	unsigned int freq;

	count_val = read_cntpct_el0();
	freq = read_cntfrq_el0();
	count_val += (freq * time_out_ms) / 1000;
	write_cntp_cval_el0(count_val);

	/* Enable the timer */
	cntp_ctl = read_cntp_ctl_el0();
	set_cntp_ctl_enable(cntp_ctl);
	clr_cntp_ctl_imask(cntp_ctl);
	write_cntp_ctl_el0(cntp_ctl);

	/*
	 * Ensure that we have programmed a timer interrupt for a time in
	 * future. Else we will have to wait for the gentimer to rollover
	 * for the interrupt to fire (which is 64 years).
	 */
	if (count_val < read_cntpct_el0())
		panic();

	VERBOSE("%s : interrupt requested at sys_counter: %llu "
		"time_out_ms: %ld\n", __func__, count_val, time_out_ms);

	return 0;
}

static void disable_gentimer(void)
{
	uint32_t val;

	/* Deassert and disable the timer interrupt */
	val = 0;
	set_cntp_ctl_imask(val);
	write_cntp_ctl_el0(val);
}

static int cancel_gentimer(void)
{
	disable_gentimer();
	return 0;
}

static int handler_gentimer(void)
{
	disable_gentimer();
	return 0;
}

static int init_gentimer(void)
{
	/* Disable the timer as the reset value is unknown */
	disable_gentimer();

	/* Initialise CVAL to zero */
	write_cntp_cval_el0(0);

	return 0;
}

static const plat_timer_t plat_timers = {
	.program = program_gentimer,
	.cancel = cancel_gentimer,
	.handler = handler_gentimer,
	.timer_step_value = 2,
	.timer_irq = IRQ_PCPU_EL1_TIMER
};

int plat_initialise_timer_ops(const plat_timer_t **timer_ops)
{
	assert(timer_ops != NULL);
	*timer_ops = &plat_timers;

	/* Initialise the generic timer */
	init_gentimer();

	return 0;
}


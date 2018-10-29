/*
 * Copyright (c) 2018, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <mmio.h>
#include <platform.h>
#include <platform_def.h>
#include <sp804.h>
#include <stddef.h>
#include <timer.h>

static const plat_timer_t plat_timers = {
	.program = sp804_timer_program,
	.cancel = sp804_timer_cancel,
	.handler = sp804_timer_handler,
	.timer_step_value = 2,
	.timer_irq = TIMER0_IRQ /* SP804 IRQ */
};

int plat_initialise_timer_ops(const plat_timer_t **timer_ops)
{
	assert(timer_ops != NULL);
	*timer_ops = &plat_timers;

	/* Initialise the system timer */
	sp804_timer_init(TIMER0_BASE, TIMER0_FREQ);

	return 0;
}

unsigned long long plat_get_current_time_ms(void)
{
	assert(systicks_per_ms);
	return mmio_read_32(TIMER0_BASE + TIMER_VALREG_OFFSET) /
		systicks_per_ms;
}

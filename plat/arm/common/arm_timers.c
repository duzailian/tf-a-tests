/*
 * Copyright (c) 2018, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <mmio.h>
#include <platform.h>
#include <stddef.h>
#include <system_timer.h>
#include <timer.h>

#pragma weak plat_initialise_timer_ops

static const plat_timer_t plat_timers = {
	.program = program_systimer,
	.cancel = cancel_systimer,
	.handler = handler_systimer,
	.timer_step_value = 2,
	.timer_irq = IRQ_CNTPSIRQ1
};

int plat_initialise_timer_ops(const plat_timer_t **timer_ops)
{
	assert(timer_ops != NULL);
	*timer_ops = &plat_timers;

	/* Initialise the system timer */
	init_systimer(SYS_CNT_BASE1);

	return 0;
}

unsigned long long plat_get_current_time_ms(void)
{
	assert(systicks_per_ms);
	return mmio_read_64(SYS_CNT_BASE1 + CNTPCT_LO) / systicks_per_ms;
}

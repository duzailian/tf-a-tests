/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>

#include <context.h>
#include <ffa_helpers.h>
#include <sp_helpers.h>
#include <spm_helpers.h>
#include <drivers/arm/sp805.h>
#include <platform.h>
#include <platform_def.h>

#include "cactus_test_cmds.h"
#include "cactus_message_loop.h"
#include "spm_common.h"

extern ffa_id_t g_ffa_id;

#define DEFERRED_INT_ID 0xFFFF

static void managed_exit_handler(void)
{
	/*
	 * Real SP will save its context here.
	 * Send interrupt ID for acknowledgement
	 */
	cactus_response(g_ffa_id, HYP_ID, MANAGED_EXIT_INTERRUPT_ID);
}

static void secure_interrupt_handler(uint32_t intid)
{
	switch(intid) {
	case IRQ_TWDOG_INTID:
		/*
		 * Interrupt triggered due to Trusted watchdog timer expiry.
		 * Clear the interrupt and stop the timer.
		 */
		NOTICE("Trusted WatchDog timer stopped\n");
		sp805_twdog_stop();
		break;
	default:
		/*
		 * Currently the only source of secure interrupt is Trusted
		 * Watchdog timer.
		 */
		ERROR("%s: Interrupt ID %x not handled!\n", __func__,
			 intid);
		panic();
	}

	/* Perform de-activation of secure interrupt. */
	spm_interrupt_deactivate(intid, intid);
}

smc_ret_values handle_interrupt(uint32_t int_id)
{
	uint32_t para_virtualized_irq;

	/*
	 * spm_interrupt_get() invocation is necessary to advance the interrupt
	 * state machine maintained by Hafnium(SPM) for virtual interrupts.
	 */
	para_virtualized_irq = spm_interrupt_get();

	/* SP is in BLOCKED state */
	if (int_id == DEFERRED_INT_ID) {
		unsigned int mpid = read_mpidr_el1() & MPID_MASK;
		unsigned int core_pos = platform_get_core_pos(mpid);

		secure_interrupt_handler(para_virtualized_irq);

		/* Perform signal completion.
		 * TODO: Find the target of current blocked request */
		return ffa_run(SP_ID(2), core_pos);
	} else {
		/*
		 * When SP is in WAITING state, SPM passes interrupt id as
		 * part of FFA_INTERRUPT ABI call.
		 */
		secure_interrupt_handler(int_id);

		/* Perform signal completion */
		return ffa_msg_wait();
	}
}

int cactus_irq_handler(struct ctx_regs *context)
{
	smc_ret_values ffa_ret;
	uint32_t irq_num;

	save_context_ffa(&ffa_ret, context);
	if (ffa_func_id(ffa_ret) == FFA_INTERRUPT) {
		irq_num = ffa_interrupt_id(ffa_ret);
		ffa_ret = handle_interrupt(irq_num);
		restore_context_ffa(ffa_ret, context);
	} else {
		irq_num = spm_interrupt_get();
		secure_interrupt_handler(irq_num);
	}
		return 0;
}

int cactus_fiq_handler(void)
{
	uint32_t fiq_num;

	fiq_num = spm_interrupt_get();

	if (fiq_num == MANAGED_EXIT_INTERRUPT_ID) {
		managed_exit_handler();
	} else {
		ERROR("%s: Interrupt ID %u not handled!\n", __func__, fiq_num);
	}

	return 0;
}

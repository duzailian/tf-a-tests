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
	/*
	 * Currently the only source of secure interrupt is Trusted Watchdog
	 * timer. Interrupt triggered due to Trusted watchdog timer expiry.
	 * Clear the interrupt and stop the timer.
	 */
	 NOTICE("Trusted WatchDog timer stopped\n");

	sp805_twdog_stop();

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

	ffa_ret.ret0 = context->gp_regs[0];
	ffa_ret.ret1 = context->gp_regs[1];
	ffa_ret.ret2 = context->gp_regs[2];
	ffa_ret.ret3 = context->gp_regs[3];
	ffa_ret.ret4 = context->gp_regs[4];
	ffa_ret.ret5 = context->gp_regs[5];
	ffa_ret.ret6 = context->gp_regs[6];
	ffa_ret.ret7 = context->gp_regs[7];

	if (ffa_ret.ret0 == FFA_INTERRUPT) {
		irq_num = (ffa_ret.ret1);
		ffa_ret = handle_interrupt(irq_num);
		context->gp_regs[0] = ffa_ret.ret0;
		context->gp_regs[1] = ffa_ret.ret1;
		context->gp_regs[2] = ffa_ret.ret2;
		context->gp_regs[3] = ffa_ret.ret3;
		context->gp_regs[4] = ffa_ret.ret4;
		context->gp_regs[5] = ffa_ret.ret5;
		context->gp_regs[6] = ffa_ret.ret6;
		context->gp_regs[7] = ffa_ret.ret7;
	} else {
		irq_num = spm_interrupt_get();

		if (irq_num == IRQ_TWDOG_INTID){
			secure_interrupt_handler(irq_num);
		} else {
			ERROR("%s: Interrupt ID %x not handled!\n", __func__, irq_num);
			panic();
		}
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

/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SPMC_H
#define SPMC_H

#include <context.h>
#include <ffa_helpers.h>
#include <spm_common.h>

/* Should match with IDs defined in SPM/Hafnium */
#define SPM_INTERRUPT_ENABLE            (0xFF03)
#define SPM_INTERRUPT_GET               (0xFF04)
#define SPM_INTERRUPT_DEACTIVATE	(0xFF08)
#define SPM_DEBUG_LOG                   (0xBD000000)

/*
 * Hypervisor Calls Wrappers
 */

uint32_t spm_interrupt_get(void);
int64_t spm_interrupt_enable(uint32_t int_id, bool enable, enum interrupt_pin pin);
int64_t spm_interrupt_deactivate(uint32_t pint_id, uint32_t vint_id);
void spm_debug_log(char c);
smc_ret_values handle_interrupt(uint32_t int_id);

static inline void save_context_ffa(smc_ret_values *ffa_ret,
			struct ctx_regs *context)
{
	ffa_ret->ret0 = context->gp_regs[0];
	ffa_ret->ret1 = context->gp_regs[1];
	ffa_ret->ret2 = context->gp_regs[2];
	ffa_ret->ret3 = context->gp_regs[3];
	ffa_ret->ret4 = context->gp_regs[4];
	ffa_ret->ret5 = context->gp_regs[5];
	ffa_ret->ret6 = context->gp_regs[6];
	ffa_ret->ret7 = context->gp_regs[7];
}

static inline void restore_context_ffa(smc_ret_values ffa_ret,
			struct ctx_regs *context)
{
	context->gp_regs[0] = ffa_ret.ret0;
	context->gp_regs[1] = ffa_ret.ret1;
	context->gp_regs[2] = ffa_ret.ret2;
	context->gp_regs[3] = ffa_ret.ret3;
	context->gp_regs[4] = ffa_ret.ret4;
	context->gp_regs[5] = ffa_ret.ret5;
	context->gp_regs[6] = ffa_ret.ret6;
	context->gp_regs[7] = ffa_ret.ret7;
}
#endif /* SPMC_H */

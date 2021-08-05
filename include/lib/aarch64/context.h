/*
 * Copyright (c) 2021, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CONTEXT_H
#define CONTEXT_H

#include <utils_def.h>

#ifndef __ASSEMBLER__

struct ctx_regs {
	unsigned long gp_regs[32]; //x0-x30 and xzr
	unsigned long elr_reg;
	unsigned long spsr_reg;
};

#endif /* __ASSEMBLER__ */

#define CTX_REGS_X0		U(0x0)
#define CTX_REGS_X1		U(0x8)
#define CTX_REGS_X2		U(0x10)
#define CTX_REGS_X3		U(0x18)
#define CTX_REGS_X4		U(0x20)
#define CTX_REGS_X5		U(0x28)
#define CTX_REGS_X6		U(0x30)
#define CTX_REGS_X7		U(0x38)
#define CTX_REGS_X8		U(0x40)
#define CTX_REGS_X9		U(0x48)
#define CTX_REGS_X10		U(0x50)
#define CTX_REGS_X11		U(0x58)
#define CTX_REGS_X12		U(0x60)
#define CTX_REGS_X13		U(0x68)
#define CTX_REGS_X14		U(0x70)
#define CTX_REGS_X15		U(0x78)
#define CTX_REGS_X16		U(0x80)
#define CTX_REGS_X17		U(0x88)
#define CTX_REGS_X18		U(0x90)
#define CTX_REGS_X19		U(0x98)
#define CTX_REGS_X20		U(0xa0)
#define CTX_REGS_X21		U(0xa8)
#define CTX_REGS_X22		U(0xb0)
#define CTX_REGS_X23		U(0xb8)
#define CTX_REGS_X24		U(0xc0)
#define CTX_REGS_X25		U(0xc8)
#define CTX_REGS_X26		U(0xd0)
#define CTX_REGS_X27		U(0xd8)
#define CTX_REGS_X28		U(0xe0)
#define CTX_REGS_X29		U(0xe8)
#define CTX_REGS_LR		U(0xf0)
#define CTX_REGS_XZR		U(0xf8)
#define CTX_REGS_ELR_EL1	U(0x100)
#define CTX_REGS_SPSR_EL1	U(0x108)
#define CTX_REGS_END		U(0x110)

#endif /* CONTEXT_H */

/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FPU_H
#define FPU_H

/* The FPU and SIMD register bank is 32 quadword (128 bits) Q registers. */
#define FPU_Q_SIZE		16U
#define FPU_Q_COUNT		32U

/* These defines are needed by assembly code to access FPU registers. */
#define FPU_OFFSET_Q		0U
#define FPU_OFFSET_FPSR		(FPU_Q_SIZE * FPU_Q_COUNT)
#define FPU_OFFSET_FPCR		(FPU_OFFSET_FPSR + 8)

#ifndef __ASSEMBLER__

#include <stdint.h>

typedef struct fpu_reg_state {
#ifdef __aarch64__
	unsigned __int128 q[FPU_Q_COUNT];
#else
	unsigned char q[FPU_Q_SIZE * FPU_Q_COUNT];
#endif
	unsigned long fpsr;
	unsigned long fpcr;
} fpu_reg_state_t;

/*
 * This assembly function copy data from the provided FPU structure to the
 * core's FPU registers
 */
extern void fill_fpu_state_registers(fpu_reg_state_t *fpu);

/*
 * This assembly function read data from the core's FPU registers to the
 * provided FPU structure.
 */
extern void read_fpu_state_registers(fpu_reg_state_t *fpu);
/*
 * This assembly function read data from the core's FPU registers FPCR FPSR to the
 * provided FPU structure offset.
 */
extern void read_fpu_state_registers_fpcr_fpsr(void *fpu);

/*
 * Read and compare FPU state registers with provided template values in parameters.
 */
bool fpu_state_compare_template(fpu_reg_state_t *fpu);

/*
 * Fill FPU state registers(SIMD vectors, FPCR, FPSR) with provided
 * template values in parameters.
 */
fpu_reg_state_t fpu_state_write_template(uint8_t regs_val);

/*
 * This function populates the provided FPU structure with the provided template
 * regs_val for all the 32 FPU/SMID registers, and the status registers FPCR/FPSR
 */
void fpu_state_set(fpu_reg_state_t *vec,
		uint8_t regs_val);

/*
 * This function prints the content of the provided FPU structure
 */
void fpu_state_print(fpu_reg_state_t *vec);

#endif /* __ASSEMBLER__ */
#endif /* FPU_H */

/*
 * Copyright (c) 2021-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SVE_H
#define SVE_H

#include <stdbool.h>
#include <stdint.h>

/*
 * SVE vector length in bytes and derived values
 * Max. vector length permitted by the architecture:
 * SVE:	 2048 bits = 256 bytes
 */
#define SVE_VLA_ZCR_LEN_BITS	UL(4)
#define SVE_VLA_LEN_MAX		(UL(1) << SVE_VLA_ZCR_LEN_BITS)
#define SVE_VLA_ZCR_LEN_MAX	(SVE_VLA_LEN_MAX - UL(1))
#define SVE_VLA_VL_MIN_Z	UL(16)
#define SVE_VLA_VL_MIN_P	UL(2)
#define SVE_VLA_VL_MAX_Z	(SVE_VLA_VL_MIN_Z * SVE_VLA_LEN_MAX)
#define SVE_VLA_VL_MAX_P	(SVE_VLA_VL_MIN_P * SVE_VLA_LEN_MAX)
#define SVE_VLA_Z_REGS_MAX	UL(32)
#define SVE_VLA_P_REGS_MAX	UL(16)
#define SVE_VLA_FFR_REGS_MAX	UL(1)
#define SVE_VLA_Z_LEN_MAX	(SVE_VLA_VL_MAX_Z * SVE_VLA_Z_REGS_MAX)
#define SVE_VLA_P_LEN_MAX	(SVE_VLA_VL_MAX_P * SVE_VLA_P_REGS_MAX)
#define SVE_VLA_FFR_LEN_MAX	(SVE_VLA_VL_MAX_P * SVE_VLA_FFR_REGS_MAX)
#define SVE_ZCR_FP_REGS_NUM	UL(4)
#define SVE_VLA_VL_Z(X)		(SVE_VLA_VL_MIN_Z * (X+1))
#define SVE_VLA_VL_P(X)		(SVE_VLA_VL_MIN_P * (X+1))

#define fill_sve_helper(num)	"ldr z"#num", [%0, #"#num", MUL VL];"
#define read_sve_helper(num)	"str z"#num", [%0, #"#num", MUL VL];"

/*
 * Converts SVE vector size restriction in bytes to LEN according to ZCR_EL3 documentation.
 * VECTOR_SIZE = (LEN+1) * 128
 */
#define CONVERT_SVE_LENGTH(x)	(((x / 128) - 1))

#ifndef __ASSEMBLY__

#ifdef __aarch64__

/* Returns the SVE implemented VL in bytes (constrained by ZCR_EL3.LEN) */
static inline uint64_t sve_vector_length_get(void)
{
	uint64_t vl;

	__asm__ volatile(
		".arch_extension sve\n"
		"rdvl %0, #1;"
		".arch_extension nosve\n"
		: "=r" (vl)
	);

	return vl;
}

#endif /* __aarch64__ */

/*
 * SVE status comparison's results.
 */
typedef enum {
	SVE_STATE_SUCCESS = 0,
	Z_REG_ERROR,
	P_REG_ERROR,
	FFR_REG_ERROR,
	ZCR_ELR1_ERROR,
	ZCR_ELR2_ERROR,
	FPSR_REG_ERROR,
	FPCR_REG_ERROR,
	SVE_STATE_ERROR,
} sve_compare_result_t;

/*
 * SVE context structure.
 */
struct sve_state {
	union {
		unsigned long zcr_fpu[SVE_ZCR_FP_REGS_NUM];
		struct {
			unsigned long fpsr;
			unsigned long fpcr;
			unsigned long zcr_el1;
			unsigned long zcr_el2;
		};
	};
	unsigned char z[SVE_VLA_Z_LEN_MAX];
	union {
		struct {
			unsigned char p[SVE_VLA_P_LEN_MAX];
			unsigned char ffr[SVE_VLA_FFR_LEN_MAX];
			//we need one dummy FFR space to save/restore P0
			unsigned char dummy[SVE_VLA_FFR_LEN_MAX];
		};
	};
} __attribute__((aligned(SVE_VLA_VL_MAX_Z)));

/* Read from SVE registers ZCR_EL1/ZCR_EL2 and FPU status FPCR/FPSR */
void read_sve_zcr_fpu_state(unsigned long *data);
/* Read from SVE Z registers */
void read_sve_z_state(unsigned char *data);

/* Read from SVE P registers */
void read_sve_p_state(unsigned char *data);

/* Read from SVE FFR register */
void read_sve_ffr_state(unsigned char *data);

/* Write to SVE Z registers provided from the pinter offset *data */
void write_sve_zcr_fpu_state(unsigned long *data);

/* Write to SVE Z registers */
void write_sve_z_state(unsigned char *data);

/* Write to SVE P registers */
void write_sve_p_state(unsigned char *data);

/* Write to SVE FFR register */
void write_sve_ffr_state(unsigned char *data);



/*
 * Read the SVE context from registers to memory "struct sve_state *sve".
 * The function reads the maximum implemented SVE context size.
 */
void read_sve_state(struct sve_state *sve);

/*
 * Write data from the memory "struct sve_state *sve" to the SVE registers.
 * The function writes the maximum implemented SVE context size.
 */
void write_sve_state(struct sve_state *sve);

/* Print SVE registers Z, P, FFR, ZCR_EL1/ZCR_EL2 and FPU status FPCR/FPSR */
void sve_state_print(struct sve_state *sve);

/* Set SVE registers Z, P, FFR, ZCR_EL1/ZCR_EL2 and FPU status FPCR/FPSR */
void sve_state_set(struct sve_state *sve,
			uint8_t z_regs,
			uint8_t p_regs,
			uint32_t fpsr_val,
			uint32_t fpcr_val,
			uint32_t zcr_el1,
			uint32_t zcr_el2,
			uint8_t ffr_regs);

/*
 * Read current state of SVE registers and compare with the provided parameter
 * strut sve_state sve_compare
 */
sve_compare_result_t sve_read_compare(const struct sve_state *sve_compare,
		struct sve_state *sve_read);

/*
 * Read current state of SVE registers and compare with the provided SVE
 * template values
 */
bool sve_state_compare_template(uint8_t z_regs,
			uint8_t p_regs,
			uint32_t fpsr_val,
			uint32_t fpcr_val,
			uint32_t zcr_el1,
			uint32_t zcr_el2,
			uint8_t ffr_regs);

/*
 * Fill SVE registers Z, P, FFR, ZCR_EL1/ZCR_EL2 and FPU status FPCR/FPSR
 * with provided template values in parameters.
 */
void sve_state_write_template(uint8_t z_regs,
		uint8_t p_regs,
		uint32_t fpsr_val,
		uint32_t fpcr_val,
		uint32_t zcr_el1,
		uint32_t zcr_el2,
		uint8_t ffr_regs);

#endif /* __ASSEMBLY__ */

#endif /* __SVE_H_ */

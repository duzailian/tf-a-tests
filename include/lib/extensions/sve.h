/*
 * Copyright (c) 2021-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SVE_H
#define SVE_H

#include <arch.h>
#include <utils_def.h>

/*
 * SVE vector length in bytes and derived values
 */
#define SVE_VL	UL(3)
#define SVE_VLA_ZCR_LEN_BITS	UL(4)
#define SVE_VLA_LEN_MAX		(UL(1) << SVE_VLA_ZCR_LEN_BITS)
#define SVE_VLA_ZCR_LEN_MAX	(SVE_VLA_LEN_MAX - UL(1))
#define SVE_VLA_VL_MIN_Z		UL(16)//128
#define SVE_VLA_VL_MIN_P		UL(2)//16
#define SVE_VLA_VL_MAX_Z		(SVE_VLA_VL_MIN_Z * SVE_VLA_LEN_MAX)//16*16  = 256 byte = 2048 bits
#define SVE_VLA_VL_MAX_P		(SVE_VLA_VL_MIN_P * SVE_VLA_LEN_MAX)//2*16 = 32 byte
#define SVE_VLA_Z_REGS_MAX	UL(32)
#define SVE_VLA_P_REGS_MAX	UL(16)
#define SVE_VLA_FFR_REGS_MAX	UL(1)
#define SVE_VLA_Z_LEN_MAX	(SVE_VLA_VL_MAX_Z * SVE_VLA_Z_REGS_MAX)
#define SVE_VLA_P_LEN_MAX	(SVE_VLA_VL_MAX_P * SVE_VLA_P_REGS_MAX)
#define SVE_VLA_FFR_LEN_MAX	(SVE_VLA_VL_MAX_P * SVE_VLA_FFR_REGS_MAX)
#define SVE_ZCR_FP_REGS_NUM	UL(4)
#define SVE_VLA_VL_Z(X)		(SVE_VLA_VL_MIN_Z * (X+1))
#define SVE_VLA_VL_P(X)		(SVE_VLA_VL_MIN_P * (X+1))

#define fill_sve_helper(num) "ldr z"#num", [%0, #"#num", MUL VL];"
#define read_sve_helper(num) "str z"#num", [%0, #"#num", MUL VL];"

/*
 * Converts SVE vector size restriction in bytes to LEN according to ZCR_EL3 documentation.
 * VECTOR_SIZE = (LEN+1) * 128
 */
#define CONVERT_SVE_LENGTH(x)	(((x / 128) - 1))

/*
 * Max. vector length permitted by the architecture:
 * SVE:	 2048 bits = 256 bytes
 */
#define SVE_VECTOR_LEN_BYTES		256
#define SVE_NUM_VECTORS			32

#ifndef __ASSEMBLY__

typedef uint8_t sve_vector_t[SVE_VECTOR_LEN_BYTES];

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
 * SVE context structure.
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
}sve_compare_result_t;

struct sve_state {
	union
	{
		unsigned long zcr_fpu[SVE_ZCR_FP_REGS_NUM];
		struct
		{
			unsigned long fpsr;
			unsigned long fpcr;
			unsigned long zcr_el1;
			unsigned long zcr_el2;
		};
	};
	unsigned char z[SVE_VLA_Z_LEN_MAX];
	union
	{
		struct
		{
			unsigned char p[SVE_VLA_P_LEN_MAX];
			unsigned char ffr[SVE_VLA_FFR_LEN_MAX];
			//we need one dummy FFR space to save/restore P0
			unsigned char dummy[SVE_VLA_FFR_LEN_MAX];
		};
	};
}__attribute__((aligned(UL(256))));

void read_sve_zcr_fpu_state(unsigned long *data);
void read_sve_z_state(unsigned char *data);
void read_sve_p_state(unsigned char *data);

void write_sve_zcr_fpu_state(unsigned long *data);
void write_sve_z_state(unsigned char *data);
void write_sve_p_state(unsigned char *data);

void read_sve_state(struct sve_state *sve);
void write_sve_state(struct sve_state *sve);
void sve_state_print(struct sve_state *sve);
void sve_state_set(struct sve_state *sve,
						uint8_t z_regs,
						uint8_t p_regs,
						uint32_t fpsr_val,
						uint32_t fpcr_val,
						uint32_t zcr_el1,
						uint32_t zcr_el2,
						uint8_t ffr_regs);
void write_sve_ffr_state(unsigned char *data);
void read_sve_ffr_state(unsigned char *data);
void disable_sve_traps_maximise_vl();
void maximise_vl_zcr_elx();
void restore_back_zcr_elx();
void write_sve_z_value();
void read_sve_z_value();
uint32_t sve_probe_vl_max(void);
sve_compare_result_t sve_read_compare(const struct sve_state *sve_state_send,
		struct sve_state *sve_state_receive,
		int* index);

#endif /* __ASSEMBLER__ */

#endif /* __SVE_H_ */

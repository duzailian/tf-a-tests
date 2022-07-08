/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SVE_H
#define SVE_H

#define fill_sve_helper(num) "ldr z"#num", [%0, #"#num", MUL VL];"
#define read_sve_helper(num) "str z"#num", [%0, #"#num", MUL VL];"

/* Returns the SVE implemented VL in bytes (constrained by ZCR_EL3.LEN) */
static inline uint64_t sve_vector_length_get(void)
{
	uint64_t vl;

	__asm__ volatile("rdvl %0, #1" : "=r" (vl));

	return vl;
}

#endif /* SVE_H */

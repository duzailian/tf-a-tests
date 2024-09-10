/*
 * Copyright (c) 2022-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef REALM_DEF_H
#define REALM_DEF_H

#include <xlat_tables_defs.h>

/* 1MB for Realm payload as a default value */
#define REALM_MAX_LOAD_IMG_SIZE		U(0x100000)
#define REALM_STACK_SIZE		0x1000U
#define DATA_PATTERN_1			0x12345678U
#define DATA_PATTERN_2			0x11223344U
#define REALM_SUCCESS			0U
#define REALM_ERROR			1U
#define MAX_REC_COUNT			8U
#define MAX_REALM_COUNT			U(2)

/* Only support 4KB at the moment */

#if (PAGE_SIZE == PAGE_SIZE_4KB)
#define PAGE_ALIGNMENT			PAGE_SIZE_4KB
#define TCR_TG0				TCR_TG0_4K
#else
#error "Undefined value for PAGE_SIZE"
#endif

/*
 * MPIDR_EL1 definitions
 * 'MPIDR_EL1_AFF<n>_VAL_SHIFT' constants specify the right shift
 * for affinity field <n> that gives the field's actual value.
 */

/* Aff0[3:0] - Affinity level 0
 * For compatibility with GICv3 only Aff0[3:0] field is used,
 * and Aff0[7:4] of MPIDR_EL1 value is RES0 to match RmiRecMpidr.
 */
#define MPIDR_EL1_AFF0_SHIFT		0
#define MPIDR_EL1_AFF0_WIDTH		U(4)
#define MPIDR_EL1_AFF0_VAL_SHIFT	0

/* Aff1[15:8] - Affinity level 1 */
#define MPIDR_EL1_AFF1_SHIFT		8
#define MPIDR_EL1_AFF1_WIDTH		U(8)
#define MPIDR_EL1_AFF1_VAL_SHIFT	4

/* Aff2[23:16] - Affinity level 2 */
#define MPIDR_EL1_AFF2_SHIFT		16
#define MPIDR_EL1_AFF2_WIDTH		U(8)
#define MPIDR_EL1_AFF2_VAL_SHIFT	4

/* Aff3[39:32] - Affinity level 3 */
#define MPIDR_EL1_AFF3_SHIFT		32
#define MPIDR_EL1_AFF3_WIDTH		U(8)
#define MPIDR_EL1_AFF3_VAL_SHIFT	12

/* Aff20[23:0] - Affinity levels 2-0 mask */
#define MPIDR_AFF20_MASK					\
		((MPIDR_AFFLVL_MASK << MPIDR_AFF0_SHIFT) |	\
		 (MPIDR_AFFLVL_MASK << MPIDR_AFF1_SHIFT) |	\
		 (MPIDR_AFFLVL_MASK << MPIDR_AFF2_SHIFT))

/*
 * Extract the value of MPIDR_EL1.Aff<n> register field shifted right
 * so it can be evaluated directly.
 */
#define MPIDR_EL1_AFF(n, reg)	\
	(((reg) & MASK(MPIDR_EL1_AFF##n)) >> MPIDR_EL1_AFF##n##_VAL_SHIFT)

/* Calculate REC index from mpidr of MPIDR_EL1 register type */
#define REC_IDX(mpidr)				\
		(MPIDR_EL1_AFF(0, mpidr) |	\
		 MPIDR_EL1_AFF(1, mpidr) |	\
		 MPIDR_EL1_AFF(2, mpidr) |	\
		 MPIDR_EL1_AFF(3, mpidr))

/*
 * RmiRecMpidr type definitions.
 *
 * Aff0[3:0] - Affinity level 0
 * For compatibility with GICv3 only Aff0[3:0] field is used,
 * and Aff0[7:4] of a REC MPIDR value is RES0.
 */
#define RMI_MPIDR_AFF0_SHIFT		0
#define RMI_MPIDR_AFF0_WIDTH		U(4)

/* Aff1[15:8] - Affinity level 1 */
#define RMI_MPIDR_AFF1_SHIFT		8
#define RMI_MPIDR_AFF1_WIDTH		U(8)

/* Aff2[23:16] - Affinity level 2 */
#define RMI_MPIDR_AFF2_SHIFT		16
#define RMI_MPIDR_AFF2_WIDTH		U(8)

/* Aff3[31:24] - Affinity level 3 */
#define RMI_MPIDR_AFF3_SHIFT		24
#define RMI_MPIDR_AFF3_WIDTH		U(8)

/* Convert REC linear index to RmiRecMpidr */
#define RMI_REC_MPIDR(idx)				\
		(((idx) & MASK(RMI_MPIDR_AFF0)) |	\
		(((idx) & ~MASK(RMI_MPIDR_AFF0)) << 	\
		(RMI_MPIDR_AFF1_SHIFT - RMI_MPIDR_AFF0_WIDTH)));

#endif /* REALM_DEF_H */

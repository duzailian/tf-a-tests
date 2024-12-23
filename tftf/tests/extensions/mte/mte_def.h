/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MTE_DEF_H
#define MTE_DEF_H

/* MTE Hardware feature definitions below. */
#define MT_TAG_SHIFT		56
#define MT_TAG_MASK		0xFUL
#define MT_FREE_TAG		0x0UL
#define MT_GRANULE_SIZE         16
#define MT_TAG_COUNT		16
#define MT_INCLUDE_TAG_MASK	0xFFFF
#define MT_EXCLUDE_TAG_MASK	0x0

#define MT_ALIGN_GRANULE	(MT_GRANULE_SIZE - 1)
#define MT_CLEAR_TAG(x)		((x) & ~(MT_TAG_MASK << MT_TAG_SHIFT))
#define MT_SET_TAG(x, y)	((x) | (y << MT_TAG_SHIFT))
#define MT_FETCH_TAG(x)		((x >> MT_TAG_SHIFT) & (MT_TAG_MASK))
#define MT_ALIGN_UP(x)		((x + MT_ALIGN_GRANULE) & ~(MT_ALIGN_GRANULE))

#define MT_PSTATE_TCO_SHIFT	25
#define MT_PSTATE_TCO_MASK	~(0x1 << MT_PSTATE_TCO_SHIFT)
#define MT_PSTATE_TCO_EN	1
#define MT_PSTATE_TCO_DIS	0

#define MT_EXCLUDE_TAG(x)		(1 << (x))
#define MT_INCLUDE_VALID_TAG(x)		(MT_INCLUDE_TAG_MASK ^ MT_EXCLUDE_TAG(x))
#define MT_INCLUDE_VALID_TAGS(x)	(MT_INCLUDE_TAG_MASK ^ (x))
#define MTE_ALLOW_NON_ZERO_TAG		MT_INCLUDE_VALID_TAG(0)

#endif /* MTE_DEF_H */

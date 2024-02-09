/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLATFORM_DEF_H
#define PLATFORM_DEF_H

/* Platform specific page table and MMU setup constants */
#define PLAT_PHY_ADDR_SPACE_SIZE	(1ULL << 48)
#define PLAT_VIRT_ADDR_SPACE_SIZE	(1ULL << 48)

/* For testing xlat tables lib v2 */
#define MAX_XLAT_TABLES			10
#define MAX_MMAP_REGIONS		10

#endif /* PLATFORM_DEF_H */

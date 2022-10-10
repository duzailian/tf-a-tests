/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef REALM_DEF_H
#define REALM_DEF_H

#include <xlat_tables_defs.h>

/* 1mb for Realm payload as a default value*/
#ifdef ENABLE_REALM
 #define REALM_MAX_LOAD_IMG_SIZE	U(0x100000)
#else
 #define REALM_MAX_LOAD_IMG_SIZE	0U
#endif

#define REALM_STACK_SIZE		0x1000U
#define DATA_PATTERN_1			0x12345678U
#define DATA_PATTERN_2			0x11223344U
#define REALM_SUCCESS			0U
#define REALM_ERROR			1U

/*
 * Realm test image is position independent and can be loaded at any address.
 * Realm images have been tested using 4KB translation granule, however,
 * it is possible to configure MMU with any of supported translation granule
 * size 4KB, 16KB and 64KB.
 *
 * Also the image size must be equal to 1MB when 4KB translation granule used.
 * The image size requirement could be more in the case of 16KB or 64KB
 * translation granule.
 */

#if (PAGE_SIZE == PAGE_SIZE_4KB)
#define PAGE_ALIGNMENT			PAGE_SIZE_4KB
#define TCR_TG0				TCR_TG0_4K
#else
#error "Undefined value for PAGE_SIZE"
#endif

#define VAL_SWITCH_TO_HOST_SUCCESS	5U
#define VAL_SWITCH_TO_HOST_ERROR	6U
#define VAL_INVALID_HVC			0xFFFFFU

#endif /* REALM_DEF_H */

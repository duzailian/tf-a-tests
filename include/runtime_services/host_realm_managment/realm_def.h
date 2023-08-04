/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef REALM_DEF_H
#define REALM_DEF_H

#include <xlat_tables_defs.h>

/* 1MB for Realm payload as a default value */
#define REALM_MAX_LOAD_IMG_SIZE		U(0x100000)

#ifdef ENABLE_REALM_PAYLOAD_TESTS
 /* 1MB for shared buffer between Realm and Host */
 #define NS_REALM_SHARED_MEM_SIZE       U(0x100000)
 /* 3MB of memory used as a pool for realm's objects creation */
 #define PAGE_POOL_MAX_SIZE             U(0x300000)
#else
 #define NS_REALM_SHARED_MEM_SIZE       U(0x0)
 #define PAGE_POOL_MAX_SIZE             U(0x0)
#endif

#define REALM_STACK_SIZE		0x1000U
#define DATA_PATTERN_1			0x12345678U
#define DATA_PATTERN_2			0x11223344U
#define REALM_SUCCESS			0U
#define REALM_ERROR			1U

/* Only support 4KB at the moment */

#if (PAGE_SIZE == PAGE_SIZE_4KB)
#define PAGE_ALIGNMENT			PAGE_SIZE_4KB
#define TCR_TG0				TCR_TG0_4K
#else
#error "Undefined value for PAGE_SIZE"
#endif

#endif /* REALM_DEF_H */

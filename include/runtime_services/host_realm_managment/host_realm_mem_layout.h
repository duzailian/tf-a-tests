/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef REALM_HOST_MEM_CONFIG_H
#define REALM_HOST_MEM_CONFIG_H

#include <platform_def.h>

/*
 * Realm payload Memory Usage Layout
 *
 * +--------------------------+     +---------------------------+
 * |                          |     | Host Image                |
 * |    TFTF                  |     | (TFTF_MAX_IMAGE_SIZE)     |
 * | Normal World             | ==> +---------------------------+
 * |    Image                 |     | Realm Image               |
 * | (MAX_NS_IMAGE_SIZE)      |     | (REALM_MAX_LOAG_IMG_SIZE  |
 * +--------------------------+     +---------------------------+
 * |  Memory Pool             |     | Heap Memory               |
 * | (NS_REALM_SHARED_MEM_SIZE|     | (PAGE_POOL_MAX_SIZE)      |
 * |  + PAGE_POOL_MAX_SIZE)   | ==> |                           |
 * |                          |     |                           |
 * |                          |     +---------------------------+
 * |                          |     | Shared Region             |
 * |                          |     | (NS_REALM_SHARED_MEM_SIZE)|
 * +--------------------------+     +---------------------------+
 *
 */
/* 1mb for shared buffer between Realm and Host*/
#define NS_REALM_SHARED_MEM_SIZE	U(0x100000)
/* 3mb of memory used as a pool for realm's objects creation*/
#define PAGE_POOL_MAX_SIZE		U(0x300000)

/*
 * Default values defined in platform.mk, and can be provided as build arguments
 * TFTF_MAX_IMAGE_SIZE: 1mb
 * REALM_MAX_LOAD_IMG_SIZE: 1mb
 */

/* Base address of each section */
#define REALM_IMAGE_BASE		(TFTF_BASE + TFTF_MAX_IMAGE_SIZE)
#define PAGE_POOL_BASE			(REALM_IMAGE_BASE + REALM_MAX_LOAD_IMG_SIZE)
#define NS_REALM_SHARED_MEM_BASE	(PAGE_POOL_BASE + PAGE_POOL_MAX_SIZE)

#endif /* REALM_HOST_MEM_CONFIG_H */

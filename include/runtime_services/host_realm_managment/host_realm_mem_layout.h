/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef HOST_REALM_MEM_LAYOUT_H
#define HOST_REALM_MEM_LAYOUT_H

#include <realm_def.h>

#include <platform_def.h>

/*
 * Realm payload Memory Usage Layout
 *
 * +--------------------------+     +---------------------------+
 * |                          |     | Host Image                |
 * |    TFTF                  |     |                           |
 * | Normal World             | ==> +                           +
 * |    Image                 |     | Realm Image               |
 * | (MAX_NS_IMAGE_SIZE)      |     | (REALM_MAX_LOAD_IMG_SIZE  |
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

/*
 * Default values defined in platform.mk, and can be provided as build arguments
 * TFTF_MAX_IMAGE_SIZE: 1MB
 */

#ifdef TFTF_MAX_IMAGE_SIZE
IMPORT_SYM(uintptr_t,          __REALM_PAYLOAD_START__,                REALM_IMAGE_BASE);
 #define PAGE_POOL_BASE                        (REALM_IMAGE_BASE + REALM_MAX_LOAD_IMG_SIZE)
 #define NS_REALM_SHARED_MEM_BASE      (PAGE_POOL_BASE + PAGE_POOL_MAX_SIZE)
#else
 /* Base address of each section */
 #define REALM_IMAGE_BASE		0U
 #define PAGE_POOL_BASE			0U
 #define NS_REALM_SHARED_MEM_BASE	0U

#endif

#endif /* HOST_REALM_MEM_LAYOUT_H */

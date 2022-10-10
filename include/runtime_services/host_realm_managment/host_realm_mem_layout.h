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
 * +--------------+      +-------------+
 * |              |      | Host Image  |
 * |    TFTF      |      |    (1MB)    |
 * | Normal World | ==>  +-------------+
 * |    Image     |      | Realm Image |
 * | (2MB Size)   |      |    (1MB)    |
 * +--------------+      +-------------+
 * |  Memory Pool |      |    Heap     |
 * |    (4MB)     |      |   Memory    |
 * |              | ==>  |    (3MB)    |
 * |              |      |             |
 * |              |      +-------------+
 * |              |      |Shared Region|
 * |              |      |   (1MB)     |
 * +--------------+      +-------------+
 *
 * 2MB for Image loading and 4MB as Free NS Space.
 */

/* Size of each section */
#define PLATFORM_NS_IMAGE_SIZE		0x200000
#define PLATFORM_HOST_IMAGE_SIZE	(PLATFORM_NS_IMAGE_SIZE / 2)
#define PLATFORM_REALM_IMAGE_SIZE	(PLATFORM_NS_IMAGE_SIZE / 2)
#define PLATFORM_MEMORY_POOL_SIZE	(4 * 0x100000)
#define PLATFORM_SHARED_REGION_SIZE	0x100000
#define PLATFORM_HEAP_REGION_SIZE	(PLATFORM_MEMORY_POOL_SIZE \
					- PLATFORM_SHARED_REGION_SIZE)

/* Base address of each section */
#define PLATFORM_REALM_IMAGE_BASE	(TFTF_BASE \
					+ PLATFORM_HOST_IMAGE_SIZE)
#define PLATFORM_MEMORY_POOL_BASE	(TFTF_BASE \
					+ PLATFORM_NS_IMAGE_SIZE)
#define PLATFORM_HEAP_REGION_BASE	PLATFORM_MEMORY_POOL_BASE
#define PLATFORM_SHARED_REGION_BASE	(PLATFORM_HEAP_REGION_BASE \
					+ PLATFORM_HEAP_REGION_SIZE)
#endif /* REALM_HOST_MEM_CONFIG_H */

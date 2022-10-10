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
 * | Normal World | ==>  +-------------+  ==> REALM_IMAGE_BASE
 * |    Image     |      | Realm Image |
 * | (2MB Size)   |      |    (1MB)    |
 * +--------------+      +-------------+  ==> PAGE_POOL_BASE
 * |  Memory Pool |      |    Heap     |
 * |    (4MB)     |      |   Memory    |
 * |              | ==>  |    (3MB)    |
 * |              |      |             |
 * |              |      +-------------+  ==> NS_REALM_SHARED_MEM_BASE
 * |              |      |Shared Region|
 * |              |      |   (1MB)     |
 * +--------------+      +-------------+
 *
 * 2MB for Image loading and 4MB as Free NS Space.
 */

/* Size of each section */
#define MAX_NS_IMAGE_SIZE		0x200000

#define REALM_MAX_LOAD_IMG_SIZE 	(MAX_NS_IMAGE_SIZE - TFTF_MAX_IMAGE_SIZE)
#define NS_REALM_SHARED_MEM_SIZE	0x100000
#define PAGE_POOL_MAX_SIZE		0x300000

/* Base address of each section */
#define REALM_IMAGE_BASE		(TFTF_BASE + TFTF_MAX_IMAGE_SIZE)
#define PAGE_POOL_BASE			(REALM_IMAGE_BASE + REALM_MAX_LOAD_IMG_SIZE)
#define NS_REALM_SHARED_MEM_BASE	(PAGE_POOL_BASE + PAGE_POOL_MAX_SIZE)

#endif /* REALM_HOST_MEM_CONFIG_H */

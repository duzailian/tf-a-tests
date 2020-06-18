/*
 * Copyright (c) 2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <drivers/arm/arm_gic.h>
#include <plat_arm.h>
#include <platform.h>

/*
 * Table of regions to map using the MMU.
 */
#if IMAGE_TFTF
static const mmap_region_t mmap[] = {
	MAP_REGION_FLAT(DEVICE0_BASE, DEVICE0_SIZE, MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(DEVICE1_BASE, DEVICE1_SIZE, MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(DRAM_BASE, TFTF_BASE - DRAM_BASE, MT_MEMORY | MT_RW | MT_NS),
	{0}
};
#endif	/* IMAGE_TFTF */

const mmap_region_t *tftf_platform_get_mmap(void)
{
	return mmap;
}

void plat_arm_gic_init(void)
{
	arm_gic_init(GICC_BASE, GICD_BASE, GICR_BASE);
}

void tftf_plat_arch_setup(void)
{
}
/*
 * Copyright (c) 2018, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <debug.h>
#include <drivers/arm/pl011.h>
#include <drivers/console.h>
#include <errno.h>
#include <ffa_helpers.h>
#include <lib/aarch64/arch_helpers.h>
#include <lib/xlat_tables/xlat_mmu_helpers.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <plat_arm.h>
#include <plat/common/platform.h>
#include <platform_def.h>
#include <sp_debug.h>
#include <sp_helpers.h>
#include <spm_common.h>
#include <std_svc.h>

#include "ivy.h"
#include "ivy_def.h"

/* Host machine information injected by the build system in the ELF file. */
extern const char build_message[];
extern const char version_string[];

static const mmap_region_t ivy_mmap[] __attribute__((used)) = {
	/* DEVICE0 area includes UART2 necessary to console */
	MAP_REGION_FLAT(DEVICE0_BASE, DEVICE0_SIZE, MT_DEVICE | MT_RW),
	{0}
};

static void ivy_print_memory_layout(void)
{
	NOTICE("Secure Partition memory layout:\n");

	NOTICE("  Image regions\n");
	NOTICE("    Text region            : %p - %p\n",
		(void *)IVY_TEXT_START, (void *)IVY_TEXT_END);
	NOTICE("    Read-only data region  : %p - %p\n",
		(void *)IVY_RODATA_START, (void *)IVY_RODATA_END);
	NOTICE("    Data region            : %p - %p\n",
		(void *)IVY_DATA_START, (void *)IVY_DATA_END);
	NOTICE("    BSS region             : %p - %p\n",
		(void *)IVY_BSS_START, (void *)IVY_BSS_END);
	NOTICE("    Total image memory     : %p - %p\n",
		(void *)IVY_IMAGE_BASE,
		(void *)(IVY_IMAGE_BASE + IVY_IMAGE_SIZE));
	NOTICE("  SPM regions\n");
	NOTICE("    SPM <-> SP buffer      : %p - %p\n",
		(void *)IVY_SPM_BUF_BASE,
		(void *)(IVY_SPM_BUF_BASE + IVY_SPM_BUF_SIZE));
	NOTICE("    NS <-> SP buffer       : %p - %p\n",
		(void *)IVY_NS_BUF_BASE,
		(void *)(IVY_NS_BUF_BASE + IVY_NS_BUF_SIZE));
}

static void ivy_plat_configure_mmu(void)
{
	mmap_add_region(IVY_TEXT_START,
			IVY_TEXT_START,
			IVY_TEXT_END - IVY_TEXT_START,
			MT_CODE);
	mmap_add_region(IVY_RODATA_START,
			IVY_RODATA_START,
			IVY_RODATA_END - IVY_RODATA_START,
			MT_RO_DATA);
	mmap_add_region(IVY_DATA_START,
			IVY_DATA_START,
			IVY_DATA_END - IVY_DATA_START,
			MT_RW_DATA);
	mmap_add_region(IVY_BSS_START,
			IVY_BSS_START,
			IVY_BSS_END - IVY_BSS_START,
			MT_RW_DATA);

	mmap_add(ivy_mmap);
	init_xlat_tables();
}


void __dead2 ivy_main(void)
{
	assert(IS_IN_EL1() != 0);

	/* Clear BSS */
	memset((void *)IVY_BSS_START,
	       0, IVY_BSS_END - IVY_BSS_START);

	/* Get current FFA id */
	smc_ret_values ffa_id_ret = ffa_id_get();

	if (ffa_func_id(ffa_id_ret) != FFA_SUCCESS_SMC32) {
		ERROR("FFA_ID_GET failed.\n");
		panic();
	}

	ffa_vm_id_t ffa_id = ffa_endpoint_id(ffa_id_ret);

	/* Configure and enable Stage-1 MMU, enable D-Cache */
	ivy_plat_configure_mmu();
	enable_mmu_el1(0);

	/* Initialise console */
	if (ffa_id == SPM_VM_ID_FIRST) {
		console_init(PL011_UART2_BASE,
			PL011_UART2_CLK_IN_HZ,
			PL011_BAUDRATE);

		set_putc_impl(PL011_AS_STDOUT);
	} else {
		set_putc_impl(HVC_CALL_AS_STDOUT);
	}

	NOTICE("Booting test Secure Partition Ivy (ID: %u)\n", ffa_id);
	NOTICE("%s\n", build_message);
	NOTICE("%s\n", version_string);
	NOTICE("Running at S-EL0\n");

	ivy_print_memory_layout();

	ffa_msg_wait();
	for (;;)
		;
}

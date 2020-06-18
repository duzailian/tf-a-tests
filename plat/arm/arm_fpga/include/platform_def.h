/*
 * Copyright (c) 2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <utils_def.h>

#include "../arm_fpga_def.h"

/*******************************************************************************
 * Platform definitions used by common code
 ******************************************************************************/

#ifndef __PLATFORM_DEF_H__
#define __PLATFORM_DEF_H__

/*******************************************************************************
 * Platform binary types for linking
 ******************************************************************************/
#ifdef __aarch64__
#define PLATFORM_LINKER_FORMAT		"elf64-littleaarch64"
#define PLATFORM_LINKER_ARCH		aarch64
#else
#define PLATFORM_LINKER_FORMAT		"elf32-littlearm"
#define PLATFORM_LINKER_ARCH		arm
#endif

/*******************************************************************************
 * Run-time address of the TFTF image.
 * It has to match the location where the Trusted Firmware-A loads the BL33
 * image.
 ******************************************************************************/
#define TFTF_BASE			0x84100000

/* Base address of non-trusted watchdog (SP805) */
/* FIXME: Confirm watchdog base address */
#define SP805_WDOG_BASE			0x1C0F0000

/*******************************************************************************
 * Base address and size for non-trusted SRAM.
 ******************************************************************************/
//#define NSRAM_BASE				(0x2e000000)
//#define NSRAM_SIZE				(0x00010000)

/*******************************************************************************
 * Platform memory map related constants
 ******************************************************************************/
#define ARM_FPGA_DRAM1_BASE		0x80000000
#define ARM_FPGA_DRAM2_BASE		0x880000000ULL
#define ARM_FPGA_RESERVED_BASE		ARM_FPGA_DRAM1_BASE
#define ARM_FPGA_RESERVED_SIZE		0x02080000
#define DRAM_BASE			ARM_FPGA_DRAM1_BASE
#define DRAM_SIZE			0x80000000

/******************************************************************************
 * Memory mapped Generic timer interfaces
 ******************************************************************************/
/* REFCLK CNTControl, Generic Timer. Secure Access only. */
#define SYS_CNT_CONTROL_BASE		0x2a430000
/* REFCLK CNTRead, Generic Timer. */
#define SYS_CNT_READ_BASE		0x2a800000
/* AP_REFCLK CNTBase1, Generic Timer. */
#define SYS_CNT_BASE1			0x2a830000

/* V2M motherboard system registers & offsets */
//#define VE_SYSREGS_BASE		0x1c010000
//#define V2M_SYS_LED		0x8

/*******************************************************************************
 * Generic platform constants
 ******************************************************************************/

/* Size of cacheable stacks */
#define PLATFORM_STACK_SIZE	0x1400

/* Size of coherent stacks for debug and release builds */
#if DEBUG
#define PCPU_DV_MEM_STACK_SIZE	0x600
#else
#define PCPU_DV_MEM_STACK_SIZE	0x500
#endif

#define PLATFORM_CORE_COUNT		(ARM_FPGA_MAX_CLUSTER_COUNT * \
					 ARM_FPGA_MAX_CPUS_PER_CLUSTER * \
					 ARM_FPGA_MAX_PE_PER_CPU)
#define PLATFORM_NUM_AFFS		(1 + ARM_FPGA_MAX_CLUSTER_COUNT + \
					 PLATFORM_CORE_COUNT)
#define PLATFORM_MAX_AFFLVL		MPIDR_AFFLVL2

/* TODO : Migrate complete TFTF from affinity level to power levels */
#define PLAT_MAX_PWR_LEVEL		PLATFORM_MAX_AFFLVL
#define PLAT_MAX_PWR_STATES_PER_LVL	2

#define MAX_IO_DEVICES			1
#define MAX_IO_HANDLES			1

/* Local state bit width for each level in the state-ID field of power state */
#define PLAT_LOCAL_PSTATE_WIDTH		4

/*
 * FPGA_ARM Platform does not have support for NVM so it is emulated by RAM
 * when needed.
 * Please note that this solution might not be suitable for some test scenarios
 * so care must be taken when enabling tests that need NVM support.
 */
#define TFTF_NVM_OFFSET		ARM_FPGA_RESERVED_SIZE
#define TFTF_NVM_SIZE		(TFTF_BASE - DRAM_BASE - TFTF_NVM_OFFSET)

/*******************************************************************************
 * Platform specific page table and MMU setup constants
 ******************************************************************************/
#ifdef __aarch64__
#define PLAT_PHY_ADDR_SPACE_SIZE	(ULL(1) << 34)
#define PLAT_VIRT_ADDR_SPACE_SIZE	(ULL(1) << 34)
#else
#define PLAT_PHY_ADDR_SPACE_SIZE	(ULL(1) << 32)
#define PLAT_VIRT_ADDR_SPACE_SIZE	(ULL(1) << 32)
#endif

#if IMAGE_TFTF
/* For testing xlat tables lib v2 */
#define MAX_XLAT_TABLES			20
#define MAX_MMAP_REGIONS		50
#else
#define MAX_XLAT_TABLES			5
#define MAX_MMAP_REGIONS		16
#endif

/*******************************************************************************
 * Used to align variables on the biggest cache line size in the platform.
 * This is known only to the platform as it might have a combination of
 * integrated and external caches.
 ******************************************************************************/
#define CACHE_WRITEBACK_SHIFT		6
#define CACHE_WRITEBACK_GRANULE		(1 << CACHE_WRITEBACK_SHIFT)

/*******************************************************************************
 * Non-Secure Software Generated Interupts IDs
 ******************************************************************************/
#define IRQ_NS_SGI_0			0
#define IRQ_NS_SGI_1			1
#define IRQ_NS_SGI_2			2
#define IRQ_NS_SGI_3			3
#define IRQ_NS_SGI_4			4
#define IRQ_NS_SGI_5			5
#define IRQ_NS_SGI_6			6
#define IRQ_NS_SGI_7			7

/*
 * On FVP, consider that the last SPI is the Trusted Random Number Generator
 * interrupt.
 */
/* Fixme: Check this parameter for ARM_FPGA platform. */
#define PLAT_MAX_SPI_OFFSET_ID		107

/* AP_REFCLK, Generic Timer, CNTPSIRQ1. */
#define IRQ_CNTPSIRQ1			58
/* Per-CPU Hypervisor Timer Interrupt ID */
#define IRQ_PCPU_HP_TIMER		26
/* Per-CPU Non-Secure Timer Interrupt ID */
#define IRQ_PCPU_NS_TIMER		30


/* Times(in ms) used by test code for completion of different events */
#define PLAT_SUSPEND_ENTRY_TIME		15
#define PLAT_SUSPEND_ENTRY_EXIT_TIME	30

/*******************************************************************************
 * Location of the memory buffer shared between Normal World (i.e. TFTF) and the
 * Secure Partition (e.g. Cactus-MM) to pass data associated to secure service
 * requests. This is only needed for SPM based on MM.
 * Note: This address has to match the one used in TF (see ARM_SP_IMAGE_NS_BUF_*
 * macros).
 ******************************************************************************/
//#define ARM_SECURE_SERVICE_BUFFER_BASE	0xff600000ull
//#define ARM_SECURE_SERVICE_BUFFER_SIZE	0x10000ull

#undef PL011_BAUDRATE
#define PL011_BAUDRATE		38400

#endif /* __PLATFORM_DEF_H__ */

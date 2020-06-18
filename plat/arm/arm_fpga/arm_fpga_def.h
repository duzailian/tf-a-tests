/*
 * Copyright (c) 2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*******************************************************************************
 * ARM_FPGA specific definitions. Used only by ARM_FPGA specific code.
 ******************************************************************************/

#ifndef __ARM_FPGA_DEF_H__
#define __ARM_FPGA_DEF_H__

#include <platform_def.h>

/*******************************************************************************
 * Cluster Topology definitions
 * These are maximum values. The current board might implement less
 ******************************************************************************/
/* FIXME: Hardcoded values to make the Zeus image work */
#define ARM_FPGA_MAX_PE_PER_CPU 	2
#define ARM_FPGA_MAX_CPUS_PER_CLUSTER	4
#define ARM_FPGA_MAX_CLUSTER_COUNT	2
#define PLAT_MAX_PE_PER_CPU		ARM_FPGA_MAX_PE_PER_CPU

/*******************************************************************************
 * ARM_FPGA memory map related constants
 ******************************************************************************/

#define DEVICE0_BASE		ARM_FPGA_DRAM1_BASE
#define DEVICE0_SIZE		DRAM_SIZE

#define DEVICE1_BASE		ARM_FPGA_DRAM2_BASE
#define DEVICE1_SIZE		DRAM_SIZE

/*******************************************************************************
 * GIC-400 & interrupt handling related constants
 ******************************************************************************/
/* Base FPGA_ARM compatible GIC memory map */
#define GICD_BASE		0x30000000
#define GICR_BASE		0x30040000
#define GICC_BASE		0

/*******************************************************************************
 * PL011 related constants
 ******************************************************************************/
#define PL011_UART0_BASE	0x7ff80000
#define PL011_UART0_CLK_IN_HZ	10000000

#define PLAT_ARM_UART_BASE		PL011_UART0_BASE
#define PLAT_ARM_UART_CLK_IN_HZ		PL011_UART0_CLK_IN_HZ

#endif /* __FVP_DEF_H__ */

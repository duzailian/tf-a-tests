/*
 * Copyright (c) 2020-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform_def.h>

#ifndef SP_PLATFORM_DEF_H
#define SP_PLATFORM_DEF_H

#define PLAT_ARM_DEVICE0_BASE		DEVICE0_BASE
#define PLAT_ARM_DEVICE0_SIZE		DEVICE0_SIZE

#define SP_PL011_UART_BASE		PL011_UART2_BASE
#define SP_PL011_UART_CLK_IN_HZ		PL011_UART2_CLK_IN_HZ

#define PLAT_SP_RX_BASE			ULL(0x7300000)
#define PLAT_SP_CORE_COUNT		(8U)

/* Scratch memory used for SMMUv3 driver testing purposes in Cactus SP */
#define PLAT_SP_MEMCPY_BASE		ULL(0x7400000)
#define PLAT_SP_MEMCPY_RANGE		ULL(0x8000)

/* Base address of user and PRIV frames in SMMUv3TestEngine */
#define USR_BASE_FRAME			ULL(0x2BFE0000)
#define PRIV_BASE_FRAME			ULL(0x2BFF0000)

/* Base address for memory sharing tests. */
#define CACTUS_SP1_MEM_SHARE_BASE 0x7500000
#define CACTUS_SP2_MEM_SHARE_BASE 0x7501000
#define CACTUS_SP3_MEM_SHARE_BASE 0x7502000
#define CACTUS_SP3_NS_MEM_SHARE_BASE 0x880080001000ULL

#endif /* SP_PLATFORM_DEF_H */

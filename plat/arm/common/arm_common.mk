#
# Copyright (c) 2018-2019, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

PLAT_INCLUDES	+=	-Iinclude/plat/arm/common/

PLAT_SOURCES	+=	drivers/arm/gic/gic_common.c			\
			drivers/arm/pl011/${ARCH}/pl011_console.S	\
			plat/arm/common/arm_setup.c			\
			plat/arm/common/arm_timers.c

ifeq (${ARCH},aarch64)
# ARMv8.3 Pointer Authentication source file
PLAT_SOURCES	+=	plat/arm/common/arm_pauth.c
endif

# Flash driver sources.
PLAT_SOURCES	+=	drivers/io/io_storage.c				\
			drivers/io/vexpress_nor/io_vexpress_nor_ops.c	\
			drivers/io/vexpress_nor/io_vexpress_nor_hw.c	\
			plat/arm/common/arm_io_storage.c

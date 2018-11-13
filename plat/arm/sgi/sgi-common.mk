#
# Copyright (c) 2018, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

PLAT_INCLUDES	:=	-Iplat/arm/sgi/include/

PLAT_SOURCES	:=	drivers/arm/gic/arm_gic_v2v3.c		\
			drivers/arm/gic/gic_v2.c		\
			drivers/arm/gic/gic_v3.c		\
			drivers/arm/sp805/sp805.c		\
			drivers/arm/timer/private_timer.c	\
			drivers/arm/timer/system_timer.c	\
			plat/arm/sgi/${ARCH}/plat_helpers.S	\
			plat/arm/sgi/plat_setup.c		\
			plat/arm/sgi/sgi_mem_prot.c		\
			plat/arm/sgi/sgi_pwr_state.c

PLAT_TESTS_SKIP_LIST	:=	plat/arm/board/sgi575/tests_to_skip.txt

include plat/arm/common/arm_common.mk

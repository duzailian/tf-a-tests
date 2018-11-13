#
# Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include plat/arm/sgi/sgi-common.mk

PLAT_INCLUDES	+=	-Iplat/arm/board/sgi575/include/

PLAT_SOURCES	+=	plat/arm/board/sgi575/sgi575_topology.c

#
# Copyright (c) 2018-2022, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include plat/arm/sgi/common/sgi_common.mk

PLAT_INCLUDES		+=	-Iplat/arm/sgi/sgi575/include/

PLAT_SOURCES		+=	plat/arm/sgi/sgi575/sgi575_topology.c

PLAT_TESTS_SKIP_LIST	:=	plat/arm/sgi/sgi575/tests_to_skip.txt

ifneq ($(CSS_SGI_PLATFORM_VARIANT),0)
  $(error "CSS_SGI_PLATFORM_VARIANT for SGI575 should always be 0, \
    currently set to ${CSS_SGI_PLATFORM_VARIANT}.")
endif

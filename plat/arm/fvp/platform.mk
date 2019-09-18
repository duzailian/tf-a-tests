#
# Copyright (c) 2018-2019, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

PLAT_INCLUDES	:=	-Iplat/arm/fvp/include/

PLAT_SOURCES	:=	drivers/arm/gic/arm_gic_v2v3.c			\
			drivers/arm/gic/gic_v2.c			\
			drivers/arm/gic/gic_v3.c			\
			drivers/arm/sp805/sp805.c			\
			drivers/arm/timer/private_timer.c		\
			drivers/arm/timer/system_timer.c		\
			plat/arm/fvp/${ARCH}/plat_helpers.S		\
			plat/arm/fvp/fvp_pwr_state.c			\
			plat/arm/fvp/fvp_topology.c			\
			plat/arm/fvp/fvp_mem_prot.c			\
			plat/arm/fvp/plat_setup.c

CACTUS_SOURCES	+=	plat/arm/fvp/${ARCH}/plat_helpers.S

# Firmware update is implemented on FVP.
FIRMWARE_UPDATE := 1

ifeq (${ARCH},aarch64)
PLAT_SOURCES	+=	plat/common/aarch64/pauth.c
endif

# On some configurations of the FVP, we want to skip the PMU tests.
# One such example is when the model is configured with 0xffffffff as the
# reset value for registers and that causes some of the PMU security features to
# be temporarily disabled when the CPU wakes up from suspended state. This makes
# the counters increment until execution at EL3 reaches the code which
# initializes the MDCR and PMCR registers. Thus the tests fail, despite
# everything working as expected.
PLAT_SKIP_PMU_TESTS	:=	0

# Process PLAT_SKIP_PMU_TESTS flag
$(eval $(call assert_boolean,PLAT_SKIP_PMU_TESTS))
$(eval $(call add_define,TFTF_DEFINES,PLAT_SKIP_PMU_TESTS))

include plat/arm/common/arm_common.mk

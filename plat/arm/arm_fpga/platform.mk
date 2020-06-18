#
# Copyright (c) 2020, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Default number of threads per CPU on ARM_FPGA
ARM_FPGA_MAX_PE_PER_CPU	:= 2

# Check the PE per core count. At the moment arm_fpga only supports
# single threaded CPUs.
#ifneq ($(ARM_FPGA_MAX_PE_PER_CPU),$(filter $(ARM_FPGA_MAX_PE_PER_CPU),1))
#$(error "Incorrect ARM_FPGA_MAX_PE_PER_CPU specified for ARM_FPGA port")
#endif

# Pass ARM_FPGA_MAX_PE_PER_CPU to the build system
$(eval $(call add_define,TFTF_DEFINES,ARM_FPGA_MAX_PE_PER_CPU))

# FIXME: Remove these as they are not needed for the FPGA port.
$(eval $(call add_define,NS_BL1U_DEFINES,ARM_FPGA_MAX_PE_PER_CPU))
$(eval $(call add_define,NS_BL2U_DEFINES,ARM_FPGA_MAX_PE_PER_CPU))

PLAT_INCLUDES	:=	-Iplat/arm/arm_fpga/include/

PLAT_SOURCES	:=	drivers/arm/gic/arm_gic_v2v3.c			\
			drivers/arm/gic/gic_v2.c			\
			drivers/arm/gic/gic_v3.c			\
			drivers/arm/sp805/sp805.c			\
			drivers/arm/timer/private_timer.c		\
			drivers/arm/timer/system_timer.c		\
			plat/arm/arm_fpga/${ARCH}/plat_helpers.S	\
			plat/arm/arm_fpga/arm_fpga_pwr_state.c		\
			plat/arm/arm_fpga/arm_fpga_topology.c		\
			plat/arm/arm_fpga/arm_fpga_mem_prot.c		\
			plat/arm/arm_fpga/plat_setup.c

# Common source files and includes to ARM arquitecture.
PLAT_INCLUDES	+=	-Iinclude/plat/arm/common/

PLAT_SOURCES	+=	drivers/arm/gic/gic_common.c			\
			drivers/arm/pl011/${ARCH}/pl011_console.S	\
			drivers/console/console.c			\
			plat/arm/common/arm_setup.c			\
			plat/arm/common/arm_timers.c

# Firmware update is not implemented on ARM_FPGA
FIRMWARE_UPDATE		:=	0

# NVM must be emulated on RAM
USE_NVM			:=	0

# Only aarch64 supported
ARCH			:=	aarch64

PLAT_TESTS_SKIP_LIST	:=	plat/arm/arm_fpga/arm_fpga_tests_to_skip.txt

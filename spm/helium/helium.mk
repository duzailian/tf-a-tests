#
# Copyright (c) 2018-2019, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include lib/sprt/sprt_client.mk

HELIUM_DTB		:= $(BUILD_PLAT)/helium.dtb

HELIUM_INCLUDES :=					\
	-Iinclude					\
	-Iinclude/common				\
	-Iinclude/common/${ARCH}			\
	-Iinclude/lib					\
	-Iinclude/lib/${ARCH}				\
	-Iinclude/lib/sprt				\
	-Iinclude/lib/utils				\
	-Iinclude/lib/xlat_tables			\
	-Iinclude/runtime_services			\
	-Iinclude/runtime_services/secure_el0_payloads	\
	-Ispm/helium					\
	-Ispm/common					\
	${SPRT_LIB_INCLUDES}

HELIUM_SOURCES	:=					\
	$(addprefix spm/helium/,			\
		aarch64/helium_entrypoint.S		\
		helium_main.c				\
	)						\
	$(addprefix spm/common/,			\
		aarch64/sp_arch_helpers.S		\
		sp_helpers.c				\
	)						\

# TODO: Remove dependency on TFTF files.
HELIUM_SOURCES	+=					\
	tftf/framework/debug.c				\
	tftf/framework/${ARCH}/asm_debug.S

HELIUM_SOURCES	+= 	drivers/console/${ARCH}/dummy_console.S		\
			lib/${ARCH}/cache_helpers.S			\
			lib/${ARCH}/misc_helpers.S			\
			lib/locks/${ARCH}/spinlock.S			\
			lib/utils/mp_printf.c				\
			${SPRT_LIB_SOURCES}

HELIUM_LINKERFILE	:=	spm/helium/helium.ld.S

HELIUM_DEFINES	:=

$(eval $(call add_define,HELIUM_DEFINES,DEBUG))
$(eval $(call add_define,HELIUM_DEFINES,ENABLE_ASSERTIONS))
$(eval $(call add_define,HELIUM_DEFINES,LOG_LEVEL))
$(eval $(call add_define,HELIUM_DEFINES,PLAT_${PLAT}))
ifeq (${ARCH},aarch32)
        $(eval $(call add_define,HELIUM_DEFINES,AARCH32))
else
        $(eval $(call add_define,HELIUM_DEFINES,AARCH64))
endif

$(HELIUM_DTB) : $(BUILD_PLAT)/helium $(BUILD_PLAT)/helium/helium.elf
$(HELIUM_DTB) : spm/helium/helium.dts
	@echo "  DTBGEN  spm/helium/helium.dts"
	${Q}tools/generate_dtb/generate_dtb.sh \
		helium spm/helium/helium.dts $(BUILD_PLAT)
	@echo
	@echo "Built $@ successfully"
	@echo

helium: $(HELIUM_DTB) $(AUTOGEN_DIR)/tests_list.h

#
# Copyright (c) 2018-2022, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include branch_protection.mk
include lib/xlat_tables_v2/xlat_tables.mk

# Include ivy_dup platform Makefile
IVY_DUP_PLAT_PATH	:= $(shell find spm/ivy_dup/app/plat -wholename '*/${PLAT}')
ifneq (${IVY_DUP_PLAT_PATH},)
	include ${IVY_DUP_PLAT_PATH}/platform.mk
endif

IVY_DUP_DTB		:= $(BUILD_PLAT)/ivy_dup-sel0.dtb
SECURE_PARTITIONS	+= ivy_dup

IVY_DUP_INCLUDES :=					\
	-Itftf/framework/include			\
	-Iinclude					\
	-Iinclude/common				\
	-Iinclude/common/${ARCH}			\
	-Iinclude/lib					\
	-Iinclude/lib/${ARCH}				\
	-Iinclude/lib/utils				\
	-Iinclude/lib/xlat_tables			\
	-Iinclude/plat/common				\
	-Iinclude/runtime_services			\
	-Iinclude/runtime_services/secure_el0_payloads	\
	-Ispm/ivy_dup/app					\
	-Ispm/common					\
	-Ispm/common/sp_tests/

IVY_DUP_SOURCES	:=					\
	$(addprefix spm/ivy_dup/app/,			\
		aarch64/ivy_dup_entrypoint.S		\
		ivy_dup_main.c				\
	)						\
	$(addprefix spm/common/,			\
		sp_debug.c				\
		sp_helpers.c				\
		spm_helpers.c				\
	)						\
	$(addprefix spm/common/sp_tests/,		\
		sp_test_ffa.c				\
	)

# TODO: Remove dependency on TFTF files.
IVY_DUP_SOURCES	+=							\
	tftf/framework/debug.c						\
	tftf/framework/${ARCH}/asm_debug.S				\
	tftf/tests/runtime_services/secure_service/${ARCH}/ffa_arch_helpers.S \
	tftf/tests/runtime_services/secure_service/ffa_helpers.c 	\
	tftf/tests/runtime_services/secure_service/spm_common.c

IVY_DUP_SOURCES	+= 	drivers/arm/pl011/${ARCH}/pl011_console.S	\
			lib/${ARCH}/cache_helpers.S			\
			lib/${ARCH}/misc_helpers.S			\
			lib/smc/${ARCH}/asm_smc.S			\
			lib/smc/${ARCH}/smc.c				\
			lib/smc/${ARCH}/hvc.c				\
			lib/locks/${ARCH}/spinlock.S			\
			lib/utils/mp_printf.c				\
			${XLAT_TABLES_LIB_SRCS}

IVY_DUP_LINKERFILE	:=	spm/ivy_dup/ivy_dup.ld.S

IVY_DUP_DEFINES	:=

$(eval $(call add_define,IVY_DUP_DEFINES,ARM_ARCH_MAJOR))
$(eval $(call add_define,IVY_DUP_DEFINES,ARM_ARCH_MINOR))
$(eval $(call add_define,IVY_DUP_DEFINES,DEBUG))
$(eval $(call add_define,IVY_DUP_DEFINES,ENABLE_ASSERTIONS))
$(eval $(call add_define,IVY_DUP_DEFINES,ENABLE_BTI))
$(eval $(call add_define,IVY_DUP_DEFINES,ENABLE_PAUTH))
$(eval $(call add_define,IVY_DUP_DEFINES,LOG_LEVEL))
$(eval $(call add_define,IVY_DUP_DEFINES,PLAT_${PLAT}))
$(eval $(call add_define,IVY_DUP_DEFINES,IVY_DUP_SHIM))

$(IVY_DUP_DTB) : $(BUILD_PLAT)/ivy_dup $(BUILD_PLAT)/ivy_dup/ivy_dup.elf
$(IVY_DUP_DTB) : $(IVY_DUP_DTS)
	@echo "  DTBGEN  $@"
	${Q}tools/generate_dtb/generate_dtb.sh \
		ivy_dup ${IVY_DUP_DTS} $(BUILD_PLAT) $(IVY_DUP_DTB)
	@echo
	@echo "Built $@ successfully"
	@echo

ivy_dup: $(IVY_DUP_DTB) SP_LAYOUT

# FDTS_CP copies flattened device tree sources
#   $(1) = output directory
#   $(2) = flattened device tree source file to copy
define FDTS_CP
        $(eval FDTS := $(addprefix $(1)/,$(notdir $(2))))
FDTS_LIST += $(FDTS)
$(FDTS): $(2) $(IVY_DUP_DTB)
	@echo "  CP      $$<"
	${Q}cp $$< $$@
endef

ifdef FDTS_CP_LIST
        $(eval files := $(filter %.dts,$(FDTS_CP_LIST)))
        $(eval $(foreach file,$(files),$(call FDTS_CP,$(BUILD_PLAT),$(file))))
ivy_dup: $(FDTS_LIST)
endif

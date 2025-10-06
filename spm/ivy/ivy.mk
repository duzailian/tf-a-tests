#
# Copyright (c) 2018-2022, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include branch_protection.mk
include lib/xlat_tables_v2/xlat_tables.mk

# Include ivy platform Makefile
IVY_PLAT_PATH	:= $(shell find spm/ivy/app/plat -wholename '*/${PLAT}')
ifneq (${IVY_PLAT_PATH},)
	include ${IVY_PLAT_PATH}/platform.mk
endif

IVY_SHIM	:= 1

ifeq (${IVY_SHIM},1)
	IVY_DTB			:= $(BUILD_PLAT)/ivy-sel1.dtb
	SECURE_PARTITIONS	+= ivy_shim
else
	IVY_DTB			:= $(BUILD_PLAT)/ivy-sel0.dtb
	SECURE_PARTITIONS	+= ivy
endif

IVY_INCLUDES :=					\
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
	-Ispm/ivy/app					\
	-Ispm/ivy/shim					\
	-Ispm/common					\
	-Ispm/common/sp_tests/

IVY_SOURCES	:=					\
	$(addprefix spm/ivy/app/,			\
		aarch64/ivy_entrypoint.S		\
		ivy_main.c				\
	)						\
	$(addprefix spm/common/,			\
		sp_debug.c				\
		sp_helpers.c				\
		spm_helpers.c				\
	)						\
	$(addprefix spm/common/sp_tests/,		\
		sp_test_ffa.c				\
	)

ifeq ($(IVY_SHIM),1)
IVY_SOURCES	+=					\
	$(addprefix spm/ivy/shim/,			\
		aarch64/spm_shim_entrypoint.S		\
		aarch64/spm_shim_exceptions.S		\
		shim_main.c				\
	)
endif

# TODO: Remove dependency on TFTF files.
IVY_SOURCES	+=							\
	tftf/framework/debug.c						\
	tftf/framework/${ARCH}/asm_debug.S				\
	tftf/tests/runtime_services/secure_service/${ARCH}/ffa_arch_helpers.S \
	tftf/tests/runtime_services/secure_service/ffa_helpers.c 	\
	tftf/tests/runtime_services/secure_service/spm_common.c

IVY_SOURCES	+= 	drivers/arm/pl011/${ARCH}/pl011_console.S	\
			lib/${ARCH}/cache_helpers.S			\
			lib/${ARCH}/misc_helpers.S			\
			lib/smc/${ARCH}/asm_smc.S			\
			lib/smc/${ARCH}/smc.c				\
			lib/smc/${ARCH}/hvc.c				\
			lib/locks/${ARCH}/spinlock.S			\
			lib/utils/mp_printf.c				\
			${XLAT_TABLES_LIB_SRCS}

IVY_LINKERFILE	:=	spm/ivy/ivy.ld.S

IVY_DEFINES	:=

$(eval $(call add_define,IVY_DEFINES,ARM_ARCH_MAJOR))
$(eval $(call add_define,IVY_DEFINES,ARM_ARCH_MINOR))
$(eval $(call add_define,IVY_DEFINES,DEBUG))
$(eval $(call add_define,IVY_DEFINES,ENABLE_ASSERTIONS))
$(eval $(call add_define,IVY_DEFINES,ENABLE_BTI))
$(eval $(call add_define,IVY_DEFINES,ENABLE_PAUTH))
$(eval $(call add_define,IVY_DEFINES,LOG_LEVEL))
$(eval $(call add_define,IVY_DEFINES,PLAT_${PLAT}))
$(eval $(call add_define,IVY_DEFINES,IVY_SHIM))

ifeq ($(SMC_FUZZING), 1)

# Generate random fuzzing seeds
# If no instance count is provided, default to 1 instance
# If no seeds are provided, generate them randomly
# The number of seeds provided must match the instance count
SMC_FUZZ_INSTANCE_COUNT ?= 1
SMC_FUZZ_SEEDS ?= $(shell python -c "from random import randint; seeds = [randint(0, 4294967295) for i in range($(SMC_FUZZ_INSTANCE_COUNT))];print(\",\".join(str(x) for x in seeds));")
SMC_FUZZ_CALLS_PER_INSTANCE ?= 100
# ADDED: which fuzz call to start with per instance
SMC_FUZZ_CALL_START ?= 0
SMC_FUZZ_CALL_END ?= $(SMC_FUZZ_CALLS_PER_INSTANCE)
# ADDED: define whether events should specifically be constrained
EXCLUDE_FUNCID ?= 0
CONSTRAIN_EVENTS ?= 0
INTR_ASSERT ?= 0

# Validate SMC fuzzer parameters

# Instance count must not be zero
ifeq ($(SMC_FUZZ_INSTANCE_COUNT),0)
$(error SMC_FUZZ_INSTANCE_COUNT must not be zero!)
endif

# Calls per instance must not be zero
ifeq ($(SMC_FUZZ_CALLS_PER_INSTANCE),0)
$(error SMC_FUZZ_CALLS_PER_INSTANCE must not be zero!)
endif

# Make sure seed count and instance count match
TEST_SEED_COUNT = $(shell python -c "print(len(\"$(SMC_FUZZ_SEEDS)\".split(\",\")))")
ifneq ($(TEST_SEED_COUNT), $(SMC_FUZZ_INSTANCE_COUNT))
$(error Number of seeds does not match SMC_FUZZ_INSTANCE_COUNT!)
endif

# Start must be nonnegative and less than calls per instance
ifeq ($(shell test $(SMC_FUZZ_CALL_START) -lt 0; echo $$?),0)
$(error SMC_FUZZ_CALL_START must be nonnegative!)
endif

ifeq ($(shell test $(SMC_FUZZ_CALL_START) -gt $(shell expr $(SMC_FUZZ_CALLS_PER_INSTANCE) - 1); echo $$?),0)
$(error SMC_FUZZ_CALL_START must be less than SMC_FUZZ_CALLS_PER_INSTANCE!)
endif

# End must be greater than start and less than or equal to calls per instance
ifneq ($(shell test $(SMC_FUZZ_CALL_START) -lt $(SMC_FUZZ_CALL_END); echo $$?),0)
$(error SMC_FUZZ_CALL_END must be greater than SMC_FUZZ_CALL_START!)
endif

ifeq ($(shell test $(SMC_FUZZ_CALL_END) -gt $(SMC_FUZZ_CALLS_PER_INSTANCE); echo $$?),0)
$(error SMC_FUZZ_CALL_END must not be greater than SMC_FUZZ_CALLS_PER_INSTANCE!)
endif

IVY_INCLUDES +=					\
	-Iinclude/lib/extensions			\
	-Iinclude/runtime_services/secure_el1_payloads	\
	-Ismc_fuzz/include

# Needed for fuzz work
IVY_SOURCES	+= \
	$(addprefix tftf/tests/runtime_services/standard_service/sdei/system_tests/, \
		sdei_entrypoint.S \
		test_sdei.c \
	)								\
	$(addprefix smc_fuzz/src/,					\
		randsmcmod.c						\
		smcmalloc.c						\
		fifo3d.c						\
		runtestfunction_helpers.c				\
		sdei_fuzz_helper.c					\
		ffa_fuzz_helper.c					\
		tsp_fuzz_helper.c					\
		nfifo.c							\
		constraint.c						\
		vendor_fuzz_helper.c					\
	)

# Add definitions to IVY_DEFINES so they can be used in the code
$(eval $(call add_define,IVY_DEFINES,SMC_FUZZING))
$(eval $(call add_define,IVY_DEFINES,SMC_FUZZ_SEEDS))
$(eval $(call add_define,IVY_DEFINES,SMC_FUZZ_INSTANCE_COUNT))
$(eval $(call add_define,IVY_DEFINES,SMC_FUZZ_CALLS_PER_INSTANCE))
ifeq ($(SMC_FUZZER_DEBUG),1)
$(eval $(call add_define,IVY_DEFINES,SMC_FUZZER_DEBUG))
endif
ifeq ($(MULTI_CPU_SMC_FUZZER),1)
$(eval $(call add_define,IVY_DEFINES,MULTI_CPU_SMC_FUZZER))
endif
$(eval $(call add_define,IVY_DEFINES,SMC_FUZZ_SANITY_LEVEL))
$(eval $(call add_define,IVY_DEFINES,SMC_FUZZ_CALL_START))
$(eval $(call add_define,IVY_DEFINES,SMC_FUZZ_CALL_END))
$(eval $(call add_define,IVY_DEFINES,CONSTRAIN_EVENTS))
$(eval $(call add_define,IVY_DEFINES,EXCLUDE_FUNCID))
$(eval $(call add_define,IVY_DEFINES,INTR_ASSERT))
ifeq ($(SMC_FUZZ_VARIABLE_COVERAGE),1)
$(eval $(call add_define,IVY_DEFINES,SMC_FUZZ_VARIABLE_COVERAGE))
endif

endif

$(IVY_DTB) : $(BUILD_PLAT)/ivy $(BUILD_PLAT)/ivy/ivy.elf
$(IVY_DTB) : $(IVY_DTS)
	@echo "  DTBGEN  $@"
	${Q}tools/generate_dtb/generate_dtb.sh \
		ivy ${IVY_DTS} $(BUILD_PLAT) $(IVY_DTB)
	@echo
	@echo "Built $@ successfully"
	@echo

ivy: $(IVY_DTB) SP_LAYOUT

# FDTS_CP copies flattened device tree sources
#   $(1) = output directory
#   $(2) = flattened device tree source file to copy
define FDTS_CP
        $(eval FDTS := $(addprefix $(1)/,$(notdir $(2))))
FDTS_LIST += $(FDTS)
$(FDTS): $(2) $(IVY_DTB)
	@echo "  CP      $$<"
	${Q}cp $$< $$@
endef

ifdef FDTS_CP_LIST
        $(eval files := $(filter %.dts,$(FDTS_CP_LIST)))
        $(eval $(foreach file,$(files),$(call FDTS_CP,$(BUILD_PLAT),$(file))))
ivy: $(FDTS_LIST)
endif

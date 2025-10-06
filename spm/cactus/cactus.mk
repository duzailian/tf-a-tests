#
# Copyright (c) 2018-2024, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include branch_protection.mk
include lib/xlat_tables_v2/xlat_tables.mk

# Include cactus platform make file
CACTUS_PLAT_PATH	:= $(shell find spm/cactus/plat -wholename '*/${PLAT}')
ifneq (${CACTUS_PLAT_PATH},)
	include ${CACTUS_PLAT_PATH}/platform.mk
endif

CACTUS_DTB		:= $(BUILD_PLAT)/cactus.dtb
SECURE_PARTITIONS	+= cactus

CACTUS_INCLUDES :=					\
	-Itftf/framework/include			\
	-Iinclude					\
	-Iinclude/common				\
	-Iinclude/common/${ARCH}			\
	-Iinclude/lib					\
	-Iinclude/lib/extensions			\
	-Iinclude/lib/hob				\
	-Iinclude/lib/${ARCH}				\
	-Iinclude/lib/utils				\
	-Iinclude/lib/xlat_tables			\
	-Iinclude/plat/common				\
	-Iinclude/runtime_services			\
	-Ispm/cactus					\
	-Ispm/common					\
	-Ispm/common/sp_tests

CACTUS_SOURCES	:=					\
	$(addprefix spm/cactus/,			\
		aarch64/cactus_entrypoint.S		\
		aarch64/cactus_exceptions.S		\
		cactus_interrupt.c			\
		cactus_main.c				\
	)						\
	$(addprefix spm/common/,			\
		sp_debug.c				\
		sp_helpers.c				\
		spm_helpers.c				\
	)						\
	$(addprefix spm/common/sp_tests/,		\
		sp_test_ffa.c				\
		sp_test_cpu.c				\
	)						\
	$(addprefix spm/cactus/cactus_tests/,		\
		cactus_message_loop.c			\
		cactus_test_simd.c		\
		cactus_test_direct_messaging.c		\
		cactus_test_indirect_message.c	\
		cactus_test_interrupts.c		\
		cactus_test_memory_sharing.c		\
		cactus_tests_smmuv3.c			\
		cactus_test_notifications.c		\
		cactus_test_timer.c			\
	)

# TODO: Remove dependency on TFTF files.
CACTUS_SOURCES	+=							\
	tftf/framework/debug.c						\
	tftf/framework/${ARCH}/asm_debug.S				\
	tftf/tests/runtime_services/secure_service/${ARCH}/ffa_arch_helpers.S \
	tftf/tests/runtime_services/secure_service/ffa_helpers.c 	\
	tftf/tests/runtime_services/secure_service/spm_common.c		\
	tftf/framework/${ARCH}/exception_report.c			\
	lib/delay/delay.c

CACTUS_SOURCES	+= 	drivers/arm/pl011/${ARCH}/pl011_console.S	\
			drivers/arm/sp805/sp805.c			\
			lib/${ARCH}/cache_helpers.S			\
			lib/${ARCH}/misc_helpers.S			\
			lib/hob/hob.c					\
			lib/utils/uuid.c					\
			lib/smc/${ARCH}/asm_smc.S			\
			lib/smc/${ARCH}/smc.c				\
			lib/smc/${ARCH}/hvc.c				\
			lib/exceptions/${ARCH}/sync.c			\
			lib/locks/${ARCH}/spinlock.S			\
			lib/utils/mp_printf.c				\
			lib/extensions/fpu/fpu.c			\
			${XLAT_TABLES_LIB_SRCS}

CACTUS_LINKERFILE	:=	spm/cactus/cactus.ld.S

CACTUS_DEFINES	:=

$(eval $(call add_define,CACTUS_DEFINES,ARM_ARCH_MAJOR))
$(eval $(call add_define,CACTUS_DEFINES,ARM_ARCH_MINOR))
$(eval $(call add_define,CACTUS_DEFINES,DEBUG))
$(eval $(call add_define,CACTUS_DEFINES,ENABLE_ASSERTIONS))
$(eval $(call add_define,CACTUS_DEFINES,ENABLE_BTI))
$(eval $(call add_define,CACTUS_DEFINES,ENABLE_PAUTH))
$(eval $(call add_define,CACTUS_DEFINES,LOG_LEVEL))
$(eval $(call add_define,CACTUS_DEFINES,PLAT_${PLAT}))
$(eval $(call add_define,CACTUS_DEFINES,PLAT_XLAT_TABLES_DYNAMIC))
$(eval $(call add_define,CACTUS_DEFINES,SPMC_AT_EL3))
$(eval $(call add_define,CACTUS_DEFINES,CACTUS_PWR_MGMT_SUPPORT))

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

CACTUS_INCLUDES +=					\
	-Iinclude/runtime_services/secure_el1_payloads	\
	-Ismc_fuzz/include

# Needed for fuzz work
CACTUS_SOURCES	+= \
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

# Add definitions to CACTUS_DEFINES so they can be used in the code
$(eval $(call add_define,CACTUS_DEFINES,SMC_FUZZING))
$(eval $(call add_define,CACTUS_DEFINES,SMC_FUZZ_SEEDS))
$(eval $(call add_define,CACTUS_DEFINES,SMC_FUZZ_INSTANCE_COUNT))
$(eval $(call add_define,CACTUS_DEFINES,SMC_FUZZ_CALLS_PER_INSTANCE))
ifeq ($(SMC_FUZZER_DEBUG),1)
$(eval $(call add_define,CACTUS_DEFINES,SMC_FUZZER_DEBUG))
endif
ifeq ($(MULTI_CPU_SMC_FUZZER),1)
$(eval $(call add_define,CACTUS_DEFINES,MULTI_CPU_SMC_FUZZER))
endif
$(eval $(call add_define,CACTUS_DEFINES,SMC_FUZZ_SANITY_LEVEL))
$(eval $(call add_define,CACTUS_DEFINES,SMC_FUZZ_CALL_START))
$(eval $(call add_define,CACTUS_DEFINES,SMC_FUZZ_CALL_END))
$(eval $(call add_define,CACTUS_DEFINES,CONSTRAIN_EVENTS))
$(eval $(call add_define,CACTUS_DEFINES,EXCLUDE_FUNCID))
$(eval $(call add_define,CACTUS_DEFINES,INTR_ASSERT))
ifeq ($(SMC_FUZZ_VARIABLE_COVERAGE),1)
$(eval $(call add_define,CACTUS_DEFINES,SMC_FUZZ_VARIABLE_COVERAGE))
endif


endif

$(CACTUS_DTB) : $(BUILD_PLAT)/cactus $(BUILD_PLAT)/cactus/cactus.elf
$(CACTUS_DTB) : $(CACTUS_DTS)
	@echo "  DTBGEN  $@"
	${Q}tools/generate_dtb/generate_dtb.sh \
		cactus ${CACTUS_DTS} $(BUILD_PLAT) $(CACTUS_DTB)
	@echo
	@echo "Built $@ successfully"
	@echo

cactus: $(CACTUS_DTB) SP_LAYOUT

# FDTS_CP copies flattened device tree sources
#   $(1) = output directory
#   $(2) = flattened device tree source file to copy
define FDTS_CP
        $(eval FDTS := $(addprefix $(1)/,$(notdir $(2))))
FDTS_LIST += $(FDTS)
$(FDTS): $(2) $(CACTUS_DTB)
	@echo "  CP      $$<"
	${Q}cp $$< $$@
endef

ifdef FDTS_CP_LIST
        $(eval files := $(filter %.dts,$(FDTS_CP_LIST)))
        $(eval $(foreach file,$(files),$(call FDTS_CP,$(BUILD_PLAT),$(file))))
cactus: $(FDTS_LIST)
endif

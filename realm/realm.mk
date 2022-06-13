#
# Copyright (c) 2022, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include branch_protection.mk
include lib/xlat_tables_v2/xlat_tables.mk

REALM_INCLUDES :=							\
	-Itftf/framework/include					\
	-Iinclude							\
	-Iinclude/common						\
	-Iinclude/common/${ARCH}					\
	-Iinclude/lib							\
	-Iinclude/lib/${ARCH}						\
	-Iinclude/lib/utils						\
	-Iinclude/lib/xlat_tables					\
	-Iinclude/lib/extensions					\
	-Iinclude/plat/common						\
	-Iinclude/runtime_services					\
	-Iinclude/runtime_services/realm_payload			\
	-Iinclude/runtime_services/host_realm_managment			\
	-Irealm								\
	-Irealm/aarch64

REALM_SOURCES:=								\
	$(addprefix realm/,						\
	aarch64/realm_entrypoint.S					\
	aarch64/realm_exceptions.S					\
	realm_debug.c							\
	realm_payload_main.c						\
	realm_interrupt.c						\
	realm_rsi.c							\
	)

REALM_SOURCES	+=							\
	$(addprefix tftf/tests/runtime_services/host_realm_managment/,	\
		host_shared_data.c					\
	)

REALM_SOURCES += lib/${ARCH}/cache_helpers.S				\
	lib/${ARCH}/misc_helpers.S					\
	lib/smc/${ARCH}/asm_smc.S					\
	lib/smc/${ARCH}/smc.c						\
	lib/exceptions/${ARCH}/sync.c					\
	lib/locks/${ARCH}/spinlock.S					\
	lib/delay/delay.c						\
	lib/extensions/fpu/aarch64/fpu_halpers.S			\
	lib/extensions/fpu/fpu.c					\
	tftf/tests/runtime_services/secure_service/${ARCH}/ffa_arch_helpers.S	\
	tftf/framework/${ARCH}/exception_report.c			\
	${XLAT_TABLES_LIB_SRCS}

REALM_LINKERFILE:=	realm/realm.ld.S

REALM_DEFINES:=
$(eval $(call add_define,REALM_DEFINES,ARM_ARCH_MAJOR))
$(eval $(call add_define,REALM_DEFINES,ARM_ARCH_MINOR))
$(eval $(call add_define,REALM_DEFINES,ENABLE_BTI))
$(eval $(call add_define,REALM_DEFINES,ENABLE_PAUTH))
$(eval $(call add_define,REALM_DEFINES,LOG_LEVEL))
$(eval $(call add_define,REALM_DEFINES,PLAT_${PLAT}))
$(eval $(call add_define,REALM_DEFINES,IMAGE_REALM))

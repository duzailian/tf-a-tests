#
# Copyright (c) 2021-2022, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

TFTF_INCLUDES +=							\
	-Iinclude/runtime_services/host_realm_managment

TESTS_SOURCES	+=							\
	$(addprefix tftf/tests/runtime_services/realm_payload/,		\
		host_realm_payload_tests.c				\
		test_realm_spm.c					\
	)

TESTS_SOURCES	+=							\
	$(addprefix tftf/tests/runtime_services/host_realm_managment/,	\
		host_realm_rmi.c					\
		rmi_delegate_tests.c					\
	)

TESTS_SOURCES	+=							\
	$(addprefix tftf/tests/runtime_services/secure_service/,	\
		${ARCH}/ffa_arch_helpers.S				\
		ffa_helpers.c						\
		spm_common.c						\
	)

TESTS_SOURCES	+=							\
	$(addprefix lib/heap/,						\
		page_alloc.c						\
	)

ifeq (${ARCH},aarch64)
TESTS_SOURCES	+=							\
	$(addprefix tftf/tests/runtime_services/host_realm_managment/,	\
		host_realm_helper.c					\
		host_shared_data.c					\
	)
TESTS_SOURCES	+=							\
	$(addprefix lib/extensions/fpu/,				\
		${ARCH}/fpu_halpers.S					\
		fpu.c							\
	)
TESTS_SOURCES	+=							\
	$(addprefix lib/extensions/sve/,				\
		sve.c							\
		${ARCH}/sve_helpers.S					\
	)
endif
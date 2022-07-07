#
# Copyright (c) 2018-2022, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

TFTF_INCLUDES +=							\
	-Iinclude/runtime_services/host_realm_managment			\
	-Iinclude/runtime_services/realm_payload
TESTS_SOURCES	+=							\
	$(addprefix tftf/tests/runtime_services/host_realm_managment/,	\
		host_realm_rmi.c					\
		host_realm_helper.c					\
		host_shared_data.c					\
	)
TESTS_SOURCES	+=							\
	$(addprefix tftf/tests/runtime_services/secure_service/,	\
		${ARCH}/ffa_arch_helpers.S				\
		ffa_helpers.c						\
		spm_common.c						\
	)
TESTS_SOURCES	+=							\
	$(addprefix tftf/tests/runtime_services/secure_realm_service/,	\
		test_spm_rmm.c						\
	)
TESTS_SOURCES	+=							\
	$(addprefix lib/heap/,						\
		page_alloc.c						\
	)
TESTS_SOURCES	+=							\
	$(addprefix lib/extensions/sve/,				\
		sve.c							\
		aarch64/sve_helpers.S					\
	)
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
		host_realm_helper.c					\
		host_shared_data.c					\
	)

TESTS_SOURCES	+=							\
	$(addprefix lib/heap/,						\
		page_alloc.c						\
	)
#
# Copyright (c) 2021, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Include realm platform make file

TFTF_INCLUDES +=							\
	-Iinclude/runtime_services/host_realm_managment			\
	-Iinclude/runtime_services/realm_payload

TESTS_SOURCES	+=							\
	$(addprefix tftf/tests/runtime_services/realm_payload/,		\
		realm_payload_test.c					\
		realm_payload_spm_test.c				\
	)

TESTS_SOURCES	+=							\
	$(addprefix tftf/tests/runtime_services/host_realm_managment/,	\
		host_realm_rmi.c					\
		host_realm_helper.c				\
		host_shared_data.c					\
	)

TESTS_SOURCES	+=							\
	$(addprefix tftf/tests/runtime_services/secure_service/,	\
		${ARCH}/ffa_arch_helpers.S				\
		ffa_helpers.c						\
		spm_common.c						\
		test_ffa_direct_messaging.c				\
		test_ffa_interrupts.c					\
		test_ffa_memory_sharing.c				\
		test_ffa_setup_and_discovery.c				\
		test_spm_cpu_features.c					\
		test_spm_smmu.c						\
	)

TESTS_SOURCES	+=							\
	$(addprefix lib/heap/,						\
		page_alloc.c						\
	)
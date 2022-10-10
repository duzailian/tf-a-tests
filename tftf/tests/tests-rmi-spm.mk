#
# Copyright (c) 2021-2022, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

TFTF_INCLUDES +=							\
	-Iinclude/runtime_services/host_realm_managment

TESTS_SOURCES	+=							\
	$(addprefix tftf/tests/runtime_services/host_realm_managment/,	\
		host_realm_rmi.c					\
		rmi_spm_tests.c						\
		rmi_delegate_tests.c					\
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
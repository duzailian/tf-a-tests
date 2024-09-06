#
# Copyright (c) 2024, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

TESTS_SOURCES	+=						\
	$(addprefix tftf/tests/runtime_services/realm_payload/,	\
				host_realm_da.c			\
	)

TESTS_SOURCES	+=			\
	$(addprefix plat/arm/fvp/,	\
		fvp_pcie.c		\
	)

TESTS_SOURCES	+=							\
	$(addprefix tftf/tests/runtime_services/host_realm_managment/,	\
		host_pcie.c						\
		host_realm_rmi.c					\
		host_realm_helper.c					\
		host_shared_data.c					\
	)

TESTS_SOURCES	+=			\
	$(addprefix lib/pcie/,		\
		pcie.c			\
		pcie_doe.c		\
	)

TESTS_SOURCES	+=			\
	$(addprefix lib/heap/,		\
		page_alloc.c		\
	)

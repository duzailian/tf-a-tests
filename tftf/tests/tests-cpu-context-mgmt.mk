#
# Copyright (c) 2024, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#		context_mgmt_tests/el1/test_spm_cactus_el1_context_mgmt.c

TESTS_SOURCES	+=	$(addprefix tftf/tests/,				\
		context_mgmt_tests/el1/test_tsp_el1_context_mgmt.c		\
)

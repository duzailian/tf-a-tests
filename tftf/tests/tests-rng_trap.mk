#
# Copyright (c) 2022, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

TESTS_SOURCES	+=	$(addprefix tftf/tests/,			\
	extensions/rng_trap/test_rndr_trap_enabled.c			\
	extensions/rng_trap/test_rndr_trap_rng.c			\
	extensions/rng_trap/test_rndrrs_trap_enabled.c			\
	extensions/rng_trap/test_rndrrs_trap_rng.c			\
)

#
# Copyright (c) 2025, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

TESTS_SOURCES	+=	$(addprefix tftf/tests/neg_scenario_tests/,	\
					test_invalid_rotpk.c \
					neg_scenario_test_infra.c \
)
TESTS_SOURCES	+=							\
	$(addprefix tftf/tests/runtime_services/host_realm_managment/,	\
	)

TESTS_SOURCES	+=	plat/common/fwu_nvm_accessors.c \
					plat/common/image_loader.c \
					plat/arm/common/arm_fwu_io_storage.c \
					plat/arm/fvp/fvp_trusted_boot.c \

TESTS_SOURCES	+= $(addprefix drivers/auth/,	\
		img_parser_mod.c	\
	)


TESTS_SOURCES	+= $(addprefix ext/mbedtls/library/,	\
		error.c \
		memory_buffer_alloc.c \
	)

include lib/ext_mbedtls/mbedtls.mk


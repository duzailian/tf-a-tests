#
# Copyright (c) 2018, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

TESTS_SOURCES	+=	$(addprefix tftf/tests/fwu_tests/,	\
	test_fwu_auth.c						\
	test_fwu_toc.c						\
)

TESTS_SOURCES	+=	plat/common/fwu_nvm_accessors.c

#
# By default, a new test session is initialized each time the platform is
# booted, clearing any test state data stored in non-volatile memory (NVM).
# Because this test suite depends on preserving the test state across reboots,
# it is necessary to override the default behaviour and to prevent the NVM
# from being wiped indiscriminately. Note that the NVM is still cleared at the
# end of the test suite, after the last test has completed.
#
NEW_TEST_SESSION := 0

#
# By default, test results are stored in volatile memory as this is faster than
# using non-volatile memory (NVM). Since these tests require the system to reset
# the default behaviour must be to use the NVM instead.
#
USE_NVM := 1
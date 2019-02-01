#
# Copyright (c) 2018, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

TESTS_SOURCES	+=							\
	$(addprefix tftf/tests/runtime_services/standard_service/psci/api_tests/, \
		mem_protect/test_mem_protect.c				\
		psci_stat/test_psci_stat.c				\
		reset2/reset2.c 					\
		system_off/test_system_off.c 				\
	)

#
# By default, a new test session is initialized each time the platform is
# booted, clearing any test state data stored in non-volatile memory (NVM).
# Because this test suite depends on preserving the test state across reboots,
# it is necessary to override the default behaviour and to prevent the NVM
# from being wiped indiscriminately. Note that the NVM is still cleared at the
# end of the test suite, after the last test has completed.
#
NEW_TEST_SESSION := 0

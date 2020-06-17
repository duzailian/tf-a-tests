/*
 * Copyright (c) 2018-2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>
#include <debug.h>
#include <errno.h>
#include <sp_helpers.h>

#include "ffa_helpers.h"

/* FFA version test helpers */
#define FFA_MAJOR 1U
#define FFA_MINOR 0U

void ffa_tests(void)
{
	const char *test_ffa = "FFA Interfaces";
	const char *test_ffa_version = "FFA Version interface";

	announce_test_section_start(test_ffa);

	announce_test_start(test_ffa_version);

	smc_ret_values ret = ffa_version(MAKE_FFA_VERSION(FFA_MAJOR, FFA_MINOR));
	uint32_t spm_version = (uint32_t)(0xFFFFFFFF & ret.ret0);

	if (spm_version == FFA_ERROR_NOT_SUPPORTED) {
		ERROR("FFA_VERSION not supported\n");
	} else if ((spm_version & 0x80000000) != 0) {
		ERROR("FFA_VERSION bad response: %u\n", spm_version);
	} else {
		NOTICE("FFA_VERSION returned %u.%u\n",
				spm_version >> FFA_VERSION_MAJOR_SHIFT,
				spm_version & FFA_VERSION_MINOR_MASK);
	}

	announce_test_end(test_ffa_version);

	announce_test_section_end(test_ffa);
}

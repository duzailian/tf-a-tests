/*
 * Copyright (c) 2018-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <debug.h>
#include <errno.h>
#include <sp_helpers.h>
#include <spm_svc.h>
#include <stdint.h>
#include <tftf_lib.h>

#include "cactus_mm.h"
#include "cactus_mm_tests.h"

/*
 * Miscellaneous SPM tests.
 */
void misc_tests(void)
{
	int32_t version;

	const char *test_sect_desc = "miscellaneous";

	announce_test_section_start(test_sect_desc);

	const char *test_version = "SPM version check";

	announce_test_start(test_version);
	svc_args svc_values = { SPM_VERSION_AARCH32 };
	svc_ret_values svc_ret = tftf_svc(&svc_values);

	version = svc_ret.ret0;

	INFO("Version = 0x%x (%u.%u)\n", version,
	     (version >> 16) & 0x7FFF, version & 0xFFFF);
	expect(version, SPM_VERSION_COMPILED);
	announce_test_end(test_version);

	announce_test_section_end(test_sect_desc);
}

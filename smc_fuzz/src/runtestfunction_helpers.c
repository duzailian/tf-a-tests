/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define CMP_SUCCESS 0

#include <power_management.h>
#include <sdei.h>
#include <tftf_lib.h>
#include <timer.h>
#include <sdei_fuzz_helper.h>
#include <tsp_fuzz_helper.h>

#include <test_helpers.h>

/*
 * Invoke the SMC call based on the function name specified.
 */
void runtestfunction(char *funcstr)
{
	run_sdei_fuzz(funcstr);
	run_tsp_fuzz(funcstr);
}

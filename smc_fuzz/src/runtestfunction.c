/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <power_management.h>
#include <sdei.h>
#include <tftf_lib.h>
#include <timer.h>
#include <test_helpers.h>
#include <sdei_fuzz_helper.h>
#include <tsp_fuzz_helper.h>

/*
 * Running SMC call from what function name is selected
 */
void runtestfunction(char *funcstr)
{
	run_sdei_fuzz(funcstr);
	run_tsp_fuzz(funcstr);
}

/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <debug.h>
#include <realm_tests.h>
#include <realm_helpers.h>
#include <sync.h>

 /* Try to enable Branch recording in R-EL1 via BRBCR_EL1 */

bool test_realm_dc_ops(void)
{
	bool ret_result = true;

	u_register_t ccsidr_el1 = read_ccsidr_el1();
	u_register_t clidr_el1 = read_clidr_el1();

	u_register_t linesize = EXTRACT(CCSIDR_EL1_LINESIZE, ccsidr_el1);
	u_register_t ways = EXTRACT(CCSIDR_EL1_WAYS, ccsidr_el1);
	u_register_t sets = EXTRACT(CCSIDR_EL1_SETS, ccsidr_el1);

	INFO("CCSIDR_EL1 = 0x%lx\n", ccsidr_el1);
	INFO("CLIDR_EL1 = 0x%lx\n", clidr_el1);

	INFO("Data cache clean operations\n");
	dcsw_op_all(DCCSW);
	dcsw_op_all(DCISW);
	dcsw_op_all(DCCISW);

	INFO("linesize = 0x%lx\n", linesize);
	INFO("ways = 0x%lx\n", ways);
	INFO("sets = 0x%lx\n", sets);

	return ret_result;

}

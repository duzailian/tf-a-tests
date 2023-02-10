/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <arch_helpers.h>
#include <debug.h>
#include <mmio.h>
#include <tftf_lib.h>
#include <xlat_tables_v2.h>

/* Invalid address for fvp platform */
const uintptr_t test_address = 0x7FFFF000;

/*
 * These tests exercise the path in EL3 which reports crash dump from lower Els.
 *
 * These tests inject External aborts, sync and async(SError), to the system.
 * In conjuction with feature to handle EAs in EL3, these errors are trapped
 * in EL3. Since EL3 does not have handlers for these error it ends up in
 * crash reporting from lower ELs.
 *
 * Map non-existent memory as Device memory and try to access it, which will cause
 * External aborts. Based on type of access it traps either as sync EA(read) or as
 * SError(write).
 */

/* Try to write to non-existent Device memory to generate SError */
test_result_t test_inject_serror(void)
{
	int rc;

	rc = mmap_add_dynamic_region(test_address, test_address, PAGE_SIZE,
						MT_DEVICE | MT_RW | MT_NS);
	if (rc != 0) {
		tftf_testcase_printf("%d: mapping address %lu(%d) failed\n",
				      __LINE__, test_address, rc);
		return TEST_RESULT_FAIL;
	}

	/* Try writing to invalid address */
	mmio_write_32(test_address, 1);

	/* Should not come this far */
	return TEST_RESULT_FAIL;
}

/* Try to read non-existent Device memory to generate Sync EA */
test_result_t test_inject_syncEA(void)
{
	int rc;

	rc = mmap_add_dynamic_region(test_address, test_address, PAGE_SIZE,
						MT_DEVICE | MT_RO | MT_NS);
	if (rc != 0) {
		tftf_testcase_printf("%d: mapping address %lu(%d) failed\n",
				      __LINE__, test_address, rc);
		return TEST_RESULT_FAIL;
	}

	/* Try reading invalid address */
	rc = mmio_read_32(test_address);

	/* Should not come this far, print rc to avoid compiler optimization */
	ERROR("Reading invalid address did not cause syncEA, rc = %u\n", rc);

	return TEST_RESULT_FAIL;
}

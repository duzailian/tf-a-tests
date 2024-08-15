/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include <test_helpers.h>
#include <tftf_lib.h>
#include <tftf.h>
#include <sync.h>

static volatile bool undef_injection_triggered;

static bool trbe_trap_exception_handler(void)
{
	uint64_t esr_el2 = read_esr_el2();
	if (EC_BITS(esr_el2) == EC_UNKNOWN) {
		undef_injection_triggered = true;
		return true;
	}

	return false;
}

/*
 * EL3 is expected to allow access to trace control registers from EL2.
 * Reading these register will trap to EL3 and crash when EL3 has not
 * allowed access.
 */
test_result_t test_trbe_enabled(void)
{
	SKIP_TEST_IF_AARCH32();

#ifdef __aarch64__
	SKIP_TEST_IF_TRBE_NOT_SUPPORTED();

	undef_injection_triggered = false;

	register_custom_sync_exception_handler(trbe_trap_exception_handler);

	read_trblimitr_el1();
	read_trbptr_el1();
	read_trbbaser_el1();
	read_trbsr_el1();
	read_trbmar_el1();
	read_trbtrg_el1();
	read_trbidr_el1();

	unregister_custom_sync_exception_handler();

	if (undef_injection_triggered == true) {
		VERBOSE("UNDEF injection from EL3. TRBE is Disabled\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
#endif  /* __aarch64__ */
}

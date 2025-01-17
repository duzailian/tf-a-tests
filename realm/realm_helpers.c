/*
 * Copyright (c) 2024-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>

#include <realm_helpers.h>

#include <stdlib.h>

static unsigned int volatile realm_got_undef_abort;

/* Generate 64-bit random number */
unsigned long long realm_rand64(void)
{
	return ((unsigned long long)rand() << 32) | rand();
}

bool realm_sync_exception_handler(void)
{
	uint64_t esr_el1 = read_esr_el1();

	if (EC_BITS(esr_el1) == EC_UNKNOWN) {
		realm_printf("received undefined abort. "
			     "esr_el1: 0x%llx elr_el1: 0x%llx\n",
			     esr_el1, read_elr_el1());
		realm_got_undef_abort++;
	}

	return true;
}

void realm_reset_undef_abort_count(void)
{
	realm_got_undef_abort = 0U;
}

unsigned int realm_get_undef_abort_count(void)
{
	return realm_got_undef_abort;
}

/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdbool.h>

#include <arch_helpers.h>
#include <debug.h>
#include <sync.h>

static exception_handler_t custom_sync_exception_handler;

void register_custom_sync_exception_handler(exception_handler_t handler)
{
	custom_sync_exception_handler = handler;
}

void unregister_custom_sync_exception_handler(void)
{
	custom_sync_exception_handler = NULL;
}

bool tftf_sync_exception_handler(void)
{
	uint64_t elr_el2 = read_elr_el2();
	bool resume = false;

	if (custom_sync_exception_handler == NULL) {
		return false;
	}

	resume = custom_sync_exception_handler();

	if (resume) {
		write_elr_el2(elr_el2 + 4);
	}

	return resume;
}

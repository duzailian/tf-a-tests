/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdbool.h>

#include <arch_helpers.h>
#include <debug.h>
#include <serror.h>

static exception_handler_t custom_serror_handler;
static bool increment_pc = false;

void register_custom_serror_handler(exception_handler_t handler)
{
	custom_serror_handler = handler;
}

void unregister_custom_serror_handler(void)
{
	custom_serror_handler = NULL;
}

void enable_pc_increment_on_serror(void)
{
	increment_pc = true;
}

bool tftf_serror_handler(void)
{
	uint64_t elr_elx = IS_IN_EL2() ? read_elr_el2() : read_elr_el1();
	bool resume = false;

	if (custom_serror_handler == NULL) {
		return false;
	}

	resume = custom_serror_handler();

	if (resume && increment_pc) {
		/* Move ELR to next instruction to allow tftf to continue */
		if (IS_IN_EL2()) {
			write_elr_el2(elr_elx + 4U);
		} else {
			write_elr_el1(elr_elx + 4U);
		}
	}

	increment_pc = false;

	return resume;
}

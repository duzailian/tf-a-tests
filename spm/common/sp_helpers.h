/*
 * Copyright (c) 2018-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SP_HELPERS_H
#define SP_HELPERS_H

#include <stdint.h>
#include <tftf_lib.h>
#include <spm_common.h>
#include <spinlock.h>

/* Currently, Hafnium/SPM supports only 64 virtual interrupt IDs. */
#define NUM_VINT_ID	64

/*
 * Choose a pseudo-random number within the [min,max] range (both limits are
 * inclusive).
 */
uintptr_t bound_rand(uintptr_t min, uintptr_t max);

/*
 * Check that expr == expected.
 * If not, loop forever.
 */
void expect(int expr, int expected);

/*
 * Test framework functions
 */

void announce_test_section_start(const char *test_sect_desc);
void announce_test_section_end(const char *test_sect_desc);

void announce_test_start(const char *test_desc);
void announce_test_end(const char *test_desc);

/* Sleep for at least 'ms' milliseconds and return the elapsed time(ms). */
uint64_t sp_sleep_elapsed_time(uint32_t ms);

/* Sleep for at least 'ms' milliseconds. */
void sp_sleep(uint32_t ms);

void sp_handler_spin_lock_init(void);

/* Handler invoked at the tail end of interrupt processing by SP. */
extern void (*sp_interrupt_tail_end_handler[NUM_VINT_ID])(void);

/* Register the handler. */
void sp_register_interrupt_tail_end_handler(void (*handler)(void),
						uint32_t interrupt_id);

/* Un-register the handler. */
void sp_unregister_interrupt_tail_end_handler(uint32_t interrupt_id);

#endif /* SP_HELPERS_H */

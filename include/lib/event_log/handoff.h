/*
 * Copyright (c) 2024, ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef HANDOFF_H
#define HANDOFF_H

#include <stdint.h>
#include <lib/transfer_list.h>

uint8_t *transfer_list_event_log_init(struct transfer_list_header *tl, size_t *max_size);
uint8_t *transfer_list_event_log_finish(struct transfer_list_header *tl,
				uintptr_t cursor);
struct transfer_list_entry *
transfer_list_get_event_log_entry(struct transfer_list_header *tl);
size_t transfer_list_get_event_log_size(struct transfer_list_header *tl);

#define EVENT_LOG_RESERVED_BYTES U(4)

#endif /* HANDOFF_H */

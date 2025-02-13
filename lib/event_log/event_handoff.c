/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <string.h>

#include "event_log.h"
#include "transfer_list.h"

uint8_t *transfer_list_event_log_init(struct transfer_list_header *tl, size_t *max_size)
{
	struct transfer_list_entry *te, *prev_te = NULL;
	uint8_t *data = NULL;
	size_t offset = EVENT_LOG_RESERVED_BYTES;

	/*
	 * If a log is already present in the transfer list, resize it fit this
	 * stages event log. That may fail, in which case we need a new entry.
	 */
	te = transfer_list_get_event_log_entry(tl);
	if (te) {
		offset = te->data_size;
		prev_te = te;

		if (transfer_list_set_data_size(tl, te, *max_size + offset)) {
			VERBOSE("Event log TE re-sized at address %p\n", te);
			*max_size = te->data_size - offset;
			return (uint8_t *)(transfer_list_entry_data(te) +
					   offset);
		}
	}

	te = transfer_list_add(tl, TL_TAG_TPM_EVLOG, *max_size + offset,
			       NULL);
	if (te == NULL) {
		ERROR("Failed to add event log entry to TL at address %p\n",
		      tl);
		return NULL;
	}

	VERBOSE("Event log added to TL at address %p\n",
	     transfer_list_entry_data(te));

	if (prev_te != 0) {
		data = transfer_list_entry_data(prev_te);
		offset = prev_te->data_size;

		VERBOSE("Copying existing event log from %p\n", data);
		memmove(transfer_list_entry_data(te), data, offset);

		/*
		 * Delete the previous event log entry now we've copied it's contents
		 * over.
		 */
		transfer_list_rem(tl, prev_te);
	}

	*max_size = te->data_size - offset;
	return (uint8_t *)(transfer_list_entry_data(te) + offset);
}

uint8_t *transfer_list_event_log_finish(struct transfer_list_header *tl,
				uintptr_t cursor)
{
	uintptr_t te_data_start;

	struct transfer_list_entry *te = transfer_list_get_event_log_entry(tl);

	if (te) {
		/*
		 * Resize the TE to exactly the size of the event log to allow the next
		 * stage to know where to continue extending it from.
		 */
		te_data_start = (uintptr_t)transfer_list_entry_data(te);

		if (!transfer_list_set_data_size(tl, te,
						 cursor - te_data_start)) {
			WARN("Unable to resize event log TE.\n");
			return NULL;
		}

		transfer_list_update_checksum(tl);

		/*
		 * The measured boot finisher runs late, so ensure these changes are visible
		 * in memory for the next stage.
		 */
		flush_dcache_range((uintptr_t)tl, tl->size);
		return (uint8_t *)(te_data_start + EVENT_LOG_RESERVED_BYTES);
	}

	return NULL;
}

struct transfer_list_entry *
transfer_list_get_event_log_entry(struct transfer_list_header *tl)
{
	struct transfer_list_entry *te =
		transfer_list_find(tl, TL_TAG_TPM_EVLOG);
	if (te == NULL) {
		VERBOSE("Event log not found in TL at address %p\n", tl);
		return NULL;
	}

	return te;
}

size_t transfer_list_get_event_log_size(struct transfer_list_header *tl)
{
	struct transfer_list_entry *te = transfer_list_get_event_log_entry(tl);

	return te->data_size;
}

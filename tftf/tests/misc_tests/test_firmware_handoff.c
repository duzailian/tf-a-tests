/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <tftf_lib.h>
#include <test_helpers.h>
#include <transfer_list.h>

extern u_register_t hw_config_base;
extern u_register_t ns_tl;

#define DTB_PREAMBLE			U(0xedfe0dd0)
#define TL_TAG_FDT			U(1)

test_result_t test_handoff_header()
{
	struct transfer_list_header * tl = (struct transfer_list_header *) ns_tl;

	if (tl->signature != TRANSFER_LIST_SIGNATURE || tl->size > tl->max_size)
	{
		return TEST_RESULT_FAIL;
	}

	uint8_t byte_sum = 0;
	uint8_t *b = (uint8_t *) tl;

	for (size_t i = 0; i < tl->size; i++) {
		byte_sum += b[i];
	}

	if (byte_sum - tl->checksum == tl->checksum) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

test_result_t test_handoff_dtb_payload()
{
	tftf_testcase_printf("Validating HW_CONFIG from transfer list.\n");
	struct transfer_list_header * tl = (struct transfer_list_header *) ns_tl;
	struct transfer_list_entry * te = (void *)tl + tl->hdr_size;

	while (te->tag_id != TL_TAG_FDT) {
		te += round_up(te->hdr_size + te->data_size, tl->alignment);
	}

	uintptr_t dtb_ptr = (uintptr_t)te + te->hdr_size;

	if (dtb_ptr != hw_config_base && *(uint32_t*)dtb_ptr != DTB_PREAMBLE)
	{
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

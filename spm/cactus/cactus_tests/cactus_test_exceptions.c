/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "cactus_message_loop.h"
#include "cactus_test_cmds.h"
#include "cactus_tests.h"
#include <ffa_helpers.h>
#include <sp_helpers.h>
#include <debug.h>
#include <sync.h>

static volatile uint32_t data_abort_gpf_triggered;

static bool data_abort_gpf_handler(void)
{
	uint64_t esr_el1 = read_esr_el1();

	VERBOSE("%s count %u esr_el1 %llx elr_el1 %llx\n",
		__func__, data_abort_gpf_triggered, esr_el1,
		read_elr_el1());

	/* Expect a data abort because of a GPF. */
	if ((EC_BITS(esr_el1) == EC_DABORT_CUR_EL) &&
	    ((ISS_BITS(esr_el1) & 0x3f) == 0x28)) {
		data_abort_gpf_triggered++;
		return true;
	}

	return false;
}

CACTUS_CMD_HANDLER(exceptions_cmd, CACTUS_EXCEPTIONS_CMD)
{
	struct ffa_memory_region *m;
	struct ffa_composite_memory_region *composite;
	int ret;
	ffa_id_t source = ffa_dir_msg_source(*args);
	ffa_id_t vm_id = ffa_dir_msg_dest(*args);
	uint64_t handle = args->ret5;

	expect(memory_retrieve(mb, &m, handle, source, vm_id), true);

	composite = ffa_memory_region_get_composite(m, 0);

	VERBOSE("Address: %p; page_count: %x\n",
		composite->constituents[0].address,
		composite->constituents[0].page_count);

	/* This test is only concerned with RW permissions. */
	if (ffa_get_data_access_attr(
			m->receivers[0].receiver_permissions.permissions) !=
		FFA_DATA_ACCESS_RW) {
		ERROR("Permissions not expected!\n");
		return cactus_error_resp(vm_id, source, CACTUS_ERROR_TEST);
	}

	ret = mmap_add_dynamic_region(
			(uint64_t)composite->constituents[0].address,
			(uint64_t)composite->constituents[0].address,
			composite->constituents[0].page_count * PAGE_SIZE,
			MT_RW_DATA | MT_EXECUTE_NEVER);
	if (ret != 0) {
		ERROR("Failed mapping received memory region(%d)!\n", ret);
		return cactus_error_resp(vm_id, source, CACTUS_ERROR_TEST);
	}

	void *test_address = composite->constituents[0].address;

	VERBOSE("Attempt read access to realm region (%p)\n", test_address);

	register_custom_sync_exception_handler(data_abort_gpf_handler);
	expect(data_abort_gpf_triggered, 0);

	*((volatile uintptr_t*)test_address);

	unregister_custom_sync_exception_handler();
	expect(data_abort_gpf_triggered, 1);

	ret = mmap_remove_dynamic_region(
		(uint64_t)composite->constituents[0].address,
		composite->constituents[0].page_count * PAGE_SIZE);

	if (ret != 0) {
		ERROR("Failed unmapping received memory region(%d)!\n", ret);
		return cactus_error_resp(vm_id, source,
					 CACTUS_ERROR_TEST);
	}

	if (!memory_relinquish((struct ffa_mem_relinquish *)mb->send,
				m->handle, vm_id)) {
		return cactus_error_resp(vm_id, source,
					 CACTUS_ERROR_TEST);
	}

	if (ffa_func_id(ffa_rx_release()) != FFA_SUCCESS_SMC32) {
		ERROR("Failed to release buffer!\n");
		return cactus_error_resp(vm_id, source,
					 CACTUS_ERROR_FFA_CALL);
	}

	return cactus_success_resp(vm_id, source, 0);
}

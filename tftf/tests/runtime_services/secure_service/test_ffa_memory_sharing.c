/*
 * Copyright (c) 2020-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ffa_helpers.h"
#include <debug.h>

#include <cactus_test_cmds.h>
#include <ffa_endpoints.h>
#include <spm_common.h>
#include <spm_test_helpers.h>
#include <test_helpers.h>
#include <tftf_lib.h>
#include <xlat_tables_defs.h>

#define MAILBOX_SIZE PAGE_SIZE

#define SENDER HYP_ID
#define RECEIVER SP_ID(1)

static const struct ffa_uuid expected_sp_uuids[] = {
		{PRIMARY_UUID}, {SECONDARY_UUID}, {TERTIARY_UUID}
	};

/* Memory section to be used for memory share operations */
static __aligned(PAGE_SIZE) uint8_t share_page[PAGE_SIZE];
static __aligned(PAGE_SIZE) uint8_t consecutive_donate_page[PAGE_SIZE];

static bool check_written_words(uint32_t *ptr, uint32_t word, uint32_t wcount)
{
	VERBOSE("TFTF - Memory contents after SP use:\n");
	for (unsigned int i = 0U; i < wcount; i++) {
		VERBOSE("      %u: %x\n", i, ptr[i]);

		/* Verify content of memory is as expected. */
		if (ptr[i] != word) {
			return false;
		}
	}
	return true;
}

static bool test_memory_send_expect_denied(uint32_t mem_func,
					   void *mem_ptr,
					   ffa_id_t borrower)
{
	struct ffa_value ret;
	struct mailbox_buffers mb;
	struct ffa_memory_region_constituent constituents[] = {
						{(void *)mem_ptr, 1, 0}
					};
	ffa_memory_handle_t handle;

	const uint32_t constituents_count = sizeof(constituents) /
			sizeof(struct ffa_memory_region_constituent);
	GET_TFTF_MAILBOX(mb);

	handle = memory_init_and_send((struct ffa_memory_region *)mb.send,
					MAILBOX_SIZE, SENDER, borrower,
					constituents, constituents_count,
					mem_func, &ret);

	if (handle != FFA_MEMORY_HANDLE_INVALID) {
		ERROR("Received a valid FF-A memory handle, and that isn't"
		       " expected.\n");
		return false;
	}

	if (!is_expected_ffa_error(ret, FFA_ERROR_DENIED)) {
		return false;
	}

	return true;
}

/**
 * Test invocation to FF-A memory sharing interfaces that should return in an
 * error.
 */
test_result_t test_share_forbidden_ranges(void)
{
	const uintptr_t forbidden_address[] = {
		/* Cactus SP memory. */
		(uintptr_t)0x7200000,
		/* SPMC Memory. */
		(uintptr_t)0x6000000,
		/* NS memory defined in cactus tertiary. */
		(uintptr_t)0x0000880080001000,
	};

	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);

	for (unsigned i = 0; i < 3; i++) {
		if (!test_memory_send_expect_denied(
			FFA_MEM_SHARE_SMC32, (void *)forbidden_address[i],
			RECEIVER)) {
			return TEST_RESULT_FAIL;
		}
	}

	return TEST_RESULT_SUCCESS;
}

/**
 * Tests that it is possible to share memory with SWd from NWd.
 * After calling the respective memory send API, it will expect a reply from
 * cactus SP, at which point it will reclaim access to the memory region and
 * check the memory region has been used by receiver SP.
 *
 * Accessing memory before a memory reclaim operation should only be possible
 * in the context of a memory share operation.
 * According to the FF-A spec, the owner is temporarily relinquishing
 * access to the memory region on a memory lend operation, and on a
 * memory donate operation the access is relinquished permanently.
 * SPMC is positioned in S-EL2, and doesn't control stage-1 mapping for
 * EL2. Therefore, it is impossible to enforce the expected access
 * policy for a donate and lend operations within the SPMC.
 * Current SPMC implementation is under the assumption of trust that
 * Hypervisor (sitting in EL2) would relinquish access from EL1/EL0
 * FF-A endpoint at relevant moment.
 */
static test_result_t test_memory_send_sp(uint32_t mem_func, ffa_id_t borrower,
					 struct ffa_memory_region_constituent *constituents,
					 size_t constituents_count)
{
	struct ffa_value ret;
	ffa_memory_handle_t handle;
	uint32_t *ptr;
	struct mailbox_buffers mb;

	/* Arbitrarily write 5 words after using memory. */
	const uint32_t nr_words_to_write = 5;

	/***********************************************************************
	 * Check if SPMC has ffa_version and expected FFA endpoints are deployed.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);

	GET_TFTF_MAILBOX(mb);

	if (constituents_count != 1) {
		WARN("Test expects constituents_count to be 1\n");
	}

	for (size_t i = 0; i < constituents_count; i++) {
		VERBOSE("TFTF - Address: %p\n", constituents[0].address);
	}

	handle = memory_init_and_send((struct ffa_memory_region *)mb.send,
					MAILBOX_SIZE, SENDER, borrower,
					constituents, constituents_count,
					mem_func, &ret);

	if (handle == FFA_MEMORY_HANDLE_INVALID) {
		return TEST_RESULT_FAIL;
	}

	VERBOSE("TFTF - Handle: %llx\n", handle);

	ptr = (uint32_t *)constituents[0].address;

	ret = cactus_mem_send_cmd(SENDER, borrower, mem_func, handle, 0,
				  true, nr_words_to_write);

	if (!is_ffa_direct_response(ret)) {
		return TEST_RESULT_FAIL;
	}

	if (cactus_get_response(ret) != CACTUS_SUCCESS) {
		ERROR("Failed memory send operation!\n");
		return TEST_RESULT_FAIL;
	}

	/* Check that borrower used the memory as expected for this test. */
	if (!check_written_words(ptr, mem_func, nr_words_to_write)) {
		ERROR("Words written to shared memory, not as expected.\n");
		return TEST_RESULT_FAIL;
	}

	if (mem_func != FFA_MEM_DONATE_SMC32 &&
	    is_ffa_call_error(ffa_mem_reclaim(handle, 0))) {
			tftf_testcase_printf("Couldn't reclaim memory\n");
			return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

test_result_t test_mem_share_sp(void)
{
	struct ffa_memory_region_constituent constituents[] = {
		{(void *)share_page, 1, 0}
	};

	const uint32_t constituents_count = sizeof(constituents) /
				sizeof(struct ffa_memory_region_constituent);

	return test_memory_send_sp(FFA_MEM_SHARE_SMC32, RECEIVER, constituents,
				   constituents_count);
}

test_result_t test_mem_lend_sp(void)
{
	struct ffa_memory_region_constituent constituents[] = {
		{(void *)share_page, 1, 0}
	};

	const uint32_t constituents_count = sizeof(constituents) /
				sizeof(struct ffa_memory_region_constituent);

	return test_memory_send_sp(FFA_MEM_LEND_SMC32, RECEIVER, constituents,
				   constituents_count);
}

test_result_t test_mem_donate_sp(void)
{
	struct ffa_memory_region_constituent constituents[] = {
		{(void *)share_page, 1, 0}
	};
	const uint32_t constituents_count = sizeof(constituents) /
				sizeof(struct ffa_memory_region_constituent);
	return test_memory_send_sp(FFA_MEM_DONATE_SMC32, RECEIVER, constituents,
				   constituents_count);
}

test_result_t test_consecutive_donate(void)
{
	struct ffa_memory_region_constituent constituents[] = {
		{(void *)consecutive_donate_page, 1, 0}
	};
	const uint32_t constituents_count = sizeof(constituents) /
				sizeof(struct ffa_memory_region_constituent);

	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);

	test_result_t ret = test_memory_send_sp(FFA_MEM_DONATE_SMC32, SP_ID(1),
						constituents,
						constituents_count);

	if (ret != TEST_RESULT_SUCCESS) {
		ERROR("Failed at first attempting of sharing.\n");
		return TEST_RESULT_FAIL;
	}

	if (!test_memory_send_expect_denied(FFA_MEM_DONATE_SMC32,
					    consecutive_donate_page,
					    SP_ID(1))) {
		ERROR("Memory was successfully donated again from the NWd, to "
		      "the same borrower.\n");
		return TEST_RESULT_FAIL;
	}

	if (!test_memory_send_expect_denied(FFA_MEM_DONATE_SMC32,
					    consecutive_donate_page,
					    SP_ID(2))) {
		ERROR("Memory was successfully donated again from the NWd, to "
		      "another borrower.\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * Test requests a memory send operation between cactus SPs.
 * Cactus SP should reply to TFTF on whether the test succeeded or not.
 */
static test_result_t test_req_mem_send_sp_to_sp(uint32_t mem_func,
						ffa_id_t sender_sp,
						ffa_id_t receiver_sp,
						bool non_secure)
{
	struct ffa_value ret;

	/***********************************************************************
	 * Check if SPMC's ffa_version and presence of expected FF-A endpoints.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);

	ret = cactus_req_mem_send_send_cmd(HYP_ID, sender_sp, mem_func,
					   receiver_sp, non_secure);

	if (!is_ffa_direct_response(ret)) {
		return TEST_RESULT_FAIL;
	}

	if (cactus_get_response(ret) == CACTUS_ERROR) {
		ERROR("Failed sharing memory between SPs. Error code: %d\n",
			cactus_error_code(ret));
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/*
 * Test requests a memory send operation from SP to VM.
 * The tests expects cactus to reply CACTUS_ERROR, providing FF-A error code of
 * the last memory send FF-A call that cactus performed.
 */
static test_result_t test_req_mem_send_sp_to_vm(uint32_t mem_func,
						ffa_id_t sender_sp,
						ffa_id_t receiver_vm)
{
	struct ffa_value ret;

	/**********************************************************************
	 * Check if SPMC's ffa_version and presence of expected FF-A endpoints.
	 *********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);

	ret = cactus_req_mem_send_send_cmd(HYP_ID, sender_sp, mem_func,
					   receiver_vm, false);

	if (!is_ffa_direct_response(ret)) {
		return TEST_RESULT_FAIL;
	}

	if (cactus_get_response(ret) == CACTUS_ERROR &&
	    cactus_error_code(ret) == FFA_ERROR_DENIED) {
		return TEST_RESULT_SUCCESS;
	}

	tftf_testcase_printf("Did not get the expected error, "
			     "mem send returned with %d\n",
			     cactus_get_response(ret));
	return TEST_RESULT_FAIL;
}

test_result_t test_req_mem_share_sp_to_sp(void)
{
	return test_req_mem_send_sp_to_sp(FFA_MEM_SHARE_SMC32, SP_ID(3),
					  SP_ID(2), false);
}

test_result_t test_req_ns_mem_share_sp_to_sp(void)
{
	/*
	 * Skip the test when RME is enabled (for test setup reasons).
	 * For RME tests, the model specifies 48b physical address size
	 * at the PE, but misses allocating RAM and increasing the PA at
	 * the interconnect level.
	 */
	if (get_armv9_2_feat_rme_support() != 0U) {
		return TEST_RESULT_SKIPPED;
	}

	/* This test requires 48b physical address size capability. */
	SKIP_TEST_IF_PA_SIZE_LESS_THAN(48);

	return test_req_mem_send_sp_to_sp(FFA_MEM_SHARE_SMC32, SP_ID(3),
					  SP_ID(2), true);
}

test_result_t test_req_mem_lend_sp_to_sp(void)
{
	return test_req_mem_send_sp_to_sp(FFA_MEM_LEND_SMC32, SP_ID(3),
					  SP_ID(2), false);
}

test_result_t test_req_mem_donate_sp_to_sp(void)
{
	return test_req_mem_send_sp_to_sp(FFA_MEM_DONATE_SMC32, SP_ID(1),
					  SP_ID(3), false);
}

test_result_t test_req_mem_share_sp_to_vm(void)
{
	return test_req_mem_send_sp_to_vm(FFA_MEM_SHARE_SMC32, SP_ID(1),
					  HYP_ID);
}

test_result_t test_req_mem_lend_sp_to_vm(void)
{
	return test_req_mem_send_sp_to_vm(FFA_MEM_LEND_SMC32, SP_ID(2),
					  HYP_ID);
}

test_result_t test_mem_share_to_sp_clear_memory(void)
{
	struct ffa_memory_region_constituent constituents[] = {
						{(void *)share_page, 1, 1}};
	const uint32_t constituents_count = sizeof(constituents) /
			sizeof(struct ffa_memory_region_constituent);
	struct mailbox_buffers mb;
	uint32_t remaining_constituent_count;
	uint32_t total_length;
	uint32_t fragment_length;
	ffa_memory_handle_t handle;
	struct ffa_value ret;
	uint32_t *ptr;
	/* Arbitrarily write 10 words after using shared memory. */
	const uint32_t nr_words_to_write = 10U;

	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);

	GET_TFTF_MAILBOX(mb);

	remaining_constituent_count = ffa_memory_region_init_single_receiver(
		(struct ffa_memory_region *)mb.send, MAILBOX_SIZE, SENDER,
		RECEIVER, constituents, constituents_count, 0,
		FFA_MEMORY_REGION_FLAG_CLEAR, FFA_DATA_ACCESS_RW,
		FFA_INSTRUCTION_ACCESS_NOT_SPECIFIED,
		FFA_MEMORY_NOT_SPECIFIED_MEM, 0, 0, &total_length,
		&fragment_length);

	if (remaining_constituent_count != 0) {
		ERROR("Transaction descriptor initialization failed!\n");
		return TEST_RESULT_FAIL;
	}

	handle = memory_send(mb.send, FFA_MEM_LEND_SMC32, fragment_length,
			     total_length, &ret);

	if (handle == FFA_MEMORY_HANDLE_INVALID) {
		ERROR("Memory Share failed!\n");
		return TEST_RESULT_FAIL;
	}

	VERBOSE("Memory has been shared!\n");

	ret = cactus_mem_send_cmd(SENDER, RECEIVER, FFA_MEM_LEND_SMC32, handle,
				  FFA_MEMORY_REGION_FLAG_CLEAR, true,
				  nr_words_to_write);

	if (!is_ffa_direct_response(ret)) {
		return TEST_RESULT_FAIL;
	}

	if (cactus_get_response(ret) != CACTUS_SUCCESS) {
		ERROR("Failed memory send operation!\n");
		return TEST_RESULT_FAIL;
	}

	ret = ffa_mem_reclaim(handle, 0);

	if (is_ffa_call_error(ret)) {
		ERROR("Memory reclaim failed!\n");
		return TEST_RESULT_FAIL;
	}

	ptr = (uint32_t *)constituents[0].address;

	/* Check that borrower used the memory as expected for this test. */
	if (!check_written_words(ptr, FFA_MEM_LEND_SMC32, nr_words_to_write)) {
		ERROR("Words written to shared memory, not as expected.\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

test_result_t compare_ffa_memory_region_constituents(
	struct ffa_memory_region_constituent *rx_constituent,
	struct ffa_memory_region_constituent *tx_constituent)
{
	EXPECT_EQ(rx_constituent->address, tx_constituent->address);
	EXPECT_EQ(rx_constituent->page_count, tx_constituent->page_count);
	EXPECT_EQ(rx_constituent->reserved, tx_constituent->reserved);
	EXPECT_EQ(rx_constituent->reserved, 0);

	return TEST_RESULT_SUCCESS;
}

test_result_t compare_ffa_composite_memory_regions(
	struct ffa_composite_memory_region *rx_composite_region,
	struct ffa_composite_memory_region *tx_composite_region)
{

	EXPECT_EQ(rx_composite_region->page_count,
		  tx_composite_region->page_count);
	EXPECT_EQ(rx_composite_region->constituent_count,
		  tx_composite_region->constituent_count);
	EXPECT_EQ(rx_composite_region->reserved_0,
		  tx_composite_region->reserved_0);
	EXPECT_EQ(rx_composite_region->reserved_0, 0);

	return TEST_RESULT_SUCCESS;
}

test_result_t compare_ffa_memory_region_attributes(
	struct ffa_memory_region_attributes *rx_attrs,
	struct ffa_memory_region_attributes *tx_attrs)
{

	EXPECT_EQ(rx_attrs->receiver, tx_attrs->receiver);
	EXPECT_EQ(rx_attrs->permissions, tx_attrs->permissions);
	EXPECT_EQ(rx_attrs->flags, tx_attrs->flags);

	return TEST_RESULT_SUCCESS;
}

test_result_t compare_ffa_memory_accesses(struct ffa_memory_access *rx_access,
					  struct ffa_memory_access *tx_access)
{

	EXPECT_EQ(rx_access->composite_memory_region_offset,
		  tx_access->composite_memory_region_offset);
	TRY(compare_ffa_memory_region_attributes(
		&rx_access->receiver_permissions,
		&tx_access->receiver_permissions));
	EXPECT_EQ(rx_access->reserved_0, tx_access->reserved_0);
	EXPECT_EQ(rx_access->reserved_0, 0);

	return TEST_RESULT_SUCCESS;
}

test_result_t compare_ffa_memory_regions(struct ffa_memory_region *rx_region,
					 struct ffa_memory_region *tx_region)
{

	// TODO: these compares fail
	// EXPECT_EQ(rx_region->flags, tx_region->flags);
	// EXPECT_EQ(rx_region->attributes, tx_region->attributes);

	EXPECT_EQ(rx_region->sender, tx_region->sender);
	EXPECT_EQ(rx_region->handle, tx_region->handle);
	EXPECT_EQ(rx_region->tag, tx_region->tag);
	EXPECT_EQ(rx_region->memory_access_desc_size,
		  tx_region->memory_access_desc_size);
	EXPECT_EQ(rx_region->receiver_count, tx_region->receiver_count);
	EXPECT_EQ(rx_region->receivers_offset, tx_region->receivers_offset);

	EXPECT_EQ(rx_region->reserved[0], tx_region->reserved[0]);
	EXPECT_EQ(rx_region->reserved[1], tx_region->reserved[1]);
	EXPECT_EQ(rx_region->reserved[2], tx_region->reserved[2]);

	EXPECT_EQ(rx_region->reserved[0], 0);
	EXPECT_EQ(rx_region->reserved[1], 0);
	EXPECT_EQ(rx_region->reserved[2], 0);

	return TEST_RESULT_SUCCESS;
}

test_result_t hypervisor_retrieve_request_test_helper(uint32_t mem_func)
{

	struct ffa_memory_region_constituent tx_constituents[] = {
		{(void *)share_page, 1, 0}};
	uint32_t sent_constituents_count = ARRAY_SIZE(tx_constituents);
	struct mailbox_buffers mb;
	ffa_memory_handle_t handle;
	struct ffa_value ret;
	struct ffa_memory_region sent_region;
	struct ffa_memory_region *hypervisor_retrieve_response;

	uint32_t permissions = mem_func == FFA_MEM_DONATE_SMC32
				       ? FFA_DATA_ACCESS_NOT_SPECIFIED
				       : FFA_DATA_ACCESS_RW;

	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);
	GET_TFTF_MAILBOX(mb);

	/* Share */
	handle = memory_init_and_send(mb.send, MAILBOX_SIZE, SENDER, RECEIVER,
				      tx_constituents, sent_constituents_count,
				      mem_func, &ret);
	if (handle == FFA_MEMORY_HANDLE_INVALID) {
		ERROR("Memory share failed: %d\n", ffa_error_code(ret));
		return TEST_RESULT_FAIL;
	}

	memcpy(&sent_region, mb.send, sizeof(sent_region));
	struct ffa_memory_access sent_receivers[sent_region.receiver_count];
	memcpy(sent_receivers, ((struct ffa_memory_region *)mb.send)->receivers,
	       sizeof(sent_receivers));

	struct ffa_composite_memory_region
		sent_composite_regions[sent_region.receiver_count];
	for (uint32_t i = 0; i < sent_region.receiver_count; i++) {
		memcpy(&sent_composite_regions[i],
		       ffa_memory_region_get_composite(mb.send, i),
		       sizeof(struct ffa_composite_memory_region));
	}

	/*
	 * Send Hypervisor Retrieve request according to section 17.4.3 of FFA
	 * v1.2-REL0 specification
	 */
	if (!memory_retrieve(&mb, &hypervisor_retrieve_response, handle, SENDER,
			     RECEIVER, 0))
	{
		return TEST_RESULT_FAIL;
	}

	/*
	 * Verify the received `FFA_MEM_RETRIEVE_RESP` aligns with
	 * transaction description sent above
	 */
	TRY(compare_ffa_memory_regions(hypervisor_retrieve_response,
				       &sent_region));

	for (uint32_t i = 0; i < hypervisor_retrieve_response->receiver_count;
	     i++)
	{
		struct ffa_memory_access rx_access = {
			.reserved_0 = 0,
			.composite_memory_region_offset = 64,
			.receiver_permissions = {.permissions = permissions,
						 .receiver = RECEIVER,
						 .flags = 0},

		};
		TRY(compare_ffa_memory_accesses(&rx_access,
						&sent_receivers[i]));

		struct ffa_composite_memory_region *rx_composite =
			ffa_memory_region_get_composite(
				hypervisor_retrieve_response, i);
		struct ffa_composite_memory_region *tx_composite =
			&sent_composite_regions[i];
		TRY(compare_ffa_composite_memory_regions(rx_composite,
							 tx_composite));

		for (uint32_t j = 0; j < rx_composite->constituent_count; j++) {
			TRY(compare_ffa_memory_region_constituents(
				&rx_composite->constituents[j],
				&tx_constituents[j]));
		}
	}

	/* Reclaim for the SPMC to deallocate any data related to the
	   handle. */
	ret = ffa_mem_reclaim(handle, 0);
	if (is_ffa_call_error(ret)) {
		return TEST_RESULT_FAIL;
	}

	ret = ffa_rx_release();
	if (is_ffa_call_error(ret)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

test_result_t share_retrieve_helper_multiple_receivers(
	uint32_t mem_func, uint32_t region_type,
	enum ffa_data_access data_access,
	enum ffa_instruction_access instruction_access)
{

	struct ffa_memory_region_constituent tx_constituents[] = {
		{(void *)share_page, 1, 0}};
	const uint32_t sent_constituents_count = ARRAY_SIZE(tx_constituents);
	struct mailbox_buffers mb;
	ffa_memory_handle_t handle;
	struct ffa_value ret;
	struct ffa_memory_region tx_region;
	struct ffa_memory_region *rx_region;

	ffa_id_t hypervisor = HYP_ID;
	ffa_id_t sp = SP_ID(1);

	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);
	GET_TFTF_MAILBOX(mb);

	ffa_memory_access_permissions_t permissions = 0;
	ffa_set_data_access_attr(&permissions, data_access);
	ffa_set_instruction_access_attr(&permissions, instruction_access);

	struct ffa_memory_access receivers[2] = {
		{
			.receiver_permissions = {.permissions = permissions,
						 .receiver = SP_ID(1),
						 .flags = 0},
			.reserved_0 = 0,
		},
		{
			.receiver_permissions = {.permissions = permissions,
						 .receiver = SP_ID(2),
						 .flags = 0},
			.reserved_0 = 0,
		}};

	/* Share */
	handle = memory_init_and_send_multiple_receivers(
		mb.send, MAILBOX_SIZE, hypervisor, receivers,
		ARRAY_SIZE(receivers), tx_constituents, sent_constituents_count,
		mem_func, &ret);
	if (handle == FFA_MEMORY_HANDLE_INVALID) {
		ERROR("Memory share failed: %d\n", ffa_error_code(ret));
		return TEST_RESULT_FAIL;
	}

	memcpy(&tx_region, mb.send, sizeof(tx_region));
	struct ffa_memory_access tx_receivers[tx_region.receiver_count];
	memcpy(tx_receivers, ((struct ffa_memory_region *)mb.send)->receivers,
	       sizeof(tx_receivers));

	struct ffa_composite_memory_region
		tx_composite_regions[tx_region.receiver_count];
	for (uint32_t i = 0; i < tx_region.receiver_count; i++) {
		memcpy(&tx_composite_regions[i],
		       ffa_memory_region_get_composite(mb.send, i),
		       sizeof(struct ffa_composite_memory_region));
	}

	/*
	 * Send Hypervisor Retrieve request according to section 16.4.3 of
	 * FFA v1.1-REL0 specification
	 */
	ffa_memory_retrieve_request_init(mb.send, handle, hypervisor, 0, 0, 0,
					 0, 0, 0, 0, 0);
	((struct ffa_memory_region *)mb.send)->receiver_count = 0;

	ret = ffa_mem_retrieve_req(sizeof(struct ffa_memory_region),
				   sizeof(struct ffa_memory_region));
	if (ffa_func_id(ret) != FFA_MEM_RETRIEVE_RESP) {
		ERROR("Memory retrieve failed: (FID = %lu, error = %d)\n",
		      ret.fid, ffa_error_code(ret));
		return TEST_RESULT_FAIL;
	}

	/*
	 * Verify the received `FFA_MEM_RETRIEVE_RESP` aligns with
	 * transaction description sent above
	 */
	rx_region = mb.recv;
	TRY(compare_ffa_memory_regions(rx_region, &tx_region, region_type));

	for (uint32_t i = 0; i < rx_region->receiver_count; i++) {
		struct ffa_memory_access rx_access = {
			.reserved_0 = 0,
			/* sizeof(ffa_memory_region) + receiver_count *
			   sizeof(ffa_memory_access), */
			.composite_memory_region_offset = 80,
			.receiver_permissions = {.permissions = permissions,
						 .receiver = sp,
						 .flags = 0},

		};
		struct ffa_memory_access tx_access = tx_receivers[i];
		TRY(compare_ffa_memory_accesses(&rx_access, &tx_access));

		struct ffa_composite_memory_region *rx_composite =
			ffa_memory_region_get_composite(rx_region, i);
		struct ffa_composite_memory_region *tx_composite =
			&tx_composite_regions[i];
		TRY(compare_ffa_composite_memory_regions(rx_composite,
							 tx_composite));

		for (uint32_t j = 0; j < rx_composite->constituent_count; j++) {
			TRY(compare_ffa_memory_region_constituents(
				&rx_composite->constituents[j],
				&tx_constituents[j]));
		}
	}

	/* Reclaim for the SPMC to deallocate any data related to the handle. */
	ret = ffa_mem_reclaim(handle, 0);
	if (is_ffa_call_error(ret)) {
		ERROR("Memory reclaim failed: %d\n", ffa_error_code(ret));
		return TEST_RESULT_FAIL;
	}

	ret = ffa_rx_release();
	if (is_ffa_call_error(ret)) {
		ERROR("rx release failed: %d\n", ffa_error_code(ret));
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

test_result_t test_hypervisor_share_retrieve(void)
{
	return hypervisor_retrieve_request_test_helper(FFA_MEM_SHARE_SMC32);
}

test_result_t test_hypervisor_lend_retrieve(void)
{
	return hypervisor_retrieve_request_test_helper(FFA_MEM_LEND_SMC32);
}

test_result_t test_hypervisor_donate_retrieve(void)
{
	return hypervisor_retrieve_request_test_helper(FFA_MEM_DONATE_SMC32);
}

test_result_t test_hypervisor_share_retrieve_multiple_receivers(void)
{
	return share_retrieve_helper_multiple_receivers(
		FFA_MEM_SHARE_SMC32, FFA_MEMORY_REGION_TRANSACTION_TYPE_SHARE,
		FFA_DATA_ACCESS_RW, FFA_INSTRUCTION_ACCESS_NOT_SPECIFIED);
}

test_result_t test_hypervisor_lend_retrieve_multiple_receivers(void)
{
	return share_retrieve_helper_multiple_receivers(
		FFA_MEM_LEND_SMC32, FFA_MEMORY_REGION_TRANSACTION_TYPE_LEND,
		FFA_DATA_ACCESS_RW, FFA_INSTRUCTION_ACCESS_NOT_SPECIFIED);
}

test_result_t test_hypervisor_donate_retrieve_multiple_receivers(void)
{
	struct ffa_memory_region_constituent tx_constituents[] = {
		{(void *)share_page, 1, 0}};
	const uint32_t sent_constituents_count = ARRAY_SIZE(tx_constituents);
	struct mailbox_buffers mb;
	ffa_memory_handle_t handle;
	struct ffa_value ret;

	ffa_id_t hypervisor = HYP_ID;

	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);
	GET_TFTF_MAILBOX(mb);

	ffa_memory_access_permissions_t permissions = 0;
	ffa_set_data_access_attr(&permissions, FFA_DATA_ACCESS_RW);
	ffa_set_instruction_access_attr(&permissions,
					FFA_INSTRUCTION_ACCESS_NOT_SPECIFIED);

	struct ffa_memory_access receivers[2] = {
		{
			.receiver_permissions = {.permissions = permissions,
						 .receiver = SP_ID(1),
						 .flags = 0},
			.reserved_0 = 0,
		},
		{
			.receiver_permissions = {.permissions = permissions,
						 .receiver = SP_ID(2),
						 .flags = 0},
			.reserved_0 = 0,
		}};

	/* Share */
	handle = memory_init_and_send_multiple_receivers(
		mb.send, MAILBOX_SIZE, hypervisor, receivers,
		ARRAY_SIZE(receivers), tx_constituents, sent_constituents_count,
		FFA_MEM_DONATE_SMC32, &ret);

	/* Share should fail, because donate only supports one reciever */
	EXPECT_EQ(handle, FFA_MEMORY_HANDLE_INVALID);
	EXPECT_EQ(ffa_error_code(ret), -2);

	return TEST_RESULT_SUCCESS;
}

/*
 * Copyright (c) 2020-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "arch_features.h"
#include "arch_helpers.h"
#include "ffa_helpers.h"
#include "ffa_svc.h"
#include "stdint.h"
#include "utils_def.h"
#include <debug.h>
#include <sync.h>

#include <cactus_test_cmds.h>
#include <ffa_endpoints.h>
#include <host_realm_rmi.h>
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
static __aligned(PAGE_SIZE) uint8_t donate_page[PAGE_SIZE];
static __aligned(PAGE_SIZE) uint8_t consecutive_donate_page[PAGE_SIZE];
static __aligned(PAGE_SIZE) uint8_t four_share_pages[PAGE_SIZE * 4];

static bool gpc_abort_triggered;

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

static bool data_abort_handler(void)
{
	uint64_t esr_elx = IS_IN_EL2() ? read_esr_el2() : read_esr_el1();

	VERBOSE("%s esr_elx %llx\n", __func__, esr_elx);

	if (EC_BITS(esr_elx) == EC_DABORT_CUR_EL) {
		/* Synchronous data abort triggered by Granule protection */
		if ((ISS_BITS(esr_elx) & ISS_DFSC_MASK) == DFSC_GPF_DABORT) {
			VERBOSE("%s GPF Data Abort caught to address: %llx\n",
				__func__, (uint64_t)read_far_el2());
			gpc_abort_triggered = true;
			return true;
		}
	}

	return false;
}

static bool get_gpc_abort_triggered(void)
{
	bool ret = gpc_abort_triggered;

	gpc_abort_triggered = false;

	return ret;
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
	unsigned int rme_supported = get_armv9_2_feat_rme_support();
	const bool check_gpc_fault =
		mem_func != FFA_MEM_SHARE_SMC32 &&
		rme_supported != 0U;

	/* Arbitrarily write 5 words after using memory. */
	const uint32_t nr_words_to_write = 5;

	/***********************************************************************
	 * Check if SPMC has ffa_version and expected FFA endpoints are deployed.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);

	GET_TFTF_MAILBOX(mb);

	/*
	 * If the RME is enabled for the platform under test, check that the
	 * GPCs are working as expected, as such setup the exception handler.
	 */
	if (check_gpc_fault) {
		register_custom_sync_exception_handler(data_abort_handler);
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
				  nr_words_to_write);

	if (!is_ffa_direct_response(ret) ||
	    cactus_get_response(ret) != CACTUS_SUCCESS) {
		ffa_mem_reclaim(handle, 0);
		ERROR("Failed memory send operation!\n");
		return TEST_RESULT_FAIL;
	}

	/*
	 * If there is RME support, look to trigger an exception as soon as the
	 * security state is update, due to GPC fault.
	 */
	if (check_gpc_fault) {
		*ptr = 0xBEEF;
	}

	if (mem_func != FFA_MEM_DONATE_SMC32) {

		/* Reclaim memory entirely before checking its state. */
		if (is_ffa_call_error(ffa_mem_reclaim(handle, 0))) {
			tftf_testcase_printf("Couldn't reclaim memory\n");
			return TEST_RESULT_FAIL;
		}

		for (uint32_t i = 0; i < constituents_count; i++) {
			ptr = constituents[i].address;

			/*
			 * Check that borrower used the memory as expected for this
			 * test.
			 */
			if (!check_written_words(ptr, mem_func,
						 nr_words_to_write)) {
				ERROR("Fail because of state of memory.\n");
				return TEST_RESULT_FAIL;
			}
		}
	}

	if (check_gpc_fault) {
		unregister_custom_sync_exception_handler();
		if (!get_gpc_abort_triggered()) {
			ERROR("No exception due to GPC for lend/donate with RME.\n");
			return TEST_RESULT_FAIL;
		}
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
		{(void *)four_share_pages, 4, 0},
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
		{(void *)donate_page, 1, 0}
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
		{(void *)four_share_pages, 4, 0},
		{(void *)share_page, 1, 0}
	};

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

	remaining_constituent_count = ffa_memory_region_init(
		(struct ffa_memory_region *)mb.send, MAILBOX_SIZE, SENDER,
		RECEIVER, constituents, constituents_count, 0,
		FFA_MEMORY_REGION_FLAG_CLEAR, FFA_DATA_ACCESS_RW,
		FFA_INSTRUCTION_ACCESS_NOT_SPECIFIED,
		FFA_MEMORY_NOT_SPECIFIED_MEM, 0, 0,
		&total_length, &fragment_length);

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
				  FFA_MEMORY_REGION_FLAG_CLEAR, nr_words_to_write);

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

/**
 * Test helper that perfoms memory sharing operation, and alters the PAS
 * of the memory, to validate that SPM intersects operation in case the PAS
 * is not coherent with its use. Relevant for the functioning of FFA_MEM_LEND
 * and FFA_MEM_DONATE from NWd to an SP.
 *
 * In cases the memory is not in NS state, the SPMC should intersect memory
 * management call with and appropriate FFA_ERROR.
 */
static test_result_t test_ffa_mem_send_realm_expect_fail(
		uint32_t mem_func, ffa_id_t borrower,
		struct ffa_memory_region_constituent *constituents,
		size_t constituents_count, uint64_t delegate_addr)
{
	struct ffa_value ret;
	uint32_t remaining_constituent_count;
	uint32_t total_length;
	uint32_t fragment_length;
	struct mailbox_buffers mb;
	u_register_t ret_rmm;
	test_result_t result = TEST_RESULT_FAIL;

	if (get_armv9_2_feat_rme_support() == 0U) {
		return TEST_RESULT_SKIPPED;
	}

	/***********************************************************************
	 * Check if SPMC has ffa_version and expected FFA endpoints are deployed.
	 **********************************************************************/
	CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);

	GET_TFTF_MAILBOX(mb);

	register_custom_sync_exception_handler(data_abort_handler);

	/*
	 * Delegate page to a realm. This should make memory sharing operation
	 * fail.
	 */
	ret_rmm = host_rmi_granule_delegate((u_register_t)delegate_addr);

	if (ret_rmm != 0UL) {
		INFO("Delegate operation returns 0x%lx for address %llx\n",
		     ret_rmm, delegate_addr);
		return TEST_RESULT_FAIL;
	}

	remaining_constituent_count = ffa_memory_region_init(
		(struct ffa_memory_region *)mb.send, MAILBOX_SIZE, SENDER,
		RECEIVER, constituents, constituents_count, 0,
		0, FFA_DATA_ACCESS_RW,
		FFA_INSTRUCTION_ACCESS_NOT_SPECIFIED,
		FFA_MEMORY_NOT_SPECIFIED_MEM, 0, 0,
		&total_length, &fragment_length);

	if (remaining_constituent_count != 0) {
		goto out;
	}

	ret = ffa_mem_lend(total_length, fragment_length);

	if (!is_expected_ffa_error(ret, FFA_ERROR_DENIED)) {
		goto out;
	}

	/* Undelegate to reestablish the same security state for PAS. */
	ret_rmm = host_rmi_granule_undelegate((u_register_t)delegate_addr);

	for (uint32_t i = 0; i < constituents_count; i++) {
		uint32_t *ptr = (uint32_t *)constituents[i].address;

		/*
		 * Check that if memory lend/donate has failed, the memory is
		 * remains in the NS PAS.
		 */
		*ptr = 0xFFA;
	}

	if (get_gpc_abort_triggered()) {
		ERROR("Exception due to GPC for lend/donate with RME. Not"
		      " expected for this case.\n");
		result = TEST_RESULT_FAIL;
	} else {
		result = TEST_RESULT_SUCCESS;
	}
out:
	unregister_custom_sync_exception_handler();

	if (ret_rmm != 0UL) {
		INFO("Undelegate operation returns 0x%lx for address %llx\n",
		     ret_rmm, (uint64_t)delegate_addr);
		return TEST_RESULT_FAIL;
	}

	return result;
}

/**
 * Memory to be shared between partitions is described in a composite, with
 * various constituents. In an RME system, the memory must be in NS PAS in
 * operations from NWd to an SP. In case the PAS is not following this
 * expectation memory lend/donate should fail, and all constituents must
 * remain in the NS PAS.
 *
 * This test validates that if one page in the middle of one of the constituents
 * is not in the NS PAS the operation fails.
 */
test_result_t test_ffa_mem_lend_sp_realm_memory(void)
{
	test_result_t ret;
	struct ffa_memory_region_constituent constituents[] = {
		{(void *)four_share_pages, 4, 0},
		{(void *)share_page, 1, 0}
	};

	const uint32_t constituents_count = sizeof(constituents) /
				sizeof(struct ffa_memory_region_constituent);

	for (unsigned int i = 0; i < 4; i++) {
		/* Address to be delegated to Realm PAS. */
		uint64_t realm_addr = (uint64_t)&four_share_pages[i * PAGE_SIZE];

		INFO("Sharing memory with realm addr: %llx\n", realm_addr);

		ret = test_ffa_mem_send_realm_expect_fail(
			FFA_MEM_LEND_SMC32, SP_ID(1), constituents,
			constituents_count, realm_addr);

		if (ret != TEST_RESULT_SUCCESS) {
			break;
		}
	}

	return ret;
}

/**
 * Memory to be shared between partitions is described in a composite, with
 * various constituents. In an RME system, the memory must be in NS PAS in
 * operations from NWd to an SP. In case the PAS is not following this
 * expectation memory lend/donate should fail, and all constituents must
 * remain in the NS PAS.
 *
 * This test validates the case in which the memory lend/donate fail in
 * in case one of the constituents in the composite is not in the NS PAS.
 */
test_result_t test_ffa_mem_lend_sp_realm_memory_separate_constituent(void)
{
	test_result_t ret;
	struct ffa_memory_region_constituent constituents[] = {
		{(void *)four_share_pages, 4, 0},
		{(void *)share_page, 1, 0}
	};
	const uint32_t constituents_count = sizeof(constituents) /
				sizeof(struct ffa_memory_region_constituent);
	/* Address to be delegated to Realm PAS. */
	uint64_t realm_addr = (uint64_t)&share_page[0];

	INFO("Sharing memory with realm addr: %llx\n", realm_addr);

	ret = test_ffa_mem_send_realm_expect_fail(
		FFA_MEM_LEND_SMC32, SP_ID(1), constituents,
		constituents_count, realm_addr);

	return ret;
}

/*
 * Copyright (c) 2018-2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>
#include <smccc.h>

#include <arch_helpers.h>
#include <cactus_test_cmds.h>
#include <ffa_endpoints.h>
#include <ffa_svc.h>
#include <platform.h>
#include <test_helpers.h>
#include <spm_common.h>

static const struct ffa_uuid expected_sp_uuids[] = {
		{PRIMARY_UUID}, {SECONDARY_UUID}, {TERTIARY_UUID}
};

static uint64_t g_notifications = FFA_NOTIFICATION(0)  |
				  FFA_NOTIFICATION(1)  |
				  FFA_NOTIFICATION(30) |
				  FFA_NOTIFICATION(50) |
				  FFA_NOTIFICATION(63);

/**
 * Helper to create bitmap for NWd VMs.
 */
static bool notifications_bitmap_create(ffa_id_t vm_id, uint32_t vcpu_count)
{
	VERBOSE("Creating bitmap for VM.\n");
	smc_ret_values ret = ffa_notification_bitmap_create(vm_id, vcpu_count);

	return !is_ffa_call_error(ret);
}

/**
 * Helper to destroy bitmap for NWd VMs.
 */
static bool notifications_bitmap_destroy(ffa_id_t vm_id)
{
	VERBOSE("Destroying bitmap of VM.\n");
	smc_ret_values ret = ffa_notification_bitmap_destroy(vm_id);

	return !is_ffa_call_error(ret);
}

/**
 * Test notifications bitmap create and destroy interfaces.
 */
test_result_t test_ffa_notifications_bitmap_create_destroy(void)
{
	const ffa_id_t vm_id = HYP_ID + 1;

	/* TODO: update to 1.1 */
	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 0);

	if (!notifications_bitmap_create(vm_id, PLATFORM_CORE_COUNT)) {
		return TEST_RESULT_FAIL;
	}

	if (!notifications_bitmap_destroy(vm_id)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/**
 * Test notifications bitmap destroy in a case the bitmap hasn't been created.
 */
test_result_t test_ffa_notifications_destroy_not_created(void)
{
	/* TODO: update to 1.1 */
	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 0);

	smc_ret_values ret = ffa_notification_bitmap_destroy(HYP_ID + 1);

	if (!is_expected_ffa_error(ret, FFA_ERROR_DENIED)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/**
 * Test attempt to create notifications bitmap for NWd VM if it had been
 * already created.
 */
test_result_t test_ffa_notifications_create_after_create(void)
{
	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 0);

	smc_ret_values ret;
	const ffa_id_t vm_id = HYP_ID + 2;

	/* First successfully create a notifications bitmap */
	if (!notifications_bitmap_create(vm_id, 1)) {
		return TEST_RESULT_FAIL;
	}

	/* Attempt to do the same to the same VM. */
	ret = ffa_notification_bitmap_create(vm_id, 1);

	if (!is_expected_ffa_error(ret, FFA_ERROR_DENIED)) {
		return TEST_RESULT_FAIL;
	}

	/* Destroy to not affect other tests */
	if (!notifications_bitmap_destroy(vm_id)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/**
 * Helper function to test FFA_NOTIFICATION_BIND interface.
 * Receives all arguments to use 'cactus_notification_bind_send_cmd', and
 * expected response for the test command.
 *
 * Returns:
 * - 'true' if response was obtained and it was as expected;
 * - 'false' if there was an error with use of FFA_MSG_SEND_DIRECT_REQ, or
 * the obtained response was not as expected.
 */
static bool request_notification_bind(
	ffa_id_t cmd_dest, ffa_id_t receiver, ffa_id_t sender,
	uint64_t notifications, uint32_t flags, uint32_t expected_resp,
	uint32_t error_code)
{
	smc_ret_values ret;

	VERBOSE("TFTF requesting SP to bind notifications!\n");

	ret = cactus_notification_bind_send_cmd(HYP_ID, cmd_dest, receiver,
						sender, notifications, flags);

	return is_expected_cactus_response(ret, expected_resp, error_code);
}

/**
 * Helper function to test FFA_NOTIFICATION_UNBIND interface.
 * Receives all arguments to use 'cactus_notification_unbind_send_cmd', and
 * expected response for the test command.
 *
 * Returns:
 * - 'true' if response was obtained and it was as expected;
 * - 'false' if there was an error with use of FFA_MSG_SEND_DIRECT_REQ, or
 * the obtained response was not as expected.
 */
static bool request_notification_unbind(
	ffa_id_t cmd_dest, ffa_id_t receiver, ffa_id_t sender,
	uint64_t notifications, uint32_t expected_resp, uint32_t error_code)
{
	smc_ret_values ret;

	VERBOSE("TFTF requesting SP to unbind notifications!\n");

	ret = cactus_notification_unbind_send_cmd(HYP_ID, cmd_dest, receiver,
						  sender, notifications);

	return is_expected_cactus_response(ret, expected_resp, error_code);
}

/**
 * Optimistic test to calls from SPs of the bind and unbind interfaces.
 * This test issues a request via direct messaging to the SP, which executes
 * the test and responds with the result to the call.
 */
test_result_t test_ffa_notifications_sp_bind_unbind(void)
{
	CHECK_SPMC_TESTING_SETUP(1, 0, expected_sp_uuids);

	/** First bind... */
	if (!request_notification_bind(SP_ID(1), SP_ID(1), SP_ID(2),
				       g_notifications, 0, CACTUS_SUCCESS, 0)) {
		return TEST_RESULT_FAIL;
	}

	if (!request_notification_bind(SP_ID(1), SP_ID(1), 1,
				       g_notifications, 0, CACTUS_SUCCESS, 0)) {
		return TEST_RESULT_FAIL;
	}

	/** ... then unbind using the same arguments. */
	if (!request_notification_unbind(SP_ID(1), SP_ID(1), SP_ID(2),
					 g_notifications, CACTUS_SUCCESS, 0)) {
		return TEST_RESULT_FAIL;
	}

	if (!request_notification_unbind(SP_ID(1), SP_ID(1), 1,
					 g_notifications, CACTUS_SUCCESS, 0)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/**
 * Test successful attempt of doing bind and unbind of the same set of
 * notifications.
 */
test_result_t test_ffa_notifications_vm_bind_unbind(void)
{
	CHECK_SPMC_TESTING_SETUP(1, 0, expected_sp_uuids);
	const ffa_id_t vm_id = 1;
	smc_ret_values ret;

	if (!notifications_bitmap_create(vm_id, 1)) {
		return TEST_RESULT_FAIL;
	}

	ret = ffa_notification_bind(SP_ID(2), vm_id, 0, g_notifications);

	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32)) {
		return TEST_RESULT_FAIL;
	}

	ret = ffa_notification_unbind(SP_ID(2), vm_id, g_notifications);

	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32)) {
		return TEST_RESULT_FAIL;
	}

	if (!notifications_bitmap_destroy(vm_id)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/**
 * Test expected failure of using a NS FF-A ID for the sender.
 */
test_result_t test_ffa_notifications_vm_bind_vm(void)
{
	CHECK_SPMC_TESTING_SETUP(1, 0, expected_sp_uuids);
	const ffa_id_t vm_id = 1;
	const ffa_id_t sender_id = 2;
	smc_ret_values ret;

	if (!notifications_bitmap_create(vm_id, 1)) {
		return TEST_RESULT_FAIL;
	}

	ret = ffa_notification_bind(sender_id, vm_id, 0, g_notifications);

	if (!is_expected_ffa_error(ret, FFA_ERROR_INVALID_PARAMETER)) {
		return TEST_RESULT_FAIL;
	}

	if (!notifications_bitmap_destroy(vm_id)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/**
 * Test failure of both bind and unbind in case at least one notification is
 * already bound to another FF-A endpoint.
 * Expect error code FFA_ERROR_DENIED.
 */
test_result_t test_ffa_notifications_already_bound(void)
{
	CHECK_SPMC_TESTING_SETUP(1, 0, expected_sp_uuids);

	/** Bind first to test */
	if (!request_notification_bind(SP_ID(1), SP_ID(1), SP_ID(2),
				       g_notifications, 0, CACTUS_SUCCESS, 0)) {
		return TEST_RESULT_FAIL;
	}

	/** Attempt to bind notifications bound in above request. */
	if (!request_notification_bind(SP_ID(1), SP_ID(1), SP_ID(3),
				       g_notifications, 0, CACTUS_ERROR,
				       FFA_ERROR_DENIED)) {
		return TEST_RESULT_FAIL;
	}

	/** Attempt to unbind notifications bound in initial request. */
	if (!request_notification_unbind(SP_ID(1), SP_ID(1), SP_ID(3),
					 g_notifications, CACTUS_ERROR,
					 FFA_ERROR_DENIED)) {
		return TEST_RESULT_FAIL;
	}

	/** Reset the state the SP's notifications state. */
	if (!request_notification_unbind(SP_ID(1), SP_ID(1), SP_ID(2),
					 g_notifications, CACTUS_SUCCESS, 0)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/**
 * Try to bind/unbind notifications spoofing the identity of the receiver.
 * Commands will be sent to SP_ID(1), which will use SP_ID(3) as the receiver.
 * Expect error code FFA_ERROR_INVALID_PARAMETER.
 */
test_result_t test_ffa_notifications_bind_unbind_spoofing(void)
{
	uint64_t notifications = FFA_NOTIFICATION(8);

	CHECK_SPMC_TESTING_SETUP(1, 0, expected_sp_uuids);

	if (!request_notification_bind(SP_ID(1), SP_ID(3), SP_ID(2),
				       notifications, 0, CACTUS_ERROR,
				       FFA_ERROR_INVALID_PARAMETER)) {
		return TEST_RESULT_FAIL;
	}

	if (!request_notification_unbind(SP_ID(1), SP_ID(3), SP_ID(2),
					 notifications, CACTUS_ERROR,
					 FFA_ERROR_INVALID_PARAMETER)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/**
 * Call FFA_NOTIFICATION_BIND with notifications bitmap zeroed.
 * Expecting error code FFA_ERROR_INVALID_PARAMETER.
 */
test_result_t test_ffa_notifications_bind_unbind_zeroed(void)
{
	CHECK_SPMC_TESTING_SETUP(1, 0, expected_sp_uuids);

	if (!request_notification_bind(SP_ID(1), SP_ID(1), SP_ID(2),
				       0, 0, CACTUS_ERROR,
				       FFA_ERROR_INVALID_PARAMETER)) {
		return TEST_RESULT_FAIL;
	}

	if (!request_notification_unbind(SP_ID(1), SP_ID(1), SP_ID(2),
					 0, CACTUS_ERROR,
					 FFA_ERROR_INVALID_PARAMETER)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/**
 * Helper function to test FFA_NOTIFICATION_GET interface.
 * Receives all arguments to use 'cactus_notification_get_send_cmd', and returns
 * the received response. Depending on the testing scenario, this will allow
 * to validate if the returned bitmaps are as expected.
 *
 * Returns:
 * - 'true' if response was obtained.
 * - 'false' if there was an error sending the request.
 */
static bool request_notification_get(
	ffa_id_t cmd_dest, ffa_id_t receiver, uint32_t vcpu_id,
	uint32_t flags, smc_ret_values *response)
{
	VERBOSE("TFTF requesting SP to get notifications!\n");

	*response  = cactus_notification_get_send_cmd(HYP_ID, cmd_dest,
						      receiver, vcpu_id,
						      flags);

	return is_ffa_direct_response(*response);
}

static bool request_notification_set(
	ffa_id_t cmd_dest, ffa_id_t receiver, ffa_id_t sender,
	uint32_t flags, uint64_t notifications, uint32_t exp_resp,
	int32_t exp_error)
{
	smc_ret_values ret;

	VERBOSE("TFTF requesting SP to set notifications\n");

	ret = cactus_notifications_set_send_cmd(HYP_ID, cmd_dest, receiver,
						sender, flags, notifications);

	return is_expected_cactus_response(ret, exp_resp, exp_error);
}

/**
 * Check that SP's response to CACTUS_NOTIFICATION_GET_CMD is as expected.
 */
static bool is_notifications_get_as_expected(
	smc_ret_values *ret, uint64_t exp_from_sp, uint64_t exp_from_vm,
	ffa_id_t receiver)
{
	uint64_t from_sp;
	uint64_t from_vm;
	bool success_ret;

	/**
	 * If receiver ID is SP, this is to evaluate the response to test
	 * command 'CACTUS_NOTIFICATION_GET_CMD'.
	 */
	if (IS_SP_ID(receiver)) {
		success_ret = (cactus_get_response(*ret) == CACTUS_SUCCESS);
		from_sp = cactus_notifications_get_from_sp(*ret);
		from_vm = cactus_notifications_get_from_vm(*ret);
	} else {
		/**
		 * Else, this is to evaluate the return of FF-A call:
		 * ffa_notification_get.
		 */
		success_ret = (ffa_func_id(*ret) == FFA_SUCCESS_SMC32);
		from_sp = ffa_notifications_get_from_sp(*ret);
		from_vm = ffa_notifications_get_from_vm(*ret);
	}

	if (success_ret != true ||
	    exp_from_sp != from_sp ||
	    exp_from_vm != from_vm) {
		VERBOSE("Notifications not as expected:\n"
			"   from sp: %llx\n"
			"   from vm: %llx\n",
			from_sp,
			from_vm);
		return false;
	}

	return true;
}

static bool is_notifications_info_get_as_expected(
	smc_ret_values *ret, uint16_t ids[FFA_NOTIFICATIONS_INFO_GET_MAX_IDS],
	uint32_t lists_sizes[FFA_NOTIFICATIONS_INFO_GET_MAX_IDS],
	uint32_t lists_count,
	bool more_pending)
{
	if (lists_count != ffa_notifications_info_get_lists_count(*ret) ||
	    more_pending != ffa_notifications_info_get_more_pending(*ret)) {
		ERROR("Notification info get not as expected.\n"
		      "    Lists counts: %u; more pending %u\n",
		      ffa_notifications_info_get_lists_count(*ret),
		      ffa_notifications_info_get_more_pending(*ret));
		dump_smc_ret_values(*ret);
		return false;
	}

	for (unsigned int i = 0; i < lists_count; i++) {
		uint32_t cur_size =
				ffa_notifications_info_get_list_size(*ret,
								     i + 1);

		if (lists_sizes[i] != cur_size) {
			ERROR("Expected list size[%u] %u != %u\n", i,
			      lists_sizes[i], cur_size);
			return false;
		}
	}

	/* Compare the IDs list */
	if (memcmp(&ret->ret3, ids,
		   sizeof(ids[0]) * FFA_NOTIFICATIONS_INFO_GET_MAX_IDS) != 0) {
		ERROR("List of IDs not as expected\n");
		return false;
	}

	return true;
}

/**
 * Helper to get the SP to bind notification, and set it for the SP.
 */
static bool request_notification_bind_and_set(ffa_id_t sender,
	ffa_id_t receiver, uint64_t notifications, uint32_t flags)
{
	smc_ret_values ret;

	if (!request_notification_bind(receiver, receiver, sender,
				      notifications,
				      flags & FFA_NOTIFICATIONS_FLAG_PER_VCPU,
				      CACTUS_SUCCESS,
				      0)) {
		return false;
	}

	VERBOSE("VM %x Setting notifications %llx to receiver %x\n",
		sender, notifications, receiver);

	if (IS_SP_ID(sender)) {
		return request_notification_set(sender, receiver, sender, flags,
						notifications, CACTUS_SUCCESS,
						0);
	}

	ret = ffa_notification_set(sender, receiver, flags, notifications);

	return is_expected_ffa_return(ret, FFA_SUCCESS_SMC32);
}

/**
 * Helper to request SP to get the notifications and validate the return.
 */
static bool request_notification_get_and_validate(ffa_id_t sender,
	ffa_id_t receiver, uint32_t vcpu_id, uint64_t from_vm_notif,
	uint64_t from_sp_notif, uint32_t flags)
{
	smc_ret_values ret;

	if (!request_notification_get(
		receiver, receiver, vcpu_id, flags, &ret)) {
		return false;
	}

	return is_notifications_get_as_expected(&ret, from_sp_notif,
						from_vm_notif, receiver);
}

static bool notifications_info_get(
	uint16_t expected_ids[FFA_NOTIFICATIONS_INFO_GET_MAX_IDS],
	uint32_t expected_lists_count,
	uint32_t expected_lists_sizes[FFA_NOTIFICATIONS_INFO_GET_MAX_IDS],
	bool expected_more_pending)
{
	smc_ret_values ret;

	VERBOSE("Getting pending notification's info.\n");

	ret = ffa_notification_info_get();

	return !is_ffa_call_error(ret) &&
		is_notifications_info_get_as_expected(&ret, expected_ids,
						      expected_lists_sizes,
						      expected_lists_count,
						      expected_more_pending);
}

/**
 * Test to validate a VM can signal an SP.
 */
test_result_t test_ffa_notifications_vm_signals_sp(void)
{
	CHECK_SPMC_TESTING_SETUP(1, 0, expected_sp_uuids);
	const ffa_id_t sender = 1;
	const ffa_id_t receiver = SP_ID(1);
	uint64_t notification = FFA_NOTIFICATION(1) | FFA_NOTIFICATION(60);
	const uint32_t flags_get = FFA_NOTIFICATIONS_FLAG_BITMAP_VM;
	test_result_t result = TEST_RESULT_SUCCESS;

	/** Variables to validate calls to FFA_NOTIFICATION_INFO_GET. */
	uint16_t ids[FFA_NOTIFICATIONS_INFO_GET_MAX_IDS] = {0};
	uint32_t lists_count;
	uint32_t lists_sizes[FFA_NOTIFICATIONS_INFO_GET_MAX_IDS] = {0};
	const bool more_notif_pending = false;

	if (!request_notification_bind_and_set(sender, receiver,
					       notification, 0)) {
		return TEST_RESULT_FAIL;
	}

	/**
	 * Simple list of IDs expected on return from FFA_NOTIFICATION_INFO_GET.
	 */
	ids[0] = receiver;
	lists_count = 1;

	if (!notifications_info_get(ids, lists_count, lists_sizes,
				    more_notif_pending)) {
		result = TEST_RESULT_FAIL;
	}

	if (!request_notification_get_and_validate(sender, receiver, 0,
						  notification, 0, flags_get)) {
		result = TEST_RESULT_FAIL;
	}

	if (!request_notification_unbind(receiver, receiver, sender,
					 notification, CACTUS_SUCCESS, 0)) {
		return TEST_RESULT_FAIL;
	}

	return result;
}

/**
 * Test to validate an SP can signal an SP.
 */
test_result_t test_ffa_notifications_sp_signals_sp(void)
{
	CHECK_SPMC_TESTING_SETUP(1, 0, expected_sp_uuids);
	const ffa_id_t sender = SP_ID(1);
	const ffa_id_t receiver = SP_ID(2);
	uint32_t get_flags = FFA_NOTIFICATIONS_FLAG_BITMAP_SP;
	smc_ret_values ret;

	/** Variables to validate calls to FFA_NOTIFICATION_INFO_GET. */
	uint16_t ids[FFA_NOTIFICATIONS_INFO_GET_MAX_IDS] = {0};
	uint32_t lists_count;
	uint32_t lists_sizes[FFA_NOTIFICATIONS_INFO_GET_MAX_IDS] = {0};

	/** Request receiver to bind a set of notifications to the sender */
	if (!request_notification_bind(receiver, receiver, sender,
				       g_notifications, 0, CACTUS_SUCCESS, 0)) {
		return TEST_RESULT_FAIL;
	}

	if (!request_notification_set(sender, receiver, sender, 0,
				      g_notifications, CACTUS_SUCCESS, 0)) {
		return TEST_RESULT_FAIL;
	}

	/**
	 * FFA_NOTIFICATION_INFO_GET return list should be simple, containing
	 * only the receiver's ID.
	 */
	ids[0] = receiver;
	lists_count = 1;

	if (!notifications_info_get(ids, lists_count, lists_sizes, false)) {
		return TEST_RESULT_FAIL;
	}

	if (!request_notification_get(receiver, receiver, 0, get_flags, &ret)) {
		return TEST_RESULT_FAIL;
	}

	if (!is_notifications_get_as_expected(&ret, g_notifications, 0,
					      receiver)) {
		return TEST_RESULT_FAIL;
	}

	if (!request_notification_unbind(receiver, receiver, sender,
					 g_notifications, CACTUS_SUCCESS, 0)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/**
 * Test to validate an SP can signal a VM.
 */
test_result_t test_ffa_notifications_sp_signals_vm(void)
{
	CHECK_SPMC_TESTING_SETUP(1, 0, expected_sp_uuids);
	const ffa_id_t sender = SP_ID(1);
	const ffa_id_t receiver = 1;
	uint32_t get_flags = FFA_NOTIFICATIONS_FLAG_BITMAP_SP;
	smc_ret_values ret;

	/** Variables to validate calls to FFA_NOTIFICATION_INFO_GET. */
	uint16_t ids[FFA_NOTIFICATIONS_INFO_GET_MAX_IDS] = {0};
	uint32_t lists_count;
	uint32_t lists_sizes[FFA_NOTIFICATIONS_INFO_GET_MAX_IDS] = {0};

	/** Ask SPMC to allocate notifications bitmap */
	if (!notifications_bitmap_create(receiver, 1)) {
		return TEST_RESULT_FAIL;
	}

	/** Allow sp to send notifications */
	ret = ffa_notification_bind(sender, receiver, 0, g_notifications);

	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32)) {
		return TEST_RESULT_FAIL;
	}

	/* Request SP to set notifications */
	if (!request_notification_set(sender, receiver, sender, 0,
				      g_notifications, CACTUS_SUCCESS, 0)) {
		return TEST_RESULT_FAIL;
	}

	/**
	 * FFA_NOTIFICATION_INFO_GET return list should be simple, containing
	 * only the receiver's ID.
	 */
	ids[0] = receiver;
	lists_count = 1;

	if (!notifications_info_get(ids, lists_count, lists_sizes, false)) {
		return TEST_RESULT_FAIL;
	}

	ret = ffa_notification_get(receiver, 0, get_flags);

	if (!is_notifications_get_as_expected(&ret, g_notifications, 0,
					      receiver)) {
		return TEST_RESULT_FAIL;
	}

	ret = ffa_notification_unbind(sender, receiver, g_notifications);

	if (!is_expected_ffa_return(ret, FFA_SUCCESS_SMC32)) {
		return TEST_RESULT_FAIL;
	}

	if (!notifications_bitmap_destroy(receiver)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/**
 * Test to validate it is not possible to unbind a pending notification.
 */
test_result_t test_ffa_notifications_unbind_pending(void)
{
	CHECK_SPMC_TESTING_SETUP(1, 0, expected_sp_uuids);
	const ffa_id_t receiver = SP_ID(1);
	const ffa_id_t sender = 1;
	const uint64_t notifications = FFA_NOTIFICATION(30) |
				       FFA_NOTIFICATION(35);
	uint32_t get_flags = FFA_NOTIFICATIONS_FLAG_BITMAP_VM;
	smc_ret_values ret;

	if (!request_notification_bind(receiver, receiver, sender,
				       notifications, 0, CACTUS_SUCCESS, 0)) {
		return TEST_RESULT_FAIL;
	}

	/* Set one of the bound notifications */
	ret = ffa_notification_set(sender, receiver, 0, FFA_NOTIFICATION(30));

	if (is_ffa_call_error(ret)) {
		return TEST_RESULT_FAIL;
	}

	/* Attempt to unbind the pending notification */
	if (!request_notification_unbind(receiver, receiver, sender,
					 FFA_NOTIFICATION(30),
					 CACTUS_ERROR, FFA_ERROR_DENIED)) {
		return TEST_RESULT_FAIL;
	}

	/**
	 * Request receiver partition to get pending notifications from VMs.
	 * Only notification 30 is expected.
	 */
	if (!request_notification_get_and_validate(sender, receiver, 0,
						   FFA_NOTIFICATION(30), 0,
						   get_flags)) {
		return TEST_RESULT_FAIL;
	}

	/* Unbind all notifications, to not interfere with other tests */
	if (!request_notification_unbind(receiver, receiver, sender,
					notifications, CACTUS_SUCCESS, 0)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

/**
 * Defining variables to test the per VCPU notifications.
 * The conceived test follows the same logic, despite the sender receiver type
 * of endpoint (VM or secure partition).
 * Using global variables because these need to accessed in the cpu on handler
 * function 'request_notification_get_per_vcpu_on_handler'.
 * In each specific test function, change 'per_vcpu_receiver' and
 * 'per_vcpu_sender' have the logic work for:
 * - NWd to SP;
 * - SP to NWd;
 * - SP to SP.
 */
static ffa_id_t per_vcpu_receiver;
static ffa_id_t per_vcpu_sender;
uint32_t per_vcpu_flags_get;
static event_t per_vcpu_finished[PLATFORM_CORE_COUNT];

static test_result_t request_notification_get_per_vcpu_on_handler(void)
{
	unsigned int mpid = read_mpidr_el1() & MPID_MASK;
	unsigned int core_pos = platform_get_core_pos(mpid);
	smc_ret_values ret;
	test_result_t result = TEST_RESULT_FAIL;

	uint64_t exp_from_vm = 0;
	uint64_t exp_from_sp = 0;

	if (IS_SP_ID(per_vcpu_sender)) {
		exp_from_sp = FFA_NOTIFICATION(core_pos);
	} else {
		exp_from_vm = FFA_NOTIFICATION(core_pos);
	}

	VERBOSE("Request get per VCPU notification to %x, core: %u.\n",
		 per_vcpu_receiver, core_pos);

	ret = ffa_run(per_vcpu_receiver, core_pos);

	if (ffa_func_id(ret) != FFA_MSG_WAIT) {
		ERROR("Failed to run SP%x on core %u\n",
		      per_vcpu_receiver, core_pos);
		goto out;
	}

	/* Request to get notifications sent to the respective VCPU. */
	if (!request_notification_get_and_validate(
		per_vcpu_sender, per_vcpu_receiver, core_pos,
		exp_from_vm, exp_from_sp, per_vcpu_flags_get)) {
		goto out;
	}

	result = TEST_RESULT_SUCCESS;

out:
	/* Tell the lead CPU that the calling CPU has completed the test. */
	tftf_send_event(&per_vcpu_finished[core_pos]);

	return result;
}

/**
 * Base function to test signaling of per VCPU notifications.
 * Test whole flow from binding, to getting notifications' info, and getting
 * pending notifications, between two FF-A endpoints.
 * Each VCPU will receive a notification whose ID is the same as the core
 * position.
 */
static test_result_t base_test_per_vcpu_notifications(void)
{
	CHECK_SPMC_TESTING_SETUP(1, 0, expected_sp_uuids);

	/**
	 * Manually set variables to validate what should be the return of to
	 * FFA_NOTIFICATION_INFO_GET.
	 */
	uint16_t exp_ids[FFA_NOTIFICATIONS_INFO_GET_MAX_IDS] = {
		per_vcpu_receiver, 0, 1, 2,
		per_vcpu_receiver, 3, 4, 5,
		per_vcpu_receiver, 6, 7, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
	};
	uint32_t exp_lists_count = 3;
	uint32_t exp_lists_sizes[FFA_NOTIFICATIONS_INFO_GET_MAX_IDS] = {
		3, 3, 2, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	const bool exp_more_notif_pending = false;
	test_result_t result = TEST_RESULT_SUCCESS;
	uint64_t notifications_to_unbind = 0;

	per_vcpu_flags_get = IS_SP_ID(per_vcpu_sender)
				? FFA_NOTIFICATIONS_FLAG_BITMAP_SP
				: FFA_NOTIFICATIONS_FLAG_BITMAP_VM;

	/*
	 * Prepare notifications bitmap to request Cactus to bind them as
	 * per VCPU.
	 */
	for (unsigned int i = 0; i < PLATFORM_CORE_COUNT; i++) {
		notifications_to_unbind |= FFA_NOTIFICATION(i);

		uint32_t flags = FFA_NOTIFICATIONS_FLAG_PER_VCPU |
				 FFA_NOTIFICATIONS_FLAGS_VCPU_ID((uint16_t)i);

		if (!request_notification_bind_and_set(per_vcpu_sender,
						       per_vcpu_receiver,
						       FFA_NOTIFICATION(i),
						       flags)) {
			return TEST_RESULT_FAIL;
		}
	}

	/* Call FFA_NOTIFICATION_INFO_GET and validate return. */
	if (!notifications_info_get(exp_ids, exp_lists_count,
				    exp_lists_sizes, exp_more_notif_pending)) {
		VERBOSE("Info Get Failed....\n");
		result = TEST_RESULT_FAIL;
		goto out;
	}

	/*
	 * Request SP to get notifications in core 0, as this is not iterated
	 * at the CPU ON handler.
	 */
	if (!request_notification_get_and_validate(
		per_vcpu_sender, per_vcpu_receiver, 0,
		!IS_SP_ID(per_vcpu_sender) ? FFA_NOTIFICATION(0) : 0,
		IS_SP_ID(per_vcpu_sender) ? FFA_NOTIFICATION(0) : 0,
		per_vcpu_flags_get)) {
		result = TEST_RESULT_FAIL;
	}

	/*
	 * Bring up all the cores, and request the receiver to get notifications
	 * in each one of them.
	 */
	if (spm_run_multi_core_test(
		(uintptr_t)request_notification_get_per_vcpu_on_handler,
		per_vcpu_finished) != TEST_RESULT_SUCCESS) {
		result = TEST_RESULT_FAIL;
	}

out:
	/* As a clean-up, unbind notifications. */
	if (!request_notification_unbind(per_vcpu_receiver, per_vcpu_receiver,
					 per_vcpu_sender,
					 notifications_to_unbind,
					 CACTUS_SUCCESS, 0)) {
		result = TEST_RESULT_FAIL;
	}

	return result;
}

/**
 * Test to validate a VM can signal a per VCPU notification to an SP.
 */
test_result_t test_ffa_notifications_vm_signals_sp_per_vcpu(void)
{
	per_vcpu_receiver = SP_ID(1);
	per_vcpu_sender = 0;

	return base_test_per_vcpu_notifications();
}

/**
 * Test to validate an SP can signal a per VCPU notification to an SP.
 */
test_result_t test_ffa_notifications_sp_signals_sp_per_vcpu(void)
{
	per_vcpu_receiver = SP_ID(2);
	per_vcpu_sender = SP_ID(1);

	return base_test_per_vcpu_notifications();
}

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

/**
 * Helper to create bitmap for NWd VMs.
 */
static bool notifications_bitmap_create(ffa_vm_id_t vm_id, uint32_t vcpu_count)
{
	VERBOSE("Creating bitmap for VM.\n");
	smc_ret_values ret = ffa_notification_bitmap_create(vm_id, vcpu_count);

	return !is_ffa_call_error(ret);
}

/**
 * Helper to destroy bitmap for NWd VMs.
 */
static bool notifications_bitmap_destroy(ffa_vm_id_t vm_id)
{
	VERBOSE("Destroying bitmap of VM.\n");
	smc_ret_values ret = ffa_notification_bitmap_destroy(vm_id);

	return !is_ffa_call_error(ret);
}

test_result_t test_ffa_notifications_bitmap_create_destroy(void)
{
	const ffa_vm_id_t vm_id = HYP_ID + 1;

	/* TODO: update to 1.1 */
	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 0);

	/*
	 * Number of VCPUs is either 0 or 8 for FVP platforms.
	 * TODO: MP patches have some platform specific files to be useful here
	 * for the number of CPUs of each endpoint.
	 */
	if (!notifications_bitmap_create(vm_id, 8)) {
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

	if (!is_expected_ffa_error(ret, FFA_ERROR_DENIED)){
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
	const ffa_vm_id_t vm_id = HYP_ID + 2;

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

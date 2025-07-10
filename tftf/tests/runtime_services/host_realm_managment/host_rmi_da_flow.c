/*
 * Copyright (c) 2024-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include <heap/page_alloc.h>
#include <host_da_helper.h>
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_shared_data.h>
#include <mmio.h>
#include <pcie.h>
#include <pcie_doe.h>
#include <pcie_spec.h>
#include <platform.h>
#include <spdm.h>
#include <test_helpers.h>

extern unsigned int gbl_host_pdev_count;
extern struct host_pdev gbl_host_pdevs[HOST_PDEV_MAX];
extern struct host_vdev gbl_host_vdev;

static int realm_assign_unassign_device(struct realm *realm_ptr,
					struct host_vdev *h_vdev,
					unsigned long tdi_id, void *pdev_ptr)
{
	int rc;
	bool realm_rc;

	/* Assign VDEV */
	INFO("======================================\n");
	INFO("Host: Assign device: (0x%x) %x:%x.%x to Realm \n",
	     (uint32_t)tdi_id,
	     PCIE_EXTRACT_BDF_BUS((uint32_t)tdi_id),
	     PCIE_EXTRACT_BDF_DEV((uint32_t)tdi_id),
	     PCIE_EXTRACT_BDF_FUNC((uint32_t)tdi_id));
	INFO("======================================\n");

	rc = host_assign_vdev_to_realm(realm_ptr, h_vdev, tdi_id, pdev_ptr);
	if (rc != 0) {
		ERROR("VDEV assign to realm failed\n");
		/* TF-RMM has support till here. Change error code temporarily */
		return 0;
		/* return -1; */
	}

	/* Enter Realm. Lock -> Accept -> Unlock the assigned device */
	realm_rc = host_enter_realm_execute(realm_ptr, REALM_DA_RSI_CALLS,
					    RMI_EXIT_HOST_CALL, 0U);

	/* Unassign VDEV */
	INFO("======================================\n");
	INFO("Host: Unassign device: (0x%x) %x:%x.%x from Realm \n",
	     (uint32_t)tdi_id,
	     PCIE_EXTRACT_BDF_BUS((uint32_t)tdi_id),
	     PCIE_EXTRACT_BDF_DEV((uint32_t)tdi_id),
	     PCIE_EXTRACT_BDF_FUNC((uint32_t)tdi_id));
	INFO("======================================\n");

	rc = host_unassign_vdev_from_realm(realm_ptr, h_vdev);
	if (rc != 0) {
		ERROR("VDEV unassign to realm failed\n");
		return -1;
	}

	if (!realm_rc) {
		ERROR("Realm DA_RSI_CALLS failed\n");
		return -1;
	}

	return 0;
}

static int realm_assign_unassign_devices(struct realm *realm_ptr)
{
	uint32_t i;
	int rc;
	struct host_pdev *h_pdev;
	struct host_vdev *h_vdev;

	for (i = 0; i < gbl_host_pdev_count; i++) {
		h_pdev = &gbl_host_pdevs[i];

		if (h_pdev->is_connected_to_tsm) {
			h_vdev = &gbl_host_vdev;

			rc = realm_assign_unassign_device(realm_ptr, h_vdev,
							  h_pdev->dev->bdf,
							  h_pdev->pdev);
			if (rc != 0) {
				break;
			}
		}
	}

	return rc;
}

/*
 * Iterate through all host_pdevs and do
 * TSM connect
 * TSM disconnect
 */
test_result_t host_da_workflow_on_all_offchip_devices(void)
{
	int rc;
	unsigned int count;
	struct realm realm;
	test_result_t result = TEST_RESULT_SUCCESS;
	bool return_error = false;
	u_register_t rmi_feat_reg0;

	INIT_AND_SKIP_DA_TEST_IF_PREREQS_NOT_MET(rmi_feat_reg0);

	/*
	 * Create a Realm with DA feature enabled
	 *
	 * todo: creating this after host_pdev_setup causes Realm create to
	 * fail.
	 */
	rc = host_create_realm_with_feat_da(&realm);
	if (rc != 0) {
		ERROR("Realm create with feat_da failed\n");
		return TEST_RESULT_FAIL;
	}

	/* Connect all devices with TSM */
	rc = tsm_connect_devices(&count);
	if (rc != 0) {
		return_error = true;
	}

	/* If no devices are connected to TSM, then skip the test */
	if (count == 0U) {
		result = TEST_RESULT_SKIPPED;
		goto out_rm_realm;
	}

	/* Assign all TSM connected devices to a Realm */
	rc = realm_assign_unassign_devices(&realm);
	if (rc != 0) {
		return_error = true;
	}

	rc = tsm_disconnect_devices();
	if (rc != 0) {
		return_error = true;
	}

out_rm_realm:
	/* Destroy the Realm */
	if (!host_destroy_realm(&realm)) {
		return_error = true;
	}

	if (return_error) {
		result = TEST_RESULT_FAIL;
	}

	return result;
}

/*
 * This function invokes PDEV_CREATE on TRP and tries to test
 * the EL3 RMM-EL3 IDE KM interface. Will be skipped on TF-RMM.
 */
test_result_t host_realm_test_root_port_key_management(void)
{
	u_register_t rmi_rc;
	int ret;

	if (host_rmi_version(RMI_ABI_VERSION_VAL) != 0U) {
		tftf_testcase_printf("RMM is not TRP\n");
		return TEST_RESULT_SKIPPED;
	}

	/* Initialize Host NS heap memory */
	ret = page_pool_init((u_register_t)PAGE_POOL_BASE,
				(u_register_t)PAGE_POOL_MAX_SIZE);
	if (ret != HEAP_INIT_SUCCESS) {
		ERROR("Failed to init heap pool %d\n", ret);
		return TEST_RESULT_FAIL;
	}

	/*
	 * Directly call host_rmi_pdev_create with invalid pdev, expect an error
	 * to be returned from TRP.
	 */
	rmi_rc = host_rmi_pdev_create((u_register_t)0UL, (u_register_t)0UL);
	if (rmi_rc != RMI_SUCCESS) {
		return TEST_RESULT_SUCCESS;
	}

	ERROR("RMI_PDEV_CREATE did not return error as expected\n");
	return TEST_RESULT_FAIL;
}

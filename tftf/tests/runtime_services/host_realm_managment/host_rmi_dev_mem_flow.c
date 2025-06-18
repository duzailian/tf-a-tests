/*
 * Copyright (c) 2024-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include <common_def.h>
#include <heap/page_alloc.h>
#include <host_crypto_utils.h>
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

/*
 * Base Platform RevC:
 * 0: PCIE_MEM_1_BASE 0x50000000
 * 1: PCIE_MEM_2_BASE 0x4000000000
 */
#define PCIE_MEM_REGION			1U

#define DEV_MEM_TEST1_SIZE		SZ_2M
#define DEV_MEM_NUM1_GRANULES		(DEV_MEM_TEST1_SIZE / GRANULE_SIZE)

#define DEV_MEM_TEST2_SIZE		32 * SZ_4K
#define DEV_MEM_NUM2_GRANULES		(DEV_MEM_TEST2_SIZE / GRANULE_SIZE)

extern struct host_vdev gbl_host_vdev;

/*
 * This invokes various RMI calls related to PDEV, VDEV management that does
 * PDEV create/communicate/set_key/abort/stop/destroy and assigns the device
 * to a Realm using RMI VDEV ABIs
 *
 * 1.  Create a Realm with DA feature enabled.
 *     Find a known PCIe endpoint and connect with TSM to get_cert and establish
 *     secure session.
 * 2.  Find and connect device with TSM.
 * 3.  Assign VDEV (the PCIe endpoint) to the Realm.
 * 4.  Delegate 2MB of device granules.
 * 5.  Map 2MB of device granules.
 * 6.  Read L3 RTT entries, walk should terminate at L3, and HIPAS=RMI_ASSIGNED_DEV,
 *     RIPAS=RMI_EMPTY.
 * 7.  Call RTT Fold.
 * 8.  Read L2 RTT entry, walk should terminate at L2, and HIPAS=RMI_ASSIGNED_DEV,
 *     RIPAS=RMI_EMPTY.
 * 9.  Call Realm to do DA related RSI calls. Realm should exit with
 *     RMI_EXIT_DEV_MEM_MAP.
 * 10. Enter device memory validation loop.
 *     Validate device memory.
 *     Realm should exit with RMI_EXIT_HOST_CALL or RMI_EXIT_RIPAS_CHANGE.
 * 11. Read L2 RTT entry, walk should terminate at L2, and HIPAS=RMI_ASSIGNED_DEV,
 *     RIPAS=RMI_DEV.
 * 12. Complete Realm's requests to change RIPAS. Realm should exit with
 *     RMI_EXIT_HOST_CALL.
 * 13. Read RTT entry, walk should terminate at L2, and HIPAS=RMI_ASSIGNED_DEV,
 *     RIPAS=exit.ripas_value.
 * 14. Unassign VDEV (the PCIe endpoint) from the Realm.
 * 15. Unfold. Create L3 RTT entries.
 * 16. Read L3 RTT entries, walk should terminate at L3, and HIPAS=RMI_ASSIGNED_DEV,
 *     RIPAS=exit.ripas_value.
 * 17. Unmap 2MB of device memory.
 * 18. Read L3 RTT entries, walk should terminate at L3, and HIPAS=RMI_UNASSIGNED,
 *     RIPAS=exit.ripas_value.
 * 19. Undelegate 2MB of device granules.
 * 20. Reclaim PDEV (the PCIe TDI) from TSM.
 * 21. Destroy Realm.
 */
test_result_t host_realm_dev_mem_block(void)
{
	u_register_t rmi_feat_reg0;
	u_register_t res, exit_reason, out_top;
	u_register_t map_addr[DEV_MEM_NUM1_GRANULES];
	uintptr_t base_pa;
	size_t dev_size;
	unsigned int host_call_result = TEST_RESULT_FAIL;
	struct host_pdev *h_pdev;
	struct host_vdev *h_vdev;
	struct realm realm;
	struct rmi_rec_run *run;
	struct rtt_entry rtt;
	unsigned int num_dlg, num_map, i;
	int ret;
	bool realm_rc;
	test_result_t test_res = TEST_RESULT_SUCCESS;
	u_register_t ripas = RMI_EMPTY;

	INIT_AND_SKIP_DA_TEST_IF_PREREQS_NOT_MET(rmi_feat_reg0);

	/* Initialize Host NS heap memory */
	ret = page_pool_init((u_register_t)PAGE_POOL_BASE,
				(u_register_t)PAGE_POOL_MAX_SIZE);
	if (ret != HEAP_INIT_SUCCESS) {
		ERROR("Failed to init heap pool %d\n", ret);
		return TEST_RESULT_FAIL;
	}

	/*
	 * Todo: set device memory base address and size in attestation
	 * report
	 */

	/* Retrieve platform PCIe memory region */
	ret = plat_get_dev_region((uint64_t *)&base_pa, &dev_size,
				  DEV_MEM_NON_COHERENT, PCIE_MEM_REGION);

	if ((ret != 0) || (dev_size < DEV_MEM_TEST1_SIZE)) {
		return TEST_RESULT_SKIPPED;
	}

	INFO("Testing PCIe memory region %u 0x%lx-0x%lx\n",
		PCIE_MEM_REGION, base_pa, base_pa + DEV_MEM_TEST1_SIZE - 1UL);

	/*
	 * 1. Create a Realm with DA feature enabled
	 *
	 * todo: creating this after host_pdev_setup cases Realm create to
	 * fail.
	 */
	ret = host_create_realm_with_feat_da(&realm);
	if (ret != 0) {
		ERROR("Realm create with feat_da failed\n");
		return TEST_RESULT_FAIL;
	}

	INFO("Realm created with feat_da enabled\n");

	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 0U,
					HOST_ARG1_INDEX, base_pa);
	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 0U,
					HOST_ARG2_INDEX, DEV_MEM_TEST1_SIZE);

	h_vdev = &gbl_host_vdev;

	/* 2. Find and connect device with TSM */
	h_pdev = get_host_pdev_by_type(DEV_TYPE_INDEPENDENTLY_ATTESTED);
	if (h_pdev == NULL) {
		/* If no device is connected to TSM, then skip the test */
		test_res = TEST_RESULT_SKIPPED;
		goto destroy_realm;
	}

	ret = tsm_connect_device(h_pdev);
	if (ret != 0) {
		ERROR("TSM connect failed for device 0x%x\n", h_pdev->dev->bdf);
		test_res = TEST_RESULT_FAIL;
		goto destroy_realm;
	}

	h_pdev->ncoh_num_addr_range = NCOH_ADDR_RANGE_NUM;
	h_pdev->ncoh_addr_range[0].base = base_pa;
	h_pdev->ncoh_addr_range[0].top = base_pa + DEV_MEM_TEST1_SIZE;

	/*
	 * 3. Assign VDEV (the PCIe endpoint) to the Realm
	 */
	INFO("======================================\n");
	INFO("Host: Assign device: (0x%x) %x:%x.%x to Realm \n",
	     (uint32_t)h_pdev->dev->bdf,
	     PCIE_EXTRACT_BDF_BUS((uint32_t)h_pdev->dev->bdf),
	     PCIE_EXTRACT_BDF_DEV((uint32_t)h_pdev->dev->bdf),
	     PCIE_EXTRACT_BDF_FUNC((uint32_t)h_pdev->dev->bdf));
	INFO("======================================\n");

	ret = host_assign_vdev_to_realm(&realm, h_vdev, h_pdev->dev->bdf, h_pdev->pdev);
	if (ret != 0) {
		ERROR("VDEV assign to realm failed\n");
		test_res = TEST_RESULT_FAIL;
		goto destroy_realm;
	}

	/* 4. Delegate device granules */
	for (num_dlg = 0U; num_dlg < DEV_MEM_NUM1_GRANULES; num_dlg++) {
		res = host_rmi_granule_delegate(base_pa + num_dlg * GRANULE_SIZE);
		if (res != RMI_SUCCESS) {
			ERROR("%s() for 0x%lx failed, 0x%lx\n",
				"host_rmi_granule_delegate",
				(base_pa + num_dlg * GRANULE_SIZE), res);
			test_res = TEST_RESULT_FAIL;
			goto undelegate_granules;
		}
	}

	/* 5. Map device memory */
	for (num_map = 0U; num_map < DEV_MEM_NUM1_GRANULES; num_map++) {
		res = host_dev_mem_map(&realm, base_pa + num_map * GRANULE_SIZE,
					RTT_MAX_LEVEL, &map_addr[num_map]);
		if (res != REALM_SUCCESS) {
			ERROR("%s() for 0x%lx failed, 0x%lx\n",
				"host_dev_mem_map",
				base_pa + num_map * GRANULE_SIZE, res);
			test_res = TEST_RESULT_FAIL;
			goto unmap_memory;
		}
	}

	/*
	 * 6. Read L3 RTT entries, walk should terminate at L3.
	 *    Expect HIPAS=RMI_ASSIGNED_DEV, RIPAS=RMI_EMPTY.
	 */
	for (i = 0U; i < DEV_MEM_NUM1_GRANULES; i++) {
		res = host_rmi_rtt_readentry(realm.rd, map_addr[i], RTT_MAX_LEVEL, &rtt);
		if ((res != RMI_SUCCESS) || (rtt.walk_level != RTT_MAX_LEVEL) ||
			(rtt.state != RMI_ASSIGNED_DEV) || (rtt.ripas != RMI_EMPTY) ||
			(rtt.out_addr != map_addr[i])) {
			ERROR("host_rmi_rtt_readentry() failed, 0x%lx rtt.state=%lu"
				" rtt.ripas=%lu rtt.walk_level=%ld rtt.out_addr=0x%llx\n",
				res, rtt.ripas, rtt.state, rtt.walk_level, rtt.out_addr);
			test_res = TEST_RESULT_FAIL;
			goto unmap_memory;
		}
	}

	/* 7. RTT Fold */
	res = host_realm_fold_rtt(realm.rd, map_addr[0], RTT_MAX_LEVEL);
	if (res != RMI_SUCCESS) {
		ERROR("%s() for 0x%lx failed, 0x%lx\n",
			"host_rmi_rtt_fold", map_addr[0], res);
		test_res = TEST_RESULT_FAIL;
		goto unmap_memory;
	}

	/*
	 * 8. Read L2 RTT entry, walk should terminate at L2.
	 *    Expect HIPAS=RMI_ASSIGNED_DEV, RIPAS=RMI_EMPTY.
	 */
	res = host_rmi_rtt_readentry(realm.rd, map_addr[0], RTT_MIN_DEV_BLOCK_LEVEL, &rtt);
	if ((res != RMI_SUCCESS) || (rtt.walk_level != RTT_MIN_DEV_BLOCK_LEVEL) ||
		(rtt.state != RMI_ASSIGNED_DEV) || (rtt.ripas != RMI_EMPTY) ||
		(rtt.out_addr != map_addr[0])) {
		ERROR("host_rmi_rtt_readentry() failed, 0x%lx rtt.state=%lu"
			" rtt.ripas=%lu rtt.walk_level=%ld rtt.out_addr=0x%llx\n",
			res, rtt.ripas, rtt.state, rtt.walk_level, rtt.out_addr);
		test_res = TEST_RESULT_FAIL;
		goto unmap_granules;
	}

	/*
	 * 9. Call Realm to do DA related RSI calls
	 */
	realm_rc = host_enter_realm_execute(&realm, REALM_DEV_MEM_ACCESS,
					    RMI_EXIT_DEV_MEM_MAP, 0U);

	run = (struct rmi_rec_run *)realm.run[0];
	exit_reason = run->exit.exit_reason;

	/* Realm should exit with RMI_EXIT_DEV_MEM_MAP */
	if (!realm_rc || (exit_reason != RMI_EXIT_DEV_MEM_MAP)) {
		ERROR("Rec enter failed, exit_reason %lu\n", exit_reason);
		test_res = TEST_RESULT_FAIL;
		goto unmap_granules;
	}

	/*
	 * 10. Validate device memory
	 */
	INFO("Enter device memory @0x%lx [0x%lx-0x%lx] validation loop\n",
		run->exit.dev_mem_pa, run->exit.dev_mem_base,
		run->exit.dev_mem_top - 1UL);

	out_top = 0UL;

	/* Complete Realm's requests to validate mappings to device memory */
	while (out_top != run->exit.dev_mem_top) {
		res = host_rmi_rtt_dev_mem_validate(realm.rd, realm.rec[0],
						run->exit.dev_mem_base,
						run->exit.dev_mem_top,
						&out_top);
		if (res != RMI_SUCCESS) {
			ERROR("%s() failed, 0x%lx out_top=0x%lx\n",
				"host_rmi_rtt_dev_mem_validate", res, out_top);
				run->entry.flags = REC_ENTRY_FLAG_DEV_MEM_RESPONSE;
				test_res = TEST_RESULT_FAIL;
		}

		res = host_realm_rec_enter(&realm, &exit_reason, &host_call_result, 0U);
		if ((res != RMI_SUCCESS) || (exit_reason != RMI_EXIT_DEV_MEM_MAP)) {
			break;
		}
	}

	/* Exit with RMI_EXIT_HOST_CALL or RMI_EXIT_RIPAS_CHANGE */
	if ((res != RMI_SUCCESS) || ((exit_reason == RMI_EXIT_HOST_CALL) &&
	    (host_call_result != TEST_RESULT_SUCCESS))) {
		ERROR("%s() failed, 0x%lx exit_reason=%lu host_call_result=%u\n",
			"host_realm_rec_enter", res, exit_reason, host_call_result);
		test_res = TEST_RESULT_FAIL;
	}

	/*
	 * 11. Read L2 RTT entry, walk should terminate at L2.
	 *     Expect HIPAS=RMI_ASSIGNED_DEV, RIPAS=RMI_DEV.
	 */
	ret = host_rmi_rtt_readentry(realm.rd, map_addr[0], RTT_MIN_DEV_BLOCK_LEVEL, &rtt);
	if ((res != RMI_SUCCESS) || (rtt.walk_level != RTT_MIN_DEV_BLOCK_LEVEL) ||
		(rtt.state != RMI_ASSIGNED_DEV) || (rtt.ripas != RMI_DEV) ||
		(rtt.out_addr != map_addr[0])) {
		ERROR("host_rmi_rtt_readentry() failed, 0x%lx rtt.state=%lu"
			" rtt.ripas=%lu rtt.walk_level=%ld rtt.out_addr=0x%llx\n",
			res, rtt.ripas, rtt.state, rtt.walk_level, rtt.out_addr);
		test_res = TEST_RESULT_FAIL;
	}

	INFO("Exit device memory validation loop\n");

	out_top = 0UL;

	INFO("Enter change RIPAS loop\n");

	/* 12. Complete Realm's requests to change RIPAS */
	while (exit_reason == RMI_EXIT_RIPAS_CHANGE) {
		while (out_top != run->exit.ripas_top) {
			INFO("set RIPAS=%u [0x%lx-0x%lx]\n",
				run->exit.ripas_value, run->exit.ripas_base,
				run->exit.ripas_top - 1UL);

			/* This call fails for RSI_DEV -> RSI_RAM */
			res = host_rmi_rtt_set_ripas(realm.rd,
					     realm.rec[0],
					     run->exit.ripas_base,
					     run->exit.ripas_top,
					     &out_top);
			if (res != RMI_SUCCESS) {
				INFO("%s() failed, 0x%lx ripas=%u\n", "host_rmi_rtt_set_ripas",
					res, run->exit.ripas_value);
				run->entry.flags = REC_ENTRY_FLAG_RIPAS_RESPONSE_REJECT;
			}

			res = host_realm_rec_enter(&realm, &exit_reason, &host_call_result, 0U);
			if ((res != RMI_SUCCESS) || (exit_reason != RMI_EXIT_RIPAS_CHANGE)) {
				break;
			}
		}
	}

	ripas = (u_register_t)run->exit.ripas_value;

	INFO("Exit change RIPAS loop, RIPAS=%lu exit_reason=%lu\n",
						ripas, exit_reason);

	/* Exit with RMI_EXIT_HOST_CALL */
	if ((res != RMI_SUCCESS) || (exit_reason != RMI_EXIT_HOST_CALL) ||
	    (host_call_result != TEST_RESULT_SUCCESS)) {
		ERROR("%s() failed, 0x%lx exit_reason=%lu host_call_result=%u\n",
			"host_realm_rec_enter", res, exit_reason, host_call_result);
		test_res = TEST_RESULT_FAIL;
	}

	/*
	 * 13. Read L2 RTT entry, walk should terminate at L2.
	 *     Expect HIPAS=RMI_ASSIGNED_DEV, RIPAS=exit.ripas_value.
	 */
	ret = host_rmi_rtt_readentry(realm.rd, map_addr[0], RTT_MIN_DEV_BLOCK_LEVEL, &rtt);
	if ((res != RMI_SUCCESS) || (rtt.walk_level != RTT_MIN_DEV_BLOCK_LEVEL) ||
		(rtt.state != RMI_ASSIGNED_DEV) || (rtt.ripas != ripas) ||
		(rtt.out_addr != map_addr[0])) {
		ERROR("host_rmi_rtt_readentry() failed, 0x%lx rtt.state=%lu"
			" rtt.ripas=%lu rtt.walk_level=%ld rtt.out_addr=0x%llx\n",
			res, rtt.ripas, rtt.state, rtt.walk_level, rtt.out_addr);
		test_res = TEST_RESULT_FAIL;
	}

	/*
	 * 14. Unassign VDEV (the PCIe endpoint) from the Realm
	 */
	ret = host_unassign_vdev_from_realm(&realm, h_vdev);
	if (ret != 0) {
		ERROR("VDEV unassign to realm failed\n");
		test_res = TEST_RESULT_FAIL;
	}

unmap_granules:
	/* 15. Unfold. Create L3 RTT entries */
	res = host_rmi_create_rtt_levels(&realm, base_pa,
					 RTT_MIN_DEV_BLOCK_LEVEL, RTT_MAX_LEVEL);
	if (res != RMI_SUCCESS) {
		ERROR("%s() for 0x%lx failed, 0x%lx\n",
			"host_rmi_create_rtt_levels", base_pa, res);
		test_res = TEST_RESULT_FAIL;
		goto unmap_memory;
	}

	if (test_res != TEST_RESULT_SUCCESS) {
		goto unmap_memory;
	}

	/*
	 * 16. Read L3 RTT entries, walk should terminate at L3.
	 *     Expect HIPAS=RMI_ASSIGNED_DEV, RIPAS=exit.ripas_value.
	 */
	for (i = 0U; i < DEV_MEM_NUM1_GRANULES; i++) {
		res = host_rmi_rtt_readentry(realm.rd, map_addr[i], RTT_MAX_LEVEL, &rtt);
		if ((res != RMI_SUCCESS) || (rtt.walk_level != RTT_MAX_LEVEL) ||
			(rtt.state != RMI_ASSIGNED_DEV) || (rtt.ripas != ripas) ||
			(rtt.out_addr != map_addr[i])) {
			ERROR("host_rmi_rtt_readentry() failed, 0x%lx rtt.state=%lu"
				" rtt.ripas=%lu rtt.walk_level=%ld rtt.out_addr=0x%llx\n",
				res, rtt.ripas, rtt.state, rtt.walk_level, rtt.out_addr);
			test_res = TEST_RESULT_FAIL;
			goto unmap_memory;
		}
	}

unmap_memory:
	/* 17. Unmap device memory */
	for (i = 0U; i < num_map; i++) {
		__unused u_register_t pa, top;

		res = host_rmi_dev_mem_unmap(realm.rd, map_addr[i],
						RTT_MAX_LEVEL, &pa, &top);
		if (res != RMI_SUCCESS) {
			ERROR("%s for 0x%lx failed, 0x%lx\n",
				"Device memory unmap", map_addr[i], res);
			test_res = TEST_RESULT_FAIL;
			goto undelegate_granules;
		}
	}

	if (test_res != TEST_RESULT_SUCCESS) {
		goto undelegate_granules;
	}

	/*
	 * 18. Read RTT entries, walk should terminate at L3.
	 *     Expect HIPAS=RMI_UNASSIGNED, RIPAS=exit.ripas_value.
	 */
	for (i = 0U; i < DEV_MEM_NUM1_GRANULES; i++) {
		res = host_rmi_rtt_readentry(realm.rd, map_addr[i], RTT_MAX_LEVEL, &rtt);
		if ((res != RMI_SUCCESS) || (rtt.walk_level != RTT_MAX_LEVEL) ||
			(rtt.state != RMI_UNASSIGNED) || (rtt.ripas != ripas) ||
			(rtt.out_addr != 0UL)) {
			ERROR("host_rmi_rtt_readentry() failed, 0x%lx rtt.state=%lu"
				" rtt.ripas=%lu rtt.walk_level=%ld rtt.out_addr=0x%llx\n",
				res, rtt.ripas, rtt.state, rtt.walk_level, rtt.out_addr);
			test_res = TEST_RESULT_FAIL;
			goto undelegate_granules;
		}
	}

undelegate_granules:
	/*
	 * 19. Undelegate device granules
	 */
	for (i = 0U; i < num_dlg; i++) {
		res = host_rmi_granule_undelegate(base_pa + i * GRANULE_SIZE);
		if (res != RMI_SUCCESS) {
			ERROR("%s for 0x%lx failed, 0x%lx\n",
				"Granule undelegate",
				(base_pa + i * GRANULE_SIZE), res);
			test_res = TEST_RESULT_FAIL;
		}
	}

	/*
	 * 20. Reclaim PDEV (the PCIe TDI) from TSM
	 */

	if (h_pdev->is_connected_to_tsm) {
		ret = tsm_disconnect_device(h_pdev);

		INFO("tsm_disconnect_device %u\n", ret);
		if (ret != 0) {
			test_res = TEST_RESULT_FAIL;
		}
	}

destroy_realm:
	/*
	 * 21. Destroy Realm
	 */
	if (!host_destroy_realm(&realm)) {
		ERROR("Realm destroy failed\n");
		test_res = TEST_RESULT_FAIL;
	}

	return test_res;
}

/*
 * This invokes various RMI calls related to PDEV, VDEV management that does
 * PDEV create/communicate/set_key/abort/stop/destroy and assigns the device
 * to a Realm using RMI VDEV ABIs.
 *
 * 1.  Create a Realm with DA feature enabled.
 *     Find a known PCIe endpoint and connect with TSM to get_cert and establish
 *     secure session.
 * 2.  Find and connect device with TSM.
 * 3.  Assign VDEV (the PCIe endpoint) to the Realm
 * 4.  Call Realm to do DA related RSI calls. Realm should exit with
 *     RMI_EXIT_DEV_MEM_MAP.
 * 5.  Enter device memory validation loop.
 *     Delegate device granule.
 *     Map device memory.
 *     Validate device memory for a granule.
 *     Read L3 RTT entry, walk should terminate at L3, and HIPAS=RMI_ASSIGNED_DEV,
 *     RIPAS=RMI_EMPTY.
 *     Realm should exit with RMI_EXIT_HOST_CALL or RMI_EXIT_RIPAS_CHANGE.
 * 6.  Read L3 RTT entries, walk should terminate at L3, and HIPAS=RMI_ASSIGNED_DEV,
 *     RIPAS=RMI_DEV.
 * 7.  Complete Realm's requests to change RIPAS. Realm should exit with
 *     RMI_EXIT_HOST_CALL.
 * 8.  Read L3 RTT entries, walk should terminate at L3, and HIPAS=RMI_ASSIGNED_DEV,
 *     RIPAS=exit.ripas_value.
 * 9.  Unassign VDEV (the PCIe endpoint) from the Realm
 * 10. Unmap device memory.
 * 11. Read L3 RTT entries, walk should terminate at L3, and HIPAS=RMI_UNASSIGNED,
 *     RIPAS=exit.ripas_value.
 * 12. Undelegate device granules.
 * 13. Reclaim PDEV (the PCIe TDI) from TSM
 * 14. Destroy Realm.
 */
test_result_t host_realm_dev_mem_pages(void)
{
	u_register_t rmi_feat_reg0;
	u_register_t res, exit_reason, out_top;
	u_register_t map_addr[DEV_MEM_NUM2_GRANULES];
	uintptr_t base_pa;
	unsigned int host_call_result = TEST_RESULT_FAIL;
	struct host_pdev *h_pdev;
	struct host_vdev *h_vdev;
	struct realm realm;
	struct rmi_rec_run *run;
	struct rtt_entry rtt;
	unsigned int num_reg, num_dlg = 0U, num_map = 0U, i;
	int ret;
	bool realm_rc;
	test_result_t test_res = TEST_RESULT_SUCCESS;
	u_register_t ripas = RMI_EMPTY;

	INIT_AND_SKIP_DA_TEST_IF_PREREQS_NOT_MET(rmi_feat_reg0);

	/* Initialize Host NS heap memory */
	ret = page_pool_init((u_register_t)PAGE_POOL_BASE,
				(u_register_t)PAGE_POOL_MAX_SIZE);
	if (ret != HEAP_INIT_SUCCESS) {
		ERROR("Failed to init heap pool %d\n", ret);
		return TEST_RESULT_FAIL;
	}

	/*
	 * TODO: set device memory base address and size in attestation
	 * report
	 */

	/* Retrieve platform PCIe memory region 1, if available */
	num_reg = 1U;

	while (true) {
		size_t dev_size;

		ret = plat_get_dev_region((uint64_t *)&base_pa, &dev_size,
					  DEV_MEM_NON_COHERENT, num_reg);
		if ((ret == 0) && (dev_size >= DEV_MEM_TEST2_SIZE)) {
			break;
		}
		if (num_reg == 0U) {
			return TEST_RESULT_SKIPPED;
		}
		--num_reg;
	}

	INFO("Testing PCIe memory region %u 0x%lx-0x%lx\n",
		num_reg, base_pa, base_pa + DEV_MEM_TEST2_SIZE - 1UL);

	/*
	 * 1. Create a Realm with DA feature enabled
	 *
	 * todo: creating this after host_pdev_setup cases Realm create to
	 * fail.
	 */
	ret = host_create_realm_with_feat_da(&realm);
	if (ret != 0) {
		ERROR("Realm create with feat_da failed\n");
		return TEST_RESULT_FAIL;
	}

	INFO("Realm created with feat_da enabled\n");

	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 0U,
					HOST_ARG1_INDEX, base_pa);
	host_shared_data_set_host_val(&realm, PRIMARY_PLANE_ID, 0U,
					HOST_ARG2_INDEX, DEV_MEM_TEST2_SIZE);
	h_vdev = &gbl_host_vdev;

	/* 2. Find and connect device with TSM */
	h_pdev = get_host_pdev_by_type(DEV_TYPE_INDEPENDENTLY_ATTESTED);
	if (h_pdev == NULL) {
		/* If no device is connected to TSM, then skip the test */
		test_res = TEST_RESULT_SKIPPED;
		goto destroy_realm;
	}

	ret = tsm_connect_device(h_pdev);
	if (ret != 0) {
		ERROR("TSM connect failed for device 0x%x\n", h_pdev->dev->bdf);
		test_res = TEST_RESULT_FAIL;
		goto destroy_realm;
	}

	h_pdev->ncoh_num_addr_range = NCOH_ADDR_RANGE_NUM;
	h_pdev->ncoh_addr_range[0].base = base_pa;
	h_pdev->ncoh_addr_range[0].top = base_pa + DEV_MEM_TEST2_SIZE;

	/*
	 * 3. Assign VDEV (the PCIe endpoint) to the Realm
	 */
	INFO("======================================\n");
	INFO("Host: Assign device: (0x%x) %x:%x.%x to Realm \n",
	     (uint32_t)h_pdev->dev->bdf,
	     PCIE_EXTRACT_BDF_BUS((uint32_t)h_pdev->dev->bdf),
	     PCIE_EXTRACT_BDF_DEV((uint32_t)h_pdev->dev->bdf),
	     PCIE_EXTRACT_BDF_FUNC((uint32_t)h_pdev->dev->bdf));
	INFO("======================================\n");

	ret = host_assign_vdev_to_realm(&realm, h_vdev, h_pdev->dev->bdf, h_pdev->pdev);
	if (ret != 0) {
		ERROR("VDEV assign to realm failed\n");
		test_res = TEST_RESULT_FAIL;
		goto destroy_realm;
	}

	/*
	 * 4. Call Realm to do DA related RSI calls
	 */
	realm_rc = host_enter_realm_execute(&realm, REALM_DEV_MEM_ACCESS,
					    RMI_EXIT_DEV_MEM_MAP, 0U);

	run = (struct rmi_rec_run *)realm.run[0];
	exit_reason = run->exit.exit_reason;

	/* Realm should exit with RMI_EXIT_DEV_MEM_MAP */
	if (!realm_rc || (exit_reason != RMI_EXIT_DEV_MEM_MAP)) {
		ERROR("Rec enter failed, exit_reason %lu\n", exit_reason);
		test_res = TEST_RESULT_FAIL;
		goto unmap_memory;
	}

	/*
	 * 5. Enter device memory validation loop
	 */
	INFO("Enter device memory @0x%lx [0x%lx-0x%lx] validation loop\n",
		run->exit.dev_mem_pa, run->exit.dev_mem_base,
		run->exit.dev_mem_top - 1UL);

	out_top = 0UL;

	/* Complete Realm's requests to validate mappings to device memory */
	while (out_top != run->exit.dev_mem_top) {
		unsigned long dev_mem_pa = run->exit.dev_mem_pa;

		/* Delegate device granule */
		res = host_rmi_granule_delegate(dev_mem_pa);
		if (res != RMI_SUCCESS) {
			ERROR("%s() for 0x%lx failed, 0x%lx\n",
				"host_rmi_granule_delegate",
				dev_mem_pa, res);
			test_res = TEST_RESULT_FAIL;
			goto undelegate_granules;
		}

		++num_dlg;

		/* Map device memory */
		res = host_dev_mem_map(&realm, dev_mem_pa, RTT_MAX_LEVEL,
					&map_addr[num_map]);
		if (res != REALM_SUCCESS) {
			ERROR("%s() for 0x%lx failed, 0x%lx\n",
				"host_dev_mem_map",
				dev_mem_pa, res);
			test_res = TEST_RESULT_FAIL;
			goto unmap_memory;
		}

		/*
		 * Read L3 RTT entry, walk should terminate at L3.
		 * Expect HIPAS=RMI_ASSIGNED_DEV, RIPAS=RMI_EMPTY.
		 */
		res = host_rmi_rtt_readentry(realm.rd, map_addr[num_map], RTT_MAX_LEVEL, &rtt);
		if ((res != RMI_SUCCESS) || (rtt.walk_level != RTT_MAX_LEVEL) ||
			(rtt.state != RMI_ASSIGNED_DEV) || (rtt.ripas != RMI_EMPTY) ||
			(rtt.out_addr != map_addr[num_map])) {
			ERROR("host_rmi_rtt_readentry() failed, 0x%lx rtt.state=%lu"
				" rtt.ripas=%lu rtt.walk_level=%ld rtt.out_addr=0x%llx\n",
				res, rtt.ripas, rtt.state, rtt.walk_level, rtt.out_addr);
			test_res = TEST_RESULT_FAIL;
			goto unmap_memory;
		}

		++num_map;

		/*
		 * Validate device memory
		 */
		res = host_rmi_rtt_dev_mem_validate(realm.rd, realm.rec[0],
						run->exit.dev_mem_base,
						run->exit.dev_mem_top,
						&out_top);
		if (res != RMI_SUCCESS) {
			ERROR("%s() failed, 0x%lx out_top=0x%lx\n",
				"host_rmi_rtt_dev_mem_validate", res, out_top);
				run->entry.flags = REC_ENTRY_FLAG_DEV_MEM_RESPONSE;
				test_res = TEST_RESULT_FAIL;
		}

		res = host_realm_rec_enter(&realm, &exit_reason, &host_call_result, 0U);
		if ((res != RMI_SUCCESS) || (exit_reason != RMI_EXIT_DEV_MEM_MAP)) {
			break;
		}
	}

	INFO("Exit device memory validation loop\n");

	/* Exit with RMI_EXIT_HOST_CALL or RMI_EXIT_RIPAS_CHANGE */
	if ((res != RMI_SUCCESS) || ((exit_reason == RMI_EXIT_HOST_CALL) &&
	    (host_call_result != TEST_RESULT_SUCCESS))) {
		ERROR("%s() failed, 0x%lx exit_reason=%lu host_call_result=%u\n",
			"host_realm_rec_enter", res, exit_reason, host_call_result);
		test_res = TEST_RESULT_FAIL;
		goto unmap_memory;
	}

	/*
	 * 6. Read L3 RTT entries, walk should terminate at L3.
	 *    Expect HIPAS=RMI_ASSIGNED_DEV, RIPAS=RMI_DEV.
	 */
	for (i = 0U; i < DEV_MEM_NUM2_GRANULES; i++) {
		res = host_rmi_rtt_readentry(realm.rd, map_addr[i], RTT_MAX_LEVEL, &rtt);
		if ((res != RMI_SUCCESS) || (rtt.walk_level != RTT_MAX_LEVEL) ||
			(rtt.state != RMI_ASSIGNED_DEV) || (rtt.ripas != RMI_DEV) ||
			(rtt.out_addr != map_addr[i])) {
			ERROR("host_rmi_rtt_readentry() failed, 0x%lx rtt.state=%lu"
				" rtt.ripas=%lu rtt.walk_level=%ld rtt.out_addr=0x%llx\n",
				res, rtt.ripas, rtt.state, rtt.walk_level, rtt.out_addr);
			test_res = TEST_RESULT_FAIL;
			goto unmap_memory;
		}
	}

	out_top = 0UL;

	INFO("Enter change RIPAS loop\n");

	/* 7. Complete Realm's requests to change RIPAS */
	while (exit_reason == RMI_EXIT_RIPAS_CHANGE) {
		while (out_top != run->exit.ripas_top) {
			INFO("set RIPAS=%u [0x%lx-0x%lx]\n",
				run->exit.ripas_value, run->exit.ripas_base,
				run->exit.ripas_top - 1UL);

			/* This call fails for RSI_DEV -> RSI_RAM */
			res = host_rmi_rtt_set_ripas(realm.rd,
					     realm.rec[0],
					     run->exit.ripas_base,
					     run->exit.ripas_top,
					     &out_top);
			if (res != RMI_SUCCESS) {
				INFO("%s() failed, 0x%lx ripas=%u\n", "host_rmi_rtt_set_ripas",
					res, run->exit.ripas_value);
				run->entry.flags = REC_ENTRY_FLAG_RIPAS_RESPONSE_REJECT;
			}

			res = host_realm_rec_enter(&realm, &exit_reason, &host_call_result, 0U);
			if ((res != RMI_SUCCESS) || (exit_reason != RMI_EXIT_RIPAS_CHANGE)) {
				break;
			}
		}
	}

	ripas = (u_register_t)run->exit.ripas_value;

	INFO("Exit change RIPAS loop, ripas=%lu exit_reason=%lu\n",
						ripas, exit_reason);

	/* Exit with RMI_EXIT_HOST_CALL */
	if ((res != RMI_SUCCESS) || (exit_reason != RMI_EXIT_HOST_CALL) ||
	    (host_call_result != TEST_RESULT_SUCCESS)) {
		ERROR("%s() failed, 0x%lx exit_reason=%lu host_call_result=%u\n",
			"host_realm_rec_enter", res, exit_reason, host_call_result);
		test_res = TEST_RESULT_FAIL;
	}

	/*
	 * 8. Read L3 RTT entries, walk should terminate at L3.
	 *    Expect HIPAS=RMI_ASSIGNED_DEV, RIPAS=exit.ripas_value.
	 */
	for (i = 0U; i < DEV_MEM_NUM2_GRANULES; i++) {
		res = host_rmi_rtt_readentry(realm.rd, map_addr[i], RTT_MAX_LEVEL, &rtt);
		if ((res != RMI_SUCCESS) || (rtt.walk_level != RTT_MAX_LEVEL) ||
			(rtt.state != RMI_ASSIGNED_DEV) || (rtt.ripas != ripas) ||
			(rtt.out_addr != map_addr[i])) {
			ERROR("host_rmi_rtt_readentry() failed, 0x%lx rtt.state=%lu"
				" rtt.ripas=%lu rtt.walk_level=%ld rtt.out_addr=0x%llx\n",
				res, rtt.ripas, rtt.state, rtt.walk_level, rtt.out_addr);
			test_res = TEST_RESULT_FAIL;
			goto unmap_memory;
		}
	}

	/*
	 * 9. Unassign VDEV (the PCIe endpoint) from the Realm
	 */
	ret = host_unassign_vdev_from_realm(&realm, h_vdev);
	if (ret != 0) {
		ERROR("VDEV unassign to realm failed\n");
		test_res = TEST_RESULT_FAIL;
	}

unmap_memory:
	/* 10. Unmap device memory */
	for (i = 0U; i < num_map; i++) {
		__unused u_register_t pa, top;

		res = host_rmi_dev_mem_unmap(realm.rd, map_addr[i],
						RTT_MAX_LEVEL, &pa, &top);
		if (res != RMI_SUCCESS) {
			ERROR("%s for 0x%lx failed, 0x%lx\n",
				"Device memory unmap", map_addr[i], res);
			test_res = TEST_RESULT_FAIL;
			goto undelegate_granules;
		}
	}

	if (test_res != TEST_RESULT_SUCCESS) {
		goto undelegate_granules;
	}

	/*
	 * 11. Read RTT entries, walk should terminate at L3.
	 *     Expect HIPAS=RMI_UNASSIGNED, RIPAS=exit.ripas_value.
	 */
	for (i = 0U; i < DEV_MEM_NUM2_GRANULES; i++) {
		res = host_rmi_rtt_readentry(realm.rd, map_addr[i], RTT_MAX_LEVEL, &rtt);
		if ((res != RMI_SUCCESS) || (rtt.walk_level != RTT_MAX_LEVEL) ||
			(rtt.state != RMI_UNASSIGNED) || (rtt.ripas != ripas) ||
			(rtt.out_addr != 0UL)) {
			ERROR("host_rmi_rtt_readentry() failed, 0x%lx rtt.state=%lu"
				" rtt.ripas=%lu rtt.walk_level=%ld rtt.out_addr=0x%llx\n",
				res, rtt.ripas, rtt.state, rtt.walk_level, rtt.out_addr);
			test_res = TEST_RESULT_FAIL;
			goto undelegate_granules;
		}
	}

undelegate_granules:
	/*
	 * 12. Undelegate device granules
	 */
	for (i = 0U; i < num_dlg; i++) {
		res = host_rmi_granule_undelegate(base_pa + i * GRANULE_SIZE);
		if (res != RMI_SUCCESS) {
			ERROR("%s for 0x%lx failed, 0x%lx\n",
				"Granule undelegate",
				(base_pa + i * GRANULE_SIZE), res);
			test_res = TEST_RESULT_FAIL;
		}
	}

	/*
	 * 13. Reclaim PDEV (the PCIe TDI) from TSM
	 */
	if (h_pdev->is_connected_to_tsm) {
		ret = tsm_disconnect_device(h_pdev);

		INFO("tsm_disconnect_device %u\n", ret);
		if (ret != 0) {
			test_res = TEST_RESULT_FAIL;
		}
	}

destroy_realm:
	/*
	 * 14. Destroy Realm
	 */
	if (!host_destroy_realm(&realm)) {
		ERROR("Realm destroy failed\n");
		test_res = TEST_RESULT_FAIL;
	}

	return test_res;
}

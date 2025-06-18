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

#define DEV_MEM_TEST1_SIZE		SZ_2M
#define DEV_MEM_NUM1_GRANULES		(DEV_MEM_TEST1_SIZE / GRANULE_SIZE)

#define DEV_MEM_TEST2_SIZE		32 * SZ_4K
#define DEV_MEM_NUM2_GRANULES		(DEV_MEM_TEST2_SIZE / GRANULE_SIZE)

extern struct host_pdev gbl_host_pdev;
extern struct host_vdev gbl_host_vdev;

static uint32_t pdev_bdf;
static uint32_t doe_cap_base;

static int pdev_setup_assign(struct realm *realm,
				struct host_pdev *h_pdev,
				struct host_vdev *h_vdev,
				unsigned long base_pa,
				size_t size)
{
	uint8_t public_key_algo;
	int rc;

	/* Allocate granules. Skip DA ABIs if host_pdev_setup fails */
	rc = host_pdev_setup(h_pdev);
	if (rc != 0) {
		ERROR("host_pdev_setup() failed\n");
		return rc;
	}

	/* todo: move to tdi_pdev_setup */
	h_pdev->bdf = pdev_bdf;
	h_pdev->doe_cap_base = doe_cap_base;
	h_pdev->ncoh_num_addr_range = NCOH_ADDR_RANGE_NUM;
	h_pdev->ncoh_addr_range[0].base = base_pa;
	h_pdev->ncoh_addr_range[0].top = base_pa + size;

	/* Call rmi_pdev_create to transition PDEV to STATE_NEW */
	rc = host_pdev_transition(h_pdev, RMI_PDEV_STATE_NEW);
	if (rc != 0) {
		ERROR("PDEV transition: NULL -> STATE_NEW failed\n");
		return rc;
	}

	/* Call rmi_pdev_communicate to transition PDEV to NEEDS_KEY */
	rc = host_pdev_transition(h_pdev, RMI_PDEV_STATE_NEEDS_KEY);
	if (rc != 0) {
		ERROR("PDEV transition: PDEV_NEW -> PDEV_NEEDS_KEY failed\n");
		return rc;
	}

	/* Get public key. Verifying cert_chain not done by host but by Realm? */
	rc = host_get_public_key_from_cert_chain(h_pdev->cert_chain,
						 h_pdev->cert_chain_len,
						 h_pdev->public_key,
						 &h_pdev->public_key_len,
						 h_pdev->public_key_metadata,
						 &h_pdev->public_key_metadata_len,
						 &public_key_algo);
	if (rc != 0) {
		ERROR("Get public key failed\n");
		return rc;
	}

	if (public_key_algo == PUBLIC_KEY_ALGO_ECDSA_ECC_NIST_P256) {
		h_pdev->public_key_sig_algo = RMI_SIGNATURE_ALGORITHM_ECDSA_P256;
	} else if (public_key_algo == PUBLIC_KEY_ALGO_ECDSA_ECC_NIST_P384) {
		h_pdev->public_key_sig_algo = RMI_SIGNATURE_ALGORITHM_ECDSA_P384;
	} else {
		h_pdev->public_key_sig_algo = RMI_SIGNATURE_ALGORITHM_RSASSA_3072;
	}
	INFO("DEV public key len/sig_algo: %ld/%d\n", h_pdev->public_key_len,
	     h_pdev->public_key_sig_algo);

	/* Call rmi_pdev_set_key transition PDEV to HAS_KEY */
	rc = host_pdev_transition(h_pdev, RMI_PDEV_STATE_HAS_KEY);
	if (rc != 0) {
		ERROR("PDEV transition: PDEV_NEEDS_KEY -> PDEV_HAS_KEY failed\n");
		return rc;
	}

	/* Call rmi_pdev_comminucate to transition PDEV to READY state */
	rc = host_pdev_transition(h_pdev, RMI_PDEV_STATE_READY);
	if (rc != 0) {
		ERROR("PDEV transition: PDEV_HAS_KEY -> PDEV_READY failed\n");
		return rc;
	}

	/*
	 * Assign VDEV (the PCIe endpoint) from the Realm
	 */
	rc = host_assign_vdev_to_realm(realm, h_pdev, h_vdev);
	if (rc != 0) {
		ERROR("VDEV assign to realm failed\n");
	}
	return rc;
}

/*
 * This invokes various RMI calls related to PDEV, VDEV management that does
 * PDEV create/communicate/set_key/abort/stop/destroy and assigns the device
 * to a Realm using RMI VDEV ABIs
 *
 * 1.  Create a Realm with DA feature enabled.
 *     Find a known PCIe endpoint and connect with TSM to get_cert and establish
 *     secure session.
 * 2.  Assign the PCIe endpoint (a PF) to the Realm.
 * 3.  Delegate 2MB of device granules.
 * 4.  Map 2MB of device granules.
 * 5.  Read L3 RTT entries, walk should terminate at L3, and HIPAS=RMI_ASSIGNED_DEV,
 *     RIPAS=RMI_EMPTY.
 * 6.  Call RTT Fold.
 * 7.  Read L2 RTT entry, walk should terminate at L2, and HIPAS=RMI_ASSIGNED_DEV,
 *     RIPAS=RMI_EMPTY.
 * 8.  Call Realm to do DA related RSI calls. Realm should exit with
 *     RMI_EXIT_DEV_MEM_MAP.
 * 9.  Enter device memory validation loop.
 *     Validate device memory.
 *     Realm should exit with RMI_EXIT_HOST_CALL or RMI_EXIT_RIPAS_CHANGE.
 * 10. Read L2 RTT entry, walk should terminate at L2, and HIPAS=RMI_ASSIGNED_DEV,
 *     RIPAS=RMI_DEV.
 * 11. Complete Realm's requests to change RIPAS. Realm should exit with
 *     RMI_EXIT_HOST_CALL.
 * 12. Read RTT entry, walk should terminate at L2, and HIPAS=RMI_ASSIGNED_DEV,
 *     RIPAS=exit.ripas_value.
 * 13. Unassign the PCIe endpoint from the Realm
 * 14. Unfold. Create L3 RTT entries.
 * 15. Read L3 RTT entries, walk should terminate at L3, and HIPAS=RMI_ASSIGNED_DEV,
 *     RIPAS=exit.ripas_value.
 * 16. Unmap 2MB of device memory.
 * 17. Read L3 RTT entries, walk should terminate at L3, and HIPAS=RMI_UNASSIGNED,
 *     RIPAS=exit.ripas_value.
 * 18. Undelegate 2MB of device granules.
 * 19. Destroy Realm.
 * 20. Reclaim PDEV (the PCIe TDI) from TSM.
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

	CHECK_DA_SUPPORT_IN_RMI(rmi_feat_reg0);
	SKIP_TEST_IF_DOE_NOT_SUPPORTED(pdev_bdf, doe_cap_base);

	INFO("DA on bdf: 0x%x, doe_cap_base: 0x%x\n", pdev_bdf, doe_cap_base);

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

	/* Retrieve platform PCIe memory region 0 */
	ret = plat_get_dev_region((uint64_t *)&base_pa, &dev_size,
				  DEV_MEM_NON_COHERENT, 0U);

	if ((ret != 0) || (dev_size < DEV_MEM_TEST1_SIZE)) {
		return TEST_RESULT_SKIPPED;
	}

	INFO("Testing PCIe memory region 0 0x%lx-0x%lx\n",
		base_pa, base_pa + DEV_MEM_TEST1_SIZE - 1UL);

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

	h_pdev = &gbl_host_pdev;
	h_vdev = &gbl_host_vdev;

	/*
	 * 2. Assign VDEV (the PCIe endpoint) from the Realm.
	 */
	ret = pdev_setup_assign(&realm, h_pdev, h_vdev, base_pa,
				DEV_MEM_TEST1_SIZE);
	if (ret != 0) {
		test_res = TEST_RESULT_FAIL;
		goto destroy_realm;
	}

	/* 3. Delegate device granules */
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

	/* 4. Map device memory */
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
	 * 5. Read L3 RTT entries, walk should terminate at L3.
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

	/* 6. RTT Fold */
	res = host_realm_fold_rtt(realm.rd, map_addr[0], RTT_MAX_LEVEL);
	if (res != RMI_SUCCESS) {
		ERROR("%s() for 0x%lx failed, 0x%lx\n",
			"host_rmi_rtt_fold", map_addr[0], res);
		test_res = TEST_RESULT_FAIL;
		goto unmap_memory;
	}

	/*
	 * 7. Read L2 RTT entry, walk should terminate at L2.
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

	h_pdev = &gbl_host_pdev;
	h_vdev = &gbl_host_vdev;

	/*
	 * 8. Call Realm to do DA related RSI calls
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
	 * 9. Validate device memory
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
	 * 10. Read L2 RTT entry, walk should terminate at L2.
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

	/* 11. Complete Realm's requests to change RIPAS */
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
	 * 12. Read L2 RTT entry, walk should terminate at L2.
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
	 * 13. Unassign VDEV (the PCIe endpoint) from the Realm
	 */
	ret = host_unassign_vdev_from_realm(&realm, h_pdev, h_vdev);
	if (ret != 0) {
		ERROR("VDEV unassign to realm failed\n");
		test_res = TEST_RESULT_FAIL;
	}

unmap_granules:
	/* 14. Unfold. Create L3 RTT entries */
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
	 * 15. Read L3 RTT entries, walk should terminate at L3.
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
	/* 16. Unmap device memory */
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
	 * 17. Read RTT entries, walk should terminate at L3.
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
	 * 18. Undelegate device granules
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

destroy_realm:
	/*
	 * 19. Destroy Realm
	 */
	if (!host_destroy_realm(&realm)) {
		ERROR("Realm destroy failed\n");
		test_res = TEST_RESULT_FAIL;
	}

	/*
	 * 20. Reclaim PDEV (the PCIe TDI) from TSM
	 */
	ret = host_pdev_reclaim(h_pdev);
	if (ret != 0) {
		ERROR("Reclaim PDEV from TSM failed\n");
		return TEST_RESULT_FAIL;
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
 * 2.  Assign the PCIe endpoint (a PF) to the Realm.
 * 3.  Call Realm to do DA related RSI calls. Realm should exit with
 *     RMI_EXIT_DEV_MEM_MAP.
 * 4.  Enter device memory validation loop.
 *     Delegate device granule.
 *     Map device memory.
 *     Validate device memory for a granule.
 *     Read L3 RTT entry, walk should terminate at L3, and HIPAS=RMI_ASSIGNED_DEV,
 *     RIPAS=RMI_EMPTY.
 *     Realm should exit with RMI_EXIT_HOST_CALL or RMI_EXIT_RIPAS_CHANGE.
 * 5.  Read L3 RTT entries, walk should terminate at L3, and HIPAS=RMI_ASSIGNED_DEV,
 *     RIPAS=RMI_DEV.
 * 6.  Complete Realm's requests to change RIPAS. Realm should exit with
 *     RMI_EXIT_HOST_CALL.
 * 7.  Read L3 RTT entries, walk should terminate at L3, and HIPAS=RMI_ASSIGNED_DEV,
 *     RIPAS=exit.ripas_value.
 * 8.  Unassign the PCIe endpoint from the Realm.
 * 9.  Unmap device memory.
 * 10. Read L3 RTT entries, walk should terminate at L3, and HIPAS=RMI_UNASSIGNED,
 *     RIPAS=exit.ripas_value.
 * 11. Undelegate device granules.
 * 12. Destroy Realm.
 * 13. Reclaim PDEV (the PCIe TDI) from TSM.
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

	CHECK_DA_SUPPORT_IN_RMI(rmi_feat_reg0);
	SKIP_TEST_IF_DOE_NOT_SUPPORTED(pdev_bdf, doe_cap_base);

	INFO("DA on bdf: 0x%x, doe_cap_base: 0x%x\n", pdev_bdf, doe_cap_base);

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

	h_pdev = &gbl_host_pdev;
	h_vdev = &gbl_host_vdev;

	/*
	 * 2. Assign VDEV (the PCIe endpoint) from the Realm
	 */
	ret = pdev_setup_assign(&realm, h_pdev, h_vdev, base_pa,
				DEV_MEM_TEST1_SIZE);
	if (ret != 0) {
		test_res = TEST_RESULT_FAIL;
		goto destroy_realm;
	}

	h_pdev = &gbl_host_pdev;
	h_vdev = &gbl_host_vdev;

	/*
	 * 3. Call Realm to do DA related RSI calls
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
	 * 4. Validate device memory
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
	 * 5. Read L3 RTT entries, walk should terminate at L3.
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

	/* 6. Complete Realm's requests to change RIPAS */
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
	 * 7. Read L3 RTT entries, walk should terminate at L3.
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
	 * 8. Unassign VDEV (the PCIe endpoint) from the Realm
	 */
	ret = host_unassign_vdev_from_realm(&realm, h_pdev, h_vdev);
	if (ret != 0) {
		ERROR("VDEV unassign to realm failed\n");
		test_res = TEST_RESULT_FAIL;
	}

unmap_memory:
	/* 9. Unmap device memory */
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
	 * 10. Read RTT entries, walk should terminate at L3.
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
	 * 11. Undelegate device granules
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

destroy_realm:
	/*
	 * 12. Destroy Realm
	 */
	if (!host_destroy_realm(&realm)) {
		ERROR("Realm destroy failed\n");
		test_res = TEST_RESULT_FAIL;
	}

	/*
	 * 13. Reclaim PDEV (the PCIe TDI) from TSM
	 */
	ret = host_pdev_reclaim(h_pdev);
	if (ret != 0) {
		ERROR("Reclaim PDEV from TSM failed\n");
		return TEST_RESULT_FAIL;
	}

	return test_res;
}

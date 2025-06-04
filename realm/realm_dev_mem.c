/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <arch_features.h>
#include <arch_helpers.h>
#include <assert.h>
#include <debug.h>
#include <stdlib.h>
#include <realm_rsi.h>
#include <sync.h>

#include <host_shared_data.h>
#include <realm_da_helpers.h>

static struct rdev gbl_rdev;

/* Measurement parameters */
static struct rsi_dev_measure_params gbl_rdev_meas_params
	__aligned(GRANULE_SIZE);

/* RDEV info. device type and attestation evidence digest */
static struct rsi_dev_info gbl_rdev_info __aligned(GRANULE_SIZE);

static void dev_mem_dump(u_register_t base, unsigned int num)
{
	unsigned long *ptr = (unsigned long *)base;

	while (num != 0) {
		switch (num) {
		case 1:
			realm_printf("%lx: %8lx\n", (unsigned long)ptr, *ptr);
			return;
		case 2:
			realm_printf("%lx: %8lx %8lx\n", (unsigned long)ptr, *ptr,
					*(ptr + 1));
			return;
		case 3:
			realm_printf("%lx: %8lx %8lx %8lx\n",
					(unsigned long)ptr, *ptr, *(ptr + 1),
					*(ptr + 2));
			return;
		default:
			realm_printf("%lx: %8lx %8lx %8lx %8lx\n",
					(unsigned long)ptr, *ptr, *(ptr + 1),
					*(ptr + 2), *(ptr + 3));
			ptr += 4;
			num -= 4;
			continue;
		}
	}
}

/*
 * If the Realm supports DA feature, this function calls series of RSI RDEV
 * on the assigned device.
 *
 * Returns 'true' success.
 */
bool test_realm_dev_mem_access(void)
{
	struct rdev *rdev;
	unsigned long ret;
	u_register_t rsi_feature_reg0;
	struct rsi_dev_measure_params *rdev_meas_params;
	struct rsi_dev_info *rdev_info;
	u_register_t pa_base, flags;
	u_register_t ipa_base, new_ipa_base;
	u_register_t ipa_top, new_ipa_top;
	size_t size;
	rsi_response_type response;
	rsi_ripas_type ripas;
	bool res = true;

	/* Check if RSI_FEATURES support DA */
	ret = rsi_features(RSI_FEATURE_REGISTER_0_INDEX, &rsi_feature_reg0);
	if (ret != RSI_SUCCESS) {
		return false;
	}

	if (EXTRACT(RSI_FEATURE_REGISTER_0_DA, rsi_feature_reg0) !=
	    RSI_FEATURE_TRUE) {
		ERROR("RSI feature DA not supported for current Realm\n");
		return false;
	}

	/* Get the global RDEV. Currently only one RDEV is supported */
	rdev = &gbl_rdev;
	rdev_meas_params = &gbl_rdev_meas_params;
	rdev_info = &gbl_rdev_info;

	ret = realm_rdev_init(rdev, RDEV_ID);
	if (ret != RSI_SUCCESS) {
		return false;
	}

	ret = realm_rsi_rdev_get_state(rdev);
	if (ret != RSI_SUCCESS) {
		return false;
	}

	/* Before lock get_measurement */
	ret = realm_rsi_rdev_get_measurements(rdev, rdev_meas_params);
	if (ret != RSI_SUCCESS) {
		return false;
	}

	ret = realm_rsi_rdev_lock(rdev);
	if (ret != RSI_SUCCESS) {
		return false;
	}

	/* After lock get_measurement */
	ret = realm_rsi_rdev_get_measurements(rdev, rdev_meas_params);
	if (ret != RSI_SUCCESS) {
		return false;
	}

	ret = realm_rsi_rdev_get_interface_report(rdev);
	if (ret != RSI_SUCCESS) {
		return false;
	}

	/* After meas and ifc_report, get device info */
	ret = realm_rsi_rdev_get_info(rdev, rdev_info);
	if (ret != RSI_SUCCESS) {
		return false;
	}

	/*
	 * Get cached device attestation from Host and verify it with device
	 * attestation digest
	 */
	(void)realm_verify_device_attestation(rdev, rdev_info);

	/* Todo: get device memory from attestation report */
	pa_base = realm_shared_data_get_my_host_val(HOST_ARG1_INDEX);
	size = realm_shared_data_get_my_host_val(HOST_ARG2_INDEX);

	flags = INPLACE(RSI_DEV_MEM_FLAGS_COH, RSI_DEV_MEM_NON_COHERENT) |
		INPLACE(RSI_DEV_MEM_FLAGS_LOR, RSI_DEV_MEM_NOT_LIMITED_ORDER);

	/* 1:1 mapping */
	ipa_base = pa_base;
	new_ipa_base = ipa_base;
	ipa_top = ipa_base + size;

	while (new_ipa_base < ipa_top) {
		ret = rsi_rdev_vaildate_mapping(rdev->id, rdev->inst_id,
						new_ipa_base, ipa_top, pa_base,
						flags, &new_ipa_base,
						&response);
		if ((ret != RSI_SUCCESS) || (response != RSI_ACCEPT)) {
			realm_printf("rsi_rdev_vaildate_mapping() failed %lu\n");
			return false;
		}

		pa_base = new_ipa_base;
	}

	/* Verify that RIAS has changed for [base, top) range */
	ret = rsi_ipa_state_get(ipa_base, ipa_top, &new_ipa_top, &ripas);
	if ((ret != RSI_SUCCESS) || (ripas != RSI_DEV) || (new_ipa_top != ipa_top)) {
		ERROR("rsi_ipa_state_get() failed, 0x%lx ipa_base=0x%lx ipa_top=0x%lx",
				"new_ipa_top=0x%lx ripas=%u\n",
				ret, ipa_base, ipa_top, new_ipa_top, ripas);
		return false;
	}

	/* Dump device memory */
	realm_printf("Device memory:\n");
	dev_mem_dump(ipa_base, 8);
	dev_mem_dump(ipa_top - 8 * sizeof(unsigned long), 8);

	ret = realm_rsi_rdev_start(rdev);
	if (ret != RSI_SUCCESS) {
		return false;
	}

	/* Set RIPAS to RSI_RAM, this should return RSI_REJECT */
	ret = rsi_ipa_state_set(ipa_base, ipa_top, RSI_RAM,
				RSI_NO_CHANGE_DESTROYED, &new_ipa_top, &response);

	if ((ret == RSI_SUCCESS) && (response == RSI_REJECT) &&
	    (new_ipa_top == ipa_base)) {
		realm_printf("%s passed, response=%u\n",
				"rsi_ipa_state_set() RSI_RAM ", response);

		ret = rsi_ipa_state_get(ipa_base, ipa_top, &new_ipa_top, &ripas);
		if ((ret != RSI_SUCCESS) || (ripas != RSI_DEV) ||
		    (new_ipa_top != ipa_top)) {
			realm_printf("rsi_ipa_state_get() failed 0x%lx, top=0x%lx",
					"new_ipa_top=0x%lx ripas=%u\n",
					ret, ipa_top, new_ipa_top, ripas);
			res = false;
		}
	} else {
		realm_printf("%s failed, response=%u\n",
			"rsi_ipa_state_set() RSI_RAM ", response);
		res = false;
	}

	/* Set RIPAS to RSI_EMPTY, this should return RSI_ACCEPT */
	ret = rsi_ipa_state_set(ipa_base, ipa_top, RSI_EMPTY,
				RSI_NO_CHANGE_DESTROYED, &new_ipa_top, &response);

	if ((ret == RSI_SUCCESS) && (response == RSI_ACCEPT) &&
	    (new_ipa_top == ipa_top)) {
		realm_printf("%s passed, response=%u\n",
				"rsi_ipa_state_set() RSI_EMPTY ", response);
		ret = rsi_ipa_state_get(ipa_base, ipa_top, &new_ipa_top, &ripas);
		if ((ret != RSI_SUCCESS) && (ripas != RSI_EMPTY) &&
		    (new_ipa_top != ipa_top)) {
			realm_printf("rsi_ipa_state_get() failed 0x%lx, top=0x%lx",
					"new_ipa_top=0x%lx ripas=%u\n",
					ret, ipa_top, new_ipa_top, ripas);
			res = false;
		}
	} else {
		ERROR("%s failed, response=%u\n",
			"rsi_ipa_state_set() RSI_EMPTY ", response);
		res = false;
	}

	ret = realm_rsi_rdev_stop(rdev);
	if (ret != RSI_SUCCESS) {
		return false;
	}

	return res;
}

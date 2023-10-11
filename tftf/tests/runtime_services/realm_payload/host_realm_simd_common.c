/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <tftf.h>
#include <host_realm_rmi.h>
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <lib/extensions/sve.h>

#include "host_realm_simd_common.h"

/*
 * Testcases in host_realm_spm.c, host_realm_payload_simd_tests.c uses these
 * common variables.
 */
sve_z_regs_t ns_sve_z_regs_write;
sve_z_regs_t ns_sve_z_regs_read;

test_result_t host_create_sve_realm_payload(struct realm *realm, bool sve_en,
					    uint8_t sve_vq)
{
	u_register_t feature_flag;
	u_register_t rec_flag[1] = {RMI_RUNNABLE};

	if (sve_en) {
		feature_flag = RMI_FEATURE_REGISTER_0_SVE_EN |
				INPLACE(FEATURE_SVE_VL, sve_vq);
	} else {
		feature_flag = 0UL;
	}

	/* Initialise Realm payload */
	if (!host_create_activate_realm_payload(realm,
				       (u_register_t)REALM_IMAGE_BASE,
				       (u_register_t)PAGE_POOL_BASE,
				       (u_register_t)PAGE_POOL_MAX_SIZE,
				       feature_flag, rec_flag, 1U)) {
		return TEST_RESULT_FAIL;
	}

	/* Create shared memory between Host and Realm */
	if (!host_create_shared_mem(realm, NS_REALM_SHARED_MEM_BASE,
				    NS_REALM_SHARED_MEM_SIZE)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

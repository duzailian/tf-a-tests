/*
 * Copyright (c) 2018, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <firmware_image_package.h>
#include <platform.h>
#include <tftf_lib.h>
#include <uuid.h>
#include <uuid_utils.h>
#include "tbb_test_infra.h"

unsigned int fip_offset(const uuid_t* uuid) {
	fip_toc_entry_t* current_file = (fip_toc_entry_t*) (PLAT_ARM_FIP_BASE + sizeof(fip_toc_header_t));
	while (!is_uuid_null(&(current_file->uuid))) {
		if (uuid_equal(&(current_file->uuid), uuid)) {
			return current_file->offset_address;
		}
		current_file += 1;
	};
	return 0;
}

test_result_t test_tbb_tkey_cert_header(void){
	static const uuid_t tkey_cert_uuid = UUID_TRUSTED_KEY_CERT;
	return corrupt_boot_fip_test(fip_offset(&tkey_cert_uuid));
}


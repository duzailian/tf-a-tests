/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
//#include <stdio.h>
//#include <stdlib.h>
#include <uuid.h>

//#include <drivers/auth/img_parser_mod.h>
//#include "mbedtls/error.h"
//#include "mbedtls/memory_buffer_alloc.h"
//#include <mbedtls/x509_crt.h>

//#include <plat/arm/common/plat_arm.h>
//#include <platform.h>
//#include <firmware_image_package.h>
//#include <fwu_nvm.h>
//#include <image_loader.h>
#include <io_storage.h>
#include <platform.h>
#include <platform_def.h>
#include <psci.h>
#include <smccc.h>
#include <status.h>
#include <tftf_lib.h>


#include <uuid_utils.h>
//#include <host_crypto_utils.h>

#include "neg_scenario_test_infra.h"

#define CRYPTO_SUPPORT 1

//#include <drivers/auth/crypto_mod.h>

static fip_toc_entry_t *
find_fiptoc_entry_t(const int fip_base, const uuid_t *uuid)
{
	fip_toc_entry_t *current_file =
		(fip_toc_entry_t *) (fip_base + sizeof(fip_toc_header_t));

	while (!is_uuid_null(&(current_file->uuid))) {
		if (uuid_equal(&(current_file->uuid), uuid))
			return current_file;

		current_file += 1;
	};
	return NULL;
}

static test_result_t test_invalid_rotpk(void)
{
	smc_args args = { SMC_PSCI_SYSTEM_RESET };

	if(tftf_is_rebooted() ){
		/* ROTPK is tampered with and upon reboot tfa should not reach this point */
		return TEST_RESULT_FAIL;
	}

	/* Locate Trusted Key certificate memory address by using UUID */
	const uuid_t trusted_cert = UUID_TRUSTED_KEY_CERT;
	fip_toc_entry_t * cert;
	cert = find_fiptoc_entry_t(PLAT_ARM_FIP_BASE, &trusted_cert);
	int address = cert->offset_address;
	uint8_t cert_buffer[cert->size];

	/* Read certicate into cert_buffer */
	uintptr_t handle;
	plat_get_nvm_handle(&handle);
	io_seek(handle, IO_SEEK_SET, address);
	size_t length= cert->size;
	io_read(handle, (uintptr_t) &cert_buffer, sizeof(cert_buffer), &length);

	/* Parse certifacte to retrieve public key */
	int rc;
	unsigned int img_len = cert->size;
	void * paramOut = NULL;

	rc = get_pubKey_from_cert(&cert_buffer, img_len, &paramOut);

	if (rc != 0){
		tftf_testcase_printf("%d rc VALUE: \n", rc);
	}


	/* Corrupt Public key in certificate */
		int garbage = 0xFFFF;
		size_t garbage_len = sizeof(garbage);
		io_seek(handle, IO_SEEK_SET, address);
		io_write(handle, (uintptr_t) &garbage, garbage_len, &garbage_len);

		/* Reboot */
		tftf_notify_reboot();
		smc_ret_values ret;
		ret = tftf_smc(&args);

		if(ret.ret0 == -1){
			return TEST_RESULT_SKIPPED;
		}

	/* If this point is reached, reboot failed to trigger*/
	return TEST_RESULT_FAIL;
}

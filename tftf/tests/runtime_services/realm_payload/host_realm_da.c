/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include <heap/page_alloc.h>

#include <mmio.h>

#include <spdm.h>
#include <pcie.h>
#include <pcie_doe.h>
#include <pcie_spec.h>

#include <test_helpers.h>

#include <host_pcie.h>
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_shared_data.h>

#include <platform.h>

#define SKIP_TEST_IF_DOE_NOT_SUPPORTED()					\
	do {									\
		if (host_find_doe_device(&bdf, &doe_cap_base) != 0) {		\
			tftf_testcase_printf("PCIe DOE not supported\n");	\
			return TEST_RESULT_SKIPPED;				\
		}								\
	} while (false)

test_result_t host_doe_discovery_test(void)
{
	uint32_t bdf, doe_cap_base;
	int ret;

	host_pcie_init();

	SKIP_TEST_IF_DOE_NOT_SUPPORTED();

	ret = host_doe_discovery(bdf, doe_cap_base);
	if (ret != 0) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

test_result_t host_spdm_version_test(void)
{
	uint32_t bdf, doe_cap_base;
	int ret;

	SKIP_TEST_IF_DOE_NOT_SUPPORTED();

	ret = host_get_spdm_version(bdf, doe_cap_base);
	if (ret != 0) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

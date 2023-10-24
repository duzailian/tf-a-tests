/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Running SMC call for SDEI
 */

#define CMP_SUCCESS 0

void run_sdei_fuzz(char *funcstr)
{
	if (strcmp(funcstr, "sdei_version") == CMP_SUCCESS) {
		long long ret = sdei_version();
		if (ret != MAKE_SDEI_VERSION(1, 0, 0)) {
			tftf_testcase_printf("Unexpected SDEI version: 0x%llx\n",
					     ret);
		}
		printf("running %s\n", funcstr);
	}
	if (strcmp(funcstr, "sdei_pe_unmask") == CMP_SUCCESS) {
		long long ret = sdei_pe_unmask();
		if (ret < 0) {
			tftf_testcase_printf("SDEI pe unmask failed: 0x%llx\n",
					     ret);
		}
		printf("running %s\n", funcstr);
	}
	if (strcmp(funcstr, "sdei_pe_mask") == CMP_SUCCESS) {
		int64_t ret = sdei_pe_mask();
		if (ret < 0) {
			tftf_testcase_printf("SDEI pe mask failed: 0x%llx\n", ret);
		}
		printf("running %s\n", funcstr);
	}
	if (strcmp(funcstr, "sdei_event_status") == CMP_SUCCESS) {
		int64_t ret = sdei_event_status(0);
		if (ret < 0) {
			tftf_testcase_printf("SDEI event status failed: 0x%llx\n",
					     ret);
		}
		printf("running %s\n", funcstr);
	}
	if (strcmp(funcstr, "sdei_event_signal") == CMP_SUCCESS) {
		int64_t ret = sdei_event_signal(0);
		if (ret < 0) {
			tftf_testcase_printf("SDEI event signal failed: 0x%llx\n",
					     ret);
		}
		printf("running %s\n", funcstr);
	}
	if (strcmp(funcstr, "sdei_private_reset") == CMP_SUCCESS) {
		int64_t ret = sdei_private_reset();
		if (ret < 0) {
			tftf_testcase_printf("SDEI private reset failed: 0x%llx\n",
					     ret);
		}
		printf("running %s\n", funcstr);
	}
	if (strcmp(funcstr, "sdei_shared_reset") == CMP_SUCCESS) {
		int64_t ret = sdei_shared_reset();
		if (ret < 0) {
			tftf_testcase_printf("SDEI shared reset failed: 0x%llx\n",
					     ret);
		}
		printf("running %s\n", funcstr);
	}
}

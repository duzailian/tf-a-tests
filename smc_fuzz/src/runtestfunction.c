/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <power_management.h>
#include <sdei.h>
#include <tftf_lib.h>
#include <timer.h>
#include <test_helpers.h>

#define CMP_SUCCESS 0

/*
 * Running SMC call from what function name is selected
 */
void runtestfunction(char *funcstr)
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
	if (strcmp(funcstr, "tsp_add_op") == CMP_SUCCESS) {
		uint64_t fn_identifier = TSP_FAST_FID(TSP_ADD);
		uint64_t addarg1 = 4;
		uint64_t addarg2 = 6;
		smc_args tsp_svc_params = {fn_identifier, addarg1, addarg2};
		smc_ret_values tsp_result;

		tsp_result = tftf_smc(&tsp_svc_params);

		if (tsp_result.ret0) {
			tftf_testcase_printf("TSP operation 0x%x failed, error:0x%x\n",
					(unsigned int) fn_identifier,
					(unsigned int) tsp_result.ret0);
		}
		printf("running %s\n", funcstr);
	}
	if (strcmp(funcstr, "tsp_sub_op") == CMP_SUCCESS) {
		uint64_t fn_identifier = TSP_FAST_FID(TSP_SUB);
		uint64_t subarg1 = 4;
		uint64_t subarg2 = 6;
		smc_args tsp_svc_params = {fn_identifier, subarg1, subarg2};
		smc_ret_values tsp_result;

		tsp_result = tftf_smc(&tsp_svc_params);

		if (tsp_result.ret0) {
			tftf_testcase_printf("TSP operation 0x%x failed, error:0x%x\n",
					(unsigned int) fn_identifier,
					(unsigned int) tsp_result.ret0);
		}
		printf("running %s\n", funcstr);
	}
	if (strcmp(funcstr, "tsp_mul_op") == CMP_SUCCESS) {
		uint64_t fn_identifier = TSP_FAST_FID(TSP_MUL);
		uint64_t mularg1 = 4;
		uint64_t mularg2 = 6;
		smc_args tsp_svc_params = {fn_identifier, mularg1, mularg2};
		smc_ret_values tsp_result;

		tsp_result = tftf_smc(&tsp_svc_params);

		if (tsp_result.ret0) {
			tftf_testcase_printf("TSP operation 0x%x failed, error:0x%x\n",
					(unsigned int) fn_identifier,
					(unsigned int) tsp_result.ret0);
		}
		printf("running %s\n", funcstr);
	}
	if (strcmp(funcstr, "tsp_div_op") == CMP_SUCCESS) {
		uint64_t fn_identifier = TSP_FAST_FID(TSP_DIV);
		uint64_t divarg1 = 4;
		uint64_t divarg2 = 6;
		smc_args tsp_svc_params = {fn_identifier, divarg1, divarg2};
		smc_ret_values tsp_result;

		tsp_result = tftf_smc(&tsp_svc_params);

		if (tsp_result.ret0) {
			tftf_testcase_printf("TSP operation 0x%x failed, error:0x%x\n",
					(unsigned int) fn_identifier,
					(unsigned int) tsp_result.ret0);
		}
		printf("running %s\n", funcstr);
	}
}

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

/*
 * Running SMC call from what function name is selected
 */
void runtestfunction(char *funcstr)
{
	if (strcmp(funcstr, "sdei_version") == 0) {
		long long ret = sdei_version();
		if (ret != MAKE_SDEI_VERSION(1, 0, 0)) {
			tftf_testcase_printf("Unexpected SDEI version: 0x%llx\n",
					     ret);
		}
		printf("running %s\n", funcstr);
	}
	if (strcmp(funcstr, "sdei_pe_unmask") == 0) {
		long long ret = sdei_pe_unmask();
		if (ret < 0) {
			tftf_testcase_printf("SDEI pe unmask failed: 0x%llx\n",
					     ret);
		}
		printf("running %s\n", funcstr);
	}
	if (strcmp(funcstr, "sdei_pe_mask") == 0) {
		int64_t ret = sdei_pe_mask();
		if (ret < 0) {
			tftf_testcase_printf("SDEI pe mask failed: 0x%llx\n", ret);
		}
		printf("running %s\n", funcstr);
	}
	if (strcmp(funcstr, "sdei_event_status") == 0) {
		int64_t ret = sdei_event_status(0);
		if (ret < 0) {
			tftf_testcase_printf("SDEI event status failed: 0x%llx\n",
					     ret);
		}
		printf("running %s\n", funcstr);
	}
	if (strcmp(funcstr, "sdei_event_signal") == 0) {
		int64_t ret = sdei_event_signal(0);
		if (ret < 0) {
			tftf_testcase_printf("SDEI event signal failed: 0x%llx\n",
					     ret);
		}
		printf("running %s\n", funcstr);
	}
	if (strcmp(funcstr, "sdei_private_reset") == 0) {
		int64_t ret = sdei_private_reset();
		if (ret < 0) {
			tftf_testcase_printf("SDEI private reset failed: 0x%llx\n",
					     ret);
		}
		printf("running %s\n", funcstr);
	}
	if (strcmp(funcstr, "sdei_shared_reset") == 0) {
		int64_t ret = sdei_shared_reset();
		if (ret < 0) {
			tftf_testcase_printf("SDEI shared reset failed: 0x%llx\n",
					     ret);
		}
		printf("running %s\n", funcstr);
	}
	if (strcmp(funcstr, "tsp_add_op") == 0) {
		uint64_t fn_identifier = TSP_FAST_FID(TSP_ADD);
		uint64_t arg1 = 4;
		uint64_t arg2 = 6;
		smc_args tsp_svc_params = {fn_identifier, arg1, arg2};
		smc_ret_values tsp_result;

		tsp_result = tftf_smc(&tsp_svc_params);

		if (tsp_result.ret0) {
			tftf_testcase_printf("TSP operation 0x%x failed, error:0x%x\n",
					(unsigned int) fn_identifier,
					(unsigned int) tsp_result.ret0);
		}
		printf("running %s\n", funcstr);
	}
	if (strcmp(funcstr, "tsp_sub_op") == 0) {
		uint64_t fn_identifier = TSP_FAST_FID(TSP_SUB);
		uint64_t arg1 = 4;
		uint64_t arg2 = 6;
		smc_args tsp_svc_params = {fn_identifier, arg1, arg2};
		smc_ret_values tsp_result;

		tsp_result = tftf_smc(&tsp_svc_params);

		if (tsp_result.ret0) {
			tftf_testcase_printf("TSP operation 0x%x failed, error:0x%x\n",
					(unsigned int) fn_identifier,
					(unsigned int) tsp_result.ret0);
		}
		printf("running %s\n", funcstr);
	}
	if (strcmp(funcstr, "tsp_mul_op") == 0) {
		uint64_t fn_identifier = TSP_FAST_FID(TSP_MUL);
		uint64_t arg1 = 4;
		uint64_t arg2 = 6;
		smc_args tsp_svc_params = {fn_identifier, arg1, arg2};
		smc_ret_values tsp_result;

		tsp_result = tftf_smc(&tsp_svc_params);

		if (tsp_result.ret0) {
			tftf_testcase_printf("TSP operation 0x%x failed, error:0x%x\n",
					(unsigned int) fn_identifier,
					(unsigned int) tsp_result.ret0);
		}
		printf("running %s\n", funcstr);
	}
	if (strcmp(funcstr, "tsp_div_op") == 0) {
		uint64_t fn_identifier = TSP_FAST_FID(TSP_DIV);
		uint64_t arg1 = 4;
		uint64_t arg2 = 6;
		smc_args tsp_svc_params = {fn_identifier, arg1, arg2};
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


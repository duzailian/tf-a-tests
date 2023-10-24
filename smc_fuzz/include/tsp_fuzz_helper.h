/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Running SMC call for SDEI
 */

#define CMP_SUCCESS 0

void run_tsp_fuzz(char *funcstr)
{
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

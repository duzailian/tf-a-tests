
/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <host_realm_rmi.h>
#include <lib/aarch64/arch_features.h>
#include <realm_rsi.h>
#include <smccc.h>

static struct rsi_host_call host_cal __aligned(sizeof(struct rsi_host_call));

/* This function return RSI_ABI_VERSION */
u_register_t rsi_get_version(u_register_t req_ver)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args)
		{RSI_ABI_VERSION, req_ver, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL});

	return res.ret1;
}

/* This function will call the Host to request IPA of the NS shared buffer */
u_register_t rsi_get_ns_buffer(void)
{
	smc_ret_values res = {};

	host_cal.imm = HOST_CALL_GET_SHARED_BUFF_CMD;
	res = tftf_smc(&(smc_args) {RSI_HOST_CALL, (u_register_t)&host_cal,
		0UL, 0UL, 0UL, 0UL, 0UL, 0UL});
	if (res.ret0 != RSI_SUCCESS) {
		return 0U;
	}
	return host_cal.gprs[0];
}

/* This function call Host and request to exit Realm with proper exit code */
void rsi_exit_to_host(enum host_call_cmd exit_code, unsigned int rec_num)
{
	host_cal.imm = exit_code;
	host_cal.gprs[0] = rec_num;
	tftf_smc(&(smc_args) {RSI_HOST_CALL, (u_register_t)&host_cal,
		0UL, 0UL, 0UL, 0UL, 0UL, 0UL});
}

u_register_t rsi_ipa_state_set(u_register_t base,
			       u_register_t top,
			       rsi_ripas_type ripas,
			       u_register_t flag,
			       u_register_t *new_base,
			       rsi_ripas_respose_type *response)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args)
			{RSI_IPA_STATE_SET, base, top, ripas, flag});
	if (res.ret0 == RSI_SUCCESS) {
		*new_base = res.ret1;
		*response = res.ret2;
	}
	return res.ret0;
}

u_register_t rsi_ipa_state_get(u_register_t adr, rsi_ripas_type *ripas)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args)
			{RSI_IPA_STATE_GET, adr});
	if (res.ret0 == RSI_SUCCESS) {
		*ripas = res.ret1;
	}
	return res.ret0;
}

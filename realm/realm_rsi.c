
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

u_register_t rsi_attest_token_init(u_register_t c0, u_register_t c1, u_register_t c2,
u_register_t c3, u_register_t c4, u_register_t c5, u_register_t c6, u_register_t c7,
u_register_t *token_size)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args) {RSI_ATTEST_TOKEN_INIT, c0, c1, c2, c3, c4, c5, c6});
	*token_size = res.ret1;
	return res.ret0;
}


u_register_t rsi_attest_token_continue(u_register_t addr,
			       u_register_t offset,
			       rsi_ripas_type buf_size,
			       u_register_t *write_size)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args)
			{RSI_ATTEST_TOKEN_CONTINUE, addr, offset, buf_size});
	if (res.ret0 == RSI_SUCCESS) {
		*write_size = res.ret1;
	}
	return res.ret0;
}

/* This function return RSI_ABI_VERSION */
u_register_t rsi_get_version(u_register_t req_ver)
{
	smc_ret_values res = {};

	res = tftf_smc(&(smc_args)
		{RSI_VERSION, req_ver, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL});

	if (res.ret0 == SMC_UNKNOWN) {
		return SMC_UNKNOWN;
	}
	/* Return lower version. */
	return res.ret1;
}

/* This function will call the Host to request IPA of the NS shared buffer */
u_register_t rsi_get_ns_buffer(void)
{
	smc_ret_values res = {};
	struct rsi_host_call host_cal __aligned(sizeof(struct rsi_host_call));

	host_cal.imm = HOST_CALL_GET_SHARED_BUFF_CMD;
	res = tftf_smc(&(smc_args) {RSI_HOST_CALL, (u_register_t)&host_cal,
		0UL, 0UL, 0UL, 0UL, 0UL, 0UL});
	if (res.ret0 != RSI_SUCCESS) {
		return 0U;
	}
	return host_cal.gprs[0];
}

/* This function call Host and request to exit Realm with proper exit code */
void rsi_exit_to_host(enum host_call_cmd exit_code)
{
	struct rsi_host_call host_cal __aligned(sizeof(struct rsi_host_call));

	host_cal.imm = exit_code;
	host_cal.gprs[0] = read_mpidr_el1() & MPID_MASK;
	tftf_smc(&(smc_args) {RSI_HOST_CALL, (u_register_t)&host_cal,
		0UL, 0UL, 0UL, 0UL, 0UL, 0UL});
}

/* This function will exit to the Host to request RIPAS CHANGE of IPA range */
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

/* This function will return RIPAS of IPA */
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

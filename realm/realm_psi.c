/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <host_realm_rmi.h>
#include <realm_psi.h>
#include <realm_rsi.h>
#include <smccc.h>

extern bool is_plane0;

/* This function will call the Host or P0 to request IPA of the NS shared buffer */
u_register_t realm_get_ns_buffer(void)
{
	if (is_plane0) {
		smc_ret_values res = {};
		struct rsi_host_call host_cal __aligned(sizeof(struct rsi_host_call));

		host_cal.imm = HOST_CALL_GET_SHARED_BUFF_CMD;
		res = tftf_smc(&(smc_args) {RSI_HOST_CALL, (u_register_t)&host_cal,
			0UL, 0UL, 0UL, 0UL, 0UL, 0UL});
		if (res.ret0 != RSI_SUCCESS) {
			return 0U;
		}
		return host_cal.gprs[0];
	} else {
		hvc_ret_values res = tftf_hvc(&(hvc_args) {PSI_CALL_GET_SHARED_BUFF_CMD, 0UL, 0UL,
				0UL, 0UL, 0UL, 0UL, 0UL});

		if (res.ret0 != RSI_SUCCESS) {
			return 0U;
		}
		return res.ret1;
	}
}

void psi_exit_to_plane0(u_register_t psi_cmd)
{
	if (is_plane0) {
		return;
	}
	tftf_hvc(&(hvc_args) {psi_cmd, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL});
}

u_register_t psi_get_plane_id(void)
{
	hvc_ret_values res = tftf_hvc(&(hvc_args) {PSI_CALL_GET_PLANE_ID_CMD, 0UL, 0UL,
			0UL, 0UL, 0UL, 0UL, 0UL});

	if (res.ret0 != RSI_SUCCESS) {
		return (u_register_t)-1;
	}
	return res.ret1;
}


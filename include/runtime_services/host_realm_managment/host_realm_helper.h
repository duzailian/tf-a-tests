/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef HOST_REALM_HELPER_H
#define HOST_REALM_HELPER_H

#include <stdlib.h>
#include <host_realm_rmi.h>
#include <tftf_lib.h>
#include <utils_def.h>

/*
 * Bitmaks for Realm parameter overrides.
 */
#define		REALM_PARAMS_OVERRIDE_IPA_SIZE_SHIFT	0ULL
#define		REALM_PARAMS_OVERRIDE_IPA_SIZE_WIDTH	8ULL
#define		REALM_PARAMS_OVERRIDE_IPA_SIZE(_x)	\
			(INPLACE(REALM_PARAMS_OVERRIDE_IPA_SIZE, _x))

#define		REALM_PARAMS_OVERRIDE_SL_SHIFT		8ULL
#define		REALM_PARAMS_OVERRIDE_SL_WIDTH		8U
#define		REALM_PARAMS_OVERRIDE_SL(_x)		\
			(INPLACE(REALM_PARAMS_OVERRIDE_SL, _x))

#define		REALM_PARAMS_OVERRIDE_SL_EN_SHIFT	9ULL
#define		REALM_PARAMS_OVERRIDE_SL_EN_WIDTH	1U
#define		REALM_PARAMS_OVERRIDE_SL_EN_MASK	\
			(MASK(REALM_PARAMS_OVERRIDE_SL_EN))

bool host_create_realm_payload(u_register_t realm_payload_adr,
		u_register_t plat_mem_pool_adr,
		u_register_t plat_mem_pool_size,
		u_register_t realm_pages_size,
		u_register_t feature_flag,
		u_register_t realm_params_override);
bool host_create_shared_mem(
		u_register_t ns_shared_mem_adr,
		u_register_t ns_shared_mem_size);
bool host_destroy_realm(void);
bool host_enter_realm_execute(uint8_t cmd, struct realm **realm_ptr, int test_exit_reason);
test_result_t host_cmp_result(void);

#endif /* HOST_REALM_HELPER_H */

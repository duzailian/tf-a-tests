/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef REALM_HELPERS_H
#define REALM_HELPERS_H

#include <realm_rsi.h>

/* Generate 64-bit random number */
unsigned long long realm_rand64(void);
bool realm_plane_enter(u_register_t plane_index, u_register_t perm_index,
		u_register_t base, u_register_t flags, rsi_plane_run *run);
u_register_t realm_exit_to_host_as_plane_n(enum host_call_cmd exit_code, u_register_t plane_num);
/* This function will call the Host to request IPA of the NS shared buffer */
u_register_t realm_get_ns_buffer(void);
unsigned int realm_get_my_plane_num(void);
bool realm_is_plane0(void);

#endif /* REALM_HELPERS_H */


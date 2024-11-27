/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef REALM_HELPERS_H
#define REALM_HELPERS_H

/* Generate 64-bit random number */
unsigned long long realm_rand64(void);
bool realm_plane_enter(u_register_t plane_index, u_register_t perm_index,
		u_register_t base, u_register_t flags, rsi_plane_run *run);

#endif /* REALM_HELPERS_H */


/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>

#include <debug.h>
#include <realm_def.h>
#include <realm_helpers.h>

bool test_realm_mec(void)
{
	realm_printf("Executing trivial Realm to test MEC\n");
	return true;
}

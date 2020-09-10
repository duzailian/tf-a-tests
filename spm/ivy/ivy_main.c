/*
 * Copyright (c) 2018-2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <ffa_helpers.h>
#include <sp_helpers.h>
#include <debug.h>

#include "ivy.h"

/* Host machine information injected by the build system in the ELF file. */
extern const char build_message[];
extern const char version_string[];

void __dead2 ivy_main(void)
{
	NOTICE("Entering S-EL0 Secure Partition\n");
	NOTICE("%s\n", build_message);
	NOTICE("%s\n", version_string);

	while (1) {
		svc_args args = {
			.fid = FFA_MSG_WAIT
		};
		sp_svc(&args);
	}
}

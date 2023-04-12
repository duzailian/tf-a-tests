/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SVE_HELPERS_H
#define SVE_HELPERS_H

#define SVE_OP_ARRAYSIZE	1024

#ifndef __ASSEMBLY__
void sve_subtract_interleaved_world_switch(int *sve_op_1, int *sve_op_2,
					   int *sve_op_3,
					   bool (*world_switch_cb)(void));

#endif /* __ASSEMBLY__ */
#endif /* SVE_HELPERS_H */

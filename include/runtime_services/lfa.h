/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LFA_H
#define LFA_H

#include <arm_arch_svc.h>
#include <arch_helpers.h>
#include <smccc.h>
#include <stdint.h>

#define LFA_VERSION                     U(0xC40001E0)
#define LFA_FEATURES                    U(0xC40001E1)
#define LFA_GET_INFO                    U(0xC40001E2)
#define LFA_GET_INVENTORY               U(0xC40001E3)
#define LFA_PRIME                       U(0xC40001E4)
#define LFA_ACTIVATE                    U(0xC40001E5)
#define LFA_CANCEL                      U(0xC40001E6)

#define BL31_X1 			0x4698fe4c6d08d447
#define BL31_X2 			0x005abdcb5029959b

#define RMM_X1				0x564bf212a662076c
#define RMM_X2				0xd90636638fbacb92

#endif /* LFA_H */

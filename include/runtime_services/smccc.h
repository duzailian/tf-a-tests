/*
 * Copyright (c) 2018, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __SMCCC_H__
#define __SMCCC_H__

#include <utils_def.h>

#define SMCCC_VERSION_MAJOR_SHIFT	U(16)
#define SMCCC_VERSION_MAJOR_MASK	U(0x7FFF)
#define SMCCC_VERSION_MINOR_SHIFT	U(0)
#define SMCCC_VERSION_MINOR_MASK	U(0xFFFF)
#define MAKE_SMCCC_VERSION(_major, _minor) \
	((((uint32_t)(_major) & SMCCC_VERSION_MAJOR_MASK) << \
						SMCCC_VERSION_MAJOR_SHIFT) \
	| (((uint32_t)(_minor) & SMCCC_VERSION_MINOR_MASK) << \
						SMCCC_VERSION_MINOR_SHIFT))

#define SMC_UNKNOWN			-1

/* TODO: Import SMCCC 2.0 properly instead of having this */
#define FUNCID_NAMESPACE_SHIFT		U(28)
#define FUNCID_NAMESPACE_MASK		U(0x3)
#define FUNCID_NAMESPACE_WIDTH		U(2)
#define FUNCID_NAMESPACE_SPRT		U(2)
#define FUNCID_NAMESPACE_SPCI		U(3)

/*******************************************************************************
 * Bit definitions inside the function id as per the SMC calling convention
 ******************************************************************************/
#define FUNCID_TYPE_SHIFT		31
#define FUNCID_CC_SHIFT			30
#define FUNCID_OEN_SHIFT		24
#define FUNCID_NUM_SHIFT		0

#define FUNCID_TYPE_MASK		0x1
#define FUNCID_CC_MASK			0x1
#define FUNCID_OEN_MASK			0x3f
#define FUNCID_NUM_MASK			0xffff

#define FUNCID_TYPE_WIDTH		1
#define FUNCID_CC_WIDTH			1
#define FUNCID_OEN_WIDTH		6
#define FUNCID_NUM_WIDTH		16

#define SMC_64				1
#define SMC_32				0
#define SMC_TYPE_FAST			1
#define SMC_TYPE_STD			0

/*******************************************************************************
 * Owning entity number definitions inside the function ID as per the SMC
 * calling convention
 ******************************************************************************/
#define OEN_ARM		0
#define OEN_CPU		1
#define OEN_SIP		2
#define OEN_OEM		3
#define OEN_STD		4  /* Standard secure service. */
#define OEN_STD_HYP	5  /* Standard hypervisor service. */
#define OEN_VENDOR_HYP	6  /* Vendor specific hypervisor service. */

#define OEN_TAP_START	48 /* Trusted Applications */
#define OEN_TAP_END	49

#define OEN_TOS_START	50 /* Trusted OS */
#define OEN_TOS_END	63

#endif /* __SMCCC_H__ */

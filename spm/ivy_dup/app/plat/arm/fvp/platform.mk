#
# Copyright (c) 2021-2022, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

FVP_IVY_DUP_BASE		= spm/ivy_dup/app/plat/arm/fvp

PLAT_INCLUDES		+= -I${FVP_IVY_DUP_BASE}/include/

# Add the FDT source
IVY_DUP_DTS		= ${FVP_IVY_DUP_BASE}/fdts/ivy_dup-sel0.dts

# List of FDTS to copy
FDTS_CP_LIST		=  $(IVY_DUP_DTS)

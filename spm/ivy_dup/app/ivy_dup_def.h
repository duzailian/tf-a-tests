/*
 * Copyright (c) 2018-2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef IVY_DUP_DEF_H
#define IVY_DUP_DEF_H

/*
 * Layout of the Secure Partition image.
 */

/* Up to 2 MiB at an arbitrary address that doesn't overlap the devices. */
#define IVY_DUP_IMAGE_BASE			ULL(0x1000)
#define IVY_DUP_IMAGE_SIZE			ULL(0x200000)

/* Memory reserved for stacks */
#define IVY_DUP_STACKS_SIZE			ULL(0x1000)

/* Memory shared between EL3 and S-EL0 (64 KiB). */
#define IVY_DUP_SPM_BUF_BASE		(IVY_DUP_IMAGE_BASE + IVY_DUP_IMAGE_SIZE)
#define IVY_DUP_SPM_BUF_SIZE		ULL(0x10000)

/* Memory shared between Normal world and S-EL0 (64 KiB). */
#define IVY_DUP_NS_BUF_BASE			(IVY_DUP_SPM_BUF_BASE + IVY_DUP_SPM_BUF_SIZE)
#define IVY_DUP_NS_BUF_SIZE			ULL(0x10000)

/*
 * UUIDs of Secure Services provided by Cactus
 */

#define IVY_DUP_SERVICE1_UUID	U(0x76543210), U(0x89ABCDEF), U(0x76543210), U(0xFEDCBA98)

#define IVY_DUP_SERVICE1_UUID_RD	U(0x76543210) U(0x89ABCDEF) U(0x76543210) U(0xFEDCBA98)

/*
 * Service IDs
 */
/* Print a magic number unique to IVY_DUP and return */
#define IVY_DUP_PRINT_MAGIC			U(1001)
/* Return a magic number unique to IVY_DUP */
#define IVY_DUP_GET_MAGIC			U(1002)
/* Sleep for a number of milliseconds */
#define IVY_DUP_SLEEP_MS			U(1003)

#define IVY_DUP_MAGIC_NUMBER		U(0x97531842)

#endif /* IVY_DUP_DEF_H */

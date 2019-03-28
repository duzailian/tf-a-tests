/*
 * Copyright (c) 2018-2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef QUARK_DEF_H
#define QUARK_DEF_H

/*
 * Layout of the Secure Partition image.
 */

/* Up to 64 KiB at an arbitrary address that doesn't overlap the devices. */
#define QUARK_IMAGE_BASE		ULL(0x00000000)
#define QUARK_IMAGE_SIZE		ULL(0x10000)

/* Memory reserved for stacks */
#define QUARK_STACKS_SIZE		ULL(0x1000)

/* Memory shared between EL3 and S-EL0 (64 KiB). */
#define QUARK_SPM_BUF_BASE		(QUARK_IMAGE_BASE + QUARK_IMAGE_SIZE)
#define QUARK_SPM_BUF_SIZE		ULL(0x10000)

/*
 * UUIDs of Secure Services provided by Cactus
 */

/* Atomic number, atomic weight, melting point (K), boiling point (K) */
#define QUARK_SERVICE1_UUID	U(0x2), U(0x4002602), U(0x095), U(0x4222)

#define QUARK_SERVICE1_UUID_RD	U(0x2) U(0x4002602) U(0x095) U(0x4222)

/*
 * Service IDs
 */
/* Return a magic number unique to QUARK */
#define QUARK_GET_MAGIC		U(2002)

/* Thermal conductivity W/(m·K) */
#define QUARK_MAGIC_NUMBER		U(0x01513)

#endif /* QUARK_DEF_H */

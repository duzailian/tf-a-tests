/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef VAL_RMM_H
#define VAL_RMM_H

/* RSI SMC FID */
#define RSI_MEMORY_DISPOSE		(0xC4000191)
#define RSI_REALM_MEASUREMENT		(0xC4000192)
#define RSI_VERSION			(0xC4000190)

/* RsiDisposeResponse types */
#define RSI_DISPOSE_ACCEPT		0U
#define RSI_DISPOSE_REJECT		1U

/* RsiInterfaceVersion type */
#define RSI_MAJOR_VERSION		9U
#define RSI_MINOR_VERSION		0U

/* RsiStatusCode types */
#define RSI_SUCCESS			0U
#define RSI_ERROR_INPUT			1U

#endif /* VAL_RMM_H */

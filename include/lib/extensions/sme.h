/*
 * Copyright (c) 2021-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SME_H
#define SME_H

#define SME_SMCR_LEN_MAX	U(0x1FF)
#define SME2_ARRAYSIZE		(512/64)
#define SME2_INPUT_DATA		(0x1fffffffffffffff)

typedef enum {
	SMSTART,	/* enters streaming sve mode and enables SME ZA array */
	SMSTART_SM,	/* enters streaming sve mode only */
	SMSTART_ZA,	/* enables SME ZA array storage only */
} smestart_instruction_type_t;

typedef enum {
	SMSTOP,		/* exits streaming sve mode, & disables SME ZA array */
	SMSTOP_SM,	/* exits streaming sve mode only */
	SMSTOP_ZA,	/* disables SME ZA array storage only */
} smestop_instruction_type_t;

/* SME feature related prototypes*/
bool is_feat_sme_supported(void);
bool is_feat_sme_fa64_supported(void);
int sme_enable(void);
void sme_smstart(smestart_instruction_type_t smstart_type);
void sme_smstop(smestop_instruction_type_t smstop_type);

/* SME2 feature related prototypes*/
bool is_feat_sme2_supported(void);
int sme2_enable(void);

/* Assembly function prototypes */
uint64_t sme_rdvl_1(void);
void sme_try_illegal_instruction(void);
void sme2_load_zt0_instruction(const uint64_t *inputbuf);
void sme2_store_zt0_instruction(const uint64_t *outputbuf);

#endif /* SME_H */

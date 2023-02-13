/*
 * Copyright (c) 2021-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SME_H
#define SME_H

#define MAX_VL                  (512)
#define MAX_VL_B                (MAX_VL / 8)
#define SME_SMCR_LEN_MAX	U(0x1FF)

/* SME feature related prototypes */
bool feat_sme_supported(void);
bool feat_sme_fa64_supported(void);
int sme_enable(void);
void sme_smstart(bool enable_za);
void sme_smstop(bool disable_za);

/* Assembly function prototypes */
uint64_t sme_rdvl_1(void);
void sme_try_illegal_instruction(void);
void sme_vector_to_ZA(const uint64_t *input_vector);
void sme_ZA_to_vector(const uint64_t *output_vector);

#endif /* SME_H */

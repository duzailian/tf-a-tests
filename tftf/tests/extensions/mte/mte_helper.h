/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MTE_HELPER_H
#define MTE_HELPER_H

void *mte_insert_random_tag(void *ptr);
void *mte_insert_new_tag(void *ptr);
void *mte_get_tag_address(void *ptr);
void mte_set_tag_address_range(void *ptr, int range);
void mte_clear_tag_address_range(void *ptr, int range);
void mte_disable_pstate_tco(void);
void mte_enable_pstate_tco(void);
unsigned int mte_get_pstate_tco(void);

#endif /* MTE_HELPER_H */

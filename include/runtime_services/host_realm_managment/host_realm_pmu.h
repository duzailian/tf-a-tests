/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef HOST_REALM_PMU_H
#define HOST_REALM_PMU_H

/* PMU physical interrupt */
#define PMU_PPI		23UL

/* PMU virtual interrupt */
#define PMU_VIRQ	PMU_PPI

#define ALL_CLEAR	0x1FFFF

void host_set_pmu_state(void);
bool host_check_pmu_state(void);

#endif /* HOST_REALM_PMU_H */

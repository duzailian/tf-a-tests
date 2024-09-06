/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef HOST_PCIE_H
#define HOST_PCIE_H

void host_pcie_init(void);
pcie_device_bdf_table_t *host_get_pcie_bdf_table();
int host_find_doe_device(uint32_t *bdf_ptr, uint32_t *cap_base_ptr);
int host_doe_discovery(uint32_t bdf, uint32_t doe_cap_base);
int host_get_spdm_version(uint32_t bdf, uint32_t doe_cap_base);
int host_vendor_id(uint32_t bdf, uint32_t doe_cap_base);

#endif /* HOST_PCIE_H */

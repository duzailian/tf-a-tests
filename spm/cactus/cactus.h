/*
 * Copyright (c) 2017-2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __CACTUS_H__
#define __CACTUS_H__

#include <stdint.h>

#include <ffa_helpers.h>

/* Linker symbols used to figure out the memory layout of Cactus. */
extern uintptr_t __TEXT_START__, __TEXT_END__;
#define CACTUS_TEXT_START	((uintptr_t)&__TEXT_START__)
#define CACTUS_TEXT_END		((uintptr_t)&__TEXT_END__)

extern uintptr_t __RODATA_START__, __RODATA_END__;
#define CACTUS_RODATA_START	((uintptr_t)&__RODATA_START__)
#define CACTUS_RODATA_END	((uintptr_t)&__RODATA_END__)

extern uintptr_t __DATA_START__, __DATA_END__;
#define CACTUS_DATA_START	((uintptr_t)&__DATA_START__)
#define CACTUS_DATA_END		((uintptr_t)&__DATA_END__)

extern uintptr_t __BSS_START__, __BSS_END__;
#define CACTUS_BSS_START	((uintptr_t)&__BSS_START__)
#define CACTUS_BSS_END		((uintptr_t)&__BSS_END__)

enum stdout_route {
	UART_AS_STDOUT = 0,
	HVC_CALL_AS_STDOUT,
};

ffa_vcpu_count_t cactus_vcpu_get_count(ffa_vm_id_t vm_id);
ffa_vm_count_t cactus_vm_get_count(void);
void cactus_debug_log(char c);

void set_putc_impl(enum stdout_route);

#endif /* __CACTUS_H__ */

/*
 * Copyright (c) 2017-2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __CACTUS_H__
#define __CACTUS_H__

#include <stdint.h>

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

#if SMC_FUZZING
extern uintptr_t __SMCFUZZ_START__, __SMCFUZZ_END__;
#define SMCFUZZ_SECTION_START	((uintptr_t)&__SMCFUZZ_START__)
#define SMCFUZZ_SECTION_END	((uintptr_t)&__SMCFUZZ_END__)
#endif

#endif /* __CACTUS_H__ */

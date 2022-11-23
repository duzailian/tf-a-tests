/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef RMI_HELPERS_H
#define RMI_HELPERS_H

#include <tftf_lib.h>

#define CONCAT(x, y)	x##y
#define CONC(x, y)	CONCAT(x, y)

#define SET_ARG(_n) {			\
	case _n:			\
	regs[_n] = rand();		\
	CONC(args->arg, _n) = regs[_n];	\
	__attribute__((fallthrough));	\
}

#define	CHECK_RET(_n) {					\
	if (CONC(ret_val.ret, _n) != regs[_n]) {	\
		cmp_flag |= (1U << _n);			\
	}						\
}

#endif /* RMI_HELPERS_H */

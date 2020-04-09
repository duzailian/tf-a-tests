/*
 * Copyright (c) 2020, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


extern int console_pl011_putc(int);

int console_putc(int c)
{
	return console_pl011_putc(c);
}

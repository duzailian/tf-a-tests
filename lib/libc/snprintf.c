/*
 * Copyright (c) 2017-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>

#include <common/debug.h>
#include <plat/common/platform.h>

#define get_unum_va_args(_args, _lcount)				\
	(((_lcount) > 1)  ? va_arg(_args, unsigned long long int) :	\
	(((_lcount) == 1) ? va_arg(_args, unsigned long int) :		\
			    va_arg(_args, unsigned int)))

static void string_print(char **s, size_t n, size_t *chars_printed,
			 const char *str)
{
	while (*str != '\0') {
		if (*chars_printed < n) {
			*(*s) = *str;
			(*s)++;
		}

		(*chars_printed)++;
		str++;
	}
}

static void unsigned_dec_print(char **s, size_t n, size_t *chars_printed,
			       unsigned int unum)
{
	/* Enough for a 32-bit unsigned decimal integer (4294967295). */
	char num_buf[10];
	int i = 0;
	unsigned int rem;

	do {
		rem = unum % 10U;
		num_buf[i++] = '0' + rem;
		unum /= 10U;
	} while (unum > 0U);

	while (--i >= 0) {
		if (*chars_printed < n) {
			*(*s) = num_buf[i];
			(*s)++;
		}

		(*chars_printed)++;
	}
}

static int unsigned_num_print(unsigned long long int unum, unsigned int radix,
			      char padc, int padn)
{
	/* Just need enough space to store 64 bit decimal integer */
	char num_buf[20];
	int i = 0, count = 0;
	unsigned int rem;

	do {
		rem = unum % radix;
		if (rem < 0xa)
			num_buf[i] = '0' + rem;
		else
			num_buf[i] = 'a' + (rem - 0xa);
		i++;
		unum /= radix;
	} while (unum > 0U);

	if (padn > 0) {
		while (i < padn) {
			(void)putchar(padc);
			count++;
			padn--;
		}
	}

	while (--i >= 0) {
		(void)putchar(num_buf[i]);
		count++;
	}

	return count;
}

/*
 * Scaled down version of vsnprintf(3).
 */
int vsnprintf(char *s, size_t n, const char *fmt, va_list args)
{
	int l_count;
	char *str;
	int num;
	unsigned int unum;
	size_t chars_printed = 0U;

	if (n == 0U) {
		/* There isn't space for anything. */
	} else if (n == 1U) {
		/* Buffer is too small to actually write anything else. */
		*s = '\0';
		n = 0U;
	} else {
		/* Reserve space for the terminator character. */
		n--;
	}

	while (*fmt != '\0') {
		l_count = 0;

		if (*fmt == '%') {
			fmt++;
			/* Check the format specifier. */
loop:
			switch (*fmt) {
			case 'i':
			case 'd':
				num = va_arg(args, int);

				if (num < 0) {
					if (chars_printed < n) {
						*s = '-';
						s++;
					}
					chars_printed++;

					unum = (unsigned int)-num;
				} else {
					unum = (unsigned int)num;
				}

				unsigned_dec_print(&s, n, &chars_printed, unum);
				break;
			case 'l':
				l_count++;
				fmt++;
				goto loop;
			case 's':
				str = va_arg(args, char *);
				string_print(&s, n, &chars_printed, str);
				break;
			case 'u':
				unum = va_arg(args, unsigned int);
				unsigned_dec_print(&s, n, &chars_printed, unum);
				break;
			case 'x':
				unum = get_unum_va_args(args, l_count);
				chars_printed += unsigned_num_print(unum, 16,
								    '\0', 0);
				break;
			default:
				/* Panic on any other format specifier. */
				ERROR("snprintf: specifier with ASCII code '%d' not supported.",
				      *fmt);
				panic();
				assert(0); /* Unreachable */
			}
			fmt++;
			continue;
		}

		if (chars_printed < n) {
			*s = *fmt;
			s++;
		}

		fmt++;
		chars_printed++;
	}

	if (n > 0U)
		*s = '\0';

	return (int)chars_printed;
}

/*******************************************************************
 * Reduced snprintf to be used for Trusted firmware.
 * The following type specifiers are supported:
 *
 * %d or %i - signed decimal format
 * %s - string format
 * %u - unsigned decimal format
 *
 * The function panics on all other formats specifiers.
 *
 * It returns the number of characters that would be written if the
 * buffer was big enough. If it returns a value lower than n, the
 * whole string has been written.
 *******************************************************************/
int snprintf(char *s, size_t n, const char *fmt, ...)
{
	va_list args;
	int chars_printed;

	va_start(args, fmt);
	chars_printed = vsnprintf(s, n, fmt, args);
	va_end(args);

	return chars_printed;
}

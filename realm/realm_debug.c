/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <arch_helpers.h>
#include <host_shared_data.h>

/* We save x0-x30. */
#define GPREGS_CNT 31

/* Set of registers saved by the crash_dump() assembly function. */
struct cpu_context {
	u_register_t regs[GPREGS_CNT];
	u_register_t sp;
};

/*
 * Read the EL1 or EL2 version of a register, depending on the current exception
 * level.
 */
#define read_sysreg(_name)			\
	(IS_IN_EL2() ? read_##_name##_el2() : read_##_name##_el1())


/*
 * A printf formatted function used in the Realm world to log messages
 * in the shared buffer.
 * Locate the shared logging buffer and print its content
 */
void realm_printf(const char *fmt, ...)
{
	host_shared_data_t *guest_shared_data = realm_get_shared_structure();
	char *log_buffer = (char *)guest_shared_data->log_buffer;
	va_list args;

	va_start(args, fmt);
	spin_lock((spinlock_t *)&guest_shared_data->printf_lock);
	if (strnlen((const char *)log_buffer, MAX_BUF_SIZE) == MAX_BUF_SIZE) {
		(void)memset((char *)log_buffer, 0, MAX_BUF_SIZE);
	}
	(void)vsnprintf((char *)log_buffer +
			strnlen((const char *)log_buffer, MAX_BUF_SIZE),
			MAX_BUF_SIZE, fmt, args);
	spin_unlock((spinlock_t *)&guest_shared_data->printf_lock);
	va_end(args);
}

void __attribute__((__noreturn__)) do_panic(const char *file, int line)
{
	realm_printf("PANIC in file: %s line: %d\n", file, line);
	while (true) {
		continue;
	}
}

/* This is used from printf() when crash dump is reached */
int console_putc(int c)
{
	host_shared_data_t *guest_shared_data = realm_get_shared_structure();
	char *log_buffer = (char *)guest_shared_data->log_buffer;

	if ((c < 0) || (c > 127)) {
		return -1;
	}
	spin_lock((spinlock_t *)&guest_shared_data->printf_lock);
	if (strnlen((const char *)log_buffer, MAX_BUF_SIZE) == MAX_BUF_SIZE) {
		(void)memset((char *)log_buffer, 0, MAX_BUF_SIZE);
	}
	*((char *)log_buffer + strnlen((const char *)log_buffer, MAX_BUF_SIZE)) = c;
	spin_unlock((spinlock_t *)&guest_shared_data->printf_lock);

	return c;
}
 /* We cannot use tftf/framework/${ARCH}/exception_report.c
  * because it uses platform specific API
  */
void __dead2 print_exception(const struct cpu_context *ctx)
{
	u_register_t mpid = read_mpidr_el1();

	/*
	 * The instruction barrier ensures we don't read stale values of system
	 * registers.
	 */
	isb();
	/* Dump some interesting system registers. */
	printf("System registers:\n");
	printf("  MPIDR=0x%lx\n", mpid);
	printf("  ESR=0x%lx  ELR=0x%lx  FAR=0x%lx\n", read_sysreg(esr),
	       read_sysreg(elr), read_sysreg(far));
	printf("  SCTLR=0x%lx  SPSR=0x%lx  DAIF=0x%lx\n",
	       read_sysreg(sctlr), read_sysreg(spsr), read_daif());

	/* Dump general-purpose registers. */
	printf("General-purpose registers:\n");
	for (int i = 0; i < GPREGS_CNT; ++i) {
		printf("  x%u=0x%lx\n", i, ctx->regs[i]);
	}
	printf("  SP=0x%lx\n", ctx->sp);

	while (1)
		wfi();
}

/*
 * Copyright (c) 2019-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <drivers/console.h>
#include <drivers/arm/pl011.h>
#include <platform_def.h>
#include <stddef.h>
#include <string.h>
#include <tftf_lib.h>
#include <xlat_tables_v2.h>

#include "debugfs.h"

#define SMC_OK			(0)

#define DEBUGFS_VERSION		(0x00000001)
#define MAX_PATH_LEN		(256)

#ifndef __aarch64__
#define DEBUGFS_SMC   0x87000010
#else
#define DEBUGFS_SMC   0xC7000010
#endif


/* DebugFS shared buffer area */
#ifndef PLAT_ARM_DEBUGFS_BASE
#define PLAT_ARM_DEBUGFS_BASE		(0x81000000)
#define PLAT_ARM_DEBUGFS_SIZE		(0x1000)
#endif /* PLAT_ARM_DEBUGFS_BASE */

union debugfs_parms {
	struct {
		char fname[MAX_PATH_LEN];
	} open;

	struct mount {
		char srv[MAX_PATH_LEN];
		char where[MAX_PATH_LEN];
		char spec[MAX_PATH_LEN];
	} mount;

	struct {
		char path[MAX_PATH_LEN];
		dir_t dir;
	} stat;

	struct {
		char oldpath[MAX_PATH_LEN];
		char newpath[MAX_PATH_LEN];
	} bind;
};


// These phrases correspond with phrases sent by a expect script in
// the platform-ci repo
static const char* test_phrases[] = {
	"hello world\n",
	"qwertyuiop\n",
	"asdfghjkl\n",
	"zxcvbnm\n",
	"123456789012345\n",
	"h\n",
	",./_\n",
	"|\n",
	"{}\n",
	//"\\\\\n",
	"<>\n",
	"()\n",
	"=\n",
	"`\n",
	"~\n",
	"?+^*\n",
	"\\-\n",
	"!#%&:;\n",
	"[]\n"
	//"!#$%^&*\n",
	//FIXME: This test case has a bug where it hangs after 18 characters
	//"123456789012345678901234567890",
};

static unsigned int read_buffer[4096 / sizeof(unsigned int)];

static void *const payload = (void *) PLAT_ARM_DEBUGFS_BASE;

static int init(unsigned long long phys_addr)
{
	smc_ret_values ret;
	smc_args args;

	args.fid  = DEBUGFS_SMC;
	args.arg1 = INIT;
	args.arg2 = phys_addr;
	ret = tftf_smc(&args);

	return (ret.ret0 == SMC_OK) ? 0 : -1;
}

static int version(void)
{
	smc_ret_values ret;
	smc_args args;

	args.fid  = DEBUGFS_SMC;
	args.arg1 = VERSION;
	ret = tftf_smc(&args);
	return (ret.ret0 == SMC_OK) ? ret.ret1 : -1;
}

static int open(const char *name, int flags)
{
	union debugfs_parms *parms = payload;
	smc_ret_values ret;
	smc_args args;

	strlcpy(parms->open.fname, name, MAX_PATH_LEN);

	args.fid  = DEBUGFS_SMC;
	args.arg1 = OPEN;
	args.arg2 = (u_register_t) flags;
	ret = tftf_smc(&args);

	return (ret.ret0 == SMC_OK) ? ret.ret1 : -1;
}

static int read(int fd, void *buf, size_t size)
{
	smc_ret_values ret;
	smc_args args;

	args.fid  = DEBUGFS_SMC;
	args.arg1 = READ;
	args.arg2 = (u_register_t) fd;
	args.arg3 = (u_register_t) size;

	ret = tftf_smc(&args);

	if (ret.ret0 == SMC_OK) {
		memcpy(buf, payload, size);
		return ret.ret1;
	}

	return -1;
}

static int write(int fd, void *buf, size_t size)
{
	smc_ret_values ret;
	smc_args args;

	//Debugfs internally copies this struct out of the shared buffer so
	//size of write must be limited
	if((buf == NULL) || (size > sizeof(union debugfs_parms))) {
		return -1;
	}
	memcpy(payload, buf, size);

	args.fid  = DEBUGFS_SMC;
	args.arg1 = WRITE;
	args.arg2 = (u_register_t) fd;
	args.arg3 = (u_register_t) size;
	ret = tftf_smc(&args);

	if (ret.ret0 == SMC_OK) {
		return ret.ret1;
	}

	return -1;
}

static int close(int fd)
{
	smc_ret_values ret;
	smc_args args;

	args.fid  = DEBUGFS_SMC;
	args.arg1 = CLOSE;
	args.arg2 = (u_register_t) fd;

	ret = tftf_smc(&args);

	return (ret.ret0 == SMC_OK) ? 0 : -1;
}

static int mount(char *srv, char *where, char *spec)
{
	union debugfs_parms *parms = payload;
	smc_ret_values ret;
	smc_args args;

	strlcpy(parms->mount.srv, srv, MAX_PATH_LEN);
	strlcpy(parms->mount.where, where, MAX_PATH_LEN);
	strlcpy(parms->mount.spec, spec, MAX_PATH_LEN);

	args.fid  = DEBUGFS_SMC;
	args.arg1 = MOUNT;

	ret = tftf_smc(&args);

	return (ret.ret0 == SMC_OK) ? 0 : -1;
}

/*
 * @Test_Aim@ Test the TF-A console driver via exposing it through the
 * debugfs interface
 */
test_result_t test_getc(void)
{
	int ret, fd, len;

	// Get debugfs interface version (if implemented)
	ret = version();
	if (ret != DEBUGFS_VERSION) {
		// Likely debugfs feature is not implemented
		return TEST_RESULT_SKIPPED;
	}

	// Initialize debugfs feature, this maps the NS shared buffer in SWd
	ret = init(PLAT_ARM_DEBUGFS_BASE);
	if (ret != 0) {
		return TEST_RESULT_FAIL;
	}

	// Mount the console driver
	ret = mount("#C", "/dev/console", "");
	if (ret < 0) {
		tftf_testcase_printf("mount failed ret=%d\n", ret);
		return TEST_RESULT_FAIL;
	}

	// Open console for reading and writing
	fd = open("/dev/console", O_RDWR);
	if (fd < 0) {
		tftf_testcase_printf("open failed fd=%d\n", fd);
		return TEST_RESULT_FAIL;
	}

	// It is undefined for the console driver to read or write negative
	// numbers of bytes
	ret = read(fd, read_buffer, -10);
	if(ret != -1) {
		tftf_testcase_printf("Read did not fail with negative length ret=%d\n", ret);
		return TEST_RESULT_FAIL;
	}

	ret = write(fd, read_buffer, -10);
	if(ret != -1) {
		tftf_testcase_printf("Write did not fail with negative length ret=%d\n", ret);
		return TEST_RESULT_FAIL;
	}

	// The console driver should not accept reads over one page as that
	// would overflow the shared buffer
	ret = read(fd, read_buffer, 4097);
	if(ret != -1) {
		tftf_testcase_printf("Read did not fail with a length greater than one page ret=%d\n", ret);
		return TEST_RESULT_FAIL;
	}

	ret = read(fd, read_buffer, 0);
	if(ret != 0) {
		tftf_testcase_printf("Read for zero bytes did not return 0 ret=%d\n", ret);
		return TEST_RESULT_FAIL;
	}

	ret = write(fd, read_buffer, 0);
	if(ret != 0) {
		tftf_testcase_printf("Write for zero bytes did not return 0 ret=%d\n", ret);
		return TEST_RESULT_FAIL;
	}

	const char* msg = "Starting console getc tests!\n";
	// Null terminator is not accounted for since this should only output
	// the specified number of characters
	len = strlen(msg);
	memcpy(read_buffer, msg, len);
	ret = write(fd, read_buffer, len);
	if(ret != len) {
		tftf_testcase_printf("write failed ret=%d\n", ret);
		return TEST_RESULT_FAIL;
	}

	const int n_phrases = sizeof(test_phrases) / sizeof(test_phrases[0]);
	for(int i = 0; i < n_phrases; ++i) {
		len = strlen(test_phrases[i]);
		ret = read(fd, read_buffer, len);
		if(ret != len) {
			tftf_testcase_printf("read failed expected %d ret=%d\n", len, ret);
			return TEST_RESULT_FAIL;
		}

		if(strncmp(test_phrases[i], (char*)read_buffer, len) != 0) {
			((char*)read_buffer)[len] = '\0';
			tftf_testcase_printf("Read returned wrong value. Expected %s got: %s", test_phrases[i], (char*)read_buffer);
			return TEST_RESULT_FAIL;
		}

		//((char*)read_buffer)[len] = '\n';
		// accounting for newline
		//++len;
		ret = write(fd, read_buffer, len);
		if(ret != len) {
			tftf_testcase_printf("write failed expected %d ret=%d\n", len, ret);
			return TEST_RESULT_FAIL;
		}
	}

	/*
	//FIXME: This has a strange bug where after 18 characters
	//getc starts returning 2145 until 238th byte and then hangs

	// Reading a full page of the character 'a'
	len = 4096;
	ret = read(fd, read_buffer, len);
	if(ret != len) {
		tftf_testcase_printf("read failed expected %d ret=%d\n", len, ret);
		return TEST_RESULT_FAIL;
	}

	for(int i = 0; i < len; ++i) {
		if(((char*)read_buffer)[i] != 'a') {
			tftf_testcase_printf("page read has wrong character at index %d expected a got %d\n", i, ((char*)read_buffer)[i]);
			return TEST_RESULT_FAIL;
		}
	}
	*/

	/*
	const char* done_msg = "Done with getc tests!\n";
	len = strlen(done_msg);
	memcpy(read_buffer, done_msg, len);
	ret = write(fd, read_buffer, len);
	if(ret != len) {
		tftf_testcase_printf("write failed ret=%d\n", ret);
		return TEST_RESULT_FAIL;
	}
	*/

	ret = close(fd);
	if(ret != 0) {
		tftf_testcase_printf("close failed ret=%d\n", ret);
		return TEST_RESULT_FAIL;
	}

	/*
	//This works to print to the console
	console_init(PLAT_ARM_UART_BASE, PLAT_ARM_UART_CLK_IN_HZ,
		     PL011_BAUDRATE);
	console_putc('H');
	console_putc('e');
	console_putc('l');
	console_putc('l');
	console_putc('o');
	console_putc('!');
	console_putc('\n');
	*/

	return TEST_RESULT_SUCCESS;
}

#include <fwu_nvm.h>
#include <drivers/io/io_driver.h>
#include <drivers/io/io_fip.h>
#include <io_storage.h>
#include <platform.h>
#include <psci.h>
#include <smccc.h>
#include <status.h>
#include <tftf_lib.h>
#include "tbb_test_infra.h"

test_result_t test_corrupt_boot_fip(unsigned int offset) {
	smc_args args = { SMC_PSCI_SYSTEM_RESET };
	smc_ret_values ret = {0};
	unsigned int flag = 0xDEADBEEF;
	size_t written = 0;
	uintptr_t dev_handle;
	int result;

	if (tftf_is_rebooted()) {
		/* FIP succesfully repaired */
		return TEST_RESULT_SUCCESS;
	}

	/* Corrupt the FIP at the provided offset */
	plat_get_nvm_handle(&dev_handle);
	result = io_seek(dev_handle, IO_SEEK_SET, offset);
	test_assert(result == IO_SUCCESS);
	result = io_write(dev_handle, (uintptr_t) &flag, sizeof(flag), &written);
	test_assert(result == IO_SUCCESS);
	test_assert(written == sizeof(flag));

	/*
         * Now reboot the system.
         * On the next boot, EL3 firmware should notice and repair the corruption
         * before re-entering TFTF
         */

	tftf_notify_reboot();
	psci_system_reset();
	return TEST_RESULT_FAIL;
}

/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <debug.h>
#include <tftf_lib.h>
#include <sdei.h>
#include <drivers/arm/arm_gic.h>

/*
 * Only this many events can be bound in the PPI range. If you attempt to bind
 * more, an error code should be returned.
 */
#define MAX_EVENTS_IN_PPI_RANGE		U(3)

/*
 * Test that it doesn't crash when you attempt to bind more events in the PPI
 * range than are available.
 */
test_result_t test_sdei_bind_failure(void)
{
	int64_t ret, ev;
	struct sdei_intr_ctx intr_ctx;
	int intr;
	bool bind_failed, last_call;

	ret = sdei_version();
	if (ret != MAKE_SDEI_VERSION(1, 0, 0)) {
		printf("%d: Unexpected SDEI version: 0x%llx\n",
			__LINE__, (unsigned long long) ret);
		return TEST_RESULT_SKIPPED;
	}

	for (intr = MIN_PPI_ID; intr < (MIN_PPI_ID + MAX_EVENTS_IN_PPI_RANGE + 1); intr++) {
		printf("Binding to interrupt %d.\n", intr);

		ev = sdei_interrupt_bind(intr, &intr_ctx);

		/*
		 * Every bind should succeed except the last one, which should
		 * fail.
		 */
		bind_failed = ev < 0;
		last_call = intr == (MIN_PPI_ID + MAX_EVENTS_IN_PPI_RANGE);
		if (bind_failed != last_call) {
			printf("%d: SDEI interrupt bind; ret=%lld; Bind should "
			       "have %s\n",
			       __LINE__,
			       ev,
			       last_call ? "failed" : "succeeded");
			return TEST_RESULT_FAIL;
		}
	}

	return TEST_RESULT_SUCCESS;
}

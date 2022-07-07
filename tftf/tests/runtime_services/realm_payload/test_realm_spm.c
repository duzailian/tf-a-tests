/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdlib.h>

#include <cactus_test_cmds.h>
#include <ffa_endpoints.h>
#include <ffa_helpers.h>
#include <host_realm_helper.h>
#include <host_realm_mem_layout.h>
#include <host_shared_data.h>
#include <lib/extensions/fpu.h>
#include <lib/extensions/sve.h>
#include <test_helpers.h>

#define SIMD_NS_VALUE			0x11U
fpu_reg_state_t fpu_temp_ns;
struct sve_state sve_ns;
#define SVE_Z_NS_VALUE			0x44U
#define SVE_P_NS_VALUE			0x77U
#define SVE_FFR_NS_VALUE		0xaaU

#define REALM_TIME_SLEEP		300U
#define SENDER HYP_ID
#define RECEIVER SP_ID			(1)

typedef enum realm_test_cmd {
	CMD_SIMD_NS_FILL = 0U,
	CMD_SIMD_NS_CMP,
	CMD_SIMD_SEC_FILL,
	CMD_SIMD_SEC_CMP,
	CMD_SIMD_RL_FILL,
	CMD_SIMD_RL_CMP,
	CMD_SVE_NS_FILL,
	CMD_SVE_NS_CMP,
	CMD_SVE_SEC_FILL,
	CMD_SVE_SEC_CMP,
	CMD_SVE_RL_FILL,
	CMD_SVE_RL_CMP,
	CMD_MAX_COUNT
} realm_test_cmd_t;

#ifdef __aarch64__
static const struct ffa_uuid expected_sp_uuids[] = { {PRIMARY_UUID} };
static struct mailbox_buffers mb;
static bool secure_mailbox_initialised;

/*
 * This function helps to Initialise secure_mailbox, creates realm payload and
 * shared memory to be used between Host and Realm.
 * Skip test if RME is not supported or not the right RMM version is begin used
 */
static test_result_t init_test(void)
{
	u_register_t retrmm;


	/* Verify that FFA is there and that it has the correct version. */
	SKIP_TEST_IF_AARCH32();
	SKIP_TEST_IF_FFA_VERSION_LESS_THAN(1, 1);

	if (!secure_mailbox_initialised) {
		GET_TFTF_MAILBOX(mb);
		CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);
		secure_mailbox_initialised = true;
	}

	if (get_armv9_2_feat_rme_support() == 0U) {
		return TEST_RESULT_SKIPPED;
	}

	retrmm = rmi_version();
	VERBOSE("RMM version is: %lu.%lu\n",
			RMI_ABI_VERSION_GET_MAJOR(retrmm),
			RMI_ABI_VERSION_GET_MINOR(retrmm));
	/*
	 * Skip the test if RMM is TRP, TRP version is always null.
	 */
	if (retrmm == 0UL) {
		return TEST_RESULT_SKIPPED;
	}

	/*
	 * Initialise Realm payload
	 */
	if (!host_create_realm_payload((u_register_t)REALM_IMAGE_BASE,
			(u_register_t)PAGE_POOL_BASE,
			(u_register_t)(PAGE_POOL_MAX_SIZE +
			NS_REALM_SHARED_MEM_SIZE),
			(u_register_t)PAGE_POOL_MAX_SIZE)) {
		return TEST_RESULT_FAIL;
	}

	/*
	 * Create shared memory between Host and Realm
	 */
	if (!host_create_shared_mem(NS_REALM_SHARED_MEM_BASE,
			NS_REALM_SHARED_MEM_SIZE)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
}

 /* Send request to SP to fill FPU/SIMD regs with secure template values */
static bool fpu_fill_sec(void)
{
	struct ffa_value ret = cactus_req_simd_fill_send_cmd(SENDER, RECEIVER);

	if (!is_ffa_direct_response(ret)) {
		ERROR("%s failed %d\n", __func__, __LINE__);
		return false;
	}
	if (cactus_get_response(ret) == CACTUS_ERROR) {
		ERROR("%s failed %d\n", __func__, __LINE__);
		return false;
	}
	return true;
}

/* Send request to SP to compare FPU/SIMD regs with secure template values */
static bool fpu_cmp_sec(void)
{
	struct ffa_value ret = cactus_req_simd_compare_send_cmd(SENDER, RECEIVER);

	if (!is_ffa_direct_response(ret)) {
		ERROR("%s failed %d\n", __func__, __LINE__);
		return false;
	}
	if (cactus_get_response(ret) == CACTUS_ERROR) {
		ERROR("%s failed %d\n", __func__, __LINE__);
		return false;
	}
	return true;
}


/* Send request to Realm to fill FPU/SIMD regs with realm template values */
static bool fpu_fill_rl(void)
{
	if (!host_enter_realm_execute(REALM_REQ_FPU_FILL_CMD)) {
		ERROR("%s failed %d\n", __func__, __LINE__);
		return false;
	}
	return true;
}

static bool sve_fill_rl(void)
{
	if (!host_enter_realm_execute(REALM_REQ_SVE_FILL_CMD)) {
		ERROR("%s failed %d\n", __func__, __LINE__);
		return false;
	}
	return true;
}

#endif
/*
 * @Test_Aim@ Test secure interrupt handling while Secure Partition is in waiting
 * state and Realm world runs a busy loop at R-EL1.
 *
 * 1. Send a direct message request command to first Cactus SP to start the
 *    trusted watchdog timer.
 *
 * 2. Once the SP returns with a direct response message, it moves to WAITING
 *    state.
 *
 * 3. Create and execute a busy loop to sleep the PE in the realm world for
 *    REALM_TIME_SLEEP ms.
 *
 * 4. Trusted watchdog timer expires during this time which leads to secure
 *    interrupt being triggered while cpu is executing in realm world.
 *
 * 5. Realm EL1 exits to host, but because the FIQ is still pending,
 *    the Host will be pre-empted to EL3.
 *
 * 6. The interrupt is trapped to BL31/SPMD as FIQ and later synchronously
 *    delivered to SPM.
 *
 * 7. SPM injects a virtual IRQ to first Cactus Secure Partition.
 *
 * 8. Once the SP has handled the interrupt, it returns execution back to normal
 *    world using FFA_MSG_WAIT call.
 *
 * 9. Normal world parses REC's exit reason (FIQ in this case) and resumes
 *    REC by re-entering the realm world
 *
 * 10. We make sure the time elapsed in the sleep routine is not less than
 *    the requested value.
 *
 * 11. TFTF sends a direct request message to SP to query the ID of last serviced
 *     secure virtual interrupt.
 *
 * 12. Further, TFTF expects SP to return the ID of Trusted Watchdog timer
 *     interrupt through a direct response message.
 *
 * 13. Test finishes successfully once the TFTF disables the trusted watchdog
 *     interrupt through a direct message request command.
 *
 */
test_result_t test_sec_interrupt_can_preempt_rl(void)
{
#ifdef __aarch64__
	uint64_t time1;
	volatile uint64_t time2, time_lapsed;
	uint64_t timer_freq = read_cntfrq_el0();
	struct ffa_value ret_values;
	test_result_t res;

	res = init_test();
	if (res != TEST_RESULT_SUCCESS) {
		return res;
	}

	/* Enable trusted watchdog interrupt as IRQ in the secure side. */
	if (!enable_trusted_wdog_interrupt(SENDER, RECEIVER)) {
		return TEST_RESULT_FAIL;
	}

	/*
	 * Send a message to SP1 through direct messaging.
	 */
	ret_values = cactus_send_twdog_cmd(SENDER, RECEIVER, (REALM_TIME_SLEEP/2));

	if (!is_ffa_direct_response(ret_values)) {
		ERROR("Expected a direct response for starting TWDOG timer\n");
		return TEST_RESULT_FAIL;
	}
	time1 = syscounter_read();

	/*
	 * Spin Realm payload for REALM_TIME_SLEEP ms, This ensures secure wdog
	 * timer triggers during this time.
	 */
	realm_shared_data_set_host_val(HOST_SLEEP_INDEX, REALM_TIME_SLEEP);
	if (!host_enter_realm_execute(REALM_SLEEP_CMD)) {
		ERROR("realm_enter_and_sleep error\n");
		host_destroy_realm();
		return TEST_RESULT_FAIL;
	}
	if (!host_destroy_realm()) {
		ERROR("host_destroy_realm error\n");
		return TEST_RESULT_FAIL;
	}

	time2 = syscounter_read();

	/* Lapsed time should be at least equal to sleep time */
	time_lapsed = ((time2 - time1) * 1000) / timer_freq;

	if (time_lapsed < REALM_TIME_SLEEP) {
		ERROR("Time elapsed less than expected value: %llu vs %u\n",
				time_lapsed, REALM_TIME_SLEEP);
		return TEST_RESULT_FAIL;
	}

	/* Check for the last serviced secure virtual interrupt. */
	ret_values = cactus_get_last_interrupt_cmd(SENDER, RECEIVER);

	if (!is_ffa_direct_response(ret_values)) {
		ERROR("Expected a direct response for last serviced interrupt"
			" command\n");
		return TEST_RESULT_FAIL;
	}

	/* Make sure Trusted Watchdog timer interrupt was serviced*/
	if (cactus_get_response(ret_values) != IRQ_TWDOG_INTID) {
		ERROR("Trusted watchdog timer interrupt not serviced by SP\n");
		return TEST_RESULT_FAIL;
	}

	/* Disable Trusted Watchdog interrupt. */
	if (!disable_trusted_wdog_interrupt(SENDER, RECEIVER)) {
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
#else
	return TEST_RESULT_SKIPPED;
#endif
}

/*
 * Test that FPU/SIMD state are preserved during a randomly context switch
 * between secure/non-secure/realm(R-EL1)worlds.
 * FPU/SIMD state consist of the 32 SIMD vectors, FPCR and FPSR registers,
 * the test runs for 1000 iterations with random combination of:
 * SECURE_FILL_FPU, SECURE_READ_FPU, REALM_FILL_FPU, REALM_READ_FPU,
 * NONSECURE_FILL_FPU, NONSECURE_READ_FPU commands,to test all possible situations
 * of synchronous context switch between worlds, while the content of those registers
 * is being used.
 */
test_result_t fpu_concurrent_in_rl_ns_se(void)
{
#ifdef __aarch64__
	int cmd = -1, old_cmd  = -1;
	test_result_t res;

	res = init_test();
	if (res != TEST_RESULT_SUCCESS) {
		return res;
	}

	/*
	 * Fill all 3 world's FPU/SIMD state regs with some known values in the
	 * beginning to have something later to compare to.
	 */
	fpu_state_write_template(SIMD_NS_VALUE);
	if (!fpu_fill_sec()) {
		ERROR("fpu_fill_sec error\n");
		goto destroy_realm;
	}
	if (!fpu_fill_rl()) {
		ERROR("fpu_fill_rl error\n");
		goto destroy_realm;
	}

	for (uint32_t i = 0; i < 1000; i++) {
		cmd = rand() % CMD_MAX_COUNT;
		if (cmd == old_cmd) {
			continue;
		}
		old_cmd = cmd;

		switch (cmd) {
		case CMD_SIMD_NS_FILL:
			/* Non secure world fill FPU/SIMD state registers */
			fpu_temp_ns = fpu_state_write_template(SIMD_NS_VALUE);
			break;
		case CMD_SIMD_NS_CMP:
			/* Normal world verify its FPU/SIMD state registers data */
			if (!fpu_state_compare_template(&fpu_temp_ns)) {
				ERROR("%s failed %d\n", __func__, __LINE__);
				goto destroy_realm;
			}
			break;
		case CMD_SIMD_SEC_FILL:
			/* secure world fill FPU/SIMD state registers */
			if (!fpu_fill_sec()) {
				goto destroy_realm;
			}
			break;
		case CMD_SIMD_SEC_CMP:
			/* Secure world verify its FPU/SIMD state registers data */
			if (!fpu_cmp_sec()) {
				goto destroy_realm;
			}
			break;
		case CMD_SIMD_RL_FILL:
			/* Realm R-EL1 world fill FPU/SIMD state registers */
			if (!fpu_fill_rl()) {
				goto destroy_realm;
			}
			break;
		case CMD_SIMD_RL_CMP:
			/* Realm R-EL1 world verify its FPU/SIMD state registers data */
			if (!host_enter_realm_execute(REALM_REQ_FPU_FILL_CMD)) {
				ERROR("%s failed %d\n", __func__, __LINE__);
				goto destroy_realm;
			}
			break;
		default:
			break;
		}
	}

	if (!host_destroy_realm()) {
		ERROR("host_destroy_realm error\n");
		return TEST_RESULT_FAIL;
	}
	return TEST_RESULT_SUCCESS;
destroy_realm:
	host_destroy_realm();
	return TEST_RESULT_FAIL;
#else
	return TEST_RESULT_SKIPPED;
#endif
}

/*
 * Test that SVE state are preserved during a randomly context switch between
 * secure/non-secure/realm(R-EL1)worlds.
 *
 * SVE state consist of 32 scalable vector registers named Z0-Z31, 16 scalable
 * predicate registers named P0-P15, First Fault Register named FFR,
 * Vector length  ZCR_EL1/ZCR_EL2, FPCR/FPSR registers.
 *
 * The test run for 1000 iterations with random combination of:
 * SECURE_FILL_FPU, SECURE_READ_FPU,REALM_FILL_SVE, REALM_READ_SVE,
 * NONSECURE_FILL_SVE, NONSECURE_READ_SVE, to test all possible situations
 * of synchronous context switch between worlds, while the content of those registers
 * is being used.
 */
test_result_t sve_concurrent_in_rl_ns_se(bool secure, bool realm)
{
#ifdef __aarch64__
	int cmd = -1, old_cmd  = -1;
	uint32_t max_sve_vl;
	test_result_t res;

	res = init_test();
	if (res != TEST_RESULT_SUCCESS) {
		return res;
	}

	/* Get the implemented VL. */
	max_sve_vl = CONVERT_SVE_LENGTH(8*sve_vector_length_get());

	realm_shared_data_clear_realm_val();
	/*
	 * Set the implemented SVE/VL value in the shared buffer to be used later
	 * by the R-EL1 payload test
	 */
	realm_shared_data_set_host_val(HOST_SVE_VL_INDEX, max_sve_vl);

	/* Fill all 3 worlds SVE state regs with some known template values
	 * in the beginning to have something later to compare to.
	 */
	sve_ns = sve_state_write_template(SVE_Z_NS_VALUE,
			SVE_P_NS_VALUE,
			max_sve_vl,
			max_sve_vl,
			SVE_FFR_NS_VALUE);

	/*Fill secure world SVE registers*/
	if (!fpu_fill_sec()) {
		ERROR("fpu_fill_sec error\n");
		goto destroy_realm;
	}

	/*Fill R-EL1 world SVE registers*/
	if (!sve_fill_rl()) {
		ERROR("sve_fill_rl error\n");
		goto destroy_realm;
	}
	for (uint32_t i = 0; i < 1000; i++) {
		cmd = rand() % CMD_MAX_COUNT;
		if (cmd == old_cmd)
			continue;
		old_cmd = cmd;

		switch (cmd) {
		case CMD_SVE_NS_FILL:
			/* Non secure world fill SVE state registers */
			sve_ns = sve_state_write_template(SVE_Z_NS_VALUE,
					SVE_P_NS_VALUE,
					max_sve_vl,
					max_sve_vl,
					SVE_FFR_NS_VALUE);
			break;
		case CMD_SVE_NS_CMP:
			/* Normal world verify its SVE state registers */
			if (!sve_state_compare_template(&sve_ns)) {
				ERROR("%s failed %d\n", __func__, __LINE__);
				goto destroy_realm;
			}
			break;
		case CMD_SIMD_SEC_FILL:
			/* secure world fill FPU/SIMD state registers
			 * (no SVE support in SPs)
			 */
			if (!fpu_fill_sec()) {
				goto destroy_realm;
			}
			break;
		case CMD_SIMD_SEC_CMP:
			/* Secure world verify its FPU/SIMD state registers
			 * (no SVE support in SPs)
			 */
			if (!fpu_cmp_sec()) {
				goto destroy_realm;
			}
			break;
		case CMD_SVE_RL_FILL:
			/* Realm R-EL1 world fill SVE state registers */
			if (!sve_fill_rl()) {
				goto destroy_realm;
			}
			break;
		case CMD_SVE_RL_CMP:
			/* Realm R-EL1 world verify its SVE state registers */
			if (!host_enter_realm_execute(REALM_REQ_SVE_CMP_CMD)) {
				ERROR("%s failed %d\n", __func__, __LINE__);
				goto destroy_realm;
			}
			break;
		default:
			break;
		}
	}

	if (!host_destroy_realm()) {
		ERROR("host_destroy_realm error\n");
		return TEST_RESULT_FAIL;
	}
	return TEST_RESULT_SUCCESS;
destroy_realm:
	host_destroy_realm();
	return TEST_RESULT_FAIL;
#else
	return TEST_RESULT_SKIPPED;
#endif
}

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
#include <test_helpers.h> 
#include <lib/extensions/fpu.h> 

#define SIMD_NS_VALUE 			0x11U
#define FPCR_NS_VALUE 			0x7FF9F00U
#define FPSR_NS_VALUE 			0xF800009FU

#define REALM_TIME_SLEEP		300U
#define SENDER HYP_ID
#define RECEIVER SP_ID			(1)
static const struct ffa_uuid expected_sp_uuids[] = { {PRIMARY_UUID} };
static struct mailbox_buffers mb;
static bool secure_mailbox_initialised;

typedef enum realm_test_cmd {
	CMD_SIMD_NS_FILL = 0U,
	CMD_SIMD_NS_CMP,
	CMD_SIMD_SEC_FILL,
	CMD_SIMD_SEC_CMP,
	CMD_SIMD_RL_FILL,
	CMD_SIMD_RL_CMP,
	CMD_MAX_COUNT
} realm_test_cmd_t;

/*
 * This function helps to Initialise secure_mailbox, creates realm payload and
 * shared memory to be used between Host and Realm.
 * Skip test if RME is not supported or not the right RMM version is begin used
 */
test_result_t init_test(void)
{
	u_register_t retrmm;

	if (!secure_mailbox_initialised) {
	/* Verify that FFA is there and that it has the correct version. */
		INIT_TFTF_MAILBOX(mb);
		CHECK_SPMC_TESTING_SETUP(1, 1, expected_sp_uuids);
		GET_TFTF_MAILBOX(mb);
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

bool fpu_fill_sec(void)
{
	struct ffa_value ret = cactus_req_simd_fill_send_cmd(SENDER, RECEIVER);
	if (!is_ffa_direct_response(ret)) {
		ERROR("fpu_fill_sec failed %d\n", __LINE__);
		return false;
	}
	if (cactus_get_response(ret) == CACTUS_ERROR) {
		ERROR("fpu_fill_sec failed %d\n", __LINE__);
		return false;
	}
	return true;
}

bool fpu_fill_rl(void)
{
	if(!host_enter_realm_execute(REALM_REQ_FPU_FILL_CMD)) {
		ERROR("realm_create_enter_and_fill_simd error\n");
		return false;
	}
	return true;
}

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
}

/*
 * Tests that FPU state(SIMD vectors, FPCR, FPSR) are preserved during the
 * context switches between normal world and the realm world.
 * Fills the FPU state regs with known values, requests Realm at R-EL1 
 * to fill the vectors with a different values, 
 * checks that the context is restored on return.
 */
test_result_t fpu_concurrent_in_rl_ns_se(void)
{
	struct ffa_value ret;
	int cmd = -1, old_cmd  = -1;
	test_result_t res;

	res = init_test();
	if(res != TEST_RESULT_SUCCESS) {
		return res;
	}

	fpu_reg_state_t fpu_state_send, fpu_state_receive;

	/* SIMD_NS_VALUE is just a dummy value to be distinguished from the value in the
	 * secure world. 
	 */
	fpu_state_set(&fpu_state_send, SIMD_NS_VALUE, FPCR_NS_VALUE, FPSR_NS_VALUE);
	//read_fpu_state_registers(&fpu_state_receive);

	/* Fill all 3 worlds FPU/SIMD state regs with some known values in the beginning
	 * to have something later to compare to
	 */
	fill_fpu_state_registers(&fpu_state_send);
	if(!fpu_fill_sec()) {
		ERROR("fpu_fill_sec error\n");
		goto destroy_realm;
	}
	if(!fpu_fill_rl()) {
		ERROR("fpu_fill_rl error\n");
		goto destroy_realm;
	}

	for (uint32_t i = 0; i < 1000; i++)
	{
		cmd = rand() % 6;
		if(cmd == old_cmd)
			continue;
		old_cmd = cmd;
		//INFO("fpu_concurrent_in_rl_ns_se cmd:%d\n", cmd);
		switch (cmd)
		{
			case CMD_SIMD_NS_FILL:
				/* Non secure world fill FPU/SIMD state registers */
				fill_fpu_state_registers(&fpu_state_send);
				break;
			case CMD_SIMD_NS_CMP:
				/* Normal world verify its FPU/SIMD state registers data */
				read_fpu_state_registers(&fpu_state_receive);
				if (memcmp((uint8_t *)&fpu_state_send,
						(uint8_t *)&fpu_state_receive,
						sizeof(fpu_reg_state_t)) != 0) {
					ERROR("fpu_concurrent_in_rl_ns_se failed %d\n", __LINE__);
					goto destroy_realm;
				}
				break;
			case CMD_SIMD_SEC_FILL:
				/* secure world fill FPU/SIMD state registers */
				if(!fpu_fill_sec()) {
					ERROR("fpu_fill_sec error\n");
					goto destroy_realm;
				}
				break;
			case CMD_SIMD_SEC_CMP:
				/* Secure world verify its FPU/SIMD state registers data */
				ret = cactus_req_simd_compare_send_cmd(SENDER, RECEIVER);
				if (!is_ffa_direct_response(ret)) {
					ERROR("fpu_concurrent_in_rl_ns_se failed %d\n", __LINE__);
					goto destroy_realm;
				}
				if (cactus_get_response(ret) == CACTUS_ERROR) {
					ERROR("fpu_concurrent_in_rl_ns_se failed %d\n", __LINE__);
					goto destroy_realm;
				}
				break;
			case CMD_SIMD_RL_FILL:
				/* Realm R-EL1 world fill FPU/SIMD state registers */
				if(!fpu_fill_rl()) {
					ERROR("fpu_fill_rl error\n");
					goto destroy_realm;
				}
				break;
			case CMD_SIMD_RL_CMP:
				/* Realm R-EL1 world verify its FPU/SIMD state registers data */
				if(!host_enter_realm_execute(REALM_REQ_FPU_FILL_CMD)) {
					ERROR("realm_enter_compare_simd error\n");
					goto destroy_realm;
				}
				break;
			default:
				break;
		}
	}

	if(!host_destroy_realm()) {
		ERROR("host_destroy_realm error\n");
		return TEST_RESULT_FAIL;
	}
	return TEST_RESULT_SUCCESS;
destroy_realm:
	host_destroy_realm();
	return TEST_RESULT_FAIL;
}

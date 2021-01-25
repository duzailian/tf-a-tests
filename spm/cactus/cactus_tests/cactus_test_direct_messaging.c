/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "cactus_test_cmds.h"
#include <debug.h>
#include <ffa_helpers.h>

CACTUS_CMD_HANDLER(echo_cmd, CACTUS_ECHO_CMD)
{
	uint64_t echo_val = cactus_echo_get_val(*args);

	VERBOSE("Received echo at %x, value %llx.\n", ffa_dir_msg_dest(*args),
						      echo_val);

	return echo_val;
}

CACTUS_CMD_HANDLER(req_echo_cmd, CACTUS_REQ_ECHO_CMD)
{
	smc_ret_values ffa_ret;
	ffa_vm_id_t vm_id = ffa_dir_msg_dest(*args);
	ffa_vm_id_t echo_dest = cactus_req_echo_get_echo_dest(*args);
	uint64_t echo_val = cactus_echo_get_val(*args);

	VERBOSE("%x requested to send echo to %x, value %llx\n",
		ffa_dir_msg_source(*args), echo_dest, echo_val);

	ffa_ret = cactus_echo_send_cmd(vm_id, echo_dest, echo_val);

	if (ffa_func_id(ffa_ret) != FFA_MSG_SEND_DIRECT_RESP_SMC32) {
		ERROR("Failed to send message. error: %lx\n",
			ffa_ret.ret2);
		return CACTUS_ERROR;
	}

	if (cactus_get_response(ffa_ret) != echo_val) {
		ERROR("Echo Failed!\n");
		return CACTUS_ERROR;
	}

	return CACTUS_SUCCESS;
}

static uint32_t base_deadlock_handler(ffa_vm_id_t vm_id,
				      ffa_vm_id_t deadlock_dest,
				      ffa_vm_id_t deadlock_next_dest)
{
	smc_ret_values ffa_ret;

	ffa_ret = cactus_deadlock_send_cmd(vm_id, deadlock_dest,
					   deadlock_next_dest);

	/*
	 * Should be true for the last partition to attempt
	 * an FF-A direct message, to the first partition.
	 */
	bool is_deadlock_detected = (ffa_func_id(ffa_ret) == FFA_ERROR) &&
				    (ffa_ret.ret2 == FFA_ERROR_BUSY);

	/*
	 * Should be true after the deadlock has been detected and after the
	 * first response has been sent down the request chain.
	 */
	bool is_returning_from_deadlock =
		(ffa_func_id(ffa_ret) == FFA_MSG_SEND_DIRECT_RESP_SMC32) &&
		(cactus_get_response(ffa_ret) == CACTUS_SUCCESS);

	if (is_deadlock_detected) {
		NOTICE("Attempting dealock but got error %lx\n",
			ffa_ret.ret2);
	}

	if (is_deadlock_detected || is_returning_from_deadlock) {
		/*
		 * This is not the partition, that would have created the
		 * deadlock. As such, reply back to the partitions.
		 */
		return CACTUS_SUCCESS;
	}

	/* Shouldn't get to this point */
	ERROR("Deadlock test went wrong!\n");
	return CACTUS_ERROR;
}

CACTUS_CMD_HANDLER(deadlock_cmd, CACTUS_DEADLOCK_CMD)
{
	ffa_vm_id_t source = ffa_dir_msg_source(*args);
	ffa_vm_id_t deadlock_dest = cactus_deadlock_get_next_dest(*args);
	ffa_vm_id_t deadlock_next_dest = source;

	VERBOSE("%x is creating deadlock. next: %x\n", source, deadlock_dest);

	return base_deadlock_handler(ffa_dir_msg_dest(*args), deadlock_dest,
				     deadlock_next_dest);
}

CACTUS_CMD_HANDLER(req_deadlock_cmd, CACTUS_REQ_DEADLOCK_CMD)
{
	ffa_vm_id_t vm_id = ffa_dir_msg_dest(*args);
	ffa_vm_id_t deadlock_dest = cactus_deadlock_get_next_dest(*args);
	ffa_vm_id_t deadlock_next_dest = cactus_deadlock_get_next_dest2(*args);;

	VERBOSE("%x requested deadlock with %x and %x\n",
		ffa_dir_msg_source(*args), deadlock_dest, deadlock_next_dest);

	return base_deadlock_handler(vm_id, deadlock_dest, deadlock_next_dest);
}

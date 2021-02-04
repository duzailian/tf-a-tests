/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CACTUS_TEST_CMDS
#define CACTUS_TEST_CMDS

#include <debug.h>
#include <ffa_helpers.h>

/**
 * Success and error return to be sent over a msg response.
 */
#define CACTUS_SUCCESS		U(0)
#define CACTUS_ERROR		U(-1)
#define CACTUS_INVALID		U(-2)

/**
 * Get command from struct smc_ret_values.
 */
static inline uint64_t cactus_get_cmd(smc_ret_values ret)
{
	return (uint64_t)ret.ret3;
}

/**
 * Template for commands to be sent to CACTUS partitions over direct
 * messages interfaces.
 */
static inline smc_ret_values cactus_send_cmd(
	ffa_vm_id_t source, ffa_vm_id_t dest, uint64_t cmd, uint64_t val0,
	uint64_t val1, uint64_t val2, uint64_t val3)
{
	return 	ffa_msg_send_direct_req64_5args(source, dest, cmd, val0, val1,
						val2, val3);
}

#define PRINT_CMD(smc_ret)						\
	VERBOSE("cmd %lx; args: %lx, %lx, %lx, %lx\n",	 		\
		smc_ret.ret3, smc_ret.ret4, smc_ret.ret5, 		\
		smc_ret.ret6, smc_ret.ret7)

/**
 * Basic test command for direct message in coming from NWd.
 * The response will be the "payload | receiver FF-A id".
 *
 * The id is the hex representation of the string 'dirmsg'.
 */
#define CACTUS_DIR_MSG_TEST_CMD U(0x6469726d7367)

static inline uint32_t cactus_dir_msg_test_payload(smc_ret_values ret)
{
	return (uint32_t)ret.ret4;
}

static inline smc_ret_values cactus_dir_msg_test_send_cmd(
	ffa_vm_id_t source, ffa_vm_id_t receiver, uint32_t payload)
{
	return cactus_send_cmd(source, receiver, CACTUS_DIR_MSG_TEST_CMD,
			       payload, 0, 0, 0);
}

/**
 * With this test command the sender transmits a 64-bit value that it then
 * expects to receive on the respective command response.
 *
 * The id is the hex representation of the string 'echo'.
 */
#define CACTUS_ECHO_CMD U(0x6563686f)

static inline smc_ret_values cactus_echo_send_cmd(
	ffa_vm_id_t source, ffa_vm_id_t dest, uint64_t echo_val)
{
	return cactus_send_cmd(source, dest, CACTUS_ECHO_CMD, echo_val, 0, 0,
			       0);
}

static inline uint64_t cactus_echo_get_val(smc_ret_values ret)
{
	return (uint64_t)ret.ret4;
}

/**
 * Command to request a cactus secure partition to send an echo command to
 * another partition.
 *
 * The sender of this command expects to receive CACTUS_SUCCESS if the requested
 * echo interaction happened successfully, or CACTUS_ERROR otherwise.
 */
#define CACTUS_REQ_ECHO_CMD (CACTUS_ECHO_CMD + 1)

static inline smc_ret_values cactus_req_echo_send_cmd(
	ffa_vm_id_t source, ffa_vm_id_t dest, ffa_vm_id_t echo_dest,
	uint64_t echo_val)
{
	return cactus_send_cmd(source, dest, CACTUS_REQ_ECHO_CMD, echo_val,
			       echo_dest, 0, 0);
}

static inline ffa_vm_id_t cactus_req_echo_get_echo_dest(smc_ret_values ret)
{
	return (ffa_vm_id_t)ret.ret5;
}

/**
 * Command to create a cyclic dependency between SPs, which could result in
 * a deadlock. This aims at proving such scenario cannot happen.
 * If the deadlock happens, the system will just hang.
 * If the deadlock is prevented, the last partition to use the command will
 * send response CACTUS_SUCCESS.
 *
 * The id is the hex representation of the string 'dead'.
 */
#define CACTUS_DEADLOCK_CMD U(0x64656164)

static inline smc_ret_values cactus_deadlock_send_cmd(
	ffa_vm_id_t source, ffa_vm_id_t dest, ffa_vm_id_t next_dest)
{
	return cactus_send_cmd(source, dest, CACTUS_DEADLOCK_CMD, next_dest, 0,
			       0, 0);
}

static inline ffa_vm_id_t cactus_deadlock_get_next_dest(smc_ret_values ret)
{
	return (ffa_vm_id_t)ret.ret4;
}

/**
 * Command to request a sequence CACTUS_DEADLOCK_CMD between the partitions
 * of specified IDs.
 */
#define CACTUS_REQ_DEADLOCK_CMD (CACTUS_DEADLOCK_CMD + 1)

static inline smc_ret_values cactus_req_deadlock_send_cmd(
	ffa_vm_id_t source, ffa_vm_id_t dest, ffa_vm_id_t next_dest1,
	ffa_vm_id_t next_dest2)
{
	return cactus_send_cmd(source, dest, CACTUS_REQ_DEADLOCK_CMD,
			       next_dest1, next_dest2, 0, 0);
}

/* To get next_dest1 use CACTUS_DEADLOCK_GET_NEXT_DEST */
static inline ffa_vm_id_t cactus_deadlock_get_next_dest2(smc_ret_values ret)
{
	return (ffa_vm_id_t)ret.ret5;
}

/**
 * Command to notify cactus of a memory management operation. The cmd value
 * should be the memory management smc function id.
 *
 * The id is the hex representation of the string "mem"
 */
#define CACTUS_MEM_SEND_CMD U(0x6d656d)

static inline smc_ret_values cactus_mem_send_cmd(
	ffa_vm_id_t source, ffa_vm_id_t dest, uint32_t mem_func,
	ffa_memory_handle_t handle)
{
	return cactus_send_cmd(source, dest, CACTUS_MEM_SEND_CMD, mem_func,
			       handle, 0, 0);
}

static inline ffa_memory_handle_t cactus_mem_send_get_handle(smc_ret_values ret)
{
	return (ffa_memory_handle_t)ret.ret5;
}

/**
 * Command to request a memory management operation. The 'mem_func' argument
 * identifies the operation that is to be performend, and 'receiver' is the id
 * of the partition to receive the memory region.
 *
 * The command id is the hex representation of the string "memory".
 */
#define CACTUS_REQ_MEM_SEND_CMD U(0x6d656d6f7279)

static inline smc_ret_values cactus_req_mem_send_send_cmd(
	ffa_vm_id_t source, ffa_vm_id_t dest, uint32_t mem_func,
	ffa_vm_id_t receiver)
{
	return cactus_send_cmd(source, dest, CACTUS_REQ_MEM_SEND_CMD, mem_func,
			       receiver, 0, 0);
}

static inline uint32_t cactus_req_mem_send_get_mem_func(smc_ret_values ret)
{
	return (uint32_t)ret.ret4;
}

static inline ffa_vm_id_t cactus_req_mem_send_get_receiver(smc_ret_values ret)
{
	return (ffa_vm_id_t)ret.ret5;
}

/**
 * Template for responses to CACTUS commands.
 */
static inline smc_ret_values cactus_response(
	ffa_vm_id_t source, ffa_vm_id_t dest, uint32_t response)
{
	return ffa_msg_send_direct_resp(source, dest, response);
}

static inline smc_ret_values cactus_success_resp(
		ffa_vm_id_t source, ffa_vm_id_t dest)
{
	return cactus_response(source, dest, CACTUS_SUCCESS);
}

static inline smc_ret_values cactus_error_resp(
		ffa_vm_id_t source, ffa_vm_id_t dest)
{
	return cactus_response(source, dest, CACTUS_ERROR);
}

static inline uint32_t cactus_get_response(smc_ret_values ret)
{
	return (uint32_t)ret.ret3;
}

/**
 * Pairs a command id with a function call, to handle the command ID	.
 */
struct cactus_cmd_handler {
	const uint64_t id;
	uint32_t (*fn)(const smc_ret_values*, struct mailbox_buffers*);
};

/**
 * Helper to create the name of a handler function.
 */
#define CACTUS_HANDLER_FN_NAME(name) cactus_##name##_handler

/**
 * Define handler's function signature.
 */
#define CACTUS_HANDLER_FN(name)						\
	static uint32_t CACTUS_HANDLER_FN_NAME(name)(			\
		const smc_ret_values *args, struct mailbox_buffers *mb)

/**
 * Helper to define Cactus command handler, and pair it with a command ID.
 * It also creates a table with this information, to be traversed by
 * 'cactus_handle_cmd' function.
 */
#define CACTUS_CMD_HANDLER(name, ID)					\
	CACTUS_HANDLER_FN(name);					\
	struct cactus_cmd_handler name __section(".cactus_handler") = {	\
		.id = ID, .fn = CACTUS_HANDLER_FN_NAME(name),		\
	};								\
	CACTUS_HANDLER_FN(name)

bool cactus_handle_cmd(smc_ret_values *cmd_args, smc_ret_values *ret,
		       struct mailbox_buffers *mb);
#endif

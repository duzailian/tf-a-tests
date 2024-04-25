/*
 * Copyright (c) 2023-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fuzz_names.h>
#include <ffa_fuzz_helper.h>
#include "constraint.h"
#include <arg_struct_def.h>
#include <runtime_services/spm_common.h>
#include <runtime_services/cactus_test_cmds.h>
#include <runtime_services/ffa_endpoints.h>

#define PRIMARY_LO ffa_get_uuid_lo((struct ffa_uuid){PRIMARY_UUID})
#define PRIMARY_HI ffa_get_uuid_hi((struct ffa_uuid){PRIMARY_UUID})
#define SECONDARY_LO ffa_get_uuid_lo((struct ffa_uuid){SECONDARY_UUID})
#define SECONDARY_HI ffa_get_uuid_hi((struct ffa_uuid){SECONDARY_UUID})
#define TERTIARY_LO ffa_get_uuid_lo((struct ffa_uuid){TERTIARY_UUID})
#define TERTIARY_HI ffa_get_uuid_hi((struct ffa_uuid){TERTIARY_UUID})

void inputparameters_to_ffa_value(struct inputparameters ip, struct ffa_value* args)
{
	args->arg1 = ip.x1;
	args->arg2 = ip.x2;
	args->arg3 = ip.x3;
	args->arg4 = ip.x4;
	args->arg5 = ip.x5;
	args->arg6 = ip.x6;
	args->arg7 = ip.x7;
	args->arg8 = ip.x8;
	args->arg9 = ip.x9;
	args->arg10 = ip.x10;
	args->arg11 = ip.x12;
	args->arg12 = ip.x12;
	args->arg13 = ip.x13;
	args->arg14 = ip.x14;
	args->arg15 = ip.x15;
	args->arg16 = ip.x16;
	args->arg17 = ip.x17;

}

static struct ffa_value ffa_call_with_params(struct inputparameters ip, uint64_t ffa_function_id) {
	struct ffa_value args = {.fid = ffa_function_id};
	inputparameters_to_ffa_value(ip, &args);

	return ffa_service_call(&args);
}

static bool is_info_get_regs_valid (struct inputparameters ip) {
	uint64_t uuid_lo = ip.x1;
	uint64_t uuid_hi = ip.x2;
	uint64_t start = get_generated_value(FFA_PARTITION_INFO_GET_REGS_CALL_ARG3_START, ip);
	uint64_t tag = get_generated_value(FFA_PARTITION_INFO_GET_REGS_CALL_ARG3_TAG, ip);

	if ((uuid_lo == PRIMARY_LO && uuid_hi == PRIMARY_HI) ||
		(uuid_lo == SECONDARY_LO && uuid_hi == SECONDARY_HI) ||
		(uuid_lo == TERTIARY_LO && uuid_hi == TERTIARY_HI)){
		return (start == 0 && tag == 0);
	}else if (uuid_lo == 0 && uuid_hi == 0){
		if (start == 0){
			return (tag == 0);
		}else{
			return (start < 4);
		}
	}
	return false;
}

void run_ffa_fuzz(int funcid, struct memmod *mmod)
{
	if (funcid == ffa_id_get_funcid) {
		struct ffa_value args = {.fid = FFA_ID_GET};
		struct ffa_value ret = ffa_service_call(&args);
		if (ret.fid == FFA_ERROR){
			printf("FAIL error code %d\n", ffa_error_code(ret));
		}
	}else if (funcid == ffa_partition_info_get_regs_funcid) {

		/*TODO a way to pair the x1/x2 args so that UUID input is cohesive? */
		uint64_t uuid_lo_values[] = {0, PRIMARY_LO, SECONDARY_LO, TERTIARY_LO};
		uint64_t uuid_hi_values[] = {0, PRIMARY_HI, SECONDARY_HI, TERTIARY_HI};

		setconstraint(FUZZER_CONSTRAINT_VECTOR, uuid_lo_values, 2, FFA_PARTITION_INFO_GET_REGS_CALL_ARG1_UUID_LO, mmod, FUZZER_CONSTRAINT_EXCMODE);
		setconstraint(FUZZER_CONSTRAINT_VECTOR, uuid_hi_values, 2, FFA_PARTITION_INFO_GET_REGS_CALL_ARG2_UUID_HI, mmod, FUZZER_CONSTRAINT_EXCMODE);

		struct inputparameters ip = generate_args(FFA_PARTITION_INFO_GET_REGS_CALL, SMC_FUZZ_SANITY_LEVEL);
		struct ffa_value args = {0};
		inputparameters_to_ffa_value(ip, &args);
		args.fid = FFA_PARTITION_INFO_GET_REGS_SMC64;

		struct ffa_value ret = ffa_service_call(&args);

		if (is_info_get_regs_valid(ip) && ret.fid == FFA_ERROR){
			printf("FAIL error code %d\n", ffa_error_code(ret));
		}
	}else if (funcid == ffa_version_funcid) {
		struct inputparameters ip = generate_args(FFA_VERSION_CALL, SMC_FUZZ_SANITY_LEVEL);
		struct ffa_value ret = ffa_call_with_params(ip, FFA_VERSION);
		uint32_t version = ret.fid;

		if(version == FFA_ERROR_NOT_SUPPORTED){
			printf("FFA_VERSION_NOT_SUPPORTED\n");
		}
	}else if (funcid == ffa_msg_send_direct_req_funcid) {
		uint64_t receiver_ids[] = {SP_ID(1), SP_ID(2), SP_ID(3)};
		uint64_t sender_id[] = {HYP_ID};
		uint64_t message_type[] = {0, 1};
		uint64_t frmwrk_msg_type[] = {0,1};
		uint64_t msg_0_input[] = {CACTUS_ECHO_CMD, CACTUS_REQ_ECHO_CMD}; /* Cactus cmd */
		uint64_t msg_1_input[] = {100,200}; /* for echo cmds, echo_val */

		setconstraint(FUZZER_CONSTRAINT_SVALUE, sender_id, 1, FFA_MSG_SEND_DIRECT_REQ_CALL_ARG1_SENDER_ID, mmod, FUZZER_CONSTRAINT_EXCMODE);
		setconstraint(FUZZER_CONSTRAINT_VECTOR, receiver_ids, 3, FFA_MSG_SEND_DIRECT_REQ_CALL_ARG1_RECEIVER_ID, mmod, FUZZER_CONSTRAINT_EXCMODE);
		setconstraint(FUZZER_CONSTRAINT_VECTOR, message_type, 2, FFA_MSG_SEND_DIRECT_REQ_CALL_ARG2_MESSAGE_TYPE, mmod, FUZZER_CONSTRAINT_EXCMODE);
		setconstraint(FUZZER_CONSTRAINT_VECTOR, frmwrk_msg_type, 2, FFA_MSG_SEND_DIRECT_REQ_CALL_ARG2_FRMWK_MSG_TYPE, mmod, FUZZER_CONSTRAINT_EXCMODE);
		setconstraint(FUZZER_CONSTRAINT_VECTOR, msg_0_input, 2, FFA_MSG_SEND_DIRECT_REQ_CALL_ARG3_MSG_0, mmod, FUZZER_CONSTRAINT_EXCMODE);
		setconstraint(FUZZER_CONSTRAINT_RANGE, msg_1_input, 2, FFA_MSG_SEND_DIRECT_REQ_CALL_ARG4_MSG_1, mmod, FUZZER_CONSTRAINT_EXCMODE);

		struct inputparameters ip = generate_args(FFA_MSG_SEND_DIRECT_REQ_CALL, SMC_FUZZ_SANITY_LEVEL);
		struct ffa_value ret = ffa_call_with_params(ip, FFA_MSG_SEND_DIRECT_REQ_SMC64);

		if (ret.fid == FFA_MSG_SEND_DIRECT_RESP_SMC64) {
			printf("Received direct response, ret.arg4 = %ld\n", ret.arg4);
		}else if (ret.fid == FFA_ERROR) {
			printf("Direct request returned with FFA_ERROR code %d\n", ffa_error_code(ret));
		}else if (ret.fid == FFA_MSG_YIELD) {
			printf("FFA_MSG_SEND_DIRECT_REQ returned with FFA_MSG_YIELD\n");
		}else if (ret.fid == FFA_INTERRUPT){
			printf("FFA_MSG_SEND_DIRECT_REQ returned with FFA_INTERRUPT\n");
		}else{
			printf("FAIL FFA_MSG_SEND_DIRECT_REQ returned with 0x%lx\n", ret.fid);
		}
	}else if (funcid == ffa_msg_send_direct_resp_funcid) {
		uint64_t receiver_ids[] = {SP_ID(1), SP_ID(2), SP_ID(3)};
		uint64_t sender_id[] = {HYP_ID};

		setconstraint(FUZZER_CONSTRAINT_SVALUE, sender_id, 1, FFA_MSG_SEND_DIRECT_RESP_CALL_ARG1_SENDER_ID, mmod, FUZZER_CONSTRAINT_EXCMODE);
		setconstraint(FUZZER_CONSTRAINT_VECTOR, receiver_ids, 3, FFA_MSG_SEND_DIRECT_RESP_CALL_ARG1_RECEIVER_ID, mmod, FUZZER_CONSTRAINT_EXCMODE);

		struct inputparameters ip = generate_args(FFA_MSG_SEND_DIRECT_RESP_CALL, SMC_FUZZ_SANITY_LEVEL);
		struct ffa_value ret = ffa_call_with_params(ip, FFA_MSG_SEND_DIRECT_RESP_SMC64);

		/* NWd cannot send a direct response. */
		if (ret.fid == FFA_ERROR) {
			printf("Direct response returned with FFA_ERROR code %d\n", ffa_error_code(ret));
		}else{
			printf("FAIL FFA_MSG_SEND_DIRECT_RESP returned with 0x%lx\n", ret.fid);
		}
	}else if (funcid == ffa_features_feat_id_funcid) {
		/* Allow feature_ids in range of 8bits */
		uint64_t feat_ids_range[] = {0x0, 0xFF};

		setconstraint(FUZZER_CONSTRAINT_RANGE, feat_ids_range, 2, FFA_FEATURES_FEAT_ID_CALL_ARG1_FEAT_ID, mmod, FUZZER_CONSTRAINT_EXCMODE);

		struct inputparameters ip = generate_args(FFA_FEATURES_FEAT_ID_CALL, SMC_FUZZ_SANITY_LEVEL);
		struct ffa_value ret = ffa_call_with_params(ip, FFA_FEATURES);
		printf("ret.fid: 0x%lx\n", ret.fid);
	}else if (funcid == ffa_features_func_id_funcid) {
		uint64_t ffa_funcid_range[] = {0x84000060, 0x8400008C};

		setconstraint(FUZZER_CONSTRAINT_RANGE, ffa_funcid_range, 2, FFA_FEATURES_FUNC_ID_CALL_ARG1_FUNC_ID, mmod, FUZZER_CONSTRAINT_EXCMODE);

		struct inputparameters ip = generate_args(FFA_FEATURES_FUNC_ID_CALL, SMC_FUZZ_SANITY_LEVEL);

		/*TODO good use case for one field constraint dependent on value of another generated field */
		uint64_t funcid_gen_input = get_generated_value(FFA_FEATURES_FUNC_ID_CALL_ARG1_FUNC_ID, ip);
		if (SMC_FUZZ_SANITY_LEVEL == SANITY_LEVEL_3 && funcid_gen_input == FFA_MEM_RETRIEVE_REQ_SMC32) {
			// Set arg2 to a certain value
			ip.x2 = 0x10;
			//would be more interesting if could set a constraint and regenerate just that arg
			//TODO if I set a constraint in this if and used generate_field_con directly, would that constraint remain on next run? not desired
		}

		struct ffa_value ret = ffa_call_with_params(ip, FFA_FEATURES);
		printf("ret.fid: 0x%lx\n", ret.fid);
	}else if (funcid == ffa_run_funcid) {
		uint64_t target_ids[] = {SP_ID(1), SP_ID(2), SP_ID(3)};
		uint64_t target_vcpu_ids[] = {0,8};
		setconstraint(FUZZER_CONSTRAINT_VECTOR, target_ids, 3, FFA_RUN_CALL_ARG1_TARGET_VM_ID, mmod, FUZZER_CONSTRAINT_EXCMODE);
		setconstraint(FUZZER_CONSTRAINT_RANGE, target_vcpu_ids, 2, FFA_RUN_CALL_ARG1_TARGET_VCPU_ID, mmod, FUZZER_CONSTRAINT_EXCMODE);
		struct inputparameters ip = generate_args(FFA_RUN_CALL, SMC_FUZZ_SANITY_LEVEL);
		struct ffa_value ret = ffa_call_with_params(ip, FFA_RUN);

		switch(ret.fid){
			case FFA_ERROR:
				printf("FFA_RUN returned with FFA_ERROR code %d\n", ffa_error_code(ret));
				break;
			case FFA_INTERRUPT:
			case FFA_MSG_WAIT:
			case FFA_MSG_YIELD:
			case FFA_MSG_SEND_DIRECT_RESP_SMC64:
			case FFA_MSG_SEND_DIRECT_RESP_SMC32:
				printf("FFA_RUN successfully returned with 0x%lx\n", ret.fid);
				break;
			default:
				printf("FAIL FFA_RUN returned with 0x%lx\n", ret.fid);
		}

	}else if (funcid == ffa_notification_bitmap_create_funcid) {
		uint64_t max_vm_id = (1 << 16) - 1;
		uint64_t vm_id[] = {0, max_vm_id};
		uint64_t n_vcpus[] = {0,16};
		setconstraint(FUZZER_CONSTRAINT_RANGE, n_vcpus, 2, FFA_NOTIFICATION_BITMAP_CREATE_CALL_ARG2_N_VCPUS, mmod, FUZZER_CONSTRAINT_EXCMODE);
		setconstraint(FUZZER_CONSTRAINT_RANGE, vm_id, 2, FFA_NOTIFICATION_BITMAP_CREATE_CALL_ARG1_VM_ID, mmod, FUZZER_CONSTRAINT_EXCMODE);
		struct inputparameters ip = generate_args(FFA_NOTIFICATION_BITMAP_CREATE_CALL, SMC_FUZZ_SANITY_LEVEL);
		struct ffa_value ret = ffa_call_with_params(ip, FFA_NOTIFICATION_BITMAP_CREATE);
	
		uint64_t gen_vm_id = get_generated_value(FFA_NOTIFICATION_BITMAP_CREATE_CALL_ARG1_VM_ID, ip);
		uint64_t gen_n_vcpus = get_generated_value(FFA_NOTIFICATION_BITMAP_CREATE_CALL_ARG2_N_VCPUS, ip);
	
		if(ret.fid == FFA_SUCCESS_SMC32 || ret.fid == FFA_SUCCESS_SMC64){
			printf("FFA_NOTIFICATION_BITMAP_CREATE succeeded for VM %lld with vCPU count %lld\n", gen_vm_id, gen_n_vcpus);
		}else if (ret.fid == FFA_ERROR){
			printf("FFA_NOTIFICATION_BITMAP_CREATE returned with FFA_ERROR code %d\n", ffa_error_code(ret));
		}else{
			printf("FAIL FFA_NOTIFICATION_BITMAP_CREATE returned with 0x%lx\n", ret.fid);
		}
		
	}else if (funcid == ffa_notification_bind_funcid) {
		uint64_t sps[] = {SP_ID(1), SP_ID(2), SP_ID(3)};
		uint64_t max_vm_id = (1 << 16) - 1;
		uint64_t vm_ids[] = {0, max_vm_id};
		uint64_t max_bitmap_value = 0xFFFFFFFF;
		uint64_t bitmap_range[] = {0, max_bitmap_value};

		setconstraint(FUZZER_CONSTRAINT_RANGE, vm_ids, 2, FFA_NOTIFICATION_BIND_CALL_ARG1_RECEIVER_ID, mmod, FUZZER_CONSTRAINT_ACCMODE);
		setconstraint(FUZZER_CONSTRAINT_RANGE, vm_ids, 2, FFA_NOTIFICATION_BIND_CALL_ARG1_SENDER_ID, mmod, FUZZER_CONSTRAINT_ACCMODE);
		setconstraint(FUZZER_CONSTRAINT_VECTOR, sps, 3, FFA_NOTIFICATION_BIND_CALL_ARG1_RECEIVER_ID, mmod, FUZZER_CONSTRAINT_ACCMODE);
		setconstraint(FUZZER_CONSTRAINT_VECTOR, sps, 3, FFA_NOTIFICATION_BIND_CALL_ARG1_SENDER_ID, mmod, FUZZER_CONSTRAINT_ACCMODE);
		setconstraint(FUZZER_CONSTRAINT_RANGE, bitmap_range, 2, FFA_NOTIFICATION_BIND_CALL_ARG3_BITMAP, mmod, FUZZER_CONSTRAINT_EXCMODE);
		setconstraint(FUZZER_CONSTRAINT_RANGE, bitmap_range, 2, FFA_NOTIFICATION_BIND_CALL_ARG4_BITMAP, mmod, FUZZER_CONSTRAINT_EXCMODE);
		struct inputparameters ip = generate_args(FFA_NOTIFICATION_BIND_CALL, SMC_FUZZ_SANITY_LEVEL);
		struct ffa_value ret = ffa_call_with_params(ip, FFA_NOTIFICATION_BIND);

		if(ret.fid == FFA_SUCCESS_SMC32 || ret.fid == FFA_SUCCESS_SMC64){
			printf("FFA_NOTIFICATION_BIND succeeded\n");
		}else if (ret.fid == FFA_ERROR){
			printf("FFA_NOTIFICATION_BIND returned with FFA_ERROR code %d\n", ffa_error_code(ret));
		}else{
			printf("FAIL FFA_NOTIFICATION_BIND returned with 0x%lx\n", ret.fid);
		}
	}else if (funcid == ffa_notification_bitmap_destroy_funcid) {
		uint64_t max_vm_id = (1 << 16) - 1;
		uint64_t vm_id[] = {0, max_vm_id};
		setconstraint(FUZZER_CONSTRAINT_RANGE, vm_id, 2, FFA_NOTIFICATION_BITMAP_DESTROY_CALL_ARG1_VM_ID, mmod, FUZZER_CONSTRAINT_EXCMODE);
		struct inputparameters ip = generate_args(FFA_NOTIFICATION_BITMAP_DESTROY_CALL, SMC_FUZZ_SANITY_LEVEL);
		struct ffa_value ret = ffa_call_with_params(ip, FFA_NOTIFICATION_BITMAP_DESTROY);
	
		uint64_t gen_vm_id = get_generated_value(FFA_NOTIFICATION_BITMAP_DESTROY_CALL_ARG1_VM_ID, ip);
	
		if(ret.fid == FFA_SUCCESS_SMC32 || ret.fid == FFA_SUCCESS_SMC64){
			printf("FFA_NOTIFICATION_BITMAP_DESTROY succeeded for VM %lld\n", gen_vm_id);
		}else if (ret.fid == FFA_ERROR){
			printf("FFA_NOTIFICATION_BITMAP_CREATE returned with FFA_ERROR code %d\n", ffa_error_code(ret));
		}else{
			printf("FAIL FFA_NOTIFICATION_BITMAP_CREATE returned with 0x%lx\n", ret.fid);
		}
		
	}
}

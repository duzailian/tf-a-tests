/*
 * Copyright (c) 2023-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>
#include <arch_features.h>
#include <assert.h>
#include <debug.h>
#include <realm_def.h>
#include <realm_rsi.h>
#include <sync.h>

static u_register_t challenge[] = { 
	0x7EE1FF093D754C6D,
	0x2444F4521F3F535,
	0xa41266d15ca2eddc,
	0xec16e84d995cb87e,
	0x82e9737b7f2ab19a,
	0x1c246c02682233e9,
	0xc545857ce34a0cc1,
	0xb7df9b4c317c602d
};

static unsigned char attest_token_buffers[MAX_REC_COUNT][REALM_TOKEN_BUF_SIZE]
	__aligned(GRANULE_SIZE);
static uint64_t attest_token_offsets[MAX_REC_COUNT];

bool test_realm_attestation(void)
{
	unsigned int rec = read_mpidr_el1() & MPID_MASK;
	u_register_t rsi_ret;
	u_register_t bytes_copied = 0;
	u_register_t token_upper_bound = 0;
	
	rsi_ret = rsi_attest_token_init(challenge[0],
					challenge[1],
					challenge[2],
					challenge[3],
					challenge[4],
					challenge[5],
					challenge[6],
					challenge[7],
					&token_upper_bound);
	
	if (rsi_ret != RSI_SUCCESS) {
		return false;
	}

	if (token_upper_bound > REALM_TOKEN_BUF_SIZE) {
		realm_printf("Attestation token buffer is not large enough"
				" to hold the token.\n");
		return false;
	}

	do {
		rsi_ret = rsi_attest_token_continue(
					(u_register_t)attest_token_buffers[rec],
                                        attest_token_offsets[rec],
                                      	REALM_TOKEN_BUF_SIZE,
                                        &bytes_copied);
		
		if ((rsi_ret != RSI_SUCCESS) && (rsi_ret != RSI_INCOMPLETE)) {
			realm_printf("RSI_ATTEST_TOKEN_CONTINUE"
				     " returned with code %d\n", rsi_ret);
			return false;
		}

		attest_token_offsets[rec] += bytes_copied;

	} while (rsi_ret != RSI_SUCCESS);

	return true;
}

bool test_realm_attestation_fault(void)
{
	unsigned int rec = read_mpidr_el1() & MPID_MASK;
	u_register_t rsi_ret;
	u_register_t bytes_copied = 0;

	rsi_ret = rsi_attest_token_continue(
				(u_register_t)attest_token_buffers[rec],
                                attest_token_offsets[rec],
                              	REALM_TOKEN_BUF_SIZE,
                                &bytes_copied);

	if ((rsi_ret == RSI_SUCCESS) || (rsi_ret == RSI_INCOMPLETE)) {
		return false;
	}

	return true;
}

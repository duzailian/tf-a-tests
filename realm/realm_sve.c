/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <arch_helpers.h>
#include <debug.h>
#include <stdlib.h>
#include <lib/extensions/sve.h>

#include <host_realm_sve.h>
#include <host_shared_data.h>

#define RL_SVE_OP_ARRAYSIZE		512
static int rl_sve_op_1[RL_SVE_OP_ARRAYSIZE];
static int rl_sve_op_2[RL_SVE_OP_ARRAYSIZE];
#define SVE_TEST_ITERATIONS		4

bool realm_sve_rdvl(void)
{
	host_shared_data_t *sd = realm_get_shared_structure();
	struct sve_cmd_rdvl *output;

	output = (struct sve_cmd_rdvl *)sd->realm_cmd_output_buffer;
	memset((void *)output, 0, sizeof(struct sve_cmd_rdvl));

	sve_config_vq(SVE_VQ_ARCH_MAX);
	output->rdvl = sve_vector_length_get();

	return true;
}

bool realm_sve_read_id_registers(void)
{
	host_shared_data_t *sd = realm_get_shared_structure();
	struct sve_cmd_id_regs *output;

	output = (struct sve_cmd_id_regs *)sd->realm_cmd_output_buffer;
	memset((void *)output, 0, sizeof(struct sve_cmd_id_regs));

	realm_printf("Realm: reading ID registers: ID_AA64PFR0_EL1, "
		    " ID_AA64ZFR0_EL1\n");
	output->id_aa64pfr0_el1 = read_id_aa64pfr0_el1();
	output->id_aa64zfr0_el1 = read_id_aa64zfr0_el1();

	return true;
}

bool realm_sve_probe_vl(void)
{
	host_shared_data_t *sd = realm_get_shared_structure();
	struct sve_cmd_probe_vl *output;

	output = (struct sve_cmd_probe_vl *)&sd->realm_cmd_output_buffer;
	memset((void *)output, 0, sizeof(struct sve_cmd_probe_vl));

	/* Probe all VLs */
	output->vl_bitmap = sve_probe_vl(SVE_VQ_ARCH_MAX);

	return true;
}

bool realm_sve_ops(void)
{
	int val, i;

	/* get at random value to do sve_subtract */
	val = rand();
	for (i = 0; i < RL_SVE_OP_ARRAYSIZE; i++) {
		rl_sve_op_1[i] = val - i;
		rl_sve_op_2[i] = 1;
	}

	for (i = 0; i < SVE_TEST_ITERATIONS; i++) {
		/* Config Realm with random SVE length */
		sve_config_vq(SVE_GET_RANDOM_VQ);

		/* Perform SVE operations, without world switch */
		sve_subtract_arrays(rl_sve_op_1, rl_sve_op_1, rl_sve_op_2,
				    RL_SVE_OP_ARRAYSIZE);
	}

	/* Check result of SVE operations. */
	for (i = 0; i < RL_SVE_OP_ARRAYSIZE; i++) {
		if (rl_sve_op_1[i] != (val - i - SVE_TEST_ITERATIONS)) {
			realm_printf("Realm: SVE ops failed\n");
			return false;
		}
	}

	return true;
}

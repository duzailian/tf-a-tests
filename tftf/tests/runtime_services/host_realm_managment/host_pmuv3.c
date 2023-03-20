/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <stdlib.h>

#include <arch_helpers.h>
#include <debug.h>

#include <host_realm_helper.h>
#include <host_realm_pmu.h>

#define MAX_COUNTERS		31

/* PMCCFILTR_EL0 mask */
#define PMCCFILTR_EL0_MASK (	  \
	PMCCFILTR_EL0_P_BIT	| \
	PMCCFILTR_EL0_U_BIT	| \
	PMCCFILTR_EL0_NSK_BIT	| \
	PMCCFILTR_EL0_NSH_BIT	| \
	PMCCFILTR_EL0_M_BIT	| \
	PMCCFILTR_EL0_RLK_BIT	| \
	PMCCFILTR_EL0_RLU_BIT	| \
	PMCCFILTR_EL0_RLH_BIT)

/* PMEVTYPER<n>_EL0 mask */
#define PMEVTYPER_EL0_MASK (	  \
	PMEVTYPER_EL0_P_BIT	| \
	PMEVTYPER_EL0_U_BIT	| \
	PMEVTYPER_EL0_NSK_BIT	| \
	PMEVTYPER_EL0_NSU_BIT	| \
	PMEVTYPER_EL0_NSH_BIT	| \
	PMEVTYPER_EL0_M_BIT	| \
	PMEVTYPER_EL0_RLK_BIT	| \
	PMEVTYPER_EL0_RLU_BIT	| \
	PMEVTYPER_EL0_RLH_BIT	| \
	PMEVTYPER_EL0_EVTCOUNT_BITS)

/* PMSELR_EL0 mask */
#define PMSELR_EL0_MASK		0x1F

#define WRITE_PMEV_EL0(n) {				\
	case n:						\
	pmevcntr[n] = rand_reg();			\
	write_pmevcntrn_el0(n, pmevcntr[n]);		\
	pmevtyper[n] = rand() & PMEVTYPER_EL0_MASK;	\
	write_pmevtypern_el0(n, pmevtyper[n]);		\
}

#define CHECK_PMEV_EL0(n) {						\
	case n:								\
	value = read_pmevcntrn_el0(n);					\
	if (value != pmevcntr[n]) {					\
		ERROR("Corrupted pmev%sr%d_el0=0x%lx (0x%lx)\n",	\
			"cnt", n, value, pmevcntr[n]);			\
		return false;						\
	}								\
	value = read_pmevtypern_el0(n);					\
	if (value != pmevtyper[n]) {					\
		ERROR("Corrupted pmev%sr%d_el0=0x%lx (0x%lx)\n",	\
			"type", n, value, pmevtyper[n]);		\
		return false;						\
	}								\
}

#define WRITE_PMREG_EL0(reg, mask) {	\
	reg = rand_reg() & mask;	\
	write_##reg##_el0(reg);		\
}

#define CHECK_PMREG_EL(reg, n) {					\
	value = read_##reg##_el##n();					\
	if (value != reg) {						\
		ERROR("Corrupted "#reg"_el"#n"=0x%lx (0x%lx)\n",	\
			value, reg);					\
		return false;						\
	}								\
}

u_register_t pmcr;
u_register_t pmcntenset;
u_register_t pmovsset;
u_register_t pmintenset;
u_register_t pmccntr;
u_register_t pmccfiltr;
u_register_t pmuserenr;
u_register_t pmevcntr[MAX_COUNTERS];
u_register_t pmevtyper[MAX_COUNTERS];
u_register_t pmselr;

void host_set_pmu_state(void)
{
	int num_cnts = (read_pmcr_el0() >> PMCR_EL0_N_SHIFT) &
			PMCR_EL0_N_MASK;

	pmcr = read_pmcr_el0() | PMCR_EL0_DP_BIT;

	/* Disable cycle counting and reset all counters */
	write_pmcr_el0(pmcr | PMCR_EL0_C_BIT | PMCR_EL0_P_BIT);

	/* Disable all counters */
	pmcntenset = 0UL;
	write_pmcntenclr_el0(ALL_CLEAR);

	/* Clear overflow status */
	pmovsset = 0UL;
	write_pmovsclr_el0(ALL_CLEAR);

	/* Disable overflow interrupts on all counters */
	pmintenset = 0UL;
	write_pmintenclr_el1(ALL_CLEAR);

	WRITE_PMREG_EL0(pmccntr, UINT64_MAX);
	WRITE_PMREG_EL0(pmccfiltr, PMCCFILTR_EL0_MASK);
	pmuserenr = read_pmuserenr_el0();

	if (num_cnts != 0U) {
		switch (--num_cnts) {
		WRITE_PMEV_EL0(30);
		WRITE_PMEV_EL0(29);
		WRITE_PMEV_EL0(28);
		WRITE_PMEV_EL0(27);
		WRITE_PMEV_EL0(26);
		WRITE_PMEV_EL0(25);
		WRITE_PMEV_EL0(24);
		WRITE_PMEV_EL0(23);
		WRITE_PMEV_EL0(22);
		WRITE_PMEV_EL0(21);
		WRITE_PMEV_EL0(20);
		WRITE_PMEV_EL0(19);
		WRITE_PMEV_EL0(18);
		WRITE_PMEV_EL0(17);
		WRITE_PMEV_EL0(16);
		WRITE_PMEV_EL0(15);
		WRITE_PMEV_EL0(14);
		WRITE_PMEV_EL0(13);
		WRITE_PMEV_EL0(12);
		WRITE_PMEV_EL0(11);
		WRITE_PMEV_EL0(10);
		WRITE_PMEV_EL0(9);
		WRITE_PMEV_EL0(8);
		WRITE_PMEV_EL0(7);
		WRITE_PMEV_EL0(6);
		WRITE_PMEV_EL0(5);
		WRITE_PMEV_EL0(4);
		WRITE_PMEV_EL0(3);
		WRITE_PMEV_EL0(2);
		WRITE_PMEV_EL0(1);
		default:
		WRITE_PMEV_EL0(0);
		}

		/* Generate a random number between 0 and num_cnts */
		pmselr = rand() % ++num_cnts;
	} else {
		pmselr = 0UL;
	}

	write_pmselr_el0(pmselr);
}

bool host_check_pmu_state(void)
{
	u_register_t value;
	int num_cnts;

	CHECK_PMREG_EL(pmcr, 0);
	CHECK_PMREG_EL(pmcntenset, 0);
	CHECK_PMREG_EL(pmovsset, 0);
	CHECK_PMREG_EL(pmintenset, 1);
	CHECK_PMREG_EL(pmccntr, 0);
	CHECK_PMREG_EL(pmccfiltr, 0);
	CHECK_PMREG_EL(pmselr, 0);
	CHECK_PMREG_EL(pmuserenr, 0);

	num_cnts = (read_pmcr_el0() >> PMCR_EL0_N_SHIFT) & PMCR_EL0_N_MASK;
	if (num_cnts != 0UL) {
		switch (--num_cnts) {
		CHECK_PMEV_EL0(30);
		CHECK_PMEV_EL0(29);
		CHECK_PMEV_EL0(28);
		CHECK_PMEV_EL0(27);
		CHECK_PMEV_EL0(26);
		CHECK_PMEV_EL0(25);
		CHECK_PMEV_EL0(24);
		CHECK_PMEV_EL0(23);
		CHECK_PMEV_EL0(22);
		CHECK_PMEV_EL0(21);
		CHECK_PMEV_EL0(20);
		CHECK_PMEV_EL0(19);
		CHECK_PMEV_EL0(18);
		CHECK_PMEV_EL0(17);
		CHECK_PMEV_EL0(16);
		CHECK_PMEV_EL0(15);
		CHECK_PMEV_EL0(14);
		CHECK_PMEV_EL0(13);
		CHECK_PMEV_EL0(12);
		CHECK_PMEV_EL0(11);
		CHECK_PMEV_EL0(10);
		CHECK_PMEV_EL0(9);
		CHECK_PMEV_EL0(8);
		CHECK_PMEV_EL0(7);
		CHECK_PMEV_EL0(6);
		CHECK_PMEV_EL0(5);
		CHECK_PMEV_EL0(4);
		CHECK_PMEV_EL0(3);
		CHECK_PMEV_EL0(2);
		CHECK_PMEV_EL0(1);
		default:
		CHECK_PMEV_EL0(0);
		}
	}
	return true;
}

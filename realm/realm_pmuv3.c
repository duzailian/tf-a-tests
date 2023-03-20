/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <arm_arch_svc.h>
#include <test_helpers.h>
#include <drivers/arm/gic_v3.h>

/* PMUv3 events */
#define PMU_EVT_SW_INCR		0x0
#define PMU_EVT_INST_RETIRED	0x8
#define PMU_EVT_CPU_CYCLES	0x11
#define PMU_EVT_MEM_ACCESS	0x13

#define NOP_REPETITIONS		50
#define MAX_COUNTERS		32

#define ALL_CLEAR		0x1FFFF

#define PRE_OVERFLOW		~(0xF)

#define	DELAY_MS		3000ULL

#define tftf_testcase_printf realm_printf

static inline void read_all_counters(u_register_t *array, int impl_ev_ctrs)
{
	array[0] = read_pmccntr_el0();
	for (unsigned int i = 0U; i < impl_ev_ctrs; i++) {
		array[i + 1] = read_pmevcntrn_el0(i);
	}
}

static inline void read_all_counter_configs(u_register_t *array, int impl_ev_ctrs)
{
	array[0] = read_pmccfiltr_el0();
	for (unsigned int i = 0U; i < impl_ev_ctrs; i++) {
		array[i + 1] = read_pmevtypern_el0(i);
	}
}

static inline void read_all_pmu_configs(u_register_t *array)
{
	array[0] = read_pmcntenset_el0();
	array[1] = read_pmcr_el0();
	array[2] = read_pmselr_el0();
}

static inline void enable_counting(void)
{
	write_pmcr_el0(read_pmcr_el0() | PMCR_EL0_E_BIT);
	/* This function means we are about to use the PMU, synchronize */
	isb();
}

static inline void disable_counting(void)
{
	write_pmcr_el0(read_pmcr_el0() & ~PMCR_EL0_E_BIT);
	/* We also rely that disabling really did work */
	isb();
}

static inline void clear_counters(void)
{
	write_pmcr_el0(read_pmcr_el0() | PMCR_EL0_C_BIT | PMCR_EL0_P_BIT);
	isb();
}

static void pmu_reset(void)
{
	/* Reset all counters */
	write_pmcr_el0(read_pmcr_el0() |
			PMCR_EL0_DP_BIT | PMCR_EL0_C_BIT | PMCR_EL0_P_BIT);

	/* Disable all counters */
	write_pmcntenclr_el0(ALL_CLEAR);

	/* Clear overflow status */
	write_pmovsclr_el0(ALL_CLEAR);

	/* Disable overflow interrupts on all counters */
	write_pmintenclr_el1(ALL_CLEAR);
	isb();
}

/*
 * This test runs in Realm EL1, don't bother enabling counting at lower ELs
 * and secure world. TF-A has other controls for them and counting there
 * doesn't impact us.
 */
static inline void enable_cycle_counter(void)
{
	/*
	 * Set PMCCFILTR_EL0.U != PMCCFILTR_EL0.RLU
	 * to disable counting in Realm EL0.
	 * Set PMCCFILTR_EL0.P = PMCCFILTR_EL0.RLK
	 * to enable counting in Realm EL1.
	 * Set PMCCFILTR_EL0.NSH = PMCCFILTR_EL0_EL0.RLH
	 * to disable event counting in Realm EL2.
	 */
	write_pmccfiltr_el0(PMCCFILTR_EL0_U_BIT |
			    PMCCFILTR_EL0_P_BIT | PMCCFILTR_EL0_RLK_BIT |
			    PMCCFILTR_EL0_NSH_BIT | PMCCFILTR_EL0_RLH_BIT);
	write_pmcntenset_el0(read_pmcntenset_el0() | PMCNTENSET_EL0_C_BIT);
	isb();
}

static inline void enable_event_counter(int ctr_num)
{
	/*
	 * Set PMEVTYPER_EL0.U != PMEVTYPER_EL0.RLU
	 * to disable event counting in Realm EL0.
	 * Set PMEVTYPER_EL0.P = PMEVTYPER_EL0.RLK
	 * to enable counting in Realm EL1.
	 * Set PMEVTYPER_EL0.NSH = PMEVTYPER_EL0.RLH
	 * to disable event counting in Realm EL2.
	 */
	write_pmevtypern_el0(ctr_num,
			PMEVTYPER_EL0_U_BIT |
			PMEVTYPER_EL0_P_BIT | PMEVTYPER_EL0_RLK_BIT |
			PMEVTYPER_EL0_NSH_BIT | PMEVTYPER_EL0_RLH_BIT |
			(PMU_EVT_INST_RETIRED & PMEVTYPER_EL0_EVTCOUNT_BITS));
	write_pmcntenset_el0(read_pmcntenset_el0() |
		PMCNTENSET_EL0_P_BIT(ctr_num));
	isb();
}

/* Doesn't really matter what happens, as long as it happens a lot */
static inline void execute_nops(void)
{
	for (unsigned int i = 0U; i < NOP_REPETITIONS; i++) {
		__asm__ ("orr x0, x0, x0\n");
	}
}

static inline void execute_el3_nop(void)
{
	/* Ask EL3 for some info, no side effects */
	smc_args args = { SMCCC_VERSION };

	/* Return values don't matter */
	tftf_smc(&args);
}

/*
 * Try the cycle counter with some NOPs to see if it works
 */
bool test_pmuv3_cycle_works_realm(void)
{
	u_register_t ccounter_start;
	u_register_t ccounter_end;

	SKIP_TEST_IF_PMUV3_NOT_SUPPORTED();

	pmu_reset();

	enable_cycle_counter();
	enable_counting();

	ccounter_start = read_pmccntr_el0();
	execute_nops();
	ccounter_end = read_pmccntr_el0();
	disable_counting();
	clear_counters();

	tftf_testcase_printf("Counted from %lu to %lu\n",
		ccounter_start, ccounter_end);
	if (ccounter_start != ccounter_end) {
		return true;
	}
	return false;
}

/*
 * Try an event counter with some NOPs to see if it works.
 */
bool test_pmuv3_event_works_realm(void)
{
	u_register_t evcounter_start;
	u_register_t evcounter_end;

	SKIP_TEST_IF_PMUV3_NOT_SUPPORTED();

	if (((read_pmcr_el0() >> PMCR_EL0_N_SHIFT) & PMCR_EL0_N_MASK) == 0) {
		tftf_testcase_printf("No event counters implemented\n");
		return false;
	}

	pmu_reset();

	enable_event_counter(0);
	enable_counting();

	/*
	 * If any is enabled it will be in the first range.
	 */
	evcounter_start = read_pmevcntrn_el0(0);
	execute_nops();
	disable_counting();
	evcounter_end = read_pmevcntrn_el0(0);
	clear_counters();

	tftf_testcase_printf("Counted from %lu to %lu\n",
		evcounter_start, evcounter_end);
	if (evcounter_start != evcounter_end) {
		return true;
	}
	return false;
}

/*
 * Check if entering/exiting EL3 (with a NOP) preserves all PMU registers.
 */
bool test_pmuv3_el3_preserves(void)
{
	u_register_t ctr_start[MAX_COUNTERS] = {0};
	u_register_t ctr_cfg_start[MAX_COUNTERS] = {0};
	u_register_t pmu_cfg_start[3];
	u_register_t ctr_end[MAX_COUNTERS] = {0};
	u_register_t ctr_cfg_end[MAX_COUNTERS] = {0};
	u_register_t pmu_cfg_end[3];
	unsigned int impl_ev_ctrs;

	SKIP_TEST_IF_PMUV3_NOT_SUPPORTED();

	impl_ev_ctrs = (read_pmcr_el0() >> PMCR_EL0_N_SHIFT) & PMCR_EL0_N_MASK;

	tftf_testcase_printf("Testing %u event counters\n", impl_ev_ctrs);

	pmu_reset();

	/* Pretend counters have just been used */
	enable_cycle_counter();
	enable_event_counter(0);
	enable_counting();
	execute_nops();
	disable_counting();

	/* Get before reading */
	read_all_counters(ctr_start, impl_ev_ctrs);
	read_all_counter_configs(ctr_cfg_start, impl_ev_ctrs);
	read_all_pmu_configs(pmu_cfg_start);

	/* Give EL3 a chance to scramble everything */
	execute_el3_nop();

	/* Get after reading */
	read_all_counters(ctr_end, impl_ev_ctrs);
	read_all_counter_configs(ctr_cfg_end, impl_ev_ctrs);
	read_all_pmu_configs(pmu_cfg_end);

	if (memcmp(ctr_start, ctr_end, sizeof(ctr_start)) != 0) {
		tftf_testcase_printf("SMC call did not preserve counters\n");
		return false;
	}

	if (memcmp(ctr_cfg_start, ctr_cfg_end, sizeof(ctr_cfg_start)) != 0) {
		tftf_testcase_printf("SMC call did not preserve counter config\n");
		return false;
	}

	if (memcmp(pmu_cfg_start, pmu_cfg_end, sizeof(pmu_cfg_start)) != 0) {
		tftf_testcase_printf("SMC call did not preserve PMU registers\n");
		return false;
	}

	return true;
}

bool test_pmuv3_overflow_interrupt(void)
{
	unsigned long priority_bits, priority;
	uint64_t delay_time = DELAY_MS;

	pmu_reset();

	/* Get the number of priority bits implemented */
	priority_bits = ((read_icv_ctrl_el1() >> ICV_CTLR_EL1_PRIbits_SHIFT) &
				ICV_CTLR_EL1_PRIbits_MASK) + 1UL;

	/* Unimplemented bits are RES0 and start from LSB */
	priority = (0xFFUL << (8UL - priority_bits)) & 0xFFUL;

	/* Set the priority mask register to allow all interrupts */
	write_icv_pmr_el1(priority);

	/* Enable Virtual Group 1 interrupts */
	write_icv_igrpen1_el1(ICV_IGRPEN1_EL1_Enable);

	/* Enable IRQ */
	enable_irq();

	write_pmevcntrn_el0(0, PRE_OVERFLOW);
	enable_event_counter(0);

	/* Enable interrupt on event counter #0 */
	write_pmintenset_el1((1UL << 0));

	tftf_testcase_printf("Waiting for PMU vIRQ...\n");

	enable_counting();
	execute_nops();

	while ((read_pmintenset_el1() != 0UL) &&
	       (delay_time != 0ULL)) {
		--delay_time;
	}

	/* Disable IRQ */
	disable_irq();

	pmu_reset();

	if (delay_time == 0ULL) {
		tftf_testcase_printf("PMU vIRQ %sreceived in %llums\n",
			"not ", DELAY_MS);
		return false;
	}

	tftf_testcase_printf("PMU vIRQ %sreceived in %llums\n", "",
			DELAY_MS - delay_time);

	return true;
}

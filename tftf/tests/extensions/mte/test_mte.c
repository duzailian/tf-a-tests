/*
 * Copyright (c) 2019-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <tftf.h>
#include <tftf_lib.h>
#include <tsp.h>
#include <test_helpers.h>
#include <debug.h>
#include <xlat_tables_v2.h>

#ifdef __aarch64__
#include "mte_helper.h"
#include "mte_def.h"

/* Magic values */
#define RGSR_SEED		(0x4252)
#define WRITE_VALUE_BASE	(0xAF2420)

/* Tweakables */
#define MAPPING_NUMBER		(20)
#define MAPPING_SIZE		(1 * 1024 * 1024)

typedef struct mapping_data_s {
	size_t size;
	unsigned long long pa;
	uintptr_t va;
	uint64_t *ptr_start;
	uint64_t *ptr_end;
	int write_val;
	int tag;
} mapping_data_t;

static void enable_disable_async_mte_aborts(bool enable)
{
	u_register_t reg_val;

	reg_val = read_sctlr_el2();
	reg_val &= ~(SCTLR_TCF_MASK << SCTLR_TCF_SHIFT);

	if (enable)
		reg_val |= (SCTRL_TCF_ASYNC << SCTLR_TCF_SHIFT);

	write_sctlr_el2(reg_val);
}

static void configure_mte_el2(void)
{
	uint64_t permitted_mask;

	/* Set TBI and TBID */
	write_tcr_el2(read_tcr_el2() | TCR_TBI_BIT | TCR_TBID_BIT);
	/* Set ATA */
	write_sctlr_el2(read_sctlr_el2() | SCTLR_ATA_BIT);
	/* Clear any pending faults */
	write_tfsr_el2(0);

	/* Permit values between 0x1 and 0xF */
	permitted_mask = ((1ULL << (0xF + 1)) - 1) & ~((1ULL << 0x1) - 1);
	write_gcr_el1((1 << 16) | (~permitted_mask & 0xFFFF));
	/* 'Random' seed value */
	write_rgsr_el1(RGSR_SEED << 8);

	/* Enable asynchronous MTE aborts */
	enable_disable_async_mte_aborts(true);

	/* Disable Tag Check Override */
	mte_disable_pstate_tco();

	isb();
	dsbsy();
}

static test_result_t insert_tag_add_range(uint64_t *ptr, size_t range, int *tag)
{
	uint64_t *tagged_ptr;

	tagged_ptr = mte_insert_random_tag(ptr);
	*tag = MT_FETCH_TAG((uintptr_t)tagged_ptr);
	if (!*tag) {
		ERROR("Ptr %p given zero tag\n", tagged_ptr);
		return TEST_RESULT_FAIL;
	} else {
		VERBOSE("Ptr %p given tag %d\n", tagged_ptr, *tag);
	}

	mte_set_tag_address_range(tagged_ptr, MT_ALIGN_UP(range));

	return TEST_RESULT_SUCCESS;
}

static test_result_t check_tag(uint64_t *start, uint64_t *end, int tag)
{
	uint64_t *ptr;
	int test_tag;

	for (ptr = start; ptr <= end; ptr++) {
		test_tag = MT_FETCH_TAG((uintptr_t)mte_get_tag_address(ptr));
		if (test_tag != tag) {
			ERROR("Tags not the same for ptr %p, expected %d, got %d\n",
				ptr, tag, test_tag);
			return TEST_RESULT_FAIL;
		}
	}

	return TEST_RESULT_SUCCESS;
}

static bool check_and_clear_fault(void)
{
	bool fault = read_tfsr_el2();
	write_tfsr_el2(0);
	return fault;
}

static test_result_t check_write(uint64_t *start, uint64_t *end,
				int value, int tag)
{
	volatile uint64_t *ptr;

	/* Check invalid write */
	for (ptr = start; ptr <= end; ptr++)
		*ptr = value;

	dsbsy();

	if (!check_and_clear_fault()) {
		ERROR("Expected fault from write but non raised\n");
		return TEST_RESULT_FAIL;
	}

	/* Check valid write */
	start = (uint64_t *)MT_SET_TAG((uintptr_t)start, (uintptr_t)tag);
	for (ptr = start; ptr <= end; ptr++) {
		*ptr = value;

		if (check_and_clear_fault()) {
			ERROR("Unexpected fault on write to ptr %p\n", ptr);
			return TEST_RESULT_FAIL;
		}
	}

	return TEST_RESULT_SUCCESS;
}

static test_result_t check_read(uint64_t *start, uint64_t *end,
				int expected_value, int tag)
{
	volatile uint64_t *ptr;
	int read_value;

	/* Check invalid read */
	for (ptr = start; ptr <= end; ptr++)
		read_value = *ptr;

	dsbsy();

	if (!check_and_clear_fault()) {
		ERROR("Expected fault from read but non raised\n");
		return TEST_RESULT_FAIL;
	}

	/* Check valid read */
	start = (uint64_t *)MT_SET_TAG((uintptr_t)start, (uintptr_t)tag);
	for (ptr = start; ptr <= end; ptr++) {
		read_value = *ptr;

		if (check_and_clear_fault()) {
			ERROR("Unexpected fault on read from ptr %p\n", ptr);
			return TEST_RESULT_FAIL;
		}

		if (read_value != expected_value) {
			ERROR("Ptr %p value wrong, expected %d, got %d\n",
				ptr, expected_value, read_value);
		}
	}

	return TEST_RESULT_SUCCESS;
}
#endif /* __aarch64__ */

test_result_t test_mte_tagging(void)
{
	SKIP_TEST_IF_AARCH32();
#ifdef __aarch64__
	SKIP_TEST_IF_MTE_SUPPORT_LESS_THAN(MTE_IMPLEMENTED_ELX);
	static __aligned(MT_GRANULE_SIZE) __section("mte_tagged")
	uint64_t mapping_mem[(MAPPING_SIZE * MAPPING_NUMBER) / sizeof(uint64_t)];
	static mapping_data_t mapping_data[MAPPING_NUMBER];
	/* Assume identity mapped */
	unsigned long long mapping_pa_start = (unsigned long long)mapping_mem;

	configure_mte_el2();

	for (unsigned mapping_idx = 0; mapping_idx < MAPPING_NUMBER; mapping_idx++) {
		test_result_t result;
		mapping_data_t *data = &mapping_data[mapping_idx];

		data->size = MAPPING_SIZE;
		data->pa = mapping_pa_start + (mapping_idx * data->size);
		data->va = data->pa;
		/* Pseudo-random value to test write/read */
		data->write_val = WRITE_VALUE_BASE + (mapping_idx * 50);
		data->ptr_start = (uint64_t *)(data->va);
		data->ptr_end = (uint64_t *)(data->va + data->size - sizeof(uint64_t));

		result = insert_tag_add_range(data->ptr_start, data->size, &data->tag);
		if (result != TEST_RESULT_SUCCESS)
			return result;

		result = check_tag(data->ptr_start, data->ptr_end, data->tag);
		if (result != TEST_RESULT_SUCCESS)
			return result;

		result = check_write(data->ptr_start, data->ptr_end, data->write_val, data->tag);
		if (result != TEST_RESULT_SUCCESS)
			return result;

		result = check_read(data->ptr_start, data->ptr_end, data->write_val, data->tag);
		if (result != TEST_RESULT_SUCCESS)
			return result;
	}

	for (unsigned mapping_idx = 0; mapping_idx < MAPPING_NUMBER; mapping_idx++) {
		test_result_t result;
		mapping_data_t *data = &mapping_data[mapping_idx];

		result = check_tag(data->ptr_start, data->ptr_end, data->tag);
		if (result != TEST_RESULT_SUCCESS)
			return result;

		result = check_read(data->ptr_start, data->ptr_end, data->write_val, data->tag);
		if (result != TEST_RESULT_SUCCESS)
			return result;
	}

	return TEST_RESULT_SUCCESS;
#endif /* __aarch64__ */
}

test_result_t test_mte_instructions(void)
{
	SKIP_TEST_IF_AARCH32();
#ifdef __aarch64__
	SKIP_TEST_IF_MTE_SUPPORT_LESS_THAN(MTE_IMPLEMENTED_EL0);

	/*
	 * This code must be compiled with '-march=armv8.5-memtag' option
	 * by setting 'ARM_ARCH_FEATURE=memtag' and 'ARM_ARCH_MINOR=5'
	 * build flags in tftf_config/fvp-cpu-extensions when this CI
	 * configuration is built separately.
	 * Otherwise this compiler's option must be specified explicitly.
	 *
	 * Execute Memory Tagging Extension instructions.
	 */
	__asm__ volatile (
		".arch	armv8.5-a+memtag\n"
		"mov	x0, #0xDEAD\n"
		"irg	x0, x0\n"
		"addg	x0, x0, #0x0, #0x0\n"
		"subg	x0, x0, #0x0, #0x0"
	);

	return TEST_RESULT_SUCCESS;
#endif /* __aarch64__ */
}

test_result_t test_mte_leakage(void)
{
	SKIP_TEST_IF_AARCH32();
#ifdef __aarch64__
	smc_args tsp_svc_params;
	int gcr_el1;

	SKIP_TEST_IF_MTE_SUPPORT_LESS_THAN(MTE_IMPLEMENTED_ELX);
	SKIP_TEST_IF_TSP_NOT_PRESENT();

	/* We only test gcr_el1 as writes to other MTE registers are ignored */
	write_gcr_el1(0xdd);

	/* Standard SMC to ADD two numbers */
	tsp_svc_params.fid = TSP_STD_FID(TSP_ADD);
	tsp_svc_params.arg1 = 4;
	tsp_svc_params.arg2 = 6;
	tftf_smc(&tsp_svc_params);

	gcr_el1 = read_gcr_el1();
	if (gcr_el1 != 0xdd) {
		printf("gcr_el1 has changed to %d\n", gcr_el1);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_SUCCESS;
#endif /* __aarch64__ */
}

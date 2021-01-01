/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <cactus_platform_def.h>
#include <debug.h>
#include <mmio.h>
#include <sp_helpers.h>
#include <stdint.h>

#include "cactus.h"

/* Base address of user and PRIV frames */
#define USR_BASE_FRAME	0x2BFE0000
#define PRIV_BASE_FRAME	0x2BFF0000
#define FRAME_SIZE	0x80 /* 128 bytes */

/* The test engine supports numerous frames but we only use a few */
#define FRAME_COUNT	2
#define F_IDX(n)	(n * FRAME_SIZE)

/* Commands supported by SMMUv3TestEngine built into the AEM*/
#define ENGINE_NO_FRAME	0
#define ENGINE_HALTED	1

/*
 * ENGINE_MEMCPY: Read and Write transactions
 * ENGINE_RAND48: Only Write transactions: Source address not required
 * ENGINE_SUM64: Only read transactions; Target address not required
 */
#define ENGINE_MEMCPY	2
#define ENGINE_RAND48	3
#define ENGINE_SUM64	4
#define ENGINE_ERROR	0xFFFFFFFF
#define ENGINE_MIS_CFG	(ENGINE_ERROR - 1)

/* Refer to:
https://developer.arm.com/documentation/100964/1111-00/Trace-components/SMMUv3TestEngine---trace */

/* Offset of various control fields belonging to User Frame */
#define CMD_OFF		0x0
#define UCTRL_OFF	0x4
#define SEED_OFF	0x24
#define BEGIN_OFF	0x28
#define END_CTRL_OFF	0x30
#define STRIDE_OFF	0x38
#define UDATA_OFF	0x40

/* Offset of various control fields belonging to PRIV Frame */
#define PCTRL_OFF		0x0
#define DOWNSTREAM_PORT_OFF	0x4
#define STREAM_ID_OFF		0x8
#define SUBSTREAM_ID_OFF	0xC

#define MEMCPY_SOURCE_BASE	PLAT_CACTUS_AUX_BASE
#define MEMPCY_TOTAL_SIZE	0x4000
#define MEMCPY_TARGET_BASE	(PLAT_CACTUS_AUX_BASE + MEMPCY_TOTAL_SIZE)

/* Miscelleneous */
#define NO_SUBSTREAMID	0xFFFFFFFF
#define TRANSFR_SIZE	(MEMPCY_TOTAL_SIZE/FRAME_COUNT)
#define TEST_TIMEOUT	5000

bool run_smmuv3_test(struct mailbox_buffers *mb, ffa_vm_id_t ffa_id)
{
	uint64_t source_addr, cpy_range, target_addr;
	uint64_t begin_addr, end_addr, dest_addr;
	uint32_t status;
	unsigned int i, f, attempts;

	if (ffa_id != SPM_VM_ID_FIRST) {
		return false;
	}

	/*
	 * The test engine's MEMCPY command copies data from the region in
	 * range [begin, end_incl] to the region with base address as udata.
	 * In this test, we configure the test engine to initiate memcpy from
	 * scratch page located at MEMCPY_SOURCE_BASE to the page located at
	 * address MEMCPY_TARGET_BASE
	 */

	VERBOSE("CACTUS: run_smmuv3_test\n");

	source_addr = MEMCPY_SOURCE_BASE;
	cpy_range = MEMPCY_TOTAL_SIZE;
	target_addr = MEMCPY_TARGET_BASE;
	uint32_t streamID_list[] = { 0, 1};

	uint64_t data[] = {
		0xBAADFEEDCEEBDAAF,
		0x0123456776543210
	};

	/* Write pre-determined content to source pages */
	for (i = 0; i < (cpy_range/8); i++) {
		mmio_write64_offset(source_addr, i*8, data[i%2]);
	}

	/* Clean the data caches */
	clean_dcache_range(source_addr,cpy_range);
	clean_dcache_range(target_addr,cpy_range);

	/*
	 * Make sure above load, store and cache maintenance instructions
	 * complete before we start writing to TestEngine frame configuration
	 * fields
	 */
	dsbsy();

	for (f = 0; f < FRAME_COUNT; f++) {
		attempts = 0U;
		begin_addr = source_addr + (TRANSFR_SIZE * f);
		end_addr = begin_addr + TRANSFR_SIZE - 1;
		dest_addr = target_addr + (TRANSFR_SIZE * f);

		/* Initiate DMA sequence */
		mmio_write32_offset(PRIV_BASE_FRAME + F_IDX(f), PCTRL_OFF, 0);
		mmio_write32_offset(PRIV_BASE_FRAME + F_IDX(f), DOWNSTREAM_PORT_OFF, 0);
		mmio_write32_offset(PRIV_BASE_FRAME + F_IDX(f), STREAM_ID_OFF, streamID_list[f%2]);
		mmio_write32_offset(PRIV_BASE_FRAME + F_IDX(f), SUBSTREAM_ID_OFF, NO_SUBSTREAMID);

		mmio_write32_offset(USR_BASE_FRAME + F_IDX(f), UCTRL_OFF, 0x0);
		mmio_write32_offset(USR_BASE_FRAME + F_IDX(f), SEED_OFF , 0x0);
		mmio_write64_offset(USR_BASE_FRAME + F_IDX(f), BEGIN_OFF, begin_addr);
		mmio_write64_offset(USR_BASE_FRAME + F_IDX(f), END_CTRL_OFF, end_addr);

		/* Legal values for stride: 1 and any multiples of 8 */
		mmio_write64_offset(USR_BASE_FRAME + F_IDX(f), STRIDE_OFF, 0x1);
		mmio_write64_offset(USR_BASE_FRAME + F_IDX(f), UDATA_OFF, dest_addr);

		mmio_write32_offset(USR_BASE_FRAME + F_IDX(f), CMD_OFF, ENGINE_MEMCPY);
		VERBOSE("SMMUv3TestEngine: Waiting for MEMCPY completion for frame: %u\n", f);

		/*
		 * It is guaranteed that a read of "cmd" fields after writing to it will
		 * immediately return ENGINE_FRAME_MISCONFIGURED if the command was
		 * invalid.
		 */
		if (mmio_read32_offset(USR_BASE_FRAME + F_IDX(f), CMD_OFF) == ENGINE_MIS_CFG) {
			ERROR("SMMUv3TestEngine misconfigured for frame: %u\n", f);
			return false;
		}

		/* wait for mem copy to be complete */
		while (attempts++ < TEST_TIMEOUT) {
			status = mmio_read32_offset(USR_BASE_FRAME + F_IDX(f), CMD_OFF);
			if (status == ENGINE_HALTED) {
				break;
			} else if (status == ENGINE_ERROR) {
				ERROR("SMMUv3 Test failed\n");
				return false;
			}
		}

		if (attempts == TEST_TIMEOUT) {
			ERROR("SMMUv3 Test failed\n");
			return false;
		}

		dsbsy();
	}

	/*
	 * Invalidate cached entries to force the CPU to fetch the data from
	 * Main memory
	 */
	inv_dcache_range(source_addr,cpy_range);
	inv_dcache_range(target_addr,cpy_range);

	/* Compare source and destination memory locations for data */
	for (i = 0 ; i < (cpy_range/8); i++) {
		if (mmio_read_64(source_addr + 8*i) != mmio_read_64(target_addr + 8*i)) {
			ERROR("SMMUv3: Mem copy failed: %llx\n", target_addr + 8*i);
			return false;
		}
	}
	return true;
}

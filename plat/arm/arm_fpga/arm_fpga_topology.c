/*
 * Copyright (c) 2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <assert.h>
#include <mmio.h>
#include <plat_topology.h>
#include <platform_def.h>
#include <stddef.h>
#include <tftf_lib.h>

/* FPGA_ARM Power controller based defines */
#define PWRC_BASE		0x1c100000
#define PSYSR_OFF		0x10
#define PSYSR_INVALID		0xffffffff

#if ARM_FPGA_MAX_PE_PER_CPU == 2
#define	CPU_DEF(cluster, cpu)	\
	{ cluster, cpu, 0 },	\
	{ cluster, cpu, 1 }

#else
#error "Incorrect settings for ARM_FPGA_MAX_PE_PER_CPU"
#endif

/* 4 CPUs per cluster */
#define	CLUSTER_DEF(cluster)	\
	CPU_DEF(cluster, 0),	\
	CPU_DEF(cluster, 1),	\
	CPU_DEF(cluster, 2),	\
	CPU_DEF(cluster, 3)

static const struct {
	unsigned int cluster_id;
	unsigned int cpu_id;
#if PLAT_MAX_PE_PER_CPU > 1
	unsigned int thread_id;
#endif
} arm_fpga_base_cores[] = {
	/* Clusters 0...2 */
	CLUSTER_DEF(0),
	CLUSTER_DEF(1)
};

/*
 * The ARM_FPGA power domain tree descriptor. We always construct a topology
 * with the maximum number of cluster nodes possible for ARM_FPGA. During
 * TFTF initialization, the actual number of nodes present on the model
 * will be queried dynamically using `tftf_plat_get_mpidr()`.
 * The ARM_FPGA power domain tree does not have a single system level power
 * domain i.e. a single root node. The first entry in the power domain
 * descriptor specifies the number of power domains at the highest power level
 * which is equal to ARM_FPGA_CLUSTER_COUNT.
 */
static const unsigned char arm_fpga_power_domain_tree_desc[] = {
	/* Number of system nodes */
	1,
	/* Number of cluster nodes */
	ARM_FPGA_MAX_CLUSTER_COUNT,
	/* Number of children for the first node */
	ARM_FPGA_MAX_CPUS_PER_CLUSTER * ARM_FPGA_MAX_PE_PER_CPU,
	/* Number of children for the second node */
	ARM_FPGA_MAX_CPUS_PER_CLUSTER * ARM_FPGA_MAX_PE_PER_CPU,
	/* Number of children for the third node */
	ARM_FPGA_MAX_CPUS_PER_CLUSTER * ARM_FPGA_MAX_PE_PER_CPU,
	/* Number of children for the fourth node */
	ARM_FPGA_MAX_CPUS_PER_CLUSTER * ARM_FPGA_MAX_PE_PER_CPU
};

const unsigned char *tftf_plat_get_pwr_domain_tree_desc(void)
{
	return arm_fpga_power_domain_tree_desc;
}

/*
static unsigned int arm_fpga_pwrc_read_psysr(unsigned long mpidr)
{
	unsigned int rc;
	mmio_write_32(PWRC_BASE + PSYSR_OFF, (unsigned int) mpidr);
	rc = mmio_read_32(PWRC_BASE + PSYSR_OFF);
	return rc;
}
*/

uint64_t tftf_plat_get_mpidr(unsigned int core_pos)
{
	unsigned int mpid;

	assert(core_pos < PLATFORM_CORE_COUNT);

	mpid = make_mpid(
			arm_fpga_base_cores[core_pos].cluster_id,
#if ARM_FPGA_MAX_PE_PER_CPU > 1
			arm_fpga_base_cores[core_pos].cpu_id,
			arm_fpga_base_cores[core_pos].thread_id);
#else
			arm_fpga_base_cores[core_pos].cpu_id);
#endif

	if (tftf_psci_affinity_info(mpid, MPIDR_AFFLVL0) >= 0) {
		return mpid;
	}

	return INVALID_MPID;
}

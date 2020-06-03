/*
 * Copyright (c) 2018-2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <debug.h>
#include <drivers/arm/private_timer.h>
#include <events.h>
#include <libfdt.h>
#include <plat_topology.h>
#include <platform.h>
#include <power_management.h>
#include <sdei.h>
#include <tftf_lib.h>
#include <timer.h>
#include "fifo3d.h"


extern char _binary___build_fvp_debug_smcf_dtb_start[];

struct memmod tmod __aligned(65536) __section("smcfuzz");

/*
 * switch to use either standard c malloc or custom smc malloc
 */

#define FIRST_NODE_DEVTREE_OFFSET (8)

#ifdef SMC_FUZZ_TMALLOC
#define GENMALLOC(x)	malloc((x))
#define GENFREE(x)	free((x))
#else
#define GENMALLOC(x)	smcmalloc((x),mmod)
#define GENFREE(x)	smcfree((x),mmod)
#endif

/*
 * Device tree parameter struct
 */

struct fdt_header_sf {
	unsigned int magic;
	unsigned int totalsize;
	unsigned int off_dt_struct;
	unsigned int off_dt_strings;
	unsigned int off_mem_rsvmap;
	unsigned int version;
	unsigned int last_comp_version;
	unsigned int boot_cpuid_phys;
	unsigned int size_dt_strings;
	unsigned int size_dt_struct;
};

/*
 * Structure to read the fields of the device tree
 */
struct propval {
	unsigned int len;
	unsigned int nameoff;
};

/*
 * Converting from big endian to little endian to read values
 * of device tree
 */
unsigned int lendconv(unsigned int val)
{
	unsigned int res;

	res = val<<24;
	res |= ((val<<8)&0xFF0000);
	res |= ((val>>8)&0xFF00);
	res |= ((val>>24)&0xFF);
	return res;
}


/*
 * Function to read strings from device tree
 */
void pullstringdt(void **dvt,
		  void *dvt_beg,
		  unsigned int offset,
		  char *cset)
{
	int fistr;
	int cntchr;
	char rval;

	if (offset > 0)
		*dvt = dvt_beg + offset;
	fistr = 0;
	cntchr = 0;
	while (fistr == 0) {
		rval = *((char *)*dvt);
		*dvt += sizeof(char);
		cset[cntchr] = rval;
		if (cset[cntchr] == 0)
			fistr = 1;
		cntchr++;
	}
	if ((cntchr%4) != 0) {
		for (int i = 0; i < (4 - (cntchr%4)); i++)
			*dvt += sizeof(char);
	}
}

/*
 * Structure for Node information extracted from device tree
 */
struct rand_smc_node {
	int *biases;
	int *biasarray;
	char **snames;
	struct rand_smc_node *treenodes;
	int *norcall;
	int entries;
	int biasent;
	char **nname;
};


/*
 * Create bias tree from given device tree description
 */

struct rand_smc_node *createsmctree(int *casz,
				    struct memmod *mmod)
{
	void *dvt;
	void *dvt_pn;
	void *dvt_beg;
	struct fdt_header fhd;
	unsigned int rval;
	struct propval pv;
	char cset[50];
	char nodename[50];
	int dtdone;
	struct fifo3d f3d;
	int leafnode = 0;

	f3d.col = 0;
	f3d.currcol = 0;
	unsigned int fnode = 0;
	unsigned int bcnt = 0;
	unsigned int bintnode = 0;
	unsigned int treenodetrack = 0;

	/*
	 * Read device tree header and check for valid type
	 */
	struct fdt_header *fhdptr;

	fhdptr = (struct fdt_header *)_binary___build_fvp_debug_smcf_dtb_start;

	if (lendconv(fhdptr->magic) != 0xD00DFEED)
		printf("ERROR, not device tree compliant\n");
	fhd = *fhdptr;
	struct rand_smc_node *ndarray = NULL;
	int cntndarray;
	cntndarray = 0;
	struct rand_smc_node nrnode;
	nrnode.entries = 0;
	struct rand_smc_node *tndarray;

	/*
	  * Create pointers to device tree data
	  */
	dvt = _binary___build_fvp_debug_smcf_dtb_start;
	dvt_pn = _binary___build_fvp_debug_smcf_dtb_start;

	dvt_beg = dvt;
	fhd = *((struct fdt_header *)dvt);
	dvt += (lendconv(fhd.off_dt_struct) + FIRST_NODE_DEVTREE_OFFSET);
	dtdone = 0;

	/*
	  * Reading device tree file
	  */
	while (dtdone == 0) {
		rval = *((unsigned int *)dvt);
		dvt += sizeof(unsigned int);

		/*
		  * Reading node name from device tree and pushing it into the raw data
		  */
		if (lendconv(rval) == 1) {
			pullstringdt(&dvt,dvt_beg,0,cset);
			push3dfifocol(&f3d,cset,mmod);
			strlcpy(nodename,cset,50);

			/*
			   * Error checking to make sure that bias is specified
			   */
			if (fnode == 0)
				fnode = 1;
			else {
				if (!((fnode == 1) && (bcnt == 1))) {
					printf("ERROR: Did not find bias or multiple bias designations before %s %d %d\n",cset,fnode,bcnt);
				}
				bcnt = 0;
			}
		}

		/*
		  * Reading node parameters of bias and function name
		  */
		if (lendconv(rval) == 3) {
			pv = *((struct propval *)dvt);
			dvt += sizeof(struct propval);
			pullstringdt(&dvt_pn,dvt_beg,
				     (lendconv(fhd.off_dt_strings) +
				      lendconv(pv.nameoff)),cset);
			if (strcmp(cset,"bias") == 0) {
				rval = *((unsigned int *)dvt);
				dvt += sizeof(unsigned int);
				push3dfifobias(&f3d,lendconv(rval));
				bcnt++;
				if (bintnode == 1) {
					fnode = 0;
					bintnode = 0;
					bcnt = 0;
				}
			}
			if (strcmp(cset,"functionname") == 0) {
				pullstringdt(&dvt,dvt_beg,0,cset);
				push3dfifofname(&f3d,cset);
				leafnode = 1;
				if (bcnt == 0) {
					bintnode = 1;
					fnode = 1;
				} else {
					bcnt = 0;
					fnode = 0;
				}
			}
		}

		/*
		  * Node termination and evaluate whether the bias tree requires addition.
		  * The non tree nodes are added.
		  */
		if (lendconv(rval) == 2) {
			if ((fnode > 0) || (bcnt > 0))
				printf("ERROR: early node termination... no bias or functionname field for leaf node, near %s %d\n",nodename,fnode);
			f3d.col--;
			if (leafnode == 1)
				leafnode = 0;
			else {
				/*
				    * Create bias tree in memory from raw data
				    */
				tndarray =
					GENMALLOC( (cntndarray + 1) *
						   sizeof(struct rand_smc_node) );
				unsigned int treenodetrackmal = 0;
				for (int j = 0; j < cntndarray; j++) {
					tndarray[j].biases = GENMALLOC(ndarray[j].entries * sizeof(int) );
					tndarray[j].snames = GENMALLOC(ndarray[j].entries * sizeof(char *) );
					tndarray[j].norcall = GENMALLOC(ndarray[j].entries * sizeof(int) );
					tndarray[j].nname = GENMALLOC(ndarray[j].entries * sizeof(char *) );
					tndarray[j].treenodes = GENMALLOC(ndarray[j].entries * sizeof(struct rand_smc_node) );
					tndarray[j].entries = ndarray[j].entries;
					for (int i = 0; i < ndarray[j].entries;i++) {
						tndarray[j].snames[i] = GENMALLOC(1 * sizeof(char[50] ));
						strlcpy(tndarray[j].snames[i],ndarray[j].snames[i],50);
						tndarray[j].nname[i] = GENMALLOC(1 * sizeof(char[50] ));
						strlcpy(tndarray[j].nname[i], ndarray[j].nname[i],50);
						tndarray[j].biases[i] = ndarray[j].biases[i];
						tndarray[j].norcall[i] = ndarray[j].norcall[i];
						if (tndarray[j].norcall[i] == 1) {
							tndarray[j].treenodes[i] = tndarray[treenodetrackmal];
							treenodetrackmal++;
						}
					}
					tndarray[j].biasent = ndarray[j].biasent;
					tndarray[j].biasarray = GENMALLOC( (tndarray[j].biasent) * sizeof(int) );
					for (int i = 0; i < ndarray[j].biasent; i++) {
						tndarray[j].biasarray[i] = ndarray[j].biasarray[i];
					}
				}
				tndarray[cntndarray].biases = GENMALLOC(f3d.row[f3d.col + 1] * sizeof(int) );
				tndarray[cntndarray].snames = GENMALLOC(f3d.row[f3d.col + 1] * sizeof(char *) );
				tndarray[cntndarray].norcall = GENMALLOC(f3d.row[f3d.col + 1] * sizeof(int) );
				tndarray[cntndarray].nname = GENMALLOC(f3d.row[f3d.col + 1] * sizeof(char *) );
				tndarray[cntndarray].treenodes = GENMALLOC(f3d.row[f3d.col + 1] * sizeof(struct rand_smc_node) );
				tndarray[cntndarray].entries = f3d.row[f3d.col + 1];

				/*
				    * Populate bias tree with former values in tree
				    */
				int cntbias = 0;
				int bcnt = 0;
				for (int j = 0; j < f3d.row[f3d.col + 1]; j++) {
					tndarray[cntndarray].snames[j] = GENMALLOC(1 * sizeof(char[50] ));
					strlcpy(tndarray[cntndarray].snames[j],f3d.fnamefifo[f3d.col + 1][j],50);
					tndarray[cntndarray].nname[j] = GENMALLOC(1 * sizeof(char[50] ));
					strlcpy(tndarray[cntndarray].nname[j], f3d.nnfifo[f3d.col + 1][j],50);
					tndarray[cntndarray].biases[j] = f3d.biasfifo[f3d.col + 1][j];
					cntbias += tndarray[cntndarray].biases[j];
					if (strcmp(tndarray[cntndarray].snames[j],"none") != 0) {
						strlcpy(tndarray[cntndarray].snames[j], f3d.fnamefifo[f3d.col + 1][j],50);
						tndarray[cntndarray].norcall[j] = 0;
						tndarray[cntndarray].treenodes[j] = nrnode;
					} else {
						tndarray[cntndarray].norcall[j] = 1;
						tndarray[cntndarray].treenodes[j] = tndarray[treenodetrack];
						treenodetrack++;
					}
				}

				tndarray[cntndarray].biasent = cntbias;
				tndarray[cntndarray].biasarray = GENMALLOC( (tndarray[cntndarray].biasent) * sizeof(int) );
				for (int j = 0;j < tndarray[cntndarray].entries;j++) {
					for (int i = 0;i < tndarray[cntndarray].biases[j];i++) {
						tndarray[cntndarray].biasarray[bcnt] = j;
						bcnt++;
					}
				}

				/*
				    * Free memory of old bias tree
				    */
				if (cntndarray > 0) {
					for (int j = 0; j < cntndarray; j++) {
						for (int i = 0;
						     i < ndarray[j].entries;
						     i++) {
							GENFREE(ndarray[j].snames[i]);
							GENFREE(ndarray[j].nname[i]);
						}
						GENFREE(ndarray[j].biases);
						GENFREE(ndarray[j].norcall);
						GENFREE(ndarray[j].biasarray);
						GENFREE(ndarray[j].snames);
						GENFREE(ndarray[j].nname);
						GENFREE(ndarray[j].treenodes);
					}
					GENFREE(ndarray);
				}

				/*
				    * Move pointers to new bias tree to current tree
				    */
				ndarray = tndarray;
				cntndarray++;

				/*
				    * Free raw data
				    */
				for (int j = 0; j < f3d.row[f3d.col + 1]; j++) {
					GENFREE(f3d.nnfifo[f3d.col + 1][j]);
					GENFREE(f3d.fnamefifo[f3d.col + 1][j]);
				}
				GENFREE(f3d.nnfifo[f3d.col + 1]);
				GENFREE(f3d.fnamefifo[f3d.col + 1]);
				GENFREE(f3d.biasfifo[f3d.col + 1]);
				f3d.currcol -= 1;
			}
		}

		/*
		  * Ending device tree file and freeing raw data
		  */
		if (lendconv(rval) == 9) {
			for (int i = 0; i < f3d.col; i++) {
				for (int j = 0; j < f3d.row[i]; j++) {
					GENFREE(f3d.nnfifo[i][j]);
					GENFREE(f3d.fnamefifo[i][j]);
				}
				GENFREE(f3d.nnfifo[i]);
				GENFREE(f3d.fnamefifo[i]);
				GENFREE(f3d.biasfifo[i]);
			}
			GENFREE(f3d.nnfifo);
			GENFREE(f3d.fnamefifo);
			GENFREE(f3d.biasfifo);
			GENFREE(f3d.row);
			dtdone = 1;
		}
	}


	*casz = cntndarray;
	return ndarray;
}

/*
 * Running SMC call from what function name is selected
 */
void runtestfunction(char *funcstr)
{
	if (strcmp(funcstr,"sdei_version") == 0) {
		long long ret = sdei_version();
		if (ret != MAKE_SDEI_VERSION(1, 0, 0))
			tftf_testcase_printf("Unexpected SDEI version: 0x%llx\n",
					     ret);
		printf("running %s\n",funcstr);
	}
	if (strcmp(funcstr,"sdei_pe_unmask") == 0) {
		long long ret = sdei_pe_unmask();
		if (ret < 0)
			tftf_testcase_printf("SDEI pe unmask failed: 0x%llx\n",
					     ret);
		printf("running %s\n",funcstr);
	}
	if (strcmp(funcstr,"sdei_pe_mask") == 0) {
		int64_t ret = sdei_pe_mask();
		if (ret < 0)
			tftf_testcase_printf("SDEI pe mask failed: 0x%llx\n", ret);
		printf("running %s\n",funcstr);
	}
	if (strcmp(funcstr,"sdei_event_status") == 0) {
		int64_t ret = sdei_event_status(0);
		if (ret < 0)
			tftf_testcase_printf("SDEI event status failed: 0x%llx\n",
					     ret);
		printf("running %s\n",funcstr);
	}
	if (strcmp(funcstr,"sdei_event_signal") == 0) {
		int64_t ret = sdei_event_signal(0);
		if (ret < 0)
			tftf_testcase_printf("SDEI event signal failed: 0x%llx\n",
					     ret);
		printf("running %s\n",funcstr);
	}
	if (strcmp(funcstr,"sdei_private_reset") == 0) {
		int64_t ret = sdei_private_reset();
		if (ret < 0)
			tftf_testcase_printf("SDEI private reset failed: 0x%llx\n",
					     ret);
		printf("running %s\n",funcstr);
	}
	if (strcmp(funcstr,"sdei_shared_reset") == 0) {
		int64_t ret = sdei_shared_reset();
		if (ret < 0)
			tftf_testcase_printf("SDEI shared reset failed: 0x%llx\n",
					     ret);
		printf("running %s\n",funcstr);
	}
}

/*
 * Top of SMC fuzzing module
 */
test_result_t smc_fuzzing_top(void)
{
	/*
	 * Setting up malloc block parameters
	 */
	tmod.memptr = (void *)tmod.memory;
	tmod.memptrend = (void *)tmod.memory;
	tmod.maxmemblk = ((TOTALMEMORYSIZE/BLKSPACEDIV)/sizeof(struct memblk));
	tmod.nmemblk = 1;
	tmod.memptr->address = 0;
	tmod.memptr->size = TOTALMEMORYSIZE - (TOTALMEMORYSIZE / BLKSPACEDIV);
	tmod.memptr->valid = 1;
	tmod.mallocdeladd[0] = 0;
	tmod.precblock[0] = (void *)tmod.memory;
	tmod.trailblock[0] = NULL;
	tmod.cntdeladd = 0;
	tmod.ptrmemblkqueue = 0;
	tmod.mallocdeladd_queue_cnt = 0;
	tmod.checkadd = 1;
	tmod.checknumentries = 0;
	tmod.memerror = 0;
	struct memmod *mmod;
	mmod = &tmod;
	int cntndarray;
	struct rand_smc_node *tlnode;

	/*
	 * Creating SMC bias tree
	 */
	struct rand_smc_node *ndarray = createsmctree(&cntndarray,&tmod);

	if (tmod.memerror != 0)
		return TEST_RESULT_FAIL;

	/*
	 * Hard coded seed, will change in the near future for better strategy
	 */
	srand(89758389);

	/*
	 * Code to traverse the bias tree and select function based on the biaes within
	 */
	for (int i = 0; i < 100; i++) {
		tlnode = &ndarray[cntndarray - 1];
		int nd = 0;
		while (nd == 0) {
			int nch = rand()%tlnode->biasent;
			int selent = tlnode->biasarray[nch];
			if (tlnode->norcall[selent] == 0) {
				runtestfunction(tlnode->snames[selent]);
				nd = 1;
			} else
				tlnode = &tlnode->treenodes[selent];
		}
	}

	/*
	 * End of test SMC selection and freeing of nodes
	 */
	if (cntndarray > 0) {
		for (int j = 0; j < cntndarray; j++) {
			for (int i = 0; i < ndarray[j].entries; i++) {
				GENFREE(ndarray[j].snames[i]);
				GENFREE(ndarray[j].nname[i]);
			}
			GENFREE(ndarray[j].biases);
			GENFREE(ndarray[j].norcall);
			GENFREE(ndarray[j].biasarray);
			GENFREE(ndarray[j].snames);
			GENFREE(ndarray[j].nname);
			GENFREE(ndarray[j].treenodes);
		}
		GENFREE(ndarray);
	}

	return TEST_RESULT_SUCCESS;
}

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
#include "smcmalloc.h"

extern char _binary___build_fvp_debug_smcf_dtb_start[];

struct memmod tmod __aligned(65536) __section("smcfuzz");

/*
* Priority encoder for enabling proper alignment of returned malloc
* addresses
*/
struct peret priorityencoder(unsigned int num)
{
	unsigned int topbit = 0;
	struct peret prt;
	unsigned int cntbit = 0;

	for (int i = TOPBITSIZE; i >= 0; i--) {
		if (((num >> i) &1 ) == 1) {
			if (topbit < i)
				topbit = i;
			cntbit++;
		}
	}
	prt.pow2 = 0;
	if (cntbit == 1) {
		prt.pow2 = 1;
	}	
	prt.tbit = topbit;
	return prt;
}

/*
* Generic malloc function requesting memory.  Alignment of
* returned memory is the next largest size if not a power
* of two. The memmod structure is required to represent memory image
*/
void *smcmalloc(unsigned int rsize,
		struct memmod *mmod)
{
	unsigned int alignnum;
	unsigned int modval;
	unsigned int aladd;
	unsigned int mallocdeladd_pos = 0;
	struct memblk *newblk = NULL;
	bool foundmem = false;
	struct peret prt;
	int incrnmemblk = 0;
	int incrcntdeladd = 0;
	
        /*
	  * minimum size is 16
	  */
	if (rsize < 16)
		rsize = 16;

	/*
	  * Is size on a power of 2 boundary?  if not select next largest power of 2
	  * to place the memory request in
	  */
	prt = priorityencoder(rsize);
	if (prt.pow2 == 1)
		alignnum = 1 << prt.tbit;
	else
		alignnum = 1 << (prt.tbit + 1);
	mmod->memptr = (void *)mmod->memory;
	for (unsigned int i = 0; i < mmod->nmemblk; i++) {
		modval = mmod->memptr->address % alignnum;
		if (modval == 0)
			aladd = 0;
		else
			aladd = alignnum - modval;

		/*
		   * Searching sizes and alignments of memory blocks to find a candidate that will
		   * accept the size
		   */
		if ((rsize <= (mmod->memptr->size - aladd)) &&
		    (mmod->memptr->size > aladd) && (mmod->memptr->valid == 1)) {
			foundmem = true;

			/*
			   * Reuse malloc table entries that have been retired.
			   * If none exists create new entry
			   */
			if (mmod->mallocdeladd_queue_cnt > 0) {
				mmod->mallocdeladd_queue_cnt--;
				mallocdeladd_pos =
					mmod->mallocdeladd_queue[mmod->
								 mallocdeladd_queue_cnt
					];
			} else {
				mallocdeladd_pos = mmod->cntdeladd;
				incrcntdeladd = 1;
			}

			/*
			   * Determining if the size adheres to power of 2 boundary and
			   * if a retired malloc block
			   * can be utilized from the malloc table
			   */
			if (modval == 0) {
				if (mmod->ptrmemblkqueue > 0) {
					newblk = mmod->memblkqueue[mmod->ptrmemblkqueue - 1];
					mmod->ptrmemblkqueue--;
				} else {
					newblk = mmod->memptrend;
					newblk++;
					incrnmemblk = 1;
				}

				/*
				    * Setting memory block parameters for newly created memory
				    */
				newblk->size = 0;
				newblk->address = mmod->memptr->address;
				newblk->valid = 1;
				mmod->precblock[mallocdeladd_pos] = newblk;

				/*
				    * Scrolling through the malloc attribute table to
				    * find entries that have values that
				    * match the newly created block and replace them with it
				    */
				unsigned int fadd = newblk->address + newblk->size;
				for (unsigned int j = 0; j < mmod->cntdeladd; j++) {
					if ((fadd == mmod->mallocdeladd[j]) && (mmod->mallocdeladd_valid[j] == 1)) {
						mmod->precblock[j] = newblk;
					}
					if ((fadd ==
					     (mmod->mallocdeladd[j] + mmod->memallocsize[j])) && (mmod->mallocdeladd_valid[j] == 1)) {
						mmod->trailblock[j] = newblk;
					}
				}

				/*
				    * Setting table parameters
				    */
				mmod->mallocdeladd[mallocdeladd_pos] =
					mmod->memptr->address;
				mmod->memallocsize[mallocdeladd_pos] = rsize;
				mmod->memptr->size -= rsize;
				mmod->memptr->address += (rsize);
				mmod->trailblock[mallocdeladd_pos] = mmod->memptr;
				mmod->mallocdeladd_valid[mallocdeladd_pos] = 1;
				mmod->memptr = (void *)mmod->memory;

			        /*
				    * Removing entries from malloc table that can be
				    * merged with other blocks
				    */
				for (unsigned int j = 0; j < mmod->nmemblk; j++) {
					if (mmod->memptr->valid == 1) {
						if ((mmod->trailblock[mallocdeladd_pos]->address +
						     mmod->trailblock[mallocdeladd_pos]->size) == mmod->memptr->address) {
							if ((mmod->memptr->size ==
							     0) && (mmod->trailblock[mallocdeladd_pos]->size != 0)) {
								mmod->memptr->valid = 0;
								mmod->memblkqueue[mmod->ptrmemblkqueue] = mmod->memptr;
								mmod->ptrmemblkqueue++;
								if (mmod-> ptrmemblkqueue >= mmod->maxmemblk)
								{
									mmod->memerror = 1;
								}
							}
						}
					}
					mmod->memptr++;
				}
			} else {
				/*
				    * Allocating memory that is aligned with power of 2
				    */
				unsigned int nblksize = mmod->memptr->size - rsize - (alignnum - modval);
				if (mmod->ptrmemblkqueue > 0) {
					newblk = mmod->memblkqueue[mmod->ptrmemblkqueue - 1];
					mmod->ptrmemblkqueue--;
				} else {
					newblk = mmod->memptrend;
					newblk++;
					incrnmemblk = 1;
				}
				newblk->size = nblksize;
				newblk->address = mmod->memptr->address +
						  (alignnum - modval) + rsize;
				newblk->valid = 1;
				mmod->trailblock[mallocdeladd_pos] = newblk;

				/*
				    * Scrolling through the malloc attribute table to find entries
				    * that have values that
				    * match the newly created block and replace them with it
				    */
				unsigned int fadd = newblk->address + newblk->size;
				for (unsigned int i = 0; i < mmod->cntdeladd; i++) {
					if ((fadd == mmod->mallocdeladd[i]) && (mmod->mallocdeladd_valid[i] == 1)) {
					    mmod->precblock[i] = newblk;
					}
					if ((fadd == (mmod->mallocdeladd[i] +
					      mmod->memallocsize[i])) && (mmod->mallocdeladd_valid[i] == 1)) {
						mmod->trailblock[i] = newblk;
					}
				}

				/*
				    * Setting table parameters
				    */
				mmod->memallocsize[mallocdeladd_pos] = rsize;
				mmod->memptr->size = (alignnum - modval);
				mmod->mallocdeladd[mallocdeladd_pos] = mmod->memptr->address + mmod->memptr->size;
				mmod->precblock[mallocdeladd_pos] = mmod->memptr;
				mmod->mallocdeladd_valid[mallocdeladd_pos] = 1;
			}
			if (incrcntdeladd == 1) {
				mmod->cntdeladd++;
				if (mmod->cntdeladd >= mmod->maxmemblk) {
					printf("ERROR: size of GENMALLOC table exceeded\n");
					mmod->memerror = 2;
				}
			}
			break;
		}
		mmod->memptr++;
	}
	if (incrnmemblk == 1) {
		mmod->nmemblk++;
		mmod->memptrend++;
		if (mmod->nmemblk >=
		    ((TOTALMEMORYSIZE/BLKSPACEDIV)/sizeof(struct memblk))) {
			printf("SMC GENMALLOC exceeded block limit of %ld\n",((TOTALMEMORYSIZE/BLKSPACEDIV)/sizeof(struct memblk)));
			mmod->memerror = 3;
		}
	}
	if (foundmem == false) {
		printf("ERROR: SMC GENMALLOC did not find memory region, size is %u\n",rsize);
		mmod->memerror = 4;
	}

/*
 * Debug functions
 */

#ifdef DEBUG_SMC_MALLOC
	if (mmod->checkadd == 1) {
		for (int i = 0; i < mmod->checknumentries; i++) {
			if (((mmod->mallocdeladd[mallocdeladd_pos] >
			      mmod->checksa[i])
			     && (mmod->mallocdeladd[mallocdeladd_pos] <
				 mmod->checkea[i]))
			    || (((mmod->mallocdeladd[mallocdeladd_pos] + rsize) >
				 mmod->checksa[i])
				&& ((mmod->mallocdeladd[mallocdeladd_pos] + rsize) <
				    mmod->checkea[i]))) {
				printf("ERROR: found overlap with previous addressin smc GENMALLOC\n");
				printf("New address %u size %u\n",mmod->mallocdeladd[mallocdeladd_pos], rsize);
				printf("Conflicting address %u size %u\n",mmod->checksa[i],(mmod->checkea[i] - mmod->checksa[i]));
				mmod->memerror = 5;
			}
		}
		mmod->checksa[mmod->checknumentries] =
			mmod->mallocdeladd[mallocdeladd_pos];
		mmod->checkea[mmod->checknumentries] =
			mmod->mallocdeladd[mallocdeladd_pos] + rsize;
		mmod->checknumentries++;
		if (mmod->checknumentries >= (4*mmod->maxmemblk)) {
			printf("ERROR: check queue size exceeded\n");mmod->memerror = 6;
		}
		mmod->memptr = (void *)mmod->memory;
		for (int i = 0; i < mmod->nmemblk; i++) {
			if (mmod->memptr->valid == 1) {
				if (((mmod->mallocdeladd[mallocdeladd_pos] >
				      mmod->memptr->address)
				     && (mmod->mallocdeladd[mallocdeladd_pos] < (mmod->memptr->address + mmod->memptr->size)))
				    || (((mmod->mallocdeladd[mallocdeladd_pos] + rsize) > mmod->memptr->address)
					&& ((mmod->mallocdeladd[mallocdeladd_pos] +
					     rsize) < (mmod->memptr->address + mmod->memptr->size)))) {
					printf("ERROR: found overlap with GENFREE memory region in smc GENMALLOC\n");
					printf("New address %u size %u\n",mmod->mallocdeladd[mallocdeladd_pos],rsize);
					printf("Conflicting address %u size %u\n",mmod->memptr->address,mmod->memptr->size);
					mmod->memerror = 7;
				}
			}
			mmod->memptr++;
		}
		for (unsigned int i = 0; i < mmod->cntdeladd; i++) {
			if (mmod->mallocdeladd_valid[i] == 1) {
				mmod->memptr = (void *)mmod->memory;
				for (int j = 0; j < mmod->nmemblk; j++) {
					if (mmod->memptr->valid == 1) {
						if (((mmod->mallocdeladd[i] >
						      mmod->memptr->address)
						     && (mmod->mallocdeladd[i] < (mmod->memptr->address + mmod->memptr->size)))
						    || (((mmod->mallocdeladd[i] + mmod->memallocsize[i]) > mmod->memptr->address)
							&& ((mmod->mallocdeladd[i] + mmod->memallocsize[i]) < (mmod->memptr->address + mmod->memptr->size))))
						{
							printf("ERROR: found overlap with GENFREE memory region full search in smc GENMALLOC\n");
							printf("New address %u size %u\n",mmod->mallocdeladd[i],mmod->memallocsize[i]);
							printf("Conflicting address %u size %u\n",mmod->memptr->address,mmod->memptr->size);
							mmod->memerror = 8;
						}
					}
					mmod->memptr++;
				}
			}
		}
		mmod->memptr = (void *)mmod->memory;
		newblk = (void *)mmod->memory;
		for (unsigned int i = 0; i < mmod->nmemblk; i++) {
			if (mmod->memptr->valid == 1) {
				for (unsigned int j = 0; j < mmod->nmemblk; j++) {
					if (newblk->valid == 1) {
						if (((mmod->memptr->address >
						      newblk->address) && (mmod->memptr->address < (newblk->address + newblk->size)))
						    || (((mmod->memptr->address + mmod->memptr->size) >
							 newblk->address) && ((mmod->memptr->address + 
							 mmod->memptr->size) < (newblk->address + newblk->size)))) {
							printf("ERROR: found overlap in GENFREE memory regions in smc GENMALLOC\n");
							printf("Region 1 address %u size %u\n",mmod->memptr->address,mmod->memptr->size);
							printf("Region 2 address %u size %u\n",newblk->address,newblk->size);
							mmod->memerror = 9;
						}
					}
					newblk++;
				}
			}
			mmod->memptr++;
			newblk = (void *)mmod->memory;
		}
	}
#endif
	return (void *)mmod->memory + ((TOTALMEMORYSIZE/BLKSPACEDIV)) +
	       mmod->mallocdeladd[mallocdeladd_pos];
#ifdef DEBUG_SMC_MALLOC
	return (void*)mmod->memory + 0x100 + mmod->mallocdeladd[mallocdeladd_pos];
#endif
}

/*
 * Memory free function for meemory allocated from malloc function.  
 * The memmod structure is
 * required to represent memory image
 */

int smcfree(void *faddptr,
	    struct memmod *mmod)
{
	unsigned int fadd = faddptr - ((TOTALMEMORYSIZE/BLKSPACEDIV)) -
			    (void *)mmod->memory;
	int fentry = 0;
	struct memblk *newblk = NULL;
	int incrnmemblk = 0;

	/*
	  * Scrolling through the malloc attribute table to find entries that match
	  * the user supplied address
	  */


	for (unsigned int i = 0; i < mmod->cntdeladd; i++) {
		if ((fadd == mmod->mallocdeladd[i]) &&
		    (mmod->mallocdeladd_valid[i] == 1)) {
			fentry = 1;
			if (mmod->trailblock[i] != NULL) {
				if ((mmod->precblock[i]->address + mmod->precblock[i]->size) == fadd) {
					/*
					     * Found matching attribute block and then proceed to merge with
					     * surrounding blocks
					     */

					mmod->precblock[i]->size += mmod->memallocsize[i] + mmod->trailblock[i]->size;
					mmod->memblkqueue[mmod->ptrmemblkqueue] = mmod->trailblock[i];
					mmod->ptrmemblkqueue++;
					if (mmod->ptrmemblkqueue >= mmod->maxmemblk) {
						printf("ERROR: GENMALLOC size exceeded in memory block queue\n");
						exit(1);
					}
					mmod->trailblock[i]->valid = 0;
					newblk = mmod->precblock[i];
					mmod->memptr = (void *)mmod->memory;

					/*
					      * Scrolling through the malloc attribute table to find entries that have values that
					      * match the newly merged block and replace them with it
					      */

					for (int j = 0; j < mmod->nmemblk; j++) {
						if (mmod->memptr->valid == 1) {
							if ((mmod->trailblock[i]->address + mmod->trailblock[i]->size) == mmod->memptr->address) {
								if ((mmod->memptr->size == 0) &&
								    (mmod->trailblock[i]->size != 0)) {
									mmod->memptr->valid = 0;
									mmod->memblkqueue[mmod->ptrmemblkqueue] = mmod->memptr;
									mmod->ptrmemblkqueue++;
									if (mmod-> ptrmemblkqueue >= mmod-> maxmemblk)
									{
										printf("ERROR: GENMALLOC size exceeded in memory block queue\n");
										exit(
											1);
									}
								}
							}
						}
						mmod->memptr++;
					}
				}
			}

			/*
			    * Setting table parameters
			    */

			mmod->mallocdeladd_valid[i] = 0;
			mmod->mallocdeladd_queue[mmod->mallocdeladd_queue_cnt] = i;
			mmod->mallocdeladd_queue_cnt++;
			if (mmod->mallocdeladd_queue_cnt >= mmod->maxmemblk) {
				printf("ERROR: GENMALLOC reuse queue size exceeded\n");
				exit(1);
			}

			/*
			    * Scrolling through the malloc attribute table to find entries 
			    * that have values that
			    * match the newly merged block and replace them with it
			    */

			unsigned int faddGENFREE = newblk->address + newblk->size;
			for (unsigned int j = 0; j < mmod->cntdeladd; j++) {
				if ((faddGENFREE == mmod->mallocdeladd[j]) &&
				    (mmod->mallocdeladd_valid[j] == 1))
					mmod->precblock[j] = newblk;
				if ((faddGENFREE ==
				     (mmod->mallocdeladd[j] +
				      mmod->memallocsize[i])) &&
				    (mmod->mallocdeladd_valid[j] == 1))
					mmod->trailblock[j] = newblk;
			}
		}
	}
	if (incrnmemblk == 1) {
		mmod->nmemblk++;
		mmod->memptrend++;
		if (mmod->nmemblk >=
		    ((TOTALMEMORYSIZE/BLKSPACEDIV)/sizeof(struct memblk))) {
			printf("SMC GENFREE exceeded block limit of %ld\n",((TOTALMEMORYSIZE/BLKSPACEDIV)/sizeof(struct memblk)));
			exit(1);
		}
	}
	if (fentry == 0) {
		printf("ERROR: smcGENFREE cannot find address to GENFREE %u\n",fadd);
		exit(1);
	}
#ifdef DEBUG_SMC_MALLOC

/*
 * Debug functions
 */

	if (mmod->checkadd == 1) {
		for (int i = 0; i < mmod->checknumentries; i++) {
			if (fadd == mmod->checksa[i]) {
				mmod->checksa[i] = 0;
				mmod->checkea[i] = 0;
			}
		}
		mmod->memptr = (void *)mmod->memory;
		newblk = (void *)mmod->memory;
		for (unsigned int i = 0; i < mmod->nmemblk; i++) {
			if (mmod->memptr->valid == 1) {
				for (unsigned int j = 0; j < mmod->nmemblk; j++) {
					if (newblk->valid == 1) {
						if (((mmod->memptr->address > newblk->address)
						     && (mmod->memptr->address < (newblk->address + newblk->size)))
						    || (((mmod->memptr->address + mmod->memptr->size) > newblk->address)
							&& ((mmod->memptr->address + mmod->memptr->size) < ((newblk->address + newblk->size))))) {
							printf("ERROR: found overlap in GENFREE memory regions in smc GENMALLOC\n");
							printf("Region 1 address %u size %u\n",mmod->memptr->address,mmod->memptr->size);
							printf("Region 2 address %u size %u\n",newblk->address,newblk->size);
						}
					}
					newblk++;
				}
			}
			mmod->memptr++;
			newblk = (void *)mmod->memory;
		}
	}
#endif
	return 0;
}

/*
 * Diplay malloc tables for debug purposes
 */

#ifdef DEBUG_SMC_MALLOC
void displayblocks(struct memmod *mmod)
{
	mmod->memptr = (void *)mmod->memory;
	printf("Displaying blocks:\n");
	for (unsigned int i = 0; i < mmod->nmemblk; i++) {
		if (mmod->memptr->valid == 1) {
			printf("*********************************************************************************************\n");
			printf("%u * Address: %u * Size: %u * Valid: %u *\n",i,mmod->memptr->address,mmod->memptr->size,mmod->memptr->valid);
		}
		mmod->memptr++;
	}
}

void displaymalloctable(struct memmod *mmod)
{
	printf("\n\nDisplaying GENMALLOC table\n");
	for (unsigned int i = 0; i < mmod->cntdeladd; i++) {
		if (mmod->mallocdeladd_valid[i] == 1) {
			printf("**********************************************************************************************\n");
			printf("GENMALLOC Address: %u\n",mmod->mallocdeladd[i]);
			printf("**********************************************************************************************\n");
			printf("GENMALLOC Size: %u\n",mmod->memallocsize[i]);
			printf("**********************************************************************************************\n");
			if (mmod->trailblock[i] != NULL) {
				printf("Trail Block:\n");
				printf("* Address: %u * Size: %u *\n",
				       mmod->trailblock[i]->address,
				       mmod->trailblock[i]->size);
			}
			printf("**********************************************************************************************\n");
			if (mmod->precblock[i] != NULL) {
				printf("Previous Block:\n");
				printf("* Address: %u * Size: %u *\n",
				       mmod->precblock[i]->address,
				       mmod->precblock[i]->size);
			}
			printf("**********************************************************************************************\n\n\n");
		}
	}
}
#endif

/*
 * switch to use either standard c malloc or custom smc malloc
 */

#define FIRST_NODE_DEVTREE_OFFSET 8

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
struct propval { unsigned int len;
		 unsigned int nameoff;};

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
 * Data structure to house device tree data before processing to nodes
 */
struct fifo3d {
	char ***nnfifo;
	char ***fnamefifo;
	int **biasfifo;
	int col;
	int currcol;
	int *row;
};

/*
 * Push function name string into raw data structure
 */
void push3dfifofname(struct fifo3d *f3d,
		     char *fname)
{
	strlcpy(f3d->fnamefifo[f3d->col - 1][f3d->row[f3d->col - 1] - 1],
	fname,50);
}

/*
 * Push bias value into raw data structure
 */
void push3dfifobias(struct fifo3d *f3d,
		    int bias)
{
	f3d->biasfifo[f3d->col - 1][f3d->row[f3d->col - 1] - 1] = bias;
}

/*
 * Create new column and/or row for raw data structure for newly
 * found node from device tree
 */
void push3dfifocol(struct fifo3d *f3d,
		   char *entry,
		   struct memmod *mmod)
{
	char ***tnnfifo;
	char ***tfnamefifo;
	int **tbiasfifo;

	if (f3d->col == f3d->currcol) {
		f3d->col++;
		f3d->currcol++;
		int *trow;
		trow = GENMALLOC(f3d->col * sizeof(int) );

		/*
		  * return if error found
		  */
		if (mmod->memerror != 0)
			return;

		for (int i = 0; i < f3d->col - 1; i++)
			trow[i] = f3d->row[i];
		if (f3d->col > 1)
			GENFREE(f3d->row);
		f3d->row = trow;
		f3d->row[f3d->col - 1] = 1;

		/*
		  * Create new raw data memory
		  */
		tnnfifo = GENMALLOC(f3d->col * sizeof(char **) );
		tfnamefifo = GENMALLOC(f3d->col * sizeof(char **) );
		tbiasfifo = GENMALLOC( (f3d->col) * sizeof(int *) );
		for (int i = 0; i < f3d->col; i++) {
			tnnfifo[i] = GENMALLOC(f3d->row[i] * sizeof(char *) );
			tfnamefifo[i] = GENMALLOC(f3d->row[i] * sizeof(char *) );
			tbiasfifo[i] = GENMALLOC( (f3d->row[i]) * sizeof(int) );
			for (int j = 0; j < f3d->row[i]; j++) {
				tnnfifo[i][j] = GENMALLOC(1 * sizeof(char[50] ) );
				tfnamefifo[i][j] =
					GENMALLOC(1 * sizeof(char[50] ) );
				if (!((j == f3d->row[f3d->col - 1] - 1) &&
				      (i == (f3d->col - 1)))) {
					strlcpy(tnnfifo[i][j],f3d->nnfifo[i][j],50);
					strlcpy(tfnamefifo[i][j],
						f3d->fnamefifo[i][j],50);
					tbiasfifo[i][j] = f3d->biasfifo[i][j];
				}
			}
		}

		/*
		  * Copy data from old raw data to new memory location
		  */
		strlcpy(tnnfifo[f3d->col - 1][f3d->row[f3d->col - 1] - 1], entry,
			50);
		strlcpy(tfnamefifo[f3d->col - 1][f3d->row[f3d->col - 1] - 1],
			"none",50);
		tbiasfifo[f3d->col - 1][f3d->row[f3d->col - 1] - 1] = 0;

		/*
		   * Free the old raw data structres
		   */
		for (int i = 0; i < f3d->col - 1; i++) {
			for (int j = 0; j < f3d->row[i]; j++) {
				GENFREE(f3d->nnfifo[i][j]);
				GENFREE(f3d->fnamefifo[i][j]);
			}
			GENFREE(f3d->nnfifo[i]);
			GENFREE(f3d->fnamefifo[i]);
			GENFREE(f3d->biasfifo[i]);
		}
		if (f3d->col > 1) {
			GENFREE(f3d->nnfifo);
			GENFREE(f3d->fnamefifo);
			GENFREE(f3d->biasfifo);
		}

		/*
		  * Point to new data
		  */
		f3d->nnfifo = tnnfifo;
		f3d->fnamefifo = tfnamefifo;
		f3d->biasfifo = tbiasfifo;
	}
	if (f3d->col != f3d->currcol) {
		/*
		  *  Adding new node to raw data
		  */
		f3d->col++;
		f3d->row[f3d->col - 1]++;

		/*
		  * Create new raw data memory
		  */
		tnnfifo = GENMALLOC(f3d->col * sizeof(char **) );
		tfnamefifo = GENMALLOC(f3d->col * sizeof(char **) );
		tbiasfifo = GENMALLOC( (f3d->col) * sizeof(int *) );
		for (int i = 0; i < f3d->col; i++) {
			tnnfifo[i] = GENMALLOC(f3d->row[i] * sizeof(char *) );
			tfnamefifo[i] = GENMALLOC(f3d->row[i] * sizeof(char *) );
			tbiasfifo[i] = GENMALLOC( (f3d->row[i]) * sizeof(int) );
			for (int j = 0; j < f3d->row[i]; j++) {
				tnnfifo[i][j] = GENMALLOC(1 * sizeof(char[50] ) );
				tfnamefifo[i][j] =
					GENMALLOC(1 * sizeof(char[50] ) );
				if (!((j == f3d->row[f3d->col - 1] - 1) &&
				      (i == (f3d->col - 1)))) {
					strlcpy(tnnfifo[i][j],f3d->nnfifo[i][j],50);
					strlcpy(tfnamefifo[i][j],
						f3d->fnamefifo[i][j],50);
					tbiasfifo[i][j] = f3d->biasfifo[i][j];
				}
			}
		}

		/*
		  * Copy data from old raw data to new memory location
		  */
		strlcpy(tnnfifo[f3d->col - 1][f3d->row[f3d->col - 1] - 1], entry,
			50);
		strlcpy(tfnamefifo[f3d->col - 1][f3d->row[f3d->col - 1] - 1],
			"none",50);
		tbiasfifo[f3d->col - 1][f3d->row[f3d->col - 1] - 1] = 0;

		/*
		  * Free the old raw data structres
		  */
		for (int i = 0; i < f3d->col; i++) {
			for (int j = 0; j < f3d->row[i]; j++) {
				if (!((i == f3d->col - 1) &&
				      (j == f3d->row[i] - 1))) {
					GENFREE(f3d->nnfifo[i][j]);
					GENFREE(f3d->fnamefifo[i][j]);
				}
			}
			GENFREE(f3d->nnfifo[i]);
			GENFREE(f3d->fnamefifo[i]);
			GENFREE(f3d->biasfifo[i]);
		}
		GENFREE(f3d->nnfifo);
		GENFREE(f3d->fnamefifo);
		GENFREE(f3d->biasfifo);

		/*
		  * Point to new data
		  */
		f3d->nnfifo = tnnfifo;
		f3d->fnamefifo = tfnamefifo;
		f3d->biasfifo = tbiasfifo;
	}
}

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

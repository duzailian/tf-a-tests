/*
 * Copyright (c) 2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

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

#ifdef SMC_FUZZ_TMALLOC
#define GENMALLOC(x)	malloc((x))
#define GENFREE(x)	free((x))
#else
#define GENMALLOC(x)	smcmalloc((x),mmod)
#define GENFREE(x)	smcfree((x),mmod)
#endif

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

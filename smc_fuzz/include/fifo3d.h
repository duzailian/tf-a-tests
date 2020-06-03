/*
 * Copyright (c) 2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef __FIFO3D__
#define  __FIFO3D__

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "smcmalloc.h"


struct fifo3d {
	char ***nnfifo;
	char ***fnamefifo;
	int **biasfifo;
	int col;
	int curr_col;
	int *row;
};

/*
 * Push function name string into raw data structure
 */
void push_3dfifo_fname(struct fifo3d *,char *);

/*
 * Push bias value into raw data structure
 */
void push_3dfifo_bias(struct fifo3d *,int);

/*
 * Create new column and/or row for raw data structure for newly
 * found node from device tree
 */
void push_3dfifo_col(struct fifo3d *,char *,struct memmod *);

#endif /* __FIFO3D__ */

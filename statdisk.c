/*
 * (C) 2017, Cornell University
 * All rights reserved.
 */

/* Author: Robbert van Renesse, August 2015
 *
 * This block store module simply forwards its method calls to an
 * underlying block store, but keeps track of statistics.
 *
 *		block_store_t *statdisk_init(block_store_t *below){
 *			'below' is the underlying block store.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "block_store.h"

struct statdisk_state {
	block_store_t *below;			// block store below
	unsigned int nnblocks;	// #nblocks operations
	unsigned int nsetsize;	// #nblocks operations
	unsigned int nread;		// #read operations
	unsigned int nwrite;	// #write operations
};

static int statdisk_nblocks(block_store_t *this_bs){
	struct statdisk_state *sds = this_bs->state;

	sds->nnblocks++;
	return (*sds->below->nblocks)(sds->below);
}

static int statdisk_setsize(block_store_t *this_bs, block_no nblocks){
	struct statdisk_state *sds = this_bs->state;

	sds->nsetsize++;
	return (*sds->below->setsize)(sds->below, nblocks);
}

static int statdisk_read(block_store_t *this_bs, block_no offset, block_t *block){
	struct statdisk_state *sds = this_bs->state;

	sds->nread++;
	return (*sds->below->read)(sds->below, offset, block);
}

static int statdisk_write(block_store_t *this_bs, block_no offset, block_t *block){
	struct statdisk_state *sds = this_bs->state;

	sds->nwrite++;
	return (*sds->below->write)(sds->below, offset, block);
}

static void statdisk_destroy(block_store_t *this_bs){
	free(this_bs->state);
	free(this_bs);
}

void statdisk_dump_stats(block_store_t *this_bs){
	struct statdisk_state *sds = this_bs->state;

	printf("!$STAT: #nnblocks:  %u\n", sds->nnblocks);
	printf("!$STAT: #nsetsize:  %u\n", sds->nsetsize);
	printf("!$STAT: #nread:     %u\n", sds->nread);
	printf("!$STAT: #nwrite:    %u\n", sds->nwrite);
}

block_store_t *statdisk_init(block_store_t *below){
	/* Create the block store state structure.
	 */
	struct statdisk_state *sds = calloc(1, sizeof(*sds));
	sds->below = below;

	/* Return a block interface to this inode.
	 */
	block_store_t *this_bs = calloc(1, sizeof(*this_bs));
	this_bs->state = sds;
	this_bs->nblocks = statdisk_nblocks;
	this_bs->setsize = statdisk_setsize;
	this_bs->read = statdisk_read;
	this_bs->write = statdisk_write;
	this_bs->destroy = statdisk_destroy;
	return this_bs;
}

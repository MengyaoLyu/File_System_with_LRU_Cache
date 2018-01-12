/*
 * (C) 2017, Cornell University
 * All rights reserved.
 */

/* Author: Robbert van Renesse, August 2015
 *
 * This code implements a block store stored in memory
 * file system:
 *
 *		block_store_t *ramdisk_init(block_t *blocks, block_no nblocks)
 *			Create a new block store, stored in the array of blocks
 *			pointed to by 'blocks', which has nblocks blocks in it.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "block_store.h"

struct ramdisk_state {
	block_t *blocks;
	block_no nblocks;
	int fd;
};

static int ramdisk_nblocks(block_store_t *this_bs){
	struct ramdisk_state *rs = this_bs->state;

	return rs->nblocks;
}

static int ramdisk_setsize(block_store_t *this_bs, block_no nblocks){
	struct ramdisk_state *rs = this_bs->state;

	int before = rs->nblocks;
	rs->nblocks = nblocks;
	return before;
}

static int ramdisk_read(block_store_t *this_bs, block_no offset, block_t *block){
	struct ramdisk_state *rs = this_bs->state;

	if (offset >= rs->nblocks) {
		fprintf(stderr, "ramdisk_read: bad offset %u\n", offset);
		return -1;
	}
	memcpy(block, &rs->blocks[offset], BLOCK_SIZE);
	return 0;
}

static int ramdisk_write(block_store_t *this_bs, block_no offset, block_t *block){
	struct ramdisk_state *rs = this_bs->state;

	if (offset >= rs->nblocks) {
		fprintf(stderr, "ramdisk_write: bad offset\n");
		return -1;
	}
	memcpy(&rs->blocks[offset], block, BLOCK_SIZE);
	return 0;
}

static void ramdisk_destroy(block_store_t *this_bs){
	free(this_bs->state);
	free(this_bs);
}

block_store_t *ramdisk_init(block_t *blocks, block_no nblocks){
	struct ramdisk_state *rs = calloc(1, sizeof(*rs));

	rs->blocks = blocks;
	rs->nblocks = nblocks;

	block_store_t *this_bs = calloc(1, sizeof(*this_bs));
	this_bs->state = rs;
	this_bs->nblocks = ramdisk_nblocks;
	this_bs->setsize = ramdisk_setsize;
	this_bs->read = ramdisk_read;
	this_bs->write = ramdisk_write;
	this_bs->destroy = ramdisk_destroy;
	return this_bs;
}

/*
 * (C) 2017, Cornell University
 * All rights reserved.
 */

/* Author: Robbert van Renesse, August 2015
 *
 * This code implements a block store on top of the underlying POSIX
 * file system:
 *
 *		block_store_t *disk_init(char *file_name, block_no nblocks)
 *			Create a new block store, stored in the file by the given
 *			name and with the given number of blocks.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "block_store.h"

struct disk_state {
	block_no nblocks;			// #blocks in the block store
	int fd;						// POSIX file descriptor of underlying file
};

static int disk_nblocks(block_store_t *this_bs){
	struct disk_state *ds = this_bs->state;

	return ds->nblocks;
}

static int disk_setsize(block_store_t *this_bs, block_no nblocks){
	struct disk_state *ds = this_bs->state;

	int before = ds->nblocks;
	ds->nblocks = nblocks;
	ftruncate(ds->fd, (off_t) nblocks * BLOCK_SIZE);
	return before;
}

static struct disk_state *disk_seek(block_store_t *this_bs, block_no offset){
	struct disk_state *ds = this_bs->state;

	if (offset >= ds->nblocks) {
		fprintf(stderr, "--> %u %u\n", offset, ds->nblocks);
		panic("disk_seek: offset too large");
	}
	lseek(ds->fd, (off_t) offset * BLOCK_SIZE, SEEK_SET);
	return ds;
}

static int disk_read(block_store_t *this_bs, block_no offset, block_t *block){
	struct disk_state *ds = disk_seek(this_bs, offset);

	int n = read(ds->fd, (void *) block, BLOCK_SIZE);
	if (n < 0) {
		perror("disk_read");
		return -1;
	}
	if (n < BLOCK_SIZE) {
		memset((char *) block + n, 0, BLOCK_SIZE - n);
	}
	return 0;
}

static int disk_write(block_store_t *this_bs, block_no offset, block_t *block){
	struct disk_state *ds = disk_seek(this_bs, offset);

	int n = write(ds->fd, (void *) block, BLOCK_SIZE);
	if (n < 0) {
		perror("disk_write");
		return -1;
	}
	if (n != BLOCK_SIZE) {
		fprintf(stderr, "disk_write: wrote only %d bytes\n", n);
		return -1;
	}
	return 0;
}

static void disk_destroy(block_store_t *this_bs){
	struct disk_state *ds = this_bs->state;

	close(ds->fd);
	free(ds);
	free(this_bs);
}

block_store_t *disk_init(char *file_name, block_no nblocks){
	struct disk_state *ds = calloc(1, sizeof(*ds));

	ds->fd = open(file_name, O_RDWR | O_CREAT, 0600);
	if (ds->fd < 0) {
		perror(file_name);
		panic("disk_init");
	}
	ds->nblocks = nblocks;

	block_store_t *this_bs = calloc(1, sizeof(*this_bs));
	this_bs->state = ds;
	this_bs->nblocks = disk_nblocks;
	this_bs->setsize = disk_setsize;
	this_bs->read = disk_read;
	this_bs->write = disk_write;
	this_bs->destroy = disk_destroy;
	return this_bs;
}

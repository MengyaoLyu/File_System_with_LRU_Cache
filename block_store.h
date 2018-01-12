/*
 * (C) 2017, Cornell University
 * All rights reserved.
 */

/* Author: Robbert van Renesse, August 2015
 *
 * The include file for all block store modules.  Each such module has an
 * 'init' function that returns a block_store_t *.  The block_store_t * is
 * a pointer to a structure that contains the following five methods:
 *
 *		int nblocks(block_store_t *this_bs)
 *			returns the size of the block store
 *
 *		int setsize(block_store_t *this_bs, block_no newsize)
 *			set the size of the block store; returns the old size
 *
 *		int read(block_store_t *this_bs, block_no offset, block_t *block)
 *			read the block at offset and return in *block
 *			returns 0
 *
 *		int write(block_store_t *this_bs, block_no offset, block_t *block)
 *			write *block to the block at the given offset
 *			returns 0
 *
 *		void destroy(block_store_t *this_bs)
 *			clean up the block store interface;	returns 0
 *
 * All these return -1 upon error (typically after printing the
 * reason for the error).
 *
 * A 'block_t' is a block of BLOCK_SIZE bytes.  A block store is an array
 * of blocks.  A 'block_no' holds the index of the block in the block store.
 *
 * A block_store_t * also maintains a void* pointer called 'state' to internal
 * state the block store module needs to keep.
 */

#define BLOCK_SIZE		512			// # bytes in a block

typedef unsigned int block_no;		// index of a block

typedef struct block {
	char bytes[BLOCK_SIZE];
} block_t;

typedef struct block_store {
	void *state;
	int (*nblocks)(struct block_store *this_bs);
	int (*read)(struct block_store *this_bs, block_no offset, block_t *block);
	int (*write)(struct block_store *this_bs, block_no offset, block_t *block);
	int (*setsize)(struct block_store *this_bs, block_no size);
	void (*destroy)(struct block_store *this_bs);
} block_store_t;

/* Convenient function for error handling.
 */
void panic(char *s);

/* Each block store module has an 'init' function that returns a
 * 'block_store_t *' type.  Here are the 'init' functions of various
 * available block store types.
 */
block_store_t *ramdisk_init(block_t *blocks, block_no nblocks);
block_store_t *treedisk_init(block_store_t *below, unsigned int inode_no);
block_store_t *debugdisk_init(block_store_t *below, char *descr);
block_store_t *cachedisk_init(block_store_t *below, block_t *blocks, block_no nblocks);
block_store_t *statdisk_init(block_store_t *below);
block_store_t *checkdisk_init(block_store_t *below, char *descr);
block_store_t *tracedisk_init(block_store_t *below, char *trace, unsigned int n_inodes);

/* Some useful functions on some block store types.
 */
int treedisk_create(block_store_t *below, unsigned int n_inodes);
int treedisk_check(block_store_t *below);
void statdisk_dump_stats(block_store_t *this_bs);

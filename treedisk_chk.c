/*
 * (C) 2017, Cornell University
 * All rights reserved.
 */

/* Author: Robbert van Renesse, August 2015
 *
 * Code to check the integrity of a treedisk file system.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "block_store.h"
#include "treedisk.h"

static unsigned int log_rpb;		// log2(REFS_PER_BLOCK)

struct block_info {
	enum { BI_UNKNOWN, BI_SUPER, BI_INODE, BI_INDIR, BI_DATA, BI_FREELIST, BI_FREE } status;
};

/* Stupid ANSI C compiler leaves shifting by #bits in unsigned int or more
 * undefined, but the result should clearly be 0...
 */
static block_no log_shift_r(block_no x, unsigned int nbits){
	if (nbits >= sizeof(block_no) * 8) {
		return 0;
	}
	return x >> nbits;
}

static int check_inode(block_store_t *below, block_no nblocks, block_no node, unsigned int nlevels, block_no offset, block_no fs_nblocks, struct block_info *binfo){
	/* Basic sanity checks.
	 */
	if (node == 0) {
		return 1;
	}
	if (node >= fs_nblocks) {
		fprintf(stderr, "!!TDERR: --> %u %u %u\n", node, fs_nblocks, offset);
		fprintf(stderr, "!!TDCHK: block off the underlying file system\n");
		return 0;
	}
	if (binfo[node].status != BI_UNKNOWN) {
		fprintf(stderr, "!!TDCHK: data block already used\n");
		return 0;
	}

	/* If it's a data block, mark that and return.
	 */
	if (nlevels == 0) {
		binfo[node].status = BI_DATA;
		return 1;
	}

	/* Scan the tree below.  Start with getting the indirect block and
	 * calculate the size of the tree below.
	 */
	binfo[node].status = BI_INDIR;
	struct treedisk_indirblock ib;
	(*below->read)(below, node, (block_t *) &ib);
	nlevels--;
	block_no size = 1 << (nlevels * log_rpb);

	/* Now scan through the block references.
	 */
	unsigned int i;
	for (i = 0; i < REFS_PER_BLOCK; i++) {
		if (!check_inode(below, nblocks, ib.refs[i], nlevels, offset, fs_nblocks, binfo)) {
			return 0;
		}
		offset += size;
		if (offset >= nblocks) {
			break;
		}
	}

	return 1;
}

int treedisk_check(block_store_t *below){
	struct block_info *binfo = 0;
	block_no fs_nblocks = (*below->nblocks)(below);
	block_no b;

	if (fs_nblocks == 0) {
		fprintf(stderr, "!!TDCHK: empty underlying storage\n");
		return 0;
	}

	/* Calculate log2(REFS_PER_BLOCK).
	 */
	log_rpb = 0;
	do {
		log_rpb++;
	} while (((REFS_PER_BLOCK - 1) >> log_rpb) != 0);

	/* Get the superblock.
	 */
	union treedisk_block superblock;
	(*below->read)(below, 0, (block_t *) &superblock);

	/* Check the superblock.
	 */
	if (1 + superblock.superblock.n_inodeblocks > fs_nblocks) {
		fprintf(stderr, "!!TDERR: %u %u\n", superblock.superblock.n_inodeblocks, fs_nblocks);
		fprintf(stderr, "!!TDCHK: not enough room for inode blocks\n");
		return 0;
	}
	if (superblock.superblock.free_list >= fs_nblocks) {
		fprintf(stderr, "!!TDCHK: free list ref in superblock too large\n");
		return 0;
	}

	/* Initialie the block info.
	 */
	binfo = (struct block_info *) calloc(fs_nblocks, sizeof(*binfo));
	binfo[0].status = BI_SUPER;
	for (b = 1; b <= superblock.superblock.n_inodeblocks; b++) {
		binfo[b].status = BI_INODE;
	}

	/* Scan the inode blocks.
	 */
	struct treedisk_inodeblock tib;
	for (b = 1; b <= superblock.superblock.n_inodeblocks; b++) {
		(*below->read)(below, b, (block_t *) &tib);

		/* Scan the inodes in the block.
		 */
		unsigned int i;
		for (i = 0; i < INODES_PER_BLOCK; i++) {
			struct treedisk_inode *ti = &tib.inodes[i];
			if (ti->nblocks != 0) {
				unsigned int nlevels = 0;
				while (log_shift_r(ti->nblocks - 1, nlevels * log_rpb) != 0) {
					nlevels++;
				}
				if (!check_inode(below, ti->nblocks, ti->root, nlevels, 0, fs_nblocks, binfo)) {
					free(binfo);
					return 0;
				}
			}
		}
	}
	
	/* Scan the free list.
	 */
	block_no fl = superblock.superblock.free_list;
	while (fl != 0) {
		if (fl >= fs_nblocks) {
			fprintf(stderr, "!!TDCHK: free list block number too large\n");
			free(binfo);
			return 0;
		}
		if (binfo[fl].status != BI_UNKNOWN) {
			fprintf(stderr, "!!TDCHK: free list block already in use\n");
			free(binfo);
			return 0;
		}
		binfo[fl].status = BI_FREELIST;

		/* Read the next block off the free list.
		 */
		struct treedisk_freelistblock tfb;
		(*below->read)(below, fl, (block_t *) &tfb);

		unsigned int i;
		for (i = 1; i < REFS_PER_BLOCK; i++) {
			if (tfb.refs[i] != 0) {
				if (tfb.refs[i] >= fs_nblocks) {
					continue;
				}
				if (binfo[tfb.refs[i]].status != BI_UNKNOWN) {
					fprintf(stderr, "!!TDERR: --> %u %u %d\n", fl, tfb.refs[i],
										binfo[tfb.refs[i]].status);
					fprintf(stderr, "!!TDCHK: duplicate block in free list\n");
					free(binfo);
					return 0;
				}
				binfo[tfb.refs[i]].status = BI_FREE;
			}
		}
		fl = tfb.refs[0];
	}

	/* Check the blocks.
	 */
	for (b = 0; b < fs_nblocks; b++) {
		if (binfo[b].status == BI_UNKNOWN) {
			fprintf(stderr, "!!TDLEAK: unaccounted for block %u\n", b);
			break;
		}
	}

	free(binfo);
	return 1;
}


/*
 * (C) 2017, Cornell University
 * All rights reserved.
 */

/* This block store module mirrors the underlying block store but contains
 * a write-through cache.
 *
 *      block_store_t *cachedisk_init(block_store_t *below,
 *                                  block_t *blocks, block_no nblocks)
 *          'below' is the underlying block store.  'blocks' points to
 *          a chunk of memory wth 'nblocks' blocks for caching.
 *          NO OTHER MEMORY MAY BE USED FOR STORING DATA.  However,
 *          malloc etc. may be used for meta-data.
 *
 *      void cachedisk_dump_stats(block_store_t *this_bs)
 *          Prints cache statistics.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "block_store.h"

/* State contains the pointer to the block module below as well as caching
 * information and caching statistics.
 */
struct cachedisk_state {
    block_store_t *below;               // block store below
    block_t *blocks;            // memory for caching blocks
    block_no nblocks;           // size of cache (not size of block store!)

    /* Stats.
     */
    unsigned int read_hit, read_miss, write_hit, write_miss;
};

static int cachedisk_nblocks(block_store_t *this_bs){
    struct cachedisk_state *cs = this_bs->state;

    return (*cs->below->nblocks)(cs->below);
}

static int cachedisk_setsize(block_store_t *this_bs, block_no nblocks){
    struct cachedisk_state *cs = this_bs->state;

    // This function is not complete, but you do not need to
    // implement it for this assignment.

    return (*cs->below->setsize)(cs->below, nblocks);
}

static int cachedisk_read(block_store_t *this_bs, block_no offset, block_t *block){
    struct cachedisk_state *cs = this_bs->state;

    // TODO: check the cache first.  Otherwise read from the underlying
    //       store and, if so desired, place the new block in the cache,
    //       possibly evicting a block if there is no space.

    cs->read_miss++;
    return (*cs->below->read)(cs->below, offset, block);
}

static int cachedisk_write(block_store_t *this_bs, block_no offset, block_t *block){
    struct cachedisk_state *cs = this_bs->state;

    // TODO: this is a write-through cache, so update the layer below.
    //       However, if the block is in the cache, it should be updated
    //       as well.

    cs->write_miss++;
    return (*cs->below->write)(cs->below, offset, block);
}

static void cachedisk_destroy(block_store_t *this_bs){
    struct cachedisk_state *cs = this_bs->state;

    // TODO: clean up any allocated meta-data.

    free(cs);
    free(this_bs);
}

void cachedisk_dump_stats(block_store_t *this_bs){
    struct cachedisk_state *cs = this_bs->state;

    printf("!$CACHE: #read hits:    %u\n", cs->read_hit);
    printf("!$CACHE: #read misses:  %u\n", cs->read_miss);
    printf("!$CACHE: #write hits:   %u\n", cs->write_hit);
    printf("!$CACHE: #write misses: %u\n", cs->write_miss);
}

/* Create a new block store module on top of the specified module below.
 * blocks points to a chunk of memory of nblocks blocks that can be used
 * for caching.
 */
block_store_t *cachedisk_init(block_store_t *below, block_t *blocks, block_no nblocks){
    /* Create the block store state structure.
     */
    struct cachedisk_state *cs = calloc(1, sizeof(*cs));
    cs->below = below;
    cs->blocks = blocks;
    cs->nblocks = nblocks;

    /* Return a block interface to this inode.
     */
    block_store_t *this_bs = calloc(1, sizeof(*this_bs));
    this_bs->state = cs;
    this_bs->nblocks = cachedisk_nblocks;
    this_bs->setsize = cachedisk_setsize;
    this_bs->read = cachedisk_read;
    this_bs->write = cachedisk_write;
    this_bs->destroy = cachedisk_destroy;
    return this_bs;
}
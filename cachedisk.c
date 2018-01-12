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

// a double linkedlist
struct LinkedList{
    block_no key;
    int cache_offset;
    struct LinkedList *pre;
    struct LinkedList *next;
};
//def DLL as double linkedlist pointer
typedef struct LinkedList *DLL;

struct cache {
    DLL head;
    DLL tail;
    block_no capacity;
    block_no cnt;
};
// A hash (Collection of pointers to Queue Nodes)
typedef struct Hash {
    int capacity; // how many pages can be there
    DLL *array; // an array of queue nodes
} Hash;

// A utility function to create an empty Hash of given capacity
Hash* createHash(int capacity ) {
    // Allocate memory for hash
    Hash* hash = (Hash *) malloc( sizeof( Hash) );
    hash->capacity = capacity;

    // Create an array of pointers for refering queue nodes
    hash->array = (DLL*) malloc( hash->capacity * sizeof( DLL ) );

    // Initialize all hash entries as empty
    int i;
    for( i = 0; i < hash->capacity; ++i )
        hash->array[i] = NULL;
    return hash;
}

DLL createNode(){
    DLL temp; // declare a node
    temp = (DLL) malloc(sizeof(struct LinkedList)); // allocate memory using malloc()
    temp->pre = NULL;
    temp->next = NULL;// make next point to NULL
    return temp;//return the new node
}

static struct cache *mycache;
static Hash* hashmap;

void init_cache() {
    mycache = malloc(sizeof(*mycache));
    mycache->cnt = 0;
    mycache->head = createNode();
    mycache->tail = createNode();
    mycache->head->key = -1;
    mycache->tail->key = -1; //unsigned ???
    mycache->head->next = mycache->tail;
    mycache->tail->pre = mycache->head;
}

// static DLL find(struct cache *mycache, block_no offset) {
//     DLL ptr = mycache->head;
//     while (ptr->next != NULL && ptr->key != offset) {
//         ptr = ptr->next;
//     }
//     //printf("go out");
//     return ptr;
// }

static int add_to_head(struct cache *mycache, DLL ptr) {
    DLL tmp_next = mycache->head->next;
    ptr->next = tmp_next;
    ptr->pre = mycache->head;
    tmp_next->pre = ptr;
    mycache->head->next = ptr;
    hashmap->array[ptr->key % hashmap->capacity] = ptr;
    return 0;
}

static int delete(DLL ptr) {
    DLL tmp_pre = ptr->pre;
    DLL tmp_next = ptr->next;
    tmp_pre->next = tmp_next;
    tmp_next->pre = tmp_pre;
    //need to free ptr???
    return 0;
}


static int cachedisk_read(block_store_t *this_bs, block_no offset, block_t *block){
    struct cachedisk_state *cs = this_bs->state;

    // TODO: check the cache first.  Otherwise read from the underlying
    //       store and, if so desired, place the new block in the cache,
    //       possibly evicting a block if there is no space.
    DLL node = hashmap->array[offset % hashmap->capacity];
    if (node == NULL || node->key != offset) {
        cs->read_miss++;
        (*cs->below->read)(cs->below, offset, block);
        if (mycache->cnt == mycache->capacity) {
            DLL evictNode = mycache->tail->pre;
            evictNode->key = offset;
            memcpy(&cs->blocks[evictNode->cache_offset], block, BLOCK_SIZE);
            delete(evictNode);
            add_to_head(mycache, evictNode);
            hashmap->array[offset % hashmap->capacity] = evictNode;
        } else {
            DLL newnode = createNode(); // need allocate???
            newnode->key = offset;
            newnode->cache_offset = mycache->cnt;
            memcpy(&cs->blocks[newnode->cache_offset], block, BLOCK_SIZE);
            mycache->cnt++;
            add_to_head(mycache, newnode);
            hashmap->array[offset % hashmap->capacity] = newnode;
        }

    } else {
        cs->read_hit++;
        //printf("here %i, %i\n", node->key, node->cache_offset);

        memcpy(block, &cs->blocks[node->cache_offset], BLOCK_SIZE);
        //memcpy(block, cs->blocks, BLOCK_SIZE);
        delete(node);
        add_to_head(mycache, node);
    }
    return 0;
}

static int cachedisk_write(block_store_t *this_bs, block_no offset, block_t *block){
    struct cachedisk_state *cs = this_bs->state;

    // TODO: check the cache first.  Otherwise read from the underlying
    //       store and, if so desired, place the new block in the cache,
    //       possibly evicting a block if there is no space.
    if ((*cs->below->write)(cs->below, offset, block) < 0 ) {
        return -1;
    }
    DLL node = hashmap->array[offset % hashmap->capacity];
    if (node == NULL || node->key != offset) {
        cs->write_miss++;
        if (mycache->cnt == mycache->capacity) {
            DLL evictNode = mycache->tail->pre;
            evictNode->key = offset;
            //printf("full%i\n", evictNode->cache_offset);
            memcpy(&cs->blocks[evictNode->cache_offset], block, BLOCK_SIZE);
            delete(evictNode);
            add_to_head(mycache, evictNode);
            hashmap->array[offset % hashmap->capacity] = evictNode;
            //memcpy(cs->blocks, block, BLOCK_SIZE);
            //memset(&cs->blocks + node->cache_offset, 0, BLOCK_SIZE);
            //free(evictNode);
            //(*cs->below->read)(cs->below, offset, &cs->blocks[node->cache_offset]);
        } else {
            DLL newnode = createNode(); // need allocate???
            newnode->key = offset;
            newnode->cache_offset = mycache->cnt;
            memcpy(&cs->blocks[newnode->cache_offset], block, BLOCK_SIZE);
            //memcpy(cs->blocks, block, BLOCK_SIZE);
            //(*cs->below->read)(cs->below, offset, &cs->blocks[mycache->cnt]);
            mycache->cnt++;
            //printf("new node offset, %i, %i\n", newnode->key, newnode->cache_offset);
            add_to_head(mycache, newnode);
            hashmap->array[offset % hashmap->capacity] = newnode;
        }

    } else {
        cs->write_hit++;
        //printf("here %i, %i\n", node->key, node->cache_offset);

        memcpy(&cs->blocks[node->cache_offset], block, BLOCK_SIZE);
        //memcpy(block, cs->blocks, BLOCK_SIZE);
        delete(node);
        add_to_head(mycache, node);
    }
    return 0;
}

void freeList(DLL head) {
    DLL tmp;
    while (head != NULL){
        tmp = head;
        head = head->next;
        free(tmp);
    }
}

void freeHash(Hash* hash) {
    free(hashmap->array);
}

static void cachedisk_destroy(block_store_t *this_bs){
    struct cachedisk_state *cs = this_bs->state;

    // TODO: clean up any allocated meta-data.
    freeHash(hashmap);
    freeList(mycache->head);
    free(hashmap);
    free(mycache);
    free(cs);
    free(this_bs);
}

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

    init_cache();
    mycache->capacity = nblocks;
    hashmap = createHash(nblocks);
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

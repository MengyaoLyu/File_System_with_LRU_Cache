/*
 * (C) 2017, Cornell University
 * All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "disk.c"

#define CACHE_SIZE   10 // #blocks in cache

block_t cache[CACHE_SIZE];


int main(){
  
  block_store_t * disk = disk_init("disk2.dev", 1024);
  block_store_t * sdisk = statdisk_init(disk);
  block_store_t * cdisk = cachedisk_init(sdisk, cache, CACHE_SIZE);
  
  block_t block;
  strcpy(block.bytes, "Farewell World!");
  (*cdisk->write)(cdisk, 0, &block);
  (*cdisk->destroy)(cdisk);
  (*sdisk->destroy)(sdisk);
  (*disk->destroy)(disk);
  
  return 0;
}

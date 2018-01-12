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

int main(){
	block_store_t *disk = disk_init("disk.dev", 1024);
	block_t block;
	strcpy(block.bytes, "Hello World");
	(*disk->write)(disk, 0, &block);
	(*disk->destroy)(disk);
	return 0;
}

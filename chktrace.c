/*
 * (C) 2017, Cornell University
 * All rights reserved.
 */

/* Author: Robbert van Renesse, November 2015
 *
 * Check the format of a trace.  Also keep track of stats
 *
 * Returns 0 if the trace is good
 * Returns 1 if the trace is not good
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INODES			128
#define MAX_BLOCKS			(1 << 27)
#define MAX_COMMANDS		10000

int main(int argc, char **argv){
	FILE *fp;
	char *file;

	if (argc == 1) {
		file = "standard input";
		fp = stdin;
	}
	else {
		file = argv[1];
		if ((fp = fopen(file, "r")) == 0) {
			perror(argv[1]);
			return 1;
		}
	}

	char cmd;
	unsigned int inode, bno, line = 0;
	unsigned int nread = 0, nwrite = 0, nsetsize = 0;
	for (;;) {
		int n = fscanf(fp, "%c:%u:%u\n", &cmd, &inode, &bno);
		if (n <= 0) {
			break;
		}
		if (n < 3) {
			fprintf(stderr, "format error in file %s, line %d\n", file, line);
			return 1;
		}
		line++;
		if (line > MAX_COMMANDS) {
			fprintf(stderr, "too many command in file %s, line %d\n", file, line);
			return 1;
		}
		if (inode >= MAX_INODES) {
			fprintf(stderr, "inode number too large in file %s, line %d\n", file, line);
			return 1;
		}
		if (bno >= MAX_BLOCKS) {
			fprintf(stderr, "block number too large in file %s, line %d\n", file, line);
			break;
		}
		switch (cmd) {
		case 'R':
			nread++;
			break;
		case 'W':
			nwrite++;
			break;
		case 'S':
			nsetsize++;
			break;
		case 'N':
			break;
		default:
			fprintf(stderr, "bad command '%c' in file %s, line %d\n", cmd, file, line);
			return 1;
		}
	}

	fclose(fp);

	printf("trace file %s: ncmds=%u, %%rd = %u, %%wr = %u, %%setsize=%u\n", file, line,
		(nread * 100 + 50) / line, (nwrite * 100 + 50) / line,
		(nsetsize * 100 + 50) / line
	);

	return 0;
}

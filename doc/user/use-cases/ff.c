/*
 * [LGPL]
 *
 * $Id$
 *
 * -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*-
 * vim: syntax=c sw=4 ts=4 sts=4:
 *
 */

/**
 * @file ff.c
 *
 * @version $Id$
 *
 * @brief Simplified dd(1)-like command line tool.
 *
 * @author Michael Iatrou
 *
 * @todo Take over the world.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <libgen.h>

#define _GNU_SOURCE
#include <string.h>

const char *usage = "usage: %s [options]\n"
	"options:\n"
	"  -c <bytes>	number of bytes to copy\n"
	"  -i <file>	input file\n"
	"  -o <file>	output file\n"
	"  -s <bytes>	seek number of bytes at start of output file\n"
	"  -S <bytes>	skip number of bytes at start of input file\n";

const char *runtime_options = "len = %ld\n"
	"fin = %s\n"
	"fout = %s\n"
	"seek = %ld\n"
	"skip = %ld\n";

int main(int argc, char **argv)
{
#if HAVE_BLESS
	bless_buffer_t *bufin, *bufout;
#endif

	int o;
	char *fin, *fout;
	int fdin, fdout;
	off_t len = -1, seek = 0, skip = 0;

	while ((o = getopt(argc, argv, "c:i:o:s:S:")) != -1) {
		switch (o) {
			case 'c':
				len = atoll(optarg);
				break;
			case 'i':
				fin = strndup(optarg, strlen(optarg));
				break;
			case 'o':
				fout = strndup(optarg, strlen(optarg));
				break;
			case 's':
				seek = atoll(optarg);
				break;
			case 'S':
				skip = atoll(optarg);
				break;
			default:
				break;
		}
	}

	if (len < 0 || strlen(fin) == 0 || strlen(fout) == 0 || seek < 0 || 
			skip < 0) {
		printf(usage, basename(argv[0]));
		return -1;
	}

#if DEBUG
	printf(runtime_options, len, fin, fout, seek, skip);
#endif

#if HAVE_BLESS

/*
 * Use case: File manipulation from command line
 *
 * Description: create a new buffer (A) from file, create new buffer (B),
 * select a region from (A), copy it to (B), save (B), close (A) and (B)
 *
 * Author: Michael Iatrou
 */

	fdin = open(fin, O_RDONLY);
	if (fdin == -1) {
		perror("open");
		return 1;
	}

	/*
	 * XXX: Does it make any sense to have modes for bless_buffer_open()?
	 */
	bufin = bless_buffer_open(fdin, O_RDONLY);
	if (!bufin) {
		perror("bless_buffer_open");
		return 1;
	}

	/* Lets assume that fout is zero sized (!) */
	bufout = bless_buffer_new();
	if (!bufout) {
		fprintf(stderr, "bless_buffer_new");
		return 1;
	}

	if (!bless_buffer_copy(bufin, skip, bufout, seek, len))
		fprintf(stderr, "bless_buffer_copy");
		return 1;
	}

	fdout = open(fout, O_RDWR);
	if (fdout == -1) {
		perror("open");
		return 1;
	}

	/* 
	 * NULL callback function implies we don't care about the progress of the
	 * operation 
	 */
	if (!bless_buffer_save(bufout, fdout, NULL)) {
		fprintf(stderr, "bless_buffer_save");
		return 1;

	/* 
	 * XXX: bless_buffer_close() should change to something like
	 * delete/destroy/purge because close insinuates a possible file descriptor
	 * close, which is not the case. (for symmetry, we shouldn't close fds)
	 */
	if (!bless_buffer_close(bufin)) {
		fprintf(stderr, "bless_buffer_close");
		return 1;
	}

	if (!bless_buffer_close(bufout)) {
		fprintf(stderr, "bless_buffer_close");
		return 1;
	}

	if (close(fdin) == -1) {
		perror("close");
		return 1;
	}

	if (close(fdout) == -1) {
		perror("close");
		return 1;
	}

#endif
	return 0;
}

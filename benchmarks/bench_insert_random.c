#include <buffer.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>

#define SIZE 20000

/* 
 * Using rand() correctly. Taken from:
 * http://www.eternallyconfuzzled.com/arts/jsw_art_rand.aspx
 */
unsigned int time_seed()
{
	time_t now = time(NULL);
	unsigned char *p = (unsigned char *)&now;
	unsigned seed = 0;
	size_t i;

	for (i = 0; i < sizeof now; i++)
		seed = seed * (0xffU + 2U) + p[i];

	return seed;
}

double uniform_deviate(int seed)
{
	return seed * (1.0 / (RAND_MAX + 1.0));
}

void fail(int err)
{
	fputs(bless_strerror(err), stderr);
	exit(1);
}

int main(void)
{
	srand(time_seed());

	bless_buffer_t *buf;
	int err;

	err = bless_buffer_new(&buf);
	if (err)
		fail(err);

	/* Create and initialize memory data */
	char **data = malloc(SIZE * sizeof *data);

	int i;
	for(i = 0; i < SIZE; i++) {
		data[i] = malloc(3);
		data[i][0] = i & 0xff;
		data[i][1] = (i + 1) & 0xff;
		data[i][2] = (i + 2) & 0xff;
	}

	clock_t start = clock();

	/* Insert data randomly in the buffer */
	for(i = 0; i < SIZE; i++) {
		bless_buffer_source_t *src;
		err = bless_buffer_source_memory(&src, data[i], 3, free);
		if (err)
			fail(err);

		off_t buf_size;
		bless_buffer_get_size(buf, &buf_size);

		off_t ins_offset = uniform_deviate(rand()) * buf_size;

		if (i < 5)
			err = bless_buffer_append(buf, src, 0, 3);
		else 
			err = bless_buffer_insert(buf, ins_offset, src, 0, 3);

		if (err)
			fail(err);

		err = bless_buffer_source_unref(src);
		if (err)
			fail(err);
	}

	clock_t end = clock();

	bless_buffer_free(buf);

	free(data);

	printf("Elapsed time: %f\n", (double) (end - start) / CLOCKS_PER_SEC);
}


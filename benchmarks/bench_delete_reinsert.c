#include <buffer.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <assert.h>

#define BUFSIZE 1024 *1024
#define DELSIZE 20

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

void print_buf(bless_buffer_t *buf)
{
	off_t buf_size;
	bless_buffer_get_size(buf, &buf_size);
	
	char *read_data = malloc(buf_size + 1);
	read_data[buf_size] = '\0';

	int err = bless_buffer_read(buf, 0, read_data, 0, buf_size);
	if (err)
		fail(err);

	printf("%s\n", read_data);
	free(read_data);
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
	char *data = malloc((BUFSIZE + 1 )* sizeof *data);
	data[BUFSIZE] = '\0';

	int j = 0;
	while (j < BUFSIZE) {
		data[j] = (j + 32) % 128;
		j++;
	}

	bless_buffer_source_t *src;
	err = bless_buffer_source_memory(&src, data, BUFSIZE, free);
	err = bless_buffer_append(buf, src, 0, BUFSIZE);

	clock_t start_del = clock();

	int i;
	/* Delete data from the buffer */
	for(i = BUFSIZE - DELSIZE; i >= 0; i -= 2 * DELSIZE) {

		err = bless_buffer_delete(buf, i, DELSIZE);

		if (err)
			fail(err);
	}

	clock_t end_del = clock();

	clock_t start_ins = clock();

	/* Insert deleted data back into buffer */
	for(i = (BUFSIZE - DELSIZE) % (2 *DELSIZE); i < BUFSIZE; i += 2 * DELSIZE) {

		off_t buf_size;
		bless_buffer_get_size(buf, &buf_size);

		if (i >= buf_size)
			err = bless_buffer_append(buf, src, i, DELSIZE);
		else 
			err = bless_buffer_insert(buf, i, src, i, DELSIZE);

		if (err)
			fail(err);

	}

	clock_t end_ins = clock();

	err = bless_buffer_source_unref(src);
	if (err)
		fail(err);

	off_t buf_size;
	bless_buffer_get_size(buf, &buf_size);
	
	assert(buf_size == BUFSIZE);
	char *read_data = malloc(BUFSIZE + 1);
	read_data[BUFSIZE] = '\0';

	err = bless_buffer_read(buf, 0, read_data, 0, buf_size);
	if (err)
		fail(err);

	assert(!memcmp(data, read_data, buf_size));
	free(read_data);

	bless_buffer_free(buf);


	printf("Delete Elapsed time: %f\n", (double) (end_del - start_del) / CLOCKS_PER_SEC);
	printf("Insert Elapsed time: %f\n", (double) (end_ins - start_ins) / CLOCKS_PER_SEC);
}



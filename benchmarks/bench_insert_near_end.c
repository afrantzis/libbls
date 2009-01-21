#include <buffer.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>

#define SIZE 20000

void fail(int err)
{
	fputs(bless_strerror(err), stderr);
	exit(1);
}

int main(void)
{
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

	/* Insert data near the end of the buffer */
	for(i = 0; i < SIZE; i++) {
		bless_buffer_source_t *src;
		err = bless_buffer_source_memory(&src, data[i], 3, free);
		if (err)
			fail(err);

		if (i < 5)
			err = bless_buffer_append(buf, src, 0, 3);
		else
			err = bless_buffer_insert(buf, i - 4, src, 0, 3);

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

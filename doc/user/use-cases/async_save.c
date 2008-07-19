/*
 * Use case: Asynchronous save and progress
 * 
 * Description: John edits a file using a GUI that uses libbless and wants to
 * save it. While saving he wants to copy some data from the file and paste
 * them to another file. At the same time he wants to be able to see how the
 * save operation is progressing and be able to cancel it.
 *
 * Design: The bless_buffer_save() function is synchronous but has support for
 * callbacks. A GUI (or any other program) that wants to perform asynchronous
 * save must use a wrapper that encapsulates a suitable asynchronous mechanism
 * (eg threads).
 *
 * Author: Alexandros Frantzis
 */

#include <pthreads.h>
#include "bless.h"

bless_buffer_t *my_buf;

struct buffer_save_args
{
	bless_buffer_t *buf;
	bless_progress_cb cb;
};

void on_file_save_clicked()
{
	struct buffer_save_args *args = malloc(sizeof(struct buffer_save_args));
	args->buf = my_buf;
	args->cb = on_save_progress;

	pthread_create(NULL, NULL, buffer_save_func, args);
}

int on_save_progress(void *data)
{
	/* WARNING: This function is run in the context of the save thread */

	/* if save is still in progress show/update progress */
	/* if save has finished do stuff eg reload file */
	/* check if user pressed the cancel button */

	/* return 1 if user cancelled the save */
}

void buffer_save_func(void *data)
{
	struct buffer_save_args *args = (struct buffer_save_args *)data;

	/* 
	 * Acquire File operations lock (see user guide for this lock scheme).
	 * Note: The lock functions must be implemented by the user.
	 */
	buffer_get_file_lock(args->buf);

	bless_buffer_save(args->buf, NULL, args->cb);

	buffer_release_file_lock(args->buf);

	free(data);
}

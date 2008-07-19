/**
 * @file buffer.h
 *
 * Public libbless buffer API.
 *
 * @author Alexandros Frantzis
 * @author Michael Iatrou
 */

typedef struct bless_buffer bless_buffer_t;

typedef int (*bless_progress_cb)(void *);

/*
 * File operations
 */

bless_buffer_t *bless_buffer_new();

bless_buffer_t *bless_buffer_open(int fd, int mode);

int bless_buffer_save(bless_buffer_t *buf, int fd, bless_progress_cb cb);

int bless_buffer_close(bless_buffer_t *buf);

/*
 * Buffer operations
 */

int bless_buffer_insert(bless_buffer_t *buf, off_t offset,
		void *data, size_t len);

int bless_buffer_delete(bless_buffer_t *buf, off_t offset, size_t len);

int bless_buffer_read(bless_buffer_t *src, off_t skip, void *dst,
		off_t seek, size_t len);

int bless_buffer_copy(bless_buffer_t *src, off_t skip, bless_buffer_t *dst,
		off_t seek, size_t len);

off_t bless_buffer_find(bless_buffer_t *buf, off_t start_offset, 
		void *data, size_t len, bless_progress_cb cb);

/*
 * Undo - Redo related
 */

int bless_buffer_undo(bless_buffer_t *buf);

int bless_buffer_redo(bless_buffer_t *buf);

void bless_buffer_begin_multi_op(bless_buffer_t *buf);

void bless_buffer_end_multi_op(bless_buffer_t *buf);


/*
 * Buffer info
 */

int bless_buffer_can_undo(bless_buffer_t *buf);

int bless_buffer_can_redo(bless_buffer_t *buf);

char *bless_buffer_get_path(bless_buffer_t *buf);

size_t bless_buffer_get_size(bless_buffer_t *buf);

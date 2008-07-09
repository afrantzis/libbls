typedef struct bless_buffer bless_buffer_t;

/*
 * File operations
 */

bless_buffer_t *bless_buffer_new();

bless_buffer_t *bless_buffer_open(const char *path, int mode);

int bless_buffer_save(const char *path);

int bless_buffer_close(bless_buffer_t *buf);

/*
 * Buffer operations
 */

int bless_buffer_insert(bless_buffer_t *buf, off_t offset,
                        void *data, size_t len);

int bless_buffer_delete(bless_buffer_t *buf, off_t offset, size_t len);

off_t bless_buffer_find(bless_buffer_t *buf, off_t start_offset, 
                              void *data, size_t len);

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

char *bless_buffer_get_file(bless_buffer_t *buf);

size_t bless_buffer_get_size(bless_buffer_t *buf);

struct segcol {
	int (*insert)(void *impl, off_t offset, segment_t *seg); 
	segcol_t *(*delete)(void *impl, off_t offset, size_t length);
	segcol_iter *(*find)(void *impl, off_t offset);
	int (*iter_next)(segcol_iter *iter);
	segment_t *(*iter_get_segment)(segcol_iter *iter);
	off_t *(*iter_get_mapping)(segcol_iter *iter);
	int *(*iter_is_valid)(segcol_iter *iter);
	int (*segcol->free)(void *impl);

	void *impl;
};


struct segcol_iter {
	void *impl;
};

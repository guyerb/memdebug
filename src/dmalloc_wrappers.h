#ifndef DMALLOC_WRAPPERS_H
#define DMALLOC_WRAPPERS_H

#ifdef LINUX
extern void * dmalloc_calloc_wrapper(size_t count, size_t size);
extern void dmalloc_free_wrapper(void *ptr);
extern void * dmalloc_malloc_wrapper(size_t size);
extern void * dmalloc_realloc_wrapper(void *ptr, size_t size);
#endif

#ifdef DARWIN
#define dmalloc_calloc_wrapper		calloc
#define dmalloc_free_wrapper		free
#define dmalloc_malloc_wrapper		malloc
#define dmalloc_realloc_wrapper		realloc
#endif

#endif	/* DMALLOC_WRAPPERS_H */

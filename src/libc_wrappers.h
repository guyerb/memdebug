#ifndef DMALLOC_WRAPPERS_H
#define DMALLOC_WRAPPERS_H

#ifdef LINUX
extern void * libc_calloc_wrapper(size_t count, size_t size);
extern void libc_free_wrapper(void *ptr);
extern void * libc_malloc_wrapper(size_t size);
extern void * libc_realloc_wrapper(void *ptr, size_t size);
#endif

#ifdef DARWIN
#define libc_calloc_wrapper		calloc
#define libc_free_wrapper		free
#define libc_malloc_wrapper		malloc
#define libc_realloc_wrapper		realloc
#endif

#endif	/* DMALLOC_WRAPPERS_H */

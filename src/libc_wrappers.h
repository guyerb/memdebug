#ifndef DMALLOC_WRAPPERS_H
#define DMALLOC_WRAPPERS_H

#ifdef LINUX
extern void * libc_calloc_wrapper(size_t, size_t);
extern void libc_free_wrapper(void *);
extern void * libc_malloc_wrapper(size_t);
extern void * libc_realloc_wrapper(void *, size_t);
extern int libc_wrappers_initialized();
#endif

#ifdef DARWIN
#define libc_calloc_wrapper		calloc
#define libc_free_wrapper		free
#define libc_malloc_wrapper		malloc
#define libc_realloc_wrapper		realloc

static inline int libc_wrappers_initialized() { return 1; }
#endif

#endif	/* DMALLOC_WRAPPERS_H */

/* TODO: this doesn't do anything as yet. */
#ifndef DMALLOC_API_H
#define DMALLOC_API_H

extern void dmalloc_stop();
extern void dmalloc_start();
extern void dmalloc_reset();
extern void dmalloc_debug_set();
extern void dmalloc_debug_get();

#if 1
/* for of shared lib routines debug you may want to call the routines
   directly in the shared library. enable these defintions, change
   your application makefile to link with the shared library directly
   and change the library source to not hook the malloc routines, then
   invoke your program with the proper LD_LIBRARY_PATH (
   DYLD_FALLBACK_LIBRARY_PATH on OS X)
*/

extern void dmalloc_free(void *ptr);
extern void * dmalloc_malloc(size_t size);
#endif


#endif /* DMALLOC_API_H */

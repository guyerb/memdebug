#ifndef DMALLOC_VERSION_H
// Update these when the version changes

#define dmalloc_major 0
#define dmalloc_minor 5

#define XSTR(x) STR(x)
#define STR(x) #x

#define DMALLOC_VERSION (dmalloc_major * 1000 + dmalloc_minor)
#define DMALLOC_VERSION_STRING "dmalloc v" XSTR(dmalloc_major) "." XSTR(dmalloc_minor)

#endif	/* DMALLOC_VERSION_H */


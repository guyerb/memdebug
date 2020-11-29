#define _POSIX_C_SOURCE  200809L /* get flockfile */

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#include "dmalloc_common.h"

int is_logging = 0;

void dmalloc_log_protect()
{
  flockfile(stderr);	/* prevent output corruption from concurrency */
  is_logging = 1;	/* prevent output corruption from reentrancy */
}

void dmalloc_log_unprotect()
{
  is_logging = 0;
  funlockfile(stderr);
}

void dmalloc_logf( const char* format, ... )
{
  va_list args;

  va_start(args, format);
#ifdef LINUX
  vfprintf(stderr, format, args);
#else
  malloc_printf(format, args);
#endif
  va_end( args );
}
void dmalloc_printf( const char* format, ... )
{
  if (!is_logging) {
    va_list args;

    va_start(args, format);
#ifdef LINUX
    vfprintf(stderr, format, args);
#else
    malloc_printf(format, args);
#endif
    va_end( args );
  }
}

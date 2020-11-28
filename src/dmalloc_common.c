#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include "dmalloc_common.h"

extern int dmalloc_is_logging;

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
  if (!dmalloc_is_logging) {
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

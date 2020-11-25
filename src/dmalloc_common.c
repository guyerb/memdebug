#include <stdio.h>
#include <stdarg.h>
#include "dmalloc_common.h"

void dmalloc_printf( const char* format, ... )
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

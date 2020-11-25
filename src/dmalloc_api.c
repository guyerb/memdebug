#include <stdio.h>
#include "dmalloc_common.h"
#include "dmalloc_api.h"

/* control routines for managing dmalloc operations. The library can
   be used without these but they are handy for debugging certain
   sections of your code. Include dmalloc_api.h in your application to
   access these functions
*/

void dmalloc_stop()
{
  dmalloc_printf("dmalloc_stop\n");
}

void dmalloc_start()
{
  dmalloc_printf("dmalloc_start\n");
}

void dmalloc_reset()
{
  dmalloc_printf("dmalloc_reset\n");
}

void dmalloc_log()
{
  dmalloc_printf("dmalloc_log\n");
}

void dmalloc_debug_set()
{
  dmalloc_printf("dmalloc_debug_set\n");
}

void dmalloc_debug_get()
{
  dmalloc_printf("dmalloc_debug_get\n");
}

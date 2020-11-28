/* output logs (stderr) of current dmalloc state */
#define _POSIX_C_SOURCE  200809L /* get strdup */

#include <stdio.h>
#include <stdint.h>
#include <locale.h>
#include <time.h>

#include "dmalloc_version.h"
#include "dmalloc_common.h"
#include "dmalloc_stats.h"

int dmalloc_is_logging = 1;

/* search the array p for the biggest entry and decide how many
   entries per 'mark' should be displayed in provided columns e.g. if
   the largest field in the arrary is 160 and columns are set to 80
   then return 2, i.e. 2 elements should be displayed by one mark N.B
   the next few support routines are recursive (from the interwebz
   where noted)
*/

/* find size bucket with most bytes, assuming max bytes per bucket */
static uint32_t logline_largest(uint32_t *p, size_t len)
{
  uint32_t largest = 0;
  uint8_t exp = 2;		/* start on the 0-4 bucket  */

  for (uint32_t i=0; i<len; i++) {
    uint32_t bytes = p[i] * ((exp << (i + 1)) -1); /* (0+1) - 1 = 0 (2<2) -1 = 7 ...*/
    if (bytes > largest) largest = bytes;
  }
  return largest;
}

/* a little recursive scale determination */
static uint32_t logline_scaler(uint32_t largest, uint32_t columns)
{
  uint32_t scaler = 0;

  if (largest <= columns)
    return 1;

  scaler += logline_scaler(largest - columns, columns);
  return scaler;
}

/* a little recursive char '#' scaled dumper */
static void logline_print_scaled(char *hdr, uint32_t count, uint32_t scale)
{
  if (count < scale) {
    dmalloc_printf("%-15s: ", hdr);
    if (count != 0) {
      if (count >= scale/2) 	/* round */
	dmalloc_logf("%s:#", hdr);
      else
	dmalloc_logf("%s:", hdr);
      return;
    } else {
      logline_print_scaled(hdr, count - scale, scale);
    }
  }
  dmalloc_logf("#");
}

/* a little recursive formatter from the interwebz cuz "%'d" didn't work*/
void logline_print_commas (uint32_t n)
{
    if (n < 1000) {
      dmalloc_logf ("%d", n);
        return;
    }
    logline_print_commas(n/1000);
    dmalloc_logf (",%03d", n%1000);
}

void dmalloc_stats_log()
{
  struct dmalloc_alloc_stats stats;
  time_t now = time(NULL);
  struct tm *pgm = NULL;
  char * pgm_str = NULL;

  dmalloc_stats_getter(&stats);	/* get local copy */

  pgm = gmtime(&now);
  if (pgm == NULL) {
    perror("dmalloc gmtime:");
    return; /* really should OOM crash here*/
  }

  pgm_str= asctime(pgm);
  if (pgm_str == NULL) {
    perror("dmalloc asctime:");
    return; /* really should OOM crash here*/
  }

  pgm_str[24] = '\0'; 		/* kill the newline with bravado */

  dmalloc_is_logging = 1;
  
  dmalloc_logf("========== %s: UTC %s ==========\n", DMALLOC_VERSION_STRING, pgm_str);
  dmalloc_logf("Dmalloc stats:\n");
  dmalloc_logf("%-30s", "overall allocations:" );
  logline_print_commas(stats.s_allocated_alltime);
  dmalloc_logf("\n");
  dmalloc_logf("%-30s", "current allocations:" );
  logline_print_commas(stats.s_allocated_current);
  dmalloc_logf("\n");

#ifdef DMALLOC_STATS_UNIT
  /* for debugging */
  dmalloc_logf("\nInternal Stats enabled:\n");
  dmalloc_logf("%-25s %5d\n", "age underruns:", stats._s_agebucket_underrun_error);
  dmalloc_logf("%-25s %5d\n", "size underruns:", stats._s_sizebucket_underrun_error);
  dmalloc_logf("%-25s %5d\n", "lock errors:", stats._s_lockerror);
  dmalloc_logf("%-25s %5d\n", "declined updates:", stats._s_declined_updates);
  dmalloc_logf("\n");
#endif	/* DMALLOC_STATS_UNIT */

  /* dump age buckets */
  uint32_t largest = logline_largest( (uint32_t *)&stats.s_sizebuckets, BUCKETS_SIZE_NUM);
  uint32_t scaler = logline_scaler(largest, 65);

  dmalloc_logf("Current size allocations by bytes: ( one # represents %d bytes)\n", scaler);

  logline_print_scaled("0 - 4",		stats.s_sizebuckets[BUCKET_0000], scaler);
  logline_print_scaled("4 - 8",		stats.s_sizebuckets[BUCKET_0004], scaler);
  logline_print_scaled("8 - 16",	stats.s_sizebuckets[BUCKET_0008], scaler);
  logline_print_scaled("16 - 32",	stats.s_sizebuckets[BUCKET_0016], scaler);
  logline_print_scaled("32 - 64",	stats.s_sizebuckets[BUCKET_0032], scaler);
  logline_print_scaled("64 - 128",	stats.s_sizebuckets[BUCKET_0064], scaler);
  logline_print_scaled("128 - 256",	stats.s_sizebuckets[BUCKET_0128], scaler);
  logline_print_scaled("256 - 512",	stats.s_sizebuckets[BUCKET_0256], scaler);
  logline_print_scaled("512 - 1024",	stats.s_sizebuckets[BUCKET_0512], scaler);
  logline_print_scaled("1024 - 2048",	stats.s_sizebuckets[BUCKET_1024], scaler);
  logline_print_scaled("2048 - 4096",	stats.s_sizebuckets[BUCKET_2048], scaler);
  logline_print_scaled("4096 +",	stats.s_sizebuckets[BUCKET_4096], scaler);

  dmalloc_is_logging = 0;  

/*   >>>>>>>>>>>>> Sat Jan 6 00:36:26 UTC 2018 <<<<<<<<<<< */

/* 			Overall stats: */
/* 6,123,231 Overall allocations since start */
/* 84.3MiB Current total allocated size */
/* Current allocations by size: ( # = 8,123 current allocations) */
/* 0 - 4 bytes: ########## */
/* 4 - 8 bytes: */
/* 8 - 16 bytes: #### */
/* 16 - 32 bytes: */
/* 32 - 64 bytes: */
/* 64 - 128 bytes: */
/* 128 - 256 bytes: */
/* 256 - 512 bytes: ## 512 - 1024 bytes: # 1024 - 2048 bytes: # 2048 - 4096 bytes: # 4096 + bytes: ####### */
/* Current allocations by age: ( # = 8,123 current allocations) < 1 sec: ### */
/* < 10 sec: ## */
/* < 100 sec: ## */
/* < 1000 sec: ######################### > 1000 sec: */
}

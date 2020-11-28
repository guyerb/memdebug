/* output logs (stderr) of current dmalloc state */
#define _POSIX_C_SOURCE  200809L /* get strdup */

#include <stdio.h>
#include <stdint.h>
#include <locale.h>
#include <time.h>

#include "dmalloc_version.h"
#include "dmalloc_common.h"
#include "dmalloc_stats.h"

static struct dmalloc_alloc_stats stats = {0};

/* search the array p for the biggest entry and decide how many
   entries per 'mark' should be displayed in provided columns e.g. if
   the largest field in the arrary is 160 and columns are set to 80
   then return 2, i.e. 2 elements should be displayed by one mark N.B
   the next few support routines are recursive (from the interwebz
   where noted)
*/

/* find range (powers of 10) of age buckets with largest value */
static int logline_power10_largest(uint32_t *p)
{
  uint32_t largest = 0;
  uint32_t count = 0;

  count = 0;
  for (int i = 0; i < 10; i++) { count+= p[i]; }
  if (count > largest) largest = count;

  count = 0;
  for (int i = 10; i < 99; i++) { count+= p[i]; }
  if (count > largest) largest = count;

  count = 0;
  for (int i = 100; i < 999; i++) { count+= p[i]; }
  if (count > largest) largest = count;

  count = p[999];
  if (count > largest) largest = count;

  return largest;
}

/* find size bucket with most bytes, assuming max bytes per bucket */
static uint32_t logline_power2_largest(uint32_t *p, size_t len)
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
static void logline_scaled(char *hdr, uint32_t count, uint32_t scale)
{
  if (count < scale) {
    dmalloc_printf("%-11s: ", hdr);
    if (count == 0) {
      dmalloc_logf("%-11s:\n", hdr);
      return;
    } else {
      if (count >= scale/2) 	/* round */
	dmalloc_logf("%-11s:#\n", hdr);
      else
	logline_scaled(hdr, count - scale, scale);
    }
  } else {
    dmalloc_logf("#");
  }
}

static void logline_range_scaled(char *hdr, uint32_t *p, uint32_t floor, uint32_t ceiling, uint32_t scale)
{
  uint32_t count = 0;

  for ( ; floor < ceiling; floor++) {
    count += p[floor];
  }
  logline_scaled(hdr, count, scale);
}

/* a little recursive formatter from the interwebz cuz "%'d" didn't work*/
void logline_commas (uint32_t n)
{
    if (n < 1000) {
      dmalloc_logf ("%d", n);
        return;
    }
    logline_commas(n/1000);
    dmalloc_logf (",%03d", n%1000);
}

void dmalloc_log_stats()
{
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

  dmalloc_log_protect();
  
  dmalloc_logf("========== %s: UTC %s ==========\n", DMALLOC_VERSION_STRING, pgm_str);
  dmalloc_logf("%-26s", "overall allocations:" );
  logline_commas(stats.s_total_countalloc);
  dmalloc_logf("\n");
  dmalloc_logf("%-26s", "current allocations:" );
  logline_commas(stats.s_curr_countalloc);
  dmalloc_logf("%-26s", "current alloc bytes:" );
  logline_commas(stats.s_curr_sizealloc);
  dmalloc_logf("\n");

#ifdef DMALLOC_UNIT_TEST
  dmalloc_logf("\ndebug stats enabled:\n");
  dmalloc_logf("%-25s %d\n", "age underruns:", stats._s_underrun_agebucket);
  dmalloc_logf("%-25s %d\n", "size underruns:", stats._s_underrun_sizebucket);
  dmalloc_logf("%-25s %d\n", "lock errors:", stats._s_errorlock);
  dmalloc_logf("%-25s %d\n", "declined updates:", stats._s_declined_update);
  dmalloc_logf("\n");
#endif	/* DMALLOC_UNIT_TEST */

  /* dump age buckets */
  uint32_t sz_largest = logline_power2_largest( (uint32_t *)&stats.s_sizebucket, BUCKETS_SIZE_NUM);
  uint32_t sz_scaler = logline_scaler(sz_largest, 68);

  dmalloc_logf("Current size allocations by bytes: ( one # represents %d bytes)\n", sz_scaler);

  logline_scaled("0    -    4",	stats.s_sizebucket[BUCKET_0000], sz_scaler);
  logline_scaled("4    -    8",	stats.s_sizebucket[BUCKET_0004], sz_scaler);
  logline_scaled("8    -   16",	stats.s_sizebucket[BUCKET_0008], sz_scaler);
  logline_scaled("16   -   32",	stats.s_sizebucket[BUCKET_0016], sz_scaler);
  logline_scaled("32   -   64",	stats.s_sizebucket[BUCKET_0032], sz_scaler);
  logline_scaled("64   -  128",	stats.s_sizebucket[BUCKET_0064], sz_scaler);
  logline_scaled("128  -  256",	stats.s_sizebucket[BUCKET_0128], sz_scaler);
  logline_scaled("256  -  512",	stats.s_sizebucket[BUCKET_0256], sz_scaler);
  logline_scaled("512  - 1024",	stats.s_sizebucket[BUCKET_0512], sz_scaler);
  logline_scaled("1024 - 2048",	stats.s_sizebucket[BUCKET_1024], sz_scaler);
  logline_scaled("2048 - 4096",	stats.s_sizebucket[BUCKET_2048], sz_scaler);
  logline_scaled("4096 - infi",	stats.s_sizebucket[BUCKET_4096], sz_scaler);
  dmalloc_logf("\n");

  /* dump size buckets */
  uint32_t age_largest = logline_power10_largest( (uint32_t *)&stats.s_agebucket);
  uint32_t age_scaler = logline_scaler(age_largest, 68);

  dmalloc_logf("Current age allocations by bytes: ( one # represents %d bytes)\n", age_scaler);
  logline_range_scaled("<    10 sec", stats.s_sizebucket, 0, 9, age_scaler);
  logline_range_scaled("<   100 sec", stats.s_sizebucket, 10, 99, age_scaler);
  logline_range_scaled("<  1000 sec", stats.s_sizebucket, 100, 999, age_scaler);
  logline_range_scaled(">= 1000 sec", stats.s_sizebucket, 999, 1000, age_scaler);

  dmalloc_log_unprotect();

}


/* ------------------------------------------------------------------------- */
#ifdef DMALLOC_UNIT_TEST
/* ------------------------------------------------------------------------- */
#include <string.h>

static int unit_total;
static int unit_passed;
static int unit_failed;

void ut_delim()
{
  printf("unit total tests %d passed %d failed %d\n", unit_total, unit_passed, \
	 unit_failed);

  unit_total = 0;
  unit_failed = 0;
  unit_passed = 0;
}

#define ut_mark ut_start
void ut_start( char *descr)
{
  puts("---------------------------------------------------------------------");
  printf("DMALLOC UNIT TESTS - %s:\n\n", descr);
}

void ut_check(char *descr, uint32_t expected, uint32_t actual)
{
  int passed = (expected == actual);

  unit_total++;
  if (passed) unit_passed++;
  else unit_failed++;

  printf("%-30s %-8s expected %10u actual %10u\n", descr, \
	 passed ? "PASSED_" : "_FAILED", expected, actual);
}

void ut_clear(void)
{
  memset(&stats, 0, (sizeof(struct dmalloc_alloc_stats)));
}

#define UNUSED(x) (void)(x)
void dmalloc_stats_getter(struct dmalloc_alloc_stats *p)
{
  UNUSED(p);
}

int main()
{
  ut_start("exercise logging");
  dmalloc_log_stats();
}
#endif	/* DMALLOC_UNIT_TEST */

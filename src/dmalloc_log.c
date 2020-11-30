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

/* find range of age buckets with largest value */
static int logline_agebucket_largest()
{
  uint32_t largest = 0;
  uint32_t count = 0;

  count = 0;
  for (int i = 0; i < 10; i++) { count+= stats.s_agebucket[i]; }
  if (count > largest) largest = count;

  count = 0;
  for (int i = 10; i < 99; i++) { count+= stats.s_agebucket[i]; }
  if (count > largest) largest = count;

  count = 0;
  for (int i = 100; i < 999; i++) { count+= stats.s_agebucket[i]; }
  if (count > largest) largest = count;

  count = stats.s_agebucket[999];
  if (count > largest) largest = count;

  return largest;
}

/* rough estimate of bytes in bucket */
static uint32_t bucket_bytes_approx(uint32_t ndx)
{
  uint32_t multiplier = 0;

  switch (ndx) {
  case BUCKET_0000:
    multiplier = 2;
    break;
  case BUCKET_0004:
    multiplier = 6;
    break;
  case BUCKET_0008:
    multiplier = 12;
    break;
  case BUCKET_0016:
    multiplier = 24;
    break;
  case BUCKET_0032:
    multiplier = 48;
    break;
  case BUCKET_0064:
    multiplier = 96;
    break;
  case BUCKET_0128:
    multiplier = 192;
    break;
  case BUCKET_0256:
    multiplier = 384;
    break;
  case BUCKET_0512:
    multiplier = 768;
    break;
  case BUCKET_1024:
    multiplier = 1536;
    break;
  case BUCKET_2048:
    multiplier = 3072;
    break;
  case BUCKET_4096:
    /* this one is perfectly indefensible since we have no idea how
       big these allocs are */
    multiplier = 6144;
    break;
  default:
    dmalloc_logf("unknown agebucket index");
    return 0;
  }
  return multiplier * stats.s_sizebucket[ndx];
}

/* find size bucket with most bytes. for instance if we find 1 entry
   in the 8 - 15 size then assume it is 12 bytes. Errors can build up
   rapidly but the output histogram will still be represenative
*/
static uint32_t logline_sizebucket_largest()
{
  uint32_t largest = 0;
  uint32_t size = 0;

  /* this can (and has) be done in an undecipherable for loop so we
     just spell it out for clarity */
  size = bucket_bytes_approx(BUCKET_0000);
  if (size > largest) largest = size;

  size = bucket_bytes_approx(BUCKET_0004);
  if (size > largest) largest = size;

  size = bucket_bytes_approx(BUCKET_0008);
  if (size > largest) largest = size;

  size = bucket_bytes_approx(BUCKET_0016);
  if (size > largest) largest = size;

  size = bucket_bytes_approx(BUCKET_0032);
  if (size > largest) largest = size;

  size = bucket_bytes_approx(BUCKET_0064);
  if (size > largest) largest = size;

  size = bucket_bytes_approx(BUCKET_0128);
  if (size > largest) largest = size;

  size = bucket_bytes_approx(BUCKET_0256);
  if (size > largest) largest = size;

  size = bucket_bytes_approx(BUCKET_0512);
  if (size > largest) largest = size;

  size = bucket_bytes_approx(BUCKET_1024);
  if (size > largest) largest = size;

  size = bucket_bytes_approx(BUCKET_2048);
  if (size > largest) largest = size;

  size = bucket_bytes_approx(BUCKET_4096);
  if (size > largest) largest = size;

  return largest;
}

/* a little recursive scale determination */
static uint32_t logline_scaler(uint32_t largest, uint32_t columns)
{
  uint32_t scaler = 1;

  if (largest <= columns)
    return 1;
  else
    scaler += logline_scaler(largest - columns, columns);
  return scaler;
}

/* a little recursive char '#' scaled dumper */
static void logline_scaled(char *hdr, uint32_t count, uint32_t scale)
{
  static int depth = 0;

  if (depth == 0)
    dmalloc_logf("%-11s: ", hdr);

  if (count < scale) {
    if (scale > 1)
      dmalloc_logf(".");	/* partial # */
  } else {
    dmalloc_logf("#");
    depth++;
    logline_scaled(hdr, count - scale, scale);
    depth--;
  }

  if (depth == 0) dmalloc_logf("\n");
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

  pgm_str[24] = '\0';		/* kill the newline with bravado */

  dmalloc_log_protect();

  dmalloc_logf("========== %s: UTC %s ==========\n", DMALLOC_VERSION_STRING, pgm_str);
  dmalloc_logf("%-26s", "overall allocations:" );
  logline_commas(stats.s_total_countalloc);
  dmalloc_logf("\n");
  dmalloc_logf("%-26s", "current allocations:" );
  logline_commas(stats.s_curr_countalloc);
  dmalloc_logf("\n");
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

  /* dump size buckets */
  uint32_t sz_largest = logline_sizebucket_largest();
  uint32_t sz_scaler = logline_scaler(sz_largest, 68);

  /* this graph is a little weird, it doesn't show the number of
     allocations for that bucket but rather roughly the number of
     bytes living in allocations of that size. Obviously useful but
     definiately should add another chart which shows the distribution
     of allocation sizes */
  dmalloc_logf("histogram: allocation size: (one # represents approx %d bytes)\n", sz_scaler);
  logline_scaled("0    -    4",	bucket_bytes_approx(BUCKET_0000), sz_scaler);
  logline_scaled("4    -    8",	bucket_bytes_approx(BUCKET_0004), sz_scaler);
  logline_scaled("8    -   16",	bucket_bytes_approx(BUCKET_0008), sz_scaler);
  logline_scaled("16   -   32",	bucket_bytes_approx(BUCKET_0016), sz_scaler);
  logline_scaled("32   -   64",	bucket_bytes_approx(BUCKET_0032), sz_scaler);
  logline_scaled("64   -  128",	bucket_bytes_approx(BUCKET_0064), sz_scaler);
  logline_scaled("128  -  256",	bucket_bytes_approx(BUCKET_0128), sz_scaler);
  logline_scaled("256  -  512",	bucket_bytes_approx(BUCKET_0256), sz_scaler);
  logline_scaled("512  - 1024",	bucket_bytes_approx(BUCKET_0512), sz_scaler);
  logline_scaled("1024 - 2048",	bucket_bytes_approx(BUCKET_1024), sz_scaler);
  logline_scaled("2048 - 4096",	bucket_bytes_approx(BUCKET_2048), sz_scaler);
  logline_scaled("4096 - infi",	bucket_bytes_approx(BUCKET_4096), sz_scaler);
  dmalloc_logf("\n");

  /* dump size buckets */
  uint32_t age_largest = logline_agebucket_largest();
  uint32_t age_scaler = logline_scaler(age_largest, 68);

  dmalloc_logf("histogram: allocations age: (one # represents appox %d  allocs)\n", age_scaler);
  logline_range_scaled("<    10 sec", stats.s_agebucket, 0, 9, age_scaler);
  logline_range_scaled("<   100 sec", stats.s_agebucket, 10, 99, age_scaler);
  logline_range_scaled("<  1000 sec", stats.s_agebucket, 100, 999, age_scaler);
  logline_range_scaled(">= 1000 sec", stats.s_agebucket, 999, 1000, age_scaler);

  dmalloc_log_unprotect();

}


/* ------------------------------------------------------------------------- */
#ifdef DMALLOC_UNIT_TEST
/* ------------------------------------------------------------------------- */
#include <string.h>
#include <limits.h>

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
  /* dmalloc_log_stats(); */
  ut_delim();
  stats.s_sizebucket[BUCKET_1024] = 1;
  ut_check("1024 bucket is largest", 1536, logline_sizebucket_largest());
  stats.s_sizebucket[BUCKET_0000] = 768;
  ut_check("0000 bucket is largest", 1536, logline_sizebucket_largest());
  ut_check("scalar for 160 @ 80 columns", 2, logline_scaler(160, 80));
  ut_check("scalar for 320 @ 80 columns", 4, logline_scaler(320, 80));
  ut_check("scalar for 400 @ 80 columns", 5, logline_scaler(400, 80));
  ut_check("scalar for 100 @ 68 columns", 2, logline_scaler(100, 68));
  stats.s_sizebucket[BUCKET_4096] = 1;
  ut_check("4096 bucket is largest", 4096 + (4096/2), logline_sizebucket_largest());
  logline_scaled("160 @ 2", 160, 2);
  logline_scaled("80 @ 1", 80, 1);
  logline_scaled("40 @ 2", 40, 2);
  logline_scaled("512 @ 1024", 512, 1024);
  logline_scaled("511 @ 1024", 511, 1024);
  logline_scaled("0 @ 80", 0, 1);

  stats.s_curr_countalloc = UINT_MAX;
  stats.s_total_countalloc = UINT_MAX;
  stats.s_sizebucket[BUCKET_0000] = 1000;
  stats.s_sizebucket[BUCKET_0004] = 1000;
  stats.s_sizebucket[BUCKET_0008] = 1000;
  stats.s_sizebucket[BUCKET_0016] = 1000;
  stats.s_sizebucket[BUCKET_0032] = 1000;
  stats.s_sizebucket[BUCKET_0064] = 1000;
  stats.s_sizebucket[BUCKET_0128] = 1000;
  stats.s_sizebucket[BUCKET_0256] = 1000;
  stats.s_sizebucket[BUCKET_0512] = 1000;
  stats.s_sizebucket[BUCKET_1024] = 1000;
  stats.s_sizebucket[BUCKET_2048] = 1000;
  stats.s_sizebucket[BUCKET_4096] = 1000;

  stats.s_agebucket[0] = 100;
  stats.s_agebucket[90] = 1000;
  stats.s_agebucket[900] = 10000;
  stats.s_agebucket[999] = 100000;

  dmalloc_log_stats();
}
#endif	/* DMALLOC_UNIT_TEST */

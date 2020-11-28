#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/queue.h>

#include "dmalloc_stats.h"
#include "dmalloc_common.h"


struct dmalloc_alloc_stats dmalloc_stats = {0};

static pthread_mutex_t dmalloc_stats_mutex = PTHREAD_MUTEX_INITIALIZER;

void dmalloc_stats_getter(struct dmalloc_alloc_stats *pcopy)
{
  if (pthread_mutex_lock(&dmalloc_stats_mutex) == 0) {
    if (pcopy)
      *pcopy = dmalloc_stats;
    pthread_mutex_unlock(&dmalloc_stats_mutex);
  } else {
    dmalloc_stats._s_errorlock++;
  }
}

/*
 * we monitor the age of each allocation by grouping them in buckets,
 * where each bucket represents one second of age. We bump them
 * forward by elapsed time since last update. For a full discussion
 * see NOTES sections of top-level README.
 */
static void dmalloc_agebuckets_update(time_t now)
{
  time_t elapsed; 

  if (pthread_mutex_lock(&dmalloc_stats_mutex) == 0) {
    elapsed = now - dmalloc_stats.s_last_ageupdate;
    dmalloc_stats.s_last_ageupdate = now;
    pthread_mutex_unlock(&dmalloc_stats_mutex);

    if (elapsed > 0) {
      for (int i = BUCKETS_AGE_NUM; i > 0; i--) {
	int src_ndx = i - 1;
	int dst_ndx = src_ndx + elapsed;
	time_t src_val = dmalloc_stats.s_agebucket[src_ndx];

	if (src_ndx == 999) continue;	/* bucket 1000 doesn't age */
	dmalloc_stats.s_agebucket[src_ndx] = 0;
	if (dst_ndx >= 999)
	  dmalloc_stats.s_agebucket[999] += src_val;
	else
	  dmalloc_stats.s_agebucket[dst_ndx] = src_val;
      }
    } else {
      dmalloc_stats._s_declined_update++;
    }
  } else {
    dmalloc_stats._s_errorlock++;
  }
}

static void dmalloc_agebucket_insert()
{
  dmalloc_stats.s_agebucket[0]++;
}


static int dmalloc_agebucket_ndx(time_t now, time_t birth)
{
  int ndx = -1;

  if (birth > now) {
    dmalloc_stats._s_invalid_birthday++;
    ndx = -1;
  } else {
    ndx = now - birth;
    if (ndx > 999) ndx = 999;
  }
  return ndx;
}

/* This element was allocated sometime in the past as indicated by
   age. We can determine how old it is by subtracting the current time
   from that age then compute the bucket index. If we don't find any
   elements in that bucket we will proceed down the list and decrease
   the population by one at the next populated bucket */
static void dmalloc_agebucket_delete(time_t now, time_t birth)
{
  int ndx = 0;

  ndx = dmalloc_agebucket_ndx(now, birth);
  if (ndx >= 0) {
    dmalloc_agebuckets_update(now);

    pthread_mutex_lock(&dmalloc_stats_mutex);
    if (dmalloc_stats.s_agebucket[ndx] != 0) {
      dmalloc_stats.s_agebucket[ndx]--;
    } else {
      dmalloc_stats._s_underrun_agebucket++;
    }
    pthread_mutex_lock(&dmalloc_stats_mutex);
  }
}

static int size_bucket_ndx(size_t sz)
{
  if (sz >= 0x1000)
    return BUCKET_4096;
  if (sz & 0x0800)
    return BUCKET_2048;
  if (sz & 0x0400)
    return BUCKET_1024;
  if (sz & 0x0200)
    return BUCKET_0512;
  if (sz & 0x0100)
    return BUCKET_0256;
  if (sz & 0x0080)
    return BUCKET_0128;
  if (sz & 0x0040)
    return BUCKET_0064;
  if (sz & 0x0020)
    return BUCKET_0032;
  if (sz & 0x0010)
    return BUCKET_0016;
  if (sz & 0x0008)
    return BUCKET_0008;
  if (sz & 0x0004)
    return BUCKET_0004;
  return BUCKET_0000;
}

void dmalloc_stats_alloc(size_t sz, time_t now)
{
  if (pthread_mutex_lock(&dmalloc_stats_mutex) == 0) {
      int ndx = size_bucket_ndx(sz);

      dmalloc_stats.s_curr_countalloc += sz;
      dmalloc_stats.s_total_countalloc += sz;
      dmalloc_stats.s_sizebucket[ndx]++;
      pthread_mutex_unlock(&dmalloc_stats_mutex);
  } else {
    dmalloc_stats._s_errorlock++;
 }

  dmalloc_agebuckets_update(now);
  dmalloc_agebucket_insert();

  return;
}

void dmalloc_stats_free(size_t sz, time_t now, time_t birth)
{
  int ndx = size_bucket_ndx(sz);
  int error = 0;

  if (pthread_mutex_lock(&dmalloc_stats_mutex) == 0) {

    if ((dmalloc_stats.s_curr_countalloc >= sz) && \
	(dmalloc_stats.s_sizebucket[ndx])) {
      dmalloc_stats.s_curr_countalloc -= sz;
      dmalloc_stats.s_sizebucket[ndx]--;
    } else {
      dmalloc_stats._s_underrun_sizebucket++;
      error = 1;
    }
    pthread_mutex_unlock(&dmalloc_stats_mutex);

    if (!error) dmalloc_agebucket_delete(now, birth);

  } else {
    dmalloc_stats._s_errorlock++;
  }
  return;
}



/* ************************************************************************** */
#ifdef DMALLOC_UNIT_TEST

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
  memset(&dmalloc_stats, 0, (sizeof(struct dmalloc_alloc_stats)));
}

int main()
{
  time_t t_0    = time(NULL);
  time_t t_1    = t_0 + 1;
  time_t t_999  = t_0 + 999;
  time_t t_1500 = t_0 + 1500;
  time_t t_2000 = t_0 + 2000;

  /*alloc sz	birthday
    0		t_0
    1		t_0
    2		t_0
    4		t_0
    8		t_0
    16		t_0
    32		t_0
    64		t_0
    128	t_0
    256	t_0
    512	t_0
    1024	t_0
    2048	t_0
    4096	t_0
    8192	t_1
    8192	t_999
    8192	t_1500
    8192	t_2000
*/

  ut_start("exercise size_bucket_ndx()");
  ut_check("0 byte bucket is ", 0, size_bucket_ndx(0));
  ut_check("1 byte bucket is ", 0, size_bucket_ndx(1));
  ut_check("2 byte bucket is ", 0, size_bucket_ndx(2));
  ut_check("3 byte bucket is ", 0, size_bucket_ndx(3));
  ut_check("4 byte bucket is ", 1, size_bucket_ndx(4));
  ut_check("5 byte bucket is ", 1, size_bucket_ndx(5));
  ut_check("6 byte bucket is ", 1, size_bucket_ndx(6));
  ut_check("7 byte bucket is ", 1, size_bucket_ndx(7));
  ut_check("8 byte bucket is ", 2, size_bucket_ndx(8));
  ut_check("15 byte bucket is ", 2, size_bucket_ndx(15));
  ut_check("2047 byte bucket is ", 9, size_bucket_ndx(2047));
  ut_check("2049 byte bucket is ", 10, size_bucket_ndx(2049));
  ut_check("4095 byte bucket is ", 10, size_bucket_ndx(4095));
  ut_check("4096 byte bucket is ", 11, size_bucket_ndx(4096));
  ut_check("4097 byte bucket is ", 11, size_bucket_ndx(4097));
  ut_check("1234567 byte bucket is ", 11, size_bucket_ndx(1234567));
  ut_delim();
  /* ------------------------------------------------------------------------ */
  ut_start("exercise failed malloc and basic lock");
  ut_delim();
  /* ------------------------------------------------------------------------ */
  ut_start("exercise age and size buckets");
  ut_clear();
  dmalloc_stats_alloc(0, t_0);
  dmalloc_stats_alloc(1, t_0);
  dmalloc_stats_alloc(2, t_0);
  dmalloc_stats_alloc(4, t_0);
  dmalloc_stats_alloc(8, t_0);
  dmalloc_stats_alloc(16, t_0);
  dmalloc_stats_alloc(32, t_0);
  dmalloc_stats_alloc(64, t_0);
  dmalloc_stats_alloc(128, t_0);
  dmalloc_stats_alloc(256, t_0);
  dmalloc_stats_alloc(512, t_0);
  dmalloc_stats_alloc(1024, t_0);
  dmalloc_stats_alloc(2048, t_0);
  dmalloc_stats_alloc(4096, t_0);
  dmalloc_stats_alloc(8192, t_0);
  int bytes = 0 + 1 + 2 + 4 + 8 + 16 + 32 + 64 + 128 + 256 + 512 + 1024	\
    + 2048 +4096 + 8192;
  ut_check("current allocation", bytes, dmalloc_stats.s_curr_countalloc);
  ut_check("alltime allocation", bytes, dmalloc_stats.s_total_countalloc);
  ut_check("age  bucket 0", 15, dmalloc_stats.s_agebucket[BUCKET_0000]);
  ut_check("size bucket 0", 3, dmalloc_stats.s_sizebucket[BUCKET_0000]);
  ut_check("size bucket 1", 1, dmalloc_stats.s_sizebucket[BUCKET_0004]);
  ut_check("size bucket 2", 1, dmalloc_stats.s_sizebucket[BUCKET_0008]);
  ut_check("size bucket 3", 1, dmalloc_stats.s_sizebucket[BUCKET_0016]);
  ut_check("size bucket 4", 1, dmalloc_stats.s_sizebucket[BUCKET_0032]);
  ut_check("size bucket 5", 1, dmalloc_stats.s_sizebucket[BUCKET_0064]);
  ut_check("size bucket 6", 1, dmalloc_stats.s_sizebucket[BUCKET_0128]);
  ut_check("size bucket 7", 1, dmalloc_stats.s_sizebucket[BUCKET_0256]);
  ut_check("size bucket 8", 1, dmalloc_stats.s_sizebucket[BUCKET_0512]);
  ut_check("size bucket 9", 1, dmalloc_stats.s_sizebucket[BUCKET_1024]);
  ut_check("size bucket 10", 1, dmalloc_stats.s_sizebucket[BUCKET_2048]);
  ut_check("size bucket 11", 2, dmalloc_stats.s_sizebucket[BUCKET_4096]);
  ut_check("declined update", 14, dmalloc_stats._s_declined_update);
  dmalloc_stats_alloc(8192, t_1);
  bytes += 8192;
  ut_check("size bucket 11", 3, dmalloc_stats.s_sizebucket[BUCKET_4096]);
  ut_check("declined update", 14, dmalloc_stats._s_declined_update);
  ut_check("age  bucket 0", 1, dmalloc_stats.s_agebucket[0]);
  ut_check("age  bucket 1", 15, dmalloc_stats.s_agebucket[1]);
  ut_check("current allocation", bytes, dmalloc_stats.s_curr_countalloc);
  ut_check("alltime allocation", bytes, dmalloc_stats.s_total_countalloc);
  dmalloc_stats_alloc(8192, t_999);
  bytes += 8192;
  ut_check("size bucket 11", 4, dmalloc_stats.s_sizebucket[BUCKET_4096]);
  ut_check("age  bucket 0", 1, dmalloc_stats.s_agebucket[0]);
  ut_check("age  bucket 998", 1, dmalloc_stats.s_agebucket[998]);
  ut_check("age  bucket 999", 15, dmalloc_stats.s_agebucket[999]);
  dmalloc_stats_alloc(8192, t_1500);
  bytes += 8192;
  ut_check("size bucket 11", 5, dmalloc_stats.s_sizebucket[BUCKET_4096]);
  ut_check("age  bucket 0", 1, dmalloc_stats.s_agebucket[0]);
  ut_check("age  bucket 999", 16, dmalloc_stats.s_agebucket[999]);
  dmalloc_stats_alloc(8192, t_2000);
  bytes += 8192;
  ut_check("size bucket 11", 6, dmalloc_stats.s_sizebucket[BUCKET_4096]);
  ut_check("age  bucket 0", 1, dmalloc_stats.s_agebucket[0]);
  ut_check("age  bucket 999", 17, dmalloc_stats.s_agebucket[999]);
  ut_check("current allocation", bytes, dmalloc_stats.s_curr_countalloc);
  ut_check("alltime allocation", bytes, dmalloc_stats.s_total_countalloc);

  ut_mark("now check free:");
  ut_check("current allocation", bytes, dmalloc_stats.s_curr_countalloc);
  ut_check("alltime allocation", bytes, dmalloc_stats.s_total_countalloc);

  dmalloc_stats_free(bytes * 2, t_2000, t_0);
  ut_check("current allocation", bytes, dmalloc_stats.s_curr_countalloc);
  ut_check("alltime allocation", bytes, dmalloc_stats.s_total_countalloc);
  ut_check("size underrun", 1, dmalloc_stats._s_underrun_sizebucket);
  int tbytes = bytes;
  dmalloc_stats_free(8192, t_2000, t_2000);
  dmalloc_stats_free(8192, t_2000, t_1500);
  dmalloc_stats_free(8192, t_2000, t_999);
  dmalloc_stats_free(8192, t_2000, t_1);
  dmalloc_stats_free(8192, t_2000, t_0);
  bytes -= 8192 * 5;
  dmalloc_stats_free(4096, t_2000, t_0);
  bytes -= 4096;
  ut_check("current allocation", bytes, dmalloc_stats.s_curr_countalloc);
  ut_check("alltime allocation", tbytes, dmalloc_stats.s_total_countalloc);
  ut_check("age  bucket 999", 13, dmalloc_stats.s_agebucket[999]);
  ut_check("age  bucket 0", 0, dmalloc_stats.s_agebucket[0]);
  dmalloc_stats_free(4096, t_2000, t_0);
  ut_check("current allocation", bytes, dmalloc_stats.s_curr_countalloc);
  ut_check("age  bucket 0", 0, dmalloc_stats.s_agebucket[0]);
  dmalloc_stats_free(2048, t_2000, t_0);
  dmalloc_stats_free(1024, t_2000, t_0);
  dmalloc_stats_free(512, t_2000, t_0);
  dmalloc_stats_free(256, t_2000, t_0);
  dmalloc_stats_free(128, t_2000, t_0);
  dmalloc_stats_free(64, t_2000, t_0);
  dmalloc_stats_free(32, t_2000, t_0);
  dmalloc_stats_free(16, t_2000, t_0);
  dmalloc_stats_free(8, t_2000, t_0);
  dmalloc_stats_free(4, t_2000, t_0);
  dmalloc_stats_free(2, t_2000, t_0);
  dmalloc_stats_free(1, t_2000, t_0);
  dmalloc_stats_free(0, t_2000, t_0);
  ut_check("current allocation", 0, dmalloc_stats.s_curr_countalloc);
  ut_check("alltime allocation", tbytes, dmalloc_stats.s_total_countalloc);
  ut_check("age bucket 999", 0, dmalloc_stats.s_agebucket[999]);
  ut_check("age bucket 0", 0, dmalloc_stats.s_agebucket[0]);

  int population = 0;
  for (int i=0; i< BUCKETS_AGE_NUM; i++) {
    if (dmalloc_stats.s_agebucket[i]) printf("age_bucket: %d residents at index %d\n",\
				       dmalloc_stats.s_agebucket[i], i);
    population += dmalloc_stats.s_agebucket[i];
  }
  ut_check("age bucket population", 0, population);

  population = 0;
  for (int i=0; i< BUCKETS_SIZE_NUM; i++) {
    if (dmalloc_stats.s_sizebucket[i]) printf("sz_bucket: %d residents at index %d\n",\
				      dmalloc_stats.s_sizebucket[i], i);
    population += dmalloc_stats.s_sizebucket[i];
  }
  ut_check("size bucket population", 0, population);
  ut_delim();

  /*------------------------------------------------------------------------- */
}
#endif	/* DMALLOC_UNIT_TEST */

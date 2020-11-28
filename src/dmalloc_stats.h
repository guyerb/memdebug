#ifndef DMALLOC_STATS_H
#define DMALLOC_STATS_H

enum bucket_index {
  BUCKET_0000, BUCKET_0004, BUCKET_0008, BUCKET_0016, BUCKET_0032, BUCKET_0064,
  BUCKET_0128, BUCKET_0256, BUCKET_0512, BUCKET_1024, BUCKET_2048, BUCKET_4096,
};
#define BUCKETS_SIZE_NUM 12	/* N.B. adjust if you increase enums above*/
#define BUCKETS_AGE_NUM  1000

/* can easily be extended to uint64_t, logging and unit test output would need work */
struct dmalloc_alloc_stats {
  uint32_t s_allocated_current;
  uint32_t s_allocated_alltime;
  uint32_t s_sizebuckets[BUCKETS_SIZE_NUM]; /* allocations by size */
  uint32_t s_agebuckets[BUCKETS_AGE_NUM];  /* allocations by age (seconds) */
  uint32_t s_null_frees;
  uint32_t s_failed_allocs;
  uint32_t s_invalid_birthdays;
  time_t   s_agebuckets_lastupdate;

  /* internal debugging */
  uint32_t _s_agebucket_underrun_error;
  uint32_t _s_sizebucket_underrun_error;
  uint32_t _s_lockerror;
  uint32_t _s_declined_updates;
};

/* get a coherent copy of stats structure */
extern void dmalloc_stats_getter(struct dmalloc_alloc_stats *);

#endif /* DMALLOC_STATS_H */


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
  uint32_t s_curr_sizealloc;
  uint32_t s_curr_countalloc;
  uint32_t s_total_countalloc;
  uint32_t s_sizebucket[BUCKETS_SIZE_NUM]; /* allocations by size */
  uint32_t s_agebucket[BUCKETS_AGE_NUM];  /* allocations by age (seconds) */
  uint32_t s_null_free;
  uint32_t s_fail_alloc;
  time_t   s_last_ageupdate;
  time_t   s_last_logupdate;

  /* internal debugging */
  uint32_t _s_underrun_agebucket;
  uint32_t _s_underrun_sizebucket;
  uint32_t _s_errorlock;
  uint32_t _s_declined_update;
  uint32_t _s_invalid_birthday;
};

/* get a coherent copy of stats structure */
extern void dmalloc_stats_getter(struct dmalloc_alloc_stats *);

#endif /* DMALLOC_STATS_H */


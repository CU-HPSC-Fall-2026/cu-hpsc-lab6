#pragma once

#include <time.h>
#include <stdio.h>

// According to https://stackoverflow.com/a/74757391/7564988, the kernel (on Linux, hopefully with OS X and WSL too) should inject kernel code to have the time taken be insignificant
// Using CLOCK_MONOTONIC_RAW can lead to better behavior, if you're platform supports it (Linux should, Mac may not)

typedef double time_taken_t;
typedef struct timespec timespec_t;

static inline void get_current_time(timespec_t *timespec) {
  clock_gettime(CLOCK_MONOTONIC, timespec);
}

static inline double get_current_time_double() {
  timespec_t timespec;
  get_current_time(&timespec);
  return ((double)(timespec.tv_sec)) + ((double)(timespec.tv_nsec)) / 1E9;
}

static inline double time_delta(timespec_t start, timespec_t end) {
  return ((double)(end.tv_sec - start.tv_sec)) + ((double)(end.tv_nsec - start.tv_nsec)) / 1E9;
}

static inline double get_clock_resolution() {
  struct timespec timespec_res;
  clock_getres(CLOCK_MONOTONIC, &timespec_res);
  return timespec_res.tv_sec + (double)timespec_res.tv_nsec / 1E9;
}

typedef struct {
  double start;
  double total;
} mytimer_t;

static inline void mytimer_init(mytimer_t *timer) {
  timer->start = -1; // -1 means timer hasn't been started
  timer->total = 0;
}

static inline int mytimer_start(mytimer_t *timer) {
  if (timer->start != -1) {
    printf("error in timer start\n");
    return -1;
  }
  timer->start = get_current_time_double();
  return 0;
}

static inline int mytimer_end(mytimer_t *timer) {
  double current_time = get_current_time_double();
  if (timer->start == -1) {
    printf("error in timer end\n");
    return -1;
  }

  timer->total += current_time - timer->start;
  timer->start = -1;
  return 0;
}


// Below is the old timer implementation, which relies on x86 register functions
// It is not compatible with ARM based processors, so the above implementation should be good enough hopefully
// typedef unsigned long long ticks_t;
// ticks_t rdtsc() {
//   unsigned hi, lo;
//   __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
//   return ((ticks_t)lo)|( ((ticks_t)hi)<<32);
// }

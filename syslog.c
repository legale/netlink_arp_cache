#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <pthread.h> // pthread_spinlock_t, pthread_spin_lock, pthread_spin_unlock
#include <stdarg.h>  // va_list, va_start(), va_end()
#include <stdatomic.h>
#include <stdio.h> // printf()
#include <stdlib.h>
#include <sys/syscall.h> // SYS_gettid
#include <sys/time.h>    // gettimeofday()
#include <syslog.h>      // syslog()
#include <time.h>        // localtime(), struct tm
#include <unistd.h>      // syscall()

#include "syslog.h"

static const char *priority_texts[] = {
    "M", // EMERG
    "A", // ALERT
    "C", // CRITICAL
    "E", // ERROR
    "W", // WARNING
    "N", // NOTICE
    "I", // INFO
    "D", // DEBUG
};

static inline const char *strprio(int pri) {
  return (pri >= LOG_EMERG && pri <= LOG_DEBUG) ? priority_texts[pri] : "UNKNOWN";
}

// Spinlock for synchronizing threads
static pthread_spinlock_t spinlock;

static atomic_int initialized = 0;
static time_t offset_seconds;
static long timezone_offset;

static void initialize_time_cache() {
  if (atomic_exchange(&initialized, 1) == 0) {
    struct timespec startup_time_realtime;
    struct timespec startup_time_monotonic;
    clock_gettime(CLOCK_REALTIME, &startup_time_realtime);
    clock_gettime(CLOCK_MONOTONIC, &startup_time_monotonic);

    offset_seconds = startup_time_realtime.tv_sec - startup_time_monotonic.tv_sec;

    struct tm local_tm;
    localtime_r(&startup_time_realtime.tv_sec, &local_tm);
    timezone_offset = local_tm.tm_gmtoff;

    pthread_spin_init(&spinlock, PTHREAD_PROCESS_PRIVATE);
  }
}

static inline void get_current_time(struct timespec *ts, struct tm *tm_info) {
  static time_t cached_offset = 0;
  static long cached_tz_offset = 0;

  if (!atomic_load(&initialized)) {
    initialize_time_cache();
  }

  clock_gettime(CLOCK_MONOTONIC_COARSE, ts);

  if (__builtin_expect(cached_offset == 0, 0)) {
    cached_offset = offset_seconds;
    cached_tz_offset = timezone_offset;
  }

  time_t current_timestamp = cached_offset + ts->tv_sec + cached_tz_offset;
  gmtime_r(&current_timestamp, tm_info);
}

void syslog2_(int pri, const char *filename, int line, const char *fmt, ...) {
  if (!(setlogmask(0) & LOG_MASK(pri))) return;

  char msg[32768];
  char timebuf[32];

  struct timespec ts;
  struct tm tm_info;
  pid_t tid = syscall(SYS_gettid);

  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);

  get_current_time(&ts, &tm_info);

  sprintf(timebuf, "%02d-%02d-%02d %02d:%02d:%02d.%03ld",
    tm_info.tm_mday, tm_info.tm_mon + 1, tm_info.tm_year + 1900,
    tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec, ts.tv_nsec / 1000000);

  pthread_spin_lock(&spinlock);

  printf("[%s] %s [%d] %s:%d: %s\n", timebuf, strprio(pri), tid, filename, line, msg);
  syslog(pri, "[%d] %s:%d: %s", tid, filename, line, msg);

  pthread_spin_unlock(&spinlock);
}

void syslog2_printf_(int pri, const char *filename, int line, const char *fmt, ...) {
  if (!(setlogmask(0) & LOG_MASK(pri))) return;

  char msg[32768];

  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);


  printf("%s", msg);
  syslog(pri, "%s", msg);

}

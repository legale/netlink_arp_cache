#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <pthread.h> // pthread_spinlock_t, pthread_spin_lock, pthread_spin_unlock
#include <stdarg.h>  // va_list, va_start(), va_end()
#include <stdio.h>   // printf()
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
// static pthread_spinlock_t spinlock;

static bool log_syslog = true;
int cached_mask = -1;

static int syslog_initialized = 0;
static time_t offset_seconds_global;
static long timezone_offset_global;

static void initialize_time_cache() {
  struct timespec startup_time_realtime;
  struct timespec startup_time_monotonic;
  clock_gettime(CLOCK_REALTIME, &startup_time_realtime);
  clock_gettime(CLOCK_MONOTONIC, &startup_time_monotonic);

  offset_seconds_global = startup_time_realtime.tv_sec - startup_time_monotonic.tv_sec;

  struct tm local_tm;
  localtime_r(&startup_time_realtime.tv_sec, &local_tm);
  timezone_offset_global = local_tm.tm_gmtoff;
}

int setlogmask2(int log_level) {
  cached_mask = log_level;
  return setlogmask(log_level);
}

void setup_syslog2(int log_level, bool set_log_syslog) {
  if (!syslog_initialized) {
    initialize_time_cache();
  }
  setlogmask2(LOG_UPTO(log_level));
  log_syslog = set_log_syslog;
  syslog_initialized = 1;
}

int clock_gettime_fast(struct timespec *ts, bool coarse) {
  // Получаем время от монотонных часов
  int type = coarse ? CLOCK_MONOTONIC_COARSE : CLOCK_MONOTONIC;
  int ret = clock_gettime(type, ts);
  if (!ret) {
    // Добавляем смещение и получаем время UTC, такое же как
    // clock_gettime(CLOCK_REALTIME, ts)
    ts->tv_sec += offset_seconds_global;
  }
  return ret;
}

static void get_current_time(struct timespec *ts, struct tm *tm_info) {
  if (!clock_gettime_fast(ts, true)) {
    time_t current_timestamp = ts->tv_sec + timezone_offset_global;
    gmtime_r(&current_timestamp, tm_info);
  }
}

void syslog2_(int pri, const char *func, const char *filename, int line, const char *fmt, bool add_nl, ...) {
  if (!(cached_mask & LOG_MASK(pri))) return;

  char msg[32768];

  struct timespec ts;
  struct tm tm_info;
  pid_t tid = syscall(SYS_gettid);

  va_list args;
  va_start(args, add_nl);
  vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);

  if (likely(log_syslog)) {
    if (add_nl) {
      syslog(pri, "[%d] %s:%d: %s: %s\n", tid, filename, line, func, msg);
    } else {
      syslog(pri, "[%d] %s:%d: %s: %s", tid, filename, line, func, msg);
    }
  } else {
    char timebuf[64];
    get_current_time(&ts, &tm_info);
    sprintf(timebuf, "%02d-%02d-%02d %02d:%02d:%02d.%03ld",
            tm_info.tm_mday, tm_info.tm_mon + 1, tm_info.tm_year + 1900,
            tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec, ts.tv_nsec / 1000000);
    if (add_nl) {
      printf("[%s] %s [%d] %s:%d: %s: %s\n", timebuf, strprio(pri), tid, filename, line, func, msg);
    } else {
      printf("[%s] %s [%d] %s:%d: %s: %s", timebuf, strprio(pri), tid, filename, line, func, msg);
    }
  }
}

void syslog2_printf_(int pri, const char *func, const char *filename, int line, const char *fmt, ...) {
  (void)filename;
  (void)func;
  (void)line;
  if (!(cached_mask & LOG_MASK(pri))) return;

  char msg[32768];

  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);

  if (likely(log_syslog)) {
    syslog(pri, "%s", msg);
  } else {
    printf("%s", msg);
  }
}

void debug(const char *fmt, ...) {
#ifdef DEBUG
  char str[256];
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
#endif
}

#ifndef SYSLOG2_H_
#define SYSLOG2_H_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdarg.h>      // va_list, va_start(), va_end()
#include <stdbool.h>     //bool type
#include <stdio.h>       // printf()
#include <sys/syscall.h> // SYS_gettid
#include <syslog.h>      // syslog()
#include <time.h>        // struct timespec
#include <unistd.h>      // syscall()

#include "pthread.h" //SET_CURRENT_FUNCTION

// global cached mask value
extern int cached_mask;

#ifndef likely
#define likely(x) __builtin_expect(!!(x), 1)
#endif

#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

#ifndef DBG
#define DBG printf(__FILE__ ":%d %s\n", __LINE__, __func__)
#endif

#ifndef FUNC_START_DEBUG
#define FUNC_START_DEBUG                      \
  do {                                        \
    if ((cached_mask & LOG_MASK(LOG_INFO))) { \
      syslog2(LOG_INFO, "");                  \
    }                                         \
  } while (0)
#endif

int setlogmask2(int log_level);
void setup_syslog2(int log_level, bool set_log_syslog);
void syslog2_(int pri, const char *func, const char *filename, int line, const char *fmt, bool add_nl, ...);
void syslog2_printf_(int pri, const char *func, const char *filename, int line, const char *fmt, ...);
int clock_gettime_fast(struct timespec *ts, bool coarse);
void debug(const char *fmt, ...);

#define __FILENAME__ (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)

#ifndef syslog2
#define syslog2(pri, fmt, ...) syslog2_(pri, __func__, __FILENAME__, __LINE__, fmt, true, ##__VA_ARGS__)
#endif // syslog2

#ifndef syslog2_nonl
#define syslog2_nonl(pri, fmt, ...) syslog2_(pri, __func__, __FILENAME__, __LINE__, fmt, false, ##__VA_ARGS__)
#endif // syslog2

#ifndef syslog2_printf
#define syslog2_printf(pri, fmt, ...) syslog2_printf_(pri, __func__, __FILENAME__, __LINE__, fmt, ##__VA_ARGS__)
#endif // syslog2_printf

#endif /* SYSLOG2_H_ */

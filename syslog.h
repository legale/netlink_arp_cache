#ifndef SYSLOG2_H_
#define SYSLOG2_H_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <syslog.h>   // syslog()
#include <stdarg.h>   // va_list, va_start(), va_end()
#include <stdio.h>    // printf()
#include <unistd.h>   // syscall()
#include <sys/syscall.h>   // SYS_gettid

void syslog2_(int pri, const char *filename, int line, const char *fmt, ...);


#ifndef syslog2
#define syslog2(pri, fmt, ...) syslog2_(pri, __FILE__, __LINE__, fmt, ##__VA_ARGS__)                                                                               \


/* this MACRO is also working 
#define syslog2(pri, fmt, ...)                                                                                 \
  {                                                                                                            \
    static char msg__[32768];                                                                                  \
    (void)snprintf(msg__, 32768, "[%ld] %s:%d: " fmt, syscall(SYS_gettid), __FILE__, __LINE__, ##__VA_ARGS__); \
    syslog(pri, "%s", msg__);                                                                                  \
    if ((setlogmask(0) & LOG_MASK(pri))) {                                                                     \
      (void)printf("%s", msg__);                                                                               \
    }                                                                                                          \
  }
*/

#endif // syslog2


#endif /* SYSLOG2_H_ */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdarg.h>      // va_list, va_start(), va_end()
#include <stdio.h>       // printf()
#include <sys/syscall.h> // SYS_gettid
#include <syslog.h>      // syslog()
#include <unistd.h>      // syscall()

#include "syslog.h"

static const char *priority_texts[] = {
    "EMERG",
    "ALERT",
    "CRIT ",
    "ERROR",
    "WARN ",
    "NOTIC",
    "INFO ",
    "DEBUG"};

static const char *strprio(int pri) {
  if (pri >= LOG_EMERG && pri <= LOG_DEBUG) {
    return priority_texts[pri];
  }
  return "UNKNOWN";
}

void syslog2_(int pri, const char *filename, int line, const char *fmt, ...) {
  char msg[32768];
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 32768, fmt, args);
  va_end(args);
  syslog(pri, "[%ld] %s:%d: %s", syscall(SYS_gettid), filename, line, msg);
  if ((setlogmask(0) & LOG_MASK(pri))) {
    printf("%s [%ld] %s:%d: %s", strprio(pri), syscall(SYS_gettid), filename, line, msg);
  }
}

#include "runner.h"

#include <mach/mach.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

enum { SYN_TEST_CAPACITY = 256 };

typedef struct {
  const char *name;
  syn_test_fn fn;
} syn_test_entry;

static syn_test_entry syn_tests[SYN_TEST_CAPACITY];
static size_t syn_test_count;
static syn_test_entry syn_abort_cases[SYN_TEST_CAPACITY];
static size_t syn_abort_case_count;
static int syn_failures_in_current_test;

static void syn_append(syn_test_entry *table, size_t *count, const char *kind,
                       const char *name, syn_test_fn fn) {
  if (*count >= SYN_TEST_CAPACITY) {
    fprintf(stderr, "runner: more than %d %ss in one suite\n",
            SYN_TEST_CAPACITY, kind);
    abort();
  }
  table[*count].name = name;
  table[*count].fn = fn;
  (*count)++;
}

void syn_test_register(const char *name, syn_test_fn fn) {
  syn_append(syn_tests, &syn_test_count, "test", name, fn);
}

void syn_test_register_abort_case(const char *name, syn_test_fn fn) {
  syn_append(syn_abort_cases, &syn_abort_case_count, "abort case", name, fn);
}

void syn_test_fail(const char *file, int line, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  fflush(stdout);
  fprintf(stderr, "%s:%d: ", file, line);
  vfprintf(stderr, fmt, args);
  fputc('\n', stderr);
  fflush(stderr);
  va_end(args);
  syn_failures_in_current_test++;
}

/* A deliberate abort otherwise leaves an .ips crash report in the user's
 * diagnostics for every case. Dropping the crash exception port keeps
 * ReportCrash out of it; the process still dies on SIGABRT. */
static void syn_silence_crash_reporter(void) {
  (void)task_set_exception_ports(mach_task_self(),
                                 EXC_MASK_CRASH | EXC_MASK_CORPSE_NOTIFY,
                                 MACH_PORT_NULL, EXCEPTION_DEFAULT,
                                 THREAD_STATE_NONE);
}

static int syn_run_abort_case(const char *name) {
  syn_silence_crash_reporter();
  for (size_t i = 0; i < syn_abort_case_count; i++) {
    if (strcmp(syn_abort_cases[i].name, name) != 0) continue;
    syn_failures_in_current_test = 0;
    syn_abort_cases[i].fn();
    return syn_failures_in_current_test > 0 ? 1 : 0;
  }
  fprintf(stderr, "runner: no abort case named \"%s\"\n", name);
  return 2;
}

int syn_test_main(int argc, char **argv) {
  if (argc == 3 && strcmp(argv[1], SYN_ABORT_CASE_FLAG) == 0)
    return syn_run_abort_case(argv[2]);

  size_t failed = 0;
  for (size_t i = 0; i < syn_test_count; i++) {
    syn_failures_in_current_test = 0;
    syn_tests[i].fn();
    if (syn_failures_in_current_test > 0) {
      failed++;
      printf("FAIL %s\n", syn_tests[i].name);
    } else {
      printf("ok   %s\n", syn_tests[i].name);
    }
    fflush(stdout);
  }
  printf("%zu test(s), %zu failure(s)\n", syn_test_count, failed);
  fflush(stdout);
  return failed > 0 ? 1 : 0;
}

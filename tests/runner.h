/*
 * Syntonic's test runner (KTD2).
 *
 * One executable per suite, `main` on thread 1 so the main-thread rule (R10)
 * holds with no annotations. A suite is a plain C file that defines tests with
 * SYN_TEST; the runner supplies `main` and runs them in registration order.
 *
 * Pass lines and the summary go to stdout; failures go to stderr, so a child
 * process spawned by syn_test_expect_abort reports its failures through the
 * captured pipe.
 */

#ifndef SYNTONIC_TESTS_RUNNER_H
#define SYNTONIC_TESTS_RUNNER_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* Selects a single abort case instead of the suite: `<exe> --syn-abort-case
 * <name>`. syn_test_expect_abort re-executes the suite binary with it. */
#define SYN_ABORT_CASE_FLAG "--syn-abort-case"

typedef void (*syn_test_fn)(void);

void syn_test_register(const char *name, syn_test_fn fn);
void syn_test_register_abort_case(const char *name, syn_test_fn fn);

/* Records a failure against the running test and prints file, line and the
 * message to stderr. Prefer the SYN_ASSERT macros, which also stop the test. */
void syn_test_fail(const char *file, int line, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

/* Runs the suite, or the one case named by SYN_ABORT_CASE_FLAG. Returns the
 * process exit status. `main` lives in support.m so every test runs inside an
 * autorelease pool. */
int syn_test_main(int argc, char **argv);

/* Defines and registers a test. The body follows the macro. */
#define SYN_TEST(name)                                                        \
  static void syn_test_body_##name(void);                                     \
  __attribute__((constructor)) static void syn_test_reg_##name(void) {        \
    syn_test_register(#name, syn_test_body_##name);                           \
  }                                                                           \
  static void syn_test_body_##name(void)

/* Defines a case that only runs in a spawned child, named by
 * syn_test_expect_abort. It never runs as part of the suite, so a case that
 * aborts or fails on purpose leaves the suite green. */
#define SYN_ABORT_CASE(name)                                                  \
  static void syn_abort_body_##name(void);                                    \
  __attribute__((constructor)) static void syn_abort_reg_##name(void) {       \
    syn_test_register_abort_case(#name, syn_abort_body_##name);               \
  }                                                                           \
  static void syn_abort_body_##name(void)

/* Every assertion stops the test at the point of failure: continuing past a
 * broken AppKit expectation usually crashes instead of reporting. */
#define SYN_FAIL(...)                                                         \
  do {                                                                        \
    syn_test_fail(__FILE__, __LINE__, __VA_ARGS__);                           \
    return;                                                                   \
  } while (0)

#define SYN_ASSERT(cond)                                                      \
  do {                                                                        \
    if (!(cond)) SYN_FAIL("assertion failed: %s", #cond);                     \
  } while (0)

#define SYN_ASSERT_MSG(cond, ...)                                             \
  do {                                                                        \
    if (!(cond)) SYN_FAIL(__VA_ARGS__);                                       \
  } while (0)

#define SYN_ASSERT_STR_EQ(actual, expected)                                   \
  do {                                                                        \
    const char *syn_actual_ = (actual);                                       \
    const char *syn_expected_ = (expected);                                   \
    if (syn_actual_ == NULL || strcmp(syn_actual_, syn_expected_) != 0)       \
      SYN_FAIL("expected \"%s\", got \"%s\"", syn_expected_,                  \
               syn_actual_ ? syn_actual_ : "(null)");                         \
  } while (0)

#endif /* SYNTONIC_TESTS_RUNNER_H */

/*
 * The harness checking itself (U3): registration, assertions, the exit status,
 * the off-screen bootstrap, the run-loop spin and barrier, and the abort
 * helper that U4 onward uses for the debug checks (KTD4).
 *
 * The cases that fail or abort on purpose are SYN_ABORT_CASE, so they only run
 * in a spawned child and this suite stays green.
 */

#include <stdio.h>
#include <string.h>

#include "runner.h"
#include "support.h"
#include "syntonic/syntonic.h"

/* ---- the library still links from a plain C suite ---- */

SYN_TEST(version_string_is_usable_from_c) {
  const char *version = ns_version_string();
  SYN_ASSERT(version != NULL);
  SYN_ASSERT(strlen(version) > 0);
}

/* ---- off-screen bootstrap (KTD15) ---- */

SYN_TEST(bootstrap_yields_a_prohibited_application) {
  syn_test_bootstrap();
  syn_test_bootstrap(); /* idempotent: every suite may call it per test */
  SYN_ASSERT_STR_EQ(syn_test_class_name(syn_test_shared_application()),
                    "NSApplication");
  SYN_ASSERT_MSG(syn_test_activation_policy_is_prohibited(),
                 "the shared application is not prohibited, so it would take "
                 "a Dock icon");
}

/* ---- run loop ---- */

static bool syn_spin_flag;
static bool syn_barrier_flag;

SYN_TEST(spin_runs_the_whole_interval_and_fires_a_timer) {
  syn_test_bootstrap();
  syn_spin_flag = false;
  syn_test_schedule_flag(10.0, &syn_spin_flag);

  double started = syn_test_now_ms();
  syn_test_spin(150.0);
  double elapsed = syn_test_now_ms() - started;

  SYN_ASSERT_MSG(elapsed >= 150.0, "spin(150) returned after %.1f ms", elapsed);
  SYN_ASSERT_MSG(syn_spin_flag, "a 10 ms timer had not fired after %.1f ms",
                 elapsed);
}

SYN_TEST(wait_for_returns_as_soon_as_the_callback_lands) {
  syn_test_bootstrap();
  syn_barrier_flag = false;
  syn_test_schedule_flag(20.0, &syn_barrier_flag);

  double started = syn_test_now_ms();
  SYN_WAIT_FOR(syn_barrier_flag, 5000.0);
  double elapsed = syn_test_now_ms() - started;

  SYN_ASSERT_MSG(elapsed < 1000.0,
                 "the barrier waited %.1f ms for a 20 ms timer", elapsed);
}

/* ---- cases that only ever run in a spawned child ---- */

SYN_ABORT_CASE(deliberate_abort) {
  syn_test_bootstrap(); /* a live AppKit, as every real debug check has */
  fprintf(stderr, "u3 deliberate abort\n");
  abort();
}

SYN_ABORT_CASE(appkit_exception) {
  syn_test_raise_in_wrapper();
}

SYN_ABORT_CASE(returns_cleanly) {
  syn_test_bootstrap();
  fprintf(stderr, "u3 case returned\n");
}

SYN_ABORT_CASE(failing_assertion) {
  SYN_ASSERT_MSG(1 == 2, "u3 deliberate failure");
}

/* ---- the abort helper ---- */

SYN_TEST(expect_abort_reports_an_abort_and_captures_stderr) {
  SYN_ASSERT_ABORTS("deliberate_abort", "u3 deliberate abort");
}

SYN_TEST(expect_abort_captures_an_appkit_exception) {
  SYN_ASSERT_ABORTS("appkit_exception", "NSInvalidArgumentException");
}

SYN_TEST(expect_abort_reports_no_abort_for_a_case_that_returns) {
  syn_test_abort_result result = syn_test_expect_abort("returns_cleanly");
  bool ok = !result.aborted && !result.timed_out && result.signal == 0 &&
            result.exit_status == 0 &&
            strstr(result.stderr_text, "u3 case returned") != NULL;
  int signal_number = result.signal;
  int exit_status = result.exit_status;
  if (!ok) fprintf(stderr, "child stderr:\n%s", result.stderr_text);
  syn_test_abort_result_free(&result);

  SYN_ASSERT_MSG(ok, "a case that returns reported signal %d, exit %d",
                 signal_number, exit_status);
}

SYN_TEST(a_failing_assertion_exits_one_naming_file_and_line) {
  syn_test_abort_result result = syn_test_expect_abort("failing_assertion");
  bool ok = !result.aborted && !result.timed_out && result.signal == 0 &&
            result.exit_status == 1 &&
            strstr(result.stderr_text, "test_runner_self.c:") != NULL &&
            strstr(result.stderr_text, "u3 deliberate failure") != NULL;
  int signal_number = result.signal;
  int exit_status = result.exit_status;
  if (!ok) fprintf(stderr, "child stderr:\n%s", result.stderr_text);
  syn_test_abort_result_free(&result);

  SYN_ASSERT_MSG(ok,
                 "a failing assertion reported signal %d, exit %d; expected "
                 "exit 1 with file and line on stderr",
                 signal_number, exit_status);
}

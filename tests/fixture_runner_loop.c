/*
 * The fixture tests/test_runner_self.c spawns to prove the runner's ordinary
 * pass/fail loop (U3).
 *
 * Every suite reports through that loop - the "ok" and "FAIL" lines, the
 * summary and the exit status - and nothing else exercises it: an abort case
 * runs through the case selector instead. So this binary holds one plain
 * SYN_TEST that passes and one that fails on purpose, which makes running it
 * print both lines and exit 1.
 *
 * That is why it is deliberately not a tests/test_*.c file and why CMake
 * deliberately does not register it with CTest: `just test` never runs it, and
 * the suite that spawns it reads its stdout.
 */

#include "runner.h"

SYN_TEST(a_passing_test_for_the_loop_to_report) {
  SYN_ASSERT(1 == 1);
}

SYN_TEST(a_failing_test_for_the_loop_to_report) {
  SYN_FAIL("u3 runner loop fixture failure");
}

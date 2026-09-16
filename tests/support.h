/*
 * AppKit bootstrap and off-screen helpers for the test suites (KTD15, R21).
 *
 * Off-screen means: the shared application exists with the prohibited
 * activation policy, so nothing appears in the Dock and no window is ordered
 * front. Many AppKit initializers assert that the application exists, so call
 * syn_test_bootstrap before touching anything from AppKit.
 *
 * Pure C: suites are C files, only this file's implementation is Objective-C.
 */

#ifndef SYNTONIC_TESTS_SUPPORT_H
#define SYNTONIC_TESTS_SUPPORT_H

#include <stdbool.h>
#include <stdlib.h>

#include <syntonic/ns_base.h>

#include "runner.h"

/* Creates the shared application with the prohibited activation policy and
 * finishes launching. Idempotent; each suite is its own process. */
void syn_test_bootstrap(void);

/* The shared application as an opaque handle, for the class-name probe. */
const void *syn_test_shared_application(void);

/* True when the shared application's activation policy is prohibited. */
bool syn_test_activation_policy_is_prohibited(void);

/* The Objective-C class name behind a handle, as -class reports it: an object
 * AppKit has put under KVO hides its generated subclass there. Runtime-owned,
 * never freed. */
const char *syn_test_class_name(const void *handle);

/* Monotonic milliseconds, for measuring waits. */
double syn_test_now_ms(void);

/* Runs the run loop for the whole interval, then returns. Use this only when a
 * plain interval is what is under test; for a callback, use SYN_WAIT_FOR. */
void syn_test_spin(double milliseconds);

/* Schedules a timer that sets *flag to true after the delay. The flag must
 * outlive the wait. */
void syn_test_schedule_flag(double delay_ms, bool *flag);

/* Spins the run loop until `expr` is true, failing the test by name when the
 * timeout expires. This is the barrier for every asynchronous callback: a
 * fixed syn_test_spin is flaky in both directions. `expr` is evaluated more
 * than once, so keep it a plain flag or getter. */
#define SYN_WAIT_FOR(expr, timeout_ms)                                        \
  do {                                                                        \
    double syn_deadline_ = syn_test_now_ms() + (double)(timeout_ms);          \
    while (!(expr) && syn_test_now_ms() < syn_deadline_) syn_test_spin(5.0);  \
    if (!(expr))                                                              \
      SYN_FAIL("timed out after %g ms waiting for: %s",                       \
               (double)(timeout_ms), #expr);                                  \
  } while (0)

/* Makes AppKit raise NSInvalidArgumentException inside the call, for the
 * wrapper exception check (KTD4). It never returns normally, so call it only
 * from an abort case or from behind a handler that catches. */
void syn_test_raise_in_wrapper(void);

/* ---- plain objects and checked shims for the kernel suite (U4) ---- */

/* True when the calling thread is the main thread (R10). */
bool syn_test_is_main_thread(void);

/* Owned (+1) handles to plain objects, for the ownership and class checks.
 * Release each one with ns_release. NSButton is an NSView subclass, NSDate is
 * unrelated to both, and NSString backs the outbound string copy. */
const void *syn_test_view_create(void);
const void *syn_test_button_create(void);
const void *syn_test_date_create(void);
const void *syn_test_string_create(const char *utf8);

/* True once the object the last syn_test_view_create handed out has
 * deallocated, so a C test can watch the last reference go (R7). */
bool syn_test_view_is_gone(void);

/* Shims written with the library's own internal macros, so a C test can drive
 * the debug checks that every wrapper body carries (KTD4, R12). These two take
 * the handle at an NSView position, non-null and nullable in turn. */
void syn_test_expect_view(const void *handle);
void syn_test_expect_view_optional(const void *handle);

/* The library's inbound string conversion at a non-null and at a nullable
 * position; each reports whether the conversion produced nil. */
bool syn_test_string_in_is_nil(const char *utf8);
bool syn_test_string_in_optional_is_nil(const char *utf8);

/* The library's outbound string copy of the NSString behind `handle`. Owned:
 * free it with ns_string_free (R11). */
char *syn_test_string_copy(const void *handle);

/* A wrapper built with the library's entry macro that raises inside the call,
 * so the entry macro's exception report is what gets tested (KTD4). */
void syn_test_wrapper_raises(void);

/* ---- the callback machinery, for the U14 suites ---- */

/* Installs a target/action trampoline on an NSControl handle, and fires it.
 * The trampoline itself is what U7 installs on a button; these two drive it
 * before a wrapped control exists. A null `action` uninstalls. */
void syn_test_install_action(const void *handle, ns_action action,
                             void *context);
void syn_test_fire_action(const void *handle);

/* True when the object behind `handle` has a delegate at all, and whether that
 * delegate answers `selector`. AppKit caches the answer when the slot is
 * assigned, so the second one is how a suite sees what AppKit saw. */
bool syn_test_has_delegate(const void *handle);
bool syn_test_delegate_responds(const void *handle, const char *selector);

/* How many shims are alive, so a suite can watch a replaced or uninstalled one
 * go away. Negative in a release build, which keeps no counter. */
long syn_test_shim_live_count(void);

/* Asserts that exactly `expected` shims are alive. A release build keeps no
 * count and the assertion steps aside there. */
#define SYN_ASSERT_SHIMS(expected)                                            \
  do {                                                                        \
    long syn_live_ = syn_test_shim_live_count();                              \
    if (syn_live_ >= 0 && syn_live_ != (long)(expected))                      \
      SYN_FAIL("expected %ld live shim(s), found %ld", (long)(expected),      \
               syn_live_);                                                    \
  } while (0)

/* The shim machinery's debug validation of what a callback returns, and its
 * required-member report, driven through a stand-in protocol: no protocol U14
 * wraps has a required member or a member that returns anything, and six later
 * units have both (R9, KTD8). Each one aborts in a debug build on the value it
 * rejects, and does nothing at all under NDEBUG.
 *
 * The stand-in's table marks one member required, so installing with
 * `set_required` false is the missing-required-member case. */
void syn_test_install_required_member_struct(const void *handle,
                                             bool set_required);
void syn_test_shim_check_count(long count);
void syn_test_shim_check_returned_handle(const void *handle);

/* Posts an AppKit notification by name with `object` as its object. AppKit
 * delivers its own delegate notifications this way, so this fires a delegate
 * member that only a real launch or termination would otherwise reach. */
void syn_test_post_notification(const char *name, const void *object);

typedef struct {
  bool aborted;      /* terminated by SIGABRT */
  bool timed_out;    /* killed after the wait expired */
  int signal;        /* terminating signal, 0 when it exited normally */
  int exit_status;   /* exit code when signal is 0 */
  char *stderr_text; /* NUL-terminated, never null; free with the call below */
} syn_test_abort_result;

/* Re-executes this suite binary for the one named SYN_ABORT_CASE, in a child
 * process with a live AppKit of its own, and reports how it ended. Forking
 * without exec is not an option: CoreFoundation refuses to run after a fork
 * and AppKit aborts on its own, which would mask the abort under test. */
syn_test_abort_result syn_test_expect_abort(const char *case_name);

void syn_test_abort_result_free(syn_test_abort_result *result);

/* Returns NULL when the case aborted and its stderr contained `needle`, and
 * otherwise a malloc'd explanation including the child's stderr. */
char *syn_test_check_abort(const char *case_name, const char *needle);

/* Asserts that the named abort case died on SIGABRT with `needle` on stderr. */
#define SYN_ASSERT_ABORTS(case_name, needle)                                  \
  do {                                                                        \
    char *syn_why_ = syn_test_check_abort((case_name), (needle));             \
    if (syn_why_ != NULL) {                                                   \
      syn_test_fail(__FILE__, __LINE__, "%s", syn_why_);                      \
      free(syn_why_);                                                         \
      return;                                                                 \
    }                                                                         \
  } while (0)

#endif /* SYNTONIC_TESTS_SUPPORT_H */

/*
 * The kernel suite (U4): ownership, strings, availability, threading and the
 * debug misuse checks every wrapper is built on.
 *
 * The checks abort by design (KTD4), so each one is a SYN_ABORT_CASE driven
 * from a SYN_TEST with SYN_ASSERT_ABORTS: the case runs in a re-executed child
 * with an AppKit of its own and the parent reads its stderr.
 */

#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "runner.h"
#include "support.h"
#include "syntonic/syntonic.h"

/* ---- ownership: retain, release and the reference ledger (R7, KTD7) ---- */

SYN_TEST(retain_returns_the_handle_and_the_object_dies_on_the_last_release) {
  syn_test_bootstrap();

  const void *view = syn_test_view_create(); /* caller +1 */
  SYN_ASSERT(view != NULL);
  SYN_ASSERT_STR_EQ(syn_test_class_name(view), "NSView");

  SYN_ASSERT_MSG(ns_retain(view) == view, /* caller +2 */
                 "ns_retain did not hand back the handle it was given");
  SYN_ASSERT_MSG(!syn_test_view_is_gone(), "the object died while held twice");

  ns_release(view); /* caller +1 */
  SYN_ASSERT_MSG(!syn_test_view_is_gone(),
                 "the object died while one reference was still held");

  ns_release(view); /* caller 0 */
  SYN_ASSERT_MSG(syn_test_view_is_gone(),
                 "the object outlived its last reference");
}

SYN_TEST(the_utilities_do_nothing_at_all_with_null) {
  syn_test_bootstrap();
  SYN_ASSERT_MSG(ns_retain(NULL) == NULL, "ns_retain(NULL) did not yield null");
  ns_release(NULL);     /* R7: releasing null is a no-op */
  ns_string_free(NULL); /* and so is freeing a null string */
}

/* ---- strings (R11, KTD7) ---- */

SYN_TEST(a_copied_string_is_a_distinct_buffer_that_outlives_the_others) {
  syn_test_bootstrap();
  const void *source = syn_test_string_create("Syntonic");

  char *first = syn_test_string_copy(source);
  char *second = syn_test_string_copy(source);
  SYN_ASSERT(first != NULL && second != NULL);
  SYN_ASSERT_MSG(first != second, "two copies came back as one buffer");
  SYN_ASSERT_STR_EQ(first, "Syntonic");
  SYN_ASSERT_STR_EQ(second, "Syntonic");

  ns_string_free(first);
  SYN_ASSERT_STR_EQ(second, "Syntonic"); /* the other copy is untouched */

  char *third = syn_test_string_copy(source); /* the source still reads */
  SYN_ASSERT_STR_EQ(third, "Syntonic");

  ns_string_free(second);
  ns_string_free(third);
  ns_release(source);
}

SYN_TEST(inbound_null_is_nil_at_a_nullable_position) {
  syn_test_bootstrap();
  SYN_ASSERT_MSG(syn_test_string_in_optional_is_nil(NULL),
                 "null at a nullable position did not map to nil");
  SYN_ASSERT_MSG(!syn_test_string_in_optional_is_nil("text"),
                 "a valid string converted to nil");
  SYN_ASSERT_MSG(!syn_test_string_in_is_nil("text"),
                 "a valid string converted to nil at a non-null position");
}

/* ---- availability (R8, KTD5, AE4) ---- */

SYN_TEST(ae4_a_version_above_the_running_one_makes_the_app_skip_the_call) {
  syn_test_bootstrap();

  bool entered_guarded_call = false;
  if (ns_available(99, 0)) entered_guarded_call = true;
  SYN_ASSERT_MSG(!entered_guarded_call,
                 "ns_available(99, 0) let the app into a guarded call");

  SYN_ASSERT_MSG(ns_available(26, 0),
                 "the deployment floor is macOS 26 but ns_available(26, 0) is "
                 "false");
  SYN_ASSERT_MSG(ns_available(26, 99),
                 "a newer major version did not satisfy an older major with a "
                 "high minor");
}

SYN_TEST(ns_available_agrees_with_the_compilers_own_runtime_check) {
  syn_test_bootstrap();

  /* __builtin_available is the idiom a C source guards with; ns_available is
   * the same answer for a caller with no compile-time construct to reach for.
   * They have to agree, and this is the independent oracle for that. */
  bool builtin_floor = false;
  if (__builtin_available(macOS 26.0, *)) builtin_floor = true;
  bool builtin_next = false;
  if (__builtin_available(macOS 27.0, *)) builtin_next = true;
  bool builtin_far = false;
  if (__builtin_available(macOS 99.0, *)) builtin_far = true;

  SYN_ASSERT_MSG(ns_available(26, 0) == builtin_floor,
                 "ns_available(26, 0) is %d, __builtin_available says %d",
                 ns_available(26, 0), builtin_floor);
  SYN_ASSERT_MSG(ns_available(27, 0) == builtin_next,
                 "ns_available(27, 0) is %d, __builtin_available says %d",
                 ns_available(27, 0), builtin_next);
  SYN_ASSERT_MSG(ns_available(99, 0) == builtin_far,
                 "ns_available(99, 0) is %d, __builtin_available says %d",
                 ns_available(99, 0), builtin_far);
}

/* ---- main-thread dispatch (R10) ---- */

static int syn_dispatch_calls;
static void *syn_dispatch_context;
static const void *syn_dispatch_sender;
static bool syn_dispatch_on_main;

static void syn_dispatch_record(void *context, const void *sender) {
  syn_dispatch_calls++;
  syn_dispatch_context = context;
  syn_dispatch_sender = sender;
  syn_dispatch_on_main = syn_test_is_main_thread();
}

static void syn_dispatch_reset(void) {
  syn_dispatch_calls = 0;
  syn_dispatch_context = NULL;
  syn_dispatch_sender = (const void *)&syn_dispatch_calls; /* not NULL */
  syn_dispatch_on_main = false;
}

static void *syn_dispatch_from_background(void *context) {
  ns_main_thread_dispatch(syn_dispatch_record, context);
  return NULL;
}

SYN_TEST(dispatch_from_the_main_thread_runs_later_and_on_the_main_thread) {
  syn_test_bootstrap();
  syn_dispatch_reset();
  int marker = 41;

  ns_main_thread_dispatch(syn_dispatch_record, &marker);
  SYN_ASSERT_MSG(syn_dispatch_calls == 0,
                 "the callback ran before ns_main_thread_dispatch returned");

  SYN_WAIT_FOR(syn_dispatch_calls == 1, 5000.0);
  SYN_ASSERT_MSG(syn_dispatch_context == &marker,
                 "the callback got a different context");
  SYN_ASSERT_MSG(syn_dispatch_sender == NULL,
                 "a dispatch has no sender, so it should be null");
  SYN_ASSERT_MSG(syn_dispatch_on_main, "the callback did not run on the main "
                                       "thread");
}

SYN_TEST(dispatch_from_a_background_thread_runs_on_the_main_thread) {
  syn_test_bootstrap();
  syn_dispatch_reset();
  int marker = 42;

  pthread_t thread;
  SYN_ASSERT(pthread_create(&thread, NULL, syn_dispatch_from_background,
                            &marker) == 0);
  SYN_ASSERT(pthread_join(thread, NULL) == 0);

  /* The background thread has already returned from the call and exited, and
   * the main run loop has not turned yet, so nothing can have run. */
  SYN_ASSERT_MSG(syn_dispatch_calls == 0,
                 "the callback ran before the main run loop turned");

  SYN_WAIT_FOR(syn_dispatch_calls == 1, 5000.0);
  SYN_ASSERT_MSG(syn_dispatch_context == &marker,
                 "the callback got a different context");
  SYN_ASSERT_MSG(syn_dispatch_on_main,
                 "a callback dispatched from a background thread did not run "
                 "on the main thread");
}

/* ---- the class check passes for a subclass (R12) ---- */

SYN_TEST(a_subclass_handle_passes_the_class_check) {
  syn_test_bootstrap();
  const void *button = syn_test_button_create();
  SYN_ASSERT_STR_EQ(syn_test_class_name(button), "NSButton");

  syn_test_expect_view(button); /* NSButton is an NSView: no report, no abort */
  syn_test_expect_view_optional(button);
  syn_test_expect_view_optional(NULL); /* null passes at a nullable position */

  ns_release(button);
}

/* ---- the cases that abort, each run in a child (KTD4, R12, AE3) ---- */

static void *syn_retain_off_the_main_thread(void *handle) {
  ns_retain(handle); /* main-thread only (R10): this is the violation */
  return NULL;
}

SYN_ABORT_CASE(off_the_main_thread) {
  syn_test_bootstrap();
  const void *view = syn_test_view_create();
  pthread_t thread;
  if (pthread_create(&thread, NULL, syn_retain_off_the_main_thread,
                     (void *)view) != 0) {
    fprintf(stderr, "could not start the background thread\n");
    return;
  }
  pthread_join(thread, NULL);
}

SYN_ABORT_CASE(null_handle_at_a_non_null_position) {
  syn_test_bootstrap();
  syn_test_expect_view(NULL);
}

SYN_ABORT_CASE(handle_of_an_unrelated_class) {
  syn_test_bootstrap();
  syn_test_expect_view(syn_test_date_create());
}

SYN_ABORT_CASE(null_string_at_a_non_null_position) {
  syn_test_bootstrap();
  syn_test_string_in_is_nil(NULL);
}

SYN_ABORT_CASE(invalid_utf8_string) {
  syn_test_bootstrap();
  syn_test_string_in_is_nil("\xff\xfe not utf-8");
}

SYN_ABORT_CASE(appkit_raises_inside_a_wrapper) {
  syn_test_wrapper_raises();
}

SYN_TEST(ae3_a_call_off_the_main_thread_names_the_function_and_the_rule) {
  SYN_ASSERT_ABORTS("off_the_main_thread", "ns_retain");
  SYN_ASSERT_ABORTS("off_the_main_thread", "main thread only");
}

SYN_TEST(a_null_handle_at_a_non_null_position_names_the_class_and_function) {
  SYN_ASSERT_ABORTS("null_handle_at_a_non_null_position", "NSView");
  SYN_ASSERT_ABORTS("null_handle_at_a_non_null_position", "syn_test_expect_view");
}

SYN_TEST(a_handle_of_an_unrelated_class_names_the_class_and_function) {
  SYN_ASSERT_ABORTS("handle_of_an_unrelated_class", "expected a handle to NSView");
  SYN_ASSERT_ABORTS("handle_of_an_unrelated_class", "syn_test_expect_view");
}

SYN_TEST(a_null_string_at_a_non_null_position_is_reported) {
  SYN_ASSERT_ABORTS("null_string_at_a_non_null_position",
                    "null at a non-null parameter");
}

SYN_TEST(an_invalid_utf8_string_is_reported_in_a_debug_build) {
  SYN_ASSERT_ABORTS("invalid_utf8_string", "not valid UTF-8");
}

SYN_TEST(an_appkit_exception_inside_a_wrapper_names_it_and_the_function) {
  SYN_ASSERT_ABORTS("appkit_raises_inside_a_wrapper", "NSInvalidArgumentException");
  SYN_ASSERT_ABORTS("appkit_raises_inside_a_wrapper", "syn_test_wrapper_raises");
}

/* ---- release builds pay nothing for any of it (R12) ---- */

/* The repo root, from this file's own compiled-in path. */
static bool syn_source_root(char *out, size_t size) {
  const char *file = __FILE__;
  const char *tail = strstr(file, "/tests/test_base.c");
  if (file[0] != '/' || tail == NULL) return false;
  size_t length = (size_t)(tail - file);
  if (length + 1 > size) return false;
  memcpy(out, file, length);
  out[length] = '\0';
  return true;
}

SYN_TEST(an_ndebug_build_carries_none_of_the_check_text) {
  char root[PATH_MAX];
  SYN_ASSERT_MSG(syn_source_root(root, sizeof root),
                 "could not find the source tree from %s", __FILE__);

  /* Compiling under NDEBUG is half the check - it proves the release paths of
   * every macro still build - and grepping the object is the other half. */
  char command[2048];
  int written = snprintf(
      command, sizeof command,
      "set -e; "
      "dir=$(mktemp -d); object=\"$dir/ns_base.o\"; "
      "clang -x objective-c -std=c23 -fobjc-arc -DNDEBUG "
      "-mmacosx-version-min=26.0 -Wall -Wextra -Werror "
      "-I'%s/include' -c '%s/src/ns_base.m' -o \"$object\"; "
      "hits=$(strings \"$object\" | grep -c -e 'syntonic:' -e 'main thread' "
      "-e 'R12' || true); "
      "rm -f \"$object\"; rmdir \"$dir\"; "
      "test \"$hits\" -eq 0",
      root, root);
  SYN_ASSERT(written > 0 && (size_t)written < sizeof command);

  int status = system(command);
  SYN_ASSERT_MSG(status == 0,
                 "an NDEBUG ns_base.o either failed to build or still carries "
                 "the debug check text (shell status %d)",
                 status);
}

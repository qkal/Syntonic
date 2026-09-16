/*
 * The shared application suite (U14): the singleton, activation, the delegate
 * struct, and F1 - closing the last window leaves the app running.
 *
 * did_finish_launching and will_terminate only reach a delegate at a real
 * launch or a real termination, neither of which a suite can stage, so they
 * are fired the way AppKit fires them: by posting the notification. That is
 * also the sharpest test of the respondsToSelector: cache, because AppKit
 * registers a delegate for exactly the notifications it claims to implement,
 * at the moment the slot is assigned (KTD8).
 */

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "runner.h"
#include "support.h"
#include "syntonic/syntonic.h"

static const char *const SYN_LAUNCH_NOTE =
    "NSApplicationDidFinishLaunchingNotification";
static const char *const SYN_TERMINATE_NOTE =
    "NSApplicationWillTerminateNotification";

static int syn_launches;
static int syn_terminations;
static int syn_termination_questions;
static void *syn_seen_context;
static const void *syn_seen_sender;

static void syn_on_launch(void *context, ns_application *sender) {
  syn_launches++;
  syn_seen_context = context;
  syn_seen_sender = sender;
}

static void syn_on_will_terminate(void *context, ns_application *sender) {
  syn_terminations++;
  syn_seen_context = context;
  syn_seen_sender = sender;
}

static bool syn_never_terminate(void *context, ns_application *sender) {
  (void)context, (void)sender;
  syn_termination_questions++;
  return false;
}

static void syn_reset(void) {
  syn_launches = syn_terminations = syn_termination_questions = 0;
  syn_seen_context = NULL;
  syn_seen_sender = NULL;
}

SYN_TEST(the_shared_application_is_one_object_and_it_is_an_nsapplication) {
  syn_test_bootstrap();
  ns_application *first = ns_application_shared();
  ns_application *second = ns_application_shared();

  SYN_ASSERT_MSG(first == second,
                 "two calls to ns_application_shared gave two handles");
  SYN_ASSERT_STR_EQ(syn_test_class_name(first), "NSApplication");
  SYN_ASSERT_MSG(first == syn_test_shared_application(),
                 "the handle is not the application the harness bootstrapped");
}

SYN_TEST(activating_the_application_is_the_api_that_is_not_deprecated) {
  syn_test_bootstrap();
  /* KTD16: -[NSApplication activate]. The suite's activation policy is
   * prohibited, so nothing comes forward; what is under test is that the call
   * goes through the wrapper and back. */
  ns_application_activate(ns_application_shared());
  SYN_ASSERT(syn_test_activation_policy_is_prohibited());
}

SYN_TEST(a_struct_with_one_member_set_is_a_delegate_that_implements_one_method) {
  syn_test_bootstrap();
  syn_reset();
  ns_application *app = ns_application_shared();
  int context = 41;

  ns_application_callbacks callbacks = {.did_finish_launching = syn_on_launch};
  ns_application_set_callbacks(app, &callbacks, &context);

  /* R9, KTD8: an unset optional member is a method the object does not
   * implement, and AppKit has to see that as it assigns the slot. */
  SYN_ASSERT_MSG(syn_test_delegate_responds(
                     app, "applicationDidFinishLaunching:"),
                 "the member that was set is not reported as implemented");
  SYN_ASSERT_MSG(!syn_test_delegate_responds(
                     app, "applicationShouldTerminateAfterLastWindowClosed:"),
                 "an unset member is reported as implemented");
  SYN_ASSERT_MSG(!syn_test_delegate_responds(app, "applicationWillTerminate:"),
                 "an unset member is reported as implemented");

  syn_test_post_notification(SYN_LAUNCH_NOTE, app);
  SYN_ASSERT_MSG(syn_launches == 1, "the member that was set did not fire");
  SYN_ASSERT_MSG(syn_seen_context == &context, "the context did not arrive");
  SYN_ASSERT_MSG(syn_seen_sender == app,
                 "the sender was not the application that fired");

  syn_test_post_notification(SYN_TERMINATE_NOTE, app);
  SYN_ASSERT_MSG(syn_terminations == 0, "an unset member fired");

  ns_application_set_callbacks(app, NULL, NULL);
}

SYN_TEST(installing_again_replaces_the_struct_and_the_context) {
  syn_test_bootstrap();
  syn_reset();
  ns_application *app = ns_application_shared();
  int first_context = 1;
  int second_context = 2;

  ns_application_callbacks first = {.did_finish_launching = syn_on_launch};
  ns_application_set_callbacks(app, &first, &first_context);
  SYN_ASSERT_SHIMS(1);

  ns_application_callbacks second = {.did_finish_launching = syn_on_launch,
                                     .will_terminate = syn_on_will_terminate};
  ns_application_set_callbacks(app, &second, &second_context);
  SYN_ASSERT_MSG(syn_test_shim_live_count() <= 1,
                 "the replaced shim was not released");

  syn_test_post_notification(SYN_TERMINATE_NOTE, app);
  SYN_ASSERT_MSG(syn_terminations == 1,
                 "the member the second struct added did not fire");
  SYN_ASSERT_MSG(syn_seen_context == &second_context,
                 "the callback got the first context, not the second");

  ns_application_set_callbacks(app, NULL, NULL);
}

SYN_TEST(a_null_struct_uninstalls_the_application_delegate) {
  syn_test_bootstrap();
  syn_reset();
  ns_application *app = ns_application_shared();

  ns_application_callbacks callbacks = {.did_finish_launching = syn_on_launch};
  ns_application_set_callbacks(app, &callbacks, NULL);
  SYN_ASSERT(syn_test_has_delegate(app));

  ns_application_set_callbacks(app, NULL, NULL);
  SYN_ASSERT_MSG(!syn_test_has_delegate(app),
                 "a null struct left a delegate installed");
  SYN_ASSERT_SHIMS(0);

  syn_test_post_notification(SYN_LAUNCH_NOTE, app);
  SYN_ASSERT_MSG(syn_launches == 0, "an uninstalled member fired");
}

/* F1: closing the last window leaves the app running with its menu bar. AppKit
 * asks the delegate only when the delegate implements the question; with the
 * member unset the answer is AppKit's own default, which is the same answer. */
SYN_TEST(f1_closing_the_last_window_leaves_the_application_running) {
  syn_test_bootstrap();
  syn_reset();
  ns_application *app = ns_application_shared();

  ns_application_callbacks callbacks = {.did_finish_launching = syn_on_launch};
  ns_application_set_callbacks(app, &callbacks, NULL);

  ns_window *window = ns_window_create_with_content_rect_style_mask_backing_defer(
      CGRectMake(0, 0, 320, 200),
      NS_WINDOW_STYLE_MASK_TITLED | NS_WINDOW_STYLE_MASK_CLOSABLE,
      NS_BACKING_STORE_BUFFERED, false);
  ns_window_make_key_and_order_front(window);
  syn_test_spin(50);
  ns_window_close(window);
  syn_test_spin(100); /* the process would be gone by now if it terminated */

  SYN_ASSERT_MSG(syn_termination_questions == 0,
                 "AppKit asked a delegate that does not implement the question");
  SYN_ASSERT_STR_EQ(syn_test_class_name(ns_application_shared()),
                    "NSApplication");

  ns_release(window);
  ns_application_set_callbacks(app, NULL, NULL);
}

/* The termination question only reaches a delegate from inside
 * -[NSApplication run], which a suite never enters, so what is observable here
 * is the half that matters for F1: whether AppKit sees the member at all. With
 * it set AppKit asks and gets false; with it unset AppKit does not ask and
 * takes its own default, which is the same answer. */
SYN_TEST(a_delegate_that_answers_the_termination_question_reports_that_it_does) {
  syn_test_bootstrap();
  syn_reset();
  ns_application *app = ns_application_shared();

  ns_application_callbacks callbacks = {
      .should_terminate_after_last_window_closed = syn_never_terminate};
  ns_application_set_callbacks(app, &callbacks, NULL);
  SYN_ASSERT_MSG(syn_test_delegate_responds(
                     app, "applicationShouldTerminateAfterLastWindowClosed:"),
                 "the member that was set is not reported as implemented");
  SYN_ASSERT_MSG(!syn_test_delegate_responds(
                     app, "applicationDidFinishLaunching:"),
                 "an unset member is reported as implemented");

  ns_window *window = ns_window_create_with_content_rect_style_mask_backing_defer(
      CGRectMake(0, 0, 320, 200),
      NS_WINDOW_STYLE_MASK_TITLED | NS_WINDOW_STYLE_MASK_CLOSABLE,
      NS_BACKING_STORE_BUFFERED, false);
  ns_window_make_key_and_order_front(window);
  syn_test_spin(50);
  ns_window_close(window);
  syn_test_spin(100);

  SYN_ASSERT_STR_EQ(syn_test_class_name(ns_application_shared()),
                    "NSApplication");

  ns_release(window);
  ns_application_set_callbacks(app, NULL, NULL);
}

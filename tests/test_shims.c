/*
 * The callback machinery suite (U14): the target/action trampoline, the
 * required-member report, the shim's own lifetime and the debug validation of
 * what a callback returns (R9, KTD8).
 *
 * The protocols U14 wraps have no required member and no member that returns
 * anything, so the two halves the six later units depend on are driven through
 * the stand-in protocol in tests/support.m.
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "runner.h"
#include "support.h"
#include "syntonic/syntonic.h"

/* ---- the target/action trampoline (R9) ---- */

static int syn_first_calls;
static int syn_second_calls;
static void *syn_seen_context;
static const void *syn_seen_sender;

static void syn_first_action(void *context, const void *sender) {
  syn_first_calls++;
  syn_seen_context = context;
  syn_seen_sender = sender;
}

static void syn_second_action(void *context, const void *sender) {
  syn_second_calls++;
  syn_seen_context = context;
  syn_seen_sender = sender;
}

SYN_TEST(setting_an_action_twice_fires_only_the_second_with_its_context) {
  syn_test_bootstrap();
  syn_first_calls = syn_second_calls = 0;
  int first_context = 1;
  int second_context = 2;

  const void *button = syn_test_button_create();
  syn_test_install_action(button, syn_first_action, &first_context);
  syn_test_install_action(button, syn_second_action, &second_context);
  syn_test_fire_action(button);

  SYN_ASSERT_MSG(syn_first_calls == 0, "the replaced action still fired");
  SYN_ASSERT_MSG(syn_second_calls == 1,
                 "the installed action fired %d times, not once",
                 syn_second_calls);
  SYN_ASSERT_MSG(syn_seen_context == &second_context,
                 "the callback got the first context, not the second");
  SYN_ASSERT_MSG(syn_seen_sender == button,
                 "the callback's sender was not the object that fired");

  syn_test_install_action(button, NULL, NULL);
  ns_release(button);
}

SYN_TEST(a_null_action_uninstalls_the_trampoline) {
  syn_test_bootstrap();
  syn_first_calls = 0;

  const void *button = syn_test_button_create();
  syn_test_install_action(button, syn_first_action, NULL);
  syn_test_fire_action(button);
  SYN_ASSERT_MSG(syn_first_calls == 1, "the installed action did not fire");

  syn_test_install_action(button, NULL, NULL);
  syn_test_fire_action(button);
  SYN_ASSERT_MSG(syn_first_calls == 1, "an uninstalled action fired");

  ns_release(button);
}

/* KTD8: a callback may replace the thing that is calling it. Uninstalling from
 * inside the action releases the trampoline whose method is on the stack, so
 * the trampoline holding itself for the call is what makes the next two lines
 * legal rather than a use after free. */
static void syn_uninstalling_action(void *context, const void *sender) {
  syn_first_calls++;
  syn_test_install_action(sender, NULL, NULL);
  syn_seen_context = context;
  syn_seen_sender = sender;
}

SYN_TEST(an_action_that_uninstalls_itself_finishes_the_call_it_is_in) {
  syn_test_bootstrap();
  syn_first_calls = 0;
  int context = 7;

  const void *button = syn_test_button_create();
  syn_test_install_action(button, syn_uninstalling_action, &context);
  syn_test_fire_action(button);

  SYN_ASSERT_MSG(syn_first_calls == 1, "the action did not fire once");
  SYN_ASSERT_MSG(syn_seen_context == &context,
                 "the context was unreadable after the trampoline was replaced");

  syn_test_fire_action(button); /* it uninstalled itself, so nothing happens */
  SYN_ASSERT_MSG(syn_first_calls == 1, "the uninstalled action fired again");

  ns_release(button);
}

/* ---- required members, from the stand-in protocol (R9, KTD8) ---- */

SYN_TEST(a_struct_that_sets_every_required_member_installs_and_is_released_with_its_object) {
  syn_test_bootstrap();
  SYN_ASSERT_SHIMS(0);

  const void *view = syn_test_view_create();
  syn_test_install_required_member_struct(view, true);
  SYN_ASSERT_SHIMS(1);

  /* Installing again replaces: still one shim, not two. */
  syn_test_install_required_member_struct(view, true);
  SYN_ASSERT_SHIMS(1);

  ns_release(view); /* the object goes, and the association with it */
  SYN_ASSERT_SHIMS(0);
}

SYN_ABORT_CASE(required_member_missing) {
  syn_test_bootstrap();
  const void *view = syn_test_view_create();
  syn_test_install_required_member_struct(view, false);
}

SYN_TEST(installing_a_struct_missing_a_required_member_names_the_member_and_the_protocol) {
  SYN_ASSERT_ABORTS("required_member_missing", "NSTestDelegate");
  SYN_ASSERT_ABORTS("required_member_missing", "number_of_rows");
}

/* ---- debug validation of what a callback returns (KTD8) ---- */

SYN_TEST(the_return_validation_passes_a_count_and_a_handle_that_are_in_order) {
  syn_test_bootstrap();
  syn_test_shim_check_count(0);
  syn_test_shim_check_count(3);

  const void *view = syn_test_view_create();
  syn_test_shim_check_returned_handle(view);
  ns_release(view);

  /* A subclass of the expected class passes too (R12). */
  const void *button = syn_test_button_create();
  syn_test_shim_check_returned_handle(button);
  ns_release(button);
}

SYN_ABORT_CASE(a_callback_returns_a_negative_count) {
  syn_test_bootstrap();
  syn_test_shim_check_count(-1);
}

SYN_ABORT_CASE(a_callback_returns_null_where_the_sdk_says_non_null) {
  syn_test_bootstrap();
  syn_test_shim_check_returned_handle(NULL);
}

SYN_ABORT_CASE(a_callback_returns_a_handle_of_the_wrong_class) {
  syn_test_bootstrap();
  syn_test_shim_check_returned_handle(syn_test_date_create());
}

SYN_TEST(a_negative_count_from_a_callback_names_the_protocol_and_the_member) {
  SYN_ASSERT_ABORTS("a_callback_returns_a_negative_count", "NSTestDelegate");
  SYN_ASSERT_ABORTS("a_callback_returns_a_negative_count", "number_of_rows");
  SYN_ASSERT_ABORTS("a_callback_returns_a_negative_count", "-1");
}

SYN_TEST(a_null_handle_from_a_callback_names_the_class_the_sdk_declares) {
  SYN_ASSERT_ABORTS("a_callback_returns_null_where_the_sdk_says_non_null",
                    "cell_view");
  SYN_ASSERT_ABORTS("a_callback_returns_null_where_the_sdk_says_non_null",
                    "NSView");
}

SYN_TEST(a_handle_of_the_wrong_class_from_a_callback_names_both_classes) {
  SYN_ASSERT_ABORTS("a_callback_returns_a_handle_of_the_wrong_class",
                    "NSTestDelegate");
  SYN_ASSERT_ABORTS("a_callback_returns_a_handle_of_the_wrong_class",
                    "NSView");
}

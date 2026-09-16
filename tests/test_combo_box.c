/*
 * The combo box suite (U13): the static list, the data source, the merged
 * callbacks struct with the text field's members embedded in it, the two mode
 * checks and the index check - off-screen, the way R21 says every wrapper's
 * suite runs (R7, R9, R11, R12, KTD6, KTD8, KTD17).
 *
 * The data source slot is `assign`, so two of the tests below are about the
 * slot itself rather than about the callbacks: uninstalling leaves it nil, and
 * releasing the combo box with a data source installed needs no uninstall
 * first.
 */

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "runner.h"
#include "support.h"
#include "syntonic/syntonic.h"

/* AppKit's own spelling of the notifications a combo box and a field editor
 * post. Posting one is how an off-screen suite fires a member that a pointer
 * on the pop-up list would otherwise have to (R21). */
#define SYN_WILL_POP_UP "NSComboBoxWillPopUpNotification"
#define SYN_WILL_DISMISS "NSComboBoxWillDismissNotification"
#define SYN_SELECTION_IS_CHANGING "NSComboBoxSelectionIsChangingNotification"
#define SYN_DID_CHANGE "NSControlTextDidChangeNotification"
#define SYN_DID_END_EDITING "NSControlTextDidEndEditingNotification"

/* The model the data source answers from. */
static const char *const syn_values[] = {"Kernel", "Docs", "Tooling"};
enum { SYN_VALUE_COUNT = 3 };

static int syn_counts, syn_values_asked, syn_index_asked, syn_completions;
static int syn_did_change, syn_is_changing, syn_pop_ups, syn_dismissals;
static int syn_edits, syn_ends;
static void *syn_context_seen;
static const void *syn_sender_seen;
static long syn_selected_seen;
static long syn_value_index_seen;

static void syn_reset(void) {
  syn_counts = syn_values_asked = syn_index_asked = syn_completions = 0;
  syn_did_change = syn_is_changing = syn_pop_ups = syn_dismissals = 0;
  syn_edits = syn_ends = 0;
  syn_context_seen = NULL;
  syn_sender_seen = NULL;
  syn_selected_seen = -2;
  syn_value_index_seen = -2;
}

static long syn_number_of_items(void *context, ns_combo_box *sender) {
  (void)sender;
  syn_counts++;
  syn_context_seen = context;
  return SYN_VALUE_COUNT;
}

static const char *syn_object_value(void *context, ns_combo_box *sender,
                                    long index) {
  (void)context;
  (void)sender;
  syn_values_asked++;
  syn_value_index_seen = index;
  if (index < 0 || index >= SYN_VALUE_COUNT) return NULL;
  return syn_values[index];
}

/* -1 is the boundary's "no such value", which the wrapper turns into AppKit's
 * NSNotFound (R11). */
static long syn_index_of_value(void *context, ns_combo_box *sender,
                               const char *value) {
  (void)context;
  (void)sender;
  syn_index_asked++;
  for (long index = 0; index < SYN_VALUE_COUNT; index++)
    if (strcmp(value, syn_values[index]) == 0) return index;
  return -1;
}

static const char *syn_completed(void *context, ns_combo_box *sender,
                                 const char *prefix) {
  (void)context;
  (void)sender;
  syn_completions++;
  for (long index = 0; index < SYN_VALUE_COUNT; index++)
    if (strncmp(prefix, syn_values[index], strlen(prefix)) == 0)
      return syn_values[index];
  return NULL;
}

static void syn_on_selection_did_change(void *context, ns_combo_box *sender) {
  syn_did_change++;
  syn_context_seen = context;
  syn_sender_seen = sender;
  syn_selected_seen = ns_combo_box_index_of_selected_item(sender);
}

static void syn_on_selection_is_changing(void *context, ns_combo_box *sender) {
  (void)context;
  syn_is_changing++;
  syn_sender_seen = sender;
}

static void syn_on_will_pop_up(void *context, ns_combo_box *sender) {
  (void)context;
  syn_pop_ups++;
  syn_sender_seen = sender;
}

static void syn_on_will_dismiss(void *context, ns_combo_box *sender) {
  (void)context;
  syn_dismissals++;
  syn_sender_seen = sender;
}

/* The embedded parent struct's two members: their sender is the combo box
 * behind an ns_text_field handle (R5). */
static void syn_on_did_change(void *context, ns_text_field *sender) {
  (void)context;
  syn_edits++;
  syn_sender_seen = sender;
}

static void syn_on_did_end_editing(void *context, ns_text_field *sender) {
  (void)context;
  syn_ends++;
  syn_sender_seen = sender;
}

/* Every member set, which is what most of the tests below install. */
static ns_combo_box_callbacks syn_all_members(void) {
  ns_combo_box_callbacks callbacks = {
      .text_field = {.did_change = syn_on_did_change,
                     .did_end_editing = syn_on_did_end_editing},
      .number_of_items = syn_number_of_items,
      .object_value_for_item_at_index = syn_object_value,
      .index_of_item_with_string_value = syn_index_of_value,
      .completed_string = syn_completed,
      .selection_did_change = syn_on_selection_did_change,
      .selection_is_changing = syn_on_selection_is_changing,
      .will_pop_up = syn_on_will_pop_up,
      .will_dismiss = syn_on_will_dismiss,
  };
  return callbacks;
}

/* A combo box driven by the struct: the mode first, then the install, which is
 * the order AppKit's cell needs (R12). */
static ns_combo_box *syn_data_source_combo_box(void *context) {
  ns_combo_box *combo_box =
      ns_combo_box_create_with_frame(CGRectMake(0, 0, 160, 24));
  ns_combo_box_set_uses_data_source(combo_box, true);
  ns_combo_box_callbacks callbacks = syn_all_members();
  ns_combo_box_set_callbacks(combo_box, &callbacks, context);
  ns_combo_box_reload_data(combo_box);
  return combo_box;
}

/* ---- the static list (R11, KTD17) ---- */

SYN_TEST(a_static_combo_boxs_list_round_trips) {
  syn_test_bootstrap();
  ns_combo_box *combo_box =
      ns_combo_box_create_with_frame(CGRectMake(0, 0, 160, 24));
  SYN_ASSERT_STR_EQ(syn_test_class_name(combo_box), "NSComboBox");

  /* A new combo box is the static one: AppKit's own default. */
  SYN_ASSERT_MSG(!ns_combo_box_uses_data_source(combo_box),
                 "a new combo box started on the data source");
  SYN_ASSERT(ns_combo_box_number_of_items(combo_box) == 0);

  ns_combo_box_add_items_with_object_values(combo_box, syn_values,
                                            SYN_VALUE_COUNT);
  ns_combo_box_add_item_with_object_value(combo_box, "Design");
  SYN_ASSERT_MSG(ns_combo_box_number_of_items(combo_box) == 4,
                 "the list has %ld values, not 4",
                 ns_combo_box_number_of_items(combo_box));

  /* An owned return, twice, so a freed copy leaves the other one alone (R7). */
  char *first = ns_combo_box_copy_item_object_value_at_index(combo_box, 0);
  char *second = ns_combo_box_copy_item_object_value_at_index(combo_box, 0);
  SYN_ASSERT_STR_EQ(first, "Kernel");
  SYN_ASSERT_MSG(first != second, "two copies of a value came back as one");
  ns_string_free(first);
  SYN_ASSERT_STR_EQ(second, "Kernel");
  ns_string_free(second);

  ns_combo_box_insert_item_with_object_value_at_index(combo_box, "Kernel v2",
                                                      1);
  char *inserted = ns_combo_box_copy_item_object_value_at_index(combo_box, 1);
  SYN_ASSERT_STR_EQ(inserted, "Kernel v2");
  ns_string_free(inserted);
  SYN_ASSERT(ns_combo_box_number_of_items(combo_box) == 5);

  SYN_ASSERT_MSG(
      ns_combo_box_index_of_item_with_object_value(combo_box, "Tooling") == 3,
      "Tooling came back at index %ld",
      ns_combo_box_index_of_item_with_object_value(combo_box, "Tooling"));
  /* AppKit's NSNotFound crosses as -1, the way every index does (R11). */
  SYN_ASSERT_MSG(
      ns_combo_box_index_of_item_with_object_value(combo_box, "Missing") == -1,
      "a value the list does not carry did not come back as -1");

  ns_combo_box_remove_item_at_index(combo_box, 1);
  SYN_ASSERT(ns_combo_box_number_of_items(combo_box) == 4);
  ns_combo_box_remove_item_with_object_value(combo_box, "Design");
  SYN_ASSERT(ns_combo_box_number_of_items(combo_box) == 3);
  /* A value the list does not carry removes nothing, and is not misuse. */
  ns_combo_box_remove_item_with_object_value(combo_box, "Missing");
  SYN_ASSERT(ns_combo_box_number_of_items(combo_box) == 3);

  ns_combo_box_remove_all_items(combo_box);
  SYN_ASSERT(ns_combo_box_number_of_items(combo_box) == 0);

  /* A count of zero adds nothing and never reads the pointer (KTD17). */
  ns_combo_box_add_items_with_object_values(combo_box, NULL, 0);
  SYN_ASSERT(ns_combo_box_number_of_items(combo_box) == 0);

  ns_release(combo_box);
}

SYN_TEST(a_static_combo_boxs_selection_round_trips) {
  syn_test_bootstrap();
  ns_combo_box *combo_box =
      ns_combo_box_create_with_frame(CGRectMake(0, 0, 160, 24));
  ns_combo_box_add_items_with_object_values(combo_box, syn_values,
                                            SYN_VALUE_COUNT);

  SYN_ASSERT_MSG(ns_combo_box_index_of_selected_item(combo_box) == -1,
                 "a new combo box started with a selection");
  SYN_ASSERT_MSG(
      ns_combo_box_copy_object_value_of_selected_item(combo_box) == NULL,
      "a combo box with no selection reported a value");

  ns_combo_box_select_item_at_index(combo_box, 2);
  SYN_ASSERT(ns_combo_box_index_of_selected_item(combo_box) == 2);
  char *selected = ns_combo_box_copy_object_value_of_selected_item(combo_box);
  SYN_ASSERT_STR_EQ(selected, "Tooling");
  ns_string_free(selected);

  /* Selecting sets the field's text too, which is NSComboBox's own behaviour
   * and reads back through the control upcast. */
  char *text = ns_control_copy_string_value(ns_combo_box_as_control(combo_box));
  SYN_ASSERT_STR_EQ(text, "Tooling");
  ns_string_free(text);

  ns_combo_box_select_item_with_object_value(combo_box, "Docs");
  SYN_ASSERT(ns_combo_box_index_of_selected_item(combo_box) == 1);
  /* A value the list does not carry selects nothing, and is not misuse. */
  ns_combo_box_select_item_with_object_value(combo_box, "Missing");
  SYN_ASSERT(ns_combo_box_index_of_selected_item(combo_box) == 1);
  ns_combo_box_select_item_with_object_value(combo_box, NULL);

  ns_combo_box_select_item_at_index(combo_box, 0);
  ns_combo_box_deselect_item_at_index(combo_box, 0);
  SYN_ASSERT_MSG(ns_combo_box_index_of_selected_item(combo_box) == -1,
                 "deselecting the selected index left it selected");

  /* -1 is AppKit's own "no selection", and the wrapper passes it through. */
  ns_combo_box_select_item_at_index(combo_box, 1);
  ns_combo_box_select_item_at_index(combo_box, -1);
  SYN_ASSERT(ns_combo_box_index_of_selected_item(combo_box) == -1);

  /* Scrolling an off-screen list is a no-op rather than a crash. */
  ns_combo_box_scroll_item_at_index_to_top(combo_box, 2);
  ns_combo_box_scroll_item_at_index_to_visible(combo_box, 0);

  ns_release(combo_box);
}

SYN_TEST(a_combo_boxs_list_settings_round_trip) {
  syn_test_bootstrap();
  ns_combo_box *combo_box =
      ns_combo_box_create_with_frame(CGRectMake(0, 0, 160, 24));

  SYN_ASSERT_MSG(ns_combo_box_has_vertical_scroller(combo_box),
                 "a new combo box started with no vertical scroller");
  ns_combo_box_set_has_vertical_scroller(combo_box, false);
  SYN_ASSERT(!ns_combo_box_has_vertical_scroller(combo_box));
  ns_combo_box_set_has_vertical_scroller(combo_box, true);
  SYN_ASSERT(ns_combo_box_has_vertical_scroller(combo_box));

  ns_combo_box_set_number_of_visible_items(combo_box, 8);
  SYN_ASSERT_MSG(ns_combo_box_number_of_visible_items(combo_box) == 8,
                 "the visible item count read back as %ld",
                 ns_combo_box_number_of_visible_items(combo_box));

  ns_combo_box_set_item_height(combo_box, 22);
  SYN_ASSERT(ns_combo_box_item_height(combo_box) == 22);

  ns_combo_box_set_intercell_spacing(combo_box, CGSizeMake(4, 2));
  CGSize spacing = ns_combo_box_intercell_spacing(combo_box);
  SYN_ASSERT_MSG(spacing.width == 4 && spacing.height == 2,
                 "the intercell spacing read back as %g x %g", spacing.width,
                 spacing.height);

  ns_combo_box_set_button_bordered(combo_box, false);
  SYN_ASSERT(!ns_combo_box_button_bordered(combo_box));
  ns_combo_box_set_button_bordered(combo_box, true);
  SYN_ASSERT(ns_combo_box_button_bordered(combo_box));

  SYN_ASSERT_MSG(!ns_combo_box_completes(combo_box),
                 "a new combo box started on completion");
  ns_combo_box_set_completes(combo_box, true);
  SYN_ASSERT(ns_combo_box_completes(combo_box));

  ns_combo_box_set_uses_data_source(combo_box, true);
  SYN_ASSERT(ns_combo_box_uses_data_source(combo_box));
  ns_combo_box_set_uses_data_source(combo_box, false);
  SYN_ASSERT(!ns_combo_box_uses_data_source(combo_box));

  ns_release(combo_box);
}

/* The three upcasts are borrowed returns (R7): the same object, which a retain
 * keeps past the handle the caller created it with. */
SYN_TEST(an_upcast_is_borrowed_and_outlives_the_handle_it_came_from) {
  syn_test_bootstrap();
  ns_combo_box *combo_box =
      ns_combo_box_create_with_frame(CGRectMake(0, 0, 160, 24));

  ns_text_field *field = ns_combo_box_as_text_field(combo_box);
  ns_control *control = ns_combo_box_as_control(combo_box);
  ns_view *view = ns_combo_box_as_view(combo_box);
  SYN_ASSERT_MSG((void *)field == (void *)combo_box &&
                     (void *)control == (void *)combo_box &&
                     (void *)view == (void *)combo_box,
                 "an upcast handed back a different pointer");

  /* The field's own half of the class, reached through the upcast. */
  ns_text_field_set_placeholder_string(field, "Category");
  char *placeholder = ns_text_field_copy_placeholder_string(field);
  SYN_ASSERT_STR_EQ(placeholder, "Category");
  ns_string_free(placeholder);

  /* A borrowed handle kept past its owner's reference (R7). */
  ns_retain(view);
  ns_release(combo_box);
  ns_view_set_accessibility_label(view, "Category");
  char *label = ns_view_copy_accessibility_label(view);
  SYN_ASSERT_STR_EQ(label, "Category");
  ns_string_free(label);
  ns_release(view);
}

/* ---- the data source and the delegate, merged (KTD6, KTD8) ---- */

SYN_TEST(a_data_source_answers_the_count_the_values_and_the_selection) {
  syn_test_bootstrap();
  syn_reset();
  int context = 13;

  ns_combo_box *combo_box = syn_data_source_combo_box(&context);
  SYN_ASSERT_SHIMS(1);
  SYN_ASSERT_MSG(syn_counts > 0, "the count member was never asked");
  SYN_ASSERT_MSG(syn_context_seen == &context, "the context did not arrive");
  SYN_ASSERT_MSG(ns_combo_box_number_of_items(combo_box) == SYN_VALUE_COUNT,
                 "the list has %ld values, not %d",
                 ns_combo_box_number_of_items(combo_box), SYN_VALUE_COUNT);

  /* Selecting is what asks the value member, and what makes AppKit post the
   * selection notification the delegate half answers. */
  ns_combo_box_select_item_at_index(combo_box, 1);
  SYN_WAIT_FOR(syn_did_change == 1, 1000);
  SYN_ASSERT_MSG(syn_values_asked > 0, "the value member was never asked");
  SYN_ASSERT_MSG(syn_value_index_seen == 1, "the value member was asked for %ld",
                 syn_value_index_seen);
  SYN_ASSERT_MSG(syn_sender_seen == combo_box, "the sender was not the box");
  SYN_ASSERT_MSG(syn_selected_seen == 1, "selection_did_change saw index %ld",
                 syn_selected_seen);
  char *text = ns_control_copy_string_value(ns_combo_box_as_control(combo_box));
  SYN_ASSERT_STR_EQ(text, "Docs");
  ns_string_free(text);

  /* The count alone, for a list that grew or shrank (KTD6). */
  syn_counts = 0;
  ns_combo_box_note_number_of_items_changed(combo_box);
  SYN_ASSERT_MSG(syn_counts > 0, "noteNumberOfItemsChanged asked nothing");

  ns_release(combo_box);
  SYN_ASSERT_SHIMS(0);
}

SYN_TEST(every_optional_member_fires_and_carries_the_combo_box) {
  syn_test_bootstrap();
  syn_reset();
  int context = 7;

  ns_combo_box *combo_box = syn_data_source_combo_box(&context);
  ns_combo_box_set_completes(combo_box, true);

  /* The three notification members, posted the way AppKit posts them. */
  syn_test_post_notification(SYN_WILL_POP_UP, combo_box);
  SYN_WAIT_FOR(syn_pop_ups == 1, 1000);
  SYN_ASSERT_MSG(syn_sender_seen == combo_box, "will_pop_up got another sender");
  syn_test_post_notification(SYN_SELECTION_IS_CHANGING, combo_box);
  SYN_WAIT_FOR(syn_is_changing == 1, 1000);
  syn_test_post_notification(SYN_WILL_DISMISS, combo_box);
  SYN_WAIT_FOR(syn_dismissals == 1, 1000);

  /* The two members of the embedded text field struct (R5). */
  syn_test_post_notification(SYN_DID_CHANGE, combo_box);
  SYN_WAIT_FOR(syn_edits == 1, 1000);
  syn_test_post_notification(SYN_DID_END_EDITING, combo_box);
  SYN_WAIT_FOR(syn_ends == 1, 1000);
  SYN_ASSERT_MSG(syn_sender_seen == (const void *)combo_box,
                 "an editing member got a sender that is not the combo box");

  /* Completion goes through AppKit's own path, which asks the member. */
  char *completed = syn_test_combo_box_completed_string(combo_box, "Ker");
  SYN_ASSERT_STR_EQ(completed, "Kernel");
  ns_string_free(completed);
  SYN_ASSERT_MSG(syn_completions == 1, "completed_string fired %d times",
                 syn_completions);
  char *nothing = syn_test_combo_box_completed_string(combo_box, "zz");
  SYN_ASSERT_MSG(nothing == NULL, "a prefix nothing completes came back as %s",
                 nothing);

  /* The index question AppKit asks while it matches what the user typed. */
  SYN_ASSERT_MSG(
      syn_test_combo_box_index_of_string_value(combo_box, "Tooling") == 2,
      "the index member answered %ld",
      syn_test_combo_box_index_of_string_value(combo_box, "Tooling"));
  /* The boundary's -1 is the NSNotFound AppKit expects back (R11). */
  SYN_ASSERT_MSG(
      syn_test_combo_box_index_of_string_value(combo_box, "Missing") == -1,
      "a value the model does not carry did not come back as NSNotFound");
  SYN_ASSERT_MSG(syn_index_asked == 2, "the index member fired %d times",
                 syn_index_asked);

  ns_release(combo_box);
}

/* R9: an unset optional member is a method the object does not implement, and
 * AppKit takes its own documented default. */
SYN_TEST(an_unset_optional_member_leaves_appkits_own_behaviour_in_place) {
  syn_test_bootstrap();
  syn_reset();

  ns_combo_box *combo_box =
      ns_combo_box_create_with_frame(CGRectMake(0, 0, 160, 24));
  ns_combo_box_set_uses_data_source(combo_box, true);
  ns_combo_box_callbacks required_only = {
      .number_of_items = syn_number_of_items,
      .object_value_for_item_at_index = syn_object_value};
  ns_combo_box_set_callbacks(combo_box, &required_only, NULL);
  ns_combo_box_reload_data(combo_box);

  SYN_ASSERT_MSG(syn_test_delegate_responds(combo_box,
                                            "numberOfItemsInComboBox:"),
                 "a set member is not reported as implemented");
  SYN_ASSERT_MSG(!syn_test_delegate_responds(combo_box, "comboBoxWillPopUp:"),
                 "an unset optional member is reported as implemented");
  SYN_ASSERT_MSG(
      !syn_test_delegate_responds(combo_box, "comboBox:completedString:"),
      "an unset optional member is reported as implemented");

  syn_test_post_notification(SYN_WILL_POP_UP, combo_box);
  syn_test_post_notification(SYN_DID_CHANGE, combo_box);
  syn_test_spin(50);
  SYN_ASSERT_MSG(syn_pop_ups == 0 && syn_edits == 0, "an unset member fired");

  /* With no completed_string, AppKit completes from the values themselves -
   * its own default, and the member that is set still answering. */
  char *completed = syn_test_combo_box_completed_string(combo_box, "Ker");
  SYN_ASSERT_STR_EQ(completed, "Kernel");
  ns_string_free(completed);
  SYN_ASSERT_MSG(syn_completions == 0, "an unset member fired");
  SYN_ASSERT_MSG(syn_values_asked > 0,
                 "AppKit's own completion asked the value member nothing");

  ns_release(combo_box);
}

/* The `assign` slot, which AppKit neither retains nor zeroes: an uninstall has
 * to leave nil in it, not the address of a freed shim (R9, KTD8). */
SYN_TEST(uninstalling_leaves_both_appkit_slots_nil) {
  syn_test_bootstrap();
  syn_reset();
  int context = 21;

  ns_combo_box *combo_box = syn_data_source_combo_box(&context);
  SYN_ASSERT_MSG(syn_test_has_delegate(combo_box), "no delegate was installed");
  SYN_ASSERT_MSG(syn_test_has_data_source(combo_box),
                 "no data source was installed");
  SYN_ASSERT_SHIMS(1);

  /* Installing again replaces the struct, its context and the shim. */
  int other_context = 22;
  ns_combo_box_callbacks callbacks = syn_all_members();
  ns_combo_box_set_callbacks(combo_box, &callbacks, &other_context);
  SYN_ASSERT_SHIMS(1);
  syn_counts = 0;
  ns_combo_box_reload_data(combo_box);
  SYN_ASSERT_MSG(syn_context_seen == &other_context,
                 "the replaced struct kept the first context");

  ns_combo_box_set_callbacks(combo_box, NULL, NULL);
  SYN_ASSERT_MSG(!syn_test_has_delegate(combo_box),
                 "a null struct left a delegate installed");
  SYN_ASSERT_MSG(!syn_test_has_data_source(combo_box),
                 "a null struct left a data source installed");
  SYN_ASSERT_SHIMS(0);

  /* The reload AppKit answers from the nil slot: an empty list, not a read of
   * a freed shim. */
  syn_counts = 0;
  ns_combo_box_reload_data(combo_box);
  syn_test_spin(50);
  SYN_ASSERT_MSG(syn_counts == 0, "an uninstalled member fired");
  SYN_ASSERT_MSG(ns_combo_box_number_of_items(combo_box) == 0,
                 "a combo box with no data source reported %ld values",
                 ns_combo_box_number_of_items(combo_box));

  ns_release(combo_box);
}

/* The other half of the `assign` question: nothing has to be uninstalled
 * before the last reference goes, because the shim is held by the combo box
 * and dies with it. This is also the teardown the leak gate reads (KTD12). */
SYN_TEST(releasing_a_combo_box_with_a_data_source_installed_is_clean) {
  syn_test_bootstrap();
  syn_reset();
  int context = 4;

  ns_view *pane = ns_view_create_with_frame(CGRectMake(0, 0, 320, 80));
  ns_combo_box *combo_box = syn_data_source_combo_box(&context);
  ns_control_set_action(ns_combo_box_as_control(combo_box), NULL, NULL);
  ns_view_add_subview(pane, ns_combo_box_as_view(combo_box));
  ns_release(combo_box); /* the pane's tree is the only holder now */
  SYN_ASSERT(ns_view_subview_count(pane) == 1);
  SYN_ASSERT_SHIMS(1);

  ns_release(pane);
  SYN_ASSERT_SHIMS(0);
}

/* ---- the cases that abort, each run in a child (R9, R10, R12, KTD4) ---- */

SYN_ABORT_CASE(combo_box_installed_without_number_of_items) {
  syn_test_bootstrap();
  ns_combo_box *combo_box =
      ns_combo_box_create_with_frame(CGRectMake(0, 0, 160, 24));
  ns_combo_box_set_uses_data_source(combo_box, true);
  ns_combo_box_callbacks callbacks = {.object_value_for_item_at_index =
                                          syn_object_value};
  ns_combo_box_set_callbacks(combo_box, &callbacks, NULL);
}

SYN_ABORT_CASE(combo_box_installed_without_object_value) {
  syn_test_bootstrap();
  ns_combo_box *combo_box =
      ns_combo_box_create_with_frame(CGRectMake(0, 0, 160, 24));
  ns_combo_box_set_uses_data_source(combo_box, true);
  ns_combo_box_callbacks callbacks = {.number_of_items = syn_number_of_items};
  ns_combo_box_set_callbacks(combo_box, &callbacks, NULL);
}

SYN_TEST(a_missing_required_member_names_the_protocol_and_the_member) {
  SYN_ASSERT_ABORTS("combo_box_installed_without_number_of_items",
                    "ns_combo_box_set_callbacks");
  SYN_ASSERT_ABORTS("combo_box_installed_without_number_of_items",
                    "NSComboBoxDataSource + NSComboBoxDelegate");
  SYN_ASSERT_ABORTS("combo_box_installed_without_number_of_items",
                    "number_of_items");
  SYN_ASSERT_ABORTS("combo_box_installed_without_object_value",
                    "object_value_for_item_at_index");
}

static long syn_negative_count(void *context, ns_combo_box *sender) {
  (void)context;
  (void)sender;
  return -1;
}

SYN_ABORT_CASE(number_of_items_returning_a_negative_count) {
  syn_test_bootstrap();
  ns_combo_box *combo_box =
      ns_combo_box_create_with_frame(CGRectMake(0, 0, 160, 24));
  ns_combo_box_set_uses_data_source(combo_box, true);
  ns_combo_box_callbacks callbacks = {.number_of_items = syn_negative_count,
                                      .object_value_for_item_at_index =
                                          syn_object_value};
  ns_combo_box_set_callbacks(combo_box, &callbacks, NULL);
  ns_combo_box_reload_data(combo_box);
}

SYN_TEST(a_negative_count_from_the_struct_is_reported) {
  SYN_ASSERT_ABORTS("number_of_items_returning_a_negative_count",
                    "number_of_items");
  SYN_ASSERT_ABORTS("number_of_items_returning_a_negative_count",
                    "returned the count -1");
}

SYN_ABORT_CASE(item_selected_at_the_item_count) {
  syn_test_bootstrap();
  ns_combo_box *combo_box =
      ns_combo_box_create_with_frame(CGRectMake(0, 0, 160, 24));
  ns_combo_box_add_items_with_object_values(combo_box, syn_values,
                                            SYN_VALUE_COUNT);
  ns_combo_box_select_item_at_index(combo_box, SYN_VALUE_COUNT);
}

SYN_ABORT_CASE(item_deselected_past_the_end) {
  syn_test_bootstrap();
  ns_combo_box *combo_box =
      ns_combo_box_create_with_frame(CGRectMake(0, 0, 160, 24));
  ns_combo_box_add_items_with_object_values(combo_box, syn_values,
                                            SYN_VALUE_COUNT);
  ns_combo_box_deselect_item_at_index(combo_box, 9);
}

SYN_TEST(an_index_outside_the_list_names_the_function_and_the_range) {
  SYN_ASSERT_ABORTS("item_selected_at_the_item_count",
                    "ns_combo_box_select_item_at_index");
  SYN_ASSERT_ABORTS("item_selected_at_the_item_count", "index 3 is out of range");
  SYN_ASSERT_ABORTS("item_selected_at_the_item_count", "accepts 0 through 2");
  SYN_ASSERT_ABORTS("item_deselected_past_the_end",
                    "ns_combo_box_deselect_item_at_index");
}

/* R12: AppKit takes a call meant for the other list and answers it from the
 * one it is not showing, so the wrapper reports which mode the call needs. */
SYN_ABORT_CASE(static_call_on_a_data_source_combo_box) {
  syn_test_bootstrap();
  int context = 1;
  ns_combo_box *combo_box = syn_data_source_combo_box(&context);
  ns_combo_box_add_item_with_object_value(combo_box, "Design");
}

SYN_ABORT_CASE(callbacks_installed_before_uses_data_source) {
  syn_test_bootstrap();
  ns_combo_box *combo_box =
      ns_combo_box_create_with_frame(CGRectMake(0, 0, 160, 24));
  ns_combo_box_callbacks callbacks = syn_all_members();
  ns_combo_box_set_callbacks(combo_box, &callbacks, NULL);
}

SYN_TEST(a_call_meant_for_the_other_list_names_the_mode_it_needs) {
  SYN_ASSERT_ABORTS("static_call_on_a_data_source_combo_box",
                    "ns_combo_box_add_item_with_object_value");
  SYN_ASSERT_ABORTS("static_call_on_a_data_source_combo_box",
                    "belongs to the static list");
  SYN_ASSERT_ABORTS("static_call_on_a_data_source_combo_box",
                    "usesDataSource true");
  SYN_ASSERT_ABORTS("callbacks_installed_before_uses_data_source",
                    "ns_combo_box_set_callbacks");
  SYN_ASSERT_ABORTS("callbacks_installed_before_uses_data_source",
                    "belongs to the data source");
}

static void *syn_create_a_combo_box_off_the_main_thread(void *unused) {
  (void)unused;
  ns_combo_box_create_with_frame(CGRectMake(0, 0, 160, 24));
  return NULL;
}

SYN_ABORT_CASE(combo_box_created_off_the_main_thread) {
  syn_test_bootstrap();
  pthread_t thread;
  if (pthread_create(&thread, NULL, syn_create_a_combo_box_off_the_main_thread,
                     NULL) != 0) {
    fprintf(stderr, "could not start the background thread\n");
    return;
  }
  pthread_join(thread, NULL);
}

SYN_TEST(creating_a_combo_box_off_the_main_thread_names_the_function) {
  SYN_ASSERT_ABORTS("combo_box_created_off_the_main_thread",
                    "ns_combo_box_create_with_frame");
  SYN_ASSERT_ABORTS("combo_box_created_off_the_main_thread",
                    "main thread only");
}

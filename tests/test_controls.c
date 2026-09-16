/*
 * The controls suite (U7): the push button, the checkbox, the label and the
 * editable text field with its delegate, and the pop-up button with its items,
 * its selection and its index check - the detail pane's controls and the
 * settings panes', driven the way R17 says they commit (R9, R11, R12, R17,
 * KTD17).
 */

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "runner.h"
#include "support.h"
#include "syntonic/syntonic.h"

/* AppKit's own spelling of the notification a closing editing session posts.
 * Posting it is how an off-screen suite ends an edit: there is no field editor
 * without a key window (R21). */
#define SYN_DID_END_EDITING "NSControlTextDidEndEditingNotification"
#define SYN_DID_CHANGE "NSControlTextDidChangeNotification"

static int syn_fires;
static void *syn_context_seen;
static const void *syn_sender_seen;
static int syn_changes;
static int syn_ends;
static void *syn_editing_context;
static const void *syn_editing_sender;
static char syn_committed[64];

static void syn_reset(void) {
  syn_fires = syn_changes = syn_ends = 0;
  syn_context_seen = syn_editing_context = NULL;
  syn_sender_seen = syn_editing_sender = NULL;
  syn_committed[0] = '\0';
}

static void syn_on_action(void *context, const void *sender) {
  syn_fires++;
  syn_context_seen = context;
  syn_sender_seen = sender;
}

static void syn_on_did_change(void *context, ns_text_field *sender) {
  (void)sender;
  syn_changes++;
  syn_editing_context = context;
}

/* The commit R17 asks for: read the field's text when its editing session
 * ends, through the control upcast the text lives on. */
static void syn_on_did_end_editing(void *context, ns_text_field *sender) {
  syn_ends++;
  syn_editing_context = context;
  syn_editing_sender = sender;
  char *value = ns_control_copy_string_value(ns_text_field_as_control(sender));
  snprintf(syn_committed, sizeof syn_committed, "%s",
           value != NULL ? value : "");
  ns_string_free(value);
}

/* ---- the push button (R9, R17) ---- */

SYN_TEST(a_push_button_fires_its_action_with_the_context_and_the_button) {
  syn_test_bootstrap();
  syn_reset();
  int context = 31;

  ns_button *button = ns_button_create_with_title("Delete");
  SYN_ASSERT_STR_EQ(syn_test_class_name(button), "NSButton");
  ns_control *control = ns_button_as_control(button);
  ns_control_set_action(control, syn_on_action, &context);

  ns_control_perform_click(control);
  SYN_WAIT_FOR(syn_fires == 1, 1000);
  SYN_ASSERT_MSG(syn_sender_seen == button,
                 "the sender was not the button that fired");
  SYN_ASSERT_MSG(syn_context_seen == &context, "the context did not arrive");
  SYN_ASSERT_STR_EQ(syn_test_class_name(syn_sender_seen), "NSButton");

  /* Installing again replaces; a null action uninstalls and leaves the button
   * with no action at all (R9). */
  ns_control_set_action(control, NULL, NULL);
  SYN_ASSERT_STR_EQ(syn_test_action_name(button), "");
  ns_control_perform_click(control);
  syn_test_spin(50);
  SYN_ASSERT_MSG(syn_fires == 1, "an uninstalled action fired");

  ns_release(button);
}

SYN_TEST(a_button_keeps_its_title_its_bezel_style_and_its_enabled_flag) {
  syn_test_bootstrap();
  ns_button *button = ns_button_create_with_title("Delete");
  ns_control *control = ns_button_as_control(button);

  char *first = ns_button_copy_title(button);
  char *second = ns_button_copy_title(button);
  SYN_ASSERT_STR_EQ(first, "Delete");
  SYN_ASSERT_MSG(first != second, "two copies of the title came back as one");
  ns_string_free(first);
  SYN_ASSERT_STR_EQ(second, "Delete"); /* the other copy is untouched */
  ns_string_free(second);

  ns_button_set_title(button, "Remove");
  char *renamed = ns_button_copy_title(button);
  SYN_ASSERT_STR_EQ(renamed, "Remove");
  ns_string_free(renamed);

  /* The bezel style the twins give a form's push button (R17). */
  ns_button_set_bezel_style(button, NS_BEZEL_STYLE_PUSH);
  SYN_ASSERT_MSG(ns_button_bezel_style(button) == NS_BEZEL_STYLE_PUSH,
                 "the bezel style read back as %lu",
                 (unsigned long)ns_button_bezel_style(button));

  /* With no selection the detail pane disables Delete (R17). */
  SYN_ASSERT_MSG(ns_control_enabled(control), "a new button started disabled");
  ns_control_set_enabled(control, false);
  SYN_ASSERT(!ns_control_enabled(control));
  ns_control_set_enabled(control, true);
  SYN_ASSERT(ns_control_enabled(control));

  ns_release(button);
}

/* ---- the checkbox (R17) ---- */

SYN_TEST(a_checkbox_toggles_its_state_and_reports_it) {
  syn_test_bootstrap();
  syn_reset();
  int context = 5;

  ns_button *checkbox = ns_button_create_checkbox_with_title("Flagged");
  ns_control *control = ns_button_as_control(checkbox);
  ns_control_set_action(control, syn_on_action, &context);

  SYN_ASSERT_MSG(ns_button_state(checkbox) == NS_CONTROL_STATE_VALUE_OFF,
                 "a new checkbox did not start off");

  /* A click flips the box and fires the action in one go, which is what makes
   * the checkbox commit immediately (R17). */
  ns_control_perform_click(control);
  SYN_WAIT_FOR(syn_fires == 1, 1000);
  SYN_ASSERT_MSG(ns_button_state(checkbox) == NS_CONTROL_STATE_VALUE_ON,
                 "a click did not turn the checkbox on");
  SYN_ASSERT_MSG(syn_sender_seen == checkbox, "the sender was not the checkbox");

  ns_control_perform_click(control);
  SYN_WAIT_FOR(syn_fires == 2, 1000);
  SYN_ASSERT_MSG(ns_button_state(checkbox) == NS_CONTROL_STATE_VALUE_OFF,
                 "a second click did not turn the checkbox off");

  /* The program's own half: the detail pane sets the state from the model. */
  ns_button_set_state(checkbox, NS_CONTROL_STATE_VALUE_ON);
  SYN_ASSERT(ns_button_state(checkbox) == NS_CONTROL_STATE_VALUE_ON);
  ns_button_set_state(checkbox, NS_CONTROL_STATE_VALUE_OFF);
  SYN_ASSERT(ns_button_state(checkbox) == NS_CONTROL_STATE_VALUE_OFF);
  SYN_ASSERT_MSG(syn_fires == 2, "setting the state fired the action");

  ns_release(checkbox);
}

/* ---- the text field (R7, R9, R11, R17) ---- */

SYN_TEST(a_text_fields_string_round_trips_as_an_owned_copy) {
  syn_test_bootstrap();
  ns_text_field *field = ns_text_field_create_with_string("Kernel");
  SYN_ASSERT_STR_EQ(syn_test_class_name(field), "NSTextField");
  ns_control *control = ns_text_field_as_control(field);

  char *first = ns_control_copy_string_value(control);
  char *second = ns_control_copy_string_value(control);
  SYN_ASSERT_STR_EQ(first, "Kernel");
  SYN_ASSERT_MSG(first != second, "two copies of the value came back as one");
  ns_string_free(first);
  SYN_ASSERT_STR_EQ(second, "Kernel"); /* the other copy is untouched */
  ns_string_free(second);

  ns_control_set_string_value(control, "Kernel rewrite");
  char *edited = ns_control_copy_string_value(control);
  SYN_ASSERT_STR_EQ(edited, "Kernel rewrite");
  ns_string_free(edited);

  ns_text_field_set_placeholder_string(field, "Title");
  char *placeholder = ns_text_field_copy_placeholder_string(field);
  SYN_ASSERT_STR_EQ(placeholder, "Title");
  ns_string_free(placeholder);
  ns_text_field_set_placeholder_string(field, NULL);
  SYN_ASSERT_MSG(ns_text_field_copy_placeholder_string(field) == NULL,
                 "a null placeholder did not clear the slot");

  /* A label is the other constructor, and its text reads back the same way. */
  ns_text_field *label = ns_text_field_create_label_with_string("Title:");
  char *caption = ns_control_copy_string_value(ns_text_field_as_control(label));
  SYN_ASSERT_STR_EQ(caption, "Title:");
  ns_string_free(caption);

  ns_release(label);
  ns_release(field);
}

/* The list cell's labels are truncated at the tail rather than pushing their
 * column wider, which is NSControl's lineBreakMode (R17). */
SYN_TEST(a_labels_line_break_mode_round_trips) {
  syn_test_bootstrap();
  ns_text_field *label = ns_text_field_create_label_with_string("Title:");
  ns_control *control = ns_text_field_as_control(label);

  /* AppKit's own default for a label built this way. */
  SYN_ASSERT_MSG(ns_control_line_break_mode(control) ==
                     NS_LINE_BREAK_BY_CLIPPING,
                 "a new label started on line break mode %lu",
                 (unsigned long)ns_control_line_break_mode(control));

  ns_control_set_line_break_mode(control, NS_LINE_BREAK_BY_TRUNCATING_TAIL);
  SYN_ASSERT_MSG(ns_control_line_break_mode(control) ==
                     NS_LINE_BREAK_BY_TRUNCATING_TAIL,
                 "the line break mode read back as %lu",
                 (unsigned long)ns_control_line_break_mode(control));

  ns_control_set_line_break_mode(control, NS_LINE_BREAK_BY_WORD_WRAPPING);
  SYN_ASSERT(ns_control_line_break_mode(control) ==
             NS_LINE_BREAK_BY_WORD_WRAPPING);

  ns_release(label);
}

SYN_TEST(ending_an_edit_fires_did_end_editing_with_the_field_and_the_context) {
  syn_test_bootstrap();
  syn_reset();
  int context = 12;

  ns_text_field *field = ns_text_field_create_with_string("Kernel");
  ns_text_field_callbacks callbacks = {.did_change = syn_on_did_change,
                                       .did_end_editing =
                                           syn_on_did_end_editing};
  ns_text_field_set_callbacks(field, &callbacks, &context);
  SYN_ASSERT_SHIMS(1);
  SYN_ASSERT_MSG(syn_test_delegate_responds(field, "controlTextDidEndEditing:"),
                 "a set member is not reported as implemented");

  ns_control_set_string_value(ns_text_field_as_control(field), "Kernel rewrite");
  syn_test_post_notification(SYN_DID_CHANGE, field);
  SYN_WAIT_FOR(syn_changes == 1, 1000);

  syn_test_post_notification(SYN_DID_END_EDITING, field);
  SYN_WAIT_FOR(syn_ends == 1, 1000);
  SYN_ASSERT_MSG(syn_editing_sender == field,
                 "did_end_editing got a sender that is not the field");
  SYN_ASSERT_MSG(syn_editing_context == &context, "the context did not arrive");
  /* What the detail pane does with the callback: read the text and commit it. */
  SYN_ASSERT_STR_EQ(syn_committed, "Kernel rewrite");

  ns_text_field_set_callbacks(field, NULL, NULL);
  SYN_ASSERT_MSG(!syn_test_has_delegate(field),
                 "a null struct left a delegate installed");
  SYN_ASSERT_SHIMS(0);
  syn_test_post_notification(SYN_DID_END_EDITING, field);
  syn_test_spin(50);
  SYN_ASSERT_MSG(syn_ends == 1, "an uninstalled member fired");

  ns_release(field);
}

/* R9: an unset optional member is a method the delegate does not implement,
 * and ending an edit through it is a no-op rather than a crash. */
SYN_TEST(ending_an_edit_with_did_end_editing_unset_does_not_crash) {
  syn_test_bootstrap();
  syn_reset();

  ns_text_field *field = ns_text_field_create_with_string("Kernel");
  ns_text_field_callbacks only_change = {.did_change = syn_on_did_change};
  ns_text_field_set_callbacks(field, &only_change, NULL);

  SYN_ASSERT_MSG(syn_test_has_delegate(field),
                 "a struct with one member unset is still an install");
  SYN_ASSERT_MSG(!syn_test_delegate_responds(field, "controlTextDidEndEditing:"),
                 "an unset optional member is reported as implemented");

  syn_test_post_notification(SYN_DID_END_EDITING, field);
  syn_test_spin(50);
  SYN_ASSERT_MSG(syn_ends == 0, "an unset member fired");

  /* The member that is set still works, so this is an install, not a no-op. */
  syn_test_post_notification(SYN_DID_CHANGE, field);
  SYN_WAIT_FOR(syn_changes == 1, 1000);

  ns_release(field);
}

/* The field's action is the other commit point, and the one the twins use: it
 * fires when the editing session ends, on Return, on Tab and on focus loss
 * (R17, F6). */
SYN_TEST(a_text_fields_action_fires_the_way_an_ended_edit_fires_it) {
  syn_test_bootstrap();
  syn_reset();
  int context = 3;

  ns_text_field *field = ns_text_field_create_with_string("Kernel");
  ns_control *control = ns_text_field_as_control(field);
  ns_control_set_action(control, syn_on_action, &context);

  SYN_ASSERT_MSG(ns_control_send_action(control),
                 "an installed action did not fire");
  SYN_WAIT_FOR(syn_fires == 1, 1000);
  SYN_ASSERT_MSG(syn_sender_seen == field, "the sender was not the field");
  SYN_ASSERT_MSG(syn_context_seen == &context, "the context did not arrive");

  ns_control_set_action(control, NULL, NULL);
  SYN_ASSERT_MSG(!ns_control_send_action(control),
                 "a control with no action reported that one fired");
  syn_test_spin(50);
  SYN_ASSERT_MSG(syn_fires == 1, "an uninstalled action fired");

  ns_release(field);
}

/* ---- the pop-up button (R12, R17, KTD17) ---- */

static ns_pop_up_button *syn_categories(void) {
  ns_pop_up_button *pop_up =
      ns_pop_up_button_create_with_frame_pulls_down(CGRectZero, false);
  const char *const titles[] = {"Kernel", "Docs", "Tooling"};
  ns_pop_up_button_add_items_with_titles(pop_up, titles, 3);
  return pop_up;
}

SYN_TEST(a_pop_ups_items_and_selected_index_round_trip) {
  syn_test_bootstrap();
  ns_pop_up_button *pop_up = syn_categories();
  SYN_ASSERT_STR_EQ(syn_test_class_name(pop_up), "NSPopUpButton");

  SYN_ASSERT_MSG(ns_pop_up_button_number_of_items(pop_up) == 3,
                 "the pop-up has %ld items, not 3",
                 ns_pop_up_button_number_of_items(pop_up));
  const char *expected[] = {"Kernel", "Docs", "Tooling"};
  for (long index = 0; index < 3; index++) {
    char *title = ns_pop_up_button_copy_item_title_at_index(pop_up, index);
    SYN_ASSERT_STR_EQ(title, expected[index]);
    ns_string_free(title);
  }

  /* One at a time appends, and a pop-up selects the first item it is given. */
  ns_pop_up_button_add_item_with_title(pop_up, "Design");
  SYN_ASSERT(ns_pop_up_button_number_of_items(pop_up) == 4);
  SYN_ASSERT_MSG(ns_pop_up_button_index_of_selected_item(pop_up) == 0,
                 "a pop-up with items selected nothing");

  ns_pop_up_button_select_item_at_index(pop_up, 2);
  SYN_ASSERT(ns_pop_up_button_index_of_selected_item(pop_up) == 2);
  char *selected = ns_pop_up_button_copy_title_of_selected_item(pop_up);
  SYN_ASSERT_STR_EQ(selected, "Tooling");
  ns_string_free(selected);

  ns_pop_up_button_select_item_with_title(pop_up, "Docs");
  SYN_ASSERT(ns_pop_up_button_index_of_selected_item(pop_up) == 1);

  /* -1 is AppKit's own "no selection", and the wrapper passes it through. */
  ns_pop_up_button_select_item_at_index(pop_up, -1);
  SYN_ASSERT_MSG(ns_pop_up_button_index_of_selected_item(pop_up) == -1,
                 "-1 did not clear the selection");
  SYN_ASSERT_MSG(ns_pop_up_button_copy_title_of_selected_item(pop_up) == NULL,
                 "a pop-up with no selection reported a title");

  ns_release(pop_up);
}

SYN_TEST(picking_an_item_fires_the_pop_ups_action_with_the_new_selection) {
  syn_test_bootstrap();
  syn_reset();
  int context = 19;

  ns_pop_up_button *pop_up = syn_categories();
  ns_control *control = ns_pop_up_button_as_control(pop_up);
  ns_control_set_action(control, syn_on_action, &context);

  /* A program that changes the selection itself does not fire the action -
   * AppKit's own behaviour, and what keeps the detail pane from committing
   * while it is filling itself in. */
  ns_pop_up_button_select_item_at_index(pop_up, 1);
  syn_test_spin(50);
  SYN_ASSERT_MSG(syn_fires == 0, "a programmatic selection fired the action");

  /* Picking one does: this is the message a click on a menu item sends, and
   * it is what makes the pop-up commit immediately (R17). */
  SYN_ASSERT(ns_control_send_action(control));
  SYN_WAIT_FOR(syn_fires == 1, 1000);
  SYN_ASSERT_MSG(syn_sender_seen == pop_up, "the sender was not the pop-up");
  SYN_ASSERT_MSG(syn_context_seen == &context, "the context did not arrive");
  char *title = ns_pop_up_button_copy_title_of_selected_item(pop_up);
  SYN_ASSERT_STR_EQ(title, "Docs");
  ns_string_free(title);

  ns_release(pop_up);
}

/* A pane of controls with actions and a delegate installed, torn down by its
 * own release: what the leak gate reads (R7, KTD12). */
SYN_TEST(tearing_down_a_pane_of_controls_releases_clean) {
  syn_test_bootstrap();
  syn_reset();
  int context = 1;

  ns_view *pane = ns_view_create_with_frame(CGRectMake(0, 0, 360, 200));
  ns_text_field *label = ns_text_field_create_label_with_string("Title:");
  ns_text_field *field = ns_text_field_create_with_string("Kernel");
  ns_pop_up_button *pop_up = syn_categories();
  ns_button *checkbox = ns_button_create_checkbox_with_title("Flagged");
  ns_button *delete_button = ns_button_create_with_title("Delete");

  ns_text_field_callbacks callbacks = {.did_end_editing =
                                           syn_on_did_end_editing};
  ns_text_field_set_callbacks(field, &callbacks, &context);
  ns_control_set_action(ns_text_field_as_control(field), syn_on_action,
                        &context);
  ns_control_set_action(ns_pop_up_button_as_control(pop_up), syn_on_action,
                        &context);
  ns_control_set_action(ns_button_as_control(checkbox), syn_on_action,
                        &context);
  ns_control_set_action(ns_button_as_control(delete_button), syn_on_action,
                        &context);

  ns_view_add_subview(pane, ns_text_field_as_view(label));
  ns_view_add_subview(pane, ns_text_field_as_view(field));
  ns_view_add_subview(pane, ns_pop_up_button_as_view(pop_up));
  ns_view_add_subview(pane, ns_button_as_view(checkbox));
  ns_view_add_subview(pane, ns_button_as_view(delete_button));
  ns_release(label);
  ns_release(field);
  ns_release(pop_up);
  ns_release(checkbox);
  ns_release(delete_button);
  SYN_ASSERT(ns_view_subview_count(pane) == 5);

  ns_release(pane); /* the pane's tree is the only holder */
  SYN_ASSERT_SHIMS(0);
}

/* ---- the cases that abort, each run in a child (R10, R12, KTD4) ---- */

SYN_ABORT_CASE(item_selected_at_the_item_count) {
  syn_test_bootstrap();
  ns_pop_up_button *pop_up = syn_categories();
  ns_pop_up_button_select_item_at_index(pop_up, 3);
}

SYN_ABORT_CASE(item_selected_below_no_selection) {
  syn_test_bootstrap();
  ns_pop_up_button *pop_up = syn_categories();
  ns_pop_up_button_select_item_at_index(pop_up, -2);
}

SYN_TEST(selecting_outside_the_items_names_the_function_and_the_range) {
  SYN_ASSERT_ABORTS("item_selected_at_the_item_count",
                    "ns_pop_up_button_select_item_at_index");
  SYN_ASSERT_ABORTS("item_selected_at_the_item_count",
                    "index 3 is out of range");
  SYN_ASSERT_ABORTS("item_selected_at_the_item_count", "accepts 0 through 2");
  SYN_ASSERT_ABORTS("item_selected_below_no_selection",
                    "index -2 is out of range");
}

SYN_ABORT_CASE(a_view_passed_where_a_button_belongs) {
  syn_test_bootstrap();
  ns_view *view = ns_view_create_with_frame(CGRectMake(0, 0, 10, 10));
  ns_button_copy_title((ns_button *)(void *)view);
}

SYN_TEST(a_handle_of_the_wrong_class_names_both_classes) {
  SYN_ASSERT_ABORTS("a_view_passed_where_a_button_belongs", "NSButton");
  SYN_ASSERT_ABORTS("a_view_passed_where_a_button_belongs",
                    "ns_button_copy_title");
}

static void *syn_create_a_button_off_the_main_thread(void *unused) {
  (void)unused;
  ns_button_create_with_title("Delete");
  return NULL;
}

SYN_ABORT_CASE(button_created_off_the_main_thread) {
  syn_test_bootstrap();
  pthread_t thread;
  if (pthread_create(&thread, NULL, syn_create_a_button_off_the_main_thread,
                     NULL) != 0) {
    fprintf(stderr, "could not start the background thread\n");
    return;
  }
  pthread_join(thread, NULL);
}

SYN_TEST(creating_a_button_off_the_main_thread_names_the_function) {
  SYN_ASSERT_ABORTS("button_created_off_the_main_thread",
                    "ns_button_create_with_title");
  SYN_ASSERT_ABORTS("button_created_off_the_main_thread", "main thread only");
}

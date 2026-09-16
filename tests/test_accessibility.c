/*
 * The accessibility suite (U7, R23).
 *
 * R23 has two halves and this suite is where both are pinned. The first is
 * what the library does not do: a wrapper never touches an accessibility
 * property, so a titled button still reads as its title with no call made and
 * a nib is not needed to get that. The second is the escape hatch for the
 * controls AppKit has nothing to infer from - a text field, a pop-up - which
 * is the two setters in ns_view.h.
 */

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "runner.h"
#include "support.h"
#include "syntonic/syntonic.h"

/* AppKit's own role constants, as the UTF-8 a role crosses the boundary as
 * (R11). They are AppKit's spelling exactly, so the two documents line up. */
#define SYN_AX_BUTTON_ROLE "AXButton"
#define SYN_AX_GROUP_ROLE "AXGroup"
#define SYN_AX_POP_UP_BUTTON_ROLE "AXPopUpButton"

/* ---- what the library leaves alone (R23) ---- */

/* R23's real claim: the inference survives because nothing in the wrapper
 * touches it. No accessibility call is made anywhere in this test. */
SYN_TEST(a_titled_control_reports_the_label_appkit_infers_from_its_title) {
  syn_test_bootstrap();

  ns_button *button = ns_button_create_with_title("Delete");
  char *label = ns_view_copy_accessibility_label(ns_button_as_view(button));
  SYN_ASSERT_MSG(label != NULL && label[0] != '\0',
                 "a titled button reported no inferred label");
  SYN_ASSERT_STR_EQ(label, "Delete");
  ns_string_free(label);

  ns_button *checkbox = ns_button_create_checkbox_with_title("Flagged");
  char *checkbox_label =
      ns_view_copy_accessibility_label(ns_button_as_view(checkbox));
  SYN_ASSERT_STR_EQ(checkbox_label, "Flagged");
  ns_string_free(checkbox_label);

  /* Renaming the button renames what it reads as, because the inference is
   * AppKit's and stays live. */
  ns_button_set_title(button, "Remove");
  char *renamed = ns_view_copy_accessibility_label(ns_button_as_view(button));
  SYN_ASSERT_STR_EQ(renamed, "Remove");
  ns_string_free(renamed);

  ns_release(checkbox);
  ns_release(button);
}

/* The other side of R23: a control with no text of its own has nothing to
 * infer from, which is why the setters exist at all. */
SYN_TEST(a_control_with_no_text_of_its_own_has_no_inferred_label) {
  syn_test_bootstrap();

  ns_text_field *field = ns_text_field_create_with_string("Kernel");
  SYN_ASSERT_MSG(
      ns_view_copy_accessibility_label(ns_text_field_as_view(field)) == NULL,
      "an editable field reported a label with nothing to infer one from");

  ns_pop_up_button *pop_up =
      ns_pop_up_button_create_with_frame_pulls_down(CGRectZero, false);
  ns_pop_up_button_add_item_with_title(pop_up, "Kernel");
  SYN_ASSERT_MSG(
      ns_view_copy_accessibility_label(ns_pop_up_button_as_view(pop_up)) == NULL,
      "a pop-up reported a label with nothing to infer one from");

  ns_view *view = ns_view_create_with_frame(CGRectMake(0, 0, 40, 40));
  SYN_ASSERT_MSG(ns_view_copy_accessibility_label(view) == NULL,
                 "a plain view reported a label the wrapper did not set");

  ns_release(view);
  ns_release(pop_up);
  ns_release(field);
}

/* ---- what the caller can name (R23) ---- */

SYN_TEST(an_accessibility_label_and_role_read_back_as_they_were_set) {
  syn_test_bootstrap();

  /* The detail pane's three named rows, exactly as the Swift twin names them
   * and exactly as a nib would have (R17, R23). */
  ns_text_field *title_field = ns_text_field_create_with_string("Kernel");
  ns_view *view = ns_text_field_as_view(title_field);
  ns_view_set_accessibility_label(view, "Title");

  char *first = ns_view_copy_accessibility_label(view);
  char *second = ns_view_copy_accessibility_label(view);
  SYN_ASSERT_STR_EQ(first, "Title");
  SYN_ASSERT_MSG(first != second, "two copies of the label came back as one");
  ns_string_free(first);
  SYN_ASSERT_STR_EQ(second, "Title"); /* the other copy is untouched */
  ns_string_free(second);

  ns_pop_up_button *pop_up =
      ns_pop_up_button_create_with_frame_pulls_down(CGRectZero, false);
  ns_view_set_accessibility_label(ns_pop_up_button_as_view(pop_up), "Category");
  ns_view_set_accessibility_role(ns_pop_up_button_as_view(pop_up),
                                 SYN_AX_POP_UP_BUTTON_ROLE);
  char *label =
      ns_view_copy_accessibility_label(ns_pop_up_button_as_view(pop_up));
  char *role = ns_view_copy_accessibility_role(ns_pop_up_button_as_view(pop_up));
  SYN_ASSERT_STR_EQ(label, "Category");
  SYN_ASSERT_STR_EQ(role, SYN_AX_POP_UP_BUTTON_ROLE);
  ns_string_free(label);
  ns_string_free(role);

  /* A role on a plain view, which is the container case: a group. */
  ns_view *pane = ns_view_create_with_frame(CGRectMake(0, 0, 360, 200));
  ns_view_set_accessibility_role(pane, SYN_AX_GROUP_ROLE);
  char *pane_role = ns_view_copy_accessibility_role(pane);
  SYN_ASSERT_STR_EQ(pane_role, SYN_AX_GROUP_ROLE);
  ns_string_free(pane_role);

  ns_release(pane);
  ns_release(pop_up);
  ns_release(title_field);
}

/* A value the caller sets replaces AppKit's inference rather than adding to
 * it, and null replaces it with nothing: AppKit does not infer again. That is
 * why ns_view.h says to set a label only where there is none to infer. */
SYN_TEST(a_label_the_caller_sets_replaces_the_inferred_one_for_good) {
  syn_test_bootstrap();
  ns_button *button = ns_button_create_with_title("Delete");
  ns_view *view = ns_button_as_view(button);

  ns_view_set_accessibility_label(view, "Remove this item");
  char *set = ns_view_copy_accessibility_label(view);
  SYN_ASSERT_STR_EQ(set, "Remove this item");
  ns_string_free(set);

  ns_view_set_accessibility_label(view, NULL);
  SYN_ASSERT_MSG(ns_view_copy_accessibility_label(view) == NULL,
                 "clearing the caller's label brought the inference back, "
                 "which ns_view.h says it does not");

  ns_view_set_accessibility_role(view, SYN_AX_BUTTON_ROLE);
  char *role = ns_view_copy_accessibility_role(view);
  SYN_ASSERT_STR_EQ(role, SYN_AX_BUTTON_ROLE);
  ns_string_free(role);
  ns_view_set_accessibility_role(view, NULL);
  SYN_ASSERT_MSG(ns_view_copy_accessibility_role(view) == NULL,
                 "clearing the caller's role brought the inference back");

  ns_release(button);
}

/* ---- the case that aborts, run in a child (R10, KTD4) ---- */

static const void *syn_view_for_thread;

static void *syn_label_a_view_off_the_main_thread(void *unused) {
  (void)unused;
  ns_view_set_accessibility_label((ns_view *)(void *)syn_view_for_thread,
                                  "Title");
  return NULL;
}

SYN_ABORT_CASE(label_set_off_the_main_thread) {
  syn_test_bootstrap();
  syn_view_for_thread = ns_view_create_with_frame(CGRectMake(0, 0, 10, 10));
  pthread_t thread;
  if (pthread_create(&thread, NULL, syn_label_a_view_off_the_main_thread,
                     NULL) != 0) {
    fprintf(stderr, "could not start the background thread\n");
    return;
  }
  pthread_join(thread, NULL);
}

SYN_TEST(setting_a_label_off_the_main_thread_names_the_function) {
  SYN_ASSERT_ABORTS("label_set_off_the_main_thread",
                    "ns_view_set_accessibility_label");
  SYN_ASSERT_ABORTS("label_set_off_the_main_thread", "main thread only");
}

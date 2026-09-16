/*
 * The view suite (U7): the view tree, the frame, the autoresizing mask, the
 * hidden flag, the upcasts, and the two ownership scenarios the view hierarchy
 * is where they show - a borrowed content view kept past its window, and AE2's
 * button that outlives the caller's reference (R4, R7, AE2, KTD7, KTD9,
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

enum { SYN_STANDARD_MASK = NS_WINDOW_STYLE_MASK_TITLED |
                           NS_WINDOW_STYLE_MASK_CLOSABLE |
                           NS_WINDOW_STYLE_MASK_MINIATURIZABLE |
                           NS_WINDOW_STYLE_MASK_RESIZABLE };

static ns_window *syn_window_create(void) {
  return ns_window_create_with_content_rect_style_mask_backing_defer(
      CGRectMake(0, 0, 400, 300), (ns_window_style_mask)SYN_STANDARD_MASK,
      NS_BACKING_STORE_BUFFERED, false);
}

static int syn_fires;
static void *syn_context_seen;
static const void *syn_sender_seen;

static void syn_reset(void) {
  syn_fires = 0;
  syn_context_seen = NULL;
  syn_sender_seen = NULL;
}

static void syn_on_action(void *context, const void *sender) {
  syn_fires++;
  syn_context_seen = context;
  syn_sender_seen = sender;
}

static ns_view *syn_view(CGFloat width, CGFloat height) {
  return ns_view_create_with_frame(CGRectMake(0, 0, width, height));
}

/* ---- the class, the frame and the two flags (R5, R7) ---- */

SYN_TEST(a_view_keeps_the_frame_it_was_created_with_and_the_one_it_is_given) {
  syn_test_bootstrap();
  ns_view *view = syn_view(120, 40);
  SYN_ASSERT_STR_EQ(syn_test_class_name(view), "NSView");

  CGRect frame = ns_view_frame(view);
  SYN_ASSERT_MSG(frame.size.width == 120 && frame.size.height == 40,
                 "the frame came back as %gx%g", frame.size.width,
                 frame.size.height);

  ns_view_set_frame(view, CGRectMake(10, 20, 200, 60));
  frame = ns_view_frame(view);
  SYN_ASSERT_MSG(frame.origin.x == 10 && frame.origin.y == 20 &&
                     frame.size.width == 200 && frame.size.height == 60,
                 "the frame read back as %g,%g %gx%g", frame.origin.x,
                 frame.origin.y, frame.size.width, frame.size.height);

  ns_release(view);
}

SYN_TEST(the_autoresizing_mask_and_the_hidden_flag_round_trip) {
  syn_test_bootstrap();
  ns_view *view = syn_view(120, 40);

  /* AppKit's own default: a new view does not resize with its superview. */
  SYN_ASSERT_MSG(ns_view_autoresizing_mask(view) == NS_VIEW_NOT_SIZABLE,
                 "a new view started with mask 0x%lx",
                 (unsigned long)ns_view_autoresizing_mask(view));
  ns_view_set_autoresizing_mask(
      view, NS_VIEW_WIDTH_SIZABLE | NS_VIEW_HEIGHT_SIZABLE);
  ns_autoresizing_mask_options mask = ns_view_autoresizing_mask(view);
  SYN_ASSERT_MSG((mask & NS_VIEW_WIDTH_SIZABLE) != 0 &&
                     (mask & NS_VIEW_HEIGHT_SIZABLE) != 0,
                 "the mask read back as 0x%lx", (unsigned long)mask);

  SYN_ASSERT_MSG(!ns_view_hidden(view), "a new view started hidden");
  ns_view_set_hidden(view, true);
  SYN_ASSERT(ns_view_hidden(view));
  ns_view_set_hidden(view, false);
  SYN_ASSERT(!ns_view_hidden(view));

  ns_release(view);
}

/* A view laid out by an autoresizing mask follows its superview, which is the
 * resizing R17 asks the detail pane for without an Auto Layout API in v0. */
SYN_TEST(a_sizable_subview_follows_its_superview) {
  syn_test_bootstrap();
  ns_view *pane = syn_view(200, 100);
  ns_view *child = syn_view(200, 100);
  ns_view_set_autoresizing_mask(child,
                                NS_VIEW_WIDTH_SIZABLE | NS_VIEW_HEIGHT_SIZABLE);
  ns_view_add_subview(pane, child);
  ns_release(child);

  ns_view_set_frame(pane, CGRectMake(0, 0, 400, 300));
  CGRect frame = ns_view_frame(ns_view_subview_at_index(pane, 0));
  SYN_ASSERT_MSG(frame.size.width == 400 && frame.size.height == 300,
                 "the subview did not follow its superview: %gx%g",
                 frame.size.width, frame.size.height);

  ns_release(pane);
}

/* ---- the view tree and the array shape (R7, KTD17) ---- */

SYN_TEST(the_subview_count_and_the_index_accessor_follow_insertion_order) {
  syn_test_bootstrap();
  ns_view *pane = syn_view(300, 200);
  SYN_ASSERT_MSG(ns_view_subview_count(pane) == 0, "a new view had subviews");

  ns_view *first = syn_view(10, 10);
  ns_view *second = syn_view(20, 20);
  ns_view *third = syn_view(30, 30);
  ns_view_add_subview(pane, first);
  ns_view_add_subview(pane, second);
  ns_view_add_subview(pane, third);
  SYN_ASSERT_MSG(ns_view_subview_count(pane) == 3,
                 "the count did not follow the insertions: %ld",
                 ns_view_subview_count(pane));

  ns_view *expected[] = {first, second, third};
  for (long index = 0; index < 3; index++)
    SYN_ASSERT_MSG(ns_view_subview_at_index(pane, index) == expected[index],
                   "subview %ld is not the one added at that position", index);

  /* Each subview is borrowed from the pane, and the pane is its superview. */
  SYN_ASSERT_MSG(ns_view_superview(second) == pane,
                 "the superview is not the view that holds it");
  SYN_ASSERT_MSG(ns_view_superview(pane) == NULL,
                 "a view with no parent reported a superview");

  ns_release(first);
  ns_release(second);
  ns_release(third);
  ns_release(pane);
}

SYN_TEST(adding_then_removing_a_subview_leaves_the_count_at_zero) {
  syn_test_bootstrap();
  ns_view *pane = syn_view(300, 200);
  ns_view *child = syn_view(40, 40);

  ns_view_add_subview(pane, child);
  SYN_ASSERT(ns_view_subview_count(pane) == 1);
  SYN_ASSERT(ns_view_superview(child) == pane);

  /* The caller's own reference is what keeps the child alive past the
   * removal, which is the ownership ledger's last row (R7). */
  ns_view_remove_from_superview(child);
  SYN_ASSERT_MSG(ns_view_subview_count(pane) == 0,
                 "the subview count is %ld after a removal",
                 ns_view_subview_count(pane));
  SYN_ASSERT_MSG(ns_view_superview(child) == NULL,
                 "a removed view still reported a superview");
  SYN_ASSERT_STR_EQ(syn_test_class_name(child), "NSView");

  ns_release(child);
  ns_release(pane);
}

/* ---- the two ownership scenarios (R7, AE2, KTD7) ---- */

/* A borrowed getter return kept past its owner: ns_retain is what makes that
 * legal, and the window's close and release are what test it (R7). */
SYN_TEST(a_content_view_retained_by_the_caller_outlives_its_window) {
  syn_test_bootstrap();
  ns_window *window = syn_window_create();

  ns_view *content = ns_window_content_view(window);
  SYN_ASSERT_MSG(content != NULL, "the window has no content view");
  ns_retain(content);

  ns_window_close(window);
  ns_release(window); /* the window goes; the retained view does not */

  SYN_ASSERT_STR_EQ(syn_test_class_name(content), "NSView");
  CGRect frame = ns_view_frame(content);
  SYN_ASSERT_MSG(frame.size.width == 400 && frame.size.height == 300,
                 "the content view reported %gx%g after its window went away",
                 frame.size.width, frame.size.height);

  ns_release(content); /* caller 0: what the leak gate reads */
}

/* Covers AE2: the caller gives its reference back and the button carries on -
 * still in the subviews, still firing - until the parent goes away (R7). */
SYN_TEST(ae2_a_button_released_by_the_caller_stays_in_the_tree_and_still_fires) {
  syn_test_bootstrap();
  syn_reset();
  int context = 7;

  ns_window *window = syn_window_create();
  ns_view *content = ns_window_content_view(window);

  ns_button *button = ns_button_create_with_title("Delete");
  ns_control_set_action(ns_button_as_control(button), syn_on_action, &context);
  ns_view_add_subview(content, ns_button_as_view(button));
  ns_release(button); /* caller 0, parent +1 */

  SYN_ASSERT_MSG(ns_view_subview_count(content) == 1,
                 "the button is not in the content view's subviews");
  ns_view *borrowed = ns_view_subview_at_index(content, 0);
  SYN_ASSERT_MSG(borrowed == ns_button_as_view(button),
                 "the subview is not the button that was added");
  SYN_ASSERT_STR_EQ(syn_test_class_name(borrowed), "NSButton");

  char *title = ns_button_copy_title(button);
  SYN_ASSERT_STR_EQ(title, "Delete");
  ns_string_free(title);

  ns_control_perform_click(ns_button_as_control(button));
  SYN_WAIT_FOR(syn_fires == 1, 1000);
  SYN_ASSERT_MSG(syn_sender_seen == button,
                 "the sender was not the button that fired");
  SYN_ASSERT_MSG(syn_context_seen == &context, "the context did not arrive");

  /* Closing and releasing the window tears the tree down with it, and the
   * leak gate is what reads the result (F3, KTD12). */
  ns_window_close(window);
  ns_release(window);
}

/* ---- the upcasts (R4, KTD9) ---- */

SYN_TEST(every_upcast_hands_back_the_same_pointer) {
  syn_test_bootstrap();
  ns_button *button = ns_button_create_with_title("Delete");
  ns_text_field *field = ns_text_field_create_with_string("Kernel");
  ns_pop_up_button *pop_up =
      ns_pop_up_button_create_with_frame_pulls_down(CGRectZero, false);

  SYN_ASSERT((void *)ns_button_as_control(button) == (void *)button);
  SYN_ASSERT((void *)ns_button_as_view(button) == (void *)button);
  SYN_ASSERT((void *)ns_control_as_view(ns_button_as_control(button)) ==
             (void *)button);

  SYN_ASSERT((void *)ns_text_field_as_control(field) == (void *)field);
  SYN_ASSERT((void *)ns_text_field_as_view(field) == (void *)field);

  SYN_ASSERT((void *)ns_pop_up_button_as_button(pop_up) == (void *)pop_up);
  SYN_ASSERT((void *)ns_pop_up_button_as_control(pop_up) == (void *)pop_up);
  SYN_ASSERT((void *)ns_pop_up_button_as_view(pop_up) == (void *)pop_up);

  /* An upcast is a pointer, not a conversion: the view it hands back reports
   * the subclass it really is. */
  SYN_ASSERT_STR_EQ(syn_test_class_name(ns_pop_up_button_as_view(pop_up)),
                    "NSPopUpButton");

  ns_release(pop_up);
  ns_release(field);
  ns_release(button);
}

/* ---- the cases that abort, each run in a child (R10, R12, KTD4) ---- */

SYN_ABORT_CASE(a_window_passed_where_a_view_belongs) {
  syn_test_bootstrap();
  ns_window *window = syn_window_create();
  ns_view_subview_count((ns_view *)(void *)window);
}

SYN_TEST(a_handle_of_the_wrong_class_names_both_classes) {
  SYN_ASSERT_ABORTS("a_window_passed_where_a_view_belongs", "NSView");
  SYN_ASSERT_ABORTS("a_window_passed_where_a_view_belongs",
                    "ns_view_subview_count");
}

/* The index accessor needs no check of its own: NSArray raises on its own and
 * the entry macro reports that exception with this function's name (KTD4). */
SYN_ABORT_CASE(subview_read_past_the_end) {
  syn_test_bootstrap();
  ns_view *pane = syn_view(100, 100);
  ns_view_subview_at_index(pane, 0);
}

SYN_TEST(reading_past_the_end_is_reported_as_the_appkit_exception_it_is) {
  SYN_ASSERT_ABORTS("subview_read_past_the_end", "ns_view_subview_at_index");
  SYN_ASSERT_ABORTS("subview_read_past_the_end", "AppKit raised");
}

static void *syn_create_a_view_off_the_main_thread(void *unused) {
  (void)unused;
  ns_view_create_with_frame(CGRectMake(0, 0, 10, 10));
  return NULL;
}

SYN_ABORT_CASE(view_created_off_the_main_thread) {
  syn_test_bootstrap();
  pthread_t thread;
  if (pthread_create(&thread, NULL, syn_create_a_view_off_the_main_thread,
                     NULL) != 0) {
    fprintf(stderr, "could not start the background thread\n");
    return;
  }
  pthread_join(thread, NULL);
}

SYN_TEST(creating_a_view_off_the_main_thread_names_the_function) {
  SYN_ASSERT_ABORTS("view_created_off_the_main_thread",
                    "ns_view_create_with_frame");
  SYN_ASSERT_ABORTS("view_created_off_the_main_thread", "main thread only");
}

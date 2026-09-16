/*
 * The split view suite (U9): the main window's sidebar, list and detail panes,
 * the sidebar item's system behaviour, and toggleSidebar: - sent directly, and
 * wired to arrive from the standard menu bar's View menu (R15, R21).
 *
 * The menu half is pinned as wiring rather than driven: an application with
 * the prohibited activation policy has no key window and no main window, so
 * AppKit has no responder chain to send a nil-target action down. The test
 * below measures that rather than assuming it.
 *
 * The collapse is animated, so every flag it changes is read through
 * SYN_WAIT_FOR rather than on the next line.
 */

#include <objc/message.h>
#include <objc/runtime.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "runner.h"
#include "support.h"
#include "syntonic/syntonic.h"

#define SYN_APP_NAME "Syntonic Twin"

/* Forcing layout rather than waiting for a display pass is what makes an
 * off-screen suite deterministic (R21). NSView's layoutSubtreeIfNeeded has no
 * C function of its own yet, so this suite reaches it through the runtime
 * rather than spelling somebody else's wrapper a second time, exactly as
 * tests/test_tab_view_controller.c does for the reads it needs. */
static bool syn_responds(const void *object, const char *selector) {
  return ((BOOL (*)(id, SEL, SEL))objc_msgSend)(
             (id)(void *)object, sel_getUid("respondsToSelector:"),
             sel_getUid(selector)) != 0;
}

static bool syn_window_is_key(const void *window) {
  return ((BOOL (*)(id, SEL))objc_msgSend)((id)(void *)window,
                                           sel_getUid("isKeyWindow")) != 0;
}

static void syn_layout(const void *view) {
  ((void (*)(id, SEL))objc_msgSend)((id)(void *)view,
                                    sel_getUid("layoutSubtreeIfNeeded"));
}

/* The twin's geometry, so the numbers here are the numbers the comparison
 * uses. */
enum { SYN_SIDEBAR_MIN = 180, SYN_SIDEBAR_MAX = 320, SYN_LIST_MIN = 280 };

/* A view controller with a view of its own, which is all a pane needs to be
 * laid out. Owned (+1). */
static ns_view_controller *syn_pane_create(CGFloat width) {
  ns_view_controller *pane = ns_view_controller_create();
  ns_view *view = ns_view_create_with_frame(CGRectMake(0, 0, width, 640));
  ns_view_controller_set_view(pane, view);
  ns_release(view);
  return pane;
}

/* The twin's buildSplitViewController, in C: a sidebar, a content list and a
 * plain detail pane. Owned (+1). */
static ns_split_view_controller *syn_split_create(void) {
  ns_split_view_controller *split = ns_split_view_controller_create();

  ns_view_controller *sidebar_pane = syn_pane_create(220);
  ns_split_view_item *sidebar =
      ns_split_view_item_create_sidebar_with_view_controller(sidebar_pane);
  ns_split_view_item_set_minimum_thickness(sidebar, SYN_SIDEBAR_MIN);
  ns_split_view_item_set_maximum_thickness(sidebar, SYN_SIDEBAR_MAX);
  ns_split_view_item_set_can_collapse(sidebar, true);
  ns_split_view_controller_add_split_view_item(split, sidebar);
  ns_release(sidebar);
  ns_release(sidebar_pane);

  ns_view_controller *list_pane = syn_pane_create(400);
  ns_split_view_item *list =
      ns_split_view_item_create_content_list_with_view_controller(list_pane);
  ns_split_view_item_set_minimum_thickness(list, SYN_LIST_MIN);
  ns_split_view_controller_add_split_view_item(split, list);
  ns_release(list);
  ns_release(list_pane);

  ns_view_controller *detail_pane = syn_pane_create(380);
  ns_split_view_item *detail =
      ns_split_view_item_create_with_view_controller(detail_pane);
  ns_split_view_item_set_minimum_thickness(detail, SYN_LIST_MIN);
  ns_split_view_controller_add_split_view_item(split, detail);
  ns_release(detail);
  ns_release(detail_pane);

  return split;
}

/* The twin's main window, off screen. Owned (+1). */
static ns_window *syn_window_create(ns_split_view_controller *split) {
  ns_window *window = ns_window_create_with_content_rect_style_mask_backing_defer(
      CGRectMake(0, 0, 1000, 640),
      NS_WINDOW_STYLE_MASK_TITLED | NS_WINDOW_STYLE_MASK_CLOSABLE |
          NS_WINDOW_STYLE_MASK_MINIATURIZABLE | NS_WINDOW_STYLE_MASK_RESIZABLE,
      NS_BACKING_STORE_BUFFERED, false);
  ns_window_set_title(window, SYN_APP_NAME);
  ns_window_set_restorable(window, false);
  ns_window_set_content_view_controller(
      window, ns_split_view_controller_as_view_controller(split));
  syn_layout(ns_window_content_view(window));
  return window;
}

/* ---- the three panes and what each constructor decided (R15) ---- */

SYN_TEST(a_sidebar_and_a_content_list_report_their_behaviours) {
  syn_test_bootstrap();
  ns_split_view_controller *split = syn_split_create();
  ns_window *window = syn_window_create(split);

  SYN_ASSERT_STR_EQ(syn_test_class_name(split), "NSSplitViewController");
  SYN_ASSERT_MSG(ns_split_view_controller_split_view_item_count(split) == 3,
                 "the controller holds %ld items, not 3",
                 ns_split_view_controller_split_view_item_count(split));

  ns_split_view_item *sidebar =
      ns_split_view_controller_split_view_item_at_index(split, 0);
  ns_split_view_item *list =
      ns_split_view_controller_split_view_item_at_index(split, 1);
  ns_split_view_item *detail =
      ns_split_view_controller_split_view_item_at_index(split, 2);
  SYN_ASSERT_STR_EQ(syn_test_class_name(sidebar), "NSSplitViewItem");

  /* The constructor is what a pane is: the behaviour is read-only afterwards
   * and is the only way to tell a sidebar from a plain pane (R15). */
  SYN_ASSERT_MSG(ns_split_view_item_get_behavior(sidebar) ==
                     NS_SPLIT_VIEW_ITEM_BEHAVIOR_SIDEBAR,
                 "the sidebar constructor did not produce a sidebar");
  SYN_ASSERT_MSG(ns_split_view_item_get_behavior(list) ==
                     NS_SPLIT_VIEW_ITEM_BEHAVIOR_CONTENT_LIST,
                 "the content list constructor did not produce a content list");
  SYN_ASSERT_MSG(ns_split_view_item_get_behavior(detail) ==
                     NS_SPLIT_VIEW_ITEM_BEHAVIOR_DEFAULT,
                 "the plain constructor did not produce a plain pane");

  SYN_ASSERT_MSG(ns_split_view_item_can_collapse(sidebar),
                 "the sidebar cannot collapse");
  SYN_ASSERT_MSG(!ns_split_view_item_can_collapse(detail),
                 "a plain pane collapses, which is not its default");
  SYN_ASSERT_MSG(ns_split_view_item_minimum_thickness(sidebar) ==
                     (CGFloat)SYN_SIDEBAR_MIN,
                 "the sidebar's minimum thickness did not read back");
  SYN_ASSERT_MSG(ns_split_view_item_maximum_thickness(sidebar) ==
                     (CGFloat)SYN_SIDEBAR_MAX,
                 "the sidebar's maximum thickness did not read back");
  SYN_ASSERT_MSG(!ns_split_view_item_collapsed(sidebar),
                 "the sidebar starts collapsed");

  /* Borrowed: the item's view controller is the pane that was handed in, and
   * its view is in the window's tree by now. */
  ns_view_controller *pane = ns_split_view_item_view_controller(sidebar);
  SYN_ASSERT_MSG(pane != NULL, "the sidebar item carries no view controller");
  SYN_WAIT_FOR(ns_view_superview(ns_view_controller_view(pane)) != NULL, 1000);

  ns_window_close(window);
  ns_release(window);
  ns_release(split);
}

/* ---- collapsing (R15) ---- */

SYN_TEST(toggling_the_sidebar_collapses_it_and_reveals_it_again) {
  syn_test_bootstrap();
  ns_split_view_controller *split = syn_split_create();
  ns_window *window = syn_window_create(split);
  ns_split_view_item *sidebar =
      ns_split_view_controller_split_view_item_at_index(split, 0);

  ns_split_view_controller_toggle_sidebar(split);
  SYN_WAIT_FOR(ns_split_view_item_collapsed(sidebar), 2000);

  ns_split_view_controller_toggle_sidebar(split);
  SYN_WAIT_FOR(!ns_split_view_item_collapsed(sidebar), 2000);

  /* The same state the flag drives directly. */
  ns_split_view_item_set_collapsed(sidebar, true);
  SYN_WAIT_FOR(ns_split_view_item_collapsed(sidebar), 2000);
  ns_split_view_item_set_collapsed(sidebar, false);
  SYN_WAIT_FOR(!ns_split_view_item_collapsed(sidebar), 2000);

  ns_window_close(window);
  ns_release(window);
  ns_release(split);
}

/* The View menu's Toggle Sidebar item has a nil target and the action
 * toggleSidebar:, so choosing it walks the responder chain to the split view
 * controller in the front window. Nothing wires the two together (R15, AE5). */
static long syn_index_of(ns_menu *menu, const char *title) {
  for (long index = 0; index < ns_menu_number_of_items(menu); index++) {
    char *found = ns_menu_item_copy_title(ns_menu_item_at_index(menu, index));
    bool hit = found != NULL && strcmp(found, title) == 0;
    ns_string_free(found);
    if (hit) return index;
  }
  return -1;
}

SYN_TEST(the_view_menus_toggle_sidebar_is_wired_to_reach_the_controller) {
  syn_test_bootstrap();
  ns_menu *bar = ns_menu_bar_install_standard(ns_application_shared(),
                                              SYN_APP_NAME, NULL, NULL);
  long view_index = syn_index_of(bar, "View");
  SYN_ASSERT_MSG(view_index >= 0, "there is no View menu");
  ns_menu *view_menu =
      ns_menu_item_submenu(ns_menu_item_at_index(bar, view_index));
  long toggle = syn_index_of(view_menu, "Toggle Sidebar");
  SYN_ASSERT_MSG(toggle >= 0, "the View menu has no Toggle Sidebar item");

  /* The item carries no target of its own: that nil is what sends the action
   * down the chain, and a SEL never crosses the boundary (R11, AE5). */
  ns_menu_item *item = ns_menu_item_at_index(view_menu, toggle);
  SYN_ASSERT_MSG(!syn_test_has_target(item),
                 "the Toggle Sidebar item has a target, so its action never "
                 "reaches the responder chain");
  SYN_ASSERT_STR_EQ(syn_test_action_name(item), "toggleSidebar:");

  /* The other end of the chain: the controller answers that selector, so a
   * key window whose content view controller it is receives the action. */
  ns_split_view_controller *split = syn_split_create();
  ns_window *window = syn_window_create(split);
  SYN_ASSERT_MSG(syn_responds(split, "toggleSidebar:"),
                 "the split view controller does not answer toggleSidebar:, "
                 "so the View menu item would reach nothing");

  /* Choosing the item cannot be driven here: the test application has the
   * prohibited activation policy, so it has no key window and no main window,
   * and -[NSApplication sendAction:to:from:] has no responder chain to walk -
   * measured, not assumed, and the reason this test pins the wiring and
   * ns_split_view_controller_toggle_sidebar pins the effect. */
  SYN_ASSERT_MSG(syn_test_activation_policy_is_prohibited(),
                 "the test application is not off-screen");
  ns_window_make_key_and_order_front(window);
  syn_test_spin(50);
  SYN_ASSERT_MSG(!syn_window_is_key(window),
                 "an off-screen window became key after all, so this suite "
                 "can drive the menu item directly instead");

  ns_window_close(window);
  ns_release(window);
  ns_release(split);
}

/* ---- ownership (R7) ---- */

SYN_TEST(a_borrowed_item_retained_outlives_the_controller) {
  syn_test_bootstrap();
  ns_split_view_controller *split = syn_split_create();
  ns_split_view_item *sidebar =
      ns_split_view_controller_split_view_item_at_index(split, 0);
  ns_retain(sidebar);

  ns_release(split); /* the controller's reference is gone */

  /* The caller's retain is what keeps it: reading it here is not a use after
   * free, and the leak gate proves the release below balances it. */
  SYN_ASSERT_MSG(ns_split_view_item_get_behavior(sidebar) ==
                     NS_SPLIT_VIEW_ITEM_BEHAVIOR_SIDEBAR,
                 "the retained item did not survive its controller");
  ns_release(sidebar);
}

/* ---- the upcast (R4, KTD9) ---- */

SYN_TEST(the_upcast_to_a_view_controller_is_the_same_pointer) {
  syn_test_bootstrap();
  ns_split_view_controller *split = ns_split_view_controller_create();
  SYN_ASSERT_MSG((const void *)ns_split_view_controller_as_view_controller(
                     split) == (const void *)split,
                 "the upcast returned a different pointer");
  ns_release(split);
}

/* ---- teardown, which is what the leak gate reads (F3, KTD12) ---- */

SYN_TEST(tearing_the_split_window_down_releases_clean) {
  syn_test_bootstrap();
  ns_split_view_controller *split = syn_split_create();
  ns_window *window = syn_window_create(split);
  ns_window_make_key_and_order_front(window);
  ns_split_view_controller_toggle_sidebar(split);
  syn_test_spin(50);
  ns_window_close(window);
  ns_release(window);
  ns_release(split);
}

/* ---- the cases that abort (R10, R12, KTD4) ---- */

SYN_ABORT_CASE(split_view_item_read_past_the_end) {
  syn_test_bootstrap();
  ns_split_view_controller *split = syn_split_create();
  ns_split_view_controller_split_view_item_at_index(split, 3);
}

/* The accessor writes no check of its own: NSArray raises before it changes
 * anything, and the entry macro's exception report names the function (KTD4). */
SYN_TEST(reading_a_pane_past_the_end_names_the_function_and_the_exception) {
  SYN_ASSERT_ABORTS("split_view_item_read_past_the_end",
                    "ns_split_view_controller_split_view_item_at_index");
  SYN_ASSERT_ABORTS("split_view_item_read_past_the_end", "AppKit raised");
}

SYN_ABORT_CASE(split_view_controller_passed_where_an_item_is_expected) {
  syn_test_bootstrap();
  ns_split_view_controller *split = ns_split_view_controller_create();
  ns_split_view_item_collapsed((ns_split_view_item *)split);
}

SYN_TEST(a_handle_of_the_wrong_class_names_both_classes) {
  SYN_ASSERT_ABORTS("split_view_controller_passed_where_an_item_is_expected",
                    "NSSplitViewItem");
  SYN_ASSERT_ABORTS("split_view_controller_passed_where_an_item_is_expected",
                    "ns_split_view_item_collapsed");
}

static void *syn_create_split_off_the_main_thread(void *ignored) {
  (void)ignored;
  ns_split_view_controller_create();
  return NULL;
}

SYN_ABORT_CASE(split_view_controller_created_off_the_main_thread) {
  syn_test_bootstrap();
  pthread_t thread;
  if (pthread_create(&thread, NULL, syn_create_split_off_the_main_thread,
                     NULL) != 0) {
    fprintf(stderr, "could not start the background thread\n");
    return;
  }
  pthread_join(thread, NULL);
}

SYN_TEST(creating_a_split_view_controller_off_the_main_thread_names_it) {
  SYN_ASSERT_ABORTS("split_view_controller_created_off_the_main_thread",
                    "ns_split_view_controller_create");
  SYN_ASSERT_ABORTS("split_view_controller_created_off_the_main_thread",
                    "main thread only");
}

/*
 * The tab view controller suite (U10): the Settings window the Swift twin
 * builds, in C - toolbar-style tabs, SF Symbol images, the selected index and
 * its range check, and the menu callback that shows the window (R19, F1,
 * KTD16).
 *
 * Three of the reads below have no C function yet: a window's toolbar, its
 * toolbar style, and whether it is visible are all ns_window.h's, and
 * NSToolbar and NSToolbarItem are a unit of their own. This suite goes through
 * the Objective-C runtime for those rather than spelling somebody else's
 * wrapper a second time, exactly as tests/support.m does for a delegate or an
 * action.
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

enum { SYN_TAB_COUNT = 2 };

static const char *const syn_labels[SYN_TAB_COUNT] = {"General", "Appearance"};
static const char *const syn_symbols[SYN_TAB_COUNT] = {"gearshape",
                                                       "paintpalette"};

/* ---- the reads the C surface has no function for yet ---- */

static id syn_object(const void *receiver, const char *selector) {
  return ((id (*)(id, SEL))objc_msgSend)((id)(void *)receiver,
                                         sel_getUid(selector));
}

static long syn_integer(const void *receiver, const char *selector) {
  return ((long (*)(id, SEL))objc_msgSend)((id)(void *)receiver,
                                           sel_getUid(selector));
}

static bool syn_flag(const void *receiver, const char *selector) {
  return ((BOOL (*)(id, SEL))objc_msgSend)((id)(void *)receiver,
                                           sel_getUid(selector)) != 0;
}

static id syn_object_at_index(id array, long index) {
  return ((id (*)(id, SEL, long))objc_msgSend)(
      array, sel_getUid("objectAtIndex:"), index);
}

static const char *syn_utf8(id string) {
  return ((const char *(*)(id, SEL))objc_msgSend)(string,
                                                  sel_getUid("UTF8String"));
}

/* ---- the twin's Settings window, in C ---- */

/* The twin's SettingsPaneViewController reduced to what this suite reads: a
 * controller with a view of its own. Owned (+1). */
static ns_view_controller *syn_pane_create(void) {
  ns_view_controller *pane = ns_view_controller_create();
  ns_view *view = ns_view_create_with_frame(CGRectMake(0, 0, 400, 200));
  ns_view_controller_set_view(pane, view);
  ns_release(view);
  return pane;
}

/* The twin's SettingsTabViewController: the toolbar tab style, and one tab per
 * pane carrying a label and an SF Symbol image. Owned (+1). */
static ns_tab_view_controller *syn_tabs_create(void) {
  ns_tab_view_controller *tabs = ns_tab_view_controller_create();
  ns_tab_view_controller_set_tab_style(
      tabs, NS_TAB_VIEW_CONTROLLER_TAB_STYLE_TOOLBAR);
  for (long index = 0; index < SYN_TAB_COUNT; index++) {
    ns_view_controller *pane = syn_pane_create();
    ns_tab_view_item *item = ns_tab_view_item_create_with_view_controller(pane);
    ns_tab_view_item_set_label(item, syn_labels[index]);
    ns_image *image =
        ns_image_create_with_system_symbol_name_accessibility_description(
            syn_symbols[index], syn_labels[index]);
    ns_tab_view_item_set_image(item, image);
    ns_release(image);
    ns_tab_view_controller_add_tab_view_item(tabs, item);
    ns_release(item);
    ns_release(pane);
  }
  return tabs;
}

/* The twin's SettingsWindowController: titled and closable, the tabs as the
 * content view controller, and the preference toolbar style. Owned (+1). */
static ns_window *syn_settings_window_create(ns_tab_view_controller *tabs) {
  ns_window *window =
      ns_window_create_with_content_rect_style_mask_backing_defer(
          CGRectMake(0, 0, 400, 300),
          NS_WINDOW_STYLE_MASK_TITLED | NS_WINDOW_STYLE_MASK_CLOSABLE,
          NS_BACKING_STORE_BUFFERED, false);
  ns_window_set_title(window, "Settings");
  ns_window_set_content_view_controller(
      window, ns_tab_view_controller_as_view_controller(tabs));
  ns_window_set_toolbar_style(window, NS_WINDOW_TOOLBAR_STYLE_PREFERENCE);
  return window;
}

/* ---- the toolbar the tab style produces (R19) ---- */

SYN_TEST(two_tabs_in_a_window_yield_a_toolbar_carrying_the_tab_labels) {
  syn_test_bootstrap();

  ns_tab_view_controller *tabs = syn_tabs_create();
  SYN_ASSERT_STR_EQ(syn_test_class_name(tabs), "NSTabViewController");
  SYN_ASSERT_MSG(ns_tab_view_controller_get_tab_style(tabs) ==
                     NS_TAB_VIEW_CONTROLLER_TAB_STYLE_TOOLBAR,
                 "the tab style did not read back as the toolbar style");
  SYN_ASSERT_MSG(ns_tab_view_controller_tab_view_item_count(tabs) ==
                     SYN_TAB_COUNT,
                 "the controller holds %ld tabs, not %d",
                 ns_tab_view_controller_tab_view_item_count(tabs),
                 SYN_TAB_COUNT);

  for (long index = 0; index < SYN_TAB_COUNT; index++) {
    ns_tab_view_item *item =
        ns_tab_view_controller_tab_view_item_at_index(tabs, index);
    SYN_ASSERT_STR_EQ(syn_test_class_name(item), "NSTabViewItem");
    char *label = ns_tab_view_item_copy_label(item);
    SYN_ASSERT_STR_EQ(label, syn_labels[index]);
    ns_string_free(label);
    SYN_ASSERT_MSG(ns_tab_view_item_image(item) != NULL,
                   "tab %ld carries no image", index);
    SYN_ASSERT_MSG(ns_tab_view_item_view_controller(item) != NULL,
                   "tab %ld carries no view controller", index);
  }

  /* The tab view controller owns the toolbar: it creates one, makes itself its
   * delegate, and hands it to the window it lands in. Nothing in this test
   * builds a toolbar. */
  ns_window *window = syn_settings_window_create(tabs);
  id toolbar = syn_object(window, "toolbar");
  SYN_ASSERT_MSG(toolbar != NULL,
                 "the window has no toolbar; the toolbar tab style is what "
                 "puts one there (R19)");

  id items = syn_object((const void *)toolbar, "items");
  SYN_ASSERT_MSG(syn_integer((const void *)items, "count") == SYN_TAB_COUNT,
                 "the toolbar has %ld items, not one per tab",
                 syn_integer((const void *)items, "count"));
  for (long index = 0; index < SYN_TAB_COUNT; index++) {
    id item = syn_object_at_index(items, index);
    SYN_ASSERT_STR_EQ(syn_utf8(syn_object((const void *)item, "label")),
                      syn_labels[index]);
  }

  ns_window_close(window);
  ns_release(window);
  ns_release(tabs);
}

SYN_TEST(the_settings_window_reports_the_preference_toolbar_style) {
  syn_test_bootstrap();
  ns_tab_view_controller *tabs = syn_tabs_create();
  ns_window *window = syn_settings_window_create(tabs);

  SYN_ASSERT_MSG(syn_integer(window, "toolbarStyle") ==
                     NS_WINDOW_TOOLBAR_STYLE_PREFERENCE,
                 "the window's toolbar style reads %ld, not the preference "
                 "style; without it the toolbar tabs draw with the wrong "
                 "metrics (R19)",
                 syn_integer(window, "toolbarStyle"));

  ns_window_close(window);
  ns_release(window);
  ns_release(tabs);
}

/* ---- the selected index (R19, R12) ---- */

SYN_TEST(selecting_the_second_tab_shows_its_view) {
  syn_test_bootstrap();
  ns_tab_view_controller *tabs = syn_tabs_create();
  ns_window *window = syn_settings_window_create(tabs);

  ns_view *first = ns_view_controller_view(ns_tab_view_item_view_controller(
      ns_tab_view_controller_tab_view_item_at_index(tabs, 0)));
  ns_view *second = ns_view_controller_view(ns_tab_view_item_view_controller(
      ns_tab_view_controller_tab_view_item_at_index(tabs, 1)));

  SYN_ASSERT_MSG(ns_tab_view_controller_selected_tab_view_item_index(tabs) == 0,
                 "a fresh tab view controller does not start on the first tab");
  SYN_WAIT_FOR(ns_view_superview(first) != NULL, 1000);

  ns_tab_view_controller_set_selected_tab_view_item_index(tabs, 1);
  SYN_ASSERT_MSG(ns_tab_view_controller_selected_tab_view_item_index(tabs) == 1,
                 "the selected index did not read back as 1");
  /* The default transition is a crossfade, so the swap finishes on a later
   * turn of the run loop rather than inside the setter. */
  SYN_WAIT_FOR(ns_view_superview(second) != NULL, 1000);
  SYN_WAIT_FOR(ns_view_superview(first) == NULL, 1000);

  ns_window_close(window);
  ns_release(window);
  ns_release(tabs);
}

SYN_ABORT_CASE(index_equal_to_the_tab_count_selected) {
  syn_test_bootstrap();
  ns_tab_view_controller *tabs = syn_tabs_create();
  ns_tab_view_controller_set_selected_tab_view_item_index(tabs, SYN_TAB_COUNT);
}

SYN_TEST(selecting_the_index_equal_to_the_tab_count_names_the_function) {
  SYN_ASSERT_ABORTS("index_equal_to_the_tab_count_selected",
                    "ns_tab_view_controller_set_selected_tab_view_item_index");
  SYN_ASSERT_ABORTS("index_equal_to_the_tab_count_selected",
                    "index 2 is out of range");
}

SYN_ABORT_CASE(tab_view_item_read_past_the_end) {
  syn_test_bootstrap();
  ns_tab_view_controller *tabs = syn_tabs_create();
  ns_tab_view_controller_tab_view_item_at_index(tabs, SYN_TAB_COUNT);
}

/* The accessor writes no check of its own: NSArray raises before it changes
 * anything, and the entry macro's exception report names the function (KTD4).
 * The setter above is the other half - there AppKit is not clean. */
SYN_TEST(reading_a_tab_past_the_end_names_the_function_and_the_exception) {
  SYN_ASSERT_ABORTS("tab_view_item_read_past_the_end",
                    "ns_tab_view_controller_tab_view_item_at_index");
  SYN_ASSERT_ABORTS("tab_view_item_read_past_the_end", "AppKit raised");
}

/* ---- the upcast (R4, KTD9) ---- */

SYN_TEST(the_upcast_to_a_view_controller_is_the_same_pointer) {
  syn_test_bootstrap();
  ns_tab_view_controller *tabs = ns_tab_view_controller_create();
  SYN_ASSERT_MSG((const void *)ns_tab_view_controller_as_view_controller(
                     tabs) == (const void *)tabs,
                 "the upcast returned a different pointer");
  ns_release(tabs);
}

/* ---- the Settings menu item's callback (R19, KTD16) ---- */

static ns_window *syn_settings_window;
static ns_tab_view_controller *syn_settings_tabs;
static int syn_settings_opens;

/* What an application does behind the Settings item: build the window the
 * first time it is asked for, then show it. */
static void syn_on_settings(void *context, const void *sender) {
  (void)context;
  (void)sender;
  syn_settings_opens++;
  if (syn_settings_window == NULL) {
    syn_settings_tabs = syn_tabs_create();
    syn_settings_window = syn_settings_window_create(syn_settings_tabs);
  }
  ns_window_make_key_and_order_front(syn_settings_window);
}

static long syn_index_of(ns_menu *menu, const char *title) {
  for (long index = 0; index < ns_menu_number_of_items(menu); index++) {
    char *found = ns_menu_item_copy_title(ns_menu_item_at_index(menu, index));
    bool hit = found != NULL && strcmp(found, title) == 0;
    ns_string_free(found);
    if (hit) return index;
  }
  return -1;
}

SYN_TEST(the_settings_callback_shows_the_window_it_creates_lazily) {
  syn_test_bootstrap();
  ns_menu *bar = ns_menu_bar_install_standard(ns_application_shared(),
                                              SYN_APP_NAME, syn_on_settings,
                                              NULL);
  long app_index = syn_index_of(bar, SYN_APP_NAME);
  SYN_ASSERT_MSG(app_index >= 0, "there is no application menu");
  ns_menu *app_menu = ns_menu_item_submenu(ns_menu_item_at_index(bar,
                                                                 app_index));
  long settings = syn_index_of(app_menu, "Settings…");
  SYN_ASSERT_MSG(settings >= 0, "the application menu has no Settings item");

  SYN_ASSERT_MSG(syn_settings_window == NULL,
                 "the settings window exists before anything asked for it");
  ns_menu_perform_action_for_item_at_index(app_menu, settings);
  SYN_WAIT_FOR(syn_settings_opens == 1, 1000);
  SYN_ASSERT_MSG(syn_settings_window != NULL,
                 "the callback did not create the settings window");

  /* Visible, and nothing on screen: the test application's activation policy
   * is prohibited, so ordering a window front shows it to nobody. */
  SYN_ASSERT_MSG(syn_test_activation_policy_is_prohibited(),
                 "the test application is not off-screen");
  SYN_ASSERT_MSG(syn_flag(syn_settings_window, "isVisible"),
                 "the settings window is not visible after the callback ran");

  /* Asking a second time shows the window that already exists. */
  ns_menu_perform_action_for_item_at_index(app_menu, settings);
  SYN_WAIT_FOR(syn_settings_opens == 2, 1000);

  ns_window_close(syn_settings_window);
  SYN_ASSERT_MSG(!syn_flag(syn_settings_window, "isVisible"),
                 "close did not order the settings window out");
  ns_release(syn_settings_window);
  ns_release(syn_settings_tabs);
  syn_settings_window = NULL;
  syn_settings_tabs = NULL;
}

/* The whole tree, built and torn down with nothing else in the picture, is
 * what the leak gate reads (F3). */
SYN_TEST(tearing_the_settings_window_down_releases_clean) {
  syn_test_bootstrap();
  ns_tab_view_controller *tabs = syn_tabs_create();
  ns_window *window = syn_settings_window_create(tabs);
  ns_window_make_key_and_order_front(window);
  ns_tab_view_controller_set_selected_tab_view_item_index(tabs, 1);
  syn_test_spin(50);
  ns_window_close(window);
  ns_release(window);
  ns_release(tabs);
}

/* ---- the SF Symbol image (R23) ---- */

SYN_TEST(an_sf_symbol_image_round_trips_its_accessibility_description) {
  syn_test_bootstrap();
  ns_image *image =
      ns_image_create_with_system_symbol_name_accessibility_description(
          "gearshape", "General");
  SYN_ASSERT_MSG(image != NULL, "the gearshape symbol produced no image");

  char *description = ns_image_copy_accessibility_description(image);
  SYN_ASSERT_STR_EQ(description, "General");
  ns_string_free(description);

  /* A null description does not leave the image undescribed: AppKit puts the
   * symbol's own localized name there, which is why a caller who wants a
   * control read as something else has to say so. The name is localized, so
   * this pins that there is one rather than what it says. */
  ns_image *bare =
      ns_image_create_with_system_symbol_name_accessibility_description(
          "gearshape", NULL);
  SYN_ASSERT_MSG(bare != NULL, "a symbol with no description produced nothing");
  char *fallback = ns_image_copy_accessibility_description(bare);
  SYN_ASSERT_MSG(fallback != NULL && fallback[0] != '\0',
                 "AppKit no longer names an undescribed symbol image");
  ns_string_free(fallback);

  ns_release(bare);
  ns_release(image);
}

/* AppKit answers nil for a name the running system has no symbol for. That is
 * documented behaviour and an answer the caller handles, not misuse, so the
 * wrapper passes the null through rather than aborting (R12). */
SYN_TEST(an_unknown_sf_symbol_name_comes_back_null) {
  syn_test_bootstrap();
  ns_image *missing =
      ns_image_create_with_system_symbol_name_accessibility_description(
          "not.a.real.symbol.xyz", "nothing");
  SYN_ASSERT_MSG(missing == NULL,
                 "an unknown symbol name produced an image after all");
  ns_release(missing); /* releasing null is a no-op (R7) */
}

/* ---- off the main thread (R10) ---- */

static void *syn_create_tabs_off_the_main_thread(void *ignored) {
  (void)ignored;
  ns_tab_view_controller_create();
  return NULL;
}

SYN_ABORT_CASE(tab_view_controller_created_off_the_main_thread) {
  syn_test_bootstrap();
  pthread_t thread;
  if (pthread_create(&thread, NULL, syn_create_tabs_off_the_main_thread,
                     NULL) != 0) {
    fprintf(stderr, "could not start the background thread\n");
    return;
  }
  pthread_join(thread, NULL);
}

SYN_TEST(creating_a_tab_view_controller_off_the_main_thread_names_it) {
  SYN_ASSERT_ABORTS("tab_view_controller_created_off_the_main_thread",
                    "ns_tab_view_controller_create");
  SYN_ASSERT_ABORTS("tab_view_controller_created_off_the_main_thread",
                    "main thread only");
}

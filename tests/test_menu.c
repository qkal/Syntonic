/*
 * The menu suite (U6): menus, menu items, the boundary array shape, the
 * target/action installer, the index check, and the standard menu bar -
 * including AE5, where Edit's Copy is `copy:` with no target and therefore no
 * C code behind it (R7, R12, R18, R19, KTD16, KTD17).
 */

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "runner.h"
#include "support.h"
#include "syntonic/syntonic.h"

#define SYN_APP_NAME "Syntonic Twin"

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

static ns_menu_item *syn_item(const char *title, const char *key) {
  return ns_menu_item_create_with_title_key_equivalent(title, key);
}

/* Adds an item and gives the caller's reference back, the way every caller
 * that builds a menu does (R7). */
static void syn_add(ns_menu *menu, ns_menu_item *item) {
  ns_menu_add_item(menu, item);
  ns_release(item);
}

/* The index of the first item with this title, or -1. AppKit injects items of
 * its own into a menu titled "Edit" - AutoFill, Start Dictation, Emoji &
 * Symbols - so a fixed index would be a false failure. */
static long syn_index_of(ns_menu *menu, const char *title) {
  long count = ns_menu_number_of_items(menu);
  for (long index = 0; index < count; index++) {
    char *found = ns_menu_item_copy_title(ns_menu_item_at_index(menu, index));
    bool matched = found != NULL && strcmp(found, title) == 0;
    ns_string_free(found);
    if (matched) return index;
  }
  return -1;
}

static ns_menu_item *syn_find(ns_menu *menu, const char *title) {
  long index = syn_index_of(menu, title);
  return index < 0 ? NULL : ns_menu_item_at_index(menu, index);
}

/* The submenu behind the top-level item with this title. */
static ns_menu *syn_top_level(ns_menu *bar, const char *title) {
  ns_menu_item *holder = syn_find(bar, title);
  return holder == NULL ? NULL : ns_menu_item_submenu(holder);
}

/* ---- the class, the title and the item properties (R5, R7, R11) ---- */

SYN_TEST(a_menu_and_an_item_report_their_titles_through_owned_copies) {
  syn_test_bootstrap();
  ns_menu *menu = ns_menu_create_with_title("File");
  SYN_ASSERT_STR_EQ(syn_test_class_name(menu), "NSMenu");

  char *first = ns_menu_copy_title(menu);
  char *second = ns_menu_copy_title(menu);
  SYN_ASSERT_STR_EQ(first, "File");
  SYN_ASSERT_MSG(first != second, "two copies of the title came back as one");
  ns_string_free(first);
  SYN_ASSERT_STR_EQ(second, "File"); /* the other copy is untouched */
  ns_string_free(second);

  ns_menu_set_title(menu, "Archive");
  char *renamed = ns_menu_copy_title(menu);
  SYN_ASSERT_STR_EQ(renamed, "Archive");
  ns_string_free(renamed);

  ns_menu_item *item = syn_item("New Item", "n");
  SYN_ASSERT_STR_EQ(syn_test_class_name(item), "NSMenuItem");
  ns_menu_item_set_title(item, "New Thing");
  char *item_title = ns_menu_item_copy_title(item);
  SYN_ASSERT_STR_EQ(item_title, "New Thing");
  ns_string_free(item_title);

  ns_release(item);
  ns_release(menu);
}

SYN_TEST(an_item_keeps_its_key_equivalent_and_its_modifiers) {
  syn_test_bootstrap();
  ns_menu_item *item = syn_item("Redo", "z");

  char *key = ns_menu_item_copy_key_equivalent(item);
  SYN_ASSERT_STR_EQ(key, "z");
  ns_string_free(key);

  /* AppKit's own default for a new item, which is why the standard menu bar
   * sets a mask only where a second modifier is wanted. */
  SYN_ASSERT_MSG(ns_menu_item_key_equivalent_modifier_mask(item) ==
                     NS_EVENT_MODIFIER_FLAG_COMMAND,
                 "a new item did not start on Command alone");

  ns_menu_item_set_key_equivalent_modifier_mask(
      item, NS_EVENT_MODIFIER_FLAG_COMMAND | NS_EVENT_MODIFIER_FLAG_SHIFT);
  ns_event_modifier_flags mask = ns_menu_item_key_equivalent_modifier_mask(item);
  SYN_ASSERT_MSG((mask & NS_EVENT_MODIFIER_FLAG_COMMAND) != 0 &&
                     (mask & NS_EVENT_MODIFIER_FLAG_SHIFT) != 0,
                 "the modifier mask read back as 0x%lx", (unsigned long)mask);

  ns_menu_item_set_key_equivalent(item, "");
  char *cleared = ns_menu_item_copy_key_equivalent(item);
  SYN_ASSERT_STR_EQ(cleared, "");
  ns_string_free(cleared);

  ns_release(item);
}

SYN_TEST(a_separator_is_a_separator_and_an_ordinary_item_is_not) {
  syn_test_bootstrap();
  ns_menu_item *separator = ns_menu_item_create_separator_item();
  ns_menu_item *other = ns_menu_item_create_separator_item();
  SYN_ASSERT_MSG(separator != other,
                 "two separators came back as the same object, so one menu "
                 "would steal the other's");
  SYN_ASSERT(ns_menu_item_separator_item(separator));

  ns_menu_item *ordinary = syn_item("Close", "w");
  SYN_ASSERT(!ns_menu_item_separator_item(ordinary));

  ns_release(ordinary);
  ns_release(other);
  ns_release(separator);
}

/* While a menu autoenables, AppKit computes the flag and setEnabled: is
 * ignored; the pair is what makes the flag the caller's (R18). */
SYN_TEST(enabled_is_the_callers_once_the_menu_stops_autoenabling) {
  syn_test_bootstrap();
  ns_menu *menu = ns_menu_create_with_title("File");
  ns_menu_set_autoenables_items(menu, false);

  ns_menu_item *item = syn_item("Close", "w");
  SYN_ASSERT_MSG(ns_menu_item_enabled(item), "a new item started disabled");
  ns_menu_item_set_enabled(item, false);
  SYN_ASSERT(!ns_menu_item_enabled(item));
  ns_menu_item_set_enabled(item, true);
  SYN_ASSERT(ns_menu_item_enabled(item));

  syn_add(menu, item);
  ns_release(menu);
}

/* ---- the array shape and the submenu getter (R7, KTD7, KTD17) ---- */

SYN_TEST(the_item_count_and_the_index_accessor_follow_every_insertion) {
  syn_test_bootstrap();
  ns_menu *menu = ns_menu_create_with_title("File");
  SYN_ASSERT_MSG(ns_menu_number_of_items(menu) == 0,
                 "a new menu was not empty");

  syn_add(menu, syn_item("Close", "w"));
  SYN_ASSERT(ns_menu_number_of_items(menu) == 1);

  /* index 0 puts it in front, the count appends, and the middle is the middle:
   * the three positions insert has. */
  ns_menu_item *first = syn_item("New Item", "n");
  ns_menu_insert_item_at_index(menu, first, 0);
  ns_release(first);
  ns_menu_item *last = syn_item("Save", "s");
  ns_menu_insert_item_at_index(menu, last, ns_menu_number_of_items(menu));
  ns_release(last);
  ns_menu_item *middle = ns_menu_item_create_separator_item();
  ns_menu_insert_item_at_index(menu, middle, 1);
  ns_release(middle);

  SYN_ASSERT_MSG(ns_menu_number_of_items(menu) == 4,
                 "the count did not follow the insertions: %ld",
                 ns_menu_number_of_items(menu));

  const char *expected[] = {"New Item", "", "Close", "Save"};
  for (long index = 0; index < 4; index++) {
    char *title = ns_menu_item_copy_title(ns_menu_item_at_index(menu, index));
    SYN_ASSERT_STR_EQ(title, expected[index]);
    ns_string_free(title);
  }
  SYN_ASSERT(ns_menu_item_separator_item(ns_menu_item_at_index(menu, 1)));

  ns_release(menu);
}

/* The element the index accessor hands back is borrowed: the menu holds it,
 * and ns_retain is what keeps it past the menu (R7, the borrowed-return
 * allowlist). */
SYN_TEST(an_item_read_back_from_a_menu_is_borrowed_and_outlives_it_when_retained) {
  syn_test_bootstrap();
  ns_menu *menu = ns_menu_create_with_title("File");
  syn_add(menu, syn_item("Close", "w"));

  ns_menu_item *borrowed = ns_menu_item_at_index(menu, 0);
  ns_retain(borrowed);
  ns_release(menu); /* the menu goes; the retained item does not */

  char *title = ns_menu_item_copy_title(borrowed);
  SYN_ASSERT_STR_EQ(title, "Close");
  ns_string_free(title);
  ns_release(borrowed);
}

SYN_TEST(a_submenu_is_borrowed_from_the_item_that_holds_it) {
  syn_test_bootstrap();
  ns_menu_item *holder = syn_item("File", "");
  SYN_ASSERT_MSG(ns_menu_item_submenu(holder) == NULL,
                 "a new item already had a submenu");

  ns_menu *menu = ns_menu_create_with_title("File");
  ns_menu_item_set_submenu(holder, menu);
  ns_release(menu); /* the item retains it; the caller's reference goes */

  ns_menu *borrowed = ns_menu_item_submenu(holder);
  SYN_ASSERT_MSG(borrowed == menu, "the submenu came back as another object");
  char *title = ns_menu_copy_title(borrowed);
  SYN_ASSERT_STR_EQ(title, "File");
  ns_string_free(title);

  ns_menu_item_set_submenu(holder, NULL);
  SYN_ASSERT_MSG(ns_menu_item_submenu(holder) == NULL,
                 "a null submenu did not clear the slot");
  ns_release(holder);
}

/* A menu with items, submenus and an installed action, torn down by its own
 * release: what the leak gate reads (R7, KTD12). */
SYN_TEST(tearing_down_a_menu_releases_clean) {
  syn_test_bootstrap();
  syn_reset();
  int context = 11;

  ns_menu *menu = ns_menu_create_with_title("File");
  ns_menu_item *item = syn_item("New Item", "n");
  ns_menu_item_set_action(item, syn_on_action, &context);
  syn_add(menu, item);
  syn_add(menu, ns_menu_item_create_separator_item());

  ns_menu_item *holder = syn_item("More", "");
  ns_menu *submenu = ns_menu_create_with_title("More");
  syn_add(submenu, syn_item("Deeper", ""));
  ns_menu_item_set_submenu(holder, submenu);
  ns_release(submenu);
  syn_add(menu, holder);

  ns_release(menu);
}

/* ---- the target/action installer (R9) ---- */

SYN_TEST(an_app_defined_item_fires_its_callback_with_the_context_and_the_item) {
  syn_test_bootstrap();
  syn_reset();
  int context = 42;

  ns_menu *menu = ns_menu_create_with_title("File");
  ns_menu_item *item = syn_item("New Item", "n");
  ns_menu_item_set_action(item, syn_on_action, &context);
  syn_add(menu, item);

  ns_menu_perform_action_for_item_at_index(menu, 0);
  SYN_WAIT_FOR(syn_fires == 1, 1000);
  SYN_ASSERT_MSG(syn_context_seen == &context, "the context did not arrive");
  SYN_ASSERT_MSG(syn_sender_seen == item,
                 "the sender was not the item that fired");
  SYN_ASSERT_STR_EQ(syn_test_class_name(syn_sender_seen), "NSMenuItem");

  /* Installing again replaces; a null action uninstalls and leaves the item
   * with no action at all. */
  ns_menu_item_set_action(item, NULL, NULL);
  SYN_ASSERT_STR_EQ(syn_test_action_name(item), "");
  ns_menu_perform_action_for_item_at_index(menu, 0);
  syn_test_spin(50);
  SYN_ASSERT_MSG(syn_fires == 1, "an uninstalled action fired");

  ns_release(menu);
}

/* ---- the standard menu bar (R18, R19, KTD16) ---- */

static ns_menu *syn_install_bar(void *settings_context) {
  return ns_menu_bar_install_standard(ns_application_shared(), SYN_APP_NAME,
                                      syn_on_action, settings_context);
}

/* Covers AE5: the six menus in order, and Edit's Copy carrying `copy:` with no
 * target, which is what makes Command-C work with no C code behind it. */
SYN_TEST(the_standard_menu_bar_has_six_menus_and_a_copy_item_on_the_chain) {
  syn_test_bootstrap();
  syn_reset();
  ns_menu *bar = syn_install_bar(NULL);

  const char *order[] = {SYN_APP_NAME, "File", "Edit",
                         "View",       "Window", "Help"};
  SYN_ASSERT_MSG(ns_menu_number_of_items(bar) == 6,
                 "the menu bar has %ld top-level menus, not 6",
                 ns_menu_number_of_items(bar));
  for (long index = 0; index < 6; index++) {
    ns_menu_item *holder = ns_menu_item_at_index(bar, index);
    char *title = ns_menu_item_copy_title(holder);
    SYN_ASSERT_STR_EQ(title, order[index]);
    ns_string_free(title);
    SYN_ASSERT_MSG(ns_menu_item_submenu(holder) != NULL,
                   "top-level menu %ld holds no submenu", index);
  }

  ns_menu *edit = syn_top_level(bar, "Edit");
  SYN_ASSERT_MSG(edit != NULL, "there is no Edit menu");
  ns_menu_item *copy = syn_find(edit, "Copy");
  SYN_ASSERT_MSG(copy != NULL, "the Edit menu has no Copy item");
  SYN_ASSERT_STR_EQ(syn_test_action_name(copy), "copy:");
  SYN_ASSERT_MSG(!syn_test_has_target(copy),
                 "Copy has a target, so it would not reach the responder "
                 "chain (AE5)");
  char *key = ns_menu_item_copy_key_equivalent(copy);
  SYN_ASSERT_STR_EQ(key, "c");
  ns_string_free(key);
  SYN_ASSERT_MSG(ns_menu_item_key_equivalent_modifier_mask(copy) ==
                     NS_EVENT_MODIFIER_FLAG_COMMAND,
                 "Copy is not on Command-c alone");

  /* Every other standard item is on the chain too, by the same test. */
  const char *chain[][2] = {
      {"Cut", "cut:"},          {"Paste", "paste:"},
      {"Delete", "delete:"},    {"Select All", "selectAll:"},
      {"Undo", "undo:"},        {"Redo", "redo:"},
  };
  for (size_t i = 0; i < sizeof chain / sizeof *chain; i++) {
    ns_menu_item *item = syn_find(edit, chain[i][0]);
    SYN_ASSERT_MSG(item != NULL, "the Edit menu has no %s item", chain[i][0]);
    SYN_ASSERT_STR_EQ(syn_test_action_name(item), chain[i][1]);
    SYN_ASSERT_MSG(!syn_test_has_target(item), "%s has a target", chain[i][0]);
  }
}

SYN_TEST(the_standard_menus_carry_the_selectors_and_shortcuts_they_should) {
  syn_test_bootstrap();
  syn_reset();
  ns_menu *bar = syn_install_bar(NULL);

  struct {
    const char *menu;
    const char *title;
    const char *selector;
    const char *key;
    unsigned long modifiers;
  } expected[] = {
      {SYN_APP_NAME, "About " SYN_APP_NAME, "orderFrontStandardAboutPanel:",
       "", NS_EVENT_MODIFIER_FLAG_COMMAND},
      {SYN_APP_NAME, "Hide " SYN_APP_NAME, "hide:", "h",
       NS_EVENT_MODIFIER_FLAG_COMMAND},
      {SYN_APP_NAME, "Hide Others", "hideOtherApplications:", "h",
       NS_EVENT_MODIFIER_FLAG_COMMAND | NS_EVENT_MODIFIER_FLAG_OPTION},
      {SYN_APP_NAME, "Show All", "unhideAllApplications:", "",
       NS_EVENT_MODIFIER_FLAG_COMMAND},
      {SYN_APP_NAME, "Quit " SYN_APP_NAME, "terminate:", "q",
       NS_EVENT_MODIFIER_FLAG_COMMAND},
      {"File", "Close", "performClose:", "w", NS_EVENT_MODIFIER_FLAG_COMMAND},
      {"View", "Toggle Sidebar", "toggleSidebar:", "s",
       NS_EVENT_MODIFIER_FLAG_COMMAND | NS_EVENT_MODIFIER_FLAG_CONTROL},
      {"Window", "Minimize", "performMiniaturize:", "m",
       NS_EVENT_MODIFIER_FLAG_COMMAND},
      {"Window", "Zoom", "performZoom:", "", NS_EVENT_MODIFIER_FLAG_COMMAND},
      {"Window", "Bring All to Front", "arrangeInFront:", "",
       NS_EVENT_MODIFIER_FLAG_COMMAND},
      {"Help", SYN_APP_NAME " Help", "showHelp:", "?",
       NS_EVENT_MODIFIER_FLAG_COMMAND},
  };

  for (size_t i = 0; i < sizeof expected / sizeof *expected; i++) {
    ns_menu *menu = syn_top_level(bar, expected[i].menu);
    SYN_ASSERT_MSG(menu != NULL, "there is no %s menu", expected[i].menu);
    ns_menu_item *item = syn_find(menu, expected[i].title);
    SYN_ASSERT_MSG(item != NULL, "%s has no \"%s\" item", expected[i].menu,
                   expected[i].title);
    SYN_ASSERT_STR_EQ(syn_test_action_name(item), expected[i].selector);
    SYN_ASSERT_MSG(!syn_test_has_target(item),
                   "\"%s\" has a target, so it is not on the responder chain",
                   expected[i].title);
    char *key = ns_menu_item_copy_key_equivalent(item);
    SYN_ASSERT_STR_EQ(key, expected[i].key);
    ns_string_free(key);
    SYN_ASSERT_MSG((unsigned long)ns_menu_item_key_equivalent_modifier_mask(
                       item) == expected[i].modifiers,
                   "\"%s\" carries modifiers 0x%lx, not 0x%lx",
                   expected[i].title,
                   (unsigned long)ns_menu_item_key_equivalent_modifier_mask(
                       item),
                   expected[i].modifiers);
  }

  /* Enter Full Screen is the one item whose shortcut is not the builder's to
   * keep: the builder asks for Control-Command-f, as the Swift twin does, and
   * once the app has finished launching AppKit swaps in the system's own Full
   * Screen shortcut - Globe-f, the Function modifier alone. Both twins get the
   * same swap, so what is pinned here is the selector and the empty target. */
  ns_menu_item *full_screen =
      syn_find(syn_top_level(bar, "View"), "Enter Full Screen");
  SYN_ASSERT_MSG(full_screen != NULL, "the View menu has no Full Screen item");
  SYN_ASSERT_STR_EQ(syn_test_action_name(full_screen), "toggleFullScreen:");
  SYN_ASSERT_MSG(!syn_test_has_target(full_screen),
                 "Enter Full Screen has a target");
  char *full_screen_key = ns_menu_item_copy_key_equivalent(full_screen);
  SYN_ASSERT_STR_EQ(full_screen_key, "f");
  ns_string_free(full_screen_key);

  /* The separators, where the Swift twin has them. */
  ns_menu *app_menu = syn_top_level(bar, SYN_APP_NAME);
  SYN_ASSERT_MSG(ns_menu_item_separator_item(ns_menu_item_at_index(app_menu, 1)),
                 "the App menu has no separator under About");
  ns_menu *view = syn_top_level(bar, "View");
  SYN_ASSERT_MSG(ns_menu_item_separator_item(ns_menu_item_at_index(view, 1)),
                 "the View menu has no separator under Toggle Sidebar");

  /* Services is a submenu with no action of its own; AppKit fills it. */
  ns_menu_item *services = syn_find(app_menu, "Services");
  SYN_ASSERT_MSG(services != NULL, "the App menu has no Services item");
  SYN_ASSERT_MSG(ns_menu_item_submenu(services) != NULL,
                 "the Services item holds no submenu");
}

SYN_TEST(the_settings_item_is_command_comma_and_fires_the_c_callback) {
  syn_test_bootstrap();
  syn_reset();
  int context = 19;
  ns_menu *bar = syn_install_bar(&context);

  ns_menu *app_menu = syn_top_level(bar, SYN_APP_NAME);
  SYN_ASSERT_MSG(app_menu != NULL, "there is no application menu");
  long index = syn_index_of(app_menu, "Settings…");
  SYN_ASSERT_MSG(index >= 0,
                 "the application menu has no \"Settings…\" item; AppKit's own "
                 "rename is private and does not count (KTD16)");
  ns_menu_item *settings = ns_menu_item_at_index(app_menu, index);

  char *key = ns_menu_item_copy_key_equivalent(settings);
  SYN_ASSERT_STR_EQ(key, ",");
  ns_string_free(key);
  SYN_ASSERT_MSG(ns_menu_item_key_equivalent_modifier_mask(settings) ==
                     NS_EVENT_MODIFIER_FLAG_COMMAND,
                 "Settings is not on Command-comma");
  SYN_ASSERT_MSG(syn_test_has_target(settings),
                 "Settings has no target, so AppKit has no action for it "
                 "and nothing would happen (KTD16)");

  ns_menu_perform_action_for_item_at_index(app_menu, index);
  SYN_WAIT_FOR(syn_fires == 1, 1000);
  SYN_ASSERT_MSG(syn_context_seen == &context,
                 "the settings context did not arrive");
  SYN_ASSERT_MSG(syn_sender_seen == settings,
                 "the sender was not the Settings item");
}

/* ---- the cases that abort, each run in a child (R10, R12, KTD4) ---- */

SYN_ABORT_CASE(item_inserted_past_the_end) {
  syn_test_bootstrap();
  ns_menu *menu = ns_menu_create_with_title("File");
  ns_menu_add_item(menu, syn_item("Close", "w"));
  ns_menu_insert_item_at_index(menu, syn_item("New Item", "n"), 7);
}

SYN_ABORT_CASE(item_inserted_at_a_negative_index) {
  syn_test_bootstrap();
  ns_menu *menu = ns_menu_create_with_title("File");
  ns_menu_insert_item_at_index(menu, syn_item("New Item", "n"), -1);
}

SYN_TEST(inserting_outside_the_range_names_the_function_and_the_range) {
  SYN_ASSERT_ABORTS("item_inserted_past_the_end",
                    "ns_menu_insert_item_at_index");
  SYN_ASSERT_ABORTS("item_inserted_past_the_end", "index 7 is out of range");
  SYN_ASSERT_ABORTS("item_inserted_past_the_end", "accepts 0 through 1");
  SYN_ASSERT_ABORTS("item_inserted_at_a_negative_index",
                    "index -1 is out of range");
}

/* The index accessor needs no check of its own: AppKit raises on its own and
 * the entry macro reports that exception with this function's name (KTD4). */
SYN_ABORT_CASE(item_read_past_the_end) {
  syn_test_bootstrap();
  ns_menu *menu = ns_menu_create_with_title("File");
  ns_menu_item_at_index(menu, 3);
}

SYN_TEST(reading_past_the_end_is_reported_as_the_appkit_exception_it_is) {
  SYN_ASSERT_ABORTS("item_read_past_the_end", "ns_menu_item_at_index");
  SYN_ASSERT_ABORTS("item_read_past_the_end", "AppKit raised");
}

static void *syn_create_a_menu_off_the_main_thread(void *unused) {
  (void)unused;
  ns_menu_create_with_title("File");
  return NULL;
}

SYN_ABORT_CASE(menu_created_off_the_main_thread) {
  syn_test_bootstrap();
  pthread_t thread;
  if (pthread_create(&thread, NULL, syn_create_a_menu_off_the_main_thread,
                     NULL) != 0) {
    fprintf(stderr, "could not start the background thread\n");
    return;
  }
  pthread_join(thread, NULL);
}

SYN_TEST(creating_a_menu_off_the_main_thread_names_the_function) {
  SYN_ASSERT_ABORTS("menu_created_off_the_main_thread",
                    "ns_menu_create_with_title");
  SYN_ASSERT_ABORTS("menu_created_off_the_main_thread", "main thread only");
}

SYN_ABORT_CASE(a_window_passed_where_a_menu_belongs) {
  syn_test_bootstrap();
  ns_window *window = ns_window_create_with_content_rect_style_mask_backing_defer(
      CGRectMake(0, 0, 100, 100), NS_WINDOW_STYLE_MASK_TITLED,
      NS_BACKING_STORE_BUFFERED, false);
  ns_menu_number_of_items((ns_menu *)(void *)window);
}

SYN_TEST(a_handle_of_the_wrong_class_names_both_classes) {
  SYN_ASSERT_ABORTS("a_window_passed_where_a_menu_belongs", "NSMenu");
  SYN_ASSERT_ABORTS("a_window_passed_where_a_menu_belongs",
                    "ns_menu_number_of_items");
}

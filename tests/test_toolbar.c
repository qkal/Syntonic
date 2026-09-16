/*
 * The toolbar suite (U9): the main window's toolbar with a search field and an
 * Add button, the one required callbacks member, the identifier lists the
 * wrapper answers from stored data, and the ownership of the item a callback
 * returns (R9, R16, R23, F2, KTD8, KTD17).
 *
 * Three things this suite reaches through the Objective-C runtime rather than
 * spelling somebody else's wrapper a second time, exactly as
 * tests/test_tab_view_controller.c does: AppKit's own identifier constants,
 * whose values the exported C ones have to equal; the toolbar's delegate, so
 * the one member can be asked directly rather than only through a window; and
 * an item's target and action, because NSToolbarItem has no performClick: of
 * its own on macOS 27 - the Add item is fired the way AppKit fires it, by
 * sending the action to the target.
 */

#include <objc/message.h>
#include <objc/runtime.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "runner.h"
#include "support.h"
#include "syntonic/syntonic.h"

#define SYN_TOOLBAR_ID "dev.kaino.syntonic.twin.toolbar"
#define SYN_ADD_ID "dev.kaino.syntonic.twin.add"
#define SYN_SEARCH_ID "dev.kaino.syntonic.twin.search"
#define SYN_UNKNOWN_ID "dev.kaino.syntonic.twin.nothing"

enum { SYN_DEFAULT_COUNT = 5, SYN_MAX_ASKED = 16 };

/* The twin's toolbarDefaultItemIdentifiers, in order (R16). Three of the five
 * are extern constants rather than literals, which is not a compile-time
 * initializer in C, so the array is filled before main. */
static const char *syn_default_identifiers[SYN_DEFAULT_COUNT];

__attribute__((constructor)) static void syn_fill_default_identifiers(void) {
  syn_default_identifiers[0] = NS_TOOLBAR_TOGGLE_SIDEBAR_ITEM_IDENTIFIER;
  syn_default_identifiers[1] =
      NS_TOOLBAR_SIDEBAR_TRACKING_SEPARATOR_ITEM_IDENTIFIER;
  syn_default_identifiers[2] = SYN_ADD_ID;
  syn_default_identifiers[3] = NS_TOOLBAR_FLEXIBLE_SPACE_ITEM_IDENTIFIER;
  syn_default_identifiers[4] = SYN_SEARCH_ID;
}

/* AppKit's own constants, declared here as opaque pointers: the C header
 * exports the same values under NS_-prefixed names, and this suite is what
 * pins the two equal. */
extern const void *const NSToolbarToggleSidebarItemIdentifier;
extern const void *const NSToolbarSidebarTrackingSeparatorItemIdentifier;
extern const void *const NSToolbarFlexibleSpaceItemIdentifier;

/* ---- the runtime reads this suite needs ---- */

static id syn_object(const void *receiver, const char *selector) {
  return ((id (*)(id, SEL))objc_msgSend)((id)(void *)receiver,
                                         sel_getUid(selector));
}

static const char *syn_utf8(const void *string) {
  return ((const char *(*)(id, SEL))objc_msgSend)((id)(void *)string,
                                                  sel_getUid("UTF8String"));
}

static id syn_nsstring(const char *utf8) {
  return ((id (*)(Class, SEL, const char *))objc_msgSend)(
      objc_getClass("NSString"), sel_getUid("stringWithUTF8String:"), utf8);
}

static long syn_retain_count(const void *object) {
  return ((long (*)(id, SEL))objc_msgSend)((id)(void *)object,
                                           sel_getUid("retainCount"));
}

/* The one member, asked the way AppKit asks it. `inserted` false is the
 * customization-palette question. */
static const void *syn_ask_delegate(ns_toolbar *toolbar, const char *identifier,
                                    bool inserted) {
  id delegate = syn_object(toolbar, "delegate");
  if (delegate == NULL) return NULL;
  return (const void *)((id (*)(id, SEL, id, id, BOOL))objc_msgSend)(
      delegate,
      sel_getUid("toolbar:itemForItemIdentifier:willBeInsertedIntoToolbar:"),
      (id)(void *)toolbar, syn_nsstring(identifier), inserted);
}

/* NSToolbarItem has no performClick:, so an item is fired the way AppKit fires
 * it: the action, sent to the target. */
static bool syn_fire_item(ns_toolbar_item *item) {
  id target = syn_object(item, "target");
  const char *action = syn_test_action_name(item);
  if (target == NULL || action == NULL || action[0] == '\0') return false;
  ((void (*)(id, SEL, id))objc_msgSend)(target, sel_getUid(action),
                                        (id)(void *)item);
  return true;
}

/* ---- the twin's MainWindowController, reduced to its toolbar ---- */

struct syn_model {
  char asked[SYN_MAX_ASKED][80];
  const void *asked_sender[SYN_MAX_ASKED];
  long asked_count;
  ns_toolbar_item *add;            /* borrowed: the toolbar holds it */
  ns_search_toolbar_item *search;  /* borrowed: the toolbar holds it */
  long add_clicks;
  long search_fires;
  char search_text[128];
  const void *search_sender;
};

static struct syn_model syn_model;

static void syn_reset(void) {
  memset(&syn_model, 0, sizeof syn_model);
}

/* The twin's addNewItem: (R16). */
static void syn_on_add(void *context, const void *sender) {
  struct syn_model *model = (struct syn_model *)context;
  model->add_clicks++;
  model->add = (ns_toolbar_item *)sender;
}

/* The twin's searchChanged: - the sender is the search field, and the text it
 * holds is read through the copy function (F2, KTD17). */
static void syn_on_search(void *context, const void *sender) {
  struct syn_model *model = (struct syn_model *)context;
  model->search_fires++;
  model->search_sender = sender;
  char *text = ns_control_copy_string_value((ns_control *)sender);
  snprintf(model->search_text, sizeof model->search_text, "%s",
           text != NULL ? text : "");
  ns_string_free(text);
}

/* The twin's toolbar:itemForItemIdentifier:willBeInsertedIntoToolbar: - the
 * one required member. What it returns is owned: the library hands the item to
 * AppKit and releases it once AppKit has retained it, so nothing here releases
 * an item it returns (R9). */
static ns_toolbar_item *syn_item_for(void *context, ns_toolbar *sender,
                                     const char *identifier, bool inserted) {
  struct syn_model *model = (struct syn_model *)context;
  (void)inserted;
  if (model != NULL && model->asked_count < SYN_MAX_ASKED) {
    snprintf(model->asked[model->asked_count],
             sizeof model->asked[model->asked_count], "%s", identifier);
    model->asked_sender[model->asked_count] = sender;
    model->asked_count++;
  }

  if (strcmp(identifier, SYN_ADD_ID) == 0) {
    ns_toolbar_item *item =
        ns_toolbar_item_create_with_item_identifier(identifier);
    ns_toolbar_item_set_label(item, "Add");
    ns_toolbar_item_set_palette_label(item, "Add");
    ns_toolbar_item_set_tool_tip(item, "Add an item");
    /* R23: an image-only button has no title to infer a label from, so the
     * description travels with the image. */
    ns_image *image =
        ns_image_create_with_system_symbol_name_accessibility_description(
            "plus", "Add");
    ns_toolbar_item_set_image(item, image);
    ns_release(image);
    ns_toolbar_item_set_bordered(item, true);
    ns_toolbar_item_set_action(item, syn_on_add, model);
    return item;
  }

  if (strcmp(identifier, SYN_SEARCH_ID) == 0) {
    ns_search_toolbar_item *item =
        ns_search_toolbar_item_create_with_item_identifier(identifier);
    ns_search_toolbar_item_set_resigns_first_responder_with_cancel(item, true);
    ns_control_set_action(
        ns_text_field_as_control(ns_search_toolbar_item_search_field(item)),
        syn_on_search, model);
    /* The upcast carries the same +1 the constructor handed back. */
    return ns_search_toolbar_item_as_toolbar_item(item);
  }

  return NULL; /* no item for that identifier, which is an answer (R9) */
}

static const ns_toolbar_callbacks syn_callbacks = {
    .item_for_item_identifier_will_be_inserted_into_toolbar = syn_item_for};

/* The twin's toolbar, with its callbacks and identifier lists installed.
 * Owned (+1). */
static ns_toolbar *syn_toolbar_create(void) {
  ns_toolbar *toolbar = ns_toolbar_create_with_identifier(SYN_TOOLBAR_ID);
  ns_toolbar_set_allows_user_customization(toolbar, false);
  ns_toolbar_set_display_mode(toolbar, NS_TOOLBAR_DISPLAY_MODE_ICON_ONLY);
  ns_toolbar_set_callbacks(toolbar, &syn_callbacks, syn_default_identifiers,
                           SYN_DEFAULT_COUNT, syn_default_identifiers,
                           SYN_DEFAULT_COUNT, &syn_model);
  return toolbar;
}

/* A window with a sidebar, so the sidebar tracking separator has a divider to
 * track, in the unified toolbar style (R15, R16). Owned (+1). */
static ns_window *syn_window_create(ns_toolbar *toolbar,
                                    ns_split_view_controller **out_split) {
  ns_split_view_controller *split = ns_split_view_controller_create();
  ns_view_controller *sidebar_pane = ns_view_controller_create();
  ns_view *sidebar_view = ns_view_create_with_frame(CGRectMake(0, 0, 220, 640));
  ns_view_controller_set_view(sidebar_pane, sidebar_view);
  ns_release(sidebar_view);
  ns_split_view_item *sidebar =
      ns_split_view_item_create_sidebar_with_view_controller(sidebar_pane);
  ns_split_view_controller_add_split_view_item(split, sidebar);
  ns_release(sidebar);
  ns_release(sidebar_pane);

  ns_view_controller *content_pane = ns_view_controller_create();
  ns_view *content_view = ns_view_create_with_frame(CGRectMake(0, 0, 780, 640));
  ns_view_controller_set_view(content_pane, content_view);
  ns_release(content_view);
  ns_split_view_item *content =
      ns_split_view_item_create_content_list_with_view_controller(content_pane);
  ns_split_view_controller_add_split_view_item(split, content);
  ns_release(content);
  ns_release(content_pane);

  ns_window *window = ns_window_create_with_content_rect_style_mask_backing_defer(
      CGRectMake(0, 0, 1000, 640),
      NS_WINDOW_STYLE_MASK_TITLED | NS_WINDOW_STYLE_MASK_CLOSABLE |
          NS_WINDOW_STYLE_MASK_MINIATURIZABLE |
          NS_WINDOW_STYLE_MASK_RESIZABLE |
          NS_WINDOW_STYLE_MASK_FULL_SIZE_CONTENT_VIEW,
      NS_BACKING_STORE_BUFFERED, false);
  ns_window_set_restorable(window, false);
  ns_window_set_content_view_controller(
      window, ns_split_view_controller_as_view_controller(split));
  ns_window_set_toolbar(window, toolbar);
  ns_window_set_toolbar_style(window, NS_WINDOW_TOOLBAR_STYLE_UNIFIED);
  *out_split = split;
  return window;
}

static long syn_index_of_item(ns_toolbar *toolbar, const char *identifier) {
  for (long index = 0; index < ns_toolbar_item_count(toolbar); index++) {
    char *found = ns_toolbar_item_copy_item_identifier(
        ns_toolbar_item_at_index(toolbar, index));
    bool hit = found != NULL && strcmp(found, identifier) == 0;
    ns_string_free(found);
    if (hit) return index;
  }
  return -1;
}

/* ---- AppKit's identifiers and the exported ones (R5, R11) ---- */

SYN_TEST(the_exported_identifiers_equal_appkits_own) {
  syn_test_bootstrap();
  SYN_ASSERT_STR_EQ(NS_TOOLBAR_SIDEBAR_TRACKING_SEPARATOR_ITEM_IDENTIFIER,
                    syn_utf8(NSToolbarSidebarTrackingSeparatorItemIdentifier));
  SYN_ASSERT_STR_EQ(NS_TOOLBAR_TOGGLE_SIDEBAR_ITEM_IDENTIFIER,
                    syn_utf8(NSToolbarToggleSidebarItemIdentifier));
  SYN_ASSERT_STR_EQ(NS_TOOLBAR_FLEXIBLE_SPACE_ITEM_IDENTIFIER,
                    syn_utf8(NSToolbarFlexibleSpaceItemIdentifier));
}

/* ---- the toolbar AppKit builds from the identifier lists (R16, KTD8) ---- */

SYN_TEST(the_default_identifiers_become_the_toolbars_items) {
  syn_test_bootstrap();
  syn_reset();
  ns_toolbar *toolbar = syn_toolbar_create();

  char *identifier = ns_toolbar_copy_identifier(toolbar);
  SYN_ASSERT_STR_EQ(identifier, SYN_TOOLBAR_ID);
  ns_string_free(identifier);
  SYN_ASSERT_MSG(!ns_toolbar_allows_user_customization(toolbar),
                 "user customization did not read back as off");
  SYN_ASSERT_MSG(ns_toolbar_get_display_mode(toolbar) ==
                     NS_TOOLBAR_DISPLAY_MODE_ICON_ONLY,
                 "the display mode did not read back as icon only");

  /* A toolbar with no window builds nothing: AppKit asks for the items when
   * the toolbar lands on a window. */
  SYN_ASSERT_MSG(ns_toolbar_item_count(toolbar) == 0,
                 "a toolbar with no window already has %ld items",
                 ns_toolbar_item_count(toolbar));

  ns_split_view_controller *split = NULL;
  ns_window *window = syn_window_create(toolbar, &split);

  SYN_ASSERT_MSG(ns_toolbar_item_count(toolbar) == SYN_DEFAULT_COUNT,
                 "the toolbar has %ld items, not one per default identifier",
                 ns_toolbar_item_count(toolbar));
  for (long index = 0; index < SYN_DEFAULT_COUNT; index++) {
    ns_toolbar_item *item = ns_toolbar_item_at_index(toolbar, index);
    char *found = ns_toolbar_item_copy_item_identifier(item);
    SYN_ASSERT_STR_EQ(found, syn_default_identifiers[index]);
    ns_string_free(found);
  }

  /* AppKit builds the items for its own identifiers and never asks the struct
   * for one, so the two app-defined identifiers are what the member saw - each
   * with the handle of the toolbar that asked. */
  SYN_ASSERT_MSG(syn_model.asked_count == 2,
                 "the member was asked %ld times, not twice",
                 syn_model.asked_count);
  SYN_ASSERT_STR_EQ(syn_model.asked[0], SYN_ADD_ID);
  SYN_ASSERT_STR_EQ(syn_model.asked[1], SYN_SEARCH_ID);
  for (long index = 0; index < syn_model.asked_count; index++)
    SYN_ASSERT_MSG(syn_model.asked_sender[index] == (const void *)toolbar,
                   "the member was handed a different toolbar than the one "
                   "that asked");

  /* The window's own readers, which U9 added alongside (the rider). */
  SYN_ASSERT_MSG(ns_window_toolbar(window) == toolbar,
                 "the window reports a different toolbar than the one set");
  SYN_ASSERT_MSG(ns_window_get_toolbar_style(window) ==
                     NS_WINDOW_TOOLBAR_STYLE_UNIFIED,
                 "the window's toolbar style did not read back as unified");

  ns_window_close(window);
  ns_release(window);
  ns_release(split);
  ns_release(toolbar);
}

/* R9: the member's return is owned, the wrapper hands the item to AppKit and
 * releases it once AppKit has retained it, and the C side is left holding
 * nothing it did not create. The item asked for directly comes back with one
 * pending autorelease and no other holder - which is exactly the +1 the
 * callback made, already handed over. */
SYN_TEST(an_item_a_callback_returns_leaves_the_c_side_holding_nothing) {
  syn_test_bootstrap();
  syn_reset();
  ns_toolbar *toolbar = syn_toolbar_create();
  ns_split_view_controller *split = NULL;
  ns_window *window = syn_window_create(toolbar, &split);

  long add = syn_index_of_item(toolbar, SYN_ADD_ID);
  SYN_ASSERT_MSG(add >= 0, "the Add item is not in the toolbar");
  /* The toolbar is the only holder of the item the callback made: nothing in
   * this suite retained or released it. */
  ns_toolbar_item *item = ns_toolbar_item_at_index(toolbar, add);
  SYN_ASSERT_MSG(item != NULL, "the toolbar lost the item it was handed");

  const void *fresh = syn_ask_delegate(toolbar, SYN_ADD_ID, false);
  SYN_ASSERT_MSG(fresh != NULL, "asking the member directly produced nothing");
  SYN_ASSERT_MSG(syn_retain_count(fresh) == 1,
                 "the item the member returned is held %ld times, not once; "
                 "the wrapper kept a reference it should have handed over "
                 "(R9)",
                 syn_retain_count(fresh));

  ns_window_close(window);
  ns_release(window);
  ns_release(split);
  ns_release(toolbar);
}

/* R9: null is an answer, not misuse - the SDK declares the return nullable, so
 * no report and no item. */
SYN_TEST(returning_null_for_an_identifier_yields_no_item_and_no_report) {
  syn_test_bootstrap();
  syn_reset();
  ns_toolbar *toolbar = ns_toolbar_create_with_identifier(SYN_TOOLBAR_ID);
  static const char *const with_unknown[] = {SYN_ADD_ID, SYN_UNKNOWN_ID};
  ns_toolbar_set_callbacks(toolbar, &syn_callbacks, with_unknown, 2,
                           with_unknown, 2, &syn_model);

  SYN_ASSERT_MSG(syn_ask_delegate(toolbar, SYN_UNKNOWN_ID, true) == NULL,
                 "an identifier the member builds nothing for produced an "
                 "item");

  ns_split_view_controller *split = NULL;
  ns_window *window = syn_window_create(toolbar, &split);
  SYN_ASSERT_MSG(ns_toolbar_item_count(toolbar) == 1,
                 "the toolbar has %ld items; the unknown identifier should "
                 "have produced none",
                 ns_toolbar_item_count(toolbar));
  SYN_ASSERT_MSG(syn_index_of_item(toolbar, SYN_ADD_ID) >= 0,
                 "the item the member did build is missing");

  ns_window_close(window);
  ns_release(window);
  ns_release(split);
  ns_release(toolbar);
}

/* ---- the identifier arrays are copied, not borrowed (KTD17's exception) ---- */

/* Installs from storage that is gone by the time this returns: the strings are
 * freed and the pointer array is a local. AppKit asks for the lists later, so
 * a wrapper that borrowed would read freed memory - which the sanitizer gate
 * catches, and which the scribble below catches without it. */
static void syn_install_from_storage_that_goes_away(ns_toolbar *toolbar) {
  const char *local[SYN_DEFAULT_COUNT];
  char *owned[SYN_DEFAULT_COUNT];
  for (long index = 0; index < SYN_DEFAULT_COUNT; index++) {
    owned[index] = strdup(syn_default_identifiers[index]);
    local[index] = owned[index];
  }
  ns_toolbar_set_callbacks(toolbar, &syn_callbacks, local, SYN_DEFAULT_COUNT,
                           local, SYN_DEFAULT_COUNT, &syn_model);
  for (long index = 0; index < SYN_DEFAULT_COUNT; index++) {
    memset(owned[index], 'X', strlen(owned[index]));
    free(owned[index]);
    local[index] = NULL;
  }
}

SYN_TEST(an_identifier_array_that_goes_out_of_scope_still_serves_appkit) {
  syn_test_bootstrap();
  syn_reset();
  ns_toolbar *toolbar = ns_toolbar_create_with_identifier(SYN_TOOLBAR_ID);
  syn_install_from_storage_that_goes_away(toolbar);

  ns_split_view_controller *split = NULL;
  ns_window *window = syn_window_create(toolbar, &split);

  SYN_ASSERT_MSG(ns_toolbar_item_count(toolbar) == SYN_DEFAULT_COUNT,
                 "the toolbar has %ld items, not one per default identifier; "
                 "the identifier copy did not outlive the caller's array",
                 ns_toolbar_item_count(toolbar));
  for (long index = 0; index < SYN_DEFAULT_COUNT; index++) {
    char *found = ns_toolbar_item_copy_item_identifier(
        ns_toolbar_item_at_index(toolbar, index));
    SYN_ASSERT_STR_EQ(found, syn_default_identifiers[index]);
    ns_string_free(found);
  }

  ns_window_close(window);
  ns_release(window);
  ns_release(split);
  ns_release(toolbar);
}

/* ---- the Add item (R16, R23) ---- */

SYN_TEST(the_add_items_action_fires_and_its_image_is_described) {
  syn_test_bootstrap();
  syn_reset();
  ns_toolbar *toolbar = syn_toolbar_create();
  ns_split_view_controller *split = NULL;
  ns_window *window = syn_window_create(toolbar, &split);

  long index = syn_index_of_item(toolbar, SYN_ADD_ID);
  SYN_ASSERT_MSG(index >= 0, "the Add item is not in the toolbar");
  ns_toolbar_item *item = ns_toolbar_item_at_index(toolbar, index);

  char *label = ns_toolbar_item_copy_label(item);
  SYN_ASSERT_STR_EQ(label, "Add");
  ns_string_free(label);
  char *palette = ns_toolbar_item_copy_palette_label(item);
  SYN_ASSERT_STR_EQ(palette, "Add");
  ns_string_free(palette);
  char *tip = ns_toolbar_item_copy_tool_tip(item);
  SYN_ASSERT_STR_EQ(tip, "Add an item");
  ns_string_free(tip);
  SYN_ASSERT_MSG(ns_toolbar_item_bordered(item),
                 "the Add item is not bordered");

  /* R23: the item is an image and nothing else, so the image's accessibility
   * description is the whole of what assistive technology reads. */
  ns_image *image = ns_toolbar_item_image(item);
  SYN_ASSERT_MSG(image != NULL, "the Add item carries no image");
  char *description = ns_image_copy_accessibility_description(image);
  SYN_ASSERT_MSG(description != NULL && description[0] != '\0',
                 "the Add item's image has no accessibility description");
  SYN_ASSERT_STR_EQ(description, "Add");
  ns_string_free(description);

  SYN_ASSERT_MSG(syn_fire_item(item), "the Add item has no target or action");
  SYN_WAIT_FOR(syn_model.add_clicks == 1, 1000);
  SYN_ASSERT_MSG(syn_model.add == item,
                 "the action was handed a different item than the one fired");

  ns_window_close(window);
  ns_release(window);
  ns_release(split);
  ns_release(toolbar);
}

/* ---- the search item's round trip (F2, R16) ---- */

SYN_TEST(typing_into_the_search_field_delivers_the_sender_and_its_text) {
  syn_test_bootstrap();
  syn_reset();
  ns_toolbar *toolbar = syn_toolbar_create();
  ns_split_view_controller *split = NULL;
  ns_window *window = syn_window_create(toolbar, &split);

  long index = syn_index_of_item(toolbar, SYN_SEARCH_ID);
  SYN_ASSERT_MSG(index >= 0, "the search item is not in the toolbar");
  ns_toolbar_item *item = ns_toolbar_item_at_index(toolbar, index);
  /* The upcast is the identity, so the item the toolbar holds is the search
   * item the member returned. */
  ns_search_toolbar_item *search = (ns_search_toolbar_item *)item;
  SYN_ASSERT_MSG(ns_search_toolbar_item_resigns_first_responder_with_cancel(
                     search),
                 "the search item does not resign first responder on cancel");

  ns_text_field *field = ns_search_toolbar_item_search_field(search);
  SYN_ASSERT_MSG(field != NULL, "the search item carries no field");
  ns_control *control = ns_text_field_as_control(field);

  ns_control_set_string_value(control, "ring");
  SYN_ASSERT_MSG(ns_control_send_action(control),
                 "the search field sent no action");
  SYN_WAIT_FOR(syn_model.search_fires == 1, 1000);
  SYN_ASSERT_MSG(syn_model.search_sender == (const void *)field,
                 "the action was handed something other than the search "
                 "field");
  SYN_ASSERT_STR_EQ(syn_model.search_text, "ring");

  /* A second edit, so the filter-as-you-type path is the one under test. */
  ns_control_set_string_value(control, "");
  SYN_ASSERT_MSG(ns_control_send_action(control),
                 "the search field sent no action the second time");
  SYN_WAIT_FOR(syn_model.search_fires == 2, 1000);
  SYN_ASSERT_STR_EQ(syn_model.search_text, "");

  /* Edit - Find, and the way back out. */
  ns_search_toolbar_item_begin_search_interaction(search);
  syn_test_spin(50);
  ns_search_toolbar_item_end_search_interaction(search);
  syn_test_spin(50);

  ns_window_close(window);
  ns_release(window);
  ns_release(split);
  ns_release(toolbar);
}

/* ---- install, replace, uninstall (R9, KTD8) ---- */

/* Counted as a delta, not as an absolute: AppKit keeps toolbars that share an
 * identifier synchronized with one another and holds on to them, so a toolbar
 * an earlier test released is not necessarily gone and its shim with it. What
 * has to hold is that an install adds exactly one and an uninstall removes
 * it. */
SYN_TEST(installing_again_replaces_the_shim_and_a_null_struct_uninstalls) {
  syn_test_bootstrap();
  syn_reset();
  long before = syn_test_shim_live_count();
  ns_toolbar *toolbar = syn_toolbar_create();
  SYN_ASSERT_SHIMS(before + 1);

  ns_toolbar_set_callbacks(toolbar, &syn_callbacks, syn_default_identifiers,
                           SYN_DEFAULT_COUNT, syn_default_identifiers,
                           SYN_DEFAULT_COUNT, &syn_model);
  SYN_ASSERT_MSG(syn_test_has_delegate(toolbar),
                 "installing again left the delegate slot empty");
  SYN_ASSERT_SHIMS(before + 1);

  ns_toolbar_set_callbacks(toolbar, NULL, NULL, 0, NULL, 0, NULL);
  SYN_ASSERT_SHIMS(before);
  SYN_ASSERT_MSG(!syn_test_has_delegate(toolbar),
                 "a null struct left the delegate slot filled");

  ns_release(toolbar);
  SYN_ASSERT_SHIMS(before);
}

/* ---- the upcast (R4, KTD9) ---- */

SYN_TEST(the_upcast_to_a_toolbar_item_is_the_same_pointer) {
  syn_test_bootstrap();
  ns_search_toolbar_item *search =
      ns_search_toolbar_item_create_with_item_identifier(SYN_SEARCH_ID);
  SYN_ASSERT_MSG((const void *)ns_search_toolbar_item_as_toolbar_item(search) ==
                     (const void *)search,
                 "the upcast returned a different pointer");
  ns_release(search);
}

/* ---- teardown, which is what the leak gate reads (F3, KTD12) ---- */

SYN_TEST(tearing_the_toolbar_window_down_releases_clean) {
  syn_test_bootstrap();
  syn_reset();
  long before = syn_test_shim_live_count();
  ns_toolbar *toolbar = syn_toolbar_create();
  ns_split_view_controller *split = NULL;
  ns_window *window = syn_window_create(toolbar, &split);
  ns_window_make_key_and_order_front(window);
  syn_test_spin(50);
  ns_toolbar_set_callbacks(toolbar, NULL, NULL, 0, NULL, 0, NULL);
  ns_window_set_toolbar(window, NULL);
  ns_window_close(window);
  ns_release(window);
  ns_release(split);
  ns_release(toolbar);
  SYN_ASSERT_SHIMS(before);
}

/* ---- the cases that abort (R9, R10, R12, KTD4) ---- */

SYN_ABORT_CASE(toolbar_installed_without_the_item_member) {
  syn_test_bootstrap();
  ns_toolbar *toolbar = ns_toolbar_create_with_identifier(SYN_TOOLBAR_ID);
  ns_toolbar_callbacks missing = {0};
  ns_toolbar_set_callbacks(toolbar, &missing, syn_default_identifiers,
                           SYN_DEFAULT_COUNT, syn_default_identifiers,
                           SYN_DEFAULT_COUNT, NULL);
}

/* R9: the report names the member and the protocol, before AppKit is ever
 * asked for an item. */
SYN_TEST(installing_without_the_required_member_names_it_and_the_protocol) {
  SYN_ASSERT_ABORTS("toolbar_installed_without_the_item_member",
                    "item_for_item_identifier_will_be_inserted_into_toolbar");
  SYN_ASSERT_ABORTS("toolbar_installed_without_the_item_member",
                    "NSToolbarDelegate");
  SYN_ASSERT_ABORTS("toolbar_installed_without_the_item_member",
                    "ns_toolbar_set_callbacks");
}

SYN_ABORT_CASE(toolbar_passed_where_a_toolbar_item_is_expected) {
  syn_test_bootstrap();
  ns_toolbar *toolbar = ns_toolbar_create_with_identifier(SYN_TOOLBAR_ID);
  ns_toolbar_item_copy_label((ns_toolbar_item *)toolbar);
}

SYN_TEST(a_handle_of_the_wrong_class_names_both_classes) {
  SYN_ASSERT_ABORTS("toolbar_passed_where_a_toolbar_item_is_expected",
                    "NSToolbarItem");
  SYN_ASSERT_ABORTS("toolbar_passed_where_a_toolbar_item_is_expected",
                    "ns_toolbar_item_copy_label");
}

static void *syn_create_toolbar_off_the_main_thread(void *ignored) {
  (void)ignored;
  ns_toolbar_create_with_identifier(SYN_TOOLBAR_ID);
  return NULL;
}

SYN_ABORT_CASE(toolbar_created_off_the_main_thread) {
  syn_test_bootstrap();
  pthread_t thread;
  if (pthread_create(&thread, NULL, syn_create_toolbar_off_the_main_thread,
                     NULL) != 0) {
    fprintf(stderr, "could not start the background thread\n");
    return;
  }
  pthread_join(thread, NULL);
}

SYN_TEST(creating_a_toolbar_off_the_main_thread_names_it) {
  SYN_ASSERT_ABORTS("toolbar_created_off_the_main_thread",
                    "ns_toolbar_create_with_identifier");
  SYN_ASSERT_ABORTS("toolbar_created_off_the_main_thread", "main thread only");
}

/*
 * NSMenu - a menu: a title and the items in it (R18).
 *
 * A menu is also the submenu behind a menu bar item, and that is where its
 * title shows: ns_menu_bar.h builds the standard menu bar out of one menu per
 * top-level title.
 *
 * `itemArray` is a copy property in the SDK, so the items do not cross as an
 * array. They cross as a count function plus an index accessor whose element
 * is borrowed from the menu - ns_menu_number_of_items and
 * ns_menu_item_at_index - which is the boundary shape for every array Syntonic
 * returns (R7, KTD7, KTD17).
 */

#ifndef SYNTONIC_NS_MENU_H
#define SYNTONIC_NS_MENU_H

#include <os/availability.h>
#include <stdbool.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_menu ns_menu;

/* Declared by ns_menu_item.h and repeated here so this header stands alone. C
 * has allowed a repeated typedef of the same type since C11. */
typedef struct ns_menu_item ns_menu_item;

/* -[NSMenu initWithTitle:] - owned (+1), released with ns_release (R7). */
ns_menu *_Nonnull ns_menu_create_with_title(const char *_Nonnull title)
    API_AVAILABLE(macos(26.0));

/* -[NSMenu setTitle:] */
void ns_menu_set_title(ns_menu *_Nonnull menu, const char *_Nonnull title)
    API_AVAILABLE(macos(26.0));

/* -[NSMenu title] - `title` is a copy property, so this is an owned copy the
 * caller frees with ns_string_free (R7, R11). */
char *_Nonnull ns_menu_copy_title(ns_menu *_Nonnull menu)
    API_AVAILABLE(macos(26.0));

/* -[NSMenu addItem:] - appends. The menu retains the item, so releasing your
 * own reference afterwards is correct (R7). */
void ns_menu_add_item(ns_menu *_Nonnull menu, ns_menu_item *_Nonnull item)
    API_AVAILABLE(macos(26.0));

/* -[NSMenu insertItem:atIndex:] - `index` runs from 0 to the item count, where
 * the count appends. A debug build reports an index outside that range and
 * stops, because AppKit raises only after it has already done part of the work
 * (R12). */
void ns_menu_insert_item_at_index(ns_menu *_Nonnull menu,
                                  ns_menu_item *_Nonnull item, long index)
    API_AVAILABLE(macos(26.0));

/* -[NSMenu numberOfItems] - the count half of the array shape (KTD17). */
long ns_menu_number_of_items(ns_menu *_Nonnull menu) API_AVAILABLE(macos(26.0));

/* -[NSMenu itemAtIndex:] - borrowed: the menu keeps the item, so this is the
 * index half of the array shape and the caller keeps it past the menu with
 * ns_retain (R7, KTD7, KTD17). An index at or past the count is an AppKit
 * exception, which a debug build reports with this function's name (KTD4). */
ns_menu_item *_Nullable ns_menu_item_at_index(ns_menu *_Nonnull menu,
                                              long index)
    API_AVAILABLE(macos(26.0));

/* -[NSMenu performActionForItemAtIndex:] - fires the item's action as
 * choosing it would, which is how a program drives a menu. */
void ns_menu_perform_action_for_item_at_index(ns_menu *_Nonnull menu,
                                              long index)
    API_AVAILABLE(macos(26.0));

/* -[NSMenu setAutoenablesItems:] - on by default, and while it is on AppKit
 * computes each item's enabled flag from the responder chain and
 * ns_menu_item_set_enabled is ignored. Turn it off to drive the flag. */
void ns_menu_set_autoenables_items(ns_menu *_Nonnull menu, bool autoenables)
    API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_MENU_H */

/*
 * NSMenu (R18, KTD7, KTD17).
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syntonic/ns_menu.h"

/* Owned (+1): the caller ends it with ns_release (R7, KTD7). */
ns_menu *ns_menu_create_with_title(const char *title) {
  NS_ENTER();
  return NS_OUT_OWNED(ns_menu,
                      [[NSMenu alloc] initWithTitle:NS_STRING_IN(title)]);
  NS_LEAVE();
}

void ns_menu_set_title(ns_menu *menu, const char *title) {
  NS_ENTER();
  NS_IN(NSMenu, menu).title = NS_STRING_IN(title);
  NS_LEAVE();
}

/* `title` is a copy property, so the value is owned and the name says copy_
 * (R7, R11). */
char *ns_menu_copy_title(ns_menu *menu) {
  NS_ENTER();
  return NS_STRING_OUT(NS_IN(NSMenu, menu).title);
  NS_LEAVE();
}

void ns_menu_add_item(ns_menu *menu, ns_menu_item *item) {
  NS_ENTER();
  [NS_IN(NSMenu, menu) addItem:NS_IN(NSMenuItem, item)];
  NS_LEAVE();
}

/* The explicit check conventions.md's "when to add one" calls for: AppKit
 * raises here only after it has already done part of the work, so the report
 * has to come first and name this function (R12, KTD4). The largest index the
 * call accepts is the count, which appends. */
void ns_menu_insert_item_at_index(ns_menu *menu, ns_menu_item *item,
                                  long index) {
  NS_ENTER();
  NSMenu *target = NS_IN(NSMenu, menu);
  NS_CHECK_INDEX(index, (long)target.numberOfItems);
  [target insertItem:NS_IN(NSMenuItem, item) atIndex:(NSInteger)index];
  NS_LEAVE();
}

/* The count half of the array shape: `itemArray` is a copy property and never
 * crosses as a handle (R7, KTD17). */
long ns_menu_number_of_items(ns_menu *menu) {
  NS_ENTER();
  return (long)NS_IN(NSMenu, menu).numberOfItems;
  NS_LEAVE();
}

/* Borrowed, and one of the three entries on conventions.md's borrowed-return
 * allowlist: the menu holds the item for as long as the caller could use it
 * (R7, KTD7). No explicit range check - AppKit raises on an index past the
 * end and the entry macro reports that exception with this function's name. */
ns_menu_item *ns_menu_item_at_index(ns_menu *menu, long index) {
  NS_ENTER();
  return NS_OUT(ns_menu_item,
                [NS_IN(NSMenu, menu) itemAtIndex:(NSInteger)index]);
  NS_LEAVE();
}

void ns_menu_perform_action_for_item_at_index(ns_menu *menu, long index) {
  NS_ENTER();
  [NS_IN(NSMenu, menu) performActionForItemAtIndex:(NSInteger)index];
  NS_LEAVE();
}

void ns_menu_set_autoenables_items(ns_menu *menu, bool autoenables) {
  NS_ENTER();
  NS_IN(NSMenu, menu).autoenablesItems = autoenables;
  NS_LEAVE();
}

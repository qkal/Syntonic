/*
 * NSTabViewController: the Settings window's container, in the toolbar tab
 * style (R19, KTD7).
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syntonic/ns_tab_view_controller.h"

/* Owned (+1): the caller ends it with ns_release (R7, KTD7). */
ns_tab_view_controller *ns_tab_view_controller_create(void) {
  NS_ENTER();
  return NS_OUT_OWNED(ns_tab_view_controller,
                      [[NSTabViewController alloc] init]);
  NS_LEAVE();
}

void ns_tab_view_controller_set_tab_style(
    ns_tab_view_controller *tab_view_controller,
    ns_tab_view_controller_tab_style style) {
  NS_ENTER();
  NS_IN(NSTabViewController, tab_view_controller).tabStyle =
      (NSTabViewControllerTabStyle)style;
  NS_LEAVE();
}

ns_tab_view_controller_tab_style ns_tab_view_controller_get_tab_style(
    ns_tab_view_controller *tab_view_controller) {
  NS_ENTER();
  return (ns_tab_view_controller_tab_style)NS_IN(NSTabViewController,
                                                 tab_view_controller)
      .tabStyle;
  NS_LEAVE();
}

void ns_tab_view_controller_add_tab_view_item(
    ns_tab_view_controller *tab_view_controller,
    ns_tab_view_item *tab_view_item) {
  NS_ENTER();
  [NS_IN(NSTabViewController, tab_view_controller)
      addTabViewItem:NS_IN(NSTabViewItem, tab_view_item)];
  NS_LEAVE();
}

long ns_tab_view_controller_tab_view_item_count(
    ns_tab_view_controller *tab_view_controller) {
  NS_ENTER();
  return (long)NS_IN(NSTabViewController, tab_view_controller)
      .tabViewItems.count;
  NS_LEAVE();
}

/* Borrowed: the element is read through the receiver, which holds it (KTD17).
 * No explicit range check - NSArray raises on an index past the end and the
 * entry macro reports that exception with this function's name (KTD4). */
ns_tab_view_item *ns_tab_view_controller_tab_view_item_at_index(
    ns_tab_view_controller *tab_view_controller, long index) {
  NS_ENTER();
  NSArray<NSTabViewItem *> *items =
      NS_IN(NSTabViewController, tab_view_controller).tabViewItems;
  return NS_OUT(ns_tab_view_item, items[(NSUInteger)index]);
  NS_LEAVE();
}

/* The index check this call writes by hand (R12). AppKit is not clean on
 * either side: a negative index is taken silently and selects the first tab,
 * and the exception it raises past the end names a range that includes the
 * rejected index. One check covers both and names this function. */
void ns_tab_view_controller_set_selected_tab_view_item_index(
    ns_tab_view_controller *tab_view_controller, long index) {
  NS_ENTER();
  NSTabViewController *target = NS_IN(NSTabViewController, tab_view_controller);
  NS_CHECK_INDEX(index, (long)target.tabViewItems.count - 1);
  target.selectedTabViewItemIndex = (NSInteger)index;
  NS_LEAVE();
}

long ns_tab_view_controller_selected_tab_view_item_index(
    ns_tab_view_controller *tab_view_controller) {
  NS_ENTER();
  return (long)NS_IN(NSTabViewController, tab_view_controller)
      .selectedTabViewItemIndex;
  NS_LEAVE();
}

/* An upcast is the identity, with the debug class check NS_IN carries (KTD9). */
ns_view_controller *ns_tab_view_controller_as_view_controller(
    ns_tab_view_controller *tab_view_controller) {
  NS_ENTER();
  return NS_OUT(ns_view_controller,
                NS_IN(NSTabViewController, tab_view_controller));
  NS_LEAVE();
}

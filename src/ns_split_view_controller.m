/*
 * NSSplitViewController - the sidebar, list and detail container (R15).
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syntonic/ns_split_view.h"
#include "syntonic/ns_split_view_controller.h"
#include "syntonic/ns_split_view_item.h"

/* Owned (+1): the caller ends it with ns_release (R7, KTD7). */
ns_split_view_controller *ns_split_view_controller_create(void) {
  NS_ENTER();
  return NS_OUT_OWNED(ns_split_view_controller, [[NSSplitViewController alloc]
                                                    init]);
  NS_LEAVE();
}

void ns_split_view_controller_add_split_view_item(
    ns_split_view_controller *split_view_controller,
    ns_split_view_item *split_view_item) {
  NS_ENTER();
  [NS_IN(NSSplitViewController, split_view_controller)
      addSplitViewItem:NS_IN(NSSplitViewItem, split_view_item)];
  NS_LEAVE();
}

long ns_split_view_controller_split_view_item_count(
    ns_split_view_controller *split_view_controller) {
  NS_ENTER();
  return (long)NS_IN(NSSplitViewController, split_view_controller)
      .splitViewItems.count;
  NS_LEAVE();
}

/* Borrowed: the controller keeps the item, so the accessor reads the live
 * child through the receiver (R7, KTD17). No index check of its own: NSArray
 * raises before it changes anything and the entry macro's exception report
 * names this function (KTD4). */
ns_split_view_item *ns_split_view_controller_split_view_item_at_index(
    ns_split_view_controller *split_view_controller, long index) {
  NS_ENTER();
  return NS_OUT(ns_split_view_item,
                NS_IN(NSSplitViewController, split_view_controller)
                    .splitViewItems[(NSUInteger)index]);
  NS_LEAVE();
}

/* Borrowed: `splitView` is a strong property (R7). */
ns_split_view *ns_split_view_controller_split_view(
    ns_split_view_controller *split_view_controller) {
  NS_ENTER();
  return NS_OUT(ns_split_view, NS_IN(NSSplitViewController,
                                     split_view_controller)
                                   .splitView);
  NS_LEAVE();
}

void ns_split_view_controller_toggle_sidebar(
    ns_split_view_controller *split_view_controller) {
  NS_ENTER();
  [NS_IN(NSSplitViewController, split_view_controller) toggleSidebar:nil];
  NS_LEAVE();
}

/* An upcast is the identity, with the debug class check NS_IN carries (KTD9). */
ns_view_controller *ns_split_view_controller_as_view_controller(
    ns_split_view_controller *split_view_controller) {
  NS_ENTER();
  return NS_OUT(ns_view_controller,
                NS_IN(NSSplitViewController, split_view_controller));
  NS_LEAVE();
}

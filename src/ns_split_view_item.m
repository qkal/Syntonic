/*
 * NSSplitViewItem - one pane of a split view controller (R15, R7).
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syntonic/ns_split_view_item.h"

/* Owned (+1): a `+…WithViewController:` class method returning instancetype is
 * a constructor, so the wrapper takes a reference the caller ends with
 * ns_release (R7, KTD7). */
ns_split_view_item *ns_split_view_item_create_with_view_controller(
    ns_view_controller *view_controller) {
  NS_ENTER();
  return NS_OUT_OWNED(
      ns_split_view_item,
      [NSSplitViewItem splitViewItemWithViewController:
                           NS_IN(NSViewController, view_controller)]);
  NS_LEAVE();
}

/* R15: everything that makes a sidebar look and behave like one - the
 * translucent material, collapsing on a resize, the overlay in full screen,
 * spring loading and the standard thickness range - is this class method's,
 * not a property the caller can set afterwards. */
ns_split_view_item *ns_split_view_item_create_sidebar_with_view_controller(
    ns_view_controller *view_controller) {
  NS_ENTER();
  return NS_OUT_OWNED(ns_split_view_item,
                      [NSSplitViewItem sidebarWithViewController:
                                           NS_IN(NSViewController,
                                                 view_controller)]);
  NS_LEAVE();
}

ns_split_view_item *ns_split_view_item_create_content_list_with_view_controller(
    ns_view_controller *view_controller) {
  NS_ENTER();
  return NS_OUT_OWNED(ns_split_view_item,
                      [NSSplitViewItem contentListWithViewController:
                                           NS_IN(NSViewController,
                                                 view_controller)]);
  NS_LEAVE();
}

/* Borrowed: `viewController` is a strong property (R7). */
ns_view_controller *ns_split_view_item_view_controller(
    ns_split_view_item *split_view_item) {
  NS_ENTER();
  return NS_OUT(ns_view_controller,
                NS_IN(NSSplitViewItem, split_view_item).viewController);
  NS_LEAVE();
}

ns_split_view_item_behavior ns_split_view_item_get_behavior(
    ns_split_view_item *split_view_item) {
  NS_ENTER();
  return (ns_split_view_item_behavior)
      NS_IN(NSSplitViewItem, split_view_item).behavior;
  NS_LEAVE();
}

void ns_split_view_item_set_collapsed(ns_split_view_item *split_view_item,
                                      bool collapsed) {
  NS_ENTER();
  NS_IN(NSSplitViewItem, split_view_item).collapsed = collapsed;
  NS_LEAVE();
}

bool ns_split_view_item_collapsed(ns_split_view_item *split_view_item) {
  NS_ENTER();
  return NS_IN(NSSplitViewItem, split_view_item).collapsed;
  NS_LEAVE();
}

void ns_split_view_item_set_can_collapse(ns_split_view_item *split_view_item,
                                         bool can_collapse) {
  NS_ENTER();
  NS_IN(NSSplitViewItem, split_view_item).canCollapse = can_collapse;
  NS_LEAVE();
}

bool ns_split_view_item_can_collapse(ns_split_view_item *split_view_item) {
  NS_ENTER();
  return NS_IN(NSSplitViewItem, split_view_item).canCollapse;
  NS_LEAVE();
}

void ns_split_view_item_set_minimum_thickness(
    ns_split_view_item *split_view_item, CGFloat thickness) {
  NS_ENTER();
  NS_IN(NSSplitViewItem, split_view_item).minimumThickness = thickness;
  NS_LEAVE();
}

CGFloat ns_split_view_item_minimum_thickness(
    ns_split_view_item *split_view_item) {
  NS_ENTER();
  return NS_IN(NSSplitViewItem, split_view_item).minimumThickness;
  NS_LEAVE();
}

void ns_split_view_item_set_maximum_thickness(
    ns_split_view_item *split_view_item, CGFloat thickness) {
  NS_ENTER();
  NS_IN(NSSplitViewItem, split_view_item).maximumThickness = thickness;
  NS_LEAVE();
}

CGFloat ns_split_view_item_maximum_thickness(
    ns_split_view_item *split_view_item) {
  NS_ENTER();
  return NS_IN(NSSplitViewItem, split_view_item).maximumThickness;
  NS_LEAVE();
}

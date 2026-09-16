/*
 * NSTabViewItem: one tab's view controller, label and image (R19, KTD7).
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syntonic/ns_tab_view_item.h"

/* Owned (+1): a +tabViewItemWith… class method returns a fresh object like any
 * other constructor, so the caller ends it with ns_release (R7, KTD7). */
ns_tab_view_item *ns_tab_view_item_create_with_view_controller(
    ns_view_controller *view_controller) {
  NS_ENTER();
  return NS_OUT_OWNED(
      ns_tab_view_item,
      [NSTabViewItem tabViewItemWithViewController:NS_IN(NSViewController,
                                                         view_controller)]);
  NS_LEAVE();
}

void ns_tab_view_item_set_label(ns_tab_view_item *tab_view_item,
                                const char *label) {
  NS_ENTER();
  NS_IN(NSTabViewItem, tab_view_item).label = NS_STRING_IN(label);
  NS_LEAVE();
}

/* `label` is a copy property, so the value is owned and the name says copy_
 * (R7, R11). */
char *ns_tab_view_item_copy_label(ns_tab_view_item *tab_view_item) {
  NS_ENTER();
  return NS_STRING_OUT(NS_IN(NSTabViewItem, tab_view_item).label);
  NS_LEAVE();
}

void ns_tab_view_item_set_image(ns_tab_view_item *tab_view_item,
                                ns_image *image) {
  NS_ENTER();
  NS_IN(NSTabViewItem, tab_view_item).image = NS_IN_OPT(NSImage, image);
  NS_LEAVE();
}

/* Borrowed: `image` is a strong property, which is the shape NS_OUT is right
 * for (R7). */
ns_image *ns_tab_view_item_image(ns_tab_view_item *tab_view_item) {
  NS_ENTER();
  return NS_OUT(ns_image, NS_IN(NSTabViewItem, tab_view_item).image);
  NS_LEAVE();
}

/* Borrowed for the same reason: `viewController` is a strong property (R7). */
ns_view_controller *ns_tab_view_item_view_controller(
    ns_tab_view_item *tab_view_item) {
  NS_ENTER();
  return NS_OUT(ns_view_controller,
                NS_IN(NSTabViewItem, tab_view_item).viewController);
  NS_LEAVE();
}

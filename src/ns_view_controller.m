/*
 * NSViewController (R1, R14). The base for the split and tab view controllers
 * later units add.
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syntonic/ns_view_controller.h"

/* Owned (+1): the caller ends it with ns_release (R7, KTD7). */
ns_view_controller *ns_view_controller_create(void) {
  NS_ENTER();
  return NS_OUT_OWNED(ns_view_controller, [[NSViewController alloc] init]);
  NS_LEAVE();
}

void ns_view_controller_set_view(ns_view_controller *view_controller,
                                 ns_view *view) {
  NS_ENTER();
  NS_IN(NSViewController, view_controller).view = NS_IN(NSView, view);
  NS_LEAVE();
}

/* Borrowed: `view` is a strong property, which is the only shape NS_OUT is
 * right for (R7). */
ns_view *ns_view_controller_view(ns_view_controller *view_controller) {
  NS_ENTER();
  return NS_OUT(ns_view, NS_IN(NSViewController, view_controller).view);
  NS_LEAVE();
}

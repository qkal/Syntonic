/*
 * NSStackView: the stacking axis, the arranged-subview list, the spacing and
 * the edge insets (KTD3, KTD7, KTD17).
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syntonic/ns_stack_view.h"

/* Owned (+1): the caller ends it with ns_release (R7, KTD7). The views are
 * borrowed for the call and the stack retains what it keeps (KTD17). */
ns_stack_view *ns_stack_view_create_with_views(ns_view *const *views,
                                               long count) {
  NS_ENTER();
  NSMutableArray<NSView *> *arranged =
      [NSMutableArray arrayWithCapacity:(NSUInteger)(count > 0 ? count : 0)];
  for (long index = 0; index < count; index++)
    [arranged addObject:NS_IN(NSView, views[index])];
  return NS_OUT_OWNED(ns_stack_view, [NSStackView stackViewWithViews:arranged]);
  NS_LEAVE();
}

void ns_stack_view_set_orientation(
    ns_stack_view *stack_view,
    ns_user_interface_layout_orientation orientation) {
  NS_ENTER();
  NS_IN(NSStackView, stack_view).orientation =
      (NSUserInterfaceLayoutOrientation)orientation;
  NS_LEAVE();
}

ns_user_interface_layout_orientation ns_stack_view_orientation(
    ns_stack_view *stack_view) {
  NS_ENTER();
  return (ns_user_interface_layout_orientation)NS_IN(NSStackView, stack_view)
      .orientation;
  NS_LEAVE();
}

void ns_stack_view_add_arranged_subview(ns_stack_view *stack_view,
                                        ns_view *view) {
  NS_ENTER();
  [NS_IN(NSStackView, stack_view) addArrangedSubview:NS_IN(NSView, view)];
  NS_LEAVE();
}

/* `arrangedSubviews` is a readonly copy property, so it never crosses as a
 * handle: it becomes a count plus an index accessor (R7, KTD17). */
long ns_stack_view_arranged_subview_count(ns_stack_view *stack_view) {
  NS_ENTER();
  return (long)NS_IN(NSStackView, stack_view).arrangedSubviews.count;
  NS_LEAVE();
}

/* Borrowed: the element is read through the receiver, which holds it (KTD17).
 * No explicit range check - NSArray raises on an index past the end and the
 * entry macro reports that exception with this function's name (KTD4). */
ns_view *ns_stack_view_arranged_subview_at_index(ns_stack_view *stack_view,
                                                 long index) {
  NS_ENTER();
  NSArray<NSView *> *arranged =
      NS_IN(NSStackView, stack_view).arrangedSubviews;
  return NS_OUT(ns_view, arranged[(NSUInteger)index]);
  NS_LEAVE();
}

void ns_stack_view_set_spacing(ns_stack_view *stack_view, CGFloat spacing) {
  NS_ENTER();
  NS_IN(NSStackView, stack_view).spacing = spacing;
  NS_LEAVE();
}

CGFloat ns_stack_view_spacing(ns_stack_view *stack_view) {
  NS_ENTER();
  return NS_IN(NSStackView, stack_view).spacing;
  NS_LEAVE();
}

/* NSEdgeInsets is not reachable from a C header, so the four fields cross as
 * Syntonic's own struct and this is where the two meet (R11). */
void ns_stack_view_set_edge_insets(ns_stack_view *stack_view,
                                   ns_edge_insets insets) {
  NS_ENTER();
  NS_IN(NSStackView, stack_view).edgeInsets =
      NSEdgeInsetsMake(insets.top, insets.left, insets.bottom, insets.right);
  NS_LEAVE();
}

ns_edge_insets ns_stack_view_edge_insets(ns_stack_view *stack_view) {
  NS_ENTER();
  NSEdgeInsets insets = NS_IN(NSStackView, stack_view).edgeInsets;
  return (ns_edge_insets){.top = insets.top,
                          .left = insets.left,
                          .bottom = insets.bottom,
                          .right = insets.right};
  NS_LEAVE();
}

/* The upcast is the same pointer; NS_IN is what checks the class (R4, KTD9). */
ns_view *ns_stack_view_as_view(ns_stack_view *stack_view) {
  NS_ENTER();
  return NS_OUT(ns_view, NS_IN(NSStackView, stack_view));
  NS_LEAVE();
}

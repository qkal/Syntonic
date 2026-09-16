/*
 * NSSplitView - where a divider goes (R15).
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syntonic/ns_split_view.h"

/* The index check docs/conventions.md asks for where AppKit takes the bad
 * value silently: setPosition:ofDividerAtIndex: with an index outside the
 * range raises nothing and moves nothing, so without this the caller would get
 * a window that quietly kept its default geometry (R12). A split view with N
 * arranged panes has N-1 dividers, so the largest index this call accepts is
 * the count minus two. */
void ns_split_view_set_position_of_divider_at_index(ns_split_view *split_view,
                                                    CGFloat position,
                                                    long divider_index) {
  NS_ENTER();
  NSSplitView *target = NS_IN(NSSplitView, split_view);
  NS_CHECK_INDEX(divider_index, (long)target.arrangedSubviews.count - 2);
  [target setPosition:position ofDividerAtIndex:(NSInteger)divider_index];
  NS_LEAVE();
}

void ns_split_view_set_vertical(ns_split_view *split_view, bool vertical) {
  NS_ENTER();
  NS_IN(NSSplitView, split_view).vertical = vertical;
  NS_LEAVE();
}

bool ns_split_view_vertical(ns_split_view *split_view) {
  NS_ENTER();
  return NS_IN(NSSplitView, split_view).vertical;
  NS_LEAVE();
}

/* An upcast is the identity, with the debug class check NS_IN carries (KTD9). */
ns_view *ns_split_view_as_view(ns_split_view *split_view) {
  NS_ENTER();
  return NS_OUT(ns_view, NS_IN(NSSplitView, split_view));
  NS_LEAVE();
}

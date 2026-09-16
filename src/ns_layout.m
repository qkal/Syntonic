/*
 * Edge pinning: the two helpers that are the whole of v0's constraint surface
 * (KTD3, KTD18).
 *
 * Both do the same four things in the same order - turn the pinned view's
 * autoresizing translation off, add it to the container, build the anchor
 * constraints, activate them. Without the first step AppKit keeps translating
 * the view's frame into constraints of its own and the pin is unsatisfiable.
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syntonic/ns_layout.h"

void ns_layout_pin_edges(ns_view *view, ns_view *container, CGFloat inset) {
  NS_ENTER();
  NSView *pinned = NS_IN(NSView, view);
  NSView *parent = NS_IN(NSView, container);
  pinned.translatesAutoresizingMaskIntoConstraints = NO;
  [parent addSubview:pinned];
  [NSLayoutConstraint activateConstraints:@[
    [pinned.leadingAnchor constraintEqualToAnchor:parent.leadingAnchor
                                         constant:inset],
    [pinned.trailingAnchor constraintEqualToAnchor:parent.trailingAnchor
                                          constant:-inset],
    [pinned.topAnchor constraintEqualToAnchor:parent.topAnchor constant:inset],
    [pinned.bottomAnchor constraintEqualToAnchor:parent.bottomAnchor
                                        constant:-inset],
  ]];
  NS_LEAVE();
}

/* The trailing edge is a "no further than" rather than an equality, so the
 * view keeps its own width as well as its own height. */
void ns_layout_pin_top_edges(ns_view *view, ns_view *container, CGFloat inset) {
  NS_ENTER();
  NSView *pinned = NS_IN(NSView, view);
  NSView *parent = NS_IN(NSView, container);
  pinned.translatesAutoresizingMaskIntoConstraints = NO;
  [parent addSubview:pinned];
  [NSLayoutConstraint activateConstraints:@[
    [pinned.leadingAnchor constraintEqualToAnchor:parent.leadingAnchor
                                         constant:inset],
    [pinned.trailingAnchor
        constraintLessThanOrEqualToAnchor:parent.trailingAnchor
                                 constant:-inset],
    [pinned.topAnchor constraintEqualToAnchor:parent.topAnchor constant:inset],
  ]];
  NS_LEAVE();
}

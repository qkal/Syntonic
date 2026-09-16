/*
 * Edge pinning - the third and last of v0's layout mechanisms, beside
 * NSStackView and NSGridView (KTD3).
 *
 * This is a non-mirror header (KTD18): neither function wraps a single AppKit
 * selector. Each one is the same four lines every hand-written AppKit view
 * controller opens with - turn the pinned view's autoresizing translation off,
 * add it to the container, build the anchor constraints, activate them - and
 * the Swift twin's `pinEdges` and `pinTopEdges` are the same two helpers.
 *
 * A general Auto Layout constraint API is deferred (KTD3): these two are the
 * whole of it in v0, and an NSLayoutConstraint handle never crosses.
 *
 * Both take the inset in points and apply it to every edge they pin. The view
 * must not already be in a different view's subtree; passing the container's
 * own superview, or a view from another window, is what AppKit raises on.
 */

#ifndef SYNTONIC_NS_LAYOUT_H
#define SYNTONIC_NS_LAYOUT_H

#include <CoreGraphics/CGGeometry.h>
#include <os/availability.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Declared by ns_view.h and repeated here so this header stands alone. C has
 * allowed a repeated typedef of the same type since C11. */
typedef struct ns_view ns_view;

/*
 * Adds `view` to `container` and pins all four of its edges to the container's,
 * `inset` points inside each one, with the constraints activated. The pinned
 * view's autoresizing translation is turned off, so it takes its whole frame
 * from those constraints and resizes with the container from then on.
 * syntonic-owned.
 */
void ns_layout_pin_edges(ns_view *_Nonnull view, ns_view *_Nonnull container,
                         CGFloat inset) API_AVAILABLE(macos(26.0));

/*
 * The same, except that the view keeps its own height and sits against the
 * container's top edge: leading and top are pinned, trailing is pinned at most
 * that far in, and the bottom is left free. This is what a form pinned to the
 * top of a pane needs - the detail pane and every settings pane use it.
 * syntonic-owned.
 */
void ns_layout_pin_top_edges(ns_view *_Nonnull view,
                             ns_view *_Nonnull container, CGFloat inset)
    API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_LAYOUT_H */

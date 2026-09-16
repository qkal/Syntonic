/*
 * NSStackView - a row or a column of views laid out by Auto Layout, which with
 * NSGridView and the edge-pinning helpers in ns_layout.h is the whole of v0's
 * layout vocabulary (KTD3).
 *
 * A stack is created from the views it starts with, and
 * ns_stack_view_add_arranged_subview appends to it. `arrangedSubviews` is a
 * readonly copy property in the SDK, so it never crosses as a handle: it is a
 * count function plus an index accessor whose element is borrowed from the
 * receiver (KTD7, KTD17), exactly as ns_view's subview list works.
 *
 * +[NSStackView stackViewWithViews:] turns the stack's own autoresizing
 * translation off, which is what lets its constraints hold. Pin it into its
 * container with ns_layout_pin_edges rather than by setting a frame.
 */

#ifndef SYNTONIC_NS_STACK_VIEW_H
#define SYNTONIC_NS_STACK_VIEW_H

#include <CoreGraphics/CGGeometry.h>
#include <os/availability.h>
#include <stdint.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_stack_view ns_stack_view;

/* Declared by ns_view.h and repeated here so this header stands alone. C has
 * allowed a repeated typedef of the same type since C11. */
typedef struct ns_view ns_view;

/* NSUserInterfaceLayoutOrientation, the axis a stack stacks along. It belongs
 * to NSUserInterfaceLayout.h, which v0 does not wrap, and the stack view is
 * the only place it is needed - so it is declared here, where it is used. */
typedef enum ns_user_interface_layout_orientation : int64_t {
  NS_USER_INTERFACE_LAYOUT_ORIENTATION_HORIZONTAL = 0,
  NS_USER_INTERFACE_LAYOUT_ORIENTATION_VERTICAL = 1,
} ns_user_interface_layout_orientation;

/* NSEdgeInsets, the padding inside a stack around all of its views. It belongs
 * to Foundation's NSGeometry.h, which is not includable from C - it pulls in
 * Objective-C declarations - so the four fields cross as a plain struct of
 * their own, the way CGRect crosses unchanged (R11). */
typedef struct ns_edge_insets {
  CGFloat top;
  CGFloat left;
  CGFloat bottom;
  CGFloat right;
} ns_edge_insets;

/* +[NSStackView stackViewWithViews:] - owned (+1), released with ns_release
 * (R7). `views` is a pointer plus a count, borrowed for the call (KTD17); pass
 * null and 0 for an empty stack. The stack is horizontal and has its
 * autoresizing translation turned off, which is AppKit's own behaviour for
 * this constructor. */
ns_stack_view *_Nonnull ns_stack_view_create_with_views(
    ns_view *_Nonnull const *_Nullable views, long count)
    API_AVAILABLE(macos(26.0));

/* -[NSStackView setOrientation:] */
void ns_stack_view_set_orientation(
    ns_stack_view *_Nonnull stack_view,
    ns_user_interface_layout_orientation orientation)
    API_AVAILABLE(macos(26.0));

/* -[NSStackView orientation] */
ns_user_interface_layout_orientation ns_stack_view_orientation(
    ns_stack_view *_Nonnull stack_view) API_AVAILABLE(macos(26.0));

/* -[NSStackView addArrangedSubview:] - appends the view to the arranged list
 * and, if it is not one already, makes it a subview. The stack retains it, so
 * releasing your own reference afterwards is correct (R7). */
void ns_stack_view_add_arranged_subview(ns_stack_view *_Nonnull stack_view,
                                        ns_view *_Nonnull view)
    API_AVAILABLE(macos(26.0));

/* -[NSStackView arrangedSubviews] - the count half of the array shape:
 * `arrangedSubviews` is a readonly copy property and never crosses as a handle
 * (R7, KTD17). */
long ns_stack_view_arranged_subview_count(ns_stack_view *_Nonnull stack_view)
    API_AVAILABLE(macos(26.0));

/* -[NSStackView arrangedSubviews] - the index half, in insertion order.
 * Borrowed: the element is read through the receiver, which holds it; keep it
 * longer with ns_retain (R7, KTD17). */
ns_view *_Nonnull ns_stack_view_arranged_subview_at_index(
    ns_stack_view *_Nonnull stack_view, long index) API_AVAILABLE(macos(26.0));

/* -[NSStackView setSpacing:] - the minimum gap between two arranged views. */
void ns_stack_view_set_spacing(ns_stack_view *_Nonnull stack_view,
                               CGFloat spacing) API_AVAILABLE(macos(26.0));

/* -[NSStackView spacing] */
CGFloat ns_stack_view_spacing(ns_stack_view *_Nonnull stack_view)
    API_AVAILABLE(macos(26.0));

/* -[NSStackView setEdgeInsets:] - the padding around all of the arranged
 * views. */
void ns_stack_view_set_edge_insets(ns_stack_view *_Nonnull stack_view,
                                   ns_edge_insets insets)
    API_AVAILABLE(macos(26.0));

/* -[NSStackView edgeInsets] */
ns_edge_insets ns_stack_view_edge_insets(ns_stack_view *_Nonnull stack_view)
    API_AVAILABLE(macos(26.0));

/* The upcast to ns_view: the same pointer, checked in a debug build (R4,
 * KTD9). syntonic-owned. */
ns_view *_Nonnull ns_stack_view_as_view(ns_stack_view *_Nonnull stack_view)
    API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_STACK_VIEW_H */

/*
 * NSSplitView - the view a split view controller lays its panes out in, and
 * the one place a program says where a divider goes (R15).
 *
 * A split view controller makes its own split view and is not the same object
 * as it, so the twin's fixed geometry is reached through the accessor:
 *
 *     ns_split_view *split_view =
 *         ns_split_view_controller_split_view(split);
 *     ns_split_view_set_position_of_divider_at_index(split_view, 220, 0);
 *     ns_split_view_set_position_of_divider_at_index(split_view, 620, 1);
 *
 * DIVIDER INDICES are zero-based, the leading-most divider in a vertical split
 * view being 0, and a split view with N panes has N-1 of them.
 *
 * THERE IS NO POSITION READER. NSSplitView declares none - only the minimum
 * and maximum a divider could take - so a program reads a pinned position back
 * the way the twins' layout reports do, from the pane's own frame:
 * ns_split_view_item_view_controller, then ns_view_controller_view, then
 * ns_view_frame. Syntonic invents no reader AppKit does not have.
 *
 * A POSITION IS A REQUEST, NOT A SETTING. AppKit applies it as though the user
 * had dragged the divider there, so an item's minimum and maximum thickness
 * clamp it and a later resize of the window moves it again. Pin the dividers
 * after the window has its final size, as ns_window_make_key_and_order_front
 * leaves it, and pin them again after a resize if the program wants them back.
 */

#ifndef SYNTONIC_NS_SPLIT_VIEW_H
#define SYNTONIC_NS_SPLIT_VIEW_H

#include <CoreGraphics/CGGeometry.h>
#include <os/availability.h>
#include <stdbool.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_split_view ns_split_view;

/* Declared by ns_view.h and repeated here so this header stands alone. C has
 * allowed a repeated typedef of the same type since C11. */
typedef struct ns_view ns_view;

/* -[NSSplitView setPosition:ofDividerAtIndex:] - places the divider as though
 * the user had dragged it there, so the adjacent items' minimum and maximum
 * thickness clamp the result and a divider may end up somewhere else.
 * `divider_index` is checked against the divider count in a debug build (R12):
 * AppKit takes an out-of-range index silently, changing nothing, which is the
 * case docs/conventions.md says to check by hand. */
void ns_split_view_set_position_of_divider_at_index(
    ns_split_view *_Nonnull split_view, CGFloat position, long divider_index)
    API_AVAILABLE(macos(26.0));

/* -[NSSplitView setVertical:] - true lays the panes out side by side, which is
 * what a sidebar, a list and a detail pane are. */
void ns_split_view_set_vertical(ns_split_view *_Nonnull split_view,
                                bool vertical) API_AVAILABLE(macos(26.0));

/* -[NSSplitView isVertical] - a BOOL getter drops AppKit's `is` (R5). True for
 * the split view a split view controller makes for itself. */
bool ns_split_view_vertical(ns_split_view *_Nonnull split_view)
    API_AVAILABLE(macos(26.0));

/* The upcast to ns_view: the same pointer, checked in a debug build (R4,
 * KTD9). syntonic-owned. */
ns_view *_Nonnull ns_split_view_as_view(ns_split_view *_Nonnull split_view)
    API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_SPLIT_VIEW_H */

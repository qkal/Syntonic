/*
 * NSSplitViewController - the container behind the main window: a sidebar, a
 * list and a detail pane side by side, one split view item each (R15).
 *
 *     ns_split_view_controller *split = ns_split_view_controller_create();
 *     ns_split_view_item *sidebar =
 *         ns_split_view_item_create_sidebar_with_view_controller(sidebar_vc);
 *     ns_split_view_controller_add_split_view_item(split, sidebar);
 *     ns_release(sidebar);
 *     ns_window_set_content_view_controller(
 *         window, ns_split_view_controller_as_view_controller(split));
 *
 * TOGGLING THE SIDEBAR. The standard menu bar's View - Toggle Sidebar item has
 * a nil target and the action `toggleSidebar:`, so choosing it walks the
 * responder chain to whichever split view controller is in it and collapses
 * the first sidebar item there. Nothing has to be wired up for that to work,
 * and no C callback sits behind it: a SEL never crosses the boundary (R11).
 * ns_split_view_controller_toggle_sidebar is the same thing sent directly, for
 * a program that wants to collapse the sidebar without a menu.
 *
 * The collapse is animated either way, so the item's collapsed flag reads back
 * on a later turn of the run loop rather than inside the call.
 *
 * Panes are laid out with Auto Layout, which the split view controller manages
 * itself; a child view controller's view is loaded lazily, when its item is
 * first uncollapsed.
 */

#ifndef SYNTONIC_NS_SPLIT_VIEW_CONTROLLER_H
#define SYNTONIC_NS_SPLIT_VIEW_CONTROLLER_H

#include <os/availability.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_split_view_controller ns_split_view_controller;

/* Declared by ns_split_view_item.h and ns_view_controller.h and repeated here
 * so this header stands alone. C has allowed a repeated typedef of the same
 * type since C11. */
typedef struct ns_split_view_item ns_split_view_item;
typedef struct ns_split_view ns_split_view;
typedef struct ns_view_controller ns_view_controller;

/* -[NSSplitViewController init] - owned (+1), released with ns_release (R7).
 * NSSplitViewController inherits `init` rather than declaring one of its own;
 * a constructor still belongs to the concrete class, because the handle it
 * returns is this class's. */
ns_split_view_controller *_Nonnull ns_split_view_controller_create(void)
    API_AVAILABLE(macos(26.0));

/* -[NSSplitViewController addSplitViewItem:] - the controller retains the
 * item, so releasing your own reference afterwards is correct (R7). The item
 * has to carry a view controller already, which every
 * ns_split_view_item_create… guarantees. */
void ns_split_view_controller_add_split_view_item(
    ns_split_view_controller *_Nonnull split_view_controller,
    ns_split_view_item *_Nonnull split_view_item) API_AVAILABLE(macos(26.0));

/* -[NSSplitViewController splitViewItems] - the count half of the array shape:
 * `splitViewItems` is a copy property, so the array itself never crosses
 * (R11, KTD17). */
long ns_split_view_controller_split_view_item_count(
    ns_split_view_controller *_Nonnull split_view_controller)
    API_AVAILABLE(macos(26.0));

/* -[NSSplitViewController splitViewItems] - the index half, in the order the
 * items were added. Borrowed: the controller holds the item, and ns_retain
 * keeps it longer (R7). */
ns_split_view_item *_Nonnull ns_split_view_controller_split_view_item_at_index(
    ns_split_view_controller *_Nonnull split_view_controller, long index)
    API_AVAILABLE(macos(26.0));

/* -[NSSplitViewController splitView] - a strong property, so this is borrowed:
 * valid while the controller holds it, kept longer with ns_retain (R7). The
 * controller makes this view for itself and it is not the controller's own
 * `view`; it is where a divider position is set. See ns_split_view.h. */
ns_split_view *_Nonnull ns_split_view_controller_split_view(
    ns_split_view_controller *_Nonnull split_view_controller)
    API_AVAILABLE(macos(26.0));

/* -[NSSplitViewController toggleSidebar:] - collapses or reveals the first
 * sidebar item, animated; a controller with no sidebar item does nothing.
 * AppKit's sender argument is an `id` and never crosses the boundary (R11), so
 * the library passes nil. This is what the standard menu bar's View - Toggle
 * Sidebar item reaches through the responder chain. */
void ns_split_view_controller_toggle_sidebar(
    ns_split_view_controller *_Nonnull split_view_controller)
    API_AVAILABLE(macos(26.0));

/* The upcast to ns_view_controller: the same pointer, checked in a debug build
 * (R4, KTD9). This is what a window's content view controller takes.
 * syntonic-owned. */
ns_view_controller *_Nonnull ns_split_view_controller_as_view_controller(
    ns_split_view_controller *_Nonnull split_view_controller)
    API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_SPLIT_VIEW_CONTROLLER_H */

/*
 * NSSplitViewItem - one pane of a split view controller, and the object that
 * carries the sidebar's system behaviour (R15).
 *
 * WHY THERE ARE THREE CONSTRUCTORS. A split view item is not a container: it
 * describes how the split view controller treats the view controller behind
 * it. Which constructor you call is what a pane is, and each one sets a
 * different set of defaults that nothing else here can reproduce:
 *
 *   ns_split_view_item_create_sidebar_with_view_controller
 *       the translucent sidebar material, collapse on a split view resize,
 *       overlay at small sizes in full screen, can_collapse on, the standard
 *       sidebar minimum and maximum thickness, spring loading, and the divider
 *       a sidebar tracking separator toolbar item tracks (R15).
 *   ns_split_view_item_create_content_list_with_view_controller
 *       the list pane's standard thickness range, as Mail's message list has.
 *   ns_split_view_item_create_with_view_controller
 *       a plain pane with every default left alone.
 *
 * The material and the collapse behaviour are the sidebar constructor's, not
 * properties to set afterwards, which is why a sidebar built out of the plain
 * constructor looks wrong however it is configured.
 *
 * The item retains its view controller, and the split view controller retains
 * the item, so releasing your own reference to either after handing it over is
 * correct (R7).
 */

#ifndef SYNTONIC_NS_SPLIT_VIEW_ITEM_H
#define SYNTONIC_NS_SPLIT_VIEW_ITEM_H

#include <CoreGraphics/CGGeometry.h>
#include <os/availability.h>
#include <stdbool.h>
#include <stdint.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_split_view_item ns_split_view_item;

/* Declared by ns_view_controller.h and repeated here so this header stands
 * alone. C has allowed a repeated typedef of the same type since C11. */
typedef struct ns_view_controller ns_view_controller;

/* NSSplitViewItemBehavior. What the item is, which is decided by the
 * constructor you call and is read-only afterwards. */
typedef enum ns_split_view_item_behavior : int64_t {
  NS_SPLIT_VIEW_ITEM_BEHAVIOR_DEFAULT = 0,
  NS_SPLIT_VIEW_ITEM_BEHAVIOR_SIDEBAR = 1,
  NS_SPLIT_VIEW_ITEM_BEHAVIOR_CONTENT_LIST = 2,
  NS_SPLIT_VIEW_ITEM_BEHAVIOR_INSPECTOR = 3,
} ns_split_view_item_behavior;

/* +[NSSplitViewItem splitViewItemWithViewController:] - owned (+1), released
 * with ns_release (R7). A plain pane: every default left alone. */
ns_split_view_item *_Nonnull ns_split_view_item_create_with_view_controller(
    ns_view_controller *_Nonnull view_controller) API_AVAILABLE(macos(26.0));

/* +[NSSplitViewItem sidebarWithViewController:] - owned (+1), released with
 * ns_release (R7). The sidebar's material, collapse behaviour, spring loading
 * and standard thickness range come with it (R15). */
ns_split_view_item *_Nonnull
ns_split_view_item_create_sidebar_with_view_controller(
    ns_view_controller *_Nonnull view_controller) API_AVAILABLE(macos(26.0));

/* +[NSSplitViewItem contentListWithViewController:] - owned (+1), released
 * with ns_release (R7). The list pane's standard thickness range comes with
 * it. */
ns_split_view_item *_Nonnull
ns_split_view_item_create_content_list_with_view_controller(
    ns_view_controller *_Nonnull view_controller) API_AVAILABLE(macos(26.0));

/* -[NSSplitViewItem viewController] - a strong property, so this is borrowed:
 * valid while the item holds it, kept longer with ns_retain (R7). */
ns_view_controller *_Nonnull ns_split_view_item_view_controller(
    ns_split_view_item *_Nonnull split_view_item) API_AVAILABLE(macos(26.0));

/* -[NSSplitViewItem behavior] - `get_`, because ns_split_view_item_behavior is
 * already a type name and C puts the two in one namespace (R5). Read-only: the
 * constructor decides it. */
ns_split_view_item_behavior ns_split_view_item_get_behavior(
    ns_split_view_item *_Nonnull split_view_item) API_AVAILABLE(macos(26.0));

/* -[NSSplitViewItem setCollapsed:] - collapses or reveals the pane. The change
 * is animated, so the flag reads back on a later turn of the run loop rather
 * than inside this call. */
void ns_split_view_item_set_collapsed(
    ns_split_view_item *_Nonnull split_view_item, bool collapsed)
    API_AVAILABLE(macos(26.0));

/* -[NSSplitViewItem isCollapsed] - a BOOL getter drops AppKit's `is` (R5). */
bool ns_split_view_item_collapsed(
    ns_split_view_item *_Nonnull split_view_item) API_AVAILABLE(macos(26.0));

/* -[NSSplitViewItem setCanCollapse:] - whether the user can collapse the pane
 * by dragging or double-clicking a divider. A sidebar item starts with it on. */
void ns_split_view_item_set_can_collapse(
    ns_split_view_item *_Nonnull split_view_item, bool can_collapse)
    API_AVAILABLE(macos(26.0));

/* -[NSSplitViewItem canCollapse] */
bool ns_split_view_item_can_collapse(
    ns_split_view_item *_Nonnull split_view_item) API_AVAILABLE(macos(26.0));

/* -[NSSplitViewItem setMinimumThickness:] - width for a vertical split view,
 * height otherwise. */
void ns_split_view_item_set_minimum_thickness(
    ns_split_view_item *_Nonnull split_view_item, CGFloat thickness)
    API_AVAILABLE(macos(26.0));

/* -[NSSplitViewItem minimumThickness] */
CGFloat ns_split_view_item_minimum_thickness(
    ns_split_view_item *_Nonnull split_view_item) API_AVAILABLE(macos(26.0));

/* -[NSSplitViewItem setMaximumThickness:] */
void ns_split_view_item_set_maximum_thickness(
    ns_split_view_item *_Nonnull split_view_item, CGFloat thickness)
    API_AVAILABLE(macos(26.0));

/* -[NSSplitViewItem maximumThickness] */
CGFloat ns_split_view_item_maximum_thickness(
    ns_split_view_item *_Nonnull split_view_item) API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_SPLIT_VIEW_ITEM_H */

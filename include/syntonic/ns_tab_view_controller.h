/*
 * NSTabViewController - the container behind a Settings window: one child view
 * controller shown at a time, with a tab for each (R19).
 *
 * THE SETTINGS WINDOW IS TWO SETTINGS, AND BOTH ARE NEEDED
 *
 * ns_tab_view_controller_set_tab_style with NS_TAB_VIEW_CONTROLLER_TAB_STYLE_-
 * TOOLBAR pushes the tabs into the window's toolbar, and
 * ns_window_set_toolbar_style with NS_WINDOW_TOOLBAR_STYLE_PREFERENCE is what
 * gives that toolbar the Settings metrics. Either one alone looks wrong.
 *
 *     ns_tab_view_controller *tabs = ns_tab_view_controller_create();
 *     ns_tab_view_controller_set_tab_style(
 *         tabs, NS_TAB_VIEW_CONTROLLER_TAB_STYLE_TOOLBAR);
 *     ns_tab_view_controller_add_tab_view_item(tabs, general);
 *     ns_window_set_content_view_controller(
 *         window, ns_tab_view_controller_as_view_controller(tabs));
 *     ns_window_set_toolbar_style(window, NS_WINDOW_TOOLBAR_STYLE_PREFERENCE);
 *
 * WHO OWNS THE TOOLBAR
 *
 * The toolbar is the tab view controller's: with the toolbar tab style it
 * creates one, makes itself the toolbar's delegate, and assigns it to the
 * window it lands in as soon as it becomes that window's content view
 * controller. Nothing here hands that toolbar to the caller, and a caller who
 * sets a toolbar of their own on the window loses the tabs.
 *
 * The controller loads its children's views lazily: a tab's view is built the
 * first time that tab is selected.
 */

#ifndef SYNTONIC_NS_TAB_VIEW_CONTROLLER_H
#define SYNTONIC_NS_TAB_VIEW_CONTROLLER_H

#include <os/availability.h>
#include <stdint.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_tab_view_controller ns_tab_view_controller;

/* Declared by ns_tab_view_item.h and ns_view_controller.h and repeated here so
 * this header stands alone. C has allowed a repeated typedef of the same type
 * since C11. */
typedef struct ns_tab_view_item ns_tab_view_item;
typedef struct ns_view_controller ns_view_controller;

/* NSTabViewControllerTabStyle. TOOLBAR is the Settings window's; the two
 * segmented styles draw a segmented control above or below the pane, and
 * UNSPECIFIED draws no tab control at all. */
typedef enum ns_tab_view_controller_tab_style : int64_t {
  NS_TAB_VIEW_CONTROLLER_TAB_STYLE_UNSPECIFIED = -1,
  NS_TAB_VIEW_CONTROLLER_TAB_STYLE_SEGMENTED_CONTROL_ON_TOP = 0,
  NS_TAB_VIEW_CONTROLLER_TAB_STYLE_SEGMENTED_CONTROL_ON_BOTTOM = 1,
  NS_TAB_VIEW_CONTROLLER_TAB_STYLE_TOOLBAR = 2,
} ns_tab_view_controller_tab_style;

/* -[NSTabViewController init] - owned (+1), released with ns_release (R7).
 * NSTabViewController inherits `init` rather than declaring one of its own; a
 * constructor still belongs to the concrete class, because the handle it
 * returns is this class's. */
ns_tab_view_controller *_Nonnull ns_tab_view_controller_create(void)
    API_AVAILABLE(macos(26.0));

/* -[NSTabViewController setTabStyle:] - set it before the controller becomes a
 * window's content view controller; that is when AppKit builds the tab UI the
 * style names. */
void ns_tab_view_controller_set_tab_style(
    ns_tab_view_controller *_Nonnull tab_view_controller,
    ns_tab_view_controller_tab_style style) API_AVAILABLE(macos(26.0));

/* -[NSTabViewController tabStyle] - `get_` because the reader's own name is
 * already the enum type's (R5). */
ns_tab_view_controller_tab_style ns_tab_view_controller_get_tab_style(
    ns_tab_view_controller *_Nonnull tab_view_controller)
    API_AVAILABLE(macos(26.0));

/* -[NSTabViewController addTabViewItem:] - the controller retains the item, so
 * releasing your own reference afterwards is correct (R7). The item has to
 * carry a view controller already, which
 * ns_tab_view_item_create_with_view_controller is what guarantees. */
void ns_tab_view_controller_add_tab_view_item(
    ns_tab_view_controller *_Nonnull tab_view_controller,
    ns_tab_view_item *_Nonnull tab_view_item) API_AVAILABLE(macos(26.0));

/* -[NSTabViewController tabViewItems] - the count half of the array shape:
 * `tabViewItems` is a copy property, so the array itself never crosses
 * (R11). */
long ns_tab_view_controller_tab_view_item_count(
    ns_tab_view_controller *_Nonnull tab_view_controller)
    API_AVAILABLE(macos(26.0));

/* -[NSTabViewController tabViewItems] - the index half, in the order the items
 * were added. Borrowed: the controller holds the item, and ns_retain keeps it
 * longer (R7). */
ns_tab_view_item *_Nonnull ns_tab_view_controller_tab_view_item_at_index(
    ns_tab_view_controller *_Nonnull tab_view_controller, long index)
    API_AVAILABLE(macos(26.0));

/* -[NSTabViewController setSelectedTabViewItemIndex:] - shows that tab's view,
 * loading it if this is its first selection. A debug build checks the index
 * and names this function (R12): AppKit takes a negative index without a word
 * and selects the first tab instead, and its own exception for an index past
 * the end reports a range that includes the index it rejected. The largest
 * index this call accepts is the count minus one, so a controller with no tabs
 * accepts none - the -1 the reader returns is an answer, not a value to set. */
void ns_tab_view_controller_set_selected_tab_view_item_index(
    ns_tab_view_controller *_Nonnull tab_view_controller, long index)
    API_AVAILABLE(macos(26.0));

/* -[NSTabViewController selectedTabViewItemIndex] - -1 when the controller has
 * no tabs, which is AppKit's own "no selection" value (R11). */
long ns_tab_view_controller_selected_tab_view_item_index(
    ns_tab_view_controller *_Nonnull tab_view_controller)
    API_AVAILABLE(macos(26.0));

/* The upcast to ns_view_controller: the same pointer, checked in a debug build
 * (R4, KTD9). This is what a window's content view controller takes, and where
 * the inherited view lives. syntonic-owned. */
ns_view_controller *_Nonnull ns_tab_view_controller_as_view_controller(
    ns_tab_view_controller *_Nonnull tab_view_controller)
    API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_TAB_VIEW_CONTROLLER_H */

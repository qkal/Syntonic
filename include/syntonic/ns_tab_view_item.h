/*
 * NSTabViewItem - one tab: a view controller, the label the tab carries, and
 * the image beside it (R19).
 *
 * An item is created around its view controller, because a tab view controller
 * throws on an item that has none. The item retains the controller, so
 * releasing your own reference after creating the item is correct (R7); the
 * tab view controller then retains the item the same way.
 *
 * The label and the image are what the toolbar tab draws: with the toolbar tab
 * style, AppKit binds each toolbar item's label, image and tool tip to the
 * corresponding tab view item's. Setting the label after the item is already
 * in a tab view controller updates the tab.
 */

#ifndef SYNTONIC_NS_TAB_VIEW_ITEM_H
#define SYNTONIC_NS_TAB_VIEW_ITEM_H

#include <os/availability.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_tab_view_item ns_tab_view_item;

/* Declared by ns_image.h and ns_view_controller.h and repeated here so this
 * header stands alone. C has allowed a repeated typedef of the same type since
 * C11. */
typedef struct ns_image ns_image;
typedef struct ns_view_controller ns_view_controller;

/* +[NSTabViewItem tabViewItemWithViewController:] - owned (+1), released with
 * ns_release (R7). The item retains the controller and takes its title as the
 * item's label, so a label of your own goes on afterwards. */
ns_tab_view_item *_Nonnull ns_tab_view_item_create_with_view_controller(
    ns_view_controller *_Nonnull view_controller) API_AVAILABLE(macos(26.0));

/* -[NSTabViewItem setLabel:] */
void ns_tab_view_item_set_label(ns_tab_view_item *_Nonnull tab_view_item,
                                const char *_Nonnull label)
    API_AVAILABLE(macos(26.0));

/* -[NSTabViewItem label] - `label` is a copy property, so this is an owned
 * copy the caller frees with ns_string_free (R7, R11). */
char *_Nonnull ns_tab_view_item_copy_label(
    ns_tab_view_item *_Nonnull tab_view_item) API_AVAILABLE(macos(26.0));

/* -[NSTabViewItem setImage:] - the item retains the image, so releasing your
 * own reference afterwards is correct (R7). Null leaves the tab without one. */
void ns_tab_view_item_set_image(ns_tab_view_item *_Nonnull tab_view_item,
                                ns_image *_Nullable image)
    API_AVAILABLE(macos(26.0));

/* -[NSTabViewItem image] - a strong property, so this is borrowed: valid while
 * the item holds it, kept longer with ns_retain (R7). */
ns_image *_Nullable ns_tab_view_item_image(
    ns_tab_view_item *_Nonnull tab_view_item) API_AVAILABLE(macos(26.0));

/* -[NSTabViewItem viewController] - a strong property, so this is borrowed:
 * valid while the item holds it, kept longer with ns_retain (R7). */
ns_view_controller *_Nullable ns_tab_view_item_view_controller(
    ns_tab_view_item *_Nonnull tab_view_item) API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_TAB_VIEW_ITEM_H */

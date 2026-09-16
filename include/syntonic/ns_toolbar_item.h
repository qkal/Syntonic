/*
 * NSToolbarItem - one item in a window toolbar: the Add button of the main
 * window, and the standard items AppKit makes for itself (R16, R23).
 *
 * WHERE AN ITEM COMES FROM. A toolbar never holds items you hand it. It asks
 * its callbacks struct for one, by identifier, each time it needs one, and the
 * struct's member returns a freshly created item - an owned handle the library
 * releases once AppKit has retained it (R9). See ns_toolbar.h.
 *
 * THE STANDARD IDENTIFIERS. Three of AppKit's own identifiers are exported
 * below. An item with one of those identifiers is built by AppKit itself: the
 * callbacks struct is never asked for it, and returning one would be ignored.
 *
 * ACCESSIBILITY (R23). An item with an image and no title has nothing for
 * assistive technology to infer a label from, so the description travels with
 * the image:
 * ns_image_create_with_system_symbol_name_accessibility_description takes it,
 * and ns_toolbar_item_set_image puts it on the item. Nothing here sets an
 * accessibility property the caller did not ask for.
 */

#ifndef SYNTONIC_NS_TOOLBAR_ITEM_H
#define SYNTONIC_NS_TOOLBAR_ITEM_H

#include <os/availability.h>
#include <stdbool.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_toolbar_item ns_toolbar_item;

/* Declared by ns_image.h and repeated here so this header stands alone. C has
 * allowed a repeated typedef of the same type since C11. */
typedef struct ns_image ns_image;

/*
 * The standard item identifiers of NSToolbarItem.h, as UTF-8 strings (R11).
 *
 * These carry AppKit's value, not AppKit's symbol name: a C constant cannot be
 * initialised from an NSString at load time, and a C declaration that reused
 * AppKit's own spelling would collide with the NSString the SDK declares under
 * that name, so no source that includes both headers would compile. The names
 * below follow the rule the enum constants follow instead, and
 * tests/test_toolbar.c pins each value equal to AppKit's own.
 *
 * NSToolbarToggleSidebarItemIdentifier - sends toggleSidebar: down the
 * responder chain, which is what reaches the split view controller (R15). */
extern const char *_Nonnull const NS_TOOLBAR_TOGGLE_SIDEBAR_ITEM_IDENTIFIER;

/* NSToolbarSidebarTrackingSeparatorItemIdentifier - a separator that tracks
 * the sidebar's divider, so the toolbar is split along the same line the panes
 * are (R15). It applies to a window with the full size content view style
 * mask, which a split view controller's window has. */
extern const char *_Nonnull const NS_TOOLBAR_SIDEBAR_TRACKING_SEPARATOR_ITEM_IDENTIFIER;

/* NSToolbarFlexibleSpaceItemIdentifier - a space that takes the width left
 * over, which is what pushes a search item to the trailing edge. */
extern const char *_Nonnull const NS_TOOLBAR_FLEXIBLE_SPACE_ITEM_IDENTIFIER;

/* -[NSToolbarItem initWithItemIdentifier:] - owned (+1), released with
 * ns_release (R7). Return it straight from the callbacks struct's member and
 * do not release it yourself: the library does that once AppKit has retained
 * it (R9). */
ns_toolbar_item *_Nonnull ns_toolbar_item_create_with_item_identifier(
    const char *_Nonnull item_identifier) API_AVAILABLE(macos(26.0));

/* -[NSToolbarItem itemIdentifier] - a copy property, so this is an owned copy
 * the caller frees with ns_string_free (R7, R11). */
char *_Nonnull ns_toolbar_item_copy_item_identifier(
    ns_toolbar_item *_Nonnull toolbar_item) API_AVAILABLE(macos(26.0));

/* -[NSToolbarItem setLabel:] - the text under the item in the toolbar, and the
 * title of its entry in the overflow menu. */
void ns_toolbar_item_set_label(ns_toolbar_item *_Nonnull toolbar_item,
                               const char *_Nonnull label)
    API_AVAILABLE(macos(26.0));

/* -[NSToolbarItem label] - a copy property, so this is an owned copy the
 * caller frees with ns_string_free (R7, R11). */
char *_Nonnull ns_toolbar_item_copy_label(
    ns_toolbar_item *_Nonnull toolbar_item) API_AVAILABLE(macos(26.0));

/* -[NSToolbarItem setPaletteLabel:] - the text under the item in the
 * customization palette. Every item needs one, and the label is usually the
 * right value. */
void ns_toolbar_item_set_palette_label(ns_toolbar_item *_Nonnull toolbar_item,
                                       const char *_Nonnull palette_label)
    API_AVAILABLE(macos(26.0));

/* -[NSToolbarItem paletteLabel] - a copy property, so this is an owned copy
 * the caller frees with ns_string_free (R7, R11). */
char *_Nonnull ns_toolbar_item_copy_palette_label(
    ns_toolbar_item *_Nonnull toolbar_item) API_AVAILABLE(macos(26.0));

/* -[NSToolbarItem setToolTip:] - null removes the tooltip. */
void ns_toolbar_item_set_tool_tip(ns_toolbar_item *_Nonnull toolbar_item,
                                  const char *_Nullable tool_tip)
    API_AVAILABLE(macos(26.0));

/* -[NSToolbarItem toolTip] - a copy property, so this is an owned copy the
 * caller frees with ns_string_free (R7, R11). */
char *_Nullable ns_toolbar_item_copy_tool_tip(
    ns_toolbar_item *_Nonnull toolbar_item) API_AVAILABLE(macos(26.0));

/* -[NSToolbarItem setImage:] - the item retains the image, so releasing your
 * own reference afterwards is correct (R7). An image-only item reads to
 * assistive technology as the image's accessibility description (R23). */
void ns_toolbar_item_set_image(ns_toolbar_item *_Nonnull toolbar_item,
                               ns_image *_Nullable image)
    API_AVAILABLE(macos(26.0));

/* -[NSToolbarItem image] - a strong property, so this is borrowed: valid while
 * the item holds it, kept longer with ns_retain (R7). */
ns_image *_Nullable ns_toolbar_item_image(
    ns_toolbar_item *_Nonnull toolbar_item) API_AVAILABLE(macos(26.0));

/* -[NSToolbarItem setBordered:] - true draws the standard button border around
 * an image-only item, which is what an Add button wants. Defaults to false. */
void ns_toolbar_item_set_bordered(ns_toolbar_item *_Nonnull toolbar_item,
                                  bool bordered) API_AVAILABLE(macos(26.0));

/* -[NSToolbarItem isBordered] - a BOOL getter drops AppKit's `is` (R5). */
bool ns_toolbar_item_bordered(ns_toolbar_item *_Nonnull toolbar_item)
    API_AVAILABLE(macos(26.0));

/* Installs the item's target and action. `context` is yours and is never
 * copied or freed; `sender` is the item, borrowed for the call. A null
 * `action` uninstalls, and installing again replaces (R9). The callback runs
 * on the main thread. NSToolbarItem is not an NSControl, so this is the
 * item's own installer rather than one reached through an upcast.
 * syntonic-owned. */
void ns_toolbar_item_set_action(ns_toolbar_item *_Nonnull toolbar_item,
                                ns_action _Nullable action,
                                void *_Nullable context)
    API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_TOOLBAR_ITEM_H */

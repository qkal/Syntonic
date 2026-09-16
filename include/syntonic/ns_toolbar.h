/*
 * NSToolbar - the window toolbar, with a search item and an Add button (R16,
 * R9, F2, KTD8, KTD17).
 *
 * A toolbar holds no items of its own. It asks its callbacks struct for one,
 * by identifier, as it builds itself, and it asks the identifier lists for the
 * order those items go in. Hand the toolbar to a window with
 * ns_window_set_toolbar, and set the window's toolbar style
 * (NS_WINDOW_TOOLBAR_STYLE_UNIFIED is the main window's) - a toolbar with no
 * window builds nothing.
 *
 *     static const char *const identifiers[] = {
 *         NS_TOOLBAR_TOGGLE_SIDEBAR_ITEM_IDENTIFIER,
 *         NS_TOOLBAR_SIDEBAR_TRACKING_SEPARATOR_ITEM_IDENTIFIER,
 *         "dev.example.add",
 *         NS_TOOLBAR_FLEXIBLE_SPACE_ITEM_IDENTIFIER,
 *         "dev.example.search"};
 *
 *     static ns_toolbar_item *item_for(void *context, ns_toolbar *sender,
 *                                      const char *identifier, bool inserted) {
 *       if (strcmp(identifier, "dev.example.add") != 0) return NULL;
 *       ns_toolbar_item *item =
 *           ns_toolbar_item_create_with_item_identifier(identifier);
 *       ns_toolbar_item_set_label(item, "Add");
 *       return item; // owned; the library releases it, you do not
 *     }
 *
 *     ns_toolbar_callbacks callbacks = {
 *         .item_for_item_identifier_will_be_inserted_into_toolbar = item_for};
 *     ns_toolbar_set_callbacks(toolbar, &callbacks, identifiers, 5,
 *                              identifiers, 5, &model);
 *
 * WHO OWNS THE ITEM THE MEMBER RETURNS (R9). The handle the member returns is
 * owned: the library hands the item to AppKit and drops the reference once
 * AppKit has retained it. Return an ns_toolbar_item_create… result directly
 * and do not release it; the C side is left holding nothing. Returning null
 * means "no item for that identifier", which is not an error.
 *
 * WHY THE IDENTIFIER LISTS ARE NOT MEMBERS (KTD8). NSToolbarDelegate's other
 * two required methods only ever return a list of identifiers, so the wrapper
 * answers them itself from the arrays passed to ns_toolbar_set_callbacks and
 * drops them from the struct. docs/conventions.md's per-protocol table records
 * that.
 *
 * WHY THOSE ARRAYS ARE COPIED, WHEN AN ARRAY IN IS NORMALLY BORROWED (KTD17).
 * AppKit asks for the identifier lists after the install returns, repeatedly,
 * for as long as the toolbar exists - a caller's stack array would be a
 * dangling read on the first question. So this is the one named exception to
 * the borrow rule: the array and every string in it are copied during
 * ns_toolbar_set_callbacks, and the copy lives as long as the toolbar, or
 * until the next install replaces it. The caller's own array may go out of
 * scope the moment the call returns. Everything else about the boundary is
 * unchanged: the strings cross as UTF-8, and the count comes alongside the
 * pointer.
 *
 * STANDARD IDENTIFIERS. AppKit builds the items for its own identifiers - the
 * three in ns_toolbar_item.h among them - and never asks the struct for one.
 */

#ifndef SYNTONIC_NS_TOOLBAR_H
#define SYNTONIC_NS_TOOLBAR_H

#include <os/availability.h>
#include <stdbool.h>
#include <stdint.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_toolbar ns_toolbar;

/* Declared by ns_toolbar_item.h and repeated here so this header stands alone.
 * C has allowed a repeated typedef of the same type since C11. */
typedef struct ns_toolbar_item ns_toolbar_item;

/* NSToolbarDisplayMode. Icon only is the main window's. */
typedef enum ns_toolbar_display_mode : uint64_t {
  NS_TOOLBAR_DISPLAY_MODE_DEFAULT = 0,
  NS_TOOLBAR_DISPLAY_MODE_ICON_AND_LABEL = 1,
  NS_TOOLBAR_DISPLAY_MODE_ICON_ONLY = 2,
  NS_TOOLBAR_DISPLAY_MODE_LABEL_ONLY = 3,
} ns_toolbar_display_mode;

/*
 * NSToolbarDelegate, as a struct of function pointers. Its one member is
 * required; the protocol's other two required methods are served from the
 * identifier arrays ns_toolbar_set_callbacks takes and are not members at all
 * (KTD8). docs/conventions.md's per-protocol table is the source for both
 * halves. An unset required member is reported at install time in a debug
 * build, before AppKit ever asks for an item (R9).
 *
 * `context` is what you passed to ns_toolbar_set_callbacks, untouched and
 * never freed by the library; `sender` is the toolbar, borrowed for the
 * duration of the call. Every member runs on the main thread. syntonic-owned.
 */
typedef struct ns_toolbar_callbacks {
  /* required. -[NSToolbarDelegate
   * toolbar:itemForItemIdentifier:willBeInsertedIntoToolbar:] -
   * `item_identifier` is borrowed for the duration of the call; copy it if you
   * need it later. `will_be_inserted_into_toolbar` is false when the toolbar
   * is asking in order to build its customization palette rather than to show
   * the item.
   *
   * The returned handle is **owned**: the library hands the item to AppKit and
   * releases it once AppKit has retained it, so do not release it yourself
   * (R9). Null means there is no item for that identifier, which is an answer
   * and not an error. A handle that is not an ns_toolbar_item is reported in a
   * debug build (KTD8). */
  ns_toolbar_item *_Nullable (*_Nullable
      item_for_item_identifier_will_be_inserted_into_toolbar)(
      void *_Nullable context, ns_toolbar *_Nonnull sender,
      const char *_Nonnull item_identifier,
      bool will_be_inserted_into_toolbar);
} ns_toolbar_callbacks;

/* -[NSToolbar initWithIdentifier:] - owned (+1), released with ns_release
 * (R7). The identifier is what toolbars share state under, so give each
 * distinct toolbar a distinct one. */
ns_toolbar *_Nonnull ns_toolbar_create_with_identifier(
    const char *_Nonnull identifier) API_AVAILABLE(macos(26.0));

/* -[NSToolbar identifier] - a copy property, so this is an owned copy the
 * caller frees with ns_string_free (R7, R11). */
char *_Nonnull ns_toolbar_copy_identifier(ns_toolbar *_Nonnull toolbar)
    API_AVAILABLE(macos(26.0));

/* -[NSToolbar items] - the count half of the array shape: `items` is a copy
 * property, so the array itself never crosses (R11, KTD17). It is zero until
 * the toolbar belongs to a window, which is when AppKit builds the items from
 * the default identifiers. */
long ns_toolbar_item_count(ns_toolbar *_Nonnull toolbar)
    API_AVAILABLE(macos(26.0));

/* -[NSToolbar items] - the index half, in the default identifiers' order.
 * Borrowed: the toolbar holds the item, and ns_retain keeps it longer (R7). */
ns_toolbar_item *_Nonnull ns_toolbar_item_at_index(
    ns_toolbar *_Nonnull toolbar, long index) API_AVAILABLE(macos(26.0));

/* -[NSToolbar setAllowsUserCustomization:] - false is a toolbar whose items
 * the user cannot rearrange, which is what a fixed toolbar wants. */
void ns_toolbar_set_allows_user_customization(ns_toolbar *_Nonnull toolbar,
                                              bool allows_customization)
    API_AVAILABLE(macos(26.0));

/* -[NSToolbar allowsUserCustomization] */
bool ns_toolbar_allows_user_customization(ns_toolbar *_Nonnull toolbar)
    API_AVAILABLE(macos(26.0));

/* -[NSToolbar setDisplayMode:] */
void ns_toolbar_set_display_mode(ns_toolbar *_Nonnull toolbar,
                                 ns_toolbar_display_mode display_mode)
    API_AVAILABLE(macos(26.0));

/* -[NSToolbar displayMode] - `get_`, because ns_toolbar_display_mode is
 * already a type name and C puts the two in one namespace (R5). */
ns_toolbar_display_mode ns_toolbar_get_display_mode(
    ns_toolbar *_Nonnull toolbar) API_AVAILABLE(macos(26.0));

/* Installs the callbacks struct as the toolbar's delegate, together with the
 * two identifier lists the wrapper answers the protocol's other required
 * methods from (KTD8).
 *
 * `default_item_identifiers` is the order the items appear in;
 * `allowed_item_identifiers` is every identifier the toolbar will accept, and
 * should at least contain the default ones. Both arrays and every string in
 * them are **copied** during this call and the copy lives as long as the
 * toolbar or until the next install - the named exception to KTD17's borrow
 * rule, because AppKit asks for these lists long after the install returns.
 * The caller's array may go out of scope immediately.
 *
 * The struct is copied too; `context` is not, and has to stay valid until you
 * uninstall or the toolbar is deallocated - ns_release does not uninstall.
 * Installing again replaces the struct, the context and both identifier lists.
 * A null `callbacks` uninstalls and drops the lists, and the identifier
 * arguments are ignored. An unset required member is reported in a debug build
 * before the delegate slot is assigned (R9, KTD8). syntonic-owned. */
void ns_toolbar_set_callbacks(
    ns_toolbar *_Nonnull toolbar,
    const ns_toolbar_callbacks *_Nullable callbacks,
    const char *_Nonnull const *_Nullable default_item_identifiers,
    long default_item_identifier_count,
    const char *_Nonnull const *_Nullable allowed_item_identifiers,
    long allowed_item_identifier_count, void *_Nullable context)
    API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_TOOLBAR_H */

/*
 * NSSearchToolbarItem - the toolbar's search field, with the standard
 * expand-on-focus behaviour a macOS search toolbar item has (R16, F2).
 *
 * The item owns the field: ask for it with ns_search_toolbar_item_search_field
 * and configure it through ns_search_field.h and the upcasts that reach its
 * inherited API (R5).
 *
 *     ns_search_toolbar_item *item =
 *         ns_search_toolbar_item_create_with_item_identifier(identifier);
 *     ns_search_field *field = ns_search_toolbar_item_search_field(item);
 *     ns_search_field_set_sends_search_string_immediately(field, true);
 *     ns_control_set_action(ns_search_field_as_control(field), on_search,
 *                           &model);
 *
 * and in the action, the text the user typed (F2):
 *
 *     static void on_search(void *context, const void *sender) {
 *       char *text = ns_control_copy_string_value((ns_control *)sender);
 *       ...
 *       ns_string_free(text);
 *     }
 *
 * `sender` is the search field, not the item: AppKit sends a control's action
 * from the control. The action fires as the user types, which is what makes
 * the list filter live.
 */

#ifndef SYNTONIC_NS_SEARCH_TOOLBAR_ITEM_H
#define SYNTONIC_NS_SEARCH_TOOLBAR_ITEM_H

#include <os/availability.h>
#include <stdbool.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_search_toolbar_item ns_search_toolbar_item;

/* Declared by ns_toolbar_item.h and ns_search_field.h and repeated here so
 * this header stands alone. C has allowed a repeated typedef of the same type
 * since C11. */
typedef struct ns_toolbar_item ns_toolbar_item;
typedef struct ns_search_field ns_search_field;

/* -[NSToolbarItem initWithItemIdentifier:] - owned (+1), released with
 * ns_release (R7). A constructor is the one inherited member a subclass
 * redeclares, because an upcast needs an object to upcast. Return it straight
 * from the toolbar's callbacks struct, upcast with
 * ns_search_toolbar_item_as_toolbar_item, and do not release it yourself (R9). */
ns_search_toolbar_item *_Nonnull
ns_search_toolbar_item_create_with_item_identifier(
    const char *_Nonnull item_identifier) API_AVAILABLE(macos(26.0));

/* -[NSSearchToolbarItem searchField] - a strong property, so this is borrowed:
 * valid while the item holds it, kept longer with ns_retain (R7). The item
 * manages the field's layout, so set its action and read its text, but leave
 * its frame alone. The SDK declares this property an NSSearchField, so that is
 * what crosses; reach NSTextField's own API through
 * ns_search_field_as_text_field rather than through an implicit upcast (R5). */
ns_search_field *_Nonnull ns_search_toolbar_item_search_field(
    ns_search_toolbar_item *_Nonnull search_toolbar_item)
    API_AVAILABLE(macos(26.0));

/* -[NSSearchToolbarItem setResignsFirstResponderWithCancel:] - true is the
 * default: the field's cancel button clears the text and gives up focus. */
void ns_search_toolbar_item_set_resigns_first_responder_with_cancel(
    ns_search_toolbar_item *_Nonnull search_toolbar_item, bool resigns)
    API_AVAILABLE(macos(26.0));

/* -[NSSearchToolbarItem resignsFirstResponderWithCancel] */
bool ns_search_toolbar_item_resigns_first_responder_with_cancel(
    ns_search_toolbar_item *_Nonnull search_toolbar_item)
    API_AVAILABLE(macos(26.0));

/* -[NSSearchToolbarItem beginSearchInteraction] - expands the item to its
 * preferred width and moves focus into the field, which is what an Edit - Find
 * command does. */
void ns_search_toolbar_item_begin_search_interaction(
    ns_search_toolbar_item *_Nonnull search_toolbar_item)
    API_AVAILABLE(macos(26.0));

/* -[NSSearchToolbarItem endSearchInteraction] - gives up focus and lets the
 * item shrink back. */
void ns_search_toolbar_item_end_search_interaction(
    ns_search_toolbar_item *_Nonnull search_toolbar_item)
    API_AVAILABLE(macos(26.0));

/* The upcast to ns_toolbar_item: the same pointer, checked in a debug build
 * (R4, KTD9). This is what the toolbar's callbacks struct returns.
 * syntonic-owned. */
ns_toolbar_item *_Nonnull ns_search_toolbar_item_as_toolbar_item(
    ns_search_toolbar_item *_Nonnull search_toolbar_item)
    API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_SEARCH_TOOLBAR_ITEM_H */

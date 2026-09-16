/*
 * NSSearchField - the field inside a search toolbar item, and the two
 * properties that decide when its action fires (R16, F2).
 *
 * WHERE ONE COMES FROM. v0 creates no search field of its own: the toolbar's
 * search item makes one and keeps it, and ns_search_toolbar_item_search_field
 * hands it back borrowed.
 *
 *     ns_search_field *field = ns_search_toolbar_item_search_field(item);
 *     ns_search_field_set_sends_whole_search_string(field, false);
 *     ns_search_field_set_sends_search_string_immediately(field, true);
 *     ns_control_set_action(ns_search_field_as_control(field), on_search, &m);
 *
 * WHEN THE ACTION FIRES. The two properties are one dial with three settings,
 * and the defaults are both false:
 *
 *   | sends_whole_search_string | sends_search_string_immediately | the action |
 *   |---|---|---|
 *   | false | false | after AppKit's own typing pause - the default |
 *   | false | true  | on every keystroke, with no pause |
 *   | true  | -     | only on Return or the magnifying-glass button |
 *
 * A list that filters as the user types wants the middle row, which is what
 * both twins set. The field a search toolbar item makes for itself arrives on
 * that row already - measured on macOS 27 - while a field built any other way
 * does not, so a program that means the middle row says so rather than relying
 * on where its field came from.
 *
 * The text itself is NSControl's `stringValue`, read through the control
 * upcast with ns_control_copy_string_value, and everything NSTextField
 * declares is reached through the text field upcast (R5).
 */

#ifndef SYNTONIC_NS_SEARCH_FIELD_H
#define SYNTONIC_NS_SEARCH_FIELD_H

#include <os/availability.h>
#include <stdbool.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_search_field ns_search_field;

/* Declared by ns_text_field.h, ns_control.h and ns_view.h and repeated here so
 * this header stands alone. C has allowed a repeated typedef of the same type
 * since C11. */
typedef struct ns_text_field ns_text_field;
typedef struct ns_control ns_control;
typedef struct ns_view ns_view;

/* -[NSSearchField setSendsWholeSearchString:] - true holds the action back
 * until Return or the magnifying-glass button; false, the default, sends it as
 * the user types. */
void ns_search_field_set_sends_whole_search_string(
    ns_search_field *_Nonnull search_field, bool sends_whole_search_string)
    API_AVAILABLE(macos(26.0));

/* -[NSSearchField sendsWholeSearchString] */
bool ns_search_field_sends_whole_search_string(
    ns_search_field *_Nonnull search_field) API_AVAILABLE(macos(26.0));

/* -[NSSearchField setSendsSearchStringImmediately:] - true sends the action on
 * every keystroke with no pause in front of it. The default is false, which is
 * AppKit's typing delay, so a live-filtering list has to set this. */
void ns_search_field_set_sends_search_string_immediately(
    ns_search_field *_Nonnull search_field, bool sends_immediately)
    API_AVAILABLE(macos(26.0));

/* -[NSSearchField sendsSearchStringImmediately] */
bool ns_search_field_sends_search_string_immediately(
    ns_search_field *_Nonnull search_field) API_AVAILABLE(macos(26.0));

/* The upcast to ns_text_field: the same pointer, checked in a debug build (R4,
 * KTD9). This is where the field's placeholder and its delegate struct live.
 * syntonic-owned. */
ns_text_field *_Nonnull ns_search_field_as_text_field(
    ns_search_field *_Nonnull search_field) API_AVAILABLE(macos(26.0));

/* The upcast to ns_control: the same pointer, checked in a debug build (R4,
 * KTD9). This is where the field's text and its action live. syntonic-owned. */
ns_control *_Nonnull ns_search_field_as_control(
    ns_search_field *_Nonnull search_field) API_AVAILABLE(macos(26.0));

/* The upcast to ns_view: the same pointer, checked in a debug build (R4,
 * KTD9). syntonic-owned. */
ns_view *_Nonnull ns_search_field_as_view(
    ns_search_field *_Nonnull search_field) API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_SEARCH_FIELD_H */

/*
 * NSTextField - both halves of it: the static label beside a form row, and the
 * editable field the caller reads back (R17).
 *
 * Which one you get is which constructor you call.
 * ns_text_field_create_label_with_string makes a label: not editable, no
 * bezel, no background. ns_text_field_create_with_string makes an editable
 * field. AppKit has no property that converts one into the other after the
 * fact, and neither does Syntonic.
 *
 * The text itself is NSControl's `stringValue`, so it is read and written
 * through the control upcast: ns_control_copy_string_value hands back an owned
 * copy the caller frees with ns_string_free (R7, R11).
 *
 * COMMITTING AN EDIT (R17, F6). A field's action - installed with
 * ns_control_set_action on the control upcast - fires when editing ends: on
 * Return, on Tab, and when the field loses focus. That is the commit point the
 * twins use. The delegate struct below is the finer-grained view of the same
 * thing: `did_change` on every keystroke, `did_end_editing` when the editing
 * session closes.
 */

#ifndef SYNTONIC_NS_TEXT_FIELD_H
#define SYNTONIC_NS_TEXT_FIELD_H

#include <os/availability.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_text_field ns_text_field;

/* Declared by ns_control.h and ns_view.h and repeated here so this header
 * stands alone. C has allowed a repeated typedef of the same type since C11. */
typedef struct ns_control ns_control;
typedef struct ns_view ns_view;

/*
 * NSTextFieldDelegate, as a struct of function pointers. Every member is
 * optional; docs/conventions.md's per-protocol table is what says so. An unset
 * member behaves exactly as a method the delegate does not implement, so a
 * field installed with an empty struct edits and ends editing as an
 * undelegated one does.
 *
 * Both members carry the editing notifications NSControlTextEditingDelegate
 * declares, which NSTextFieldDelegate inherits. `context` is what you passed
 * to ns_text_field_set_callbacks, untouched and never freed by the library;
 * `sender` is the field, borrowed for the duration of the call. Read the new
 * text with ns_control_copy_string_value on the sender's control upcast. Every
 * member runs on the main thread. syntonic-owned.
 */
typedef struct ns_text_field_callbacks {
  /* optional. -[NSControlTextEditingDelegate controlTextDidChange:] - every
   * keystroke. The notification carries nothing beyond the field, so the field
   * is what crosses (R5). */
  void (*_Nullable did_change)(void *_Nullable context,
                               ns_text_field *_Nonnull sender);
  /* optional. -[NSControlTextEditingDelegate controlTextDidEndEditing:] - the
   * editing session closed, which is the commit point (R17, F6). */
  void (*_Nullable did_end_editing)(void *_Nullable context,
                                    ns_text_field *_Nonnull sender);
} ns_text_field_callbacks;

/* +[NSTextField labelWithString:] - owned (+1), released with ns_release (R7).
 * A label: not editable, not bezeled, no background. */
ns_text_field *_Nonnull ns_text_field_create_label_with_string(
    const char *_Nonnull value) API_AVAILABLE(macos(26.0));

/* +[NSTextField textFieldWithString:] - owned (+1), released with ns_release
 * (R7). An editable, bezeled field. */
ns_text_field *_Nonnull ns_text_field_create_with_string(
    const char *_Nonnull value) API_AVAILABLE(macos(26.0));

/* -[NSTextField setPlaceholderString:] - the grey text an empty field shows;
 * null clears it. */
void ns_text_field_set_placeholder_string(ns_text_field *_Nonnull text_field,
                                          const char *_Nullable placeholder)
    API_AVAILABLE(macos(26.0));

/* -[NSTextField placeholderString] - a copy property, so this is an owned copy
 * the caller frees with ns_string_free (R7, R11). */
char *_Nullable ns_text_field_copy_placeholder_string(
    ns_text_field *_Nonnull text_field) API_AVAILABLE(macos(26.0));

/* Installs the callbacks struct as the field's delegate. The struct is copied;
 * `context` is not, and has to stay valid until you uninstall or the field is
 * deallocated - ns_release does not uninstall. Installing again replaces the
 * previous struct and context; a null `callbacks` uninstalls (R9, KTD8).
 * syntonic-owned. */
void ns_text_field_set_callbacks(
    ns_text_field *_Nonnull text_field,
    const ns_text_field_callbacks *_Nullable callbacks, void *_Nullable context)
    API_AVAILABLE(macos(26.0));

/* The upcast to ns_control: the same pointer, checked in a debug build (R4,
 * KTD9). This is where the field's text, its enabled flag and its
 * end-of-editing action live. syntonic-owned. */
ns_control *_Nonnull ns_text_field_as_control(
    ns_text_field *_Nonnull text_field) API_AVAILABLE(macos(26.0));

/* The upcast to ns_view: the same pointer, checked in a debug build (R4,
 * KTD9). syntonic-owned. */
ns_view *_Nonnull ns_text_field_as_view(ns_text_field *_Nonnull text_field)
    API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_TEXT_FIELD_H */

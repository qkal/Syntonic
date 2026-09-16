/*
 * NSControl - what every control in Syntonic inherits: an enabled flag, a
 * string value, the target/action installer, and the two ways to fire an
 * action (R4, R9, R17).
 *
 * NSControl is a base class, not something to create. A button, a text field
 * or a pop-up reaches everything here through its own upcast -
 * ns_button_as_control(button) - which is R4's promise that the inherited API
 * is one function, declared once, on the class the SDK declares it on.
 *
 * A target/action is a function pointer plus a context (R9): install it with
 * ns_control_set_action, and it runs on the main thread with the handle of the
 * control that fired. AppKit's own `target` and `action` are an `id` and a
 * `SEL`, neither of which crosses the boundary (R11), so the installer is the
 * only way to give a control an action and ns_control_send_action is how a
 * program fires the one that is installed.
 */

#ifndef SYNTONIC_NS_CONTROL_H
#define SYNTONIC_NS_CONTROL_H

#include <os/availability.h>
#include <stdbool.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_control ns_control;

/* Declared by ns_view.h and repeated here so this header stands alone. C has
 * allowed a repeated typedef of the same type since C11. */
typedef struct ns_view ns_view;

/* -[NSControl setEnabled:] - a disabled control is greyed out and fires no
 * action. */
void ns_control_set_enabled(ns_control *_Nonnull control, bool enabled)
    API_AVAILABLE(macos(26.0));

/* -[NSControl isEnabled] - a BOOL getter drops AppKit's `is` (R5). */
bool ns_control_enabled(ns_control *_Nonnull control)
    API_AVAILABLE(macos(26.0));

/* -[NSControl setStringValue:] - the text a control displays: a text field's
 * contents, a button's title. */
void ns_control_set_string_value(ns_control *_Nonnull control,
                                 const char *_Nonnull value)
    API_AVAILABLE(macos(26.0));

/* -[NSControl stringValue] - `stringValue` is a copy property, so this is an
 * owned copy the caller frees with ns_string_free (R7, R11). */
char *_Nullable ns_control_copy_string_value(ns_control *_Nonnull control)
    API_AVAILABLE(macos(26.0));

/* -[NSControl performClick:] - the click a user would make: a checkbox takes
 * its next state and the action fires. AppKit's sender argument is an `id` and
 * never crosses the boundary (R11), so the library passes nil. A pop-up button
 * opens its menu here and waits for the user, so fire that one with
 * ns_control_send_action instead. */
void ns_control_perform_click(ns_control *_Nonnull control)
    API_AVAILABLE(macos(26.0));

/* -[NSControl sendAction:to:] - sends the control's own action to its own
 * target, which is the message a click sends and the one a pop-up sends when
 * an item is picked. Both of AppKit's segments are types that never cross - a
 * SEL and an `id` (R11) - so the wrapper supplies the control's own pair and
 * the name drops them. True when an action was installed and it fired. */
bool ns_control_send_action(ns_control *_Nonnull control)
    API_AVAILABLE(macos(26.0));

/* Installs the target/action callback: `action` runs on the main thread with
 * `context` and this control as the sender. A null `action` uninstalls.
 * `context` is neither copied nor freed and has to outlive the installation.
 * syntonic-owned. */
void ns_control_set_action(ns_control *_Nonnull control,
                           ns_action _Nullable action, void *_Nullable context)
    API_AVAILABLE(macos(26.0));

/* The upcast to ns_view: the same pointer, checked in a debug build (R4,
 * KTD9). syntonic-owned. */
ns_view *_Nonnull ns_control_as_view(ns_control *_Nonnull control)
    API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_CONTROL_H */

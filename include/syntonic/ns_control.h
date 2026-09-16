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

/* Declared by ns_view.h and ns_font.h and repeated here so this header stands
 * alone. C has allowed a repeated typedef of the same type since C11. */
typedef struct ns_view ns_view;
typedef struct ns_font ns_font;

/* NSLineBreakMode, which is what a control does with text too long for the
 * space it has. It belongs to Foundation's NSParagraphStyle.h, a header no
 * unit wraps, so it is declared here where it is used and named as though its
 * own header existed (KTD18). A list cell and a sidebar row are both
 * `TRUNCATING_TAIL`: the beginning fits and an ellipsis ends the line. */
typedef enum ns_line_break_mode : uint64_t {
  NS_LINE_BREAK_BY_WORD_WRAPPING = 0,
  NS_LINE_BREAK_BY_CHAR_WRAPPING = 1,
  NS_LINE_BREAK_BY_CLIPPING = 2,
  NS_LINE_BREAK_BY_TRUNCATING_HEAD = 3,
  NS_LINE_BREAK_BY_TRUNCATING_TAIL = 4,
  NS_LINE_BREAK_BY_TRUNCATING_MIDDLE = 5,
} ns_line_break_mode;

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
char *_Nonnull ns_control_copy_string_value(ns_control *_Nonnull control)
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

/* -[NSControl setFont:] - the type a control draws its text in, which is where
 * a label's bold header comes from (R17). The control copies the font, so
 * releasing your own reference afterwards is correct; null restores AppKit's
 * own default. */
void ns_control_set_font(ns_control *_Nonnull control, ns_font *_Nullable font)
    API_AVAILABLE(macos(26.0));

/* -[NSControl font] - `font` is a copy property, so this is owned: release it
 * with ns_release (R7). */
ns_font *_Nullable ns_control_copy_font(ns_control *_Nonnull control)
    API_AVAILABLE(macos(26.0));

/* -[NSControl setLineBreakMode:] - what the control does with text too long
 * for its width. A label in a list cell or a sidebar row is set to
 * NS_LINE_BREAK_BY_TRUNCATING_TAIL, which is what makes a long title end in an
 * ellipsis instead of pushing the column wider (R17). */
void ns_control_set_line_break_mode(ns_control *_Nonnull control,
                                    ns_line_break_mode line_break_mode)
    API_AVAILABLE(macos(26.0));

/* -[NSControl lineBreakMode] */
ns_line_break_mode ns_control_line_break_mode(ns_control *_Nonnull control)
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

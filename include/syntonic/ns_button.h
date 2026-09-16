/*
 * NSButton - the push button and the checkbox the detail pane and the settings
 * panes are made of (R17).
 *
 * AppKit builds both with a convenience constructor whose `target:` and
 * `action:` segments are an `id` and a `SEL`, neither of which crosses the
 * boundary (R11). Those segments are dropped from the name, so a button is
 * created with no action and ns_control_set_action - reached through
 * ns_button_as_control - is what gives it one, exactly as a menu item works.
 *
 * A checkbox carries its state in ns_button_state and takes its next state on
 * every click, so a click both flips the box and fires the action (R17: the
 * checkbox commits immediately). A push button's state never changes.
 *
 * ACCESSIBILITY (R23). A titled button already reads as its title, because
 * AppKit infers the label from it and Syntonic does not touch the property.
 * Give a button with no title of its own a label through
 * ns_view_set_accessibility_label on its view upcast.
 */

#ifndef SYNTONIC_NS_BUTTON_H
#define SYNTONIC_NS_BUTTON_H

#include <os/availability.h>
#include <stdint.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_button ns_button;

/* Declared by ns_control.h and ns_view.h and repeated here so this header
 * stands alone. C has allowed a repeated typedef of the same type since C11. */
typedef struct ns_control ns_control;
typedef struct ns_view ns_view;

/* NSControlStateValue, the state a checkbox or radio button carries. It
 * belongs to NSCell.h, which v0 does not wrap, and a button is the only place
 * it is needed - so it is declared here, where it is used. A button reaches
 * the mixed state only when AppKit's `allowsMixedState` is on, which v0 does
 * not wrap either; a checkbox toggles between off and on. */
typedef enum ns_control_state_value : int64_t {
  NS_CONTROL_STATE_VALUE_MIXED = -1,
  NS_CONTROL_STATE_VALUE_OFF = 0,
  NS_CONTROL_STATE_VALUE_ON = 1,
} ns_control_state_value;

/* NSBezelStyle, the bezel artwork and metrics a bordered button draws with.
 * It belongs to NSButtonCell.h, which v0 does not wrap, so it is declared
 * here. AppKit's deprecated spellings are left out; NS_BEZEL_STYLE_PUSH is the
 * standard push button that NSBezelStyleRounded is now an alias for. */
typedef enum ns_bezel_style : uint64_t {
  NS_BEZEL_STYLE_AUTOMATIC = 0,
  NS_BEZEL_STYLE_PUSH = 1,
  NS_BEZEL_STYLE_FLEXIBLE_PUSH = 2,
  NS_BEZEL_STYLE_DISCLOSURE = 5,
  NS_BEZEL_STYLE_CIRCULAR = 7,
  NS_BEZEL_STYLE_HELP_BUTTON = 9,
  NS_BEZEL_STYLE_SMALL_SQUARE = 10,
  NS_BEZEL_STYLE_TOOLBAR = 11,
  NS_BEZEL_STYLE_ACCESSORY_BAR_ACTION = 12,
  NS_BEZEL_STYLE_ACCESSORY_BAR = 13,
  NS_BEZEL_STYLE_PUSH_DISCLOSURE = 14,
  NS_BEZEL_STYLE_BADGE = 15,
  NS_BEZEL_STYLE_GLASS = 16,
} ns_bezel_style;

/* +[NSButton buttonWithTitle:target:action:] - owned (+1), released with
 * ns_release (R7). The `target:` and `action:` segments are an `id` and a SEL
 * and never cross (R11), so the button is created with no action and
 * ns_control_set_action installs one. */
ns_button *_Nonnull ns_button_create_with_title(const char *_Nonnull title)
    API_AVAILABLE(macos(26.0));

/* +[NSButton checkboxWithTitle:target:action:] - owned (+1), released with
 * ns_release (R7). The dropped segments are the same two as above. */
ns_button *_Nonnull ns_button_create_checkbox_with_title(
    const char *_Nonnull title) API_AVAILABLE(macos(26.0));

/* -[NSButton setTitle:] */
void ns_button_set_title(ns_button *_Nonnull button, const char *_Nonnull title)
    API_AVAILABLE(macos(26.0));

/* -[NSButton title] - `title` is a copy property, so this is an owned copy the
 * caller frees with ns_string_free (R7, R11). */
char *_Nullable ns_button_copy_title(ns_button *_Nonnull button)
    API_AVAILABLE(macos(26.0));

/* -[NSButton setState:] */
void ns_button_set_state(ns_button *_Nonnull button,
                         ns_control_state_value state)
    API_AVAILABLE(macos(26.0));

/* -[NSButton state] */
ns_control_state_value ns_button_state(ns_button *_Nonnull button)
    API_AVAILABLE(macos(26.0));

/* -[NSButton setBezelStyle:] - ignored while the button draws no border. */
void ns_button_set_bezel_style(ns_button *_Nonnull button,
                               ns_bezel_style style) API_AVAILABLE(macos(26.0));

/* -[NSButton bezelStyle] */
ns_bezel_style ns_button_bezel_style(ns_button *_Nonnull button)
    API_AVAILABLE(macos(26.0));

/* The upcast to ns_control: the same pointer, checked in a debug build (R4,
 * KTD9). This is where a button's action, its enabled flag and its string
 * value live. syntonic-owned. */
ns_control *_Nonnull ns_button_as_control(ns_button *_Nonnull button)
    API_AVAILABLE(macos(26.0));

/* The upcast to ns_view: the same pointer, checked in a debug build (R4,
 * KTD9). syntonic-owned. */
ns_view *_Nonnull ns_button_as_view(ns_button *_Nonnull button)
    API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_BUTTON_H */

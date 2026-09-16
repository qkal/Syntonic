/*
 * NSColor - the semantic colours a programmatically built app names instead of
 * a literal, so text and tint follow the system appearance (R17).
 *
 * This is deliberately a corner of NSColor, not the class: v0 has no colour
 * component, no colour space and no literal-colour constructor. Every colour
 * here is one AppKit resolves against the current appearance, which is what
 * makes light and dark look right without the caller branching.
 *
 * OWNERSHIP (R7). Each one is a `strong` class property, not a constructor and
 * not a method: AppKit holds the shared colour for the life of the process, so
 * these are borrowed and take neither create_ nor copy_ in their names, the
 * same reasoning that makes ns_application_shared borrowed. Keep one past the
 * call with ns_retain if you have a reason to; there is none in practice.
 *
 * A text field's colour is NSTextField's `textColor` - see
 * ns_text_field_set_text_color, whose setter copies, so the shared colour
 * stays shared.
 */

#ifndef SYNTONIC_NS_COLOR_H
#define SYNTONIC_NS_COLOR_H

#include <os/availability.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_color ns_color;

/* +[NSColor labelColor] - the foreground colour for static text. Borrowed:
 * AppKit holds it for the life of the process (R7). */
ns_color *_Nonnull ns_color_label_color(void) API_AVAILABLE(macos(26.0));

/* +[NSColor secondaryLabelColor] - the foreground colour for secondary static
 * text. Borrowed (R7). */
ns_color *_Nonnull ns_color_secondary_label_color(void)
    API_AVAILABLE(macos(26.0));

/* +[NSColor controlAccentColor] - the user's chosen accent colour, which is
 * what a tinted icon follows. Borrowed (R7). */
ns_color *_Nonnull ns_color_control_accent_color(void)
    API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_COLOR_H */

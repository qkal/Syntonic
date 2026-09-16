/*
 * NSFont - the minimum a programmatically built app needs to set type on a
 * label: the system font and its bold companion at a size, and the standard
 * system font size to ask for them at (R17).
 *
 * This is deliberately a corner of NSFont, not the class: v0 has no font
 * descriptor, no weight or width axis, and no font panel. Naming a different
 * face is outside v0's surface.
 *
 * OWNERSHIP (R7). +[NSFont systemFontOfSize:] is a plain class method
 * returning an object, not an `instancetype` constructor and not a property,
 * so what it hands back is owned and the name says copy_: release it with
 * ns_release. Setting it on a control retains it, so releasing your own
 * reference afterwards is correct.
 *
 * The font a control carries is NSControl's `font`, so it is set and read
 * through the control upcast: ns_control_set_font and ns_control_copy_font.
 */

#ifndef SYNTONIC_NS_FONT_H
#define SYNTONIC_NS_FONT_H

#include <CoreGraphics/CGGeometry.h>
#include <os/availability.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_font ns_font;

/* +[NSFont systemFontOfSize:] - owned, released with ns_release (R7). */
ns_font *_Nonnull ns_font_copy_system_font_of_size(CGFloat size)
    API_AVAILABLE(macos(26.0));

/* +[NSFont boldSystemFontOfSize:] - owned, released with ns_release (R7). */
ns_font *_Nonnull ns_font_copy_bold_system_font_of_size(CGFloat size)
    API_AVAILABLE(macos(26.0));

/* +[NSFont systemFontSize] - the size a standard label is set at, which is
 * what to pass the two constructors above. */
CGFloat ns_font_system_font_size(void) API_AVAILABLE(macos(26.0));

/* -[NSFont fontName] - a copy property, so this is an owned copy the caller
 * frees with ns_string_free (R7, R11). */
char *_Nonnull ns_font_copy_font_name(ns_font *_Nonnull font)
    API_AVAILABLE(macos(26.0));

/* -[NSFont pointSize] */
CGFloat ns_font_point_size(ns_font *_Nonnull font) API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_FONT_H */

/*
 * NSImage - an SF Symbol image, which is the only kind of image v0 makes
 * (R19, R23).
 *
 * A tab item carries one for its toolbar tab, and a toolbar item carries one
 * for a button with no text of its own. Both are the image-only case R23
 * names, so the constructor takes the accessibility description alongside the
 * symbol name: it is what a screen reader reads where there is no title to
 * infer a label from, and a nib would have carried it the same way.
 *
 * Nothing else about NSImage is wrapped. A file image, a named asset, a symbol
 * configuration and the drawing methods have no consumer in v0; the unit that
 * needs one adds it here.
 */

#ifndef SYNTONIC_NS_IMAGE_H
#define SYNTONIC_NS_IMAGE_H

#include <os/availability.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_image ns_image;

/* +[NSImage imageWithSystemSymbolName:accessibilityDescription:] - owned (+1),
 * released with ns_release (R7). Null when the running system has no symbol of
 * that name: AppKit answers nil rather than raising, so an unknown name is the
 * caller's to handle and not misuse (R12). `accessibility_description` is what
 * assistive technology reads for a control the image is the whole of (R23);
 * null does not leave the image undescribed - AppKit substitutes the symbol's
 * own localized name, so a "gearshape" with no description reads as "Gear
 * shape" rather than as nothing. Pass the label the control would have had. */
ns_image *_Nullable
ns_image_create_with_system_symbol_name_accessibility_description(
    const char *_Nonnull name,
    const char *_Nullable accessibility_description)
    API_AVAILABLE(macos(26.0));

/* -[NSImage accessibilityDescription] - a copy property, so this is an owned
 * copy the caller frees with ns_string_free (R7, R11). */
char *_Nullable ns_image_copy_accessibility_description(
    ns_image *_Nonnull image) API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_IMAGE_H */

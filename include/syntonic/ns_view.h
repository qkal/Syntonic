/*
 * NSView - the rectangle every control is built on: a frame, an autoresizing
 * mask, a hidden flag, a subview list, and the accessibility label and role a
 * programmatically built app names by hand (R4, R17, R23).
 *
 * A view is added to a parent with ns_view_add_subview, which retains it, so
 * releasing your own reference afterwards is correct and the view stays in the
 * tree until the parent goes away (R7, AE2).
 *
 * `subviews` is a copy property in the SDK, so it never crosses as a handle:
 * it is a count function plus an index accessor whose element is borrowed from
 * the receiver (KTD7, KTD17).
 *
 * ACCESSIBILITY (R23). Syntonic never touches an accessibility property unless
 * the caller asks, so AppKit's own inference - a button labelled from its
 * title - survives untouched. The two setters below are for what a nib would
 * have named and a program cannot infer: the text field beside a "Title:"
 * label has nothing of its own to be called, and neither has a pop-up. A role
 * crosses as the UTF-8 spelling of AppKit's own constant, "AXButton" and its
 * kind (R11).
 *
 * Setting one of these replaces AppKit's inference rather than adding to it,
 * and setting null replaces it with nothing: AppKit does not restore the
 * inferred value afterwards. Set a label where there is none to infer, and
 * leave the property alone everywhere else.
 */

#ifndef SYNTONIC_NS_VIEW_H
#define SYNTONIC_NS_VIEW_H

#include <CoreGraphics/CGGeometry.h>
#include <os/availability.h>
#include <stdbool.h>
#include <stdint.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_view ns_view;

/* NSAutoresizingMaskOptions. What a view does when its superview resizes;
 * combine the flags with the bitwise or. A new view is not sizable at all,
 * which is AppKit's own default. */
typedef enum ns_autoresizing_mask_options : uint64_t {
  NS_VIEW_NOT_SIZABLE = 0,
  NS_VIEW_MIN_X_MARGIN = 1,
  NS_VIEW_WIDTH_SIZABLE = 2,
  NS_VIEW_MAX_X_MARGIN = 4,
  NS_VIEW_MIN_Y_MARGIN = 8,
  NS_VIEW_HEIGHT_SIZABLE = 16,
  NS_VIEW_MAX_Y_MARGIN = 32,
} ns_autoresizing_mask_options;

/* -[NSView initWithFrame:] - owned (+1), released with ns_release (R7). */
ns_view *_Nonnull ns_view_create_with_frame(CGRect frame)
    API_AVAILABLE(macos(26.0));

/* -[NSView addSubview:] - the view retains the subview, so releasing your own
 * reference afterwards is correct: the subview stays in the tree until the
 * parent goes away (R7, AE2). */
void ns_view_add_subview(ns_view *_Nonnull view, ns_view *_Nonnull subview)
    API_AVAILABLE(macos(26.0));

/* -[NSView removeFromSuperview] - the superview gives up its reference. A view
 * with no other holder is deallocated here, so retain it first if you mean to
 * put it back. */
void ns_view_remove_from_superview(ns_view *_Nonnull view)
    API_AVAILABLE(macos(26.0));

/* -[NSView subviews] - the count half of the array shape: `subviews` is a copy
 * property and never crosses as a handle (R7, KTD17). */
long ns_view_subview_count(ns_view *_Nonnull view) API_AVAILABLE(macos(26.0));

/* -[NSView subviews] - the index half, in insertion order. Borrowed: the
 * element is read through the receiver, which holds it; keep it longer with
 * ns_retain (R7, KTD17). */
ns_view *_Nonnull ns_view_subview_at_index(ns_view *_Nonnull view, long index)
    API_AVAILABLE(macos(26.0));

/* -[NSView superview] - an unretained property, so this is borrowed: valid
 * while the superview holds this view, kept longer with ns_retain (R7). */
ns_view *_Nullable ns_view_superview(ns_view *_Nonnull view)
    API_AVAILABLE(macos(26.0));

/* -[NSView setFrame:] - in the superview's coordinates. */
void ns_view_set_frame(ns_view *_Nonnull view, CGRect frame)
    API_AVAILABLE(macos(26.0));

/* -[NSView frame] */
CGRect ns_view_frame(ns_view *_Nonnull view) API_AVAILABLE(macos(26.0));

/* -[NSView setAutoresizingMask:] */
void ns_view_set_autoresizing_mask(ns_view *_Nonnull view,
                                   ns_autoresizing_mask_options mask)
    API_AVAILABLE(macos(26.0));

/* -[NSView autoresizingMask] */
ns_autoresizing_mask_options ns_view_autoresizing_mask(ns_view *_Nonnull view)
    API_AVAILABLE(macos(26.0));

/* -[NSView setHidden:] */
void ns_view_set_hidden(ns_view *_Nonnull view, bool hidden)
    API_AVAILABLE(macos(26.0));

/* -[NSView isHidden] - a BOOL getter drops AppKit's `is` (R5). */
bool ns_view_hidden(ns_view *_Nonnull view) API_AVAILABLE(macos(26.0));

/* -[NSView setAccessibilityLabel:] - the name assistive technology reads.
 * Leave it alone and AppKit infers one from the control's title; setting one
 * replaces that inference and null replaces it with nothing (R23). */
void ns_view_set_accessibility_label(ns_view *_Nonnull view,
                                     const char *_Nullable label)
    API_AVAILABLE(macos(26.0));

/* -[NSView accessibilityLabel] - a copy property, so this is an owned copy the
 * caller frees with ns_string_free (R7, R11). Null when neither the caller nor
 * AppKit has one. */
char *_Nullable ns_view_copy_accessibility_label(ns_view *_Nonnull view)
    API_AVAILABLE(macos(26.0));

/* -[NSView setAccessibilityRole:] - AppKit's own role constant as UTF-8,
 * "AXButton" and its kind (R11). Leave it alone and AppKit reports the role it
 * infers from the class and the view's place in the window; setting one
 * replaces that inference and null replaces it with nothing (R23). */
void ns_view_set_accessibility_role(ns_view *_Nonnull view,
                                    const char *_Nullable role)
    API_AVAILABLE(macos(26.0));

/* -[NSView accessibilityRole] - a copy property, so this is an owned copy the
 * caller frees with ns_string_free (R7, R11). */
char *_Nullable ns_view_copy_accessibility_role(ns_view *_Nonnull view)
    API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_VIEW_H */

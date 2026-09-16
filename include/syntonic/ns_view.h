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
 * LAYOUT (R17, KTD3). The intrinsic size and the two layout priorities below
 * are what a view tells Auto Layout about itself, and what a twin comparison
 * diffs before it trusts a screenshot. AppKit declares them on NSView in
 * NSLayoutConstraint.h rather than in NSView.h, and they are here because
 * NSView is the class that carries them.
 *
 * THE TAB CHAIN (R19). Without a nib AppKit builds a window's key view loop
 * from the view tree, which is not guaranteed to follow construction order, so
 * a program that wants a particular Tab order chains it with
 * ns_view_set_next_key_view and turns the rebuild off with
 * ns_window_set_autorecalculates_key_view_loop. A rebuild replaces the whole
 * chain - see that function, whose comment records what the flag does and does
 * not hold on this OS.
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

/* NSLayoutConstraintOrientation, the axis a layout priority applies to. It
 * belongs to NSLayoutConstraint.h, which v0 does not wrap as a class, and the
 * view's two priority pairs are the only place it is needed - so it is
 * declared here, where it is used. */
typedef enum ns_layout_constraint_orientation : int64_t {
  NS_LAYOUT_CONSTRAINT_ORIENTATION_HORIZONTAL = 0,
  NS_LAYOUT_CONSTRAINT_ORIENTATION_VERTICAL = 1,
} ns_layout_constraint_orientation;

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

/* -[NSView bounds] - the view's own rectangle, in its own coordinates. Where
 * `frame` is what the superview sees, this is what the view draws in. */
CGRect ns_view_bounds(ns_view *_Nonnull view) API_AVAILABLE(macos(26.0));

/* -[NSView convertRect:toView:] - `rect`, read in this view's coordinates,
 * expressed in `view`'s. Null converts to the window's base coordinates, which
 * is AppKit's own meaning for a nil target. The two views have to share a
 * window. */
CGRect ns_view_convert_rect_to_view(ns_view *_Nonnull view, CGRect rect,
                                    ns_view *_Nullable target)
    API_AVAILABLE(macos(26.0));

/* -[NSView intrinsicContentSize] - the size the view's own content wants,
 * before any constraint has a say. A view with no intrinsic size in an axis
 * reports NSViewNoIntrinsicMetric, which is -1, for that axis. Declared by
 * NSLayoutConstraint.h, on NSView. */
CGSize ns_view_intrinsic_content_size(ns_view *_Nonnull view)
    API_AVAILABLE(macos(26.0));

/* -[NSView setContentHuggingPriority:forOrientation:] - how hard the view
 * resists growing past its intrinsic size. AppKit's NSLayoutPriority is a
 * float, and its landmarks are 1000 required, 750 high, 250 low. Declared by
 * NSLayoutConstraint.h, on NSView. */
void ns_view_set_content_hugging_priority_for_orientation(
    ns_view *_Nonnull view, float priority,
    ns_layout_constraint_orientation orientation) API_AVAILABLE(macos(26.0));

/* -[NSView contentHuggingPriorityForOrientation:] */
float ns_view_content_hugging_priority_for_orientation(
    ns_view *_Nonnull view, ns_layout_constraint_orientation orientation)
    API_AVAILABLE(macos(26.0));

/* -[NSView setContentCompressionResistancePriority:forOrientation:] - how hard
 * the view resists shrinking below its intrinsic size. Declared by
 * NSLayoutConstraint.h, on NSView. */
void ns_view_set_content_compression_resistance_priority_for_orientation(
    ns_view *_Nonnull view, float priority,
    ns_layout_constraint_orientation orientation) API_AVAILABLE(macos(26.0));

/* -[NSView contentCompressionResistancePriorityForOrientation:] */
float ns_view_content_compression_resistance_priority_for_orientation(
    ns_view *_Nonnull view, ns_layout_constraint_orientation orientation)
    API_AVAILABLE(macos(26.0));

/* -[NSView setNextKeyView:] - the view Tab moves to from this one. A chain
 * built by hand holds only while the window's key view loop rebuild is off;
 * see ns_window_set_autorecalculates_key_view_loop (R19). */
void ns_view_set_next_key_view(ns_view *_Nonnull view,
                               ns_view *_Nullable next_key_view)
    API_AVAILABLE(macos(26.0));

/* -[NSView nextKeyView] - an unretained property, so this is borrowed: valid
 * while something else holds the view, kept longer with ns_retain (R7). */
ns_view *_Nullable ns_view_next_key_view(ns_view *_Nonnull view)
    API_AVAILABLE(macos(26.0));

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

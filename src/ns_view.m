/*
 * NSView: the view tree, the frame, and the two accessibility setters
 * (R4, R17, R23, KTD7, KTD17).
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syntonic/ns_view.h"

/* Owned (+1): the caller ends it with ns_release (R7, KTD7). */
ns_view *ns_view_create_with_frame(CGRect frame) {
  NS_ENTER();
  return NS_OUT_OWNED(ns_view, [[NSView alloc] initWithFrame:frame]);
  NS_LEAVE();
}

void ns_view_add_subview(ns_view *view, ns_view *subview) {
  NS_ENTER();
  [NS_IN(NSView, view) addSubview:NS_IN(NSView, subview)];
  NS_LEAVE();
}

void ns_view_remove_from_superview(ns_view *view) {
  NS_ENTER();
  [NS_IN(NSView, view) removeFromSuperview];
  NS_LEAVE();
}

/* `subviews` is a copy property, so it never crosses as a handle: it becomes a
 * count plus an index accessor (R7, KTD17). */
long ns_view_subview_count(ns_view *view) {
  NS_ENTER();
  return (long)NS_IN(NSView, view).subviews.count;
  NS_LEAVE();
}

/* Borrowed: the element is read through the receiver, which holds it (KTD17).
 * No explicit range check - NSArray raises on an index past the end and the
 * entry macro reports that exception with this function's name (KTD4). */
ns_view *ns_view_subview_at_index(ns_view *view, long index) {
  NS_ENTER();
  NSArray<NSView *> *subviews = NS_IN(NSView, view).subviews;
  return NS_OUT(ns_view, subviews[(NSUInteger)index]);
  NS_LEAVE();
}

/* Borrowed: `superview` is an unretained property, which is the shape NS_OUT
 * is right for (R7). */
ns_view *ns_view_superview(ns_view *view) {
  NS_ENTER();
  return NS_OUT(ns_view, NS_IN(NSView, view).superview);
  NS_LEAVE();
}

void ns_view_set_frame(ns_view *view, CGRect frame) {
  NS_ENTER();
  NS_IN(NSView, view).frame = frame;
  NS_LEAVE();
}

CGRect ns_view_frame(ns_view *view) {
  NS_ENTER();
  return NS_IN(NSView, view).frame;
  NS_LEAVE();
}

void ns_view_set_autoresizing_mask(ns_view *view,
                                   ns_autoresizing_mask_options mask) {
  NS_ENTER();
  NS_IN(NSView, view).autoresizingMask = (NSAutoresizingMaskOptions)mask;
  NS_LEAVE();
}

ns_autoresizing_mask_options ns_view_autoresizing_mask(ns_view *view) {
  NS_ENTER();
  return (ns_autoresizing_mask_options)NS_IN(NSView, view).autoresizingMask;
  NS_LEAVE();
}

void ns_view_set_hidden(ns_view *view, bool hidden) {
  NS_ENTER();
  NS_IN(NSView, view).hidden = hidden;
  NS_LEAVE();
}

bool ns_view_hidden(ns_view *view) {
  NS_ENTER();
  return NS_IN(NSView, view).hidden;
  NS_LEAVE();
}

/* R23: the only place the library touches an accessibility property is the one
 * the caller asked it to. A value set here replaces AppKit's own inference,
 * and null replaces it with nothing - AppKit does not infer again. */
void ns_view_set_accessibility_label(ns_view *view, const char *label) {
  NS_ENTER();
  NS_IN(NSView, view).accessibilityLabel = NS_STRING_IN_OPT(label);
  NS_LEAVE();
}

/* `accessibilityLabel` is a copy property, so the value is owned and the name
 * says copy_ (R7, R11). */
char *ns_view_copy_accessibility_label(ns_view *view) {
  NS_ENTER();
  return NS_STRING_OUT(NS_IN(NSView, view).accessibilityLabel);
  NS_LEAVE();
}

/* An NSAccessibilityRole is a typed NSString, so it crosses as UTF-8 like any
 * other string (R11, KTD17). */
void ns_view_set_accessibility_role(ns_view *view, const char *role) {
  NS_ENTER();
  NS_IN(NSView, view).accessibilityRole = NS_STRING_IN_OPT(role);
  NS_LEAVE();
}

/* `accessibilityRole` is a copy property, so the value is owned (R7, R11). */
char *ns_view_copy_accessibility_role(ns_view *view) {
  NS_ENTER();
  return NS_STRING_OUT(NS_IN(NSView, view).accessibilityRole);
  NS_LEAVE();
}

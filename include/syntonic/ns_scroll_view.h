/*
 * NSScrollView - the clip and the scrollers a table or an outline lives in
 * (R17).
 *
 * A table view is never added to a pane directly: it is handed to a scroll
 * view as its document view, and the scroll view is what the pane holds. The
 * two calls that matter are ns_scroll_view_create_with_frame and
 * ns_scroll_view_set_document_view, in that order.
 *
 *     ns_scroll_view *scroll = ns_scroll_view_create_with_frame(bounds);
 *     ns_table_view *table = ns_table_view_create_with_frame(bounds);
 *     ns_scroll_view_set_document_view(scroll, ns_table_view_as_view(table));
 *     ns_release(table);   // the scroll view holds it now (R7)
 *
 * `documentView` is a strong property, so the getter borrows (R7): the handle
 * is valid while the scroll view holds it, and ns_retain keeps it past that.
 */

#ifndef SYNTONIC_NS_SCROLL_VIEW_H
#define SYNTONIC_NS_SCROLL_VIEW_H

#include <CoreGraphics/CGGeometry.h>
#include <os/availability.h>
#include <stdbool.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_scroll_view ns_scroll_view;

/* Declared by ns_view.h and repeated here so this header stands alone. C has
 * allowed a repeated typedef of the same type since C11. */
typedef struct ns_view ns_view;

/* -[NSScrollView initWithFrame:] - owned (+1), released with ns_release (R7).
 * NSScrollView declares this initializer itself, as its designated one. */
ns_scroll_view *_Nonnull ns_scroll_view_create_with_frame(CGRect frame)
    API_AVAILABLE(macos(26.0));

/* -[NSScrollView setDocumentView:] - the scroll view retains the view, so
 * releasing your own reference afterwards is correct (R7). Null clears it. */
void ns_scroll_view_set_document_view(ns_scroll_view *_Nonnull scroll_view,
                                      ns_view *_Nullable document_view)
    API_AVAILABLE(macos(26.0));

/* -[NSScrollView documentView] - a strong property, so this is borrowed and
 * valid while the scroll view holds it (R7). */
ns_view *_Nullable ns_scroll_view_document_view(
    ns_scroll_view *_Nonnull scroll_view) API_AVAILABLE(macos(26.0));

/* -[NSScrollView documentVisibleRect] - the part of the document view on
 * screen, in the document view's own coordinates. This is what moves when a
 * table scrolls a row into view (R16). */
CGRect ns_scroll_view_document_visible_rect(
    ns_scroll_view *_Nonnull scroll_view) API_AVAILABLE(macos(26.0));

/* -[NSScrollView setHasVerticalScroller:] */
void ns_scroll_view_set_has_vertical_scroller(
    ns_scroll_view *_Nonnull scroll_view, bool has_vertical_scroller)
    API_AVAILABLE(macos(26.0));

/* -[NSScrollView hasVerticalScroller] */
bool ns_scroll_view_has_vertical_scroller(ns_scroll_view *_Nonnull scroll_view)
    API_AVAILABLE(macos(26.0));

/* -[NSScrollView setAutohidesScrollers:] */
void ns_scroll_view_set_autohides_scrollers(
    ns_scroll_view *_Nonnull scroll_view, bool autohides_scrollers)
    API_AVAILABLE(macos(26.0));

/* -[NSScrollView autohidesScrollers] */
bool ns_scroll_view_autohides_scrollers(ns_scroll_view *_Nonnull scroll_view)
    API_AVAILABLE(macos(26.0));

/* -[NSScrollView setDrawsBackground:] - false lets a sidebar's own material
 * through, which is what the source list wants (R15). */
void ns_scroll_view_set_draws_background(ns_scroll_view *_Nonnull scroll_view,
                                         bool draws_background)
    API_AVAILABLE(macos(26.0));

/* -[NSScrollView drawsBackground] */
bool ns_scroll_view_draws_background(ns_scroll_view *_Nonnull scroll_view)
    API_AVAILABLE(macos(26.0));

/* The upcast to ns_view: the same pointer, checked in a debug build (R4,
 * KTD9). This is how the scroll view joins a view tree. syntonic-owned. */
ns_view *_Nonnull ns_scroll_view_as_view(ns_scroll_view *_Nonnull scroll_view)
    API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_SCROLL_VIEW_H */

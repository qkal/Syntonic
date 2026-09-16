/*
 * NSScrollView: the clip a table or an outline lives in (R17).
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syntonic/ns_scroll_view.h"

/* Owned (+1): an initWith… returns a fresh object the caller ends with
 * ns_release (R7, KTD7). */
ns_scroll_view *ns_scroll_view_create_with_frame(CGRect frame) {
  NS_ENTER();
  return NS_OUT_OWNED(ns_scroll_view,
                      [[NSScrollView alloc] initWithFrame:frame]);
  NS_LEAVE();
}

void ns_scroll_view_set_document_view(ns_scroll_view *scroll_view,
                                      ns_view *document_view) {
  NS_ENTER();
  NS_IN(NSScrollView, scroll_view).documentView =
      NS_IN_OPT(NSView, document_view);
  NS_LEAVE();
}

/* `documentView` is a strong property, so the return is borrowed (R7). */
ns_view *ns_scroll_view_document_view(ns_scroll_view *scroll_view) {
  NS_ENTER();
  return NS_OUT(ns_view, NS_IN(NSScrollView, scroll_view).documentView);
  NS_LEAVE();
}

CGRect ns_scroll_view_document_visible_rect(ns_scroll_view *scroll_view) {
  NS_ENTER();
  return NS_IN(NSScrollView, scroll_view).documentVisibleRect;
  NS_LEAVE();
}

void ns_scroll_view_set_has_vertical_scroller(ns_scroll_view *scroll_view,
                                              bool has_vertical_scroller) {
  NS_ENTER();
  NS_IN(NSScrollView, scroll_view).hasVerticalScroller =
      has_vertical_scroller;
  NS_LEAVE();
}

bool ns_scroll_view_has_vertical_scroller(ns_scroll_view *scroll_view) {
  NS_ENTER();
  return NS_IN(NSScrollView, scroll_view).hasVerticalScroller;
  NS_LEAVE();
}

void ns_scroll_view_set_autohides_scrollers(ns_scroll_view *scroll_view,
                                            bool autohides_scrollers) {
  NS_ENTER();
  NS_IN(NSScrollView, scroll_view).autohidesScrollers = autohides_scrollers;
  NS_LEAVE();
}

bool ns_scroll_view_autohides_scrollers(ns_scroll_view *scroll_view) {
  NS_ENTER();
  return NS_IN(NSScrollView, scroll_view).autohidesScrollers;
  NS_LEAVE();
}

void ns_scroll_view_set_draws_background(ns_scroll_view *scroll_view,
                                         bool draws_background) {
  NS_ENTER();
  NS_IN(NSScrollView, scroll_view).drawsBackground = draws_background;
  NS_LEAVE();
}

bool ns_scroll_view_draws_background(ns_scroll_view *scroll_view) {
  NS_ENTER();
  return NS_IN(NSScrollView, scroll_view).drawsBackground;
  NS_LEAVE();
}

/* An upcast is the identity, with the debug class check NS_IN carries (KTD9). */
ns_view *ns_scroll_view_as_view(ns_scroll_view *scroll_view) {
  NS_ENTER();
  return NS_OUT(ns_view, NS_IN(NSScrollView, scroll_view));
  NS_LEAVE();
}

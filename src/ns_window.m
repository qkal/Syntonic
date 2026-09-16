/*
 * NSWindow and its delegate (R14, F1, F3, KTD7, KTD8).
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syn_shims.h"
#include "syntonic/ns_window.h"

/* NSWindowDelegate is entirely @optional in the SDK and Syntonic requires
 * nothing of it either; docs/conventions.md's per-protocol table is the
 * source (R9, KTD8). */
SYN_SHIM_TABLE(syn_window_table, ns_window_callbacks, "NSWindowDelegate",
               SYN_SHIM_OPTIONAL(ns_window_callbacks, will_close,
                                 "windowWillClose:"));

@interface SynWindowShim : SynShim <NSWindowDelegate>
@end

@implementation SynWindowShim

- (void)windowWillClose:(NSNotification *)notification {
  SYN_SHIM_ENTER(NSWindow, notification.object);
  void (*callback)(void *, ns_window *) =
      SYN_SHIM_FN(ns_window_callbacks, will_close);
  if (callback != NULL) callback(syn_context, NS_OUT(ns_window, syn_sender));
  SYN_SHIM_LEAVE();
}

@end

/* Owned (+1): the caller ends it with ns_release (R7, KTD7). */
ns_window *ns_window_create_with_content_rect_style_mask_backing_defer(
    CGRect content_rect, ns_window_style_mask style_mask,
    ns_backing_store_type backing, bool defer) {
  NS_ENTER();
  NSWindow *window =
      [[NSWindow alloc] initWithContentRect:content_rect
                                  styleMask:(NSWindowStyleMask)style_mask
                                    backing:(NSBackingStoreType)backing
                                      defer:defer];
  /* The post-init fixup docs/conventions.md lists first: NSWindow's default is
   * to release itself when closed, which would leave the caller's handle
   * dangling. With it off, close only orders the window out and ns_release is
   * what tears the tree down (KTD7, F3). */
  window.releasedWhenClosed = NO;
  return NS_OUT_OWNED(ns_window, window);
  NS_LEAVE();
}

void ns_window_set_title(ns_window *window, const char *title) {
  NS_ENTER();
  NS_IN(NSWindow, window).title = NS_STRING_IN(title);
  NS_LEAVE();
}

/* `title` is a copy property, so the value is owned and the name says copy_
 * (R7, R11). */
char *ns_window_copy_title(ns_window *window) {
  NS_ENTER();
  return NS_STRING_OUT(NS_IN(NSWindow, window).title);
  NS_LEAVE();
}

/* Borrowed: `contentView` is a strong property (R7). */
ns_view *ns_window_content_view(ns_window *window) {
  NS_ENTER();
  return NS_OUT(ns_view, NS_IN(NSWindow, window).contentView);
  NS_LEAVE();
}

void ns_window_set_content_view_controller(ns_window *window,
                                           ns_view_controller *controller) {
  NS_ENTER();
  NS_IN(NSWindow, window).contentViewController =
      NS_IN_OPT(NSViewController, controller);
  NS_LEAVE();
}

void ns_window_set_toolbar(ns_window *window, ns_toolbar *toolbar) {
  NS_ENTER();
  NS_IN(NSWindow, window).toolbar = NS_IN_OPT(NSToolbar, toolbar);
  NS_LEAVE();
}

void ns_window_set_toolbar_style(ns_window *window,
                                 ns_window_toolbar_style style) {
  NS_ENTER();
  NS_IN(NSWindow, window).toolbarStyle = (NSWindowToolbarStyle)style;
  NS_LEAVE();
}

void ns_window_set_content_min_size(ns_window *window, CGSize size) {
  NS_ENTER();
  NS_IN(NSWindow, window).contentMinSize = size;
  NS_LEAVE();
}

CGSize ns_window_content_min_size(ns_window *window) {
  NS_ENTER();
  return NS_IN(NSWindow, window).contentMinSize;
  NS_LEAVE();
}

void ns_window_set_content_size(ns_window *window, CGSize size) {
  NS_ENTER();
  [NS_IN(NSWindow, window) setContentSize:size];
  NS_LEAVE();
}

CGRect ns_window_frame(ns_window *window) {
  NS_ENTER();
  return NS_IN(NSWindow, window).frame;
  NS_LEAVE();
}

void ns_window_make_key_and_order_front(ns_window *window) {
  NS_ENTER();
  [NS_IN(NSWindow, window) makeKeyAndOrderFront:nil];
  NS_LEAVE();
}

void ns_window_close(ns_window *window) {
  NS_ENTER();
  [NS_IN(NSWindow, window) close];
  NS_LEAVE();
}

void ns_window_set_callbacks(ns_window *window,
                             const ns_window_callbacks *callbacks,
                             void *context) {
  NS_ENTER();
  NSWindow *target = NS_IN(NSWindow, window);
  SYN_SHIM_INSTALL(target, SynWindowShim, syn_window_table, callbacks, context,
                   ^(id shim) { target.delegate = shim; });
  NS_LEAVE();
}

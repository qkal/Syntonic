/*
 * NSWindow - a standard titled window with close, minimize and zoom, a
 * toolbar style, a minimum content size and a delegate (R14).
 *
 * ns_window_create_with_content_rect_style_mask_backing_defer turns
 * releasedWhenClosed off (KTD7): closing a Syntonic window only orders it out,
 * and releasing the last reference is what tears the window and its view tree
 * down (F3). docs/conventions.md keeps the list of such post-init fixups.
 *
 * A window created here is not on screen. ns_window_make_key_and_order_front
 * shows it.
 */

#ifndef SYNTONIC_NS_WINDOW_H
#define SYNTONIC_NS_WINDOW_H

#include <CoreGraphics/CGGeometry.h>
#include <os/availability.h>
#include <stdbool.h>
#include <stdint.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_window ns_window;

/* Declared by their own headers - ns_view_controller.h, and NSView's and
 * NSToolbar's in later units - and repeated here so this header stands alone.
 * C has allowed a repeated typedef of the same type since C11. */
typedef struct ns_view ns_view;
typedef struct ns_view_controller ns_view_controller;
typedef struct ns_toolbar ns_toolbar;

/* NSWindowStyleMask. The bits a titled window is made of; combine them with
 * the bitwise or. */
typedef enum ns_window_style_mask : uint64_t {
  NS_WINDOW_STYLE_MASK_BORDERLESS = 0,
  NS_WINDOW_STYLE_MASK_TITLED = 1 << 0,
  NS_WINDOW_STYLE_MASK_CLOSABLE = 1 << 1,
  NS_WINDOW_STYLE_MASK_MINIATURIZABLE = 1 << 2,
  NS_WINDOW_STYLE_MASK_RESIZABLE = 1 << 3,
  NS_WINDOW_STYLE_MASK_FULL_SIZE_CONTENT_VIEW = 1 << 15,
} ns_window_style_mask;

/* NSBackingStoreType. Buffered is the only value that is not deprecated. */
typedef enum ns_backing_store_type : uint64_t {
  NS_BACKING_STORE_BUFFERED = 2,
} ns_backing_store_type;

/* NSWindowToolbarStyle. */
typedef enum ns_window_toolbar_style : int64_t {
  NS_WINDOW_TOOLBAR_STYLE_AUTOMATIC = 0,
  NS_WINDOW_TOOLBAR_STYLE_EXPANDED = 1,
  NS_WINDOW_TOOLBAR_STYLE_PREFERENCE = 2,
  NS_WINDOW_TOOLBAR_STYLE_UNIFIED = 3,
  NS_WINDOW_TOOLBAR_STYLE_UNIFIED_COMPACT = 4,
} ns_window_toolbar_style;

/*
 * NSWindowDelegate, as a struct of function pointers. Every member is
 * optional; docs/conventions.md's per-protocol table is what says so.
 *
 * `context` is what you passed to ns_window_set_callbacks, untouched and never
 * freed by the library; `sender` is the window, borrowed for the duration of
 * the call - a callback may retain it, release it, or close it, and it stays
 * valid until the callback returns. Every member runs on the main thread.
 * syntonic-owned.
 */
typedef struct ns_window_callbacks {
  /* optional. -[NSWindowDelegate windowWillClose:] - the notification carries
   * nothing beyond the window, so the window is what crosses (R5). */
  void (*_Nullable will_close)(void *_Nullable context,
                               ns_window *_Nonnull sender);
} ns_window_callbacks;

/* -[NSWindow initWithContentRect:styleMask:backing:defer:] - owned (+1),
 * released with ns_release (R7). The window is created with
 * releasedWhenClosed off, so close only orders it out (KTD7). */
ns_window *_Nonnull ns_window_create_with_content_rect_style_mask_backing_defer(
    CGRect content_rect, ns_window_style_mask style_mask,
    ns_backing_store_type backing, bool defer) API_AVAILABLE(macos(26.0));

/* -[NSWindow setTitle:] */
void ns_window_set_title(ns_window *_Nonnull window, const char *_Nonnull title)
    API_AVAILABLE(macos(26.0));

/* -[NSWindow title] - `title` is a copy property, so this is an owned copy the
 * caller frees with ns_string_free (R7, R11). */
char *_Nullable ns_window_copy_title(ns_window *_Nonnull window)
    API_AVAILABLE(macos(26.0));

/* -[NSWindow contentView] - a strong property, so this is borrowed: valid
 * while the window holds it, kept longer with ns_retain (R7). */
ns_view *_Nullable ns_window_content_view(ns_window *_Nonnull window)
    API_AVAILABLE(macos(26.0));

/* -[NSWindow setContentViewController:] - the window retains the controller
 * and takes the controller's view as its content view, so releasing your own
 * reference afterwards is correct (R7). */
void ns_window_set_content_view_controller(
    ns_window *_Nonnull window,
    ns_view_controller *_Nullable view_controller) API_AVAILABLE(macos(26.0));

/* -[NSWindow setToolbar:] */
void ns_window_set_toolbar(ns_window *_Nonnull window,
                           ns_toolbar *_Nullable toolbar)
    API_AVAILABLE(macos(26.0));

/* -[NSWindow setToolbarStyle:] */
void ns_window_set_toolbar_style(ns_window *_Nonnull window,
                                 ns_window_toolbar_style style)
    API_AVAILABLE(macos(26.0));

/* -[NSWindow setContentMinSize:] - the floor AppKit holds a resize the user
 * drives to. It does not clamp ns_window_set_content_size: a size the program
 * asks for is the size the window takes, which is AppKit's own behaviour. */
void ns_window_set_content_min_size(ns_window *_Nonnull window, CGSize size)
    API_AVAILABLE(macos(26.0));

/* -[NSWindow contentMinSize] */
CGSize ns_window_content_min_size(ns_window *_Nonnull window)
    API_AVAILABLE(macos(26.0));

/* -[NSWindow setContentSize:] - taken as asked, above or below the minimum
 * content size; see ns_window_set_content_min_size. */
void ns_window_set_content_size(ns_window *_Nonnull window, CGSize size)
    API_AVAILABLE(macos(26.0));

/* -[NSWindow frame] - in screen coordinates, including the title bar. */
CGRect ns_window_frame(ns_window *_Nonnull window) API_AVAILABLE(macos(26.0));

/* -[NSWindow makeKeyAndOrderFront:] - AppKit's sender argument is an `id` and
 * never crosses the boundary (R11), so the library passes nil. */
void ns_window_make_key_and_order_front(ns_window *_Nonnull window)
    API_AVAILABLE(macos(26.0));

/* -[NSWindow close] - orders the window out and fires will_close. It does not
 * release the window: a Syntonic window has releasedWhenClosed off, so your
 * handle stays valid and ns_release is what tears the tree down (KTD7, F3). */
void ns_window_close(ns_window *_Nonnull window) API_AVAILABLE(macos(26.0));

/* -[NSWindow setInitialFirstResponder:] - the view that holds focus the first
 * time the window is made key, and where an explicit Tab chain starts (R19). */
void ns_window_set_initial_first_responder(ns_window *_Nonnull window,
                                           ns_view *_Nullable view)
    API_AVAILABLE(macos(26.0));

/* -[NSWindow initialFirstResponder] - a weak property, so this is borrowed:
 * valid while the view tree holds it, kept longer with ns_retain (R7). */
ns_view *_Nullable ns_window_initial_first_responder(
    ns_window *_Nonnull window) API_AVAILABLE(macos(26.0));

/* -[NSWindow setAutorecalculatesKeyViewLoop:] - whether AppKit rebuilds the
 * key view loop from the view tree rather than leaving the chain
 * ns_view_set_next_key_view built. Turn it off before chaining Tab by hand,
 * which is what a window with no nib does (R19).
 *
 * What the flag actually buys on macOS 27 is less than its name suggests, and
 * tests/test_layout.c pins the measurement: a chain set on an off-screen
 * window survives with the flag either way, and a rebuild - whether
 * ns_window_recalculate_key_view_loop asks for one or making the window key
 * does - replaces the chain with the flag either way. Set it off as the twins
 * do, and do not rely on it to hold a chain across a rebuild. */
void ns_window_set_autorecalculates_key_view_loop(ns_window *_Nonnull window,
                                                  bool autorecalculates)
    API_AVAILABLE(macos(26.0));

/* -[NSWindow recalculateKeyViewLoop] - rebuilds the key view loop from the
 * view tree now, replacing any chain set with ns_view_set_next_key_view. */
void ns_window_recalculate_key_view_loop(ns_window *_Nonnull window)
    API_AVAILABLE(macos(26.0));

/* Installs the callbacks struct as the window's delegate. The struct is
 * copied; `context` is not, and has to stay valid until you uninstall or the
 * window is deallocated - ns_release does not uninstall. Installing again
 * replaces the previous struct and context; a null `callbacks` uninstalls
 * (R9, KTD8). syntonic-owned. */
void ns_window_set_callbacks(ns_window *_Nonnull window,
                             const ns_window_callbacks *_Nullable callbacks,
                             void *_Nullable context) API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_WINDOW_H */

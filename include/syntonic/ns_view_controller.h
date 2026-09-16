/*
 * NSViewController - the base every content controller in Syntonic is built
 * on, and the type a window's content view controller and a split or tab view
 * item takes (R1, R14).
 *
 * A view controller owns its view. Create one, give it a view, and hand the
 * controller to its parent; releasing your reference afterwards is correct,
 * because the parent holds its own (R7).
 *
 * AppKit has no initWithView:, so "create with a view" is the two selectors it
 * does have: ns_view_controller_create then ns_view_controller_set_view. Do
 * both before anything reads the view: a view controller asked for a view it
 * was never given loads an empty one and keeps it.
 */

#ifndef SYNTONIC_NS_VIEW_CONTROLLER_H
#define SYNTONIC_NS_VIEW_CONTROLLER_H

#include <os/availability.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_view_controller ns_view_controller;

/* NSView's own header (U7) owns this typedef; repeating it keeps this header
 * standalone, and C has allowed a repeated typedef of the same type since
 * C11. */
typedef struct ns_view ns_view;

/* -[NSViewController init] - owned (+1), released with ns_release (R7). */
ns_view_controller *_Nonnull ns_view_controller_create(void)
    API_AVAILABLE(macos(26.0));

/* -[NSViewController setView:] - the controller retains the view, so releasing
 * your own reference afterwards is correct (R7). */
void ns_view_controller_set_view(
    ns_view_controller *_Nonnull view_controller, ns_view *_Nonnull view)
    API_AVAILABLE(macos(26.0));

/* -[NSViewController view] - a strong property, so this is borrowed: valid
 * while the controller holds it, kept longer with ns_retain (R7). Asking for
 * it before ns_view_controller_set_view makes AppKit load an empty view and
 * keep that one. */
ns_view *_Nonnull ns_view_controller_view(
    ns_view_controller *_Nonnull view_controller) API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_VIEW_CONTROLLER_H */

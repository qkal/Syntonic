/*
 * The shape of a Syntonic wrapper header (R6, KTD18).
 *
 * Copy this file to include/syntonic/ns_<class>.h, copy ns_template.m beside
 * it as src/ns_<class>.m, and replace every placeholder:
 *
 *   ns_template          -> your C type, the class name snake_cased (R5)
 *   ns_template_super    -> the ancestor you upcast to, declared in its own
 *                           header; the template declares it here only so the
 *                           file compiles on its own
 *   NSTemplate           -> the AppKit class, in the .m only
 *   receiver             -> name the receiver after its type: ns_window *window
 *
 * Every function below is one shape you will need; delete the ones you do not.
 * The comment above each one names the selector it wraps, or carries the
 * syntonic-owned tag when there is no selector - scripts/lint-headers.sh fails
 * the build on a mirror-header function that has neither (KTD18).
 *
 * Read docs/conventions.md before editing this. The procedure that uses this
 * file is its "How to wrap a new class" section.
 */

#ifndef SYNTONIC_NS_TEMPLATE_H
#define SYNTONIC_NS_TEMPLATE_H

#include <CoreGraphics/CGGeometry.h>
#include <os/availability.h>
#include <stdbool.h>
#include <stdint.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9); it is never
 * defined, so a handle of the wrong class is a compile error. */
typedef struct ns_template ns_template;

/* The ancestor this class upcasts to. In a real wrapper this typedef lives in
 * the ancestor's own header and you include that header instead. */
typedef struct ns_template_super ns_template_super;

/* An AppKit enum becomes a snake_case type with a fixed underlying type (R6)
 * and NS_-prefixed upper-case constants (R5). NSTemplateStyleRounded becomes
 * NS_TEMPLATE_STYLE_ROUNDED. */
typedef enum ns_template_style : int32_t {
  NS_TEMPLATE_STYLE_AUTOMATIC = 0,
  NS_TEMPLATE_STYLE_ROUNDED = 1,
} ns_template_style;

/*
 * The callbacks of one AppKit protocol, or of one merged pair of protocols
 * (KTD6), as a struct of function pointers. An unset optional member behaves
 * as a method the object does not implement; an unset required member is
 * reported at install time in a debug build (R9, KTD8). Every member's required
 * or optional status is in docs/conventions.md's per-protocol table, which is
 * where you add a row for this protocol.
 *
 * `context` is what you passed to the installer. `sender` is a borrowed handle
 * to the object that fired the callback, valid for the duration of the call.
 * A string a callback receives is borrowed for the duration of the call; a
 * string a callback returns is borrowed by the library, which copies it before
 * returning (R11, KTD17). syntonic-owned.
 */
typedef struct ns_template_callbacks {
  /* optional. -[NSTemplateDelegate templateDidChange:] */
  void (*_Nullable did_change)(void *_Nullable context,
                               ns_template *_Nonnull sender);
  /* required. -[NSTemplateDataSource numberOfItemsInTemplate:] */
  long (*_Nullable number_of_items)(void *_Nullable context,
                                    ns_template *_Nonnull sender);
} ns_template_callbacks;

/* -[NSTemplate initWithFrame:] - owned (+1), released with ns_release (R7). */
ns_template *_Nonnull ns_template_create(CGRect frame)
    API_AVAILABLE(macos(26.0));

/* -[NSTemplate setTitle:] */
void ns_template_set_title(ns_template *_Nonnull receiver,
                           const char *_Nonnull title)
    API_AVAILABLE(macos(26.0));

/* -[NSTemplate title] - `title` is a copy property, so this is an owned copy
 * the caller frees with ns_string_free (R7, R11). */
char *_Nullable ns_template_copy_title(ns_template *_Nonnull receiver)
    API_AVAILABLE(macos(26.0));

/* -[NSTemplate setStyle:] */
void ns_template_set_style(ns_template *_Nonnull receiver,
                           ns_template_style style) API_AVAILABLE(macos(26.0));

/* -[NSTemplate style] */
ns_template_style ns_template_get_style(ns_template *_Nonnull receiver)
    API_AVAILABLE(macos(26.0));

/* -[NSTemplate superview] - a weak property, so this is borrowed: valid while
 * its owner holds it, kept longer with ns_retain (R7). */
ns_template_super *_Nullable ns_template_superview(
    ns_template *_Nonnull receiver) API_AVAILABLE(macos(26.0));

/* -[NSTemplate subviews] - an array property becomes a count function plus an
 * index accessor returning a borrowed element (R7, KTD17). */
long ns_template_subview_count(ns_template *_Nonnull receiver)
    API_AVAILABLE(macos(26.0));

/* -[NSTemplate subviews] */
ns_template_super *_Nonnull ns_template_subview_at_index(
    ns_template *_Nonnull receiver, long index) API_AVAILABLE(macos(26.0));

/* The upcast to the ancestor type: the same pointer, checked in a debug build.
 * One per wrapped ancestor, declared in the subclass's header. No AppKit
 * selector, so it carries the tag: syntonic-owned. */
ns_template_super *_Nonnull ns_template_as_template_super(
    ns_template *_Nonnull receiver) API_AVAILABLE(macos(26.0));

/* Installs the target/action callback: `action` runs on the main thread with
 * `context` and this object as the sender. A null `action` uninstalls.
 * `context` is neither copied nor freed and has to outlive the installation.
 * syntonic-owned. */
void ns_template_set_action(ns_template *_Nonnull receiver,
                            ns_action _Nullable action, void *_Nullable context)
    API_AVAILABLE(macos(26.0));

/* Installs the callbacks struct. The struct is copied; `context` is not.
 * Installing again replaces the previous struct and context; a null `callbacks`
 * uninstalls. A missing required member is reported here, in a debug build,
 * before AppKit ever asks (R9, KTD8). syntonic-owned. */
void ns_template_set_callbacks(ns_template *_Nonnull receiver,
                               const ns_template_callbacks *_Nullable callbacks,
                               void *_Nullable context)
    API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_TEMPLATE_H */

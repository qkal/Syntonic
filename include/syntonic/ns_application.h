/*
 * NSApplication - the shared application, its run loop, its activation and its
 * delegate (R14, KTD16).
 *
 * There is one application per process and it lives as long as the process, so
 * ns_application_shared borrows rather than owning: there is nothing to
 * release and no way for the object to go away (R7).
 *
 * Closing the last window leaves the app running with its menu bar until Quit
 * (F1). That is AppKit's own default and it is also what an unset
 * should_terminate_after_last_window_closed means here, so a delegate that
 * says nothing about termination gets it.
 */

#ifndef SYNTONIC_NS_APPLICATION_H
#define SYNTONIC_NS_APPLICATION_H

#include <os/availability.h>
#include <stdbool.h>
#include <stdint.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_application ns_application;

/* NSApplicationActivationPolicy, whether the application appears in the Dock
 * and owns a menu bar. It belongs to NSRunningApplication.h, which v0 does not
 * wrap, and the application is the only place it is needed - so it is declared
 * here, where it is used. An executable with no bundle starts prohibited,
 * which is why a bare C twin has to ask for regular. */
typedef enum ns_application_activation_policy : int64_t {
  NS_APPLICATION_ACTIVATION_POLICY_REGULAR = 0,
  NS_APPLICATION_ACTIVATION_POLICY_ACCESSORY = 1,
  NS_APPLICATION_ACTIVATION_POLICY_PROHIBITED = 2,
} ns_application_activation_policy;

/*
 * NSApplicationDelegate, as a struct of function pointers. Every member is
 * optional; docs/conventions.md's per-protocol table is what says so. An unset
 * member behaves exactly as a method the application delegate does not
 * implement, which is how F1 survives an empty struct.
 *
 * `context` is what you passed to ns_application_set_callbacks, untouched and
 * never freed by the library; `sender` is the shared application, borrowed for
 * the duration of the call. Every member runs on the main thread.
 * syntonic-owned.
 */
typedef struct ns_application_callbacks {
  /* optional. -[NSApplicationDelegate applicationDidFinishLaunching:] */
  void (*_Nullable did_finish_launching)(void *_Nullable context,
                                         ns_application *_Nonnull sender);
  /* optional. -[NSApplicationDelegate
   * applicationShouldTerminateAfterLastWindowClosed:] - unset is false, which
   * is both AppKit's default and F1. */
  bool (*_Nullable should_terminate_after_last_window_closed)(
      void *_Nullable context, ns_application *_Nonnull sender);
  /* optional. -[NSApplicationDelegate applicationWillTerminate:] */
  void (*_Nullable will_terminate)(void *_Nullable context,
                                   ns_application *_Nonnull sender);
} ns_application_callbacks;

/* +[NSApplication sharedApplication] - borrowed: the shared application is a
 * process-lifetime singleton that AppKit holds, so there is no reference to
 * give back (R7). Creates it on the first call. */
ns_application *_Nonnull ns_application_shared(void) API_AVAILABLE(macos(26.0));

/* -[NSApplication run] - enters the main run loop and does not return until
 * the application terminates. */
void ns_application_run(ns_application *_Nonnull application)
    API_AVAILABLE(macos(26.0));

/* -[NSApplication activate] - brings the application forward. This is the
 * activation API; activateIgnoringOtherApps: is deprecated (KTD16). */
void ns_application_activate(ns_application *_Nonnull application)
    API_AVAILABLE(macos(26.0));

/* -[NSApplication setActivationPolicy:] - true when the policy was taken. An
 * executable that is not in a bundle starts prohibited, with no Dock icon and
 * no menu bar, so a program built as a bare binary asks for
 * NS_APPLICATION_ACTIVATION_POLICY_REGULAR before it runs. */
bool ns_application_set_activation_policy(
    ns_application *_Nonnull application,
    ns_application_activation_policy policy) API_AVAILABLE(macos(26.0));

/* -[NSApplication terminate:] - AppKit's sender argument is an `id` and never
 * crosses the boundary (R11), so the library passes nil. */
void ns_application_terminate(ns_application *_Nonnull application)
    API_AVAILABLE(macos(26.0));

/* Installs the callbacks struct as the application's delegate. The struct is
 * copied; `context` is not, and has to stay valid until you uninstall or the
 * process ends. Installing again replaces the previous struct and context; a
 * null `callbacks` uninstalls (R9, KTD8). syntonic-owned. */
void ns_application_set_callbacks(
    ns_application *_Nonnull application,
    const ns_application_callbacks *_Nullable callbacks, void *_Nullable context)
    API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_APPLICATION_H */

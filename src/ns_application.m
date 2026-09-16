/*
 * NSApplication and its delegate (R14, F1, KTD8, KTD16).
 *
 * The delegate shim is the worked example src/syn_shims.h describes: a table,
 * a subclass with one method per member, and one install call.
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syn_shims.h"
#include "syntonic/ns_application.h"

/* Every member is optional; docs/conventions.md's per-protocol table is what
 * decides that, since NSApplicationDelegate is entirely @optional in the SDK
 * (R9, KTD8). */
SYN_SHIM_TABLE(
    syn_application_table, ns_application_callbacks, "NSApplicationDelegate",
    SYN_SHIM_OPTIONAL(ns_application_callbacks, did_finish_launching,
                      "applicationDidFinishLaunching:"),
    SYN_SHIM_OPTIONAL(ns_application_callbacks,
                      should_terminate_after_last_window_closed,
                      "applicationShouldTerminateAfterLastWindowClosed:"),
    SYN_SHIM_OPTIONAL(ns_application_callbacks, will_terminate,
                      "applicationWillTerminate:"));

@interface SynApplicationShim : SynShim <NSApplicationDelegate>
@end

@implementation SynApplicationShim

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
  SYN_SHIM_ENTER(NSApplication, notification.object);
  void (*callback)(void *, ns_application *) =
      SYN_SHIM_FN(ns_application_callbacks, did_finish_launching);
  if (callback != NULL)
    callback(syn_context, NS_OUT(ns_application, syn_sender));
  SYN_SHIM_LEAVE();
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:
    (NSApplication *)sender {
  SYN_SHIM_ENTER(NSApplication, sender);
  bool (*callback)(void *, ns_application *) = SYN_SHIM_FN(
      ns_application_callbacks, should_terminate_after_last_window_closed);
  /* Unset is false, which is F1 - closing the last window leaves the app
   * running with its menu bar - and is also what AppKit does when a delegate
   * does not implement this at all, so the two agree either way. */
  if (callback == NULL) return NO;
  return callback(syn_context, NS_OUT(ns_application, syn_sender)) ? YES : NO;
  SYN_SHIM_LEAVE();
}

- (void)applicationWillTerminate:(NSNotification *)notification {
  SYN_SHIM_ENTER(NSApplication, notification.object);
  void (*callback)(void *, ns_application *) =
      SYN_SHIM_FN(ns_application_callbacks, will_terminate);
  if (callback != NULL)
    callback(syn_context, NS_OUT(ns_application, syn_sender));
  SYN_SHIM_LEAVE();
}

@end

/* Borrowed: the shared application is a process-lifetime singleton AppKit
 * holds, so there is no reference to hand back (R7). */
ns_application *ns_application_shared(void) {
  NS_ENTER();
  return NS_OUT(ns_application, NSApplication.sharedApplication);
  NS_LEAVE();
}

void ns_application_run(ns_application *application) {
  NS_ENTER();
  [NS_IN(NSApplication, application) run];
  NS_LEAVE();
}

/* activate, not activateIgnoringOtherApps:, which is deprecated (KTD16). */
void ns_application_activate(ns_application *application) {
  NS_ENTER();
  [NS_IN(NSApplication, application) activate];
  NS_LEAVE();
}

void ns_application_terminate(ns_application *application) {
  NS_ENTER();
  [NS_IN(NSApplication, application) terminate:nil];
  NS_LEAVE();
}

void ns_application_set_callbacks(ns_application *application,
                                  const ns_application_callbacks *callbacks,
                                  void *context) {
  NS_ENTER();
  NSApplication *target = NS_IN(NSApplication, application);
  SYN_SHIM_INSTALL(target, SynApplicationShim, syn_application_table, callbacks,
                   context, ^(id shim) { target.delegate = shim; });
  NS_LEAVE();
}

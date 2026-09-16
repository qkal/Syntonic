#import <AppKit/AppKit.h>
#import <objc/runtime.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ns_internal.h"

/* ---------------------------------------------------------------------------
 * Debug checks (KTD4, R10, R12)
 *
 * None of this - including the report text - exists in an NDEBUG build.
 * ------------------------------------------------------------------------- */

#ifndef NDEBUG

/* Stopping rather than logging on: a log-and-continue hides the misuse, and a
 * breakpoint trap says nothing outside a debugger (KTD4). */
__attribute__((noreturn)) static void ns_internal_stop(void) {
  fflush(stderr);
  abort();
}

void ns_internal_check_main_thread(const char *function) {
  if ([NSThread isMainThread]) return;
  fprintf(stderr,
          "syntonic: %s: called from a background thread. Every Syntonic "
          "function is main thread only; ns_main_thread_dispatch is the one "
          "that crosses onto the main thread (R10).\n",
          function);
  ns_internal_stop();
}

void ns_internal_check_handle(const void *handle, Class expected, bool required,
                              const char *function) {
  if (handle == NULL) {
    if (!required) return;
    fprintf(stderr,
            "syntonic: %s: the %s handle is null at a non-null parameter "
            "(R12).\n",
            function, class_getName(expected));
    ns_internal_stop();
  }

  id object = (__bridge id)(void *)handle;
  if ([object isKindOfClass:expected]) return;
  fprintf(stderr,
          "syntonic: %s: expected a handle to %s, got %s, which is neither "
          "%s nor a subclass of it (R12).\n",
          function, class_getName(expected), object_getClassName(object),
          class_getName(expected));
  ns_internal_stop();
}

void ns_internal_check_utf8(const char *utf8, bool required,
                            const char *function) {
  if (utf8 == NULL) {
    if (!required) return;
    fprintf(stderr,
            "syntonic: %s: the string is null at a non-null parameter (R12).\n",
            function);
    ns_internal_stop();
  }

  /* The conversion is the only honest validity test, so the debug path runs it
   * twice: once here to report, once for real. */
  if ([NSString stringWithUTF8String:utf8] != nil) return;
  fprintf(stderr,
          "syntonic: %s: the string is not valid UTF-8; text crosses the "
          "boundary as UTF-8 (R11).\n",
          function);
  ns_internal_stop();
}

/* AppKit's own report for a bad index arrives after it has already done part
 * of the work, or not at all, so the wrapper checks first and names itself
 * (R12). `largest` is the largest index the call accepts, which for an insert
 * is the count. */
void ns_internal_check_index(long index, long largest, const char *function) {
  if (index >= 0 && index <= largest) return;
  fprintf(stderr,
          "syntonic: %s: index %ld is out of range; this call accepts 0 "
          "through %ld (R12).\n",
          function, index, largest);
  ns_internal_stop();
}

void ns_internal_report_exception(NSException *exception,
                                  const char *function) {
  NSString *reason = exception.reason;
  fprintf(stderr, "syntonic: %s: AppKit raised %s: %s (KTD4).\n", function,
          exception.name.UTF8String,
          reason != nil ? reason.UTF8String : "no reason given");
  ns_internal_stop();
}

#endif /* NDEBUG */

/* ---------------------------------------------------------------------------
 * Strings (R11, KTD7)
 * ------------------------------------------------------------------------- */

NSString *ns_internal_string_in(const char *utf8) {
  if (utf8 == NULL) return nil;
  /* nil for invalid UTF-8, which a debug build has already reported. */
  return [NSString stringWithUTF8String:utf8];
}

char *ns_internal_string_out(NSString *string) {
  if (string == nil) return NULL;
  /* UTF8String points into the pool this wrapper opened, so the copy has to
   * happen here rather than at the caller (KTD7). */
  return strdup(string.UTF8String);
}

/* ---------------------------------------------------------------------------
 * The public utilities (R7, R8, R10, KTD5, KTD7, KTD9)
 * ------------------------------------------------------------------------- */

const void *ns_retain(const void *handle) {
  NS_ENTER();
  if (handle == NULL) return NULL;
  /* The bridging casts are the retain: out of ARC and straight back in at +1,
   * which hands back the same pointer (KTD7). */
  return CFBridgingRetain((__bridge id)(void *)handle);
  NS_LEAVE();
}

void ns_release(const void *handle) {
  NS_ENTER();
  /* __bridge_transfer hands the reference to ARC, which drops it at the end of
   * this statement. */
  if (handle != NULL) CFBridgingRelease(handle);
  NS_LEAVE();
}

void ns_string_free(char *owned_utf8) {
  /* No pool, no main-thread check: a strdup copy is not an Objective-C object
   * and freeing it from any thread is safe (R10). */
  free(owned_utf8);
}

bool ns_available(int major, int minor) {
  NS_ENTER();
  /* Not __builtin_available, which only accepts literal versions and so cannot
   * take arguments; this is the runtime comparison behind it (KTD5). */
  NSOperatingSystemVersion running =
      NSProcessInfo.processInfo.operatingSystemVersion;
  if (running.majorVersion != major) return running.majorVersion > major;
  return running.minorVersion >= minor;
  NS_LEAVE();
}

void ns_main_thread_dispatch(ns_action action, void *context) {
  NS_ENTER_ANY_THREAD();
  /* Always asynchronous, from the main thread as well: a callback that
   * sometimes runs before the call returns and sometimes after is the harder
   * contract to write against. */
  dispatch_async(dispatch_get_main_queue(), ^{
    @autoreleasepool {
      action(context, NULL);
    }
  });
  NS_LEAVE();
}

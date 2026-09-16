/*
 * Syntonic's internal vocabulary: what a wrapper body is made of.
 *
 * Every public function in the library is written with these macros and
 * nothing else. They are internal - this header never ships - so unlike the
 * public headers (R6) they may be function-like macros.
 *
 * A wrapper is the macros plus one line of AppKit:
 *
 *     void ns_window_set_title(ns_window *window, const char *title) {
 *       NS_ENTER();
 *       NS_IN(NSWindow, window).title = NS_STRING_IN(title);
 *       NS_LEAVE();
 *     }
 *
 * and the same two lines of scaffolding hold for every return shape. Which
 * outbound macro to use is the ownership rule (R7, KTD7), not a preference:
 *
 *     ns_view *ns_window_content_view(ns_window *window) {   // borrowed
 *       NS_ENTER();
 *       return NS_OUT(ns_view, NS_IN(NSWindow, window).contentView);
 *       NS_LEAVE();
 *     }
 *
 *     char *ns_window_copy_title(ns_window *window) {            // owned copy
 *       NS_ENTER();
 *       return NS_STRING_OUT(NS_IN(NSWindow, window).title);
 *       NS_LEAVE();
 *     }
 *
 *     ns_window *ns_window_create(CGRect frame) {                // owned +1
 *       NS_ENTER();
 *       return NS_OUT_OWNED(ns_window, [[NSWindow alloc] initWithContentRect:frame
 *                                                                 styleMask:0
 *                                                                   backing:NSBackingStoreBuffered
 *                                                                     defer:NO]);
 *       NS_LEAVE();
 *     }
 *
 * NS_OUT is only ever right for the getter of a strong, weak or assign object
 * property. A copy property, a string, and any plain method returning an
 * object are owned returns named copy_ or create_ (KTD7).
 *
 * Two things to know before using them:
 *
 *   * NS_ENTER opens a brace that NS_LEAVE closes. A `return` from between
 *     them is correct and drains the pool; falling off the end of a void
 *     wrapper is correct too.
 *   * NS_IN, NS_IN_OPT and NS_STRING_IN evaluate their handle or string
 *     argument twice in a debug build, the way assert does. Pass a parameter
 *     or a plain expression, never a call with a side effect.
 *   * Nullability annotations belong to the public headers, where they are the
 *     contract (R12). Do not write _Nullable or _Nonnull here or in a .m file:
 *     clang's -Wnullability-completeness is file-scoped, so one annotation
 *     obliges every other pointer in the same file to carry one too.
 */

#ifndef SYNTONIC_SRC_NS_INTERNAL_H
#define SYNTONIC_SRC_NS_INTERNAL_H

#import <AppKit/AppKit.h>

#include <stdbool.h>

#include "syntonic/ns_base.h"

/* ---------------------------------------------------------------------------
 * Debug checks (R10, R12, KTD4)
 *
 * Each one prints the AppKit class, the function and the violated rule to
 * stderr and stops the process. Under NDEBUG every macro below expands to
 * nothing, the functions are not declared or defined at all, and none of the
 * report text reaches the binary - that is what R12's "release builds pay
 * nothing" means in practice.
 * ------------------------------------------------------------------------- */

#ifndef NDEBUG

void ns_internal_check_main_thread(const char *function);
void ns_internal_check_handle(const void *handle, Class expected, bool required,
                              const char *function);
void ns_internal_check_utf8(const char *utf8, bool required,
                            const char *function);
__attribute__((noreturn)) void ns_internal_report_exception(
    NSException *exception, const char *function);

/* R10, AE3: this function is main-thread only. */
#define NS_CHECK_MAIN_THREAD() ns_internal_check_main_thread(__func__)

/* R12: `handle` is null at a _Nonnull position, or is not a `cls`. */
#define NS_CHECK_HANDLE(handle, cls, required)                                \
  ns_internal_check_handle((const void *)(handle), [cls class], (required),   \
                           __func__)

/* R12: `utf8` is null at a _Nonnull position, or is not valid UTF-8. */
#define NS_CHECK_UTF8(utf8, required)                                         \
  ns_internal_check_utf8((utf8), (required), __func__)

/* KTD4: report an AppKit exception rather than unwinding through C frames,
 * which would skip both the C cleanups and the ARC releases. */
#define NS_TRY_BEGIN @try {
#define NS_TRY_END                                                            \
  }                                                                           \
  @catch (NSException *ns_exception_) {                                       \
    ns_internal_report_exception(ns_exception_, __func__);                    \
  }

#else /* NDEBUG */

#define NS_CHECK_MAIN_THREAD() ((void)0)
#define NS_CHECK_HANDLE(handle, cls, required) ((void)0)
#define NS_CHECK_UTF8(utf8, required) ((void)0)
#define NS_TRY_BEGIN
#define NS_TRY_END

#endif /* NDEBUG */

/* ---------------------------------------------------------------------------
 * Wrapper entry and exit (KTD7)
 * ------------------------------------------------------------------------- */

/* Opens a wrapper: the main-thread check, this call's own autorelease pool - a
 * C caller has none before the run loop starts - and the debug exception
 * handler. Pair with NS_LEAVE. */
#define NS_ENTER()                                                            \
  @autoreleasepool {                                                          \
    NS_CHECK_MAIN_THREAD();                                                   \
    NS_TRY_BEGIN

/* NS_ENTER for the two functions R10 exempts from the main-thread rule:
 * ns_main_thread_dispatch, which exists to cross onto the main thread, and any
 * function that touches no Objective-C object. The pool still matters - a
 * background thread has none of its own. */
#define NS_ENTER_ANY_THREAD()                                                 \
  @autoreleasepool {                                                          \
    NS_TRY_BEGIN

/* Closes a wrapper opened with NS_ENTER or NS_ENTER_ANY_THREAD. */
#define NS_LEAVE()                                                            \
  NS_TRY_END                                                                  \
  }

/* ---------------------------------------------------------------------------
 * Handles (KTD9, R4, R7)
 *
 * A handle is the bridged object pointer behind a distinct opaque struct type.
 * The cast carries no retain in either direction; ownership is entirely in
 * which outbound macro the wrapper picks.
 * ------------------------------------------------------------------------- */

/* Inbound at a _Nonnull position: the checked `cls` behind `handle`. */
#define NS_IN(cls, handle)                                                    \
  (NS_CHECK_HANDLE((handle), cls, true), (__bridge cls *)(void *)(handle))

/* Inbound at a _Nullable position: nil passes, a wrong class does not. */
#define NS_IN_OPT(cls, handle)                                                \
  (NS_CHECK_HANDLE((handle), cls, false), (__bridge cls *)(void *)(handle))

/* Outbound borrowed (R7): only for the getter of a strong, weak or assign
 * object property. The caller keeps it past its owner with ns_retain. */
#define NS_OUT(type, object) ((type *)(__bridge void *)(object))

/* Outbound owned, +1 (R7, KTD7): for create_ and copy_, and for anything
 * behind a copy property. The caller ends it with ns_release. */
#define NS_OUT_OWNED(type, object)                                            \
  ((type *)(void *)CFBridgingRetain(object))

/* ---------------------------------------------------------------------------
 * Strings (R11, KTD7)
 * ------------------------------------------------------------------------- */

/* nil for null; the string for valid UTF-8; nil for invalid UTF-8, which the
 * NS_STRING_IN macros report first in a debug build. */
NSString *ns_internal_string_in(const char *utf8);

/* A strdup copy the caller owns and frees with ns_string_free; null for nil. */
char *ns_internal_string_out(NSString *string);

/* Inbound at a _Nonnull position: null is misuse (R12). */
#define NS_STRING_IN(utf8)                                                    \
  (NS_CHECK_UTF8((utf8), true), ns_internal_string_in(utf8))

/* Inbound at a _Nullable position: null maps to nil. */
#define NS_STRING_IN_OPT(utf8)                                                \
  (NS_CHECK_UTF8((utf8), false), ns_internal_string_in(utf8))

/* Outbound: always an owned copy (R11), never a pointer into the object. */
#define NS_STRING_OUT(string) ns_internal_string_out(string)

#endif /* SYNTONIC_SRC_NS_INTERNAL_H */

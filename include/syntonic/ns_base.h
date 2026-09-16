/*
 * Syntonic's kernel: the mechanics every wrapper shares.
 *
 * This is a non-mirror header (KTD18): nothing here wraps a single AppKit
 * class. It holds the five Syntonic-owned utilities and the one callback type
 * that the rest of the library is written against, and it records the four
 * rules a caller has to know. Every wrapper header assumes these rules; none
 * of them repeats the explanation.
 *
 * OWNERSHIP (R7, KTD7)
 *   A function named create_ or copy_ returns a reference the caller owns and
 *   ends with ns_release; a returned string is an owned copy the caller ends
 *   with ns_string_free. Every other function returns a borrowed reference,
 *   valid only while its owner holds it - keep one past that point by calling
 *   ns_retain and pairing it with ns_release. AppKit's own retention is never
 *   touched, so handing an object to a parent and then releasing the caller's
 *   reference is correct and leaves the parent's reference intact. Releasing
 *   null is a no-op.
 *
 * AVAILABILITY (R6, R8, KTD5)
 *   Every declaration carries API_AVAILABLE and the floor is macOS 26. The
 *   rule a wrapper follows: when the SDK annotates the API it wraps, mirror
 *   that version; when the SDK annotates nothing - some AppKit headers carry
 *   no annotation at all - declare the floor, API_AVAILABLE(macos(26.0)).
 *   The library adds no per-call guard: an API the running system does not
 *   have behaves exactly as it does in AppKit, and the caller guards. A C
 *   source guards with __builtin_available; see ns_available below.
 *
 * THREADING (R10, AE3)
 *   Every function in Syntonic is main-thread only, with two exceptions named
 *   in their own comments: ns_main_thread_dispatch, which exists to cross onto
 *   the main thread, and ns_string_free, which touches no Objective-C object.
 *   A debug build detects a call from any other thread, names the function,
 *   and stops the process.
 *
 * MISUSE (R12, KTD4)
 *   Each handle parameter and return is marked _Nonnull or _Nullable after the
 *   AppKit API it wraps. In a debug build, null at a _Nonnull position, a
 *   handle whose class is neither the expected class nor a subclass of it, and
 *   an Objective-C exception raised inside a call are each reported to stderr
 *   with the AppKit class and the function name, and stop the process. A
 *   release build - NDEBUG - compiles all of that out and pays nothing for it,
 *   and lets an exception propagate as it does from Objective-C code.
 */

#ifndef SYNTONIC_NS_BASE_H
#define SYNTONIC_NS_BASE_H

#include <os/availability.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * A C callback Syntonic calls back into. `context` is whatever the caller
 * handed over when installing it, untouched and never freed by the library;
 * `sender` is a borrowed handle to the object that triggered the call, valid
 * only for the duration of the callback, and null when there is no sender.
 * syntonic-owned.
 */
typedef void (*ns_action)(void *_Nullable context, const void *_Nullable sender);

/*
 * Claims a reference to any handle and returns that same handle, so a borrowed
 * reference can outlive its owner. Balance every call with one ns_release.
 * Null is a no-op and returns null. Main-thread only: this changes the
 * object's retain count. syntonic-owned.
 */
const void *_Nullable ns_retain(const void *_Nullable handle)
    API_AVAILABLE(macos(26.0));

/*
 * Gives up one reference to any handle: the one an ns_retain claimed, or the
 * one a create_ or copy_ function returned. Null is a no-op (R7). Main-thread
 * only: this changes the object's retain count. syntonic-owned.
 */
void ns_release(const void *_Nullable handle) API_AVAILABLE(macos(26.0));

/*
 * Frees a UTF-8 string Syntonic returned. Null is a no-op. Unlike the rest of
 * the library this may be called from any thread: the string is a plain
 * malloc'd copy and freeing it touches no Objective-C object. syntonic-owned.
 */
void ns_string_free(char *_Nullable owned_utf8) API_AVAILABLE(macos(26.0));

/*
 * True when the running macOS is at least `major`.`minor`. This is a runtime
 * answer only; it does not satisfy the compiler. A C source that calls an API
 * newer than the deployment floor guards it with clang's own construct,
 *
 *     if (__builtin_available(macOS 27.0, *)) { ns_new_thing_create(); }
 *
 * which silences -Wunguarded-availability-new; a call guarded by an if on
 * ns_available still warns, because clang only understands literal versions
 * there. ns_available is for callers that have no compile-time construct to
 * reach for - a binding generator, a foreign runtime, a dispatch table built
 * from a version number. syntonic-owned.
 */
bool ns_available(int major, int minor) API_AVAILABLE(macos(26.0));

/*
 * Runs `action` on the main thread with `context` and a null sender. The only
 * function in Syntonic that may be called from any thread, and the way work on
 * another thread reaches AppKit. The call is always asynchronous, from the
 * main thread as well: `action` never runs before this function returns, it
 * runs on a later turn of the main run loop. `context` is not copied or freed;
 * it has to stay valid until `action` runs. syntonic-owned.
 */
void ns_main_thread_dispatch(ns_action _Nonnull action, void *_Nullable context)
    API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_BASE_H */

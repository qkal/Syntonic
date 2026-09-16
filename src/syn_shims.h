/*
 * The callback machinery: one shim class per wrapped protocol, one trampoline
 * per target/action (R9, KTD8).
 *
 * This header never ships. Like src/ns_internal.h it is internal, so it may
 * have function-like macros, and like ns_internal.h it carries no nullability
 * annotations: -Wnullability-completeness is file-scoped.
 *
 * WHAT A PROTOCOL LOOKS LIKE, END TO END
 *
 * The public header declares the struct, one member per wrapped selector, and
 * the installer. The .m declares the member table, the shim subclass, one
 * method per member, and the installer's body:
 *
 *     SYN_SHIM_TABLE(syn_window_table, ns_window_callbacks, "NSWindowDelegate",
 *                    SYN_SHIM_OPTIONAL(ns_window_callbacks, will_close,
 *                                      "windowWillClose:"));
 *
 *     @interface SynWindowShim : SynShim <NSWindowDelegate>
 *     @end
 *
 *     @implementation SynWindowShim
 *     - (void)windowWillClose:(NSNotification *)notification {
 *       SYN_SHIM_ENTER(NSWindow, notification.object);
 *       void (*fn)(void *, ns_window *) =
 *           SYN_SHIM_FN(ns_window_callbacks, will_close);
 *       if (fn != NULL) fn(syn_context, NS_OUT(ns_window, syn_sender));
 *       SYN_SHIM_LEAVE();
 *     }
 *     @end
 *
 *     void ns_window_set_callbacks(ns_window *window,
 *                                  const ns_window_callbacks *callbacks,
 *                                  void *context) {
 *       NS_ENTER();
 *       NSWindow *target = NS_IN(NSWindow, window);
 *       SYN_SHIM_INSTALL(target, SynWindowShim, syn_window_table, callbacks,
 *                        context, ^(id shim) { target.delegate = shim; });
 *       NS_LEAVE();
 *     }
 *
 * A merged struct (KTD6) differs in one line: the assign block sets both AppKit
 * slots, `target.delegate = shim; target.dataSource = shim;`, and the shim
 * subclass adopts both protocols. One shim, one install, one association.
 *
 * WHAT THE MACHINERY GUARANTEES, SO A WRAPPER NEVER HAS TO THINK ABOUT IT
 *
 *   * The struct is copied at install and the copy belongs to the shim; the
 *     context is not copied and belongs to the caller.
 *   * A required member that is not set is reported and the process stops,
 *     before the AppKit slot is assigned (R9).
 *   * respondsToSelector: answers from struct membership from the moment the
 *     shim exists, because AppKit caches the answer when the slot is assigned.
 *     An unset optional member is a method the object does not implement.
 *   * Install assigns the AppKit slot first and swaps the association second;
 *     the association is what keeps the shim alive, since AppKit holds a
 *     delegate weakly. Installing again releases the previous shim; installing
 *     a null struct uninstalls. ns_release on the served object does not.
 *   * A shim method holds the shim and a +1 on the sender until AppKit's
 *     dispatch returns, so a callback may re-install its own struct, release
 *     the sender, or close the window it was handed.
 *   * A shim never holds a strong reference to the object it serves.
 */

#ifndef SYNTONIC_SRC_SYN_SHIMS_H
#define SYNTONIC_SRC_SYN_SHIMS_H

#import <Foundation/Foundation.h>

#include <stdbool.h>
#include <stddef.h>

#include "syntonic/ns_base.h"

/* ---------------------------------------------------------------------------
 * The member table (R9, KTD8)
 *
 * One row per struct member: what to call it in a report, which selector it
 * answers, where it sits in the struct, and whether docs/conventions.md's
 * per-protocol table lists it as required. Every protocol v0 wraps is entirely
 * @optional in the SDK, so that table - not the SDK - is the source.
 * ------------------------------------------------------------------------- */

typedef struct syn_shim_member {
  const char *name;     /* the C member, as the debug report spells it */
  const char *selector; /* the AppKit selector the member answers */
  size_t offset;        /* offsetof(callbacks struct, member) */
  bool required;        /* per docs/conventions.md, not per the SDK */
} syn_shim_member;

typedef struct syn_shim_table {
  const char *protocol;           /* "NSWindowDelegate", for the report */
  size_t size;                    /* sizeof the callbacks struct */
  const syn_shim_member *members; /* one row per member, any order */
  size_t count;
} syn_shim_table;

/* A row. `member` may be a path into an embedded parent struct, such as
 * text_field.did_change, and the report prints that path. */
#define SYN_SHIM_REQUIRED(type, member, selector)                             \
  {#member, (selector), offsetof(type, member), true}
#define SYN_SHIM_OPTIONAL(type, member, selector)                             \
  {#member, (selector), offsetof(type, member), false}

/* The whole table in one declaration:
 *
 *   SYN_SHIM_TABLE(syn_table_view_table, ns_table_view_callbacks,
 *                  "NSTableViewDataSource + NSTableViewDelegate",
 *                  SYN_SHIM_REQUIRED(ns_table_view_callbacks, number_of_rows,
 *                                    "numberOfRowsInTableView:"),
 *                  SYN_SHIM_OPTIONAL(ns_table_view_callbacks,
 *                                    selection_did_change,
 *                                    "tableViewSelectionDidChange:"));
 */
#define SYN_SHIM_TABLE(name, type, protocol, ...)                             \
  static const syn_shim_member name##_members[] = {__VA_ARGS__};              \
  static const syn_shim_table name = {                                        \
      (protocol), sizeof(type), name##_members,                               \
      sizeof(name##_members) / sizeof(*name##_members)}

/* ---------------------------------------------------------------------------
 * The shim base class (KTD8)
 *
 * One subclass per wrapped protocol adopts the protocol and implements every
 * member's selector. The base holds the struct copy and the context, and
 * answers respondsToSelector: from the copy.
 * ------------------------------------------------------------------------- */

@interface SynShim : NSObject

/* Takes a copy of `callbacks`, `table->size` bytes of it, and keeps `context`
 * as given. Use SYN_SHIM_INSTALL rather than calling this directly. */
- (instancetype)initWithTable:(const syn_shim_table *)table
                    callbacks:(const void *)callbacks
                      context:(void *)context;

/* What the caller handed the installer, untouched and never freed. */
@property(readonly, nonatomic) void *synContext;

/* The address of one member inside the struct copy. SYN_SHIM_FN is the way to
 * use it; it supplies the offset and the member's type. */
- (void *)synSlotAtOffset:(size_t)offset;

@end

/* Opens a shim callback body. Holds the shim and the sender for the whole of
 * AppKit's dispatch (KTD8) and opens this call's autorelease pool, then
 * declares the two things every member needs:
 *
 *   syn_sender    the `sender_class` that fired, held +1 for the call
 *   syn_context   the context the caller installed
 *
 * `sender_expr` is the AppKit object the callback reports on: the method's own
 * sender argument, or a lone notification's `object` (R5). Pair with
 * SYN_SHIM_LEAVE. */
#define SYN_SHIM_ENTER(sender_class, sender_expr)                             \
  NS_VALID_UNTIL_END_OF_SCOPE SynShim *syn_shim = self;                       \
  NS_VALID_UNTIL_END_OF_SCOPE sender_class *syn_sender = (sender_expr);       \
  void *syn_context = syn_shim.synContext;                                    \
  @autoreleasepool {

/* Closes a body opened with SYN_SHIM_ENTER. A `return` from between the two is
 * correct: the pool drains and the two references drop after it. */
#define SYN_SHIM_LEAVE() }

/* The member's function pointer out of the struct copy, or null when the
 * caller left it unset. Only valid between SYN_SHIM_ENTER and SYN_SHIM_LEAVE,
 * which is where `syn_shim` comes from. Test it against null even for a member
 * respondsToSelector: reports as absent: AppKit caches that answer and a stale
 * cache would otherwise call through a null pointer. */
#define SYN_SHIM_FN(type, member)                                             \
  (*(__typeof__(((type *)NULL)->member) *)[syn_shim                           \
      synSlotAtOffset:offsetof(type, member)])

/* ---------------------------------------------------------------------------
 * Install, replace and uninstall (KTD8)
 * ------------------------------------------------------------------------- */

/* Assigns the AppKit slots this protocol lives in. Called with the new shim,
 * or with nil to uninstall. A merged struct assigns both of its slots here. */
typedef void (^syn_shim_assign)(id shim);

void syn_shim_install(id object, Class shim_class, const syn_shim_table *table,
                      const void *callbacks, void *context,
                      syn_shim_assign assign, const char *function);

/* The one call a wrapper makes. `table` is named, not addressed; `callbacks`
 * null uninstalls. */
#define SYN_SHIM_INSTALL(object, shim_class, table, callbacks, context,       \
                         assign)                                              \
  syn_shim_install((object), [shim_class class], &(table), (callbacks),       \
                   (context), (assign), __func__)

/* Takes the owned handle a callback returned and hands its reference to ARC,
 * which drops it once AppKit has retained the object (R9). A callback that
 * returns a handle returns an ns_..._create_... result directly and never
 * releases it itself. */
id syn_shim_take(const void *handle);

/* ---------------------------------------------------------------------------
 * Debug return validation (KTD4, KTD8)
 *
 * What a callback hands back is checked before AppKit sees it, and the report
 * names the protocol, the member and the value. Compiled out under NDEBUG.
 * ------------------------------------------------------------------------- */

#ifndef NDEBUG

/* How many shims are alive. The suites watch a replaced or uninstalled shim
 * go away with it. */
long syn_shim_live_count(void);

void syn_shim_report_count(const syn_shim_table *table, const char *member,
                           long count);
void syn_shim_report_null(const syn_shim_table *table, const char *member,
                          const void *value);
void syn_shim_report_utf8(const syn_shim_table *table, const char *member,
                          const char *utf8);
void syn_shim_report_handle(const syn_shim_table *table, const char *member,
                            const void *handle, Class expected, bool required);

/* A count a callback returned is not negative. */
#define SYN_SHIM_CHECK_COUNT(table, member, count)                            \
  syn_shim_report_count(&(table), #member, (count))

/* A pointer a callback returned is not null where the member never returns
 * null: a cell's borrowed string, an outline's child item. Unlike
 * SYN_SHIM_CHECK_HANDLE this asks nothing of the pointer beyond that, because
 * what comes back is a C string or one of the caller's own item pointers, not
 * an object handle (KTD6, KTD8). */
#define SYN_SHIM_CHECK_NONNULL(table, member, value)                          \
  syn_shim_report_null(&(table), #member, (const void *)(value))

/* A string a callback returned is valid UTF-8, which is the same rule a string
 * at a wrapper parameter is held to (R11). Null passes: whether null is an
 * answer or a fault is SYN_SHIM_CHECK_NONNULL's question, not this one's. */
#define SYN_SHIM_CHECK_UTF8(table, member, utf8)                              \
  syn_shim_report_utf8(&(table), #member, (utf8))

/* A handle a callback returned is non-null where the SDK says non-null, and is
 * the expected class or a subclass of it (R12). */
#define SYN_SHIM_CHECK_HANDLE(table, member, handle, cls, required)           \
  syn_shim_report_handle(&(table), #member, (const void *)(handle),           \
                         [cls class], (required))

#else /* NDEBUG */

#define SYN_SHIM_CHECK_COUNT(table, member, count) ((void)0)
#define SYN_SHIM_CHECK_NONNULL(table, member, value) ((void)0)
#define SYN_SHIM_CHECK_UTF8(table, member, utf8) ((void)0)
#define SYN_SHIM_CHECK_HANDLE(table, member, handle, cls, required) ((void)0)

#endif /* NDEBUG */

/* ---------------------------------------------------------------------------
 * The target/action trampoline (R9)
 *
 * An object that calls one ns_action with its context and the sender's handle,
 * associated with the sender so it lives exactly as long as it. Installing
 * again replaces it; a null action uninstalls.
 * ------------------------------------------------------------------------- */

/* Assigns the target and action slots. Called with the trampoline and its
 * selector, or with nil and NULL to uninstall. */
typedef void (^syn_action_assign)(id target, SEL action);

void syn_install_action(id object, ns_action action, void *context,
                        syn_action_assign assign);

#endif /* SYNTONIC_SRC_SYN_SHIMS_H */

/*
 * The callback machinery (R9, KTD8). Read src/syn_shims.h first: it carries
 * the worked example and the guarantees this file implements.
 *
 * Nullability annotations belong to the public headers, not here.
 */

#import <Foundation/Foundation.h>
#import <objc/runtime.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "syn_shims.h"

/* One association per object for the protocol shim and one for the
 * target/action trampoline. The address is the key; the value is never read. */
static const char syn_shim_key;
static const char syn_action_key;

/* ---------------------------------------------------------------------------
 * The shim base class
 * ------------------------------------------------------------------------- */

#ifndef NDEBUG
/* How many shims exist right now. A replaced or uninstalled shim has to go
 * away immediately, and the leak gate only ever sees the aggregate, so the
 * suites watch this instead (KTD8). Debug builds only. */
static long syn_shim_live;

long syn_shim_live_count(void) {
  return syn_shim_live;
}
#endif

@implementation SynShim {
  const syn_shim_table *_table;
  void *_callbacks; /* a malloc'd copy of _table->size bytes */
}

- (instancetype)initWithTable:(const syn_shim_table *)table
                    callbacks:(const void *)callbacks
                      context:(void *)context {
  self = [super init];
  if (self == nil) return nil;
  _table = table;
  /* The struct is copied so the caller may build it on the stack (R9); the
   * context is not, so it stays the caller's to keep valid. */
  _callbacks = malloc(table->size);
  memcpy(_callbacks, callbacks, table->size);
  _synContext = context;
#ifndef NDEBUG
  syn_shim_live++;
#endif
  return self;
}

- (void)dealloc {
  free(_callbacks);
#ifndef NDEBUG
  syn_shim_live--;
#endif
}

- (void *)synSlotAtOffset:(size_t)offset {
  return (char *)_callbacks + offset;
}

/* KTD8: AppKit caches which optional methods a delegate implements when the
 * slot is assigned, so this has to be right from the moment the shim exists.
 * A member the struct left null is a method this object does not implement.
 *
 * More than one member may feed one selector - the outline's `cell_string` and
 * `cell_symbol_name` are both read inside
 * outlineView:viewForTableColumn:item: - so the answer is yes as soon as any
 * of them is set, and no only when the table names the selector and every
 * member of it is null. */
- (BOOL)respondsToSelector:(SEL)selector {
  const char *name = sel_getName(selector);
  bool named = false;
  for (size_t i = 0; i < _table->count; i++) {
    const syn_shim_member *member = &_table->members[i];
    if (strcmp(name, member->selector) != 0) continue;
    named = true;
    if (*(void *const *)((const char *)_callbacks + member->offset) != NULL)
      return YES;
  }
  if (named) return NO;
  return [super respondsToSelector:selector];
}

@end

/* ---------------------------------------------------------------------------
 * Install, replace and uninstall
 * ------------------------------------------------------------------------- */

#ifndef NDEBUG

/* Same shape as the checks in ns_base.m: the report, then stop (KTD4). */
__attribute__((noreturn)) static void syn_shim_stop(void) {
  fflush(stderr);
  abort();
}

/* R9: an unset required member is reported here, before AppKit ever asks. */
static void syn_shim_check_required(const syn_shim_table *table,
                                    const void *callbacks,
                                    const char *function) {
  for (size_t i = 0; i < table->count; i++) {
    const syn_shim_member *member = &table->members[i];
    if (!member->required) continue;
    if (*(void *const *)((const char *)callbacks + member->offset) != NULL)
      continue;
    fprintf(stderr,
            "syntonic: %s: %s requires the member %s, which this callbacks "
            "struct leaves unset; docs/conventions.md's per-protocol table is "
            "what makes it required (R9, KTD8).\n",
            function, table->protocol, member->name);
    syn_shim_stop();
  }
}

void syn_shim_report_count(const syn_shim_table *table, const char *member,
                           long count) {
  if (count >= 0) return;
  fprintf(stderr,
          "syntonic: %s's member %s returned the count %ld; a count crossing "
          "back into AppKit is never negative (KTD8).\n",
          table->protocol, member, count);
  syn_shim_stop();
}

void syn_shim_report_null(const syn_shim_table *table, const char *member,
                          const void *value) {
  if (value != NULL) return;
  fprintf(stderr,
          "syntonic: %s's member %s returned null; this member never returns "
          "null (KTD8).\n",
          table->protocol, member);
  syn_shim_stop();
}

void syn_shim_report_handle(const syn_shim_table *table, const char *member,
                            const void *handle, Class expected,
                            bool required) {
  if (handle == NULL) {
    if (!required) return;
    fprintf(stderr,
            "syntonic: %s's member %s returned null where the SDK declares a "
            "non-null %s (KTD8).\n",
            table->protocol, member, class_getName(expected));
    syn_shim_stop();
  }

  id object = (__bridge id)(void *)handle;
  if ([object isKindOfClass:expected]) return;
  fprintf(stderr,
          "syntonic: %s's member %s returned a handle to %s, which is neither "
          "%s nor a subclass of it (R12, KTD8).\n",
          table->protocol, member, object_getClassName(object),
          class_getName(expected));
  syn_shim_stop();
}

#endif /* NDEBUG */

void syn_shim_install(id object, Class shim_class, const syn_shim_table *table,
                      const void *callbacks, void *context,
                      syn_shim_assign assign, const char *function) {
  SynShim *shim = nil;
  if (callbacks != NULL) {
#ifndef NDEBUG
    syn_shim_check_required(table, callbacks, function);
#else
    (void)function;
#endif
    shim = [[shim_class alloc] initWithTable:table
                                   callbacks:callbacks
                                     context:context];
  }

  /* KTD8, in this order and not the other one: AppKit reads the delegate's
   * respondsToSelector: answers as it assigns the slot, and it holds the
   * delegate weakly, so the association that keeps the shim alive is swapped
   * second. Setting the association releases the shim that was there; a
   * callback still on the stack holds its own reference and survives it. */
  assign(shim);
  objc_setAssociatedObject(object, &syn_shim_key, shim,
                           OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

id syn_shim_take(const void *handle) {
  /* +1 from the callback, handed to ARC, which drops it when the shim method's
   * pool drains - by which point AppKit has retained what it wanted (R9). */
  return CFBridgingRelease(handle);
}

/* ---------------------------------------------------------------------------
 * The target/action trampoline
 * ------------------------------------------------------------------------- */

@interface SynTrampoline : NSObject
- (instancetype)initWithAction:(ns_action)action context:(void *)context;
- (void)synFire:(id)sender;
@end

@implementation SynTrampoline {
  ns_action _action;
  void *_context;
}

- (instancetype)initWithAction:(ns_action)action context:(void *)context {
  self = [super init];
  if (self == nil) return nil;
  _action = action;
  _context = context;
  return self;
}

- (void)synFire:(id)sender {
  /* The same lifetime rule as a shim method: an action that releases the
   * sender or replaces itself must not pull the ground out from under the
   * call still on the stack (KTD8). */
  NS_VALID_UNTIL_END_OF_SCOPE SynTrampoline *trampoline = self;
  NS_VALID_UNTIL_END_OF_SCOPE id held = sender;
  @autoreleasepool {
    trampoline->_action(trampoline->_context, (__bridge const void *)held);
  }
}

@end

void syn_install_action(id object, ns_action action, void *context,
                        syn_action_assign assign) {
  SynTrampoline *trampoline = nil;
  if (action != NULL)
    trampoline = [[SynTrampoline alloc] initWithAction:action context:context];

  /* AppKit's target slot is weak too, so the ordering is the shim's ordering:
   * slots first, association second, previous trampoline released with it. */
  assign(trampoline, trampoline != nil ? @selector(synFire:) : NULL);
  objc_setAssociatedObject(object, &syn_action_key, trampoline,
                           OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

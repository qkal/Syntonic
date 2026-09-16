/*
 * NSToolbar and its delegate (R16, R9, F2, KTD8, KTD17).
 *
 * Two of NSToolbarDelegate's three required methods are answered here from
 * data the caller passed to the installer rather than from the struct, which
 * is what KTD8 means by a member served from stored data. The identifier lists
 * are deep-copied at install: AppKit asks for them after the install returns,
 * for as long as the toolbar lives, so a borrowed array would dangle.
 */

#import <AppKit/AppKit.h>
#import <objc/runtime.h>

#include "ns_internal.h"
#include "syn_shims.h"
#include "syntonic/ns_toolbar.h"
#include "syntonic/ns_toolbar_item.h"

/* NSToolbarDelegate is entirely @optional in the SDK; docs/conventions.md's
 * per-protocol table is what makes the one member below required (R9, KTD8).
 * toolbarDefaultItemIdentifiers: and toolbarAllowedItemIdentifiers: are
 * required too and have no row here: the shim answers them itself. */
SYN_SHIM_TABLE(
    syn_toolbar_table, ns_toolbar_callbacks, "NSToolbarDelegate",
    SYN_SHIM_REQUIRED(ns_toolbar_callbacks,
                      item_for_item_identifier_will_be_inserted_into_toolbar,
                      "toolbar:itemForItemIdentifier:"
                      "willBeInsertedIntoToolbar:"));

/* The copies of the two identifier lists, held by the toolbar itself so they
 * outlive the install exactly as long as AppKit may ask for them. The address
 * is the key; the value is the copy. */
static const char syn_toolbar_default_identifiers_key;
static const char syn_toolbar_allowed_identifiers_key;

static NSArray<NSToolbarItemIdentifier> *syn_toolbar_identifiers(
    NSToolbar *toolbar, const void *key) {
  NSArray<NSToolbarItemIdentifier> *identifiers =
      objc_getAssociatedObject(toolbar, key);
  return identifiers != nil ? identifiers : @[];
}

@interface SynToolbarShim : SynShim <NSToolbarDelegate>
@end

@implementation SynToolbarShim

/* R9: the member hands back an owned item, the wrapper hands the item to
 * AppKit, and syn_shim_take drops the reference once AppKit has retained it -
 * so the C side never keeps a reference it did not create. Null is an answer,
 * not an error: the SDK declares the return nullable, which is why the handle
 * check is not a required one. */
- (NSToolbarItem *)toolbar:(NSToolbar *)toolbar
        itemForItemIdentifier:(NSToolbarItemIdentifier)itemIdentifier
    willBeInsertedIntoToolbar:(BOOL)flag {
  SYN_SHIM_ENTER(NSToolbar, toolbar);
  ns_toolbar_item *(*callback)(void *, ns_toolbar *, const char *, bool) =
      SYN_SHIM_FN(ns_toolbar_callbacks,
                  item_for_item_identifier_will_be_inserted_into_toolbar);
  if (callback == NULL) return nil;
  ns_toolbar_item *item =
      callback(syn_context, NS_OUT(ns_toolbar, syn_sender),
               itemIdentifier.UTF8String, flag != NO);
  SYN_SHIM_CHECK_HANDLE(syn_toolbar_table,
                        item_for_item_identifier_will_be_inserted_into_toolbar,
                        item, NSToolbarItem, false);
  return syn_shim_take(item);
  SYN_SHIM_LEAVE();
}

/* KTD8: served from stored data, so there is no struct member and no C
 * callback to cross into. Nothing here can re-enter the shim or release the
 * toolbar, which is why these two carry a plain pool rather than the entry
 * macro's sender hold. */
- (NSArray<NSToolbarItemIdentifier> *)toolbarDefaultItemIdentifiers:
    (NSToolbar *)toolbar {
  @autoreleasepool {
    return syn_toolbar_identifiers(toolbar,
                                   &syn_toolbar_default_identifiers_key);
  }
}

- (NSArray<NSToolbarItemIdentifier> *)toolbarAllowedItemIdentifiers:
    (NSToolbar *)toolbar {
  @autoreleasepool {
    return syn_toolbar_identifiers(toolbar,
                                   &syn_toolbar_allowed_identifiers_key);
  }
}

@end

/* Owned (+1): an initWith… returns a fresh object the caller ends with
 * ns_release (R7, KTD7). */
ns_toolbar *ns_toolbar_create_with_identifier(const char *identifier) {
  NS_ENTER();
  return NS_OUT_OWNED(ns_toolbar,
                      [[NSToolbar alloc]
                          initWithIdentifier:NS_STRING_IN(identifier)]);
  NS_LEAVE();
}

char *ns_toolbar_copy_identifier(ns_toolbar *toolbar) {
  NS_ENTER();
  return NS_STRING_OUT(NS_IN(NSToolbar, toolbar).identifier);
  NS_LEAVE();
}

long ns_toolbar_item_count(ns_toolbar *toolbar) {
  NS_ENTER();
  return (long)NS_IN(NSToolbar, toolbar).items.count;
  NS_LEAVE();
}

/* Borrowed: the toolbar keeps the item, so the accessor reads the live child
 * through the receiver (R7, KTD17). No index check of its own: NSArray raises
 * before it changes anything and the entry macro's exception report names this
 * function (KTD4). */
ns_toolbar_item *ns_toolbar_item_at_index(ns_toolbar *toolbar, long index) {
  NS_ENTER();
  return NS_OUT(ns_toolbar_item,
                NS_IN(NSToolbar, toolbar).items[(NSUInteger)index]);
  NS_LEAVE();
}

void ns_toolbar_set_allows_user_customization(ns_toolbar *toolbar,
                                              bool allows_customization) {
  NS_ENTER();
  NS_IN(NSToolbar, toolbar).allowsUserCustomization = allows_customization;
  NS_LEAVE();
}

bool ns_toolbar_allows_user_customization(ns_toolbar *toolbar) {
  NS_ENTER();
  return NS_IN(NSToolbar, toolbar).allowsUserCustomization;
  NS_LEAVE();
}

void ns_toolbar_set_display_mode(ns_toolbar *toolbar,
                                 ns_toolbar_display_mode display_mode) {
  NS_ENTER();
  NS_IN(NSToolbar, toolbar).displayMode = (NSToolbarDisplayMode)display_mode;
  NS_LEAVE();
}

ns_toolbar_display_mode ns_toolbar_get_display_mode(ns_toolbar *toolbar) {
  NS_ENTER();
  return (ns_toolbar_display_mode)NS_IN(NSToolbar, toolbar).displayMode;
  NS_LEAVE();
}

/* KTD17's named exception: the arrays are copied here, string by string,
 * because AppKit asks for them after this call returns and for as long as the
 * toolbar lives. The copies go on before the delegate slot is assigned - that
 * assignment is what makes AppKit start asking. */
void ns_toolbar_set_callbacks(ns_toolbar *toolbar,
                              const ns_toolbar_callbacks *callbacks,
                              const char *const *default_item_identifiers,
                              long default_item_identifier_count,
                              const char *const *allowed_item_identifiers,
                              long allowed_item_identifier_count,
                              void *context) {
  NS_ENTER();
  NSToolbar *target = NS_IN(NSToolbar, toolbar);

  NSMutableArray<NSToolbarItemIdentifier> *defaults = nil;
  NSMutableArray<NSToolbarItemIdentifier> *allowed = nil;
  if (callbacks != NULL) {
    defaults = NS_STRING_ARRAY_IN(default_item_identifiers,
                                  default_item_identifier_count, nil);
    allowed = NS_STRING_ARRAY_IN(allowed_item_identifiers,
                                 allowed_item_identifier_count, nil);
  }
  objc_setAssociatedObject(target, &syn_toolbar_default_identifiers_key,
                           defaults, OBJC_ASSOCIATION_COPY_NONATOMIC);
  objc_setAssociatedObject(target, &syn_toolbar_allowed_identifiers_key,
                           allowed, OBJC_ASSOCIATION_COPY_NONATOMIC);

  SYN_SHIM_INSTALL(target, SynToolbarShim, syn_toolbar_table, callbacks,
                   context, ^(id shim) { target.delegate = shim; });
  NS_LEAVE();
}

/*
 * NSMenuItem and its target/action (R18, R9, KTD7).
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syn_shims.h"
#include "syntonic/ns_menu_item.h"

/* Owned (+1): the caller ends it with ns_release (R7, KTD7). AppKit's
 * `action:` segment is a SEL and never crosses the boundary (R11), so the item
 * is built with none and ns_menu_item_set_action installs one. */
ns_menu_item *ns_menu_item_create_with_title_key_equivalent(
    const char *title, const char *key_equivalent) {
  NS_ENTER();
  return NS_OUT_OWNED(
      ns_menu_item,
      [[NSMenuItem alloc] initWithTitle:NS_STRING_IN(title)
                                 action:NULL
                          keyEquivalent:NS_STRING_IN(key_equivalent)]);
  NS_LEAVE();
}

/* Owned (+1): +separatorItem is a plain class method returning a fresh item,
 * so it is an owned return like any other (R7). */
ns_menu_item *ns_menu_item_create_separator_item(void) {
  NS_ENTER();
  return NS_OUT_OWNED(ns_menu_item, [NSMenuItem separatorItem]);
  NS_LEAVE();
}

void ns_menu_item_set_title(ns_menu_item *item, const char *title) {
  NS_ENTER();
  NS_IN(NSMenuItem, item).title = NS_STRING_IN(title);
  NS_LEAVE();
}

/* `title` is a copy property, so the value is owned and the name says copy_
 * (R7, R11). */
char *ns_menu_item_copy_title(ns_menu_item *item) {
  NS_ENTER();
  return NS_STRING_OUT(NS_IN(NSMenuItem, item).title);
  NS_LEAVE();
}

void ns_menu_item_set_key_equivalent(ns_menu_item *item,
                                     const char *key_equivalent) {
  NS_ENTER();
  NS_IN(NSMenuItem, item).keyEquivalent = NS_STRING_IN(key_equivalent);
  NS_LEAVE();
}

/* `keyEquivalent` is a copy property, so the value is owned (R7, R11). */
char *ns_menu_item_copy_key_equivalent(ns_menu_item *item) {
  NS_ENTER();
  return NS_STRING_OUT(NS_IN(NSMenuItem, item).keyEquivalent);
  NS_LEAVE();
}

void ns_menu_item_set_key_equivalent_modifier_mask(
    ns_menu_item *item, ns_event_modifier_flags modifiers) {
  NS_ENTER();
  NS_IN(NSMenuItem, item).keyEquivalentModifierMask =
      (NSEventModifierFlags)modifiers;
  NS_LEAVE();
}

ns_event_modifier_flags ns_menu_item_key_equivalent_modifier_mask(
    ns_menu_item *item) {
  NS_ENTER();
  return (ns_event_modifier_flags)NS_IN(NSMenuItem, item)
      .keyEquivalentModifierMask;
  NS_LEAVE();
}

void ns_menu_item_set_enabled(ns_menu_item *item, bool enabled) {
  NS_ENTER();
  NS_IN(NSMenuItem, item).enabled = enabled;
  NS_LEAVE();
}

bool ns_menu_item_enabled(ns_menu_item *item) {
  NS_ENTER();
  return NS_IN(NSMenuItem, item).enabled;
  NS_LEAVE();
}

bool ns_menu_item_separator_item(ns_menu_item *item) {
  NS_ENTER();
  return NS_IN(NSMenuItem, item).separatorItem;
  NS_LEAVE();
}

void ns_menu_item_set_submenu(ns_menu_item *item, ns_menu *submenu) {
  NS_ENTER();
  NS_IN(NSMenuItem, item).submenu = NS_IN_OPT(NSMenu, submenu);
  NS_LEAVE();
}

/* Borrowed: `submenu` is a strong property (R7). */
ns_menu *ns_menu_item_submenu(ns_menu_item *item) {
  NS_ENTER();
  return NS_OUT(ns_menu, NS_IN(NSMenuItem, item).submenu);
  NS_LEAVE();
}

/* The trampoline is installed, replaced and uninstalled by one call; the block
 * is where this class's target and action slots live (R9). Uninstalling clears
 * both, which leaves a standard item's responder-chain selector gone too - an
 * item carries one action, not a stack of them. */
void ns_menu_item_set_action(ns_menu_item *item, ns_action action,
                             void *context) {
  NS_ENTER();
  NSMenuItem *target = NS_IN(NSMenuItem, item);
  syn_install_action(target, action, context, ^(id trampoline, SEL selector) {
    target.target = trampoline;
    target.action = selector;
  });
  NS_LEAVE();
}

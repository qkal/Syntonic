/*
 * NSToolbarItem - one item in a window toolbar (R16, R23, R9).
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syn_shims.h"
#include "syntonic/ns_toolbar_item.h"

/* R5, R11: AppKit's own identifiers, as UTF-8. The value is a literal, not a
 * read of AppKit's NSString, because a C constant cannot be initialised from
 * one at load time; and the C name is not AppKit's spelling, because a
 * `const char *const` declared under that name is a redeclaration of the
 * SDK's NSString and no source including both headers would compile.
 * tests/test_toolbar.c pins each value against AppKit's own symbol, which is
 * what keeps the two in step. */
const char *const NS_TOOLBAR_TOGGLE_SIDEBAR_ITEM_IDENTIFIER =
    "NSToolbarToggleSidebarItem";
const char *const NS_TOOLBAR_SIDEBAR_TRACKING_SEPARATOR_ITEM_IDENTIFIER =
    "NSToolbarSidebarTrackingSeparatorItemIdentifier";
const char *const NS_TOOLBAR_FLEXIBLE_SPACE_ITEM_IDENTIFIER =
    "NSToolbarFlexibleSpaceItem";

/* Owned (+1): an initWith… returns a fresh object the caller ends with
 * ns_release (R7, KTD7) - or hands straight back to the toolbar's callbacks
 * struct, where the library releases it once AppKit has retained it (R9). */
ns_toolbar_item *ns_toolbar_item_create_with_item_identifier(
    const char *item_identifier) {
  NS_ENTER();
  return NS_OUT_OWNED(ns_toolbar_item,
                      [[NSToolbarItem alloc]
                          initWithItemIdentifier:NS_STRING_IN(
                                                     item_identifier)]);
  NS_LEAVE();
}

char *ns_toolbar_item_copy_item_identifier(ns_toolbar_item *toolbar_item) {
  NS_ENTER();
  return NS_STRING_OUT(NS_IN(NSToolbarItem, toolbar_item).itemIdentifier);
  NS_LEAVE();
}

void ns_toolbar_item_set_label(ns_toolbar_item *toolbar_item,
                               const char *label) {
  NS_ENTER();
  NS_IN(NSToolbarItem, toolbar_item).label = NS_STRING_IN(label);
  NS_LEAVE();
}

char *ns_toolbar_item_copy_label(ns_toolbar_item *toolbar_item) {
  NS_ENTER();
  return NS_STRING_OUT(NS_IN(NSToolbarItem, toolbar_item).label);
  NS_LEAVE();
}

void ns_toolbar_item_set_palette_label(ns_toolbar_item *toolbar_item,
                                       const char *palette_label) {
  NS_ENTER();
  NS_IN(NSToolbarItem, toolbar_item).paletteLabel =
      NS_STRING_IN(palette_label);
  NS_LEAVE();
}

char *ns_toolbar_item_copy_palette_label(ns_toolbar_item *toolbar_item) {
  NS_ENTER();
  return NS_STRING_OUT(NS_IN(NSToolbarItem, toolbar_item).paletteLabel);
  NS_LEAVE();
}

void ns_toolbar_item_set_tool_tip(ns_toolbar_item *toolbar_item,
                                  const char *tool_tip) {
  NS_ENTER();
  NS_IN(NSToolbarItem, toolbar_item).toolTip = NS_STRING_IN_OPT(tool_tip);
  NS_LEAVE();
}

char *ns_toolbar_item_copy_tool_tip(ns_toolbar_item *toolbar_item) {
  NS_ENTER();
  return NS_STRING_OUT(NS_IN(NSToolbarItem, toolbar_item).toolTip);
  NS_LEAVE();
}

void ns_toolbar_item_set_image(ns_toolbar_item *toolbar_item, ns_image *image) {
  NS_ENTER();
  NS_IN(NSToolbarItem, toolbar_item).image = NS_IN_OPT(NSImage, image);
  NS_LEAVE();
}

/* Borrowed: `image` is a strong property (R7). */
ns_image *ns_toolbar_item_image(ns_toolbar_item *toolbar_item) {
  NS_ENTER();
  return NS_OUT(ns_image, NS_IN(NSToolbarItem, toolbar_item).image);
  NS_LEAVE();
}

void ns_toolbar_item_set_bordered(ns_toolbar_item *toolbar_item,
                                  bool bordered) {
  NS_ENTER();
  NS_IN(NSToolbarItem, toolbar_item).bordered = bordered;
  NS_LEAVE();
}

bool ns_toolbar_item_bordered(ns_toolbar_item *toolbar_item) {
  NS_ENTER();
  return NS_IN(NSToolbarItem, toolbar_item).bordered;
  NS_LEAVE();
}

/* NSToolbarItem holds its target weakly, like every other target slot, so the
 * trampoline is kept alive by the association syn_install_action makes (R9,
 * KTD8). */
void ns_toolbar_item_set_action(ns_toolbar_item *toolbar_item,
                                ns_action action, void *context) {
  NS_ENTER();
  NSToolbarItem *target = NS_IN(NSToolbarItem, toolbar_item);
  syn_install_action(target, action, context, ^(id trampoline, SEL selector) {
    target.target = trampoline;
    target.action = selector;
  });
  NS_LEAVE();
}

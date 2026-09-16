/*
 * NSMenuItem - one row of a menu: a title, a key equivalent and its modifiers,
 * an enabled flag, a submenu, and an action (R18).
 *
 * An item's action is either AppKit's own - the standard items the menu bar
 * builder makes carry a responder-chain selector with no target, which is what
 * makes Command-C copy with no C code behind it (AE5) - or the caller's,
 * installed with ns_menu_item_set_action. A SEL never crosses the boundary
 * (R11), so an item created here starts with no action and the installer is
 * the only way to give it one.
 *
 * Enabling: a menu autoenables its items by default, and while it does,
 * ns_menu_item_set_enabled is ignored and AppKit computes the flag from
 * whether anything in the responder chain answers the item's action. Call
 * ns_menu_set_autoenables_items with false on the owning menu first if you
 * want to drive the flag yourself.
 */

#ifndef SYNTONIC_NS_MENU_ITEM_H
#define SYNTONIC_NS_MENU_ITEM_H

#include <os/availability.h>
#include <stdbool.h>
#include <stdint.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_menu_item ns_menu_item;

/* Declared by ns_menu.h and repeated here so this header stands alone. C has
 * allowed a repeated typedef of the same type since C11. */
typedef struct ns_menu ns_menu;

/* NSEventModifierFlags, the modifier keys a key equivalent is held with.
 * NSEvent.h is not wrapped in v0 and this is the only place its flags are
 * needed, so the enum is declared here, where it is used. A new item's mask is
 * NS_EVENT_MODIFIER_FLAG_COMMAND, which is AppKit's own default. */
typedef enum ns_event_modifier_flags : uint64_t {
  NS_EVENT_MODIFIER_FLAG_CAPS_LOCK = 1UL << 16,
  NS_EVENT_MODIFIER_FLAG_SHIFT = 1UL << 17,
  NS_EVENT_MODIFIER_FLAG_CONTROL = 1UL << 18,
  NS_EVENT_MODIFIER_FLAG_OPTION = 1UL << 19,
  NS_EVENT_MODIFIER_FLAG_COMMAND = 1UL << 20,
  NS_EVENT_MODIFIER_FLAG_NUMERIC_PAD = 1UL << 21,
  NS_EVENT_MODIFIER_FLAG_HELP = 1UL << 22,
  NS_EVENT_MODIFIER_FLAG_FUNCTION = 1UL << 23,
} ns_event_modifier_flags;

/* -[NSMenuItem initWithTitle:action:keyEquivalent:] - owned (+1), released
 * with ns_release (R7). AppKit's `action` segment is a SEL, which never
 * crosses the boundary (R11), so the item is created with no action and
 * ns_menu_item_set_action is what gives it one. Pass "" for no key
 * equivalent; the modifier mask starts as Command. */
ns_menu_item *_Nonnull ns_menu_item_create_with_title_key_equivalent(
    const char *_Nonnull title, const char *_Nonnull key_equivalent)
    API_AVAILABLE(macos(26.0));

/* +[NSMenuItem separatorItem] - owned (+1), released with ns_release (R7).
 * Each call returns a separate item, so one separator goes in one menu. */
ns_menu_item *_Nonnull ns_menu_item_create_separator_item(void)
    API_AVAILABLE(macos(26.0));

/* -[NSMenuItem setTitle:] */
void ns_menu_item_set_title(ns_menu_item *_Nonnull item,
                            const char *_Nonnull title)
    API_AVAILABLE(macos(26.0));

/* -[NSMenuItem title] - `title` is a copy property, so this is an owned copy
 * the caller frees with ns_string_free (R7, R11). */
char *_Nullable ns_menu_item_copy_title(ns_menu_item *_Nonnull item)
    API_AVAILABLE(macos(26.0));

/* -[NSMenuItem setKeyEquivalent:] - the unmodified character, "" for none.
 * The modifiers are separate; see ns_menu_item_set_key_equivalent_modifier_mask. */
void ns_menu_item_set_key_equivalent(ns_menu_item *_Nonnull item,
                                     const char *_Nonnull key_equivalent)
    API_AVAILABLE(macos(26.0));

/* -[NSMenuItem keyEquivalent] - a copy property, so this is an owned copy the
 * caller frees with ns_string_free (R7, R11). */
char *_Nullable ns_menu_item_copy_key_equivalent(ns_menu_item *_Nonnull item)
    API_AVAILABLE(macos(26.0));

/* -[NSMenuItem setKeyEquivalentModifierMask:] - combine the flags with the
 * bitwise or. */
void ns_menu_item_set_key_equivalent_modifier_mask(
    ns_menu_item *_Nonnull item, ns_event_modifier_flags modifiers)
    API_AVAILABLE(macos(26.0));

/* -[NSMenuItem keyEquivalentModifierMask] */
ns_event_modifier_flags ns_menu_item_key_equivalent_modifier_mask(
    ns_menu_item *_Nonnull item) API_AVAILABLE(macos(26.0));

/* -[NSMenuItem setEnabled:] - ignored while the owning menu autoenables its
 * items, which it does by default; see the header comment. */
void ns_menu_item_set_enabled(ns_menu_item *_Nonnull item, bool enabled)
    API_AVAILABLE(macos(26.0));

/* -[NSMenuItem isEnabled] - a BOOL getter drops AppKit's `is` (R5). */
bool ns_menu_item_enabled(ns_menu_item *_Nonnull item)
    API_AVAILABLE(macos(26.0));

/* -[NSMenuItem isSeparatorItem] */
bool ns_menu_item_separator_item(ns_menu_item *_Nonnull item)
    API_AVAILABLE(macos(26.0));

/* -[NSMenuItem setSubmenu:] - the item retains the menu, so releasing your own
 * reference afterwards is correct (R7). */
void ns_menu_item_set_submenu(ns_menu_item *_Nonnull item,
                              ns_menu *_Nullable submenu)
    API_AVAILABLE(macos(26.0));

/* -[NSMenuItem submenu] - a strong property, so this is borrowed: valid while
 * the item holds it, kept longer with ns_retain (R7). */
ns_menu *_Nullable ns_menu_item_submenu(ns_menu_item *_Nonnull item)
    API_AVAILABLE(macos(26.0));

/* Installs the target/action callback: `action` runs on the main thread with
 * `context` and this item as the sender. A null `action` uninstalls, which
 * leaves the item with no action at all rather than restoring a previous one.
 * `context` is neither copied nor freed and has to outlive the installation.
 * syntonic-owned. */
void ns_menu_item_set_action(ns_menu_item *_Nonnull item,
                             ns_action _Nullable action, void *_Nullable context)
    API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_MENU_ITEM_H */

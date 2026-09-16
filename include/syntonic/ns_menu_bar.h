/*
 * The standard menu bar, in one call (R18, R19, KTD16, KTD18).
 *
 * This is a non-mirror header: a menu bar has no AppKit class of its own, it
 * is a convention about what an NSMenu holds, so the builder has no selector
 * to name and nothing else belongs in here.
 *
 * WHAT THE BUILDER MAKES
 *
 * Six menus, in this order, with these items. Every item marked "chain" has a
 * nil target, which is what sends its action down the responder chain: no C
 * code sits behind Command-C (AE5), and none can, because a SEL never crosses
 * the boundary. Every key equivalent is held with Command unless another
 * modifier is named.
 *
 *   <app name>  About <app name>          chain: orderFrontStandardAboutPanel:
 *               ---
 *               Settings…            ,    your ns_action and context (KTD16)
 *               ---
 *               Services  >               AppKit fills the submenu
 *               ---
 *               Hide <app name>      h    chain: hide:
 *               Hide Others   Option-h    chain: hideOtherApplications:
 *               Show All                  chain: unhideAllApplications:
 *               ---
 *               Quit <app name>      q    chain: terminate:
 *   File        Close                w    chain: performClose:
 *   Edit        Undo                 z    chain: undo:
 *               Redo          Shift-z     chain: redo:
 *               ---
 *               Cut                  x    chain: cut:
 *               Copy                 c    chain: copy:
 *               Paste                v    chain: paste:
 *               Delete                    chain: delete:
 *               ---
 *               Select All           a    chain: selectAll:
 *   View        Toggle Sidebar Control-s  chain: toggleSidebar:
 *               ---
 *               Enter Full Screen
 *                            Control-f    chain: toggleFullScreen:
 *   Window      Minimize             m    chain: performMiniaturize:
 *               Zoom                      chain: performZoom:
 *               ---
 *               Bring All to Front        chain: arrangeInFront:
 *   Help        <app name> Help      ?    chain: showHelp:
 *
 * AppKit adds items of its own to the Window and Help menus, and to any menu
 * titled "Edit" - AutoFill, Start Dictation, Emoji & Symbols. Those are the
 * system's, they appear in every Mac app, and nothing here removes them. It
 * also takes the Full Screen item's shortcut back once the app has finished
 * launching, replacing Control-f with the system's own Globe-f.
 *
 * "Settings…" is the one standard item with a C callback behind it, because
 * AppKit defines no responder-chain action for Settings. The automatic
 * "Preferences…" rename is private and has changed between releases, so the
 * title and the Command-comma shortcut are set here in code (KTD16, R19).
 *
 * WHAT IT DOES NOT MAKE
 *
 * An app's own commands - a New Item, a Find - are app-defined items with app
 * callbacks (R18), not part of any standard menu bar. Add them through the
 * returned menu: ns_menu_item_at_index reaches the File or Edit menu's holder
 * item, ns_menu_item_submenu reaches the menu behind it, and
 * ns_menu_insert_item_at_index puts your item where you want it.
 */

#ifndef SYNTONIC_NS_MENU_BAR_H
#define SYNTONIC_NS_MENU_BAR_H

#include <os/availability.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Declared by ns_menu.h and ns_application.h and repeated here so this header
 * stands alone. C has allowed a repeated typedef of the same type since C11. */
typedef struct ns_menu ns_menu;
typedef struct ns_application ns_application;

/*
 * Builds the menu bar above and installs it as the application's main menu,
 * with the Window, Help and Services menus registered so AppKit fills them.
 * Call it once, before or from did_finish_launching.
 *
 * `app_name` is the name the App, Hide, Quit and Help items carry. It is
 * borrowed for the call.
 *
 * `settings_action` runs on the main thread with `settings_context` and the
 * Settings item as the sender; `settings_context` is neither copied nor freed
 * and has to stay valid for as long as the menu bar does. A null
 * `settings_action` leaves the Settings item without an action, which AppKit
 * draws disabled.
 *
 * Returns the menu bar, borrowed: the application holds it (R7). Installing a
 * second menu bar replaces the first, and the first goes with it.
 * syntonic-owned.
 */
ns_menu *_Nonnull ns_menu_bar_install_standard(
    ns_application *_Nonnull application, const char *_Nonnull app_name,
    ns_action _Nullable settings_action, void *_Nullable settings_context)
    API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_MENU_BAR_H */

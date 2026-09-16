/*
 * The standard menu bar (R18, R19, KTD16).
 *
 * The one place in Syntonic that names AppKit selectors on the caller's
 * behalf. Every standard item gets its selector and no target, which is what
 * sends it down the responder chain; a SEL never crosses the boundary (R11),
 * so a C caller could not write these items even if it wanted to. The one
 * exception is "Settings…", which AppKit has no responder-chain action for and
 * which therefore carries the caller's ns_action (KTD16).
 *
 * include/syntonic/ns_menu_bar.h has the menu bar this file builds, item for
 * item. Change one and change the other.
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syn_shims.h"
#include "syntonic/ns_menu_bar.h"

/* An item with a responder-chain selector and no target. `key` is "" for no
 * key equivalent; the modifier mask AppKit starts an item with is Command. */
static NSMenuItem *syn_menu_bar_item(NSString *title, SEL action,
                                     NSString *key) {
  return [[NSMenuItem alloc] initWithTitle:title action:action
                             keyEquivalent:key];
}

/* The same, with a modifier mask beside Command. */
static NSMenuItem *syn_menu_bar_item_with_modifiers(
    NSString *title, SEL action, NSString *key,
    NSEventModifierFlags modifiers) {
  NSMenuItem *item = syn_menu_bar_item(title, action, key);
  item.keyEquivalentModifierMask = modifiers;
  return item;
}

/* A top-level menu is an item with no action holding a submenu; the submenu's
 * title is what shows in the menu bar. */
static NSMenuItem *syn_menu_bar_holder(NSMenu *menu) {
  NSMenuItem *holder = [[NSMenuItem alloc] initWithTitle:menu.title
                                                  action:NULL
                                           keyEquivalent:@""];
  holder.submenu = menu;
  return holder;
}

static NSMenu *syn_menu_bar_app_menu(NSApplication *application, NSString *name,
                                     ns_action settings_action,
                                     void *settings_context) {
  NSMenu *menu = [[NSMenu alloc] initWithTitle:name];
  [menu addItem:syn_menu_bar_item(
                    [@"About " stringByAppendingString:name],
                    @selector(orderFrontStandardAboutPanel:), @"")];
  [menu addItem:NSMenuItem.separatorItem];

  /* The one standard item with C behind it: AppKit defines no responder-chain
   * action for Settings, and the automatic "Preferences…" rename is private
   * and has changed between releases (KTD16, R19). */
  NSMenuItem *settings = syn_menu_bar_item(@"Settings…", NULL, @",");
  syn_install_action(settings, settings_action, settings_context,
                     ^(id trampoline, SEL selector) {
                       settings.target = trampoline;
                       settings.action = selector;
                     });
  [menu addItem:settings];
  [menu addItem:NSMenuItem.separatorItem];

  NSMenuItem *services = syn_menu_bar_item(@"Services", NULL, @"");
  NSMenu *services_menu = [[NSMenu alloc] initWithTitle:@"Services"];
  services.submenu = services_menu;
  application.servicesMenu = services_menu;
  [menu addItem:services];
  [menu addItem:NSMenuItem.separatorItem];

  [menu addItem:syn_menu_bar_item([@"Hide " stringByAppendingString:name],
                                  @selector(hide:), @"h")];
  [menu addItem:syn_menu_bar_item_with_modifiers(
                    @"Hide Others", @selector(hideOtherApplications:), @"h",
                    NSEventModifierFlagCommand | NSEventModifierFlagOption)];
  [menu addItem:syn_menu_bar_item(@"Show All",
                                  @selector(unhideAllApplications:), @"")];
  [menu addItem:NSMenuItem.separatorItem];
  [menu addItem:syn_menu_bar_item([@"Quit " stringByAppendingString:name],
                                  @selector(terminate:), @"q")];
  return menu;
}

static NSMenu *syn_menu_bar_file_menu(void) {
  NSMenu *menu = [[NSMenu alloc] initWithTitle:@"File"];
  [menu addItem:syn_menu_bar_item(@"Close", @selector(performClose:), @"w")];
  return menu;
}

/* Every item here is a responder-chain action with no app code behind it,
 * which is AE5. AppKit adds AutoFill, Start Dictation and Emoji & Symbols to
 * any menu titled "Edit"; those are the system's and stay. */
static NSMenu *syn_menu_bar_edit_menu(void) {
  NSMenu *menu = [[NSMenu alloc] initWithTitle:@"Edit"];
  /* undo: and redo: are the informal NSUndoManager pair and no header declares
   * them, so they are named rather than written as a selector literal. */
  [menu addItem:syn_menu_bar_item(@"Undo", NSSelectorFromString(@"undo:"),
                                  @"z")];
  [menu addItem:syn_menu_bar_item_with_modifiers(
                    @"Redo", NSSelectorFromString(@"redo:"), @"z",
                    NSEventModifierFlagCommand | NSEventModifierFlagShift)];
  [menu addItem:NSMenuItem.separatorItem];
  [menu addItem:syn_menu_bar_item(@"Cut", @selector(cut:), @"x")];
  [menu addItem:syn_menu_bar_item(@"Copy", @selector(copy:), @"c")];
  [menu addItem:syn_menu_bar_item(@"Paste", @selector(paste:), @"v")];
  [menu addItem:syn_menu_bar_item(@"Delete", @selector(delete:), @"")];
  [menu addItem:NSMenuItem.separatorItem];
  [menu addItem:syn_menu_bar_item(@"Select All", @selector(selectAll:), @"a")];
  return menu;
}

static NSMenu *syn_menu_bar_view_menu(void) {
  NSMenu *menu = [[NSMenu alloc] initWithTitle:@"View"];
  /* The split view controller that answers this lands in U9; until then the
   * chain simply has no responder for it and AppKit draws the item disabled. */
  [menu addItem:syn_menu_bar_item_with_modifiers(
                    @"Toggle Sidebar", @selector(toggleSidebar:), @"s",
                    NSEventModifierFlagCommand | NSEventModifierFlagControl)];
  [menu addItem:NSMenuItem.separatorItem];
  [menu addItem:syn_menu_bar_item_with_modifiers(
                    @"Enter Full Screen", @selector(toggleFullScreen:), @"f",
                    NSEventModifierFlagCommand | NSEventModifierFlagControl)];
  return menu;
}

static NSMenu *syn_menu_bar_window_menu(void) {
  NSMenu *menu = [[NSMenu alloc] initWithTitle:@"Window"];
  [menu addItem:syn_menu_bar_item(@"Minimize", @selector(performMiniaturize:),
                                  @"m")];
  [menu addItem:syn_menu_bar_item(@"Zoom", @selector(performZoom:), @"")];
  [menu addItem:NSMenuItem.separatorItem];
  [menu addItem:syn_menu_bar_item(@"Bring All to Front",
                                  @selector(arrangeInFront:), @"")];
  return menu;
}

static NSMenu *syn_menu_bar_help_menu(NSString *name) {
  NSMenu *menu = [[NSMenu alloc] initWithTitle:@"Help"];
  [menu addItem:syn_menu_bar_item([name stringByAppendingString:@" Help"],
                                  @selector(showHelp:), @"?")];
  return menu;
}

/* Borrowed: the application holds its main menu strongly (R7). */
ns_menu *ns_menu_bar_install_standard(ns_application *application,
                                      const char *app_name,
                                      ns_action settings_action,
                                      void *settings_context) {
  NS_ENTER();
  NSApplication *target = NS_IN(NSApplication, application);
  NSString *name = NS_STRING_IN(app_name);

  NSMenu *bar = [[NSMenu alloc] initWithTitle:@""];
  [bar addItem:syn_menu_bar_holder(syn_menu_bar_app_menu(
                   target, name, settings_action, settings_context))];
  [bar addItem:syn_menu_bar_holder(syn_menu_bar_file_menu())];
  [bar addItem:syn_menu_bar_holder(syn_menu_bar_edit_menu())];
  [bar addItem:syn_menu_bar_holder(syn_menu_bar_view_menu())];

  NSMenu *window_menu = syn_menu_bar_window_menu();
  [bar addItem:syn_menu_bar_holder(window_menu)];

  NSMenu *help_menu = syn_menu_bar_help_menu(name);
  [bar addItem:syn_menu_bar_holder(help_menu)];

  target.mainMenu = bar;
  /* Registering these is what lets AppKit add the window list and the help
   * search field, which every Mac app has. */
  target.windowsMenu = window_menu;
  target.helpMenu = help_menu;
  return NS_OUT(ns_menu, bar);
  NS_LEAVE();
}

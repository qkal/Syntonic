/*
 * NSButton: the push button and the checkbox (R17, KTD7).
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syntonic/ns_button.h"

/* Owned (+1): a +buttonWith… class method returns a fresh object like any
 * other constructor, so the caller ends it with ns_release (R7, KTD7).
 * AppKit's `target:` and `action:` segments never cross (R11), so the button
 * is built with none and ns_control_set_action installs one. */
ns_button *ns_button_create_with_title(const char *title) {
  NS_ENTER();
  return NS_OUT_OWNED(ns_button, [NSButton buttonWithTitle:NS_STRING_IN(title)
                                                    target:nil
                                                    action:NULL]);
  NS_LEAVE();
}

/* Owned (+1), and the same two dropped segments as above (R7, R11). */
ns_button *ns_button_create_checkbox_with_title(const char *title) {
  NS_ENTER();
  return NS_OUT_OWNED(ns_button,
                      [NSButton checkboxWithTitle:NS_STRING_IN(title)
                                           target:nil
                                           action:NULL]);
  NS_LEAVE();
}

void ns_button_set_title(ns_button *button, const char *title) {
  NS_ENTER();
  NS_IN(NSButton, button).title = NS_STRING_IN(title);
  NS_LEAVE();
}

/* `title` is a copy property, so the value is owned and the name says copy_
 * (R7, R11). */
char *ns_button_copy_title(ns_button *button) {
  NS_ENTER();
  return NS_STRING_OUT(NS_IN(NSButton, button).title);
  NS_LEAVE();
}

void ns_button_set_state(ns_button *button, ns_control_state_value state) {
  NS_ENTER();
  NS_IN(NSButton, button).state = (NSControlStateValue)state;
  NS_LEAVE();
}

ns_control_state_value ns_button_state(ns_button *button) {
  NS_ENTER();
  return (ns_control_state_value)NS_IN(NSButton, button).state;
  NS_LEAVE();
}

void ns_button_set_bezel_style(ns_button *button, ns_bezel_style style) {
  NS_ENTER();
  NS_IN(NSButton, button).bezelStyle = (NSBezelStyle)style;
  NS_LEAVE();
}

ns_bezel_style ns_button_bezel_style(ns_button *button) {
  NS_ENTER();
  return (ns_bezel_style)NS_IN(NSButton, button).bezelStyle;
  NS_LEAVE();
}

/* An upcast is the identity, with the debug class check NS_IN carries (KTD9). */
ns_control *ns_button_as_control(ns_button *button) {
  NS_ENTER();
  return NS_OUT(ns_control, NS_IN(NSButton, button));
  NS_LEAVE();
}

ns_view *ns_button_as_view(ns_button *button) {
  NS_ENTER();
  return NS_OUT(ns_view, NS_IN(NSButton, button));
  NS_LEAVE();
}

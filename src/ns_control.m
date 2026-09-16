/*
 * NSControl: the enabled flag, the string value, and the target/action every
 * control inherits (R4, R9, R17).
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syn_shims.h"
#include "syntonic/ns_control.h"

void ns_control_set_enabled(ns_control *control, bool enabled) {
  NS_ENTER();
  NS_IN(NSControl, control).enabled = enabled;
  NS_LEAVE();
}

bool ns_control_enabled(ns_control *control) {
  NS_ENTER();
  return NS_IN(NSControl, control).enabled;
  NS_LEAVE();
}

void ns_control_set_string_value(ns_control *control, const char *value) {
  NS_ENTER();
  NS_IN(NSControl, control).stringValue = NS_STRING_IN(value);
  NS_LEAVE();
}

/* `stringValue` is a copy property, so the value is owned and the name says
 * copy_ (R7, R11). */
char *ns_control_copy_string_value(ns_control *control) {
  NS_ENTER();
  return NS_STRING_OUT(NS_IN(NSControl, control).stringValue);
  NS_LEAVE();
}

void ns_control_perform_click(ns_control *control) {
  NS_ENTER();
  [NS_IN(NSControl, control) performClick:nil];
  NS_LEAVE();
}

/* Both of AppKit's segments are types that never cross, so the wrapper sends
 * the control's own action to its own target - the same message a click sends
 * - and the name drops them (R11). */
bool ns_control_send_action(ns_control *control) {
  NS_ENTER();
  NSControl *target = NS_IN(NSControl, control);
  return [target sendAction:target.action to:target.target];
  NS_LEAVE();
}

void ns_control_set_font(ns_control *control, ns_font *font) {
  NS_ENTER();
  NS_IN(NSControl, control).font = NS_IN_OPT(NSFont, font);
  NS_LEAVE();
}

/* `font` is a copy property, so the value is owned and the name says copy_
 * (R7, KTD7). */
ns_font *ns_control_copy_font(ns_control *control) {
  NS_ENTER();
  return NS_OUT_OWNED(ns_font, NS_IN(NSControl, control).font);
  NS_LEAVE();
}

void ns_control_set_line_break_mode(ns_control *control,
                                    ns_line_break_mode line_break_mode) {
  NS_ENTER();
  NS_IN(NSControl, control).lineBreakMode = (NSLineBreakMode)line_break_mode;
  NS_LEAVE();
}

ns_line_break_mode ns_control_line_break_mode(ns_control *control) {
  NS_ENTER();
  return (ns_line_break_mode)NS_IN(NSControl, control).lineBreakMode;
  NS_LEAVE();
}

/* The trampoline is installed, replaced and uninstalled by one call; the block
 * is where this class's target and action slots live (R9). */
void ns_control_set_action(ns_control *control, ns_action action,
                           void *context) {
  NS_ENTER();
  NSControl *target = NS_IN(NSControl, control);
  syn_install_action(target, action, context, ^(id trampoline, SEL selector) {
    target.target = trampoline;
    target.action = selector;
  });
  NS_LEAVE();
}

/* An upcast is the identity, with the debug class check NS_IN carries (KTD9). */
ns_view *ns_control_as_view(ns_control *control) {
  NS_ENTER();
  return NS_OUT(ns_view, NS_IN(NSControl, control));
  NS_LEAVE();
}

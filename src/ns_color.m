/*
 * NSColor: the three semantic colours the twins name (R7, R17).
 *
 * Each one is a `strong` class property, which is the shape NS_OUT is right
 * for: AppKit holds the shared colour for the life of the process, so the
 * borrowed return cannot dangle (R7).
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syntonic/ns_color.h"

ns_color *ns_color_label_color(void) {
  NS_ENTER();
  return NS_OUT(ns_color, NSColor.labelColor);
  NS_LEAVE();
}

ns_color *ns_color_secondary_label_color(void) {
  NS_ENTER();
  return NS_OUT(ns_color, NSColor.secondaryLabelColor);
  NS_LEAVE();
}

ns_color *ns_color_control_accent_color(void) {
  NS_ENTER();
  return NS_OUT(ns_color, NSColor.controlAccentColor);
  NS_LEAVE();
}

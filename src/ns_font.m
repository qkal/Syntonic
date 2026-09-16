/*
 * NSFont: the system font, its bold companion, and the two readers a caller
 * needs to tell one from the other (R7, R17).
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syntonic/ns_font.h"

/* A plain class method returning an object, so the return is owned and the
 * name says copy_ (R7, KTD7). */
ns_font *ns_font_copy_system_font_of_size(CGFloat size) {
  NS_ENTER();
  return NS_OUT_OWNED(ns_font, [NSFont systemFontOfSize:size]);
  NS_LEAVE();
}

ns_font *ns_font_copy_bold_system_font_of_size(CGFloat size) {
  NS_ENTER();
  return NS_OUT_OWNED(ns_font, [NSFont boldSystemFontOfSize:size]);
  NS_LEAVE();
}

CGFloat ns_font_system_font_size(void) {
  NS_ENTER();
  return NSFont.systemFontSize;
  NS_LEAVE();
}

/* `fontName` is a copy property, so the value is owned (R7, R11). */
char *ns_font_copy_font_name(ns_font *font) {
  NS_ENTER();
  return NS_STRING_OUT(NS_IN(NSFont, font).fontName);
  NS_LEAVE();
}

CGFloat ns_font_point_size(ns_font *font) {
  NS_ENTER();
  return NS_IN(NSFont, font).pointSize;
  NS_LEAVE();
}

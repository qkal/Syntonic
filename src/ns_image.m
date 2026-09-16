/*
 * NSImage: the SF Symbol constructor and the description that goes with it
 * (R19, R23, KTD7).
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syntonic/ns_image.h"

/* Owned (+1): a +imageWith… class method returns a fresh object like any other
 * constructor (R7, KTD7). AppKit answers nil for a symbol the running system
 * does not have, and that nil crosses unchanged - it is an answer, not misuse,
 * so there is no check here. */
ns_image *ns_image_create_with_system_symbol_name_accessibility_description(
    const char *name, const char *accessibility_description) {
  NS_ENTER();
  return NS_OUT_OWNED(
      ns_image,
      [NSImage imageWithSystemSymbolName:NS_STRING_IN(name)
                accessibilityDescription:NS_STRING_IN_OPT(
                                             accessibility_description)]);
  NS_LEAVE();
}

/* `accessibilityDescription` is a copy property, so the value is owned and the
 * name says copy_ (R7, R11). */
char *ns_image_copy_accessibility_description(ns_image *image) {
  NS_ENTER();
  return NS_STRING_OUT(NS_IN(NSImage, image).accessibilityDescription);
  NS_LEAVE();
}

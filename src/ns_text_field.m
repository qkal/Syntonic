/*
 * NSTextField, its two constructors and its delegate (R17, F6, R9, KTD8).
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syn_shims.h"
#include "syntonic/ns_text_field.h"

/* NSTextFieldDelegate is entirely @optional in the SDK and Syntonic requires
 * nothing of it either; docs/conventions.md's per-protocol table is the source
 * (R9, KTD8). Both selectors come from NSControlTextEditingDelegate, which
 * NSTextFieldDelegate inherits. */
SYN_SHIM_TABLE(syn_text_field_table, ns_text_field_callbacks,
               "NSTextFieldDelegate",
               SYN_SHIM_OPTIONAL(ns_text_field_callbacks, did_change,
                                 "controlTextDidChange:"),
               SYN_SHIM_OPTIONAL(ns_text_field_callbacks, did_end_editing,
                                 "controlTextDidEndEditing:"));

@interface SynTextFieldShim : SynShim <NSTextFieldDelegate>
@end

@implementation SynTextFieldShim

- (void)controlTextDidChange:(NSNotification *)notification {
  SYN_SHIM_ENTER(NSTextField, notification.object);
  void (*callback)(void *, ns_text_field *) =
      SYN_SHIM_FN(ns_text_field_callbacks, did_change);
  if (callback != NULL)
    callback(syn_context, NS_OUT(ns_text_field, syn_sender));
  SYN_SHIM_LEAVE();
}

- (void)controlTextDidEndEditing:(NSNotification *)notification {
  SYN_SHIM_ENTER(NSTextField, notification.object);
  void (*callback)(void *, ns_text_field *) =
      SYN_SHIM_FN(ns_text_field_callbacks, did_end_editing);
  if (callback != NULL)
    callback(syn_context, NS_OUT(ns_text_field, syn_sender));
  SYN_SHIM_LEAVE();
}

@end

/* Owned (+1): a +labelWith… class method returns a fresh object, so the caller
 * ends it with ns_release (R7, KTD7). */
ns_text_field *ns_text_field_create_label_with_string(const char *value) {
  NS_ENTER();
  return NS_OUT_OWNED(ns_text_field,
                      [NSTextField labelWithString:NS_STRING_IN(value)]);
  NS_LEAVE();
}

/* Owned (+1), like the label above (R7, KTD7). */
ns_text_field *ns_text_field_create_with_string(const char *value) {
  NS_ENTER();
  return NS_OUT_OWNED(ns_text_field,
                      [NSTextField textFieldWithString:NS_STRING_IN(value)]);
  NS_LEAVE();
}

void ns_text_field_set_placeholder_string(ns_text_field *text_field,
                                          const char *placeholder) {
  NS_ENTER();
  NS_IN(NSTextField, text_field).placeholderString =
      NS_STRING_IN_OPT(placeholder);
  NS_LEAVE();
}

/* `placeholderString` is a copy property, so the value is owned and the name
 * says copy_ (R7, R11). */
char *ns_text_field_copy_placeholder_string(ns_text_field *text_field) {
  NS_ENTER();
  return NS_STRING_OUT(NS_IN(NSTextField, text_field).placeholderString);
  NS_LEAVE();
}

void ns_text_field_set_callbacks(ns_text_field *text_field,
                                 const ns_text_field_callbacks *callbacks,
                                 void *context) {
  NS_ENTER();
  NSTextField *target = NS_IN(NSTextField, text_field);
  SYN_SHIM_INSTALL(target, SynTextFieldShim, syn_text_field_table, callbacks,
                   context, ^(id shim) { target.delegate = shim; });
  NS_LEAVE();
}

/* An upcast is the identity, with the debug class check NS_IN carries (KTD9). */
ns_control *ns_text_field_as_control(ns_text_field *text_field) {
  NS_ENTER();
  return NS_OUT(ns_control, NS_IN(NSTextField, text_field));
  NS_LEAVE();
}

ns_view *ns_text_field_as_view(ns_text_field *text_field) {
  NS_ENTER();
  return NS_OUT(ns_view, NS_IN(NSTextField, text_field));
  NS_LEAVE();
}

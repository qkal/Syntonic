/*
 * The shape of a Syntonic wrapper implementation (R7, R10, R12, KTD4, KTD7).
 *
 * Copy this file to src/ns_<class>.m next to the header you copied from
 * ns_template.h. CMake globs the .m files directly under src/, so there is no
 * build file to edit; the glob does not descend into src/templates/, so this
 * file is not part of the library.
 *
 * Every body here is two lines of scaffolding - NS_ENTER and NS_LEAVE - around
 * one line of AppKit. Which outbound macro a return uses is the ownership rule
 * (R7), not a preference; see src/ns_internal.h and docs/conventions.md.
 *
 * Nullability annotations belong to the public header only. Do not write
 * _Nullable or _Nonnull in this file: -Wnullability-completeness is file-scoped,
 * so one annotation here obliges every other pointer in the file to carry one.
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "ns_template.h" /* a real wrapper: #include "syntonic/ns_button.h" */

/* The template has to compile, so its placeholder class is spelled as a real
 * one. A real wrapper deletes these two lines and writes NSButton throughout. */
@compatibility_alias NSTemplate NSButton;
@compatibility_alias NSTemplateSuper NSView;

/* Owned (+1): the caller ends it with ns_release (R7, KTD7). */
ns_template *ns_template_create(CGRect frame) {
  NS_ENTER();
  return NS_OUT_OWNED(ns_template, [[NSTemplate alloc] initWithFrame:frame]);
  NS_LEAVE();
}

void ns_template_set_title(ns_template *receiver, const char *title) {
  NS_ENTER();
  NS_IN(NSTemplate, receiver).title = NS_STRING_IN(title);
  NS_LEAVE();
}

/* `title` is a copy property, so the value is owned and the name says copy_.
 * NS_STRING_OUT is a strdup the caller frees with ns_string_free (R11). */
char *ns_template_copy_title(ns_template *receiver) {
  NS_ENTER();
  return NS_STRING_OUT(NS_IN(NSTemplate, receiver).title);
  NS_LEAVE();
}

void ns_template_set_style(ns_template *receiver, ns_template_style style) {
  NS_ENTER();
  NS_IN(NSTemplate, receiver).bezelStyle = (NSBezelStyle)style;
  NS_LEAVE();
}

ns_template_style ns_template_get_style(ns_template *receiver) {
  NS_ENTER();
  return (ns_template_style)NS_IN(NSTemplate, receiver).bezelStyle;
  NS_LEAVE();
}

/* Borrowed: NS_OUT is only ever right for a strong, weak or assign property
 * getter, and `superview` is weak (R7). */
ns_template_super *ns_template_superview(ns_template *receiver) {
  NS_ENTER();
  return NS_OUT(ns_template_super, NS_IN(NSTemplate, receiver).superview);
  NS_LEAVE();
}

/* `subviews` is a copy property, so it never crosses as a handle: it becomes a
 * count plus an index accessor whose element is borrowed from the receiver
 * (R7, KTD17). */
long ns_template_subview_count(ns_template *receiver) {
  NS_ENTER();
  return (long)NS_IN(NSTemplate, receiver).subviews.count;
  NS_LEAVE();
}

ns_template_super *ns_template_subview_at_index(ns_template *receiver,
                                                    long index) {
  NS_ENTER();
  /* No explicit range check: NSArray raises on an out-of-range index and the
   * debug entry macro reports that exception with this function's name (KTD4).
   * Add a check only where AppKit would accept the bad index silently. */
  NSArray<NSView *> *subviews = NS_IN(NSTemplate, receiver).subviews;
  return NS_OUT(ns_template_super, subviews[(NSUInteger)index]);
  NS_LEAVE();
}

/* An upcast is the identity, with the debug class check NS_IN carries (KTD9). */
ns_template_super *ns_template_as_template_super(ns_template *receiver) {
  NS_ENTER();
  return NS_OUT(ns_template_super, NS_IN(NSTemplate, receiver));
  NS_LEAVE();
}

void ns_template_set_action(ns_template *receiver, ns_action action,
                            void *context) {
  NS_ENTER();
  /* U14 replaces the next two lines with the trampoline installer from
   * src/syn_shims.h, which does not exist yet:
   *   syn_install_action(NS_IN(NSTemplate, receiver), action, context); */
  (void)NS_IN(NSTemplate, receiver);
  (void)action, (void)context;
  NS_LEAVE();
}

void ns_template_set_callbacks(ns_template *receiver,
                               const ns_template_callbacks *callbacks,
                               void *context) {
  NS_ENTER();
  /* U14 replaces the next two lines with the shim installer from
   * src/syn_shims.h, which does not exist yet:
   *   syn_install_callbacks(NS_IN(NSTemplate, receiver), &syn_template_members,
   *                         callbacks, sizeof *callbacks, context); */
  (void)NS_IN(NSTemplate, receiver);
  (void)callbacks, (void)context;
  NS_LEAVE();
}

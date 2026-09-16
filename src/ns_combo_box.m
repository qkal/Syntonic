/*
 * NSComboBox: its two lists, its selection and one merged callbacks struct
 * (R9, R11, R12, KTD6, KTD8, KTD17).
 *
 * The struct merges NSComboBoxDataSource and NSComboBoxDelegate and embeds
 * NSTextFieldDelegate's struct first, because NSComboBoxDelegate inherits that
 * protocol - so the one shim below adopts both combo box protocols, answers
 * the text field's two editing selectors as well, and the one install assigns
 * both AppKit slots.
 *
 * The data source slot is `assign`: AppKit neither retains it nor zeroes it.
 * The shim is held by an associated reference on the combo box, so it outlives
 * every read AppKit can make, and SYN_SHIM_INSTALL assigns the slot before
 * releasing the shim that was in it - so an uninstall leaves nil there rather
 * than a dangling pointer.
 */

#import <AppKit/AppKit.h>

#include <stdio.h>
#include <stdlib.h>

#include "ns_internal.h"
#include "syn_shims.h"
#include "syntonic/ns_combo_box.h"

/* ---------------------------------------------------------------------------
 * The mode check (R12, KTD4)
 *
 * NSComboBox has two lists and shows one of them: the static one it holds
 * itself while `usesDataSource` is false, and the data source's while it is
 * true. AppKit takes a call meant for the other half anyway - it logs a line
 * to the console, then hands back a wrong value or raises out of an empty
 * array - so this is the "AppKit would accept the bad value silently" case
 * docs/conventions.md asks for an explicit check on. Nothing under NDEBUG.
 * ------------------------------------------------------------------------- */

#ifndef NDEBUG

static void syn_combo_box_check_mode(NSComboBox *combo_box,
                                     bool uses_data_source,
                                     const char *function) {
  if ((combo_box.usesDataSource != NO) == uses_data_source) return;
  fprintf(stderr,
          "syntonic: %s: this call belongs to the %s and the combo box has "
          "usesDataSource %s; the two lists are exclusive and AppKit answers "
          "this call from the one it is not showing (R12).\n",
          function, uses_data_source ? "data source" : "static list",
          uses_data_source ? "false" : "true");
  fflush(stderr);
  abort();
}

#define NS_CHECK_COMBO_BOX_MODE(combo_box, uses_data_source)                  \
  syn_combo_box_check_mode((combo_box), (uses_data_source), __func__)

#else /* NDEBUG */

#define NS_CHECK_COMBO_BOX_MODE(combo_box, uses_data_source) ((void)0)

#endif /* NDEBUG */

/* ---------------------------------------------------------------------------
 * The protocol (R9, KTD6, KTD8)
 * ------------------------------------------------------------------------- */

/* All three protocols are entirely @optional in the SDK; docs/conventions.md's
 * per-protocol table is what makes the first two required (R9, KTD8). The
 * protocol string names every half, because that is what a report prints, and
 * a member of the embedded parent struct is named through its path. */
SYN_SHIM_TABLE(
    syn_combo_box_table, ns_combo_box_callbacks,
    "NSComboBoxDataSource + NSComboBoxDelegate",
    SYN_SHIM_REQUIRED(ns_combo_box_callbacks, number_of_items,
                      "numberOfItemsInComboBox:"),
    SYN_SHIM_REQUIRED(ns_combo_box_callbacks, object_value_for_item_at_index,
                      "comboBox:objectValueForItemAtIndex:"),
    SYN_SHIM_OPTIONAL(ns_combo_box_callbacks, index_of_item_with_string_value,
                      "comboBox:indexOfItemWithStringValue:"),
    SYN_SHIM_OPTIONAL(ns_combo_box_callbacks, completed_string,
                      "comboBox:completedString:"),
    SYN_SHIM_OPTIONAL(ns_combo_box_callbacks, selection_did_change,
                      "comboBoxSelectionDidChange:"),
    SYN_SHIM_OPTIONAL(ns_combo_box_callbacks, selection_is_changing,
                      "comboBoxSelectionIsChanging:"),
    SYN_SHIM_OPTIONAL(ns_combo_box_callbacks, will_pop_up,
                      "comboBoxWillPopUp:"),
    SYN_SHIM_OPTIONAL(ns_combo_box_callbacks, will_dismiss,
                      "comboBoxWillDismiss:"),
    SYN_SHIM_OPTIONAL(ns_combo_box_callbacks, text_field.did_change,
                      "controlTextDidChange:"),
    SYN_SHIM_OPTIONAL(ns_combo_box_callbacks, text_field.did_end_editing,
                      "controlTextDidEndEditing:"));

@interface SynComboBoxShim : SynShim <NSComboBoxDataSource, NSComboBoxDelegate>
@end

@implementation SynComboBoxShim

- (NSInteger)numberOfItemsInComboBox:(NSComboBox *)comboBox {
  SYN_SHIM_ENTER(NSComboBox, comboBox);
  long (*callback)(void *, ns_combo_box *) =
      SYN_SHIM_FN(ns_combo_box_callbacks, number_of_items);
  if (callback == NULL) return 0;
  long count = callback(syn_context, NS_OUT(ns_combo_box, syn_sender));
  SYN_SHIM_CHECK_COUNT(syn_combo_box_table, number_of_items, count);
  return (NSInteger)count;
  SYN_SHIM_LEAVE();
}

/* An object value crosses as a UTF-8 string (R11), borrowed from the callback
 * and copied here before AppKit sees it. Null is an answer and not misuse -
 * the SDK declares this return nullable - so it carries no check. */
- (id)comboBox:(NSComboBox *)comboBox
    objectValueForItemAtIndex:(NSInteger)index {
  SYN_SHIM_ENTER(NSComboBox, comboBox);
  const char *(*callback)(void *, ns_combo_box *, long) =
      SYN_SHIM_FN(ns_combo_box_callbacks, object_value_for_item_at_index);
  if (callback == NULL) return nil;
  const char *value =
      callback(syn_context, NS_OUT(ns_combo_box, syn_sender), (long)index);
  return ns_internal_string_in(value);
  SYN_SHIM_LEAVE();
}

/* AppKit's "no such value" here is NSNotFound; the boundary's is -1, the way
 * every other index crosses (R11). */
- (NSUInteger)comboBox:(NSComboBox *)comboBox
    indexOfItemWithStringValue:(NSString *)string {
  SYN_SHIM_ENTER(NSComboBox, comboBox);
  long (*callback)(void *, ns_combo_box *, const char *) =
      SYN_SHIM_FN(ns_combo_box_callbacks, index_of_item_with_string_value);
  if (callback == NULL) return NSNotFound;
  long index = callback(syn_context, NS_OUT(ns_combo_box, syn_sender),
                        string.UTF8String);
  return index < 0 ? NSNotFound : (NSUInteger)index;
  SYN_SHIM_LEAVE();
}

/* Null is no completion, which is what the SDK's nullable return means. */
- (NSString *)comboBox:(NSComboBox *)comboBox
       completedString:(NSString *)string {
  SYN_SHIM_ENTER(NSComboBox, comboBox);
  const char *(*callback)(void *, ns_combo_box *, const char *) =
      SYN_SHIM_FN(ns_combo_box_callbacks, completed_string);
  if (callback == NULL) return nil;
  const char *completed = callback(
      syn_context, NS_OUT(ns_combo_box, syn_sender), string.UTF8String);
  return ns_internal_string_in(completed);
  SYN_SHIM_LEAVE();
}

/* A lone notification parameter becomes the sender handle (R5). */
- (void)comboBoxSelectionDidChange:(NSNotification *)notification {
  SYN_SHIM_ENTER(NSComboBox, notification.object);
  void (*callback)(void *, ns_combo_box *) =
      SYN_SHIM_FN(ns_combo_box_callbacks, selection_did_change);
  if (callback != NULL) callback(syn_context, NS_OUT(ns_combo_box, syn_sender));
  SYN_SHIM_LEAVE();
}

- (void)comboBoxSelectionIsChanging:(NSNotification *)notification {
  SYN_SHIM_ENTER(NSComboBox, notification.object);
  void (*callback)(void *, ns_combo_box *) =
      SYN_SHIM_FN(ns_combo_box_callbacks, selection_is_changing);
  if (callback != NULL) callback(syn_context, NS_OUT(ns_combo_box, syn_sender));
  SYN_SHIM_LEAVE();
}

- (void)comboBoxWillPopUp:(NSNotification *)notification {
  SYN_SHIM_ENTER(NSComboBox, notification.object);
  void (*callback)(void *, ns_combo_box *) =
      SYN_SHIM_FN(ns_combo_box_callbacks, will_pop_up);
  if (callback != NULL) callback(syn_context, NS_OUT(ns_combo_box, syn_sender));
  SYN_SHIM_LEAVE();
}

- (void)comboBoxWillDismiss:(NSNotification *)notification {
  SYN_SHIM_ENTER(NSComboBox, notification.object);
  void (*callback)(void *, ns_combo_box *) =
      SYN_SHIM_FN(ns_combo_box_callbacks, will_dismiss);
  if (callback != NULL) callback(syn_context, NS_OUT(ns_combo_box, syn_sender));
  SYN_SHIM_LEAVE();
}

/* The two members of the embedded parent struct. Their selectors come from
 * NSControlTextEditingDelegate, which NSTextFieldDelegate inherits and
 * NSComboBoxDelegate inherits in turn, and their sender is the combo box
 * behind an ns_text_field handle (R5). */
- (void)controlTextDidChange:(NSNotification *)notification {
  SYN_SHIM_ENTER(NSComboBox, notification.object);
  void (*callback)(void *, ns_text_field *) =
      SYN_SHIM_FN(ns_combo_box_callbacks, text_field.did_change);
  if (callback != NULL)
    callback(syn_context, NS_OUT(ns_text_field, syn_sender));
  SYN_SHIM_LEAVE();
}

- (void)controlTextDidEndEditing:(NSNotification *)notification {
  SYN_SHIM_ENTER(NSComboBox, notification.object);
  void (*callback)(void *, ns_text_field *) =
      SYN_SHIM_FN(ns_combo_box_callbacks, text_field.did_end_editing);
  if (callback != NULL)
    callback(syn_context, NS_OUT(ns_text_field, syn_sender));
  SYN_SHIM_LEAVE();
}

@end

/* ---------------------------------------------------------------------------
 * The class
 * ------------------------------------------------------------------------- */

/* Owned (+1): an initWith… returns a fresh object the caller ends with
 * ns_release (R7, KTD7). */
ns_combo_box *ns_combo_box_create_with_frame(CGRect frame) {
  NS_ENTER();
  return NS_OUT_OWNED(ns_combo_box, [[NSComboBox alloc] initWithFrame:frame]);
  NS_LEAVE();
}

void ns_combo_box_set_uses_data_source(ns_combo_box *combo_box,
                                       bool uses_data_source) {
  NS_ENTER();
  NS_IN(NSComboBox, combo_box).usesDataSource = uses_data_source;
  NS_LEAVE();
}

bool ns_combo_box_uses_data_source(ns_combo_box *combo_box) {
  NS_ENTER();
  return NS_IN(NSComboBox, combo_box).usesDataSource;
  NS_LEAVE();
}

void ns_combo_box_add_item_with_object_value(ns_combo_box *combo_box,
                                             const char *value) {
  NS_ENTER();
  NSComboBox *target = NS_IN(NSComboBox, combo_box);
  NS_CHECK_COMBO_BOX_MODE(target, false);
  [target addItemWithObjectValue:NS_STRING_IN(value)];
  NS_LEAVE();
}

/* An array in is a pointer plus a count, borrowed for the call: the values are
 * converted here and the pointer is never kept (R11, KTD17). */
void ns_combo_box_add_items_with_object_values(ns_combo_box *combo_box,
                                               const char *const *values,
                                               long count) {
  NS_ENTER();
  NSComboBox *target = NS_IN(NSComboBox, combo_box);
  NS_CHECK_COMBO_BOX_MODE(target, false);
  [target addItemsWithObjectValues:NS_STRING_ARRAY_IN(values, count, @"")];
  NS_LEAVE();
}

/* No index check of its own: AppKit raises before it changes anything - an
 * index equal to the count appends - and the entry macro reports that
 * exception with this function's name (KTD4). */
void ns_combo_box_insert_item_with_object_value_at_index(
    ns_combo_box *combo_box, const char *value, long index) {
  NS_ENTER();
  NSComboBox *target = NS_IN(NSComboBox, combo_box);
  NS_CHECK_COMBO_BOX_MODE(target, false);
  [target insertItemWithObjectValue:NS_STRING_IN(value)
                            atIndex:(NSInteger)index];
  NS_LEAVE();
}

void ns_combo_box_remove_item_with_object_value(ns_combo_box *combo_box,
                                                const char *value) {
  NS_ENTER();
  NSComboBox *target = NS_IN(NSComboBox, combo_box);
  NS_CHECK_COMBO_BOX_MODE(target, false);
  [target removeItemWithObjectValue:NS_STRING_IN(value)];
  NS_LEAVE();
}

void ns_combo_box_remove_item_at_index(ns_combo_box *combo_box, long index) {
  NS_ENTER();
  NSComboBox *target = NS_IN(NSComboBox, combo_box);
  NS_CHECK_COMBO_BOX_MODE(target, false);
  [target removeItemAtIndex:(NSInteger)index];
  NS_LEAVE();
}

void ns_combo_box_remove_all_items(ns_combo_box *combo_box) {
  NS_ENTER();
  NSComboBox *target = NS_IN(NSComboBox, combo_box);
  NS_CHECK_COMBO_BOX_MODE(target, false);
  [target removeAllItems];
  NS_LEAVE();
}

/* An object value crosses as a string, and a string Syntonic returns is always
 * an owned copy (R11). */
char *ns_combo_box_copy_item_object_value_at_index(ns_combo_box *combo_box,
                                                   long index) {
  NS_ENTER();
  NSComboBox *target = NS_IN(NSComboBox, combo_box);
  NS_CHECK_COMBO_BOX_MODE(target, false);
  return NS_STRING_OUT([target itemObjectValueAtIndex:(NSInteger)index]);
  NS_LEAVE();
}

/* AppKit's own answer for a value the list does not carry is NSNotFound; the
 * boundary's is -1 (R11). */
long ns_combo_box_index_of_item_with_object_value(ns_combo_box *combo_box,
                                                  const char *value) {
  NS_ENTER();
  NSComboBox *target = NS_IN(NSComboBox, combo_box);
  NS_CHECK_COMBO_BOX_MODE(target, false);
  NSInteger index = [target indexOfItemWithObjectValue:NS_STRING_IN(value)];
  return index == (NSInteger)NSNotFound ? -1 : (long)index;
  NS_LEAVE();
}

void ns_combo_box_reload_data(ns_combo_box *combo_box) {
  NS_ENTER();
  [NS_IN(NSComboBox, combo_box) reloadData];
  NS_LEAVE();
}

void ns_combo_box_note_number_of_items_changed(ns_combo_box *combo_box) {
  NS_ENTER();
  [NS_IN(NSComboBox, combo_box) noteNumberOfItemsChanged];
  NS_LEAVE();
}

long ns_combo_box_number_of_items(ns_combo_box *combo_box) {
  NS_ENTER();
  return (long)NS_IN(NSComboBox, combo_box).numberOfItems;
  NS_LEAVE();
}

/* The explicit check conventions.md's "when to add one" calls for: AppKit
 * takes an index past the end silently and clears the selection, so the caller
 * would get an empty combo box rather than a report (R12, KTD4). -1 is
 * AppKit's own "no selection" and is not misuse; the largest index an accessor
 * accepts is the count minus one. */
void ns_combo_box_select_item_at_index(ns_combo_box *combo_box, long index) {
  NS_ENTER();
  NSComboBox *target = NS_IN(NSComboBox, combo_box);
  if (index != -1) NS_CHECK_INDEX(index, (long)target.numberOfItems - 1);
  [target selectItemAtIndex:(NSInteger)index];
  NS_LEAVE();
}

/* Silently taken by AppKit as well, and with nothing to deselect there is no
 * "no selection" index to spare here (R12). */
void ns_combo_box_deselect_item_at_index(ns_combo_box *combo_box, long index) {
  NS_ENTER();
  NSComboBox *target = NS_IN(NSComboBox, combo_box);
  NS_CHECK_INDEX(index, (long)target.numberOfItems - 1);
  [target deselectItemAtIndex:(NSInteger)index];
  NS_LEAVE();
}

void ns_combo_box_select_item_with_object_value(ns_combo_box *combo_box,
                                                const char *value) {
  NS_ENTER();
  NSComboBox *target = NS_IN(NSComboBox, combo_box);
  NS_CHECK_COMBO_BOX_MODE(target, false);
  [target selectItemWithObjectValue:NS_STRING_IN_OPT(value)];
  NS_LEAVE();
}

long ns_combo_box_index_of_selected_item(ns_combo_box *combo_box) {
  NS_ENTER();
  return (long)NS_IN(NSComboBox, combo_box).indexOfSelectedItem;
  NS_LEAVE();
}

/* `objectValueOfSelectedItem` is a computed id, so it crosses as an owned
 * string the caller frees with ns_string_free (R7, R11). */
char *ns_combo_box_copy_object_value_of_selected_item(
    ns_combo_box *combo_box) {
  NS_ENTER();
  NSComboBox *target = NS_IN(NSComboBox, combo_box);
  NS_CHECK_COMBO_BOX_MODE(target, false);
  return NS_STRING_OUT(target.objectValueOfSelectedItem);
  NS_LEAVE();
}

/* No index check on either scroll: a scroll to an index the list does not have
 * is a no-op with no wrong result behind it, the same call
 * ns_table_view_scroll_row_to_visible makes (R12). */
void ns_combo_box_scroll_item_at_index_to_top(ns_combo_box *combo_box,
                                              long index) {
  NS_ENTER();
  [NS_IN(NSComboBox, combo_box) scrollItemAtIndexToTop:(NSInteger)index];
  NS_LEAVE();
}

void ns_combo_box_scroll_item_at_index_to_visible(ns_combo_box *combo_box,
                                                  long index) {
  NS_ENTER();
  [NS_IN(NSComboBox, combo_box) scrollItemAtIndexToVisible:(NSInteger)index];
  NS_LEAVE();
}

void ns_combo_box_set_has_vertical_scroller(ns_combo_box *combo_box,
                                            bool has_vertical_scroller) {
  NS_ENTER();
  NS_IN(NSComboBox, combo_box).hasVerticalScroller = has_vertical_scroller;
  NS_LEAVE();
}

bool ns_combo_box_has_vertical_scroller(ns_combo_box *combo_box) {
  NS_ENTER();
  return NS_IN(NSComboBox, combo_box).hasVerticalScroller;
  NS_LEAVE();
}

void ns_combo_box_set_number_of_visible_items(ns_combo_box *combo_box,
                                              long number_of_visible_items) {
  NS_ENTER();
  NS_IN(NSComboBox, combo_box).numberOfVisibleItems =
      (NSInteger)number_of_visible_items;
  NS_LEAVE();
}

long ns_combo_box_number_of_visible_items(ns_combo_box *combo_box) {
  NS_ENTER();
  return (long)NS_IN(NSComboBox, combo_box).numberOfVisibleItems;
  NS_LEAVE();
}

void ns_combo_box_set_item_height(ns_combo_box *combo_box, CGFloat item_height) {
  NS_ENTER();
  NS_IN(NSComboBox, combo_box).itemHeight = item_height;
  NS_LEAVE();
}

CGFloat ns_combo_box_item_height(ns_combo_box *combo_box) {
  NS_ENTER();
  return NS_IN(NSComboBox, combo_box).itemHeight;
  NS_LEAVE();
}

void ns_combo_box_set_intercell_spacing(ns_combo_box *combo_box,
                                        CGSize intercell_spacing) {
  NS_ENTER();
  NS_IN(NSComboBox, combo_box).intercellSpacing = intercell_spacing;
  NS_LEAVE();
}

CGSize ns_combo_box_intercell_spacing(ns_combo_box *combo_box) {
  NS_ENTER();
  return NS_IN(NSComboBox, combo_box).intercellSpacing;
  NS_LEAVE();
}

void ns_combo_box_set_button_bordered(ns_combo_box *combo_box,
                                      bool button_bordered) {
  NS_ENTER();
  NS_IN(NSComboBox, combo_box).buttonBordered = button_bordered;
  NS_LEAVE();
}

bool ns_combo_box_button_bordered(ns_combo_box *combo_box) {
  NS_ENTER();
  return NS_IN(NSComboBox, combo_box).isButtonBordered;
  NS_LEAVE();
}

void ns_combo_box_set_completes(ns_combo_box *combo_box, bool completes) {
  NS_ENTER();
  NS_IN(NSComboBox, combo_box).completes = completes;
  NS_LEAVE();
}

bool ns_combo_box_completes(ns_combo_box *combo_box) {
  NS_ENTER();
  return NS_IN(NSComboBox, combo_box).completes;
  NS_LEAVE();
}

/* KTD6: one install, both slots, one shim conforming to both protocols. The
 * mode check comes first because it is the call's own precondition: AppKit's
 * combo box cell drops a data source assigned while `usesDataSource` is false,
 * and the list would then stay empty with nothing but a console line to say
 * why (R12). An uninstall needs no mode: nil in both slots is right in either.
 */
void ns_combo_box_set_callbacks(ns_combo_box *combo_box,
                                const ns_combo_box_callbacks *callbacks,
                                void *context) {
  NS_ENTER();
  NSComboBox *target = NS_IN(NSComboBox, combo_box);
  if (callbacks != NULL) NS_CHECK_COMBO_BOX_MODE(target, true);
  SYN_SHIM_INSTALL(target, SynComboBoxShim, syn_combo_box_table, callbacks,
                   context, ^(id shim) {
                     target.delegate = shim;
                     target.dataSource = shim;
                   });
  NS_LEAVE();
}

/* An upcast is the identity, with the debug class check NS_IN carries (KTD9). */
ns_text_field *ns_combo_box_as_text_field(ns_combo_box *combo_box) {
  NS_ENTER();
  return NS_OUT(ns_text_field, NS_IN(NSComboBox, combo_box));
  NS_LEAVE();
}

ns_control *ns_combo_box_as_control(ns_combo_box *combo_box) {
  NS_ENTER();
  return NS_OUT(ns_control, NS_IN(NSComboBox, combo_box));
  NS_LEAVE();
}

ns_view *ns_combo_box_as_view(ns_combo_box *combo_box) {
  NS_ENTER();
  return NS_OUT(ns_view, NS_IN(NSComboBox, combo_box));
  NS_LEAVE();
}

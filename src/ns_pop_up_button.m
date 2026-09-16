/*
 * NSPopUpButton: its items, its selection and the index check (R17, R12,
 * KTD17).
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syntonic/ns_pop_up_button.h"

/* Owned (+1): the caller ends it with ns_release (R7, KTD7). */
ns_pop_up_button *ns_pop_up_button_create_with_frame_pulls_down(
    CGRect frame, bool pulls_down) {
  NS_ENTER();
  return NS_OUT_OWNED(ns_pop_up_button,
                      [[NSPopUpButton alloc] initWithFrame:frame
                                                 pullsDown:pulls_down]);
  NS_LEAVE();
}

void ns_pop_up_button_add_item_with_title(ns_pop_up_button *pop_up_button,
                                          const char *title) {
  NS_ENTER();
  [NS_IN(NSPopUpButton, pop_up_button)
      addItemWithTitle:NS_STRING_IN(title)];
  NS_LEAVE();
}

/* An array in is a pointer plus a count, borrowed for the call: the titles are
 * converted here and the pointer is never kept (R11, KTD17). */
void ns_pop_up_button_add_items_with_titles(ns_pop_up_button *pop_up_button,
                                            const char *const *titles,
                                            long count) {
  NS_ENTER();
  NSPopUpButton *target = NS_IN(NSPopUpButton, pop_up_button);
  [target addItemsWithTitles:NS_STRING_ARRAY_IN(titles, count, @"")];
  NS_LEAVE();
}

/* The count half of the array shape: `itemArray` is a copy property and never
 * crosses as a handle (R7, KTD17). */
long ns_pop_up_button_number_of_items(ns_pop_up_button *pop_up_button) {
  NS_ENTER();
  return (long)NS_IN(NSPopUpButton, pop_up_button).numberOfItems;
  NS_LEAVE();
}

/* The index half. A string Syntonic returns is always an owned copy (R11). No
 * explicit range check - AppKit raises on an index past the end and the entry
 * macro reports that exception with this function's name (KTD4). */
char *ns_pop_up_button_copy_item_title_at_index(
    ns_pop_up_button *pop_up_button, long index) {
  NS_ENTER();
  return NS_STRING_OUT([NS_IN(NSPopUpButton, pop_up_button)
      itemTitleAtIndex:(NSInteger)index]);
  NS_LEAVE();
}

/* The explicit check conventions.md's "when to add one" calls for: AppKit
 * takes an out-of-range index silently, so the caller would get a wrong
 * selection rather than a report (R12, KTD4). -1 is AppKit's own "no
 * selection" and is not misuse; the largest index an accessor accepts is the
 * count minus one. */
void ns_pop_up_button_select_item_at_index(ns_pop_up_button *pop_up_button,
                                           long index) {
  NS_ENTER();
  NSPopUpButton *target = NS_IN(NSPopUpButton, pop_up_button);
  if (index != -1) NS_CHECK_INDEX(index, (long)target.numberOfItems - 1);
  [target selectItemAtIndex:(NSInteger)index];
  NS_LEAVE();
}

void ns_pop_up_button_select_item_with_title(ns_pop_up_button *pop_up_button,
                                             const char *title) {
  NS_ENTER();
  [NS_IN(NSPopUpButton, pop_up_button)
      selectItemWithTitle:NS_STRING_IN(title)];
  NS_LEAVE();
}

long ns_pop_up_button_index_of_selected_item(
    ns_pop_up_button *pop_up_button) {
  NS_ENTER();
  return (long)NS_IN(NSPopUpButton, pop_up_button).indexOfSelectedItem;
  NS_LEAVE();
}

/* `titleOfSelectedItem` is a copy property, so the value is owned and the name
 * says copy_ (R7, R11). */
char *ns_pop_up_button_copy_title_of_selected_item(
    ns_pop_up_button *pop_up_button) {
  NS_ENTER();
  return NS_STRING_OUT(
      NS_IN(NSPopUpButton, pop_up_button).titleOfSelectedItem);
  NS_LEAVE();
}

/* An upcast is the identity, with the debug class check NS_IN carries (KTD9). */
ns_button *ns_pop_up_button_as_button(ns_pop_up_button *pop_up_button) {
  NS_ENTER();
  return NS_OUT(ns_button, NS_IN(NSPopUpButton, pop_up_button));
  NS_LEAVE();
}

ns_control *ns_pop_up_button_as_control(ns_pop_up_button *pop_up_button) {
  NS_ENTER();
  return NS_OUT(ns_control, NS_IN(NSPopUpButton, pop_up_button));
  NS_LEAVE();
}

ns_view *ns_pop_up_button_as_view(ns_pop_up_button *pop_up_button) {
  NS_ENTER();
  return NS_OUT(ns_view, NS_IN(NSPopUpButton, pop_up_button));
  NS_LEAVE();
}

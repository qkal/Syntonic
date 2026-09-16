/*
 * NSSearchToolbarItem - the toolbar's search field (R16, F2).
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syntonic/ns_search_toolbar_item.h"

/* Owned (+1): the inherited initWithItemIdentifier: returns a fresh object,
 * and the caller ends it with ns_release - or hands it to the toolbar's
 * callbacks struct, where the library releases it (R7, R9, KTD7). */
ns_search_toolbar_item *ns_search_toolbar_item_create_with_item_identifier(
    const char *item_identifier) {
  NS_ENTER();
  return NS_OUT_OWNED(ns_search_toolbar_item,
                      [[NSSearchToolbarItem alloc]
                          initWithItemIdentifier:NS_STRING_IN(
                                                     item_identifier)]);
  NS_LEAVE();
}

/* Borrowed: `searchField` is a strong property, and an NSSearchField is an
 * NSTextField, which is the wrapped type it crosses as (R5, R7). */
ns_text_field *ns_search_toolbar_item_search_field(
    ns_search_toolbar_item *search_toolbar_item) {
  NS_ENTER();
  return NS_OUT(ns_text_field, NS_IN(NSSearchToolbarItem, search_toolbar_item)
                                   .searchField);
  NS_LEAVE();
}

void ns_search_toolbar_item_set_resigns_first_responder_with_cancel(
    ns_search_toolbar_item *search_toolbar_item, bool resigns) {
  NS_ENTER();
  NS_IN(NSSearchToolbarItem, search_toolbar_item)
      .resignsFirstResponderWithCancel = resigns;
  NS_LEAVE();
}

bool ns_search_toolbar_item_resigns_first_responder_with_cancel(
    ns_search_toolbar_item *search_toolbar_item) {
  NS_ENTER();
  return NS_IN(NSSearchToolbarItem, search_toolbar_item)
      .resignsFirstResponderWithCancel;
  NS_LEAVE();
}

void ns_search_toolbar_item_begin_search_interaction(
    ns_search_toolbar_item *search_toolbar_item) {
  NS_ENTER();
  [NS_IN(NSSearchToolbarItem, search_toolbar_item) beginSearchInteraction];
  NS_LEAVE();
}

void ns_search_toolbar_item_end_search_interaction(
    ns_search_toolbar_item *search_toolbar_item) {
  NS_ENTER();
  [NS_IN(NSSearchToolbarItem, search_toolbar_item) endSearchInteraction];
  NS_LEAVE();
}

/* An upcast is the identity, with the debug class check NS_IN carries (KTD9). */
ns_toolbar_item *ns_search_toolbar_item_as_toolbar_item(
    ns_search_toolbar_item *search_toolbar_item) {
  NS_ENTER();
  return NS_OUT(ns_toolbar_item,
                NS_IN(NSSearchToolbarItem, search_toolbar_item));
  NS_LEAVE();
}

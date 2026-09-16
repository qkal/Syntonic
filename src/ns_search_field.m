/*
 * NSSearchField - when the search action fires (R16, F2).
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syntonic/ns_search_field.h"

void ns_search_field_set_sends_whole_search_string(
    ns_search_field *search_field, bool sends_whole_search_string) {
  NS_ENTER();
  NS_IN(NSSearchField, search_field).sendsWholeSearchString =
      sends_whole_search_string;
  NS_LEAVE();
}

bool ns_search_field_sends_whole_search_string(ns_search_field *search_field) {
  NS_ENTER();
  return NS_IN(NSSearchField, search_field).sendsWholeSearchString;
  NS_LEAVE();
}

void ns_search_field_set_sends_search_string_immediately(
    ns_search_field *search_field, bool sends_immediately) {
  NS_ENTER();
  NS_IN(NSSearchField, search_field).sendsSearchStringImmediately =
      sends_immediately;
  NS_LEAVE();
}

bool ns_search_field_sends_search_string_immediately(
    ns_search_field *search_field) {
  NS_ENTER();
  return NS_IN(NSSearchField, search_field).sendsSearchStringImmediately;
  NS_LEAVE();
}

/* An upcast is the identity, with the debug class check NS_IN carries (KTD9). */
ns_text_field *ns_search_field_as_text_field(ns_search_field *search_field) {
  NS_ENTER();
  return NS_OUT(ns_text_field, NS_IN(NSSearchField, search_field));
  NS_LEAVE();
}

ns_control *ns_search_field_as_control(ns_search_field *search_field) {
  NS_ENTER();
  return NS_OUT(ns_control, NS_IN(NSSearchField, search_field));
  NS_LEAVE();
}

ns_view *ns_search_field_as_view(ns_search_field *search_field) {
  NS_ENTER();
  return NS_OUT(ns_view, NS_IN(NSSearchField, search_field));
  NS_LEAVE();
}

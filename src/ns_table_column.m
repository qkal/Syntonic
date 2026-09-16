/*
 * NSTableColumn: one column of a table or an outline (R17).
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syntonic/ns_table_column.h"

/* Owned (+1): an initWith… returns a fresh object the caller ends with
 * ns_release (R7, KTD7). */
ns_table_column *ns_table_column_create_with_identifier(
    const char *identifier) {
  NS_ENTER();
  return NS_OUT_OWNED(
      ns_table_column,
      [[NSTableColumn alloc] initWithIdentifier:NS_STRING_IN(identifier)]);
  NS_LEAVE();
}

/* `identifier` and `title` are copy properties, so both are owned and named
 * copy_ (R7, R11). */
char *ns_table_column_copy_identifier(ns_table_column *table_column) {
  NS_ENTER();
  return NS_STRING_OUT(NS_IN(NSTableColumn, table_column).identifier);
  NS_LEAVE();
}

void ns_table_column_set_title(ns_table_column *table_column,
                               const char *title) {
  NS_ENTER();
  NS_IN(NSTableColumn, table_column).title = NS_STRING_IN(title);
  NS_LEAVE();
}

char *ns_table_column_copy_title(ns_table_column *table_column) {
  NS_ENTER();
  return NS_STRING_OUT(NS_IN(NSTableColumn, table_column).title);
  NS_LEAVE();
}

void ns_table_column_set_width(ns_table_column *table_column, CGFloat width) {
  NS_ENTER();
  NS_IN(NSTableColumn, table_column).width = width;
  NS_LEAVE();
}

CGFloat ns_table_column_width(ns_table_column *table_column) {
  NS_ENTER();
  return NS_IN(NSTableColumn, table_column).width;
  NS_LEAVE();
}

void ns_table_column_set_min_width(ns_table_column *table_column,
                                   CGFloat min_width) {
  NS_ENTER();
  NS_IN(NSTableColumn, table_column).minWidth = min_width;
  NS_LEAVE();
}

CGFloat ns_table_column_min_width(ns_table_column *table_column) {
  NS_ENTER();
  return NS_IN(NSTableColumn, table_column).minWidth;
  NS_LEAVE();
}

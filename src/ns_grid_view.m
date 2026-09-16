/*
 * NSGridView: the two-dimensional constructor, the row and column counts and
 * the two spacings (KTD3, KTD17).
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syntonic/ns_grid_view.h"

/* Owned (+1): the caller ends it with ns_release (R7, KTD7). The rows and
 * their views are borrowed for the call; the grid retains what it keeps. The
 * two-dimensional shape - row descriptors plus a row count - is
 * docs/conventions.md's rule for an array of arrays (KTD17). */
ns_grid_view *ns_grid_view_create_with_views(const ns_grid_view_row *rows,
                                             long row_count) {
  NS_ENTER();
  NSMutableArray<NSArray<NSView *> *> *grid_rows =
      [NSMutableArray arrayWithCapacity:(NSUInteger)(row_count > 0 ? row_count
                                                                   : 0)];
  for (long row = 0; row < row_count; row++) {
    long count = rows[row].count;
    NSMutableArray<NSView *> *views =
        [NSMutableArray arrayWithCapacity:(NSUInteger)(count > 0 ? count : 0)];
    for (long column = 0; column < count; column++)
      [views addObject:NS_IN(NSView, rows[row].views[column])];
    [grid_rows addObject:views];
  }
  return NS_OUT_OWNED(ns_grid_view, [NSGridView gridViewWithViews:grid_rows]);
  NS_LEAVE();
}

long ns_grid_view_number_of_rows(ns_grid_view *grid_view) {
  NS_ENTER();
  return (long)NS_IN(NSGridView, grid_view).numberOfRows;
  NS_LEAVE();
}

long ns_grid_view_number_of_columns(ns_grid_view *grid_view) {
  NS_ENTER();
  return (long)NS_IN(NSGridView, grid_view).numberOfColumns;
  NS_LEAVE();
}

void ns_grid_view_set_row_spacing(ns_grid_view *grid_view, CGFloat spacing) {
  NS_ENTER();
  NS_IN(NSGridView, grid_view).rowSpacing = spacing;
  NS_LEAVE();
}

CGFloat ns_grid_view_row_spacing(ns_grid_view *grid_view) {
  NS_ENTER();
  return NS_IN(NSGridView, grid_view).rowSpacing;
  NS_LEAVE();
}

void ns_grid_view_set_column_spacing(ns_grid_view *grid_view, CGFloat spacing) {
  NS_ENTER();
  NS_IN(NSGridView, grid_view).columnSpacing = spacing;
  NS_LEAVE();
}

CGFloat ns_grid_view_column_spacing(ns_grid_view *grid_view) {
  NS_ENTER();
  return NS_IN(NSGridView, grid_view).columnSpacing;
  NS_LEAVE();
}

/* The upcast is the same pointer; NS_IN is what checks the class (R4, KTD9). */
ns_view *ns_grid_view_as_view(ns_grid_view *grid_view) {
  NS_ENTER();
  return NS_OUT(ns_view, NS_IN(NSGridView, grid_view));
  NS_LEAVE();
}

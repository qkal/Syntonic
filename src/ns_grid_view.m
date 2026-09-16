/*
 * NSGridView and the three classes that share its SDK header - NSGridRow,
 * NSGridColumn and NSGridCell: the two constructors, the row, column and cell
 * accessors, placement and a column's width (KTD3, KTD17).
 *
 * A row, a column and a cell are children the grid holds, so every accessor
 * here is a borrowed return on docs/conventions.md's child-accessor allowlist
 * (R7). None of the three is an NSView, so none of them has an upcast.
 *
 * Every index accessor raises NSRangeException before it changes anything, so
 * none of them carries NS_CHECK_INDEX: the entry macro's exception report is
 * the check and it names the function (R12, and "The index check").
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

/* Owned (+1), like every other constructor (R7, KTD7). */
ns_grid_view *ns_grid_view_create_with_number_of_columns_rows(long column_count,
                                                              long row_count) {
  NS_ENTER();
  return NS_OUT_OWNED(
      ns_grid_view,
      [NSGridView gridViewWithNumberOfColumns:(NSInteger)column_count
                                         rows:(NSInteger)row_count]);
  NS_LEAVE();
}

/* The views are borrowed for the call and the grid retains what it keeps; the
 * row it hands back is the grid's, so the return is borrowed (R7, KTD17). */
ns_grid_row *ns_grid_view_add_row_with_views(ns_grid_view *grid_view,
                                             ns_view *const *views,
                                             long count) {
  NS_ENTER();
  NSMutableArray<NSView *> *row =
      [NSMutableArray arrayWithCapacity:(NSUInteger)(count > 0 ? count : 0)];
  for (long index = 0; index < count; index++)
    [row addObject:NS_IN(NSView, views[index])];
  return NS_OUT(ns_grid_row,
                [NS_IN(NSGridView, grid_view) addRowWithViews:row]);
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

/* Borrowed: the grid holds its rows, its columns and its cells, which is what
 * puts these four on the child-accessor allowlist (R7). */
ns_grid_row *ns_grid_view_row_at_index(ns_grid_view *grid_view, long index) {
  NS_ENTER();
  return NS_OUT(ns_grid_row,
                [NS_IN(NSGridView, grid_view) rowAtIndex:(NSInteger)index]);
  NS_LEAVE();
}

ns_grid_column *ns_grid_view_column_at_index(ns_grid_view *grid_view,
                                             long index) {
  NS_ENTER();
  return NS_OUT(ns_grid_column,
                [NS_IN(NSGridView, grid_view) columnAtIndex:(NSInteger)index]);
  NS_LEAVE();
}

ns_grid_cell *ns_grid_view_cell_at_column_index_row_index(
    ns_grid_view *grid_view, long column_index, long row_index) {
  NS_ENTER();
  return NS_OUT(ns_grid_cell,
                [NS_IN(NSGridView, grid_view)
                    cellAtColumnIndex:(NSInteger)column_index
                             rowIndex:(NSInteger)row_index]);
  NS_LEAVE();
}

ns_grid_cell *ns_grid_view_cell_for_view(ns_grid_view *grid_view,
                                         ns_view *view) {
  NS_ENTER();
  return NS_OUT(ns_grid_cell, [NS_IN(NSGridView, grid_view)
                                  cellForView:NS_IN(NSView, view)]);
  NS_LEAVE();
}

/* NSRange is Foundation's and does not cross, so the two scalars are what the
 * caller passes and the wrapper rebuilds the range (KTD17). */
void ns_grid_row_merge_cells_in_range(ns_grid_row *grid_row, long location,
                                      long length) {
  NS_ENTER();
  [NS_IN(NSGridRow, grid_row)
      mergeCellsInRange:NSMakeRange((NSUInteger)location, (NSUInteger)length)];
  NS_LEAVE();
}

void ns_grid_column_set_x_placement(ns_grid_column *grid_column,
                                    ns_grid_cell_placement x_placement) {
  NS_ENTER();
  NS_IN(NSGridColumn, grid_column).xPlacement =
      (NSGridCellPlacement)x_placement;
  NS_LEAVE();
}

ns_grid_cell_placement ns_grid_column_x_placement(ns_grid_column *grid_column) {
  NS_ENTER();
  return (ns_grid_cell_placement)NS_IN(NSGridColumn, grid_column).xPlacement;
  NS_LEAVE();
}

void ns_grid_column_set_width(ns_grid_column *grid_column, CGFloat width) {
  NS_ENTER();
  NS_IN(NSGridColumn, grid_column).width = width;
  NS_LEAVE();
}

CGFloat ns_grid_column_width(ns_grid_column *grid_column) {
  NS_ENTER();
  return NS_IN(NSGridColumn, grid_column).width;
  NS_LEAVE();
}

/* A `strong` class property, so it is borrowed on the same footing as
 * +[NSColor labelColor] and +[NSApplication sharedApplication] (R7). */
ns_view *ns_grid_cell_empty_content_view(void) {
  NS_ENTER();
  return NS_OUT(ns_view, NSGridCell.emptyContentView);
  NS_LEAVE();
}

void ns_grid_cell_set_content_view(ns_grid_cell *grid_cell,
                                   ns_view *content_view) {
  NS_ENTER();
  NS_IN(NSGridCell, grid_cell).contentView = NS_IN_OPT(NSView, content_view);
  NS_LEAVE();
}

/* `contentView` is a strong property, so the return is borrowed (R7). */
ns_view *ns_grid_cell_content_view(ns_grid_cell *grid_cell) {
  NS_ENTER();
  return NS_OUT(ns_view, NS_IN(NSGridCell, grid_cell).contentView);
  NS_LEAVE();
}

void ns_grid_cell_set_x_placement(ns_grid_cell *grid_cell,
                                  ns_grid_cell_placement x_placement) {
  NS_ENTER();
  NS_IN(NSGridCell, grid_cell).xPlacement = (NSGridCellPlacement)x_placement;
  NS_LEAVE();
}

ns_grid_cell_placement ns_grid_cell_x_placement(ns_grid_cell *grid_cell) {
  NS_ENTER();
  return (ns_grid_cell_placement)NS_IN(NSGridCell, grid_cell).xPlacement;
  NS_LEAVE();
}

/* `row` and `column` are weak properties, so both returns are borrowed (R7). */
ns_grid_row *ns_grid_cell_row(ns_grid_cell *grid_cell) {
  NS_ENTER();
  return NS_OUT(ns_grid_row, NS_IN(NSGridCell, grid_cell).row);
  NS_LEAVE();
}

ns_grid_column *ns_grid_cell_column(ns_grid_cell *grid_cell) {
  NS_ENTER();
  return NS_OUT(ns_grid_column, NS_IN(NSGridCell, grid_cell).column);
  NS_LEAVE();
}

/* The upcast is the same pointer; NS_IN is what checks the class (R4, KTD9). */
ns_view *ns_grid_view_as_view(ns_grid_view *grid_view) {
  NS_ENTER();
  return NS_OUT(ns_view, NS_IN(NSGridView, grid_view));
  NS_LEAVE();
}

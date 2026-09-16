/*
 * NSGridView - the spreadsheet-shaped form the detail pane and every settings
 * pane is built on (R17, R19, KTD3).
 *
 * A grid is built one of two ways. All at once:
 * +[NSGridView gridViewWithViews:] takes an array of arrays, which the
 * one-dimensional "pointer plus count" rule has no shape for, so the
 * two-dimensional shape is an array of ns_grid_view_row - each a view-pointer
 * array plus its own count - plus the number of rows (docs/conventions.md,
 * Boundary types). Or incrementally:
 * +[NSGridView gridViewWithNumberOfColumns:rows:] makes the empty grid and
 * -[NSGridView addRowWithViews:] appends a row at a time, which is what a form
 * whose rows differ from one another wants. A short row leaves the cells past
 * its end empty; the grid is as wide as its widest row.
 *
 * THE FOUR CLASSES. NSGridView, NSGridRow, NSGridColumn and NSGridCell are
 * four AppKit classes in one SDK header, so they are four handle types in this
 * one mirror header. None of them is a view except the grid itself - a row, a
 * column and a cell all descend directly from NSObject - so none of the three
 * has an upcast.
 *
 * ns_grid_view_row is the boundary struct the all-at-once constructor takes,
 * and ns_grid_row is the handle for the AppKit class. The two are not the same
 * thing and neither converts to the other.
 *
 * OWNERSHIP (R7). A row, a column and a cell are children the grid holds, and
 * every accessor that reaches one hands back a borrowed reference on
 * docs/conventions.md's child-accessor allowlist - the same footing as
 * -[NSMenu itemAtIndex:]. Keep one past the call with ns_retain if you have a
 * reason to; the grid outliving it is the ordinary case.
 *
 * A grid sizes itself from its content through Auto Layout, so place it with
 * ns_layout_pin_edges or ns_layout_pin_top_edges - which turn its autoresizing
 * translation off - rather than by setting a frame.
 */

#ifndef SYNTONIC_NS_GRID_VIEW_H
#define SYNTONIC_NS_GRID_VIEW_H

#include <CoreGraphics/CGGeometry.h>
#include <os/availability.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handles. One opaque struct type per AppKit class (R4, KTD9): the four
 * classes NSGridView.h declares are the four types here. */
typedef struct ns_grid_view ns_grid_view;
typedef struct ns_grid_row ns_grid_row;
typedef struct ns_grid_column ns_grid_column;
typedef struct ns_grid_cell ns_grid_cell;

/* Declared by ns_view.h and repeated here so this header stands alone. C has
 * allowed a repeated typedef of the same type since C11. */
typedef struct ns_view ns_view;

/* NSGridCellPlacement, which is where a cell's content view sits inside it on
 * one axis. `INHERITED` is the default on a cell, a row and a column, and
 * falls back to the row's or column's value and then to the grid's own.
 * AppKit spells two of these twice - `TOP` is `LEADING` and `BOTTOM` is
 * `TRAILING`, which is the same constant read on the other axis - and the
 * duplicates are kept here with the SDK's own values (R5). */
typedef enum ns_grid_cell_placement : int64_t {
  NS_GRID_CELL_PLACEMENT_INHERITED = 0,
  NS_GRID_CELL_PLACEMENT_NONE = 1,
  NS_GRID_CELL_PLACEMENT_LEADING = 2,
  NS_GRID_CELL_PLACEMENT_TOP = 2,
  NS_GRID_CELL_PLACEMENT_TRAILING = 3,
  NS_GRID_CELL_PLACEMENT_BOTTOM = 3,
  NS_GRID_CELL_PLACEMENT_CENTER = 4,
  NS_GRID_CELL_PLACEMENT_FILL = 5,
} ns_grid_cell_placement;

/* One row of the two-dimensional array a grid is created from: the row's views
 * as a pointer plus a count, borrowed for the call (KTD17). This is a boundary
 * shape, not a wrapper for NSGridRow. */
typedef struct ns_grid_view_row {
  ns_view *_Nonnull const *_Nullable views;
  long count;
} ns_grid_view_row;

/* +[NSGridView gridViewWithViews:] - owned (+1), released with ns_release
 * (R7). `rows` is a pointer plus a count, and each row carries its own views
 * the same way; all of it is borrowed for the call (KTD17). Pass null and 0
 * for an empty grid. */
ns_grid_view *_Nonnull ns_grid_view_create_with_views(
    const ns_grid_view_row *_Nullable rows, long row_count)
    API_AVAILABLE(macos(26.0));

/* +[NSGridView gridViewWithNumberOfColumns:rows:] - owned (+1), released with
 * ns_release (R7). The empty grid a form fills in with
 * ns_grid_view_add_row_with_views: the columns are there from the start, so a
 * row added later can address a column its own views do not reach. A negative
 * count is an empty dimension, which is AppKit's own answer. */
ns_grid_view *_Nonnull ns_grid_view_create_with_number_of_columns_rows(
    long column_count, long row_count) API_AVAILABLE(macos(26.0));

/* -[NSGridView addRowWithViews:] - appends one row and hands it back borrowed:
 * the grid holds its rows (R7). `views` is a pointer plus a count, borrowed
 * for the call (KTD17); null and 0 append an empty row. The grid grows to hold
 * a row wider than itself. Pass ns_grid_cell_empty_content_view() in a
 * position whose cell is to stay empty. */
ns_grid_row *_Nonnull ns_grid_view_add_row_with_views(
    ns_grid_view *_Nonnull grid_view,
    ns_view *_Nonnull const *_Nullable views, long count)
    API_AVAILABLE(macos(26.0));

/* -[NSGridView numberOfRows] */
long ns_grid_view_number_of_rows(ns_grid_view *_Nonnull grid_view)
    API_AVAILABLE(macos(26.0));

/* -[NSGridView numberOfColumns] */
long ns_grid_view_number_of_columns(ns_grid_view *_Nonnull grid_view)
    API_AVAILABLE(macos(26.0));

/* -[NSGridView setRowSpacing:] - the vertical gap between two rows. */
void ns_grid_view_set_row_spacing(ns_grid_view *_Nonnull grid_view,
                                  CGFloat spacing) API_AVAILABLE(macos(26.0));

/* -[NSGridView rowSpacing] */
CGFloat ns_grid_view_row_spacing(ns_grid_view *_Nonnull grid_view)
    API_AVAILABLE(macos(26.0));

/* -[NSGridView setColumnSpacing:] - the horizontal gap between two columns. */
void ns_grid_view_set_column_spacing(ns_grid_view *_Nonnull grid_view,
                                     CGFloat spacing)
    API_AVAILABLE(macos(26.0));

/* -[NSGridView columnSpacing] */
CGFloat ns_grid_view_column_spacing(ns_grid_view *_Nonnull grid_view)
    API_AVAILABLE(macos(26.0));

/* -[NSGridView rowAtIndex:] - borrowed: the grid holds its rows (R7). An index
 * outside 0..numberOfRows-1 raises NSRangeException before the call changes
 * anything, and a debug build reports that exception with this function named
 * (R12), so there is no index check of its own here. */
ns_grid_row *_Nonnull ns_grid_view_row_at_index(
    ns_grid_view *_Nonnull grid_view, long index) API_AVAILABLE(macos(26.0));

/* -[NSGridView columnAtIndex:] - borrowed, and out of range the same way
 * ns_grid_view_row_at_index is (R7, R12). */
ns_grid_column *_Nonnull ns_grid_view_column_at_index(
    ns_grid_view *_Nonnull grid_view, long index) API_AVAILABLE(macos(26.0));

/* -[NSGridView cellAtColumnIndex:rowIndex:] - borrowed, and out of range the
 * same way ns_grid_view_row_at_index is (R7, R12). Every index inside a merged
 * region answers with the one merged cell. */
ns_grid_cell *_Nonnull ns_grid_view_cell_at_column_index_row_index(
    ns_grid_view *_Nonnull grid_view, long column_index, long row_index)
    API_AVAILABLE(macos(26.0));

/* -[NSGridView cellForView:] - the cell `view` sits in, borrowed (R7). Null
 * when the view is in no cell of this grid. A view inside a cell's content
 * view answers with that cell, which is AppKit's own "or one of its
 * ancestors". */
ns_grid_cell *_Nullable ns_grid_view_cell_for_view(
    ns_grid_view *_Nonnull grid_view, ns_view *_Nonnull view)
    API_AVAILABLE(macos(26.0));

/* -[NSGridRow mergeCellsInRange:] - the cell at the start of the range grows
 * over the whole of it and the rest stop holding anything. NSRange belongs to
 * Foundation's NSRange.h, which is not includable from C, so the range crosses
 * as its two scalars in AppKit's own order. A range reaching past the row
 * raises NSRangeException and merges nothing (R12). */
void ns_grid_row_merge_cells_in_range(ns_grid_row *_Nonnull grid_row,
                                      long location, long length)
    API_AVAILABLE(macos(26.0));

/* -[NSGridColumn setXPlacement:] - where every cell of this column puts its
 * content view horizontally, unless the cell says otherwise. */
void ns_grid_column_set_x_placement(ns_grid_column *_Nonnull grid_column,
                                    ns_grid_cell_placement x_placement)
    API_AVAILABLE(macos(26.0));

/* -[NSGridColumn xPlacement] */
ns_grid_cell_placement ns_grid_column_x_placement(
    ns_grid_column *_Nonnull grid_column) API_AVAILABLE(macos(26.0));

/* -[NSGridColumn setWidth:] - a fixed width for the column in points, which is
 * what a form's field column is given. AppKit's own default is the marker
 * value that means "fit the content"; this wrapper has no name for it, so read
 * the width before setting it if you mean to put it back. */
void ns_grid_column_set_width(ns_grid_column *_Nonnull grid_column,
                              CGFloat width) API_AVAILABLE(macos(26.0));

/* -[NSGridColumn width] */
CGFloat ns_grid_column_width(ns_grid_column *_Nonnull grid_column)
    API_AVAILABLE(macos(26.0));

/* +[NSGridCell emptyContentView] - the marker that says "this cell holds
 * nothing", passed in a view position to ns_grid_view_add_row_with_views or
 * ns_grid_view_create_with_views. It is a `strong` class property, so it is
 * borrowed and AppKit holds it for the life of the process (R7), and it is one
 * view: passing it in several positions is correct and is what a form whose
 * first column is empty in some rows does. */
ns_view *_Nonnull ns_grid_cell_empty_content_view(void)
    API_AVAILABLE(macos(26.0));

/* -[NSGridCell setContentView:] - the view this cell places; null empties it.
 * The cell holds the view. */
void ns_grid_cell_set_content_view(ns_grid_cell *_Nonnull grid_cell,
                                   ns_view *_Nullable content_view)
    API_AVAILABLE(macos(26.0));

/* -[NSGridCell contentView] - a `strong` property, so this is borrowed (R7).
 * Null for a cell that holds nothing, which is what a cell built from
 * ns_grid_cell_empty_content_view reports: the marker never becomes the cell's
 * content view. */
ns_view *_Nullable ns_grid_cell_content_view(ns_grid_cell *_Nonnull grid_cell)
    API_AVAILABLE(macos(26.0));

/* -[NSGridCell setXPlacement:] - where this one cell puts its content view
 * horizontally, overriding its column's and the grid's. */
void ns_grid_cell_set_x_placement(ns_grid_cell *_Nonnull grid_cell,
                                  ns_grid_cell_placement x_placement)
    API_AVAILABLE(macos(26.0));

/* -[NSGridCell xPlacement] */
ns_grid_cell_placement ns_grid_cell_x_placement(
    ns_grid_cell *_Nonnull grid_cell) API_AVAILABLE(macos(26.0));

/* -[NSGridCell row] - a `weak` property, so this is borrowed (R7). */
ns_grid_row *_Nullable ns_grid_cell_row(ns_grid_cell *_Nonnull grid_cell)
    API_AVAILABLE(macos(26.0));

/* -[NSGridCell column] - a `weak` property, so this is borrowed (R7). */
ns_grid_column *_Nullable ns_grid_cell_column(ns_grid_cell *_Nonnull grid_cell)
    API_AVAILABLE(macos(26.0));

/* The upcast to ns_view: the same pointer, checked in a debug build (R4,
 * KTD9). syntonic-owned. */
ns_view *_Nonnull ns_grid_view_as_view(ns_grid_view *_Nonnull grid_view)
    API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_GRID_VIEW_H */

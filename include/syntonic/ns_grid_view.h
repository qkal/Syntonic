/*
 * NSGridView - the spreadsheet-shaped form the detail pane and every settings
 * pane is built on (R17, R19, KTD3).
 *
 * A grid is created from its rows: +[NSGridView gridViewWithViews:] takes an
 * array of arrays, which the one-dimensional "pointer plus count" rule has no
 * shape for. The two-dimensional shape is an array of ns_grid_view_row - each
 * a view-pointer array plus its own count - plus the number of rows
 * (docs/conventions.md, Boundary types). A short row leaves the cells past its
 * end empty; the grid is as wide as its widest row.
 *
 * ns_grid_view_row is that boundary struct and nothing else. NSGridRow,
 * NSGridColumn and NSGridCell are separate AppKit classes declared in the same
 * SDK header, and v0 wraps none of them: cell placement, merged cells and
 * explicit row or column sizes are out of reach until a later unit adds them
 * here.
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

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_grid_view ns_grid_view;

/* Declared by ns_view.h and repeated here so this header stands alone. C has
 * allowed a repeated typedef of the same type since C11. */
typedef struct ns_view ns_view;

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

/* The upcast to ns_view: the same pointer, checked in a debug build (R4,
 * KTD9). syntonic-owned. */
ns_view *_Nonnull ns_grid_view_as_view(ns_grid_view *_Nonnull grid_view)
    API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_GRID_VIEW_H */

/*
 * NSTableView - the list pane's table, view-based and driven by one merged
 * callbacks struct (R9, R17, F2, F6, KTD6).
 *
 * THE MERGED STRUCT (KTD6). AppKit splits a table across two protocols: the
 * row count comes from NSTableViewDataSource, the cell and the selection from
 * NSTableViewDelegate. Syntonic merges the pair into one
 * ns_table_view_callbacks, so one call - ns_table_view_set_callbacks - fills
 * both AppKit slots with one shim. Installing again replaces the struct and
 * its context; a null struct uninstalls (R9, KTD8).
 *
 * WHAT A CELL IS. The table is view-based, and the wrapper owns the view: it
 * builds an NSTableCellView with a label the first time a row needs one, tags
 * it so AppKit's own reuse finds it again, and asks the struct for nothing but
 * the string (KTD6). `cell_string` returns a **borrowed** UTF-8 pointer, valid
 * only for the duration of the callback - the wrapper copies it into the cell
 * before returning, and the caller may reuse its buffer the moment the
 * callback ends (R11, KTD17).
 *
 *     static long rows(void *context, ns_table_view *table) {
 *       (void)table; return ((struct model *)context)->count;
 *     }
 *     static const char *cell(void *context, ns_table_view *table,
 *                             long column, long row) {
 *       (void)table; return ((struct model *)context)->rows[row][column];
 *     }
 *     ns_table_view_callbacks callbacks = {.number_of_rows = rows,
 *                                          .cell_string = cell};
 *     ns_table_view_set_callbacks(table, &callbacks, &model);
 *     ns_table_view_reload_data(table);
 *
 * SELECTION. `selectedRow` is -1 when nothing is selected, which the wrapper
 * passes through unchanged (R11). Selecting a row programmatically does not
 * scroll it into view - AppKit does not, and neither does Syntonic - so a row
 * appended past the bottom is revealed with ns_table_view_scroll_row_to_visible
 * (R16).
 *
 * A reload that leaves fewer rows than the selected one clears the selection
 * **without** firing `selection_did_change`: AppKit posts no notification for
 * that, and Syntonic invents none. Deleting a row and clearing a detail pane
 * from the callback therefore goes: ns_table_view_deselect_all, which does
 * fire, then the model change, then ns_table_view_reload_data (F6).
 *
 * A table lives inside a scroll view; see ns_scroll_view.h.
 */

#ifndef SYNTONIC_NS_TABLE_VIEW_H
#define SYNTONIC_NS_TABLE_VIEW_H

#include <CoreGraphics/CGGeometry.h>
#include <os/availability.h>
#include <stdbool.h>
#include <stdint.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_table_view ns_table_view;

/* Declared by their own headers and repeated here so this one stands alone. C
 * has allowed a repeated typedef of the same type since C11. */
typedef struct ns_table_column ns_table_column;
typedef struct ns_control ns_control;
typedef struct ns_view ns_view;

/* NSTableViewStyle. The inset style is what a list pane uses and the source
 * list style is what a sidebar outline uses (R15); an outline created by
 * ns_outline_view_create_with_frame starts on the source list style already. */
typedef enum ns_table_view_style : int64_t {
  NS_TABLE_VIEW_STYLE_AUTOMATIC = 0,
  NS_TABLE_VIEW_STYLE_FULL_WIDTH = 1,
  NS_TABLE_VIEW_STYLE_INSET = 2,
  NS_TABLE_VIEW_STYLE_SOURCE_LIST = 3,
  NS_TABLE_VIEW_STYLE_PLAIN = 4,
} ns_table_view_style;

/* NSTableViewColumnAutoresizingStyle. What a table does with its columns when
 * its own frame changes. The SDK's default is
 * NS_TABLE_VIEW_LAST_COLUMN_ONLY_AUTORESIZING_STYLE, which is what a list pane
 * with one wide title column wants, so a program that wants that need not set
 * it at all. */
typedef enum ns_table_view_column_autoresizing_style : uint64_t {
  NS_TABLE_VIEW_NO_COLUMN_AUTORESIZING = 0,
  NS_TABLE_VIEW_UNIFORM_COLUMN_AUTORESIZING_STYLE = 1,
  NS_TABLE_VIEW_SEQUENTIAL_COLUMN_AUTORESIZING_STYLE = 2,
  NS_TABLE_VIEW_REVERSE_SEQUENTIAL_COLUMN_AUTORESIZING_STYLE = 3,
  NS_TABLE_VIEW_LAST_COLUMN_ONLY_AUTORESIZING_STYLE = 4,
  NS_TABLE_VIEW_FIRST_COLUMN_ONLY_AUTORESIZING_STYLE = 5,
} ns_table_view_column_autoresizing_style;

/*
 * NSTableViewDataSource and NSTableViewDelegate, merged into one struct of
 * function pointers (KTD6). `number_of_rows` and `cell_string` are required
 * and `selection_did_change` is optional; docs/conventions.md's per-protocol
 * table is what says so, because both protocols are entirely @optional in the
 * SDK. An unset required member is reported at install time in a debug build,
 * before AppKit ever asks for a row; an unset optional member behaves exactly
 * as a method the object does not implement (R9, KTD8).
 *
 * `context` is what you passed to ns_table_view_set_callbacks, untouched and
 * never freed by the library; `sender` is the table, borrowed for the duration
 * of the call. Every member runs on the main thread. syntonic-owned.
 */
typedef struct ns_table_view_callbacks {
  /* required. -[NSTableViewDataSource numberOfRowsInTableView:] - a negative
   * count is reported in a debug build (KTD8). */
  long (*_Nullable number_of_rows)(void *_Nullable context,
                                   ns_table_view *_Nonnull sender);
  /* required. syntonic-owned: the one string this cell shows, which the
   * wrapper copies into the cell view it built and reuses for
   * -[NSTableViewDelegate tableView:viewForTableColumn:row:] (KTD6). `column`
   * is the column's index in the order the columns were added and `row` is the
   * row's index. The returned pointer is borrowed by the library for the
   * duration of the callback and copied at once; null is reported in a debug
   * build. */
  const char *_Nonnull (*_Nullable cell_string)(void *_Nullable context,
                                                ns_table_view *_Nonnull sender,
                                                long column, long row);
  /* optional. -[NSTableViewDelegate tableViewSelectionDidChange:] - `row` is
   * the newly selected row, or -1 when the selection was cleared (F6). */
  void (*_Nullable selection_did_change)(void *_Nullable context,
                                         ns_table_view *_Nonnull sender,
                                         long row);
} ns_table_view_callbacks;

/* -[NSTableView initWithFrame:] - owned (+1), released with ns_release (R7).
 * A constructor is the one inherited member a subclass redeclares, because an
 * upcast needs an object to upcast. */
ns_table_view *_Nonnull ns_table_view_create_with_frame(CGRect frame)
    API_AVAILABLE(macos(26.0));

/* -[NSTableView addTableColumn:] - the table retains the column, so releasing
 * your own reference afterwards is correct (R7). */
void ns_table_view_add_table_column(ns_table_view *_Nonnull table_view,
                                    ns_table_column *_Nonnull table_column)
    API_AVAILABLE(macos(26.0));

/* -[NSTableView numberOfColumns] */
long ns_table_view_number_of_columns(ns_table_view *_Nonnull table_view)
    API_AVAILABLE(macos(26.0));

/* -[NSTableView numberOfRows] - what the last reload asked the struct for. */
long ns_table_view_number_of_rows(ns_table_view *_Nonnull table_view)
    API_AVAILABLE(macos(26.0));

/* -[NSTableView reloadData] - asks the struct for the row count and for every
 * visible cell again. */
void ns_table_view_reload_data(ns_table_view *_Nonnull table_view)
    API_AVAILABLE(macos(26.0));

/* -[NSTableView selectedRow] - -1 when nothing is selected (R11). */
long ns_table_view_selected_row(ns_table_view *_Nonnull table_view)
    API_AVAILABLE(macos(26.0));

/* -[NSTableView selectRowIndexes:byExtendingSelection:] with a single index,
 * which is what an index set argument crosses as in v0 (KTD17). `row` is
 * checked against the row count in a debug build (R12). */
void ns_table_view_select_row_by_extending_selection(
    ns_table_view *_Nonnull table_view, long row, bool extend)
    API_AVAILABLE(macos(26.0));

/* -[NSTableView deselectAll:] - the sender argument is an `id`, which never
 * crosses (R11), so the wrapper passes nil. */
void ns_table_view_deselect_all(ns_table_view *_Nonnull table_view)
    API_AVAILABLE(macos(26.0));

/* -[NSTableView scrollRowToVisible:] - selecting a row does not scroll to it,
 * so a row appended past the bottom is revealed with this (R16). */
void ns_table_view_scroll_row_to_visible(ns_table_view *_Nonnull table_view,
                                         long row) API_AVAILABLE(macos(26.0));

/* -[NSTableView viewAtColumn:row:makeIfNecessary:] - borrowed: the table keeps
 * the view (R7). Null when there is none and `make_if_necessary` is false. The
 * cell view the wrapper builds carries its label as its one subview, reachable
 * with ns_view_subview_at_index. */
ns_view *_Nullable ns_table_view_view_at_column_row_make_if_necessary(
    ns_table_view *_Nonnull table_view, long column, long row,
    bool make_if_necessary) API_AVAILABLE(macos(26.0));

/* -[NSTableView setStyle:] */
void ns_table_view_set_style(ns_table_view *_Nonnull table_view,
                             ns_table_view_style style)
    API_AVAILABLE(macos(26.0));

/* -[NSTableView style] - get_, because ns_table_view_style is already a type
 * name and C puts the two in one namespace (R5). */
ns_table_view_style ns_table_view_get_style(ns_table_view *_Nonnull table_view)
    API_AVAILABLE(macos(26.0));

/* -[NSTableView setHeaderView:] - null removes the header row, which is what a
 * sidebar outline wants. The class check accepts only the header view the
 * table made for itself, so keep the one from ns_table_view_header_view if you
 * mean to put it back (R12). */
void ns_table_view_set_header_view(ns_table_view *_Nonnull table_view,
                                   ns_view *_Nullable header_view)
    API_AVAILABLE(macos(26.0));

/* -[NSTableView headerView] - a strong property, so this is borrowed and valid
 * while the table holds it; ns_retain keeps it past that (R7). */
ns_view *_Nullable ns_table_view_header_view(
    ns_table_view *_Nonnull table_view) API_AVAILABLE(macos(26.0));

/* -[NSTableView setColumnAutoresizingStyle:] - what the columns do when the
 * table's own frame changes. The SDK's default is already
 * NS_TABLE_VIEW_LAST_COLUMN_ONLY_AUTORESIZING_STYLE. */
void ns_table_view_set_column_autoresizing_style(
    ns_table_view *_Nonnull table_view,
    ns_table_view_column_autoresizing_style style) API_AVAILABLE(macos(26.0));

/* -[NSTableView columnAutoresizingStyle] - get_, because
 * ns_table_view_column_autoresizing_style is already a type name and C puts
 * the two in one namespace (R5). */
ns_table_view_column_autoresizing_style
ns_table_view_get_column_autoresizing_style(
    ns_table_view *_Nonnull table_view) API_AVAILABLE(macos(26.0));

/* -[NSTableView setUsesAlternatingRowBackgroundColors:] */
void ns_table_view_set_uses_alternating_row_background_colors(
    ns_table_view *_Nonnull table_view, bool uses_alternating_colors)
    API_AVAILABLE(macos(26.0));

/* -[NSTableView usesAlternatingRowBackgroundColors] */
bool ns_table_view_uses_alternating_row_background_colors(
    ns_table_view *_Nonnull table_view) API_AVAILABLE(macos(26.0));

/* -[NSTableView setAllowsEmptySelection:] - false keeps one row selected at
 * all times, which is what a sidebar wants. */
void ns_table_view_set_allows_empty_selection(
    ns_table_view *_Nonnull table_view, bool allows_empty_selection)
    API_AVAILABLE(macos(26.0));

/* -[NSTableView allowsEmptySelection] */
bool ns_table_view_allows_empty_selection(ns_table_view *_Nonnull table_view)
    API_AVAILABLE(macos(26.0));

/* -[NSTableView setFloatsGroupRows:] - false pins a source list's group rows
 * in place rather than floating them over the scrolled content (R15). */
void ns_table_view_set_floats_group_rows(ns_table_view *_Nonnull table_view,
                                         bool floats_group_rows)
    API_AVAILABLE(macos(26.0));

/* -[NSTableView floatsGroupRows] */
bool ns_table_view_floats_group_rows(ns_table_view *_Nonnull table_view)
    API_AVAILABLE(macos(26.0));

/* Installs the merged struct as both the table's data source and its delegate
 * (KTD6). The struct is copied; `context` is not, and has to stay valid until
 * you uninstall or the table is deallocated - ns_release does not uninstall.
 * Installing again replaces the previous struct and context; a null
 * `callbacks` uninstalls. An unset required member is reported in a debug
 * build before either slot is assigned (R9, KTD8). syntonic-owned. */
void ns_table_view_set_callbacks(
    ns_table_view *_Nonnull table_view,
    const ns_table_view_callbacks *_Nullable callbacks, void *_Nullable context)
    API_AVAILABLE(macos(26.0));

/* The upcast to ns_control: the same pointer, checked in a debug build (R4,
 * KTD9). syntonic-owned. */
ns_control *_Nonnull ns_table_view_as_control(
    ns_table_view *_Nonnull table_view) API_AVAILABLE(macos(26.0));

/* The upcast to ns_view: the same pointer, checked in a debug build (R4,
 * KTD9). This is what a scroll view takes as its document view.
 * syntonic-owned. */
ns_view *_Nonnull ns_table_view_as_view(ns_table_view *_Nonnull table_view)
    API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_TABLE_VIEW_H */

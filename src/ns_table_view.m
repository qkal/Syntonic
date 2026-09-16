/*
 * NSTableView, view-based, driven by one merged callbacks struct (R9, R17, F2,
 * F6, KTD6, KTD8).
 *
 * The struct merges NSTableViewDataSource and NSTableViewDelegate, so the one
 * shim below adopts both protocols and the one install assigns both slots.
 */

#import <AppKit/AppKit.h>

#include "ns_internal.h"
#include "syn_cell_views.h"
#include "syn_shims.h"
#include "syntonic/ns_table_column.h"
#include "syntonic/ns_table_view.h"

/* Both protocols are entirely @optional in the SDK; docs/conventions.md's
 * per-protocol table is what makes two of these required (R9, KTD8). The
 * protocol string names both halves, because that is what a report prints. */
SYN_SHIM_TABLE(syn_table_view_table, ns_table_view_callbacks,
               "NSTableViewDataSource + NSTableViewDelegate",
               SYN_SHIM_REQUIRED(ns_table_view_callbacks, number_of_rows,
                                 "numberOfRowsInTableView:"),
               SYN_SHIM_REQUIRED(ns_table_view_callbacks, cell_string,
                                 "tableView:viewForTableColumn:row:"),
               SYN_SHIM_OPTIONAL(ns_table_view_callbacks, selection_did_change,
                                 "tableViewSelectionDidChange:"));

@interface SynTableViewShim : SynShim <NSTableViewDataSource,
                                       NSTableViewDelegate>
@end

@implementation SynTableViewShim

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
  SYN_SHIM_ENTER(NSTableView, tableView);
  long (*callback)(void *, ns_table_view *) =
      SYN_SHIM_FN(ns_table_view_callbacks, number_of_rows);
  if (callback == NULL) return 0;
  long count = callback(syn_context, NS_OUT(ns_table_view, syn_sender));
  SYN_SHIM_CHECK_COUNT(syn_table_view_table, number_of_rows, count);
  return (NSInteger)count;
  SYN_SHIM_LEAVE();
}

/* KTD6: the struct supplies one borrowed string and the wrapper owns the view.
 * `column` is the column's index in the order the columns were added, and -1
 * for the group row AppKit passes no column for. */
- (NSView *)tableView:(NSTableView *)tableView
    viewForTableColumn:(NSTableColumn *)tableColumn
                   row:(NSInteger)row {
  SYN_SHIM_ENTER(NSTableView, tableView);
  const char *(*callback)(void *, ns_table_view *, long, long) =
      SYN_SHIM_FN(ns_table_view_callbacks, cell_string);
  if (callback == NULL) return nil;

  NSUInteger found = tableColumn != nil
                         ? [tableView.tableColumns indexOfObject:tableColumn]
                         : NSNotFound;
  long column = found == NSNotFound ? -1 : (long)found;
  const char *text = callback(syn_context, NS_OUT(ns_table_view, syn_sender),
                              column, (long)row);
  SYN_SHIM_CHECK_NONNULL(syn_table_view_table, cell_string, text);
  /* No icon: a list cell is a label, which is what the twins' list pane shows
   * in all three of its columns. The outline is the one with a symbol member,
   * because a sidebar row is the one with an icon. */
  return syn_cell_view(tableView, tableColumn, text, NULL);
  SYN_SHIM_LEAVE();
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification {
  SYN_SHIM_ENTER(NSTableView, notification.object);
  void (*callback)(void *, ns_table_view *, long) =
      SYN_SHIM_FN(ns_table_view_callbacks, selection_did_change);
  if (callback != NULL)
    callback(syn_context, NS_OUT(ns_table_view, syn_sender),
             (long)syn_sender.selectedRow);
  SYN_SHIM_LEAVE();
}

@end

/* Owned (+1): an initWith… returns a fresh object the caller ends with
 * ns_release (R7, KTD7). */
ns_table_view *ns_table_view_create_with_frame(CGRect frame) {
  NS_ENTER();
  return NS_OUT_OWNED(ns_table_view,
                      [[NSTableView alloc] initWithFrame:frame]);
  NS_LEAVE();
}

void ns_table_view_add_table_column(ns_table_view *table_view,
                                    ns_table_column *table_column) {
  NS_ENTER();
  [NS_IN(NSTableView, table_view)
      addTableColumn:NS_IN(NSTableColumn, table_column)];
  NS_LEAVE();
}

long ns_table_view_number_of_columns(ns_table_view *table_view) {
  NS_ENTER();
  return (long)NS_IN(NSTableView, table_view).numberOfColumns;
  NS_LEAVE();
}

long ns_table_view_number_of_rows(ns_table_view *table_view) {
  NS_ENTER();
  return (long)NS_IN(NSTableView, table_view).numberOfRows;
  NS_LEAVE();
}

void ns_table_view_reload_data(ns_table_view *table_view) {
  NS_ENTER();
  [NS_IN(NSTableView, table_view) reloadData];
  NS_LEAVE();
}

long ns_table_view_selected_row(ns_table_view *table_view) {
  NS_ENTER();
  return (long)NS_IN(NSTableView, table_view).selectedRow;
  NS_LEAVE();
}

/* R12: AppKit takes a row past the end here by raising from inside NSIndexSet,
 * which names neither the table nor this function, so the check is explicit
 * and an accessor's largest index is the count minus one. */
void ns_table_view_select_row_by_extending_selection(ns_table_view *table_view,
                                                     long row, bool extend) {
  NS_ENTER();
  NSTableView *target = NS_IN(NSTableView, table_view);
  NS_CHECK_INDEX(row, (long)target.numberOfRows - 1);
  [target selectRowIndexes:[NSIndexSet indexSetWithIndex:(NSUInteger)row]
      byExtendingSelection:extend];
  NS_LEAVE();
}

void ns_table_view_deselect_all(ns_table_view *table_view) {
  NS_ENTER();
  [NS_IN(NSTableView, table_view) deselectAll:nil];
  NS_LEAVE();
}

void ns_table_view_scroll_row_to_visible(ns_table_view *table_view, long row) {
  NS_ENTER();
  [NS_IN(NSTableView, table_view) scrollRowToVisible:(NSInteger)row];
  NS_LEAVE();
}

/* Borrowed: the table keeps the view it made, which is why this method is on
 * the borrowed-return allowlist in docs/conventions.md (R7). */
ns_view *ns_table_view_view_at_column_row_make_if_necessary(
    ns_table_view *table_view, long column, long row,
    bool make_if_necessary) {
  NS_ENTER();
  return NS_OUT(ns_view, [NS_IN(NSTableView, table_view)
                             viewAtColumn:(NSInteger)column
                                      row:(NSInteger)row
                          makeIfNecessary:make_if_necessary]);
  NS_LEAVE();
}

void ns_table_view_set_style(ns_table_view *table_view,
                             ns_table_view_style style) {
  NS_ENTER();
  NS_IN(NSTableView, table_view).style = (NSTableViewStyle)style;
  NS_LEAVE();
}

ns_table_view_style ns_table_view_get_style(ns_table_view *table_view) {
  NS_ENTER();
  return (ns_table_view_style)NS_IN(NSTableView, table_view).style;
  NS_LEAVE();
}

/* The class check is NSTableHeaderView's, not NSView's: assigning any other
 * view here would be caught by AppKit far from the call that did it (R12). */
void ns_table_view_set_header_view(ns_table_view *table_view,
                                   ns_view *header_view) {
  NS_ENTER();
  NS_IN(NSTableView, table_view).headerView =
      NS_IN_OPT(NSTableHeaderView, header_view);
  NS_LEAVE();
}

ns_view *ns_table_view_header_view(ns_table_view *table_view) {
  NS_ENTER();
  return NS_OUT(ns_view, NS_IN(NSTableView, table_view).headerView);
  NS_LEAVE();
}

void ns_table_view_set_uses_alternating_row_background_colors(
    ns_table_view *table_view, bool uses_alternating_colors) {
  NS_ENTER();
  NS_IN(NSTableView, table_view).usesAlternatingRowBackgroundColors =
      uses_alternating_colors;
  NS_LEAVE();
}

bool ns_table_view_uses_alternating_row_background_colors(
    ns_table_view *table_view) {
  NS_ENTER();
  return NS_IN(NSTableView, table_view).usesAlternatingRowBackgroundColors;
  NS_LEAVE();
}

void ns_table_view_set_allows_empty_selection(ns_table_view *table_view,
                                              bool allows_empty_selection) {
  NS_ENTER();
  NS_IN(NSTableView, table_view).allowsEmptySelection = allows_empty_selection;
  NS_LEAVE();
}

bool ns_table_view_allows_empty_selection(ns_table_view *table_view) {
  NS_ENTER();
  return NS_IN(NSTableView, table_view).allowsEmptySelection;
  NS_LEAVE();
}

void ns_table_view_set_floats_group_rows(ns_table_view *table_view,
                                         bool floats_group_rows) {
  NS_ENTER();
  NS_IN(NSTableView, table_view).floatsGroupRows = floats_group_rows;
  NS_LEAVE();
}

bool ns_table_view_floats_group_rows(ns_table_view *table_view) {
  NS_ENTER();
  return NS_IN(NSTableView, table_view).floatsGroupRows;
  NS_LEAVE();
}

/* KTD6: one install, both slots, one shim conforming to both protocols. */
void ns_table_view_set_callbacks(ns_table_view *table_view,
                                 const ns_table_view_callbacks *callbacks,
                                 void *context) {
  NS_ENTER();
  NSTableView *target = NS_IN(NSTableView, table_view);
  SYN_SHIM_INSTALL(target, SynTableViewShim, syn_table_view_table, callbacks,
                   context, ^(id shim) {
                     target.delegate = shim;
                     target.dataSource = shim;
                   });
  NS_LEAVE();
}

/* An upcast is the identity, with the debug class check NS_IN carries (KTD9). */
ns_control *ns_table_view_as_control(ns_table_view *table_view) {
  NS_ENTER();
  return NS_OUT(ns_control, NS_IN(NSTableView, table_view));
  NS_LEAVE();
}

ns_view *ns_table_view_as_view(ns_table_view *table_view) {
  NS_ENTER();
  return NS_OUT(ns_view, NS_IN(NSTableView, table_view));
  NS_LEAVE();
}

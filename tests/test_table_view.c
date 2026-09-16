/*
 * The table suite (U8): the list pane's view-based table, its columns, its
 * merged callbacks struct, its selection and the scroll view it lives in
 * (R9, R11, R12, R17, F2, F6, KTD6, KTD8, KTD17).
 *
 * The struct merges NSTableViewDataSource and NSTableViewDelegate, so one
 * install fills both AppKit slots; what this suite pins is that both halves
 * answer, that the required members are required, and that the borrowed string
 * a cell returns is copied before the callback returns.
 */

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "runner.h"
#include "support.h"
#include "syntonic/syntonic.h"

/* ---- the model the struct reads, which is all a C program needs ---- */

#define SYN_ROWS 3
#define SYN_COLUMNS 3

static const char *const syn_seed[SYN_ROWS][SYN_COLUMNS] = {
    {"Kernel rewrite", "Kal", "Kernel"},
    {"Conventions", "Ada", "Docs"},
    {"Leak gate", "Ida", "Tooling"},
};

static long syn_row_count;
static int syn_cell_calls;
static int syn_row_count_calls;
static int syn_selection_fires;
static long syn_selected_row;
static void *syn_selection_context;
static const void *syn_selection_sender;
static char syn_mutable_cell[32];

static void syn_reset(void) {
  syn_row_count = SYN_ROWS;
  syn_cell_calls = syn_row_count_calls = syn_selection_fires = 0;
  syn_selected_row = -2;
  syn_selection_context = NULL;
  syn_selection_sender = NULL;
  snprintf(syn_mutable_cell, sizeof syn_mutable_cell, "%s", "First");
}

static long syn_rows(void *context, ns_table_view *sender) {
  (void)context;
  (void)sender;
  syn_row_count_calls++;
  return syn_row_count;
}

/* KTD17: the string is borrowed for the duration of the callback, and the
 * wrapper copies it before this function's caller returns. */
static const char *syn_cell(void *context, ns_table_view *sender, long column,
                            long row) {
  (void)context;
  (void)sender;
  syn_cell_calls++;
  return syn_seed[row][column];
}

static const char *syn_mutable_cell_string(void *context,
                                           ns_table_view *sender, long column,
                                           long row) {
  (void)context;
  (void)sender;
  (void)column;
  (void)row;
  syn_cell_calls++;
  return syn_mutable_cell;
}

static void syn_on_selection(void *context, ns_table_view *sender, long row) {
  syn_selection_fires++;
  syn_selected_row = row;
  syn_selection_context = context;
  syn_selection_sender = sender;
}

/* ---- building one ---- */

static ns_table_view *syn_table_create(ns_scroll_view **out_scroll) {
  ns_scroll_view *scroll = ns_scroll_view_create_with_frame(
      CGRectMake(0, 0, 380, 200));
  ns_table_view *table = ns_table_view_create_with_frame(
      CGRectMake(0, 0, 380, 200));

  const char *const identifiers[SYN_COLUMNS] = {"title", "owner", "category"};
  const char *const titles[SYN_COLUMNS] = {"Title", "Owner", "Category"};
  for (long index = 0; index < SYN_COLUMNS; index++) {
    ns_table_column *column =
        ns_table_column_create_with_identifier(identifiers[index]);
    ns_table_column_set_title(column, titles[index]);
    ns_table_view_add_table_column(table, column);
    ns_release(column);
  }

  ns_scroll_view_set_document_view(scroll, ns_table_view_as_view(table));
  *out_scroll = scroll;
  return table;
}

/* The text the wrapper put in the cell it built: the cell view carries its
 * label as its one subview (KTD6). Owned, freed with ns_string_free. */
static char *syn_cell_text(ns_table_view *table, long column, long row) {
  ns_view *cell = ns_table_view_view_at_column_row_make_if_necessary(
      table, column, row, true);
  if (cell == NULL) return NULL;
  if (ns_view_subview_count(cell) != 1) return NULL;
  ns_view *label = ns_view_subview_at_index(cell, 0);
  return ns_control_copy_string_value(
      ns_text_field_as_control((ns_text_field *)(void *)label));
}

/* ---- the table over a C array (R17, F2, KTD6) ---- */

SYN_TEST(a_table_over_three_rows_reports_them_and_shows_the_first_cell) {
  syn_test_bootstrap();
  syn_reset();
  int context = 7;

  ns_scroll_view *scroll = NULL;
  ns_table_view *table = syn_table_create(&scroll);
  SYN_ASSERT_STR_EQ(syn_test_class_name(table), "NSTableView");
  SYN_ASSERT_STR_EQ(syn_test_class_name(scroll), "NSScrollView");
  SYN_ASSERT_MSG(ns_table_view_number_of_columns(table) == SYN_COLUMNS,
                 "the table has %ld columns, not 3",
                 ns_table_view_number_of_columns(table));

  /* The scroll view is what a pane holds; the table is its document view. */
  SYN_ASSERT_MSG(ns_scroll_view_document_view(scroll) ==
                     ns_table_view_as_view(table),
                 "the scroll view does not report the table as its document");

  ns_table_view_callbacks callbacks = {.number_of_rows = syn_rows,
                                       .cell_string = syn_cell,
                                       .selection_did_change =
                                           syn_on_selection};
  ns_table_view_set_callbacks(table, &callbacks, &context);
  SYN_ASSERT_SHIMS(1);
  /* One shim, both slots: the merged struct answers the data source's count
   * selector and the delegate's cell selector (KTD6). */
  SYN_ASSERT_MSG(syn_test_delegate_responds(table, "numberOfRowsInTableView:"),
                 "the data source half of the merged struct is not installed");
  SYN_ASSERT_MSG(
      syn_test_delegate_responds(table, "tableView:viewForTableColumn:row:"),
      "the delegate half of the merged struct is not installed");

  ns_table_view_reload_data(table);
  SYN_ASSERT_MSG(ns_table_view_number_of_rows(table) == 3,
                 "the table reported %ld rows, not 3",
                 ns_table_view_number_of_rows(table));

  char *title = syn_cell_text(table, 0, 0);
  SYN_ASSERT_STR_EQ(title, "Kernel rewrite");
  ns_string_free(title);
  char *owner = syn_cell_text(table, 1, 0);
  SYN_ASSERT_STR_EQ(owner, "Kal");
  ns_string_free(owner);

  /* Reuse: asking for the same cell again does not build a second view. */
  ns_view *first = ns_table_view_view_at_column_row_make_if_necessary(
      table, 0, 0, true);
  ns_view *again = ns_table_view_view_at_column_row_make_if_necessary(
      table, 0, 0, true);
  SYN_ASSERT_MSG(first == again, "the table built a second view for one cell");

  ns_table_view_set_callbacks(table, NULL, NULL);
  SYN_ASSERT_MSG(!syn_test_has_delegate(table),
                 "a null struct left a delegate installed");
  SYN_ASSERT_SHIMS(0);

  ns_release(table);
  ns_release(scroll);
}

/* ---- the columns (R7, R11) ---- */

SYN_TEST(a_columns_identifier_title_and_widths_round_trip) {
  syn_test_bootstrap();
  ns_table_column *column = ns_table_column_create_with_identifier("title");
  SYN_ASSERT_STR_EQ(syn_test_class_name(column), "NSTableColumn");

  char *identifier = ns_table_column_copy_identifier(column);
  SYN_ASSERT_STR_EQ(identifier, "title");
  ns_string_free(identifier);

  ns_table_column_set_title(column, "Title");
  char *first = ns_table_column_copy_title(column);
  char *second = ns_table_column_copy_title(column);
  SYN_ASSERT_STR_EQ(first, "Title");
  SYN_ASSERT_MSG(first != second, "two copies of the title came back as one");
  ns_string_free(first);
  SYN_ASSERT_STR_EQ(second, "Title"); /* the other copy is untouched */
  ns_string_free(second);

  ns_table_column_set_min_width(column, 60);
  ns_table_column_set_width(column, 170);
  SYN_ASSERT_MSG(ns_table_column_width(column) == 170,
                 "the width read back as %g", ns_table_column_width(column));
  SYN_ASSERT_MSG(ns_table_column_min_width(column) == 60,
                 "the minimum width read back as %g",
                 ns_table_column_min_width(column));

  ns_release(column);
}

/* ---- the properties the two panes set (R15, R17) ---- */

SYN_TEST(a_tables_style_header_and_selection_flags_round_trip) {
  syn_test_bootstrap();
  ns_scroll_view *scroll = NULL;
  ns_table_view *table = syn_table_create(&scroll);

  SYN_ASSERT_MSG(ns_table_view_get_style(table) ==
                     NS_TABLE_VIEW_STYLE_AUTOMATIC,
                 "a new table did not start on the automatic style");
  ns_table_view_set_style(table, NS_TABLE_VIEW_STYLE_INSET);
  SYN_ASSERT(ns_table_view_get_style(table) == NS_TABLE_VIEW_STYLE_INSET);

  /* A table makes its own header row; a sidebar removes it and a list keeps
   * it, so the getter has to hand back something worth putting back. */
  ns_view *header = ns_table_view_header_view(table);
  SYN_ASSERT_MSG(header != NULL, "a new table has no header view to remove");
  ns_retain(header);
  ns_table_view_set_header_view(table, NULL);
  SYN_ASSERT_MSG(ns_table_view_header_view(table) == NULL,
                 "null did not remove the header view");
  ns_table_view_set_header_view(table, header);
  SYN_ASSERT_MSG(ns_table_view_header_view(table) == header,
                 "the header view did not go back");
  ns_release(header);

  ns_table_view_set_uses_alternating_row_background_colors(table, true);
  SYN_ASSERT(ns_table_view_uses_alternating_row_background_colors(table));

  SYN_ASSERT_MSG(ns_table_view_allows_empty_selection(table),
                 "a new table did not allow an empty selection");
  ns_table_view_set_allows_empty_selection(table, false);
  SYN_ASSERT(!ns_table_view_allows_empty_selection(table));
  ns_table_view_set_allows_empty_selection(table, true);

  SYN_ASSERT_MSG(ns_table_view_floats_group_rows(table),
                 "a new table did not float its group rows");
  ns_table_view_set_floats_group_rows(table, false);
  SYN_ASSERT(!ns_table_view_floats_group_rows(table));

  ns_release(table);
  ns_release(scroll);
}

/* ---- the selection (F6, R11) ---- */

SYN_TEST(selecting_a_row_fires_selection_did_change_with_the_table_and_the_row) {
  syn_test_bootstrap();
  syn_reset();
  int context = 11;

  ns_scroll_view *scroll = NULL;
  ns_table_view *table = syn_table_create(&scroll);
  ns_table_view_callbacks callbacks = {.number_of_rows = syn_rows,
                                       .cell_string = syn_cell,
                                       .selection_did_change =
                                           syn_on_selection};
  ns_table_view_set_callbacks(table, &callbacks, &context);
  ns_table_view_reload_data(table);

  SYN_ASSERT_MSG(ns_table_view_selected_row(table) == -1,
                 "a freshly reloaded table reported a selection");

  ns_table_view_select_row_by_extending_selection(table, 2, false);
  SYN_WAIT_FOR(syn_selection_fires == 1, 1000);
  SYN_ASSERT_MSG(syn_selected_row == 2, "the callback saw row %ld, not 2",
                 syn_selected_row);
  SYN_ASSERT_MSG(syn_selection_sender == table,
                 "the sender was not the table that fired");
  SYN_ASSERT_MSG(syn_selection_context == &context,
                 "the context did not arrive");
  SYN_ASSERT_MSG(ns_table_view_selected_row(table) == 2,
                 "the table reported row %ld as selected, not 2",
                 ns_table_view_selected_row(table));

  /* Clearing the selection is the other half of F6. */
  ns_table_view_deselect_all(table);
  SYN_WAIT_FOR(syn_selection_fires == 2, 1000);
  SYN_ASSERT_MSG(syn_selected_row == -1,
                 "a cleared selection reported row %ld, not -1",
                 syn_selected_row);

  ns_table_view_set_callbacks(table, NULL, NULL);
  ns_release(table);
  ns_release(scroll);
}

/* R9: an unset optional member is a method the delegate does not implement,
 * and a table with no rows asks for no cells at all. */
SYN_TEST(a_table_with_no_rows_never_asks_for_a_cell_and_reports_no_selection) {
  syn_test_bootstrap();
  syn_reset();
  syn_row_count = 0;

  ns_scroll_view *scroll = NULL;
  ns_table_view *table = syn_table_create(&scroll);
  ns_table_view_callbacks without_selection = {.number_of_rows = syn_rows,
                                               .cell_string = syn_cell};
  ns_table_view_set_callbacks(table, &without_selection, NULL);
  SYN_ASSERT_MSG(
      !syn_test_delegate_responds(table, "tableViewSelectionDidChange:"),
      "an unset optional member is reported as implemented");

  ns_table_view_reload_data(table);
  syn_test_spin(50);
  SYN_ASSERT_MSG(ns_table_view_number_of_rows(table) == 0,
                 "the table reported %ld rows, not 0",
                 ns_table_view_number_of_rows(table));
  SYN_ASSERT_MSG(syn_cell_calls == 0, "cell_string was asked %d time(s)",
                 syn_cell_calls);
  SYN_ASSERT_MSG(ns_table_view_selected_row(table) == -1,
                 "an empty table reported a selected row");

  ns_table_view_set_callbacks(table, NULL, NULL);
  ns_release(table);
  ns_release(scroll);
}

/* F6: Delete removes the row, the table reloads with one fewer, and the
 * selection the deleted row held is gone.
 *
 * AppKit posts no selection notification for the reload itself - it drops the
 * selection silently - so the program clears the selection first and that is
 * what fires. Both halves are pinned here, because the first is the one a C
 * author would otherwise wait for forever. */
SYN_TEST(deleting_the_selected_row_clears_the_selection_and_fires_once) {
  syn_test_bootstrap();
  syn_reset();
  int context = 4;

  ns_scroll_view *scroll = NULL;
  ns_table_view *table = syn_table_create(&scroll);
  ns_table_view_callbacks callbacks = {.number_of_rows = syn_rows,
                                       .cell_string = syn_cell,
                                       .selection_did_change =
                                           syn_on_selection};
  ns_table_view_set_callbacks(table, &callbacks, &context);
  ns_table_view_reload_data(table);

  ns_table_view_select_row_by_extending_selection(table, 2, false);
  SYN_WAIT_FOR(syn_selection_fires == 1, 1000);

  /* Delete: clear the selection, drop the row from the C model, reload. */
  ns_table_view_deselect_all(table);
  SYN_WAIT_FOR(syn_selection_fires == 2, 1000);
  SYN_ASSERT_MSG(syn_selected_row == -1,
                 "the callback saw row %ld, not -1 for a cleared selection",
                 syn_selected_row);

  syn_row_count = 2;
  ns_table_view_reload_data(table);
  syn_test_spin(50);
  SYN_ASSERT_MSG(ns_table_view_number_of_rows(table) == 2,
                 "the table reported %ld rows, not 2",
                 ns_table_view_number_of_rows(table));
  SYN_ASSERT_MSG(ns_table_view_selected_row(table) == -1,
                 "the table still reports row %ld as selected",
                 ns_table_view_selected_row(table));

  /* The other order - reload first - leaves the same empty selection, and
   * AppKit posts nothing for it. */
  ns_table_view_select_row_by_extending_selection(table, 1, false);
  SYN_WAIT_FOR(syn_selection_fires == 3, 1000);
  syn_row_count = 1;
  ns_table_view_reload_data(table);
  syn_test_spin(50);
  SYN_ASSERT_MSG(ns_table_view_selected_row(table) == -1,
                 "a reload past the selected row left row %ld selected",
                 ns_table_view_selected_row(table));
  SYN_ASSERT_MSG(syn_selection_fires == 3,
                 "the reload posted a selection change of its own (%d fires)",
                 syn_selection_fires);

  ns_table_view_set_callbacks(table, NULL, NULL);
  ns_release(table);
  ns_release(scroll);
}

/* KTD17: the borrowed string is copied into the cell at once, so the caller
 * may reuse its buffer the moment the callback returns. */
SYN_TEST(a_cell_string_is_copied_at_once_so_a_later_edit_does_not_show) {
  syn_test_bootstrap();
  syn_reset();
  syn_row_count = 1;

  ns_scroll_view *scroll = NULL;
  ns_table_view *table = syn_table_create(&scroll);
  ns_table_view_callbacks callbacks = {.number_of_rows = syn_rows,
                                       .cell_string = syn_mutable_cell_string};
  ns_table_view_set_callbacks(table, &callbacks, NULL);
  ns_table_view_reload_data(table);

  char *shown = syn_cell_text(table, 0, 0);
  SYN_ASSERT_STR_EQ(shown, "First");
  ns_string_free(shown);

  /* The C side reuses the buffer the callback returned a pointer into. */
  snprintf(syn_mutable_cell, sizeof syn_mutable_cell, "%s", "Overwritten");
  char *after = syn_cell_text(table, 0, 0);
  SYN_ASSERT_MSG(after != NULL, "the cell view went away");
  SYN_ASSERT_STR_EQ(after, "First");
  ns_string_free(after);

  /* A reload asks again, and then the new text is what shows. */
  ns_table_view_reload_data(table);
  char *reloaded = syn_cell_text(table, 0, 0);
  SYN_ASSERT_STR_EQ(reloaded, "Overwritten");
  ns_string_free(reloaded);

  ns_table_view_set_callbacks(table, NULL, NULL);
  ns_release(table);
  ns_release(scroll);
}

/* R16: selecting does not scroll, so a row appended past the bottom needs
 * ns_table_view_scroll_row_to_visible to be revealed. */
SYN_TEST(scrolling_a_row_into_view_moves_the_scroll_views_visible_rect) {
  syn_test_bootstrap();
  syn_reset();
  syn_row_count = 200;

  ns_scroll_view *scroll = NULL;
  ns_table_view *table = syn_table_create(&scroll);
  ns_table_view_callbacks callbacks = {.number_of_rows = syn_rows,
                                       .cell_string = syn_mutable_cell_string};
  ns_table_view_set_callbacks(table, &callbacks, NULL);
  ns_table_view_reload_data(table);

  /* The table is taller than the clip now, and the last row is below it. */
  CGRect before = ns_scroll_view_document_visible_rect(scroll);
  CGRect table_frame = ns_view_frame(ns_table_view_as_view(table));
  SYN_ASSERT_MSG(table_frame.size.height > before.size.height,
                 "200 rows did not make the table taller than its clip");

  /* Selecting the last row leaves it out of sight; scrolling reveals it. */
  ns_table_view_select_row_by_extending_selection(table, 199, false);
  CGRect selected = ns_scroll_view_document_visible_rect(scroll);
  SYN_ASSERT_MSG(selected.origin.y == before.origin.y,
                 "selecting a row scrolled the table to y=%g",
                 selected.origin.y);

  ns_table_view_scroll_row_to_visible(table, 199);
  CGRect after = ns_scroll_view_document_visible_rect(scroll);
  SYN_ASSERT_MSG(after.origin.y > before.origin.y,
                 "scrolling to row 199 left the visible rect at y=%g",
                 after.origin.y);
  SYN_ASSERT_MSG(after.origin.y + after.size.height >=
                     table_frame.size.height - 1,
                 "scrolling to the last row stopped at y=%g", after.origin.y);

  ns_table_view_scroll_row_to_visible(table, 0);
  CGRect back = ns_scroll_view_document_visible_rect(scroll);
  SYN_ASSERT_MSG(back.origin.y == before.origin.y,
                 "scrolling back left y=%g, not %g", back.origin.y,
                 before.origin.y);

  ns_table_view_set_callbacks(table, NULL, NULL);
  ns_release(table);
  ns_release(scroll);
}

/* ---- re-installing from inside a callback (KTD8) ---- */

static int syn_second_struct_fires;
static bool syn_reinstalled;

static void syn_on_selection_second(void *context, ns_table_view *sender,
                                    long row) {
  (void)context;
  (void)sender;
  (void)row;
  syn_second_struct_fires++;
}

/* A callback may replace its own struct while AppKit's dispatch is still on
 * the stack: the shim holds itself and the sender until it returns (KTD8). */
static void syn_on_selection_reinstalling(void *context, ns_table_view *sender,
                                          long row) {
  syn_selection_fires++;
  syn_selected_row = row;
  if (syn_reinstalled) return;
  syn_reinstalled = true;
  ns_table_view_callbacks next = {.number_of_rows = syn_rows,
                                  .cell_string = syn_cell,
                                  .selection_did_change =
                                      syn_on_selection_second};
  ns_table_view_set_callbacks(sender, &next, context);
}

SYN_TEST(reinstalling_the_struct_from_inside_the_callback_replaces_it) {
  syn_test_bootstrap();
  syn_reset();
  syn_second_struct_fires = 0;
  syn_reinstalled = false;

  ns_scroll_view *scroll = NULL;
  ns_table_view *table = syn_table_create(&scroll);
  ns_table_view_callbacks callbacks = {.number_of_rows = syn_rows,
                                       .cell_string = syn_cell,
                                       .selection_did_change =
                                           syn_on_selection_reinstalling};
  ns_table_view_set_callbacks(table, &callbacks, NULL);
  ns_table_view_reload_data(table);

  ns_table_view_select_row_by_extending_selection(table, 1, false);
  SYN_WAIT_FOR(syn_selection_fires == 1, 1000);
  SYN_ASSERT_SHIMS(1); /* the replacement released the shim it replaced */

  /* From here the new member is the one that fires. */
  ns_table_view_select_row_by_extending_selection(table, 2, false);
  SYN_WAIT_FOR(syn_second_struct_fires == 1, 1000);
  SYN_ASSERT_MSG(syn_selection_fires == 1,
                 "the replaced member fired %d time(s)", syn_selection_fires);

  ns_table_view_set_callbacks(table, NULL, NULL);
  SYN_ASSERT_SHIMS(0);
  ns_release(table);
  ns_release(scroll);
}

/* ---- teardown (R7, KTD12) ---- */

SYN_TEST(tearing_down_a_window_holding_a_table_releases_clean) {
  syn_test_bootstrap();
  syn_reset();
  int context = 2;

  ns_window *window = ns_window_create_with_content_rect_style_mask_backing_defer(
      CGRectMake(0, 0, 400, 300),
      (ns_window_style_mask)(NS_WINDOW_STYLE_MASK_TITLED |
                             NS_WINDOW_STYLE_MASK_CLOSABLE),
      NS_BACKING_STORE_BUFFERED, false);
  ns_view_controller *controller = ns_view_controller_create();
  ns_view *pane = ns_view_create_with_frame(CGRectMake(0, 0, 400, 300));

  ns_scroll_view *scroll = NULL;
  ns_table_view *table = syn_table_create(&scroll);
  ns_table_view_callbacks callbacks = {.number_of_rows = syn_rows,
                                       .cell_string = syn_cell,
                                       .selection_did_change =
                                           syn_on_selection};
  ns_table_view_set_callbacks(table, &callbacks, &context);
  ns_table_view_reload_data(table);
  SYN_ASSERT_SHIMS(1);

  ns_view_add_subview(pane, ns_scroll_view_as_view(scroll));
  ns_view_controller_set_view(controller, pane);
  ns_window_set_content_view_controller(window, controller);

  ns_release(table);
  ns_release(scroll);
  ns_release(pane);
  ns_release(controller);
  ns_window_close(window);
  ns_release(window); /* the window's tree is the only holder left */
  SYN_ASSERT_SHIMS(0);
}

/* ---- the cases that abort, each run in a child (R9, R10, R12, KTD4) ---- */

SYN_ABORT_CASE(table_installed_without_number_of_rows) {
  syn_test_bootstrap();
  syn_reset();
  ns_scroll_view *scroll = NULL;
  ns_table_view *table = syn_table_create(&scroll);
  ns_table_view_callbacks missing = {.cell_string = syn_cell};
  ns_table_view_set_callbacks(table, &missing, NULL);
}

/* R9, AE1: the report names the member and the struct, before AppKit is ever
 * asked for a row. */
SYN_TEST(installing_without_a_required_member_names_it_and_the_protocol) {
  SYN_ASSERT_ABORTS("table_installed_without_number_of_rows",
                    "number_of_rows");
  SYN_ASSERT_ABORTS("table_installed_without_number_of_rows",
                    "NSTableViewDataSource + NSTableViewDelegate");
  SYN_ASSERT_ABORTS("table_installed_without_number_of_rows",
                    "ns_table_view_set_callbacks");
}

static const char *syn_null_cell(void *context, ns_table_view *sender,
                                 long column, long row) {
  (void)context;
  (void)sender;
  (void)column;
  (void)row;
  return NULL;
}

SYN_ABORT_CASE(cell_string_returning_null) {
  syn_test_bootstrap();
  syn_reset();
  ns_scroll_view *scroll = NULL;
  ns_table_view *table = syn_table_create(&scroll);
  ns_table_view_callbacks callbacks = {.number_of_rows = syn_rows,
                                       .cell_string = syn_null_cell};
  ns_table_view_set_callbacks(table, &callbacks, NULL);
  ns_table_view_reload_data(table);
  ns_table_view_view_at_column_row_make_if_necessary(table, 0, 0, true);
}

/* R11, KTD8: malformed bytes from a callback are reported, not dropped for an
 * empty cell, exactly as they are at a wrapper parameter. */
static const char *syn_malformed_cell(void *context, ns_table_view *sender,
                                      long column, long row) {
  (void)context;
  (void)sender;
  (void)column;
  (void)row;
  return "\xff\xfe not utf-8";
}

SYN_ABORT_CASE(cell_string_returning_malformed_utf8) {
  syn_test_bootstrap();
  syn_reset();
  ns_scroll_view *scroll = NULL;
  ns_table_view *table = syn_table_create(&scroll);
  ns_table_view_callbacks callbacks = {.number_of_rows = syn_rows,
                                       .cell_string = syn_malformed_cell};
  ns_table_view_set_callbacks(table, &callbacks, NULL);
  ns_table_view_reload_data(table);
  ns_table_view_view_at_column_row_make_if_necessary(table, 0, 0, true);
}

static long syn_negative_rows(void *context, ns_table_view *sender) {
  (void)context;
  (void)sender;
  return -3;
}

SYN_ABORT_CASE(number_of_rows_returning_a_negative_count) {
  syn_test_bootstrap();
  syn_reset();
  ns_scroll_view *scroll = NULL;
  ns_table_view *table = syn_table_create(&scroll);
  ns_table_view_callbacks callbacks = {.number_of_rows = syn_negative_rows,
                                       .cell_string = syn_cell};
  ns_table_view_set_callbacks(table, &callbacks, NULL);
  ns_table_view_reload_data(table);
}

/* KTD8: what a callback hands back is checked before AppKit sees it. */
SYN_TEST(a_bad_value_from_a_callback_names_the_member) {
  SYN_ASSERT_ABORTS("cell_string_returning_null", "cell_string");
  SYN_ASSERT_ABORTS("cell_string_returning_null", "returned null");
  SYN_ASSERT_ABORTS("number_of_rows_returning_a_negative_count",
                    "number_of_rows");
  SYN_ASSERT_ABORTS("number_of_rows_returning_a_negative_count",
                    "the count -3");
  SYN_ASSERT_ABORTS("cell_string_returning_malformed_utf8", "cell_string");
  SYN_ASSERT_ABORTS("cell_string_returning_malformed_utf8", "not valid UTF-8");
}

SYN_ABORT_CASE(row_selected_at_the_row_count) {
  syn_test_bootstrap();
  syn_reset();
  ns_scroll_view *scroll = NULL;
  ns_table_view *table = syn_table_create(&scroll);
  ns_table_view_callbacks callbacks = {.number_of_rows = syn_rows,
                                       .cell_string = syn_cell};
  ns_table_view_set_callbacks(table, &callbacks, NULL);
  ns_table_view_reload_data(table);
  ns_table_view_select_row_by_extending_selection(table, 3, false);
}

SYN_TEST(selecting_outside_the_rows_names_the_function_and_the_range) {
  SYN_ASSERT_ABORTS("row_selected_at_the_row_count",
                    "ns_table_view_select_row_by_extending_selection");
  SYN_ASSERT_ABORTS("row_selected_at_the_row_count", "index 3 is out of range");
  SYN_ASSERT_ABORTS("row_selected_at_the_row_count", "accepts 0 through 2");
}

static void *syn_create_a_table_off_the_main_thread(void *unused) {
  (void)unused;
  ns_table_view_create_with_frame(CGRectMake(0, 0, 10, 10));
  return NULL;
}

SYN_ABORT_CASE(table_created_off_the_main_thread) {
  syn_test_bootstrap();
  pthread_t thread;
  if (pthread_create(&thread, NULL, syn_create_a_table_off_the_main_thread,
                     NULL) != 0) {
    fprintf(stderr, "could not start the background thread\n");
    return;
  }
  pthread_join(thread, NULL);
}

SYN_TEST(creating_a_table_off_the_main_thread_names_the_function) {
  SYN_ASSERT_ABORTS("table_created_off_the_main_thread",
                    "ns_table_view_create_with_frame");
  SYN_ASSERT_ABORTS("table_created_off_the_main_thread", "main thread only");
}

/* ---- the two properties the list pane sets (U12 gaps, R17) ----
 *
 * The twin sets the column autoresizing style and the alternating row
 * background colours. Only one of the two is a change: the SDK's default
 * autoresizing style is already last-column-only, which is what the twin asks
 * for, so that line moves nothing. The alternating colours default to off and
 * turning them on is what the reviewer sees. */

SYN_TEST(the_column_autoresizing_style_and_alternating_colours_round_trip) {
  syn_test_bootstrap();
  ns_scroll_view *scroll = NULL;
  ns_table_view *table = syn_table_create(&scroll);

  SYN_ASSERT_MSG(ns_table_view_get_column_autoresizing_style(table) ==
                     NS_TABLE_VIEW_LAST_COLUMN_ONLY_AUTORESIZING_STYLE,
                 "a new table does not start on the last-column-only "
                 "autoresizing style, so the twin's line is not a no-op after "
                 "all");
  SYN_ASSERT_MSG(!ns_table_view_uses_alternating_row_background_colors(table),
                 "a new table already draws alternating row backgrounds");

  /* Every value of the enum reaches AppKit and comes back, which is what pins
   * the C enum's numbers to the SDK's (R5). */
  const ns_table_view_column_autoresizing_style styles[] = {
      NS_TABLE_VIEW_NO_COLUMN_AUTORESIZING,
      NS_TABLE_VIEW_UNIFORM_COLUMN_AUTORESIZING_STYLE,
      NS_TABLE_VIEW_SEQUENTIAL_COLUMN_AUTORESIZING_STYLE,
      NS_TABLE_VIEW_REVERSE_SEQUENTIAL_COLUMN_AUTORESIZING_STYLE,
      NS_TABLE_VIEW_LAST_COLUMN_ONLY_AUTORESIZING_STYLE,
      NS_TABLE_VIEW_FIRST_COLUMN_ONLY_AUTORESIZING_STYLE};
  for (size_t i = 0; i < sizeof(styles) / sizeof(*styles); i++) {
    ns_table_view_set_column_autoresizing_style(table, styles[i]);
    SYN_ASSERT_MSG(ns_table_view_get_column_autoresizing_style(table) ==
                       styles[i],
                   "autoresizing style %d did not read back",
                   (int)styles[i]);
  }

  ns_table_view_set_column_autoresizing_style(
      table, NS_TABLE_VIEW_LAST_COLUMN_ONLY_AUTORESIZING_STYLE);
  ns_table_view_set_uses_alternating_row_background_colors(table, true);
  SYN_ASSERT_MSG(ns_table_view_uses_alternating_row_background_colors(table),
                 "alternating row backgrounds did not read back on");
  ns_table_view_set_uses_alternating_row_background_colors(table, false);
  SYN_ASSERT_MSG(!ns_table_view_uses_alternating_row_background_colors(table),
                 "alternating row backgrounds did not read back off");

  ns_release(table);
  ns_release(scroll);
}

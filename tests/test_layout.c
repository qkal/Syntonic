/*
 * The layout suite (U15): the stack view, the grid view, edge pinning, and the
 * three riders the twins need on top of them - fonts and colours, the view
 * members a layout report is made of, and the Tab chain (R17, R19, KTD3,
 * KTD7, KTD17, KTD18).
 *
 * Off-screen throughout (R21): the application has the prohibited activation
 * policy and no window is ordered front. Auto Layout still runs - a pinned
 * view takes its superview's bounds within one turn of the run loop - which is
 * what SYN_WAIT_FOR waits on rather than a fixed sleep.
 */

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "runner.h"
#include "support.h"
#include "syntonic/syntonic.h"

enum { SYN_STANDARD_MASK = NS_WINDOW_STYLE_MASK_TITLED |
                           NS_WINDOW_STYLE_MASK_CLOSABLE |
                           NS_WINDOW_STYLE_MASK_RESIZABLE };

static ns_window *syn_window_create(CGFloat width, CGFloat height) {
  return ns_window_create_with_content_rect_style_mask_backing_defer(
      CGRectMake(0, 0, width, height), (ns_window_style_mask)SYN_STANDARD_MASK,
      NS_BACKING_STORE_BUFFERED, false);
}

static ns_view *syn_view(CGFloat width, CGFloat height) {
  return ns_view_create_with_frame(CGRectMake(0, 0, width, height));
}

/* True once the pinned view has taken the size its constraints ask for. The
 * layout pass runs on the run loop, so this is what SYN_WAIT_FOR spins on. */
static bool syn_bounds_are(ns_view *view, CGFloat width, CGFloat height) {
  CGRect bounds = ns_view_bounds(view);
  return bounds.size.width == width && bounds.size.height == height;
}

/* ---- the stack view (KTD3, KTD7, KTD17) ---- */

SYN_TEST(a_stack_reports_its_arranged_subviews_in_insertion_order) {
  syn_test_bootstrap();
  ns_view *first = syn_view(40, 20);
  ns_view *second = syn_view(40, 20);
  ns_view *seed[] = {first, second};

  ns_stack_view *stack = ns_stack_view_create_with_views(seed, 2);
  SYN_ASSERT_STR_EQ(syn_test_class_name(stack), "NSStackView");
  SYN_ASSERT_MSG(ns_stack_view_arranged_subview_count(stack) == 2,
                 "the stack started with %ld arranged subviews",
                 ns_stack_view_arranged_subview_count(stack));

  ns_view *third = syn_view(40, 20);
  ns_stack_view_add_arranged_subview(stack, third);

  ns_view *expected[] = {first, second, third};
  SYN_ASSERT_MSG(ns_stack_view_arranged_subview_count(stack) == 3,
                 "the count did not follow the insertion: %ld",
                 ns_stack_view_arranged_subview_count(stack));
  for (long index = 0; index < 3; index++)
    SYN_ASSERT_MSG(
        ns_stack_view_arranged_subview_at_index(stack, index) == expected[index],
        "arranged subview %ld is not the one added at that position", index);

  /* The element is borrowed from the stack, which holds it, so the caller's
   * own references can go (R7, KTD17). */
  ns_release(first);
  ns_release(second);
  ns_release(third);
  SYN_ASSERT_STR_EQ(
      syn_test_class_name(ns_stack_view_arranged_subview_at_index(stack, 0)),
      "NSView");

  ns_release(stack);
}

SYN_TEST(the_stacks_orientation_spacing_and_edge_insets_round_trip) {
  syn_test_bootstrap();
  ns_stack_view *stack = ns_stack_view_create_with_views(NULL, 0);
  SYN_ASSERT_MSG(ns_stack_view_arranged_subview_count(stack) == 0,
                 "an empty stack reported %ld arranged subviews",
                 ns_stack_view_arranged_subview_count(stack));

  /* AppKit's own default for this constructor. */
  SYN_ASSERT_MSG(ns_stack_view_orientation(stack) ==
                     NS_USER_INTERFACE_LAYOUT_ORIENTATION_HORIZONTAL,
                 "a new stack was not horizontal");
  ns_stack_view_set_orientation(stack,
                                NS_USER_INTERFACE_LAYOUT_ORIENTATION_VERTICAL);
  SYN_ASSERT(ns_stack_view_orientation(stack) ==
             NS_USER_INTERFACE_LAYOUT_ORIENTATION_VERTICAL);

  ns_stack_view_set_spacing(stack, 12);
  SYN_ASSERT_MSG(ns_stack_view_spacing(stack) == 12,
                 "the spacing read back as %g", ns_stack_view_spacing(stack));

  ns_stack_view_set_edge_insets(
      stack, (ns_edge_insets){.top = 1, .left = 2, .bottom = 3, .right = 4});
  ns_edge_insets insets = ns_stack_view_edge_insets(stack);
  SYN_ASSERT_MSG(insets.top == 1 && insets.left == 2 && insets.bottom == 3 &&
                     insets.right == 4,
                 "the insets read back as %g,%g,%g,%g", insets.top, insets.left,
                 insets.bottom, insets.right);

  SYN_ASSERT((void *)ns_stack_view_as_view(stack) == (void *)stack);
  SYN_ASSERT_STR_EQ(syn_test_class_name(ns_stack_view_as_view(stack)),
                    "NSStackView");

  ns_release(stack);
}

/* ---- the grid view and its two-dimensional input (KTD17) ---- */

SYN_TEST(a_grid_of_two_rows_of_two_views_reports_two_rows_and_two_columns) {
  syn_test_bootstrap();
  ns_text_field *label = ns_text_field_create_label_with_string("Title:");
  ns_text_field *field = ns_text_field_create_with_string("Kernel");
  ns_text_field *second_label =
      ns_text_field_create_label_with_string("Owner:");
  ns_text_field *second_field = ns_text_field_create_with_string("Kal");

  ns_view *top[] = {ns_text_field_as_view(label), ns_text_field_as_view(field)};
  ns_view *bottom[] = {ns_text_field_as_view(second_label),
                       ns_text_field_as_view(second_field)};
  ns_grid_view_row rows[] = {
      {.views = top, .count = 2},
      {.views = bottom, .count = 2},
  };

  ns_grid_view *grid = ns_grid_view_create_with_views(rows, 2);
  SYN_ASSERT_STR_EQ(syn_test_class_name(grid), "NSGridView");
  SYN_ASSERT_MSG(ns_grid_view_number_of_rows(grid) == 2,
                 "the grid reported %ld rows",
                 ns_grid_view_number_of_rows(grid));
  SYN_ASSERT_MSG(ns_grid_view_number_of_columns(grid) == 2,
                 "the grid reported %ld columns",
                 ns_grid_view_number_of_columns(grid));

  ns_grid_view_set_row_spacing(grid, 10);
  ns_grid_view_set_column_spacing(grid, 12);
  SYN_ASSERT_MSG(ns_grid_view_row_spacing(grid) == 10,
                 "the row spacing read back as %g",
                 ns_grid_view_row_spacing(grid));
  SYN_ASSERT_MSG(ns_grid_view_column_spacing(grid) == 12,
                 "the column spacing read back as %g",
                 ns_grid_view_column_spacing(grid));

  SYN_ASSERT((void *)ns_grid_view_as_view(grid) == (void *)grid);

  /* The grid holds the views it was built from, so the caller's references
   * can go and the views stay in the tree (R7, AE2). */
  ns_release(label);
  ns_release(field);
  ns_release(second_label);
  ns_release(second_field);
  SYN_ASSERT_MSG(ns_view_superview(top[0]) != NULL,
                 "a view handed to the grid has no superview");

  ns_release(grid);
}

SYN_TEST(an_empty_grid_reports_no_rows_and_a_short_row_is_allowed) {
  syn_test_bootstrap();
  ns_grid_view *empty = ns_grid_view_create_with_views(NULL, 0);
  SYN_ASSERT_MSG(ns_grid_view_number_of_rows(empty) == 0 &&
                     ns_grid_view_number_of_columns(empty) == 0,
                 "an empty grid reported %ldx%ld",
                 ns_grid_view_number_of_rows(empty),
                 ns_grid_view_number_of_columns(empty));
  ns_release(empty);

  /* The grid is as wide as its widest row; the short row's missing cells are
   * empty. This is the shape rule's own edge (KTD17). */
  ns_view *wide[] = {syn_view(20, 20), syn_view(20, 20), syn_view(20, 20)};
  ns_view *narrow[] = {syn_view(20, 20)};
  ns_grid_view_row rows[] = {
      {.views = wide, .count = 3},
      {.views = narrow, .count = 1},
  };
  ns_grid_view *grid = ns_grid_view_create_with_views(rows, 2);
  SYN_ASSERT_MSG(ns_grid_view_number_of_rows(grid) == 2 &&
                     ns_grid_view_number_of_columns(grid) == 3,
                 "the ragged grid reported %ldx%ld",
                 ns_grid_view_number_of_rows(grid),
                 ns_grid_view_number_of_columns(grid));

  for (long index = 0; index < 3; index++) ns_release(wide[index]);
  ns_release(narrow[0]);
  ns_release(grid);
}

/* ---- the grid's rows, columns and cells (KTD3, R7, R17) ---- */

/* The detail pane's own shape: an empty grid of two columns, filled a row at a
 * time, with a merged header row and an empty first column under it. */
SYN_TEST(a_grid_made_by_columns_and_rows_counts_the_rows_added_to_it) {
  syn_test_bootstrap();
  ns_grid_view *grid = ns_grid_view_create_with_number_of_columns_rows(2, 0);
  SYN_ASSERT_STR_EQ(syn_test_class_name(grid), "NSGridView");
  SYN_ASSERT_MSG(ns_grid_view_number_of_rows(grid) == 0 &&
                     ns_grid_view_number_of_columns(grid) == 2,
                 "the empty grid reported %ldx%ld",
                 ns_grid_view_number_of_rows(grid),
                 ns_grid_view_number_of_columns(grid));

  ns_text_field *header = ns_text_field_create_label_with_string("Details");
  ns_view *header_row[] = {ns_text_field_as_view(header)};
  ns_grid_row *row = ns_grid_view_add_row_with_views(grid, header_row, 1);
  SYN_ASSERT_MSG(row != NULL, "adding a row handed back nothing");
  SYN_ASSERT_STR_EQ(syn_test_class_name(row), "NSGridRow");
  /* The row is the grid's own, so the accessor answers with the same one. */
  SYN_ASSERT_MSG(ns_grid_view_row_at_index(grid, 0) == row,
                 "the row accessor did not answer with the added row");

  ns_text_field *label = ns_text_field_create_label_with_string("Title:");
  ns_text_field *field = ns_text_field_create_with_string("Kernel rewrite");
  ns_view *field_row[] = {ns_text_field_as_view(label),
                          ns_text_field_as_view(field)};
  ns_grid_view_add_row_with_views(grid, field_row, 2);

  SYN_ASSERT_MSG(ns_grid_view_number_of_rows(grid) == 2 &&
                     ns_grid_view_number_of_columns(grid) == 2,
                 "the filled grid reported %ldx%ld",
                 ns_grid_view_number_of_rows(grid),
                 ns_grid_view_number_of_columns(grid));

  ns_release(header);
  ns_release(label);
  ns_release(field);
  ns_release(grid);
}

/* What AppKit actually does with the marker: it never becomes the cell's
 * content view, so the cell reports nothing at all. */
SYN_TEST(a_cell_built_from_the_empty_content_view_holds_no_content_view) {
  syn_test_bootstrap();
  ns_grid_view *grid = ns_grid_view_create_with_number_of_columns_rows(2, 0);

  ns_view *empty = ns_grid_cell_empty_content_view();
  SYN_ASSERT_MSG(empty != NULL, "the empty content view is null");
  /* One view for the life of the process, which is what lets a form pass it in
   * several positions (R7). */
  SYN_ASSERT_MSG(empty == ns_grid_cell_empty_content_view(),
                 "the empty content view is not the same view twice");

  ns_button *checkbox = ns_button_create_checkbox_with_title("Flagged");
  ns_view *row[] = {empty, ns_button_as_view(checkbox)};
  ns_grid_view_add_row_with_views(grid, row, 2);

  ns_grid_cell *blank = ns_grid_view_cell_at_column_index_row_index(grid, 0, 0);
  SYN_ASSERT_STR_EQ(syn_test_class_name(blank), "NSGridCell");
  SYN_ASSERT_MSG(ns_grid_cell_content_view(blank) == NULL,
                 "a cell built from the marker reported a content view");

  ns_grid_cell *filled =
      ns_grid_view_cell_at_column_index_row_index(grid, 1, 0);
  SYN_ASSERT_MSG(ns_grid_cell_content_view(filled) ==
                     ns_button_as_view(checkbox),
                 "the second cell does not hold the checkbox");

  /* The setter is the other half of the same property, and it takes a view no
   * other cell is managing. */
  ns_view *late = syn_view(20, 20);
  ns_grid_cell_set_content_view(blank, late);
  SYN_ASSERT_MSG(ns_grid_cell_content_view(blank) == late,
                 "setting a cell's content view did not take");
  ns_grid_cell_set_content_view(blank, NULL);
  SYN_ASSERT_MSG(ns_grid_cell_content_view(blank) == NULL,
                 "clearing a cell's content view did not take");
  ns_release(late);

  ns_release(checkbox);
  ns_release(grid);
}

/* The merged header row: every index inside the range answers with one cell. */
SYN_TEST(merging_a_rows_cells_makes_the_whole_range_one_cell) {
  syn_test_bootstrap();
  ns_grid_view *grid = ns_grid_view_create_with_number_of_columns_rows(2, 0);
  ns_text_field *header = ns_text_field_create_label_with_string("Details");
  ns_view *header_row[] = {ns_text_field_as_view(header)};
  ns_grid_view_add_row_with_views(grid, header_row, 1);

  SYN_ASSERT_MSG(ns_grid_view_cell_at_column_index_row_index(grid, 0, 0) !=
                     ns_grid_view_cell_at_column_index_row_index(grid, 1, 0),
                 "the two cells were one before the merge");

  ns_grid_row_merge_cells_in_range(ns_grid_view_row_at_index(grid, 0), 0, 2);
  SYN_ASSERT_MSG(ns_grid_view_cell_at_column_index_row_index(grid, 0, 0) ==
                     ns_grid_view_cell_at_column_index_row_index(grid, 1, 0),
                 "the merged range still reads as two cells");
  /* Merging changes no coordinate: the grid is still two columns wide. */
  SYN_ASSERT_MSG(ns_grid_view_number_of_columns(grid) == 2,
                 "the merge changed the column count to %ld",
                 ns_grid_view_number_of_columns(grid));

  ns_release(header);
  ns_release(grid);
}

/* Placement and width round-trip, and the two accessors agree about which cell
 * a view landed in. */
SYN_TEST(a_columns_placement_and_width_round_trip_and_find_their_cells) {
  syn_test_bootstrap();
  ns_grid_view *grid = ns_grid_view_create_with_number_of_columns_rows(2, 0);
  ns_text_field *label = ns_text_field_create_label_with_string("Title:");
  ns_text_field *field = ns_text_field_create_with_string("Kernel rewrite");
  ns_view *row[] = {ns_text_field_as_view(label), ns_text_field_as_view(field)};
  ns_grid_view_add_row_with_views(grid, row, 2);

  ns_grid_column *first = ns_grid_view_column_at_index(grid, 0);
  ns_grid_column *second = ns_grid_view_column_at_index(grid, 1);
  SYN_ASSERT_STR_EQ(syn_test_class_name(first), "NSGridColumn");
  /* AppKit's own default on a column is to inherit the grid's placement. */
  SYN_ASSERT_MSG(ns_grid_column_x_placement(first) ==
                     NS_GRID_CELL_PLACEMENT_INHERITED,
                 "a new column started on placement %ld",
                 (long)ns_grid_column_x_placement(first));

  ns_grid_column_set_x_placement(first, NS_GRID_CELL_PLACEMENT_TRAILING);
  ns_grid_column_set_x_placement(second, NS_GRID_CELL_PLACEMENT_LEADING);
  ns_grid_column_set_width(second, 220);
  SYN_ASSERT(ns_grid_column_x_placement(first) ==
             NS_GRID_CELL_PLACEMENT_TRAILING);
  SYN_ASSERT(ns_grid_column_x_placement(second) ==
             NS_GRID_CELL_PLACEMENT_LEADING);
  SYN_ASSERT_MSG(ns_grid_column_width(second) == 220,
                 "the column width read back as %g",
                 ns_grid_column_width(second));

  /* cellForView: finds the cell a view was placed in, and the two accessors
   * answer with the same cell. */
  ns_grid_cell *cell =
      ns_grid_view_cell_for_view(grid, ns_text_field_as_view(field));
  SYN_ASSERT_MSG(cell != NULL, "cellForView found no cell for the field");
  SYN_ASSERT_MSG(cell == ns_grid_view_cell_at_column_index_row_index(grid, 1, 0),
                 "the field is not in the cell the indexes report");
  SYN_ASSERT_MSG(ns_grid_cell_row(cell) == ns_grid_view_row_at_index(grid, 0),
                 "the cell does not report the row it is in");
  SYN_ASSERT_MSG(ns_grid_cell_column(cell) == second,
                 "the cell does not report the column it is in");

  ns_grid_cell_set_x_placement(cell, NS_GRID_CELL_PLACEMENT_FILL);
  SYN_ASSERT_MSG(ns_grid_cell_x_placement(cell) == NS_GRID_CELL_PLACEMENT_FILL,
                 "the cell placement read back as %ld",
                 (long)ns_grid_cell_x_placement(cell));

  /* A view in no cell of this grid has no cell, which is an answer and not
   * misuse (R12). */
  ns_view *stranger = syn_view(20, 20);
  SYN_ASSERT_MSG(ns_grid_view_cell_for_view(grid, stranger) == NULL,
                 "a view outside the grid was reported in a cell");
  ns_release(stranger);

  ns_release(label);
  ns_release(field);
  ns_release(grid);
}

/* ---- edge pinning (KTD3, KTD18) ---- */

/* The unit's acceptance: a pinned view matches its superview's bounds, and
 * still matches after the window is resized. */
SYN_TEST(a_pinned_view_matches_its_superview_at_two_window_sizes) {
  syn_test_bootstrap();
  ns_window *window = syn_window_create(400, 300);
  ns_view *content = ns_window_content_view(window);
  ns_view *pane = syn_view(10, 10);

  ns_layout_pin_edges(pane, content, 0);
  ns_release(pane); /* the container holds it now (R7, AE2) */

  SYN_ASSERT_MSG(ns_view_superview(pane) == content,
                 "the pinned view was not added to the container");
  SYN_WAIT_FOR(syn_bounds_are(pane, 400, 300), 1000);

  ns_window_set_content_size(window, CGSizeMake(640, 480));
  SYN_WAIT_FOR(syn_bounds_are(pane, 640, 480), 1000);

  /* The pin is in the container's coordinates, so with no inset the origin is
   * the container's own. */
  CGRect in_container = ns_view_convert_rect_to_view(
      pane, ns_view_bounds(pane), content);
  SYN_ASSERT_MSG(in_container.origin.x == 0 && in_container.origin.y == 0,
                 "the pinned view sat at %g,%g", in_container.origin.x,
                 in_container.origin.y);

  ns_window_close(window);
  ns_release(window);
}

SYN_TEST(an_inset_pin_leaves_the_inset_on_every_edge) {
  syn_test_bootstrap();
  ns_window *window = syn_window_create(400, 300);
  ns_view *content = ns_window_content_view(window);
  ns_view *pane = syn_view(10, 10);

  ns_layout_pin_edges(pane, content, 20);
  ns_release(pane);

  SYN_WAIT_FOR(syn_bounds_are(pane, 360, 260), 1000);
  CGRect in_container =
      ns_view_convert_rect_to_view(pane, ns_view_bounds(pane), content);
  SYN_ASSERT_MSG(in_container.origin.x == 20 && in_container.origin.y == 20,
                 "the inset pin sat at %g,%g", in_container.origin.x,
                 in_container.origin.y);

  ns_window_close(window);
  ns_release(window);
}

/* A top pin leaves the height alone, which is what a form pinned to the top of
 * a pane needs - the detail pane and every settings pane use it. */
SYN_TEST(a_top_pinned_grid_keeps_its_own_height) {
  syn_test_bootstrap();
  ns_window *window = syn_window_create(400, 300);
  ns_view *content = ns_window_content_view(window);

  ns_text_field *label = ns_text_field_create_label_with_string("Title:");
  ns_text_field *field = ns_text_field_create_with_string("Kernel");
  ns_view *row_views[] = {ns_text_field_as_view(label),
                          ns_text_field_as_view(field)};
  ns_grid_view_row rows[] = {{.views = row_views, .count = 2}};
  ns_grid_view *grid = ns_grid_view_create_with_views(rows, 1);

  ns_layout_pin_top_edges(ns_grid_view_as_view(grid), content, 20);
  SYN_WAIT_FOR(ns_view_bounds(ns_grid_view_as_view(grid)).size.height > 0,
               1000);

  CGRect bounds = ns_view_bounds(ns_grid_view_as_view(grid));
  SYN_ASSERT_MSG(bounds.size.height > 0 && bounds.size.height < 260,
                 "a top-pinned grid took the whole height: %g",
                 bounds.size.height);
  CGRect in_content = ns_view_convert_rect_to_view(
      ns_grid_view_as_view(grid), bounds, content);
  SYN_ASSERT_MSG(in_content.origin.x == 20, "the grid sat at x=%g",
                 in_content.origin.x);

  ns_release(label);
  ns_release(field);
  ns_release(grid);
  ns_window_close(window);
  ns_release(window);
}

/* The teardown the leak gate reads: a window holding a stack and a grid, both
 * released by the caller, torn down by the window's own release (F3, KTD12). */
SYN_TEST(tearing_down_a_window_holding_a_stack_and_a_grid_leaves_nothing) {
  syn_test_bootstrap();
  ns_window *window = syn_window_create(400, 300);
  ns_view *content = ns_window_content_view(window);

  ns_view *left = syn_view(40, 20);
  ns_view *right = syn_view(40, 20);
  ns_view *seed[] = {left, right};
  ns_stack_view *stack = ns_stack_view_create_with_views(seed, 2);
  ns_stack_view_set_spacing(stack, 8);
  ns_layout_pin_top_edges(ns_stack_view_as_view(stack), content, 10);
  ns_release(left);
  ns_release(right);
  ns_release(stack);

  ns_text_field *label = ns_text_field_create_label_with_string("Owner:");
  ns_text_field *field = ns_text_field_create_with_string("Kal");
  ns_view *row_views[] = {ns_text_field_as_view(label),
                          ns_text_field_as_view(field)};
  ns_grid_view_row rows[] = {{.views = row_views, .count = 2}};
  ns_grid_view *grid = ns_grid_view_create_with_views(rows, 1);
  ns_layout_pin_edges(ns_grid_view_as_view(grid), content, 60);
  ns_release(label);
  ns_release(field);
  ns_release(grid);

  SYN_WAIT_FOR(syn_bounds_are(ns_grid_view_as_view(grid), 280, 180), 1000);
  SYN_ASSERT_MSG(ns_view_subview_count(content) == 2,
                 "the content view holds %ld subviews",
                 ns_view_subview_count(content));

  ns_window_close(window);
  ns_release(window);
}

/* The detail pane's grid built the way the twin builds it, torn down by the
 * window's own release: the rows, columns and cells are the grid's and go with
 * it, so nothing here is the caller's to free (R7, KTD12). */
SYN_TEST(tearing_down_a_window_holding_a_built_up_grid_leaves_nothing) {
  syn_test_bootstrap();
  ns_window *window = syn_window_create(400, 300);
  ns_view *content = ns_window_content_view(window);

  ns_grid_view *grid = ns_grid_view_create_with_number_of_columns_rows(2, 0);
  ns_grid_view_set_row_spacing(grid, 10);
  ns_grid_view_set_column_spacing(grid, 12);

  ns_text_field *header = ns_text_field_create_label_with_string("Details");
  ns_view *header_row[] = {ns_text_field_as_view(header)};
  ns_grid_view_add_row_with_views(grid, header_row, 1);
  ns_grid_row_merge_cells_in_range(ns_grid_view_row_at_index(grid, 0), 0, 2);
  ns_grid_cell_set_x_placement(
      ns_grid_view_cell_at_column_index_row_index(grid, 0, 0),
      NS_GRID_CELL_PLACEMENT_LEADING);

  ns_text_field *label = ns_text_field_create_label_with_string("Title:");
  ns_text_field *field = ns_text_field_create_with_string("Kernel rewrite");
  ns_view *field_row[] = {ns_text_field_as_view(label),
                          ns_text_field_as_view(field)};
  ns_grid_view_add_row_with_views(grid, field_row, 2);

  ns_button *checkbox = ns_button_create_checkbox_with_title("Flagged");
  ns_view *check_row[] = {ns_grid_cell_empty_content_view(),
                          ns_button_as_view(checkbox)};
  ns_grid_view_add_row_with_views(grid, check_row, 2);

  ns_grid_column_set_x_placement(ns_grid_view_column_at_index(grid, 0),
                                 NS_GRID_CELL_PLACEMENT_TRAILING);
  ns_grid_column_set_x_placement(ns_grid_view_column_at_index(grid, 1),
                                 NS_GRID_CELL_PLACEMENT_LEADING);
  ns_grid_column_set_width(ns_grid_view_column_at_index(grid, 1), 220);
  ns_grid_cell_set_x_placement(
      ns_grid_view_cell_for_view(grid, ns_text_field_as_view(field)),
      NS_GRID_CELL_PLACEMENT_FILL);

  ns_layout_pin_top_edges(ns_grid_view_as_view(grid), content, 20);
  ns_release(header);
  ns_release(label);
  ns_release(field);
  ns_release(checkbox);
  ns_release(grid);

  SYN_WAIT_FOR(ns_view_bounds(ns_grid_view_as_view(grid)).size.height > 0,
               1000);
  SYN_ASSERT_MSG(ns_grid_view_number_of_rows(grid) == 3,
                 "the built-up grid reported %ld rows",
                 ns_grid_view_number_of_rows(grid));

  ns_window_close(window);
  ns_release(window);
}

/* ---- fonts and colours (R7, R17) ---- */

SYN_TEST(the_bold_system_font_differs_from_the_system_font_and_round_trips) {
  syn_test_bootstrap();
  CGFloat size = ns_font_system_font_size();
  SYN_ASSERT_MSG(size > 0, "the system font size is %g", size);

  ns_font *plain = ns_font_copy_system_font_of_size(size);
  ns_font *bold = ns_font_copy_bold_system_font_of_size(size);
  /* The concrete class is AppKit's business - the system font arrives as a
   * toll-free-bridged NSCTFont - so what the suite pins is the behaviour. */
  SYN_ASSERT_MSG(ns_font_point_size(bold) == size,
                 "the bold font came back at %g", ns_font_point_size(bold));

  char *plain_name = ns_font_copy_font_name(plain);
  char *bold_name = ns_font_copy_font_name(bold);
  SYN_ASSERT_MSG(plain_name != NULL && bold_name != NULL,
                 "a font reported no name");
  SYN_ASSERT_MSG(strcmp(plain_name, bold_name) != 0,
                 "the bold and plain system fonts are both \"%s\"", bold_name);
  ns_string_free(plain_name);
  ns_string_free(bold_name);

  /* A label's font round-trips through the control upcast, which is where the
   * SDK declares it (R5). */
  ns_text_field *label = ns_text_field_create_label_with_string("Details");
  ns_control *control = ns_text_field_as_control(label);
  ns_control_set_font(control, bold);
  ns_font *read_back = ns_control_copy_font(control);
  SYN_ASSERT_MSG(read_back != NULL, "the label reported no font");
  char *read_name = ns_font_copy_font_name(read_back);
  char *expected = ns_font_copy_font_name(bold);
  SYN_ASSERT_STR_EQ(read_name, expected);
  ns_string_free(read_name);
  ns_string_free(expected);
  ns_release(read_back);

  ns_release(label);
  ns_release(bold);
  ns_release(plain);
}

SYN_TEST(a_semantic_colour_round_trips_on_a_fields_text_colour) {
  syn_test_bootstrap();
  ns_color *label_colour = ns_color_label_color();
  ns_color *secondary = ns_color_secondary_label_color();
  ns_color *accent = ns_color_control_accent_color();
  SYN_ASSERT_MSG(label_colour != NULL && secondary != NULL && accent != NULL,
                 "a semantic colour came back null");
  SYN_ASSERT_MSG(label_colour != secondary,
                 "labelColor and secondaryLabelColor are the same object");

  ns_text_field *field = ns_text_field_create_label_with_string("Details");
  ns_text_field_set_text_color(field, label_colour);
  ns_color *read_back = ns_text_field_copy_text_color(field);
  SYN_ASSERT_MSG(read_back != NULL, "the field reported no text colour");
  SYN_ASSERT_MSG(read_back == label_colour,
                 "the text colour is not the one that was set");
  ns_release(read_back);

  ns_text_field_set_text_color(field, secondary);
  ns_color *second_read = ns_text_field_copy_text_color(field);
  SYN_ASSERT_MSG(second_read == secondary,
                 "the second text colour did not round-trip");
  ns_release(second_read);

  /* A borrowed shared colour outlives the field that used it: AppKit holds it
   * for the life of the process (R7). */
  ns_release(field);
  SYN_ASSERT_STR_EQ(syn_test_class_name(ns_color_label_color()),
                    syn_test_class_name(label_colour));
}

/* ---- the members a layout report is made of (R17) ---- */

SYN_TEST(a_titled_labels_intrinsic_size_is_not_zero) {
  syn_test_bootstrap();
  ns_text_field *label = ns_text_field_create_label_with_string("Details");
  CGSize intrinsic =
      ns_view_intrinsic_content_size(ns_text_field_as_view(label));
  SYN_ASSERT_MSG(intrinsic.width > 0 && intrinsic.height > 0,
                 "a titled label reported an intrinsic size of %gx%g",
                 intrinsic.width, intrinsic.height);
  ns_release(label);
}

SYN_TEST(the_two_layout_priorities_round_trip_per_orientation) {
  syn_test_bootstrap();
  ns_text_field *label = ns_text_field_create_label_with_string("Details");
  ns_view *view = ns_text_field_as_view(label);

  ns_view_set_content_hugging_priority_for_orientation(
      view, 751, NS_LAYOUT_CONSTRAINT_ORIENTATION_HORIZONTAL);
  ns_view_set_content_hugging_priority_for_orientation(
      view, 249, NS_LAYOUT_CONSTRAINT_ORIENTATION_VERTICAL);
  SYN_ASSERT_MSG(ns_view_content_hugging_priority_for_orientation(
                     view, NS_LAYOUT_CONSTRAINT_ORIENTATION_HORIZONTAL) == 751,
                 "the horizontal hugging priority read back as %g",
                 (double)ns_view_content_hugging_priority_for_orientation(
                     view, NS_LAYOUT_CONSTRAINT_ORIENTATION_HORIZONTAL));
  SYN_ASSERT_MSG(ns_view_content_hugging_priority_for_orientation(
                     view, NS_LAYOUT_CONSTRAINT_ORIENTATION_VERTICAL) == 249,
                 "the vertical hugging priority read back as %g",
                 (double)ns_view_content_hugging_priority_for_orientation(
                     view, NS_LAYOUT_CONSTRAINT_ORIENTATION_VERTICAL));

  ns_view_set_content_compression_resistance_priority_for_orientation(
      view, 748, NS_LAYOUT_CONSTRAINT_ORIENTATION_HORIZONTAL);
  ns_view_set_content_compression_resistance_priority_for_orientation(
      view, 252, NS_LAYOUT_CONSTRAINT_ORIENTATION_VERTICAL);
  SYN_ASSERT_MSG(
      ns_view_content_compression_resistance_priority_for_orientation(
          view, NS_LAYOUT_CONSTRAINT_ORIENTATION_HORIZONTAL) == 748,
      "the horizontal resistance read back as %g",
      (double)ns_view_content_compression_resistance_priority_for_orientation(
          view, NS_LAYOUT_CONSTRAINT_ORIENTATION_HORIZONTAL));
  SYN_ASSERT_MSG(
      ns_view_content_compression_resistance_priority_for_orientation(
          view, NS_LAYOUT_CONSTRAINT_ORIENTATION_VERTICAL) == 252,
      "the vertical resistance read back as %g",
      (double)ns_view_content_compression_resistance_priority_for_orientation(
          view, NS_LAYOUT_CONSTRAINT_ORIENTATION_VERTICAL));

  ns_release(label);
}

SYN_TEST(convert_rect_to_view_maps_a_subviews_origin_into_its_superview) {
  syn_test_bootstrap();
  ns_view *pane = syn_view(200, 100);
  ns_view *child = ns_view_create_with_frame(CGRectMake(30, 40, 50, 20));
  ns_view_add_subview(pane, child);

  CGRect bounds = ns_view_bounds(child);
  SYN_ASSERT_MSG(bounds.origin.x == 0 && bounds.origin.y == 0 &&
                     bounds.size.width == 50 && bounds.size.height == 20,
                 "the child's bounds are %g,%g %gx%g", bounds.origin.x,
                 bounds.origin.y, bounds.size.width, bounds.size.height);

  CGRect in_pane = ns_view_convert_rect_to_view(child, bounds, pane);
  SYN_ASSERT_MSG(in_pane.origin.x == 30 && in_pane.origin.y == 40 &&
                     in_pane.size.width == 50 && in_pane.size.height == 20,
                 "the child mapped to %g,%g %gx%g in its superview",
                 in_pane.origin.x, in_pane.origin.y, in_pane.size.width,
                 in_pane.size.height);

  /* Converting to itself is the identity, which is the degenerate case a
   * layout report hits for the reference view. */
  CGRect in_self = ns_view_convert_rect_to_view(child, bounds, child);
  SYN_ASSERT(in_self.origin.x == 0 && in_self.origin.y == 0);

  ns_release(child);
  ns_release(pane);
}

/* ---- the Tab chain (R19) ---- */

/* What the twins rely on: a chain set by hand is still what was set after the
 * run loop has turned. Both halves of the flag are pinned, because on this OS
 * the flag is not what decides it - a rebuild is, and nothing asks for one
 * until something does. */
SYN_TEST(an_explicit_key_view_chain_survives_a_run_loop_spin) {
  syn_test_bootstrap();
  for (int autorecalculates = 0; autorecalculates <= 1; autorecalculates++) {
    ns_window *window = syn_window_create(400, 300);
    ns_view *content = ns_window_content_view(window);
    ns_button *first = ns_button_create_with_title("One");
    ns_button *second = ns_button_create_with_title("Two");
    ns_button *third = ns_button_create_with_title("Three");
    ns_view *a = ns_button_as_view(first);
    ns_view *b = ns_button_as_view(second);
    ns_view *c = ns_button_as_view(third);
    ns_view_set_frame(a, CGRectMake(10, 200, 100, 24));
    ns_view_set_frame(b, CGRectMake(10, 150, 100, 24));
    ns_view_set_frame(c, CGRectMake(10, 100, 100, 24));
    ns_view_add_subview(content, a);
    ns_view_add_subview(content, b);
    ns_view_add_subview(content, c);

    ns_window_set_autorecalculates_key_view_loop(window,
                                                 autorecalculates != 0);
    /* Deliberately the reverse of the geometric order, so a rebuild would be
     * visible as a different chain rather than the same one. */
    ns_view_set_next_key_view(a, c);
    ns_view_set_next_key_view(c, b);
    ns_view_set_next_key_view(b, a);
    ns_window_set_initial_first_responder(window, a);

    syn_test_spin(50.0);

    SYN_ASSERT_MSG(ns_view_next_key_view(a) == c &&
                       ns_view_next_key_view(c) == b &&
                       ns_view_next_key_view(b) == a,
                   "the chain did not survive with autorecalculates %s",
                   autorecalculates ? "on" : "off");
    SYN_ASSERT_MSG(ns_window_initial_first_responder(window) == a,
                   "the initial first responder is not the view that was set");

    ns_release(first);
    ns_release(second);
    ns_release(third);
    ns_window_close(window);
    ns_release(window);
  }
}

/* The other half: a rebuild is what replaces the chain, and it does so whether
 * the flag is on or off. That is the measurement behind
 * ns_window_set_autorecalculates_key_view_loop's comment - the flag suppresses
 * the rebuilds AppKit drives, not one the program asks for. */
SYN_TEST(recalculating_the_key_view_loop_replaces_an_explicit_chain) {
  syn_test_bootstrap();
  for (int autorecalculates = 0; autorecalculates <= 1; autorecalculates++) {
    ns_window *window = syn_window_create(400, 300);
    ns_view *content = ns_window_content_view(window);
    ns_button *first = ns_button_create_with_title("One");
    ns_button *second = ns_button_create_with_title("Two");
    ns_view *a = ns_button_as_view(first);
    ns_view *b = ns_button_as_view(second);
    ns_view_set_frame(a, CGRectMake(10, 200, 100, 24));
    ns_view_set_frame(b, CGRectMake(10, 150, 100, 24));
    ns_view_add_subview(content, a);
    ns_view_add_subview(content, b);

    ns_window_set_autorecalculates_key_view_loop(window,
                                                 autorecalculates != 0);
    ns_view_set_next_key_view(a, b);
    ns_view_set_next_key_view(b, a);
    SYN_ASSERT(ns_view_next_key_view(a) == b);

    ns_window_recalculate_key_view_loop(window);
    syn_test_spin(50.0);

    SYN_ASSERT_MSG(ns_view_next_key_view(a) != b,
                   "the rebuild left the explicit chain in place with "
                   "autorecalculates %s",
                   autorecalculates ? "on" : "off");

    ns_release(first);
    ns_release(second);
    ns_window_close(window);
    ns_release(window);
  }
}

SYN_TEST(a_null_next_key_view_and_a_null_first_responder_clear_the_chain) {
  syn_test_bootstrap();
  ns_window *window = syn_window_create(400, 300);
  ns_view *content = ns_window_content_view(window);
  ns_button *button = ns_button_create_with_title("One");
  ns_view *view = ns_button_as_view(button);
  ns_view_add_subview(content, view);

  ns_window_set_initial_first_responder(window, view);
  SYN_ASSERT(ns_window_initial_first_responder(window) == view);
  ns_window_set_initial_first_responder(window, NULL);
  SYN_ASSERT_MSG(ns_window_initial_first_responder(window) == NULL,
                 "a null initial first responder did not clear the slot");

  ns_view_set_next_key_view(view, NULL);
  SYN_ASSERT_MSG(ns_view_next_key_view(view) == NULL,
                 "a null next key view did not clear the slot");

  ns_release(button);
  ns_window_close(window);
  ns_release(window);
}

/* ---- the cases that abort, each run in a child (R10, R12, KTD4) ---- */

SYN_ABORT_CASE(arranged_subview_read_past_the_end) {
  syn_test_bootstrap();
  ns_stack_view *stack = ns_stack_view_create_with_views(NULL, 0);
  ns_stack_view_arranged_subview_at_index(stack, 0);
}

SYN_TEST(reading_an_arranged_subview_past_the_end_is_the_appkit_exception) {
  SYN_ASSERT_ABORTS("arranged_subview_read_past_the_end",
                    "ns_stack_view_arranged_subview_at_index");
  SYN_ASSERT_ABORTS("arranged_subview_read_past_the_end", "AppKit raised");
}

/* The grid's index accessors need no check of their own: AppKit raises
 * NSRangeException before it changes anything and the entry macro reports that
 * exception with the function named (KTD4, and "The index check"). */
SYN_ABORT_CASE(grid_row_read_past_the_end) {
  syn_test_bootstrap();
  ns_grid_view *grid = ns_grid_view_create_with_number_of_columns_rows(2, 1);
  ns_grid_view_row_at_index(grid, 4);
}

SYN_ABORT_CASE(grid_column_read_past_the_end) {
  syn_test_bootstrap();
  ns_grid_view *grid = ns_grid_view_create_with_number_of_columns_rows(2, 1);
  ns_grid_view_column_at_index(grid, 9);
}

SYN_ABORT_CASE(grid_cell_read_at_a_negative_index) {
  syn_test_bootstrap();
  ns_grid_view *grid = ns_grid_view_create_with_number_of_columns_rows(2, 1);
  ns_grid_view_cell_at_column_index_row_index(grid, -1, 0);
}

SYN_TEST(reading_a_grid_outside_its_range_is_the_appkit_exception_it_is) {
  SYN_ASSERT_ABORTS("grid_row_read_past_the_end", "ns_grid_view_row_at_index");
  SYN_ASSERT_ABORTS("grid_row_read_past_the_end", "AppKit raised");
  SYN_ASSERT_ABORTS("grid_column_read_past_the_end",
                    "ns_grid_view_column_at_index");
  SYN_ASSERT_ABORTS("grid_column_read_past_the_end", "AppKit raised");
  SYN_ASSERT_ABORTS("grid_cell_read_at_a_negative_index",
                    "ns_grid_view_cell_at_column_index_row_index");
  SYN_ASSERT_ABORTS("grid_cell_read_at_a_negative_index", "AppKit raised");
}

SYN_ABORT_CASE(a_window_merged_where_a_grid_row_belongs) {
  syn_test_bootstrap();
  ns_window *window = syn_window_create(400, 300);
  ns_grid_row_merge_cells_in_range((ns_grid_row *)(void *)window, 0, 2);
}

SYN_TEST(merging_a_handle_of_the_wrong_class_names_both_classes) {
  SYN_ASSERT_ABORTS("a_window_merged_where_a_grid_row_belongs", "NSGridRow");
  SYN_ASSERT_ABORTS("a_window_merged_where_a_grid_row_belongs",
                    "ns_grid_row_merge_cells_in_range");
}

SYN_ABORT_CASE(a_window_pinned_where_a_view_belongs) {
  syn_test_bootstrap();
  ns_window *window = syn_window_create(400, 300);
  ns_layout_pin_edges((ns_view *)(void *)window,
                      ns_window_content_view(window), 0);
}

SYN_TEST(pinning_a_handle_of_the_wrong_class_names_both_classes) {
  SYN_ASSERT_ABORTS("a_window_pinned_where_a_view_belongs", "NSView");
  SYN_ASSERT_ABORTS("a_window_pinned_where_a_view_belongs",
                    "ns_layout_pin_edges");
}

static void *syn_read_the_font_size_off_the_main_thread(void *unused) {
  (void)unused;
  ns_font_system_font_size();
  return NULL;
}

SYN_ABORT_CASE(font_size_read_off_the_main_thread) {
  syn_test_bootstrap();
  pthread_t thread;
  if (pthread_create(&thread, NULL, syn_read_the_font_size_off_the_main_thread,
                     NULL) != 0) {
    fprintf(stderr, "could not start the background thread\n");
    return;
  }
  pthread_join(thread, NULL);
}

SYN_TEST(reading_the_font_size_off_the_main_thread_names_the_function) {
  SYN_ASSERT_ABORTS("font_size_read_off_the_main_thread",
                    "ns_font_system_font_size");
  SYN_ASSERT_ABORTS("font_size_read_off_the_main_thread", "main thread only");
}

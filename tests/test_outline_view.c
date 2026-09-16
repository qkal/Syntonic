/*
 * The outline suite (U8): the sidebar's source list, its merged callbacks
 * struct, the pointer-stable items AppKit addresses its rows by, and the
 * unselectable group rows a sidebar's headings are (R9, R15, AE1, KTD6, KTD8).
 *
 * The identity test is the one that matters: AppKit keeps an item's expansion
 * state only while the object it was handed stays the same object, so the
 * wrapper boxes each C pointer once and keeps the box across a reload.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "runner.h"
#include "support.h"
#include "syntonic/syntonic.h"

/* ---- the C tree the struct walks: two groups of two rows each ---- */

typedef struct syn_node {
  const char *title;
  const struct syn_node *children;
  long child_count;
} syn_node;

static const syn_node syn_library[] = {
    {"All Items", NULL, 0},
    {"Flagged", NULL, 0},
};
static const syn_node syn_collections[] = {
    {"Kernel", NULL, 0},
    {"Docs", NULL, 0},
};
static const syn_node syn_groups[] = {
    {"Library", syn_library, 2},
    {"Collections", syn_collections, 2},
};

#define SYN_GROUP_COUNT 2

static int syn_group_questions;
static int syn_expand_questions;
static int syn_selection_fires;
static const void *syn_selected_item;
static void *syn_selection_context;
static const void *syn_selection_sender;
static bool syn_allow_expansion;

static void syn_reset(void) {
  syn_group_questions = syn_expand_questions = syn_selection_fires = 0;
  syn_selected_item = NULL;
  syn_selection_context = NULL;
  syn_selection_sender = NULL;
  syn_allow_expansion = true;
}

static long syn_children(void *context, ns_outline_view *sender,
                         const void *item) {
  (void)context;
  (void)sender;
  if (item == NULL) return SYN_GROUP_COUNT;
  return ((const syn_node *)item)->child_count;
}

/* KTD6: the same child has to come back as the same pointer every time, which
 * a C program gets for free by returning addresses into its own tree. */
static const void *syn_child(void *context, ns_outline_view *sender,
                             long index, const void *item) {
  (void)context;
  (void)sender;
  if (item == NULL) return &syn_groups[index];
  return &((const syn_node *)item)->children[index];
}

static bool syn_expandable(void *context, ns_outline_view *sender,
                           const void *item) {
  (void)context;
  (void)sender;
  return ((const syn_node *)item)->child_count > 0;
}

static const char *syn_cell(void *context, ns_outline_view *sender,
                            const void *item) {
  (void)context;
  (void)sender;
  return ((const syn_node *)item)->title;
}

/* R15: the three headings of a sidebar are group rows, and a group row is not
 * selectable. */
static bool syn_is_group(void *context, ns_outline_view *sender,
                         const void *item) {
  (void)context;
  (void)sender;
  syn_group_questions++;
  return ((const syn_node *)item)->child_count > 0;
}

static bool syn_should_select(void *context, ns_outline_view *sender,
                              const void *item) {
  (void)context;
  (void)sender;
  return ((const syn_node *)item)->child_count == 0;
}

static bool syn_should_expand(void *context, ns_outline_view *sender,
                              const void *item) {
  (void)context;
  (void)sender;
  (void)item;
  syn_expand_questions++;
  return syn_allow_expansion;
}

/* R15: a sidebar's headings never close. */
static bool syn_should_not_collapse(void *context, ns_outline_view *sender,
                                    const void *item) {
  (void)context;
  (void)sender;
  (void)item;
  return false;
}

static void syn_on_selection(void *context, ns_outline_view *sender,
                             const void *item) {
  syn_selection_fires++;
  syn_selected_item = item;
  syn_selection_context = context;
  syn_selection_sender = sender;
}

/* ---- building one ---- */

static const ns_outline_view_callbacks syn_sidebar_callbacks = {
    .number_of_children_of_item = syn_children,
    .child_of_item = syn_child,
    .is_item_expandable = syn_expandable,
    .cell_string = syn_cell,
    .is_group_item = syn_is_group,
    .should_select_item = syn_should_select,
    .selection_did_change = syn_on_selection,
};

static ns_outline_view *syn_outline_create(ns_scroll_view **out_scroll) {
  ns_scroll_view *scroll =
      ns_scroll_view_create_with_frame(CGRectMake(0, 0, 220, 400));
  ns_outline_view *outline =
      ns_outline_view_create_with_frame(CGRectMake(0, 0, 220, 400));

  ns_table_column *column =
      ns_table_column_create_with_identifier("collection");
  ns_table_view *as_table = ns_outline_view_as_table_view(outline);
  ns_table_view_add_table_column(as_table, column);
  ns_outline_view_set_outline_table_column(outline, column);
  ns_release(column);

  /* A sidebar has no header row and keeps its group rows in place (R15). */
  ns_table_view_set_header_view(as_table, NULL);
  ns_table_view_set_floats_group_rows(as_table, false);

  ns_scroll_view_set_document_view(scroll, ns_outline_view_as_view(outline));
  ns_scroll_view_set_draws_background(scroll, false);
  ns_scroll_view_set_has_vertical_scroller(scroll, true);
  ns_scroll_view_set_autohides_scrollers(scroll, true);
  *out_scroll = scroll;
  return outline;
}

static char *syn_cell_text(ns_outline_view *outline, long row) {
  ns_view *cell = ns_table_view_view_at_column_row_make_if_necessary(
      ns_outline_view_as_table_view(outline), 0, row, true);
  if (cell == NULL || ns_view_subview_count(cell) != 1) return NULL;
  ns_view *label = ns_view_subview_at_index(cell, 0);
  return ns_control_copy_string_value(
      ns_text_field_as_control((ns_text_field *)(void *)label));
}

/* ---- the tree (R15, KTD6) ---- */

SYN_TEST(an_outline_over_two_groups_reports_groups_plus_children_as_rows) {
  syn_test_bootstrap();
  syn_reset();
  int context = 9;

  ns_scroll_view *scroll = NULL;
  ns_outline_view *outline = syn_outline_create(&scroll);
  SYN_ASSERT_STR_EQ(syn_test_class_name(outline), "NSOutlineView");
  ns_table_view *as_table = ns_outline_view_as_table_view(outline);

  /* R15: the source list style is the one property the constructor sets. */
  SYN_ASSERT_MSG(ns_table_view_get_style(as_table) ==
                     NS_TABLE_VIEW_STYLE_SOURCE_LIST,
                 "a new outline is on style %ld, not the source list",
                 (long)ns_table_view_get_style(as_table));
  SYN_ASSERT_MSG(ns_outline_view_outline_table_column(outline) != NULL,
                 "the outline has no outline column");
  SYN_ASSERT_MSG(ns_scroll_view_document_view(scroll) ==
                     ns_outline_view_as_view(outline),
                 "the scroll view does not report the outline as its document");

  ns_outline_view_set_callbacks(outline, &syn_sidebar_callbacks, &context);
  SYN_ASSERT_SHIMS(1);
  /* One shim, both slots (KTD6). */
  SYN_ASSERT_MSG(
      syn_test_delegate_responds(outline, "outlineView:numberOfChildrenOfItem:"),
      "the data source half of the merged struct is not installed");
  SYN_ASSERT_MSG(
      syn_test_delegate_responds(outline, "outlineView:viewForTableColumn:item:"),
      "the delegate half of the merged struct is not installed");

  ns_table_view_reload_data(as_table);
  SYN_ASSERT_MSG(ns_table_view_number_of_rows(as_table) == SYN_GROUP_COUNT,
                 "a collapsed outline showed %ld rows, not 2",
                 ns_table_view_number_of_rows(as_table));

  /* A null item is the root, so this opens every group at once (R15). */
  ns_outline_view_expand_item_expand_children(outline, NULL, true);
  SYN_ASSERT_MSG(ns_table_view_number_of_rows(as_table) == 6,
                 "the expanded outline showed %ld rows, not 6",
                 ns_table_view_number_of_rows(as_table));

  /* Row 0 is the first group heading and row 1 is its first child, which is
   * the row a sidebar selects at launch. */
  SYN_ASSERT_MSG(ns_outline_view_item_at_row(outline, 0) == &syn_groups[0],
                 "row 0 is not the first group");
  SYN_ASSERT_MSG(ns_outline_view_item_at_row(outline, 1) == &syn_library[0],
                 "row 1 is not the first group's first child");
  SYN_ASSERT_MSG(ns_outline_view_row_for_item(outline, &syn_collections[1]) == 5,
                 "the last child is on row %ld, not 5",
                 ns_outline_view_row_for_item(outline, &syn_collections[1]));

  char *heading = syn_cell_text(outline, 0);
  SYN_ASSERT_STR_EQ(heading, "Library");
  ns_string_free(heading);
  char *child = syn_cell_text(outline, 1);
  SYN_ASSERT_STR_EQ(child, "All Items");
  ns_string_free(child);

  ns_outline_view_set_callbacks(outline, NULL, NULL);
  SYN_ASSERT_SHIMS(0);
  ns_release(outline);
  ns_release(scroll);
}

/* KTD6: the box for one C pointer is one object for the life of the outline,
 * which is what AppKit needs to keep an expanded group expanded. */
SYN_TEST(an_items_identity_and_its_expansion_survive_a_reload) {
  syn_test_bootstrap();
  syn_reset();

  ns_scroll_view *scroll = NULL;
  ns_outline_view *outline = syn_outline_create(&scroll);
  ns_table_view *as_table = ns_outline_view_as_table_view(outline);
  ns_outline_view_set_callbacks(outline, &syn_sidebar_callbacks, NULL);
  ns_table_view_reload_data(as_table);

  ns_outline_view_expand_item(outline, &syn_groups[0]);
  SYN_ASSERT_MSG(ns_outline_view_item_expanded(outline, &syn_groups[0]),
                 "the first group did not expand");
  SYN_ASSERT_MSG(!ns_outline_view_item_expanded(outline, &syn_groups[1]),
                 "the second group expanded on its own");
  SYN_ASSERT(ns_table_view_number_of_rows(as_table) == 4);
  SYN_ASSERT(ns_outline_view_item_at_row(outline, 1) == &syn_library[0]);

  ns_table_view_reload_data(as_table);
  SYN_ASSERT_MSG(ns_outline_view_item_expanded(outline, &syn_groups[0]),
                 "the reload collapsed the group, so the item is a new object");
  SYN_ASSERT_MSG(ns_table_view_number_of_rows(as_table) == 4,
                 "the reload left %ld rows, not 4",
                 ns_table_view_number_of_rows(as_table));
  SYN_ASSERT_MSG(ns_outline_view_item_at_row(outline, 1) == &syn_library[0],
                 "the same row came back as a different pointer");

  /* Replacing the struct keeps the cache too, so expansion survives that. */
  ns_outline_view_set_callbacks(outline, &syn_sidebar_callbacks, NULL);
  ns_table_view_reload_data(as_table);
  SYN_ASSERT_MSG(ns_outline_view_item_expanded(outline, &syn_groups[0]),
                 "replacing the struct collapsed the group");

  ns_outline_view_collapse_item(outline, &syn_groups[0]);
  SYN_ASSERT_MSG(!ns_outline_view_item_expanded(outline, &syn_groups[0]),
                 "the group did not collapse");
  SYN_ASSERT(ns_table_view_number_of_rows(as_table) == 2);

  ns_outline_view_set_callbacks(outline, NULL, NULL);
  ns_release(outline);
  ns_release(scroll);
}

/* The cache keys on the raw address for the outline's whole life, so the only
 * way to reuse an address is to forget it first: the next sight of it is a new
 * item carrying none of the old row's state. */
SYN_TEST(forgetting_an_item_makes_its_address_a_new_item) {
  syn_test_bootstrap();
  syn_reset();

  ns_scroll_view *scroll = NULL;
  ns_outline_view *outline = syn_outline_create(&scroll);
  ns_table_view *as_table = ns_outline_view_as_table_view(outline);
  ns_outline_view_set_callbacks(outline, &syn_sidebar_callbacks, NULL);
  ns_table_view_reload_data(as_table);
  ns_outline_view_expand_item(outline, &syn_groups[0]);

  long row = ns_outline_view_row_for_item(outline, &syn_library[0]);
  SYN_ASSERT_MSG(row == 1, "the first child is on row %ld, not 1", row);

  /* A null item, and an address the outline has never boxed, forget nothing. */
  int never_seen = 0;
  ns_outline_view_forget_item(outline, NULL);
  ns_outline_view_forget_item(outline, &never_seen);
  SYN_ASSERT_MSG(ns_outline_view_row_for_item(outline, &syn_library[0]) == row,
                 "forgetting nothing moved the row");

  ns_outline_view_forget_item(outline, &syn_library[0]);
  SYN_ASSERT_MSG(ns_outline_view_row_for_item(outline, &syn_library[0]) == -1,
                 "the same address still came back as the row AppKit holds, so "
                 "the box behind it is the old one");

  /* Nothing is destroyed with it: the next reload boxes the address afresh and
   * the row is back, and the group's own box - and with it its expansion - was
   * never touched. */
  ns_table_view_reload_data(as_table);
  SYN_ASSERT_MSG(ns_outline_view_row_for_item(outline, &syn_library[0]) == row,
                 "the forgotten address did not come back as a row");
  SYN_ASSERT_MSG(ns_outline_view_item_expanded(outline, &syn_groups[0]),
                 "forgetting a child collapsed its group");

  ns_outline_view_set_callbacks(outline, NULL, NULL);
  ns_release(outline);
  ns_release(scroll);
}

/* ---- AE1: the optional member that is not set ---- */

SYN_TEST(an_outline_with_should_expand_item_unset_expands) {
  syn_test_bootstrap();
  syn_reset();

  ns_scroll_view *scroll = NULL;
  ns_outline_view *outline = syn_outline_create(&scroll);
  ns_table_view *as_table = ns_outline_view_as_table_view(outline);

  /* The struct leaves should_expand_item unset, so the shim reports the
   * selector as one it does not implement and AppKit takes its own default. */
  ns_outline_view_callbacks without_should_expand = {
      .number_of_children_of_item = syn_children,
      .child_of_item = syn_child,
      .is_item_expandable = syn_expandable,
      .cell_string = syn_cell};
  ns_outline_view_set_callbacks(outline, &without_should_expand, NULL);
  SYN_ASSERT_MSG(
      !syn_test_delegate_responds(outline, "outlineView:shouldExpandItem:"),
      "an unset optional member is reported as implemented");

  ns_table_view_reload_data(as_table);
  ns_outline_view_expand_item(outline, &syn_groups[0]);
  SYN_ASSERT_MSG(ns_outline_view_item_expanded(outline, &syn_groups[0]),
                 "an outline with no should_expand_item did not expand");
  SYN_ASSERT_MSG(syn_expand_questions == 0,
                 "the unset member was asked %d time(s)", syn_expand_questions);

  ns_outline_view_set_callbacks(outline, NULL, NULL);
  ns_release(outline);
  ns_release(scroll);
}

SYN_TEST(an_outline_whose_should_expand_item_says_no_stays_collapsed) {
  syn_test_bootstrap();
  syn_reset();
  syn_allow_expansion = false;

  ns_scroll_view *scroll = NULL;
  ns_outline_view *outline = syn_outline_create(&scroll);
  ns_table_view *as_table = ns_outline_view_as_table_view(outline);
  ns_outline_view_callbacks refusing = {
      .number_of_children_of_item = syn_children,
      .child_of_item = syn_child,
      .is_item_expandable = syn_expandable,
      .cell_string = syn_cell,
      .should_expand_item = syn_should_expand};
  ns_outline_view_set_callbacks(outline, &refusing, NULL);
  ns_table_view_reload_data(as_table);

  ns_outline_view_expand_item(outline, &syn_groups[0]);
  SYN_ASSERT_MSG(syn_expand_questions == 1,
                 "should_expand_item was asked %d time(s), not once",
                 syn_expand_questions);
  SYN_ASSERT_MSG(!ns_outline_view_item_expanded(outline, &syn_groups[0]),
                 "the group expanded although should_expand_item said no");
  SYN_ASSERT_MSG(ns_table_view_number_of_rows(as_table) == 2,
                 "the refused expansion still showed %ld rows",
                 ns_table_view_number_of_rows(as_table));

  /* With the same member saying yes, the same call expands. */
  syn_allow_expansion = true;
  ns_outline_view_expand_item(outline, &syn_groups[0]);
  SYN_ASSERT(ns_outline_view_item_expanded(outline, &syn_groups[0]));

  /* The collapse half of the pair: unset, the group closes; set to false, it
   * stays open, which is the sidebar whose headings never close (R15). */
  ns_outline_view_collapse_item(outline, &syn_groups[0]);
  SYN_ASSERT_MSG(!ns_outline_view_item_expanded(outline, &syn_groups[0]),
                 "an outline with no should_collapse_item did not collapse");

  ns_outline_view_callbacks pinned_open = refusing;
  pinned_open.should_collapse_item = syn_should_not_collapse;
  ns_outline_view_set_callbacks(outline, &pinned_open, NULL);
  ns_outline_view_expand_item(outline, &syn_groups[0]);
  ns_outline_view_collapse_item(outline, &syn_groups[0]);
  SYN_ASSERT_MSG(ns_outline_view_item_expanded(outline, &syn_groups[0]),
                 "the group collapsed although should_collapse_item said no");

  ns_outline_view_set_callbacks(outline, NULL, NULL);
  ns_release(outline);
  ns_release(scroll);
}

/* ---- group rows and the selection (R15, F6) ---- */

SYN_TEST(a_group_row_is_drawn_as_one_and_cannot_be_selected) {
  syn_test_bootstrap();
  syn_reset();
  int context = 13;

  ns_scroll_view *scroll = NULL;
  ns_outline_view *outline = syn_outline_create(&scroll);
  ns_table_view *as_table = ns_outline_view_as_table_view(outline);
  ns_outline_view_set_callbacks(outline, &syn_sidebar_callbacks, &context);
  ns_table_view_reload_data(as_table);
  ns_outline_view_expand_item_expand_children(outline, NULL, true);

  SYN_ASSERT_MSG(syn_test_delegate_responds(outline, "outlineView:isGroupItem:"),
                 "the group-item member is not installed");
  SYN_ASSERT_MSG(
      syn_test_delegate_responds(outline, "outlineView:shouldSelectItem:"),
      "the should-select member is not installed");
  /* Building the group's row is what asks the member (R15). */
  ns_table_view_view_at_column_row_make_if_necessary(as_table, 0, 0, true);
  SYN_ASSERT_MSG(syn_group_questions > 0,
                 "AppKit never asked whether the heading is a group row");

  /* The heading is a group row and refuses the selection; the row under it is
   * neither. This is the answer AppKit gets when a click asks - a programmatic
   * selection bypasses the question, which is AppKit's own rule. */
  SYN_ASSERT_MSG(
      syn_test_delegate_answers_about_row(outline, "outlineView:isGroupItem:", 0),
      "the heading did not answer as a group row");
  SYN_ASSERT_MSG(!syn_test_delegate_answers_about_row(
                     outline, "outlineView:shouldSelectItem:", 0),
                 "the heading offered itself for selection");
  SYN_ASSERT_MSG(!syn_test_delegate_answers_about_row(
                     outline, "outlineView:isGroupItem:", 1),
                 "the row under the heading answered as a group row");
  SYN_ASSERT_MSG(syn_test_delegate_answers_about_row(
                     outline, "outlineView:shouldSelectItem:", 1),
                 "the row under the heading refused the selection");

  ns_outline_view_select_item(outline, &syn_library[0]);
  SYN_WAIT_FOR(syn_selection_fires == 1, 1000);
  SYN_ASSERT_MSG(syn_selected_item == &syn_library[0],
                 "the callback saw a different item");
  SYN_ASSERT_MSG(syn_selection_sender == outline,
                 "the sender was not the outline that fired");
  SYN_ASSERT_MSG(syn_selection_context == &context,
                 "the context did not arrive");
  SYN_ASSERT_MSG(ns_outline_view_selected_item(outline) == &syn_library[0],
                 "the outline reports a different selected item");

  /* An item in no displayed row is no selection at all. */
  ns_outline_view_collapse_item(outline, &syn_groups[1]);
  SYN_ASSERT_MSG(ns_outline_view_row_for_item(outline, &syn_collections[0]) == -1,
                 "a collapsed child still reports a row");
  ns_outline_view_select_item(outline, &syn_collections[0]);
  syn_test_spin(50);
  SYN_ASSERT_MSG(ns_outline_view_selected_item(outline) == &syn_library[0],
                 "selecting a hidden item moved the selection");

  ns_table_view_deselect_all(as_table);
  SYN_WAIT_FOR(syn_selection_fires == 2, 1000);
  SYN_ASSERT_MSG(syn_selected_item == NULL,
                 "a cleared selection reported an item");

  ns_outline_view_set_callbacks(outline, NULL, NULL);
  ns_release(outline);
  ns_release(scroll);
}

/* R16: scrolling an item into view is the outline's half of the row version. */
SYN_TEST(scrolling_an_item_into_view_moves_the_scroll_views_visible_rect) {
  syn_test_bootstrap();
  syn_reset();

  ns_scroll_view *scroll =
      ns_scroll_view_create_with_frame(CGRectMake(0, 0, 220, 60));
  ns_outline_view *outline =
      ns_outline_view_create_with_frame(CGRectMake(0, 0, 220, 60));
  ns_table_column *column =
      ns_table_column_create_with_identifier("collection");
  ns_table_view *as_table = ns_outline_view_as_table_view(outline);
  ns_table_view_add_table_column(as_table, column);
  ns_outline_view_set_outline_table_column(outline, column);
  ns_release(column);
  ns_table_view_set_header_view(as_table, NULL);
  ns_scroll_view_set_document_view(scroll, ns_outline_view_as_view(outline));

  ns_outline_view_set_callbacks(outline, &syn_sidebar_callbacks, NULL);
  ns_table_view_reload_data(as_table);
  ns_outline_view_expand_item_expand_children(outline, NULL, true);

  CGRect before = ns_scroll_view_document_visible_rect(scroll);
  CGRect frame = ns_view_frame(ns_outline_view_as_view(outline));
  SYN_ASSERT_MSG(frame.size.height > before.size.height,
                 "six rows did not make the outline taller than its clip");

  ns_outline_view_scroll_item_to_visible(outline, &syn_collections[1]);
  CGRect after = ns_scroll_view_document_visible_rect(scroll);
  SYN_ASSERT_MSG(after.origin.y > before.origin.y,
                 "scrolling to the last item left the visible rect at y=%g",
                 after.origin.y);

  /* An item in no displayed row scrolls nothing. */
  ns_outline_view_collapse_item(outline, &syn_groups[0]);
  CGRect held = ns_scroll_view_document_visible_rect(scroll);
  ns_outline_view_scroll_item_to_visible(outline, &syn_library[0]);
  CGRect unchanged = ns_scroll_view_document_visible_rect(scroll);
  SYN_ASSERT_MSG(unchanged.origin.y == held.origin.y,
                 "scrolling to a hidden item moved the visible rect");

  ns_outline_view_set_callbacks(outline, NULL, NULL);
  ns_release(outline);
  ns_release(scroll);
}

/* ---- the row icon (R15, R23, KTD6) ---- */

/* The Swift sidebar puts an SF Symbol beside every row that is not a group
 * heading, and null from the member is what "no icon" is spelled with. */
static char syn_symbol_buffer[] = "folder";
static char syn_title_buffer[] = "All Items";
static int syn_symbol_questions;

static const char *syn_cell_symbol(void *context, ns_outline_view *sender,
                                   const void *item) {
  (void)context;
  (void)sender;
  syn_symbol_questions++;
  if (((const syn_node *)item)->child_count > 0) return NULL;
  return syn_symbol_buffer;
}

/* The one row whose text comes out of a buffer the test can scribble on. */
static const char *syn_cell_from_buffer(void *context, ns_outline_view *sender,
                                        const void *item) {
  (void)context;
  (void)sender;
  if (item == &syn_library[0]) return syn_title_buffer;
  return ((const syn_node *)item)->title;
}

static const ns_outline_view_callbacks syn_icon_callbacks = {
    .number_of_children_of_item = syn_children,
    .child_of_item = syn_child,
    .is_item_expandable = syn_expandable,
    .cell_string = syn_cell_from_buffer,
    .cell_symbol_name = syn_cell_symbol,
    .is_group_item = syn_is_group,
    .should_select_item = syn_should_select,
};

/* The cell view AppKit has for a row, without asking the struct for it again:
 * `make` false hands back the view that is already there. */
static ns_view *syn_cell_view_at(ns_outline_view *outline, long row,
                                 bool make) {
  return ns_table_view_view_at_column_row_make_if_necessary(
      ns_outline_view_as_table_view(outline), 0, row, make);
}

/* The icon shape is one stack view holding the image view and the label; the
 * text-only shape is the label alone. */
static ns_view *syn_icon_stack(ns_view *cell) {
  if (cell == NULL || ns_view_subview_count(cell) != 1) return NULL;
  ns_view *stack = ns_view_subview_at_index(cell, 0);
  if (strcmp(syn_test_class_name(stack), "NSStackView") != 0) return NULL;
  return stack;
}

SYN_TEST(a_row_with_a_symbol_builds_a_cell_holding_an_icon_and_the_text) {
  syn_test_bootstrap();
  syn_reset();
  syn_symbol_questions = 0;
  int context = 4;

  ns_scroll_view *scroll = NULL;
  ns_outline_view *outline = syn_outline_create(&scroll);
  ns_outline_view_set_callbacks(outline, &syn_icon_callbacks, &context);
  ns_table_view_reload_data(ns_outline_view_as_table_view(outline));
  ns_outline_view_expand_item_expand_children(outline, NULL, true);

  /* Row 0 is a group heading, which answers null and gets the text-only cell
   * an outline built before this member existed. */
  ns_view *heading = syn_cell_view_at(outline, 0, true);
  SYN_ASSERT_MSG(syn_icon_stack(heading) == NULL,
                 "a group heading was given an icon");
  SYN_ASSERT_MSG(ns_view_subview_count(heading) == 1,
                 "the heading cell holds %ld subviews",
                 ns_view_subview_count(heading));
  SYN_ASSERT_STR_EQ(syn_test_class_name(ns_view_subview_at_index(heading, 0)),
                    "NSTextField");

  /* Row 1 is a child, which answers with a symbol. */
  ns_view *row = syn_cell_view_at(outline, 1, true);
  ns_view *stack = syn_icon_stack(row);
  SYN_ASSERT_MSG(stack != NULL, "the child row was not given an icon");
  ns_stack_view *as_stack = (ns_stack_view *)(void *)stack;
  SYN_ASSERT_MSG(ns_stack_view_arranged_subview_count(as_stack) == 2,
                 "the icon cell holds %ld arranged subviews",
                 ns_stack_view_arranged_subview_count(as_stack));

  ns_view *icon = ns_stack_view_arranged_subview_at_index(as_stack, 0);
  ns_view *label = ns_stack_view_arranged_subview_at_index(as_stack, 1);
  SYN_ASSERT_STR_EQ(syn_test_class_name(icon), "NSImageView");
  SYN_ASSERT_STR_EQ(syn_test_class_name(label), "NSTextField");
  /* The Swift sidebar's numbers: 6 points between the icon and the text, both
   * centred on the row's middle. */
  SYN_ASSERT_MSG(ns_stack_view_spacing(as_stack) == 6,
                 "the icon sits %g points from the text",
                 ns_stack_view_spacing(as_stack));

  char *text = ns_control_copy_string_value(
      ns_text_field_as_control((ns_text_field *)(void *)label));
  SYN_ASSERT_STR_EQ(text, "All Items");
  ns_string_free(text);

  /* A real symbol resolves to a real image, so the icon has a size of its
   * own; the row's own text is what a screen reader reads for it (R23). */
  CGSize icon_size = ns_view_intrinsic_content_size(icon);
  SYN_ASSERT_MSG(icon_size.width > 0 && icon_size.height > 0,
                 "the icon has no size: %gx%g", icon_size.width,
                 icon_size.height);
  char *described = ns_view_copy_accessibility_label(icon);
  SYN_ASSERT_STR_EQ(described, "All Items");
  ns_string_free(described);

  SYN_ASSERT_MSG(syn_symbol_questions > 0,
                 "the symbol member was never asked");

  ns_outline_view_set_callbacks(outline, NULL, NULL);
  ns_release(outline);
  ns_release(scroll);
}

/* The member unset is the whole of v0 before this commit: one label, no
 * report, no image view. */
SYN_TEST(an_outline_with_the_symbol_member_unset_builds_the_cell_it_always_did) {
  syn_test_bootstrap();
  syn_reset();
  int context = 5;

  ns_scroll_view *scroll = NULL;
  ns_outline_view *outline = syn_outline_create(&scroll);
  ns_outline_view_set_callbacks(outline, &syn_sidebar_callbacks, &context);
  /* AppKit is told the delegate answers the cell selector either way, because
   * the required member that feeds it is set. */
  SYN_ASSERT_MSG(syn_test_delegate_responds(
                     outline, "outlineView:viewForTableColumn:item:"),
                 "the cell selector went missing with the symbol member unset");
  ns_table_view_reload_data(ns_outline_view_as_table_view(outline));
  ns_outline_view_expand_item_expand_children(outline, NULL, true);

  for (long row = 0; row < 2; row++) {
    ns_view *cell = syn_cell_view_at(outline, row, true);
    SYN_ASSERT_MSG(syn_icon_stack(cell) == NULL, "row %ld grew an icon", row);
    SYN_ASSERT_MSG(ns_view_subview_count(cell) == 1,
                   "row %ld's cell holds %ld subviews", row,
                   ns_view_subview_count(cell));
    SYN_ASSERT_STR_EQ(syn_test_class_name(ns_view_subview_at_index(cell, 0)),
                      "NSTextField");
  }

  char *first = syn_cell_text(outline, 1);
  SYN_ASSERT_STR_EQ(first, "All Items");
  ns_string_free(first);

  ns_outline_view_set_callbacks(outline, NULL, NULL);
  ns_release(outline);
  ns_release(scroll);
}

/* R11: both strings a cell is built from are borrowed for the callback and
 * copied before it returns, so scribbling over either afterwards changes
 * nothing the cell shows. */
SYN_TEST(scribbling_over_the_name_buffers_after_a_reload_changes_no_cell) {
  syn_test_bootstrap();
  syn_reset();
  int context = 6;

  ns_scroll_view *scroll = NULL;
  ns_outline_view *outline = syn_outline_create(&scroll);
  ns_outline_view_set_callbacks(outline, &syn_icon_callbacks, &context);
  ns_table_view_reload_data(ns_outline_view_as_table_view(outline));
  ns_outline_view_expand_item_expand_children(outline, NULL, true);

  ns_view *row = syn_cell_view_at(outline, 1, true);
  ns_stack_view *as_stack = (ns_stack_view *)(void *)syn_icon_stack(row);
  SYN_ASSERT_MSG(as_stack != NULL, "the child row was not given an icon");
  CGSize before = ns_view_intrinsic_content_size(
      ns_stack_view_arranged_subview_at_index(as_stack, 0));
  SYN_ASSERT_MSG(before.width > 0, "the icon had no image to begin with");

  memcpy(syn_symbol_buffer, "zzzzzz", 6);
  memcpy(syn_title_buffer, "ZZZZZZZZZ", 9);

  /* The same view AppKit already has, with the struct not asked again. */
  ns_view *again = syn_cell_view_at(outline, 1, false);
  SYN_ASSERT_MSG(again == row, "the outline rebuilt the row's cell");
  ns_view *label = ns_stack_view_arranged_subview_at_index(as_stack, 1);
  char *text = ns_control_copy_string_value(
      ns_text_field_as_control((ns_text_field *)(void *)label));
  SYN_ASSERT_STR_EQ(text, "All Items");
  ns_string_free(text);

  CGSize after = ns_view_intrinsic_content_size(
      ns_stack_view_arranged_subview_at_index(as_stack, 0));
  SYN_ASSERT_MSG(after.width == before.width && after.height == before.height,
                 "the icon changed size after the buffer was scribbled: "
                 "%gx%g became %gx%g",
                 before.width, before.height, after.width, after.height);

  memcpy(syn_symbol_buffer, "folder", 6);
  memcpy(syn_title_buffer, "All Items", 9);
  ns_outline_view_set_callbacks(outline, NULL, NULL);
  ns_release(outline);
  ns_release(scroll);
}

/* ---- teardown (R7, KTD8, KTD12) ---- */

SYN_TEST(tearing_down_a_window_holding_an_outline_releases_clean) {
  syn_test_bootstrap();
  syn_reset();
  int context = 1;

  ns_window *window =
      ns_window_create_with_content_rect_style_mask_backing_defer(
          CGRectMake(0, 0, 400, 300),
          (ns_window_style_mask)(NS_WINDOW_STYLE_MASK_TITLED |
                                 NS_WINDOW_STYLE_MASK_CLOSABLE),
          NS_BACKING_STORE_BUFFERED, false);
  ns_view_controller *controller = ns_view_controller_create();
  ns_view *pane = ns_view_create_with_frame(CGRectMake(0, 0, 400, 300));

  ns_scroll_view *scroll = NULL;
  ns_outline_view *outline = syn_outline_create(&scroll);
  ns_outline_view_set_callbacks(outline, &syn_sidebar_callbacks, &context);
  ns_table_view_reload_data(ns_outline_view_as_table_view(outline));
  ns_outline_view_expand_item_expand_children(outline, NULL, true);
  SYN_ASSERT_SHIMS(1);

  ns_view_add_subview(pane, ns_scroll_view_as_view(scroll));
  ns_view_controller_set_view(controller, pane);
  ns_window_set_content_view_controller(window, controller);

  ns_release(outline);
  ns_release(scroll);
  ns_release(pane);
  ns_release(controller);
  ns_window_close(window);
  /* The boxes and the shim go with the outline, which goes with the tree. */
  ns_release(window);
  SYN_ASSERT_SHIMS(0);
}

/* ---- the cases that abort, each run in a child (R9, AE1, KTD4, KTD8) ---- */

SYN_ABORT_CASE(outline_installed_without_child_of_item) {
  syn_test_bootstrap();
  syn_reset();
  ns_scroll_view *scroll = NULL;
  ns_outline_view *outline = syn_outline_create(&scroll);
  ns_outline_view_callbacks missing = {
      .number_of_children_of_item = syn_children,
      .is_item_expandable = syn_expandable,
      .cell_string = syn_cell};
  ns_outline_view_set_callbacks(outline, &missing, NULL);
}

/* AE1: the report names the missing member and the data source, before AppKit
 * has been asked for a single child. */
SYN_TEST(installing_without_child_of_item_names_it_and_the_data_source) {
  SYN_ASSERT_ABORTS("outline_installed_without_child_of_item", "child_of_item");
  SYN_ASSERT_ABORTS("outline_installed_without_child_of_item",
                    "NSOutlineViewDataSource + NSOutlineViewDelegate");
  SYN_ASSERT_ABORTS("outline_installed_without_child_of_item",
                    "ns_outline_view_set_callbacks");
}

static const void *syn_null_child(void *context, ns_outline_view *sender,
                                  long index, const void *item) {
  (void)context;
  (void)sender;
  (void)index;
  (void)item;
  return NULL;
}

SYN_ABORT_CASE(child_of_item_returning_null) {
  syn_test_bootstrap();
  syn_reset();
  ns_scroll_view *scroll = NULL;
  ns_outline_view *outline = syn_outline_create(&scroll);
  ns_outline_view_callbacks callbacks = {
      .number_of_children_of_item = syn_children,
      .child_of_item = syn_null_child,
      .is_item_expandable = syn_expandable,
      .cell_string = syn_cell};
  ns_outline_view_set_callbacks(outline, &callbacks, NULL);
  ns_table_view_reload_data(ns_outline_view_as_table_view(outline));
  ns_outline_view_item_at_row(outline, 0);
}

SYN_TEST(a_null_child_is_reported_with_the_member_named) {
  SYN_ASSERT_ABORTS("child_of_item_returning_null", "child_of_item");
  SYN_ASSERT_ABORTS("child_of_item_returning_null", "returned null");
  SYN_ASSERT_ABORTS("child_of_item_returning_null",
                    "NSOutlineViewDataSource + NSOutlineViewDelegate");
}

/* R11, KTD8: the same bytes at a wrapper parameter are reported and stop the
 * process, so a string a callback returns is held to the rule too rather than
 * being dropped for an empty cell. */
static const char *syn_malformed_cell(void *context, ns_outline_view *sender,
                                      const void *item) {
  (void)context;
  (void)sender;
  (void)item;
  return "\xff\xfe not utf-8";
}

SYN_ABORT_CASE(cell_string_returning_malformed_utf8) {
  syn_test_bootstrap();
  syn_reset();
  ns_scroll_view *scroll = NULL;
  ns_outline_view *outline = syn_outline_create(&scroll);
  ns_outline_view_callbacks callbacks = {
      .number_of_children_of_item = syn_children,
      .child_of_item = syn_child,
      .is_item_expandable = syn_expandable,
      .cell_string = syn_malformed_cell};
  ns_outline_view_set_callbacks(outline, &callbacks, NULL);
  ns_table_view_reload_data(ns_outline_view_as_table_view(outline));
  ns_table_view_view_at_column_row_make_if_necessary(
      ns_outline_view_as_table_view(outline), 0, 0, true);
}

SYN_ABORT_CASE(cell_symbol_name_returning_malformed_utf8) {
  syn_test_bootstrap();
  syn_reset();
  ns_scroll_view *scroll = NULL;
  ns_outline_view *outline = syn_outline_create(&scroll);
  ns_outline_view_callbacks callbacks = {
      .number_of_children_of_item = syn_children,
      .child_of_item = syn_child,
      .is_item_expandable = syn_expandable,
      .cell_string = syn_cell,
      .cell_symbol_name = syn_malformed_cell};
  ns_outline_view_set_callbacks(outline, &callbacks, NULL);
  ns_table_view_reload_data(ns_outline_view_as_table_view(outline));
  ns_table_view_view_at_column_row_make_if_necessary(
      ns_outline_view_as_table_view(outline), 0, 0, true);
}

SYN_TEST(a_malformed_string_from_a_callback_names_the_member_and_the_rule) {
  SYN_ASSERT_ABORTS("cell_string_returning_malformed_utf8", "cell_string");
  SYN_ASSERT_ABORTS("cell_string_returning_malformed_utf8", "not valid UTF-8");
  SYN_ASSERT_ABORTS("cell_symbol_name_returning_malformed_utf8",
                    "cell_symbol_name");
  SYN_ASSERT_ABORTS("cell_symbol_name_returning_malformed_utf8",
                    "not valid UTF-8");
}

SYN_ABORT_CASE(an_outline_passed_where_a_column_belongs) {
  syn_test_bootstrap();
  ns_scroll_view *scroll = NULL;
  ns_outline_view *outline = syn_outline_create(&scroll);
  ns_table_column_copy_title((ns_table_column *)(void *)outline);
}

SYN_TEST(a_handle_of_the_wrong_class_names_both_classes) {
  SYN_ASSERT_ABORTS("an_outline_passed_where_a_column_belongs",
                    "NSTableColumn");
  SYN_ASSERT_ABORTS("an_outline_passed_where_a_column_belongs",
                    "ns_table_column_copy_title");
}

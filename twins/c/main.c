/*
 * The C twin of twins/swift (R22, AE6, AE7).
 *
 * One C file against one header, `syntonic.h`, mirroring the Swift twin file
 * for file: the geometry and the model first, then the sidebar, the list and
 * detail pane, the Settings window, the main window and toolbar, and the
 * application callbacks last. The Swift twin is the reference; every number,
 * every string and every order below comes from it.
 *
 * Build and run it:
 *
 *     just twin-c
 *
 * The environment the comparison protocol sets:
 *   SYNTONIC_TWIN_DUMP  path the layout report is written to after every event
 *                       this twin handles that changes the layout - selection,
 *                       an edit, add, delete, search, the sidebar toggle, and
 *                       the window moving or resizing; docs/twin-comparison.md
 *                       diffs it against the Swift twin's
 *   SYNTONIC_TWIN_APPEARANCE  read by the Swift twin only: Syntonic wraps no
 *                       NSAppearance, so this twin follows the system theme
 */

#include <syntonic/syntonic.h>

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "seed_data.h"

/* MARK: - Geometry (twins/swift/MainWindowController.swift, TwinGeometry) */

/* Both twins share these numbers; docs/twin-comparison.md records them as the
 * comparison's fixed geometry. */
#define TWIN_APP_NAME "Syntonic Twin"
#define TWIN_CONTENT_WIDTH 1000.0
#define TWIN_CONTENT_HEIGHT 640.0
#define TWIN_FRAME_ORIGIN_X 200.0
#define TWIN_FRAME_ORIGIN_Y 200.0
#define TWIN_MIN_CONTENT_WIDTH 720.0
#define TWIN_MIN_CONTENT_HEIGHT 480.0
#define TWIN_SIDEBAR_WIDTH 220.0
#define TWIN_LIST_WIDTH 400.0
#define TWIN_SETTINGS_WIDTH 520.0
#define TWIN_SETTINGS_HEIGHT 300.0
#define TWIN_SETTINGS_ORIGIN_X 420.0
#define TWIN_SETTINGS_ORIGIN_Y 420.0
#define TWIN_DETAIL_INSET 20.0
#define TWIN_DETAIL_FIELD_WIDTH 220.0

#define TWIN_TOOLBAR_IDENTIFIER "dev.kaino.syntonic.twin.toolbar"
#define TWIN_ADD_ITEM_IDENTIFIER "dev.kaino.syntonic.twin.add"
#define TWIN_SEARCH_ITEM_IDENTIFIER "dev.kaino.syntonic.twin.search"

/* MARK: - The twin
 *
 * Swift spreads this across five reference types; C keeps one struct and hands
 * its address to every callback as the context. It is allocated in main and
 * freed in will_terminate - never in will_close, because closing a window only
 * orders it out and the app is still running (F1, KTD7).
 */

typedef struct twin {
  /* model */
  twin_item *items;
  long item_count;
  long item_capacity;
  long *visible; /* indices into `items`, in display order */
  long visible_count;
  const char *collection;
  char search_text[TWIN_TEXT_MAX];

  /* sidebar */
  ns_outline_view *sidebar_outline;
  ns_scroll_view *sidebar_scroll;
  ns_view *sidebar_root;
  ns_view_controller *sidebar_controller;

  /* list */
  ns_table_view *list_table;
  ns_scroll_view *list_scroll;
  ns_view *list_root;
  ns_view_controller *list_controller;

  /* detail */
  ns_grid_view *detail_grid;
  ns_text_field *detail_header;
  ns_text_field *detail_title;
  ns_text_field *detail_owner;
  ns_pop_up_button *detail_category;
  ns_button *detail_flagged;
  ns_button *detail_delete;
  ns_view *detail_root;
  ns_view_controller *detail_controller;
  long detail_item; /* index into `items`, or -1 for no selection */

  /* window and toolbar */
  ns_window *window;
  ns_split_view_controller *split_controller;
  ns_toolbar *toolbar;
  ns_search_toolbar_item *search_item;
  ns_search_field *search_field;
  ns_menu *menu_bar; /* borrowed: the application holds it */
  bool dividers_placed;

  /* Settings, built the first time the menu item asks for it */
  ns_window *settings_window;
  ns_tab_view_controller *settings_tabs;
  ns_view_controller *settings_panes[3];
  ns_view *settings_pane_views[3];
  ns_grid_view *settings_grids[3];
} twin;

static void twin_write_layout_report(twin *t);

/* MARK: - Model (twins/swift/ListDetailViewController.swift, ListViewController)
 */

/* The Swift twin filters with localizedCaseInsensitiveContains; the seed list
 * is ASCII, where this agrees with it. */
static bool twin_contains_ci(const char *haystack, const char *needle) {
  if (needle[0] == '\0') {
    return true;
  }
  for (const char *at = haystack; *at != '\0'; at++) {
    const char *left = at;
    const char *right = needle;
    while (*right != '\0' &&
           tolower((unsigned char)*left) == tolower((unsigned char)*right)) {
      left++;
      right++;
    }
    if (*right == '\0') {
      return true;
    }
  }
  return false;
}

static bool twin_matches(const twin *t, const twin_item *item) {
  if (strcmp(t->collection, TWIN_ALL_ITEMS) != 0 &&
      strcmp(item->collection, t->collection) != 0) {
    return false;
  }
  if (t->search_text[0] == '\0') {
    return true;
  }
  return twin_contains_ci(item->title, t->search_text) ||
         twin_contains_ci(item->owner, t->search_text);
}

static void twin_rebuild_visible(twin *t) {
  t->visible_count = 0;
  for (long index = 0; index < t->item_count; index++) {
    if (twin_matches(t, &t->items[index])) {
      t->visible[t->visible_count++] = index;
    }
  }
}

/* The row the table has selected, as an index into `items`; -1 for none. */
static long twin_selected_item(const twin *t) {
  long row = ns_table_view_selected_row(t->list_table);
  if (row < 0 || row >= t->visible_count) {
    return -1;
  }
  return t->visible[row];
}

static long twin_row_for_item(const twin *t, long item) {
  for (long row = 0; row < t->visible_count; row++) {
    if (t->visible[row] == item) {
      return row;
    }
  }
  return -1;
}

static void twin_show_detail(twin *t, long item);

static void twin_refilter(twin *t, bool keeping_selection) {
  long selected = keeping_selection ? twin_selected_item(t) : -1;
  twin_rebuild_visible(t);
  ns_table_view_reload_data(t->list_table);
  long row = selected >= 0 ? twin_row_for_item(t, selected) : -1;
  if (row >= 0) {
    ns_table_view_select_row_by_extending_selection(t->list_table, row, false);
  } else {
    ns_table_view_deselect_all(t->list_table);
    twin_show_detail(t, -1);
  }
}

/* Add appends a row to the current collection and selects it (R16). */
static void twin_add_item(twin *t) {
  if (t->item_count == t->item_capacity) {
    long capacity = t->item_capacity * 2;
    /* Each result is committed the moment it succeeds: realloc has already
     * released the pointer it was handed, so freeing the other array on a
     * partial failure would leave a dangling one in the struct. One array
     * short leaves both valid at the old capacity and this Add a no-op. */
    twin_item *grown = realloc(t->items, (size_t)capacity * sizeof *grown);
    if (grown != NULL) {
      t->items = grown;
    }
    long *grown_visible =
        realloc(t->visible, (size_t)capacity * sizeof *grown_visible);
    if (grown_visible != NULL) {
      t->visible = grown_visible;
    }
    if (grown == NULL || grown_visible == NULL) {
      return;
    }
    t->item_capacity = capacity;
  }

  const char *target = strcmp(t->collection, TWIN_ALL_ITEMS) == 0
                           ? TWIN_DEFAULT_COLLECTION
                           : t->collection;
  twin_item *item = &t->items[t->item_count++];
  snprintf(item->title, sizeof item->title, "%s", TWIN_NEW_ITEM_TITLE);
  snprintf(item->owner, sizeof item->owner, "%s", TWIN_NEW_ITEM_OWNER);
  item->category = TWIN_NEW_ITEM_CATEGORY;
  item->flagged = false;
  item->collection = target;

  twin_rebuild_visible(t);
  ns_table_view_reload_data(t->list_table);
  long row = twin_row_for_item(t, t->item_count - 1);
  if (row < 0) {
    return;
  }
  ns_table_view_select_row_by_extending_selection(t->list_table, row, false);
  ns_table_view_scroll_row_to_visible(t->list_table, row);
}

static void twin_delete_selected_item(twin *t) {
  long selected = twin_selected_item(t);
  if (selected < 0) {
    return;
  }
  memmove(&t->items[selected], &t->items[selected + 1],
          (size_t)(t->item_count - selected - 1) * sizeof *t->items);
  t->item_count--;
  twin_rebuild_visible(t);
  ns_table_view_reload_data(t->list_table);
  ns_table_view_deselect_all(t->list_table);
  twin_show_detail(t, -1);
}

/* MARK: - Sidebar (twins/swift/SidebarViewController.swift) */

static long twin_sidebar_child_count(void *context, ns_outline_view *sender,
                                     const void *item) {
  (void)context;
  (void)sender;
  return item == NULL ? TWIN_GROUP_COUNT
                      : ((const twin_sidebar_node *)item)->child_count;
}

static const void *twin_sidebar_child(void *context, ns_outline_view *sender,
                                      long index, const void *item) {
  (void)context;
  (void)sender;
  return item == NULL ? &TWIN_GROUPS[index]
                      : &((const twin_sidebar_node *)item)->children[index];
}

static bool twin_sidebar_is_group(void *context, ns_outline_view *sender,
                                  const void *item) {
  (void)context;
  (void)sender;
  return ((const twin_sidebar_node *)item)->child_count > 0;
}

static bool twin_sidebar_should_select(void *context, ns_outline_view *sender,
                                       const void *item) {
  return !twin_sidebar_is_group(context, sender, item);
}

/* The three groups are pinned open. */
static bool twin_sidebar_should_collapse(void *context,
                                         ns_outline_view *sender,
                                         const void *item) {
  (void)context;
  (void)sender;
  (void)item;
  return false;
}

static const char *twin_sidebar_cell_string(void *context,
                                            ns_outline_view *sender,
                                            const void *item) {
  (void)context;
  (void)sender;
  return ((const twin_sidebar_node *)item)->title;
}

/* A group heading carries no icon, which is the null this answers with. */
static const char *twin_sidebar_cell_symbol(void *context,
                                            ns_outline_view *sender,
                                            const void *item) {
  const twin_sidebar_node *node = item;
  return twin_sidebar_is_group(context, sender, item) ? NULL : node->symbol;
}

static void twin_sidebar_selection_changed(void *context,
                                           ns_outline_view *sender,
                                           const void *item) {
  (void)sender;
  twin *t = context;
  if (item == NULL) {
    return;
  }
  const twin_sidebar_node *node = item;
  if (node->child_count > 0) {
    return;
  }
  t->collection = node->title;
  twin_refilter(t, false);
  twin_write_layout_report(t);
}

/* A source-list outline inside the split view's sidebar item. The sidebar item
 * supplies the material, so there is no visual effect view here (R15). */
static void twin_build_sidebar(twin *t) {
  t->sidebar_root = ns_view_create_with_frame(
      CGRectMake(0, 0, TWIN_SIDEBAR_WIDTH, TWIN_CONTENT_HEIGHT));
  t->sidebar_outline = ns_outline_view_create_with_frame(
      CGRectMake(0, 0, TWIN_SIDEBAR_WIDTH, TWIN_CONTENT_HEIGHT));

  ns_table_view *as_table = ns_outline_view_as_table_view(t->sidebar_outline);
  ns_table_column *column = ns_table_column_create_with_identifier("collection");
  ns_table_view_add_table_column(as_table, column);
  ns_outline_view_set_outline_table_column(t->sidebar_outline, column);
  ns_release(column);
  ns_table_view_set_header_view(as_table, NULL);
  ns_table_view_set_style(as_table, NS_TABLE_VIEW_STYLE_SOURCE_LIST);
  ns_table_view_set_floats_group_rows(as_table, false);
  ns_table_view_set_allows_empty_selection(as_table, false);

  static const ns_outline_view_callbacks callbacks = {
      .number_of_children_of_item = twin_sidebar_child_count,
      .child_of_item = twin_sidebar_child,
      .is_item_expandable = twin_sidebar_is_group,
      .cell_string = twin_sidebar_cell_string,
      .cell_symbol_name = twin_sidebar_cell_symbol,
      .should_collapse_item = twin_sidebar_should_collapse,
      .is_group_item = twin_sidebar_is_group,
      .should_select_item = twin_sidebar_should_select,
      .selection_did_change = twin_sidebar_selection_changed,
  };
  ns_outline_view_set_callbacks(t->sidebar_outline, &callbacks, t);

  t->sidebar_scroll = ns_scroll_view_create_with_frame(
      CGRectMake(0, 0, TWIN_SIDEBAR_WIDTH, TWIN_CONTENT_HEIGHT));
  ns_scroll_view_set_document_view(t->sidebar_scroll,
                                   ns_outline_view_as_view(t->sidebar_outline));
  ns_scroll_view_set_has_vertical_scroller(t->sidebar_scroll, true);
  ns_scroll_view_set_draws_background(t->sidebar_scroll, false);
  ns_scroll_view_set_autohides_scrollers(t->sidebar_scroll, true);
  ns_layout_pin_edges(ns_scroll_view_as_view(t->sidebar_scroll),
                      t->sidebar_root, 0);

  t->sidebar_controller = ns_view_controller_create();
  ns_view_controller_set_view(t->sidebar_controller, t->sidebar_root);

  ns_table_view_reload_data(as_table);
  ns_outline_view_expand_item_expand_children(t->sidebar_outline, NULL, true);
  /* Row 0 is the first group header; "All Items" is row 1. */
  ns_outline_view_select_item(t->sidebar_outline, &TWIN_LIBRARY_CHILDREN[0]);
}

/* MARK: - List (twins/swift/ListDetailViewController.swift, ListViewController)
 */

static long twin_list_row_count(void *context, ns_table_view *sender) {
  (void)sender;
  return ((twin *)context)->visible_count;
}

static const char *twin_list_cell_string(void *context, ns_table_view *sender,
                                         long column, long row) {
  (void)sender;
  twin *t = context;
  const twin_item *item = &t->items[t->visible[row]];
  if (column == 0) {
    return item->title;
  }
  if (column == 1) {
    return item->owner;
  }
  return item->category;
}

static void twin_list_selection_changed(void *context, ns_table_view *sender,
                                        long row) {
  (void)sender;
  twin *t = context;
  twin_show_detail(t, row >= 0 && row < t->visible_count ? t->visible[row] : -1);
  twin_write_layout_report(t);
}

static void twin_add_column(ns_table_view *table, const char *identifier,
                            const char *title, CGFloat width) {
  ns_table_column *column = ns_table_column_create_with_identifier(identifier);
  ns_table_column_set_title(column, title);
  ns_table_column_set_width(column, width);
  ns_table_column_set_min_width(column, 60);
  ns_table_view_add_table_column(table, column);
  ns_release(column);
}

/* The list pane: a view-based table fed by a data source (R17). */
static void twin_build_list(twin *t) {
  t->list_root = ns_view_create_with_frame(
      CGRectMake(0, 0, TWIN_LIST_WIDTH, TWIN_CONTENT_HEIGHT));
  t->list_table = ns_table_view_create_with_frame(
      CGRectMake(0, 0, TWIN_LIST_WIDTH, TWIN_CONTENT_HEIGHT));

  twin_add_column(t->list_table, "title", "Title", 170);
  twin_add_column(t->list_table, "owner", "Owner", 120);
  twin_add_column(t->list_table, "category", "Category", 90);
  ns_table_view_set_style(t->list_table, NS_TABLE_VIEW_STYLE_INSET);
  ns_table_view_set_uses_alternating_row_background_colors(t->list_table, true);
  ns_table_view_set_allows_empty_selection(t->list_table, true);
  ns_table_view_set_column_autoresizing_style(
      t->list_table, NS_TABLE_VIEW_LAST_COLUMN_ONLY_AUTORESIZING_STYLE);

  static const ns_table_view_callbacks callbacks = {
      .number_of_rows = twin_list_row_count,
      .cell_string = twin_list_cell_string,
      .selection_did_change = twin_list_selection_changed,
  };
  ns_table_view_set_callbacks(t->list_table, &callbacks, t);

  t->list_scroll = ns_scroll_view_create_with_frame(
      CGRectMake(0, 0, TWIN_LIST_WIDTH, TWIN_CONTENT_HEIGHT));
  ns_scroll_view_set_document_view(t->list_scroll,
                                   ns_table_view_as_view(t->list_table));
  ns_scroll_view_set_has_vertical_scroller(t->list_scroll, true);
  ns_scroll_view_set_autohides_scrollers(t->list_scroll, true);
  ns_layout_pin_edges(ns_scroll_view_as_view(t->list_scroll), t->list_root, 0);

  t->list_controller = ns_view_controller_create();
  ns_view_controller_set_view(t->list_controller, t->list_root);

  twin_refilter(t, false);
}

/* MARK: - Detail (twins/swift/ListDetailViewController.swift,
 * DetailViewController) */

static void twin_show_detail(twin *t, long item) {
  t->detail_item = item;
  bool enabled = item >= 0;
  const twin_item *row = enabled ? &t->items[item] : NULL;

  ns_control *header = ns_text_field_as_control(t->detail_header);
  ns_control_set_string_value(header, enabled ? "Details" : "No Selection");
  ns_text_field_set_text_color(t->detail_header,
                               enabled ? ns_color_label_color()
                                       : ns_color_secondary_label_color());
  ns_control_set_string_value(ns_text_field_as_control(t->detail_title),
                              enabled ? row->title : "");
  ns_control_set_string_value(ns_text_field_as_control(t->detail_owner),
                              enabled ? row->owner : "");
  ns_pop_up_button_select_item_with_title(
      t->detail_category, enabled ? row->category : TWIN_CATEGORIES[0]);
  ns_button_set_state(t->detail_flagged, enabled && row->flagged
                                             ? NS_CONTROL_STATE_VALUE_ON
                                             : NS_CONTROL_STATE_VALUE_OFF);
  ns_control_set_enabled(ns_text_field_as_control(t->detail_title), enabled);
  ns_control_set_enabled(ns_text_field_as_control(t->detail_owner), enabled);
  ns_control_set_enabled(ns_pop_up_button_as_control(t->detail_category),
                         enabled);
  ns_control_set_enabled(ns_button_as_control(t->detail_flagged), enabled);
  ns_control_set_enabled(ns_button_as_control(t->detail_delete), enabled);
}

/* The table and the detail pane never disagree, so a commit reloads the row it
 * changed. Syntonic wraps no per-row reload, and a whole-table reload drops the
 * selection, so the row is selected again afterwards. */
static void twin_commit(twin *t) {
  long row = ns_table_view_selected_row(t->list_table);
  ns_table_view_reload_data(t->list_table);
  if (row >= 0 && row < t->visible_count) {
    ns_table_view_select_row_by_extending_selection(t->list_table, row, false);
  }
  twin_write_layout_report(t);
}

/* Text fields commit when editing ends: the field's action fires on Return, on
 * Tab and when the field loses focus (F6). */
static void twin_commit_text(void *context, const void *sender) {
  twin *t = context;
  if (t->detail_item < 0) {
    return;
  }
  twin_item *item = &t->items[t->detail_item];
  bool is_title = sender == (const void *)t->detail_title;
  char *value = ns_control_copy_string_value(ns_text_field_as_control(
      is_title ? t->detail_title : t->detail_owner));
  snprintf(is_title ? item->title : item->owner, TWIN_TEXT_MAX, "%s",
           value == NULL ? "" : value);
  ns_string_free(value);
  twin_commit(t);
}

static void twin_commit_category(void *context, const void *sender) {
  (void)sender;
  twin *t = context;
  if (t->detail_item < 0) {
    return;
  }
  char *title = ns_pop_up_button_copy_title_of_selected_item(t->detail_category);
  /* The pop-up's titles are the fixed category list, so the item keeps a
   * pointer into it rather than a copy of the title AppKit handed back. */
  t->items[t->detail_item].category = TWIN_CATEGORIES[0];
  for (long index = 0; index < TWIN_CATEGORY_COUNT; index++) {
    if (title != NULL && strcmp(title, TWIN_CATEGORIES[index]) == 0) {
      t->items[t->detail_item].category = TWIN_CATEGORIES[index];
    }
  }
  ns_string_free(title);
  twin_commit(t);
}

static void twin_commit_flagged(void *context, const void *sender) {
  (void)sender;
  twin *t = context;
  if (t->detail_item < 0) {
    return;
  }
  t->items[t->detail_item].flagged =
      ns_button_state(t->detail_flagged) == NS_CONTROL_STATE_VALUE_ON;
  twin_commit(t);
}

static void twin_delete_item(void *context, const void *sender) {
  (void)sender;
  twin *t = context;
  twin_delete_selected_item(t);
  twin_write_layout_report(t);
}

static void twin_add_label_row(ns_grid_view *grid, const char *label,
                               ns_view *control) {
  ns_text_field *caption = ns_text_field_create_label_with_string(label);
  ns_view *views[] = {ns_text_field_as_view(caption), control};
  ns_grid_view_add_row_with_views(grid, views, 2);
  ns_release(caption);
}

static void twin_add_trailing_row(ns_grid_view *grid, ns_view *control) {
  ns_view *views[] = {ns_grid_cell_empty_content_view(), control};
  ns_grid_view_add_row_with_views(grid, views, 2);
}

/* The detail pane: a grid form over the selected row (R17, F6). */
static void twin_build_detail(twin *t) {
  t->detail_root =
      ns_view_create_with_frame(CGRectMake(0, 0, 360, TWIN_CONTENT_HEIGHT));

  t->detail_header = ns_text_field_create_label_with_string("");
  t->detail_title = ns_text_field_create_with_string("");
  t->detail_owner = ns_text_field_create_with_string("");
  t->detail_category =
      ns_pop_up_button_create_with_frame_pulls_down(CGRectZero, false);
  t->detail_flagged = ns_button_create_checkbox_with_title("Flagged");
  t->detail_delete = ns_button_create_with_title("Delete");

  ns_font *bold =
      ns_font_copy_bold_system_font_of_size(ns_font_system_font_size());
  ns_control_set_font(ns_text_field_as_control(t->detail_header), bold);
  ns_release(bold);
  ns_control_set_action(ns_text_field_as_control(t->detail_title),
                        twin_commit_text, t);
  ns_control_set_action(ns_text_field_as_control(t->detail_owner),
                        twin_commit_text, t);
  ns_pop_up_button_add_items_with_titles(t->detail_category, TWIN_CATEGORIES,
                                         TWIN_CATEGORY_COUNT);
  ns_control_set_action(ns_pop_up_button_as_control(t->detail_category),
                        twin_commit_category, t);
  ns_control_set_action(ns_button_as_control(t->detail_flagged),
                        twin_commit_flagged, t);
  ns_button_set_bezel_style(t->detail_delete, NS_BEZEL_STYLE_PUSH);
  ns_control_set_action(ns_button_as_control(t->detail_delete),
                        twin_delete_item, t);
  ns_view_set_accessibility_label(ns_text_field_as_view(t->detail_title),
                                  "Title");
  ns_view_set_accessibility_label(ns_text_field_as_view(t->detail_owner),
                                  "Owner");
  ns_view_set_accessibility_label(ns_pop_up_button_as_view(t->detail_category),
                                  "Category");

  t->detail_grid = ns_grid_view_create_with_number_of_columns_rows(2, 0);
  ns_grid_view_set_row_spacing(t->detail_grid, 10);
  ns_grid_view_set_column_spacing(t->detail_grid, 12);

  ns_view *header_row[] = {ns_text_field_as_view(t->detail_header)};
  ns_grid_row *first = ns_grid_view_add_row_with_views(t->detail_grid,
                                                       header_row, 1);
  ns_grid_row_merge_cells_in_range(first, 0, 2);
  ns_grid_cell_set_x_placement(
      ns_grid_view_cell_at_column_index_row_index(t->detail_grid, 0, 0),
      NS_GRID_CELL_PLACEMENT_LEADING);
  twin_add_label_row(t->detail_grid, "Title:",
                     ns_text_field_as_view(t->detail_title));
  twin_add_label_row(t->detail_grid, "Owner:",
                     ns_text_field_as_view(t->detail_owner));
  twin_add_label_row(t->detail_grid, "Category:",
                     ns_pop_up_button_as_view(t->detail_category));
  twin_add_trailing_row(t->detail_grid, ns_button_as_view(t->detail_flagged));
  twin_add_trailing_row(t->detail_grid, ns_button_as_view(t->detail_delete));

  ns_grid_column_set_x_placement(
      ns_grid_view_column_at_index(t->detail_grid, 0),
      NS_GRID_CELL_PLACEMENT_TRAILING);
  ns_grid_column_set_x_placement(
      ns_grid_view_column_at_index(t->detail_grid, 1),
      NS_GRID_CELL_PLACEMENT_LEADING);
  ns_grid_column_set_width(ns_grid_view_column_at_index(t->detail_grid, 1),
                           TWIN_DETAIL_FIELD_WIDTH);
  ns_grid_cell_set_x_placement(
      ns_grid_view_cell_for_view(t->detail_grid,
                                 ns_text_field_as_view(t->detail_title)),
      NS_GRID_CELL_PLACEMENT_FILL);
  ns_grid_cell_set_x_placement(
      ns_grid_view_cell_for_view(t->detail_grid,
                                 ns_text_field_as_view(t->detail_owner)),
      NS_GRID_CELL_PLACEMENT_FILL);
  ns_grid_cell_set_x_placement(
      ns_grid_view_cell_for_view(t->detail_grid,
                                 ns_pop_up_button_as_view(t->detail_category)),
      NS_GRID_CELL_PLACEMENT_FILL);
  ns_layout_pin_top_edges(ns_grid_view_as_view(t->detail_grid), t->detail_root,
                          TWIN_DETAIL_INSET);

  t->detail_controller = ns_view_controller_create();
  ns_view_controller_set_view(t->detail_controller, t->detail_root);
  twin_show_detail(t, -1);
}

/* MARK: - Settings (twins/swift/SettingsViewController.swift) */

/* Each pane's Tab order is chained by hand so both twins walk the controls in
 * construction order. */
static void twin_chain(ns_view *const *chain, long count) {
  for (long index = 0; index < count; index++) {
    ns_view_set_next_key_view(chain[index], chain[(index + 1) % count]);
  }
}

static void twin_build_settings_pane(twin *t, long pane) {
  ns_view *root = ns_view_create_with_frame(
      CGRectMake(0, 0, TWIN_SETTINGS_WIDTH, TWIN_SETTINGS_HEIGHT));
  ns_grid_view *grid = ns_grid_view_create_with_number_of_columns_rows(2, 0);
  ns_grid_view_set_row_spacing(grid, 10);
  ns_grid_view_set_column_spacing(grid, 12);
  ns_view *chain[3] = {NULL, NULL, NULL};
  ns_view *fill = NULL;

  if (pane == 0) {
    ns_text_field *name_field = ns_text_field_create_with_string("Syntonic");
    ns_pop_up_button *pop_up =
        ns_pop_up_button_create_with_frame_pulls_down(CGRectZero, false);
    static const char *const opens_with[] = {"All Items", "Inbox", "Favorites"};
    ns_pop_up_button_add_items_with_titles(pop_up, opens_with, 3);
    ns_button *checkbox =
        ns_button_create_checkbox_with_title("Reopen the last collection");
    ns_button_set_state(checkbox, NS_CONTROL_STATE_VALUE_ON);
    twin_add_label_row(grid, "Workspace:", ns_text_field_as_view(name_field));
    twin_add_label_row(grid, "Opens with:", ns_pop_up_button_as_view(pop_up));
    twin_add_trailing_row(grid, ns_button_as_view(checkbox));
    fill = ns_text_field_as_view(name_field);
    chain[0] = fill;
    chain[1] = ns_pop_up_button_as_view(pop_up);
    chain[2] = ns_button_as_view(checkbox);
    ns_release(name_field);
    ns_release(pop_up);
    ns_release(checkbox);
  } else if (pane == 1) {
    ns_pop_up_button *theme =
        ns_pop_up_button_create_with_frame_pulls_down(CGRectZero, false);
    static const char *const themes[] = {"System", "Light", "Dark"};
    ns_pop_up_button_add_items_with_titles(theme, themes, 3);
    ns_pop_up_button *density =
        ns_pop_up_button_create_with_frame_pulls_down(CGRectZero, false);
    static const char *const densities[] = {"Comfortable", "Compact"};
    ns_pop_up_button_add_items_with_titles(density, densities, 2);
    ns_button *checkbox =
        ns_button_create_checkbox_with_title("Show the flagged badge");
    twin_add_label_row(grid, "Theme:", ns_pop_up_button_as_view(theme));
    twin_add_label_row(grid, "Row height:", ns_pop_up_button_as_view(density));
    twin_add_trailing_row(grid, ns_button_as_view(checkbox));
    chain[0] = ns_pop_up_button_as_view(theme);
    chain[1] = ns_pop_up_button_as_view(density);
    chain[2] = ns_button_as_view(checkbox);
    ns_release(theme);
    ns_release(density);
    ns_release(checkbox);
  } else {
    ns_text_field *path_field =
        ns_text_field_create_with_string("~/Library/Syntonic");
    ns_button *checkbox =
        ns_button_create_checkbox_with_title("Log layout reports");
    ns_button *reset = ns_button_create_with_title("Reset Settings");
    ns_button_set_bezel_style(reset, NS_BEZEL_STYLE_PUSH);
    twin_add_label_row(grid, "Scratch folder:",
                       ns_text_field_as_view(path_field));
    twin_add_trailing_row(grid, ns_button_as_view(checkbox));
    twin_add_trailing_row(grid, ns_button_as_view(reset));
    fill = ns_text_field_as_view(path_field);
    chain[0] = fill;
    chain[1] = ns_button_as_view(checkbox);
    chain[2] = ns_button_as_view(reset);
    ns_release(path_field);
    ns_release(checkbox);
    ns_release(reset);
  }

  if (fill != NULL) {
    ns_grid_cell_set_x_placement(ns_grid_view_cell_for_view(grid, fill),
                                 NS_GRID_CELL_PLACEMENT_FILL);
  }
  ns_grid_column_set_x_placement(ns_grid_view_column_at_index(grid, 0),
                                 NS_GRID_CELL_PLACEMENT_TRAILING);
  ns_grid_column_set_x_placement(ns_grid_view_column_at_index(grid, 1),
                                 NS_GRID_CELL_PLACEMENT_LEADING);
  ns_grid_column_set_width(ns_grid_view_column_at_index(grid, 1),
                           TWIN_DETAIL_FIELD_WIDTH);
  ns_layout_pin_top_edges(ns_grid_view_as_view(grid), root, TWIN_DETAIL_INSET);
  twin_chain(chain, 3);

  t->settings_panes[pane] = ns_view_controller_create();
  ns_view_controller_set_view(t->settings_panes[pane], root);
  t->settings_pane_views[pane] = root;
  t->settings_grids[pane] = grid;
}

/* A separate window with toolbar-style tabs, opened by the application menu's
 * Settings item (R19). */
static void twin_build_settings(twin *t) {
  static const char *const labels[] = {"General", "Appearance", "Advanced"};
  static const char *const symbols[] = {"gearshape", "paintpalette",
                                        "slider.horizontal.3"};

  t->settings_tabs = ns_tab_view_controller_create();
  ns_tab_view_controller_set_tab_style(
      t->settings_tabs, NS_TAB_VIEW_CONTROLLER_TAB_STYLE_TOOLBAR);
  for (long pane = 0; pane < 3; pane++) {
    twin_build_settings_pane(t, pane);
    ns_tab_view_item *tab =
        ns_tab_view_item_create_with_view_controller(t->settings_panes[pane]);
    ns_tab_view_item_set_label(tab, labels[pane]);
    ns_image *image =
        ns_image_create_with_system_symbol_name_accessibility_description(
            symbols[pane], labels[pane]);
    ns_tab_view_item_set_image(tab, image);
    ns_release(image);
    ns_tab_view_controller_add_tab_view_item(t->settings_tabs, tab);
    ns_release(tab);
  }

  t->settings_window = ns_window_create_with_content_rect_style_mask_backing_defer(
      CGRectMake(0, 0, TWIN_SETTINGS_WIDTH, TWIN_SETTINGS_HEIGHT),
      NS_WINDOW_STYLE_MASK_TITLED | NS_WINDOW_STYLE_MASK_CLOSABLE,
      NS_BACKING_STORE_BUFFERED, false);
  ns_window_set_restorable(t->settings_window, false);
  ns_window_set_autorecalculates_key_view_loop(t->settings_window, false);
  ns_window_set_content_view_controller(
      t->settings_window,
      ns_tab_view_controller_as_view_controller(t->settings_tabs));
  ns_window_set_toolbar_style(t->settings_window,
                              NS_WINDOW_TOOLBAR_STYLE_PREFERENCE);
  ns_window_set_frame_origin(
      t->settings_window,
      CGPointMake(TWIN_SETTINGS_ORIGIN_X, TWIN_SETTINGS_ORIGIN_Y));
}

/* AppKit has no responder-chain action for Settings, so the menu item calls
 * back into the app (KTD16). */
static void twin_show_settings(void *context, const void *sender) {
  (void)sender;
  twin *t = context;
  if (t->settings_window == NULL) {
    twin_build_settings(t);
  }
  ns_window_make_key_and_order_front(t->settings_window);
  ns_window_set_frame_origin(
      t->settings_window,
      CGPointMake(TWIN_SETTINGS_ORIGIN_X, TWIN_SETTINGS_ORIGIN_Y));
  printf("SYNTONIC_TWIN_WINDOW %ld Settings\n",
         ns_window_window_number(t->settings_window));
}

/* MARK: - Layout report (the text half of the U12 comparison)
 *
 * With SYNTONIC_TWIN_DUMP set, every layout-changing event overwrites that
 * file with the frames and layout priorities the capture script files beside
 * the screenshot. The format is twins/swift/MainWindowController.swift's,
 * line for line.
 */

static void twin_frame_line(FILE *out, const char *key, ns_view *view,
                            ns_view *reference) {
  CGRect rect = ns_view_convert_rect_to_view(view, ns_view_bounds(view),
                                             reference);
  fprintf(out, "%s.frame=%.1f,%.1f,%.1f,%.1f\n", key, rect.origin.x,
          rect.origin.y, rect.size.width, rect.size.height);
}

static void twin_describe(FILE *out, const char *key, ns_view *view,
                          ns_view *reference) {
  twin_frame_line(out, key, view, reference);
  CGSize intrinsic = ns_view_intrinsic_content_size(view);
  fprintf(out, "%s.intrinsic=%.1f,%.1f\n", key, intrinsic.width,
          intrinsic.height);
  fprintf(out, "%s.hug=%.0f,%.0f\n", key,
          (double)ns_view_content_hugging_priority_for_orientation(
              view, NS_LAYOUT_CONSTRAINT_ORIENTATION_HORIZONTAL),
          (double)ns_view_content_hugging_priority_for_orientation(
              view, NS_LAYOUT_CONSTRAINT_ORIENTATION_VERTICAL));
  fprintf(out, "%s.resist=%.0f,%.0f\n", key,
          (double)ns_view_content_compression_resistance_priority_for_orientation(
              view, NS_LAYOUT_CONSTRAINT_ORIENTATION_HORIZONTAL),
          (double)ns_view_content_compression_resistance_priority_for_orientation(
              view, NS_LAYOUT_CONSTRAINT_ORIENTATION_VERTICAL));
}

static void twin_write_layout_report(twin *t) {
  const char *path = getenv("SYNTONIC_TWIN_DUMP");
  ns_view *content =
      t->window == NULL ? NULL : ns_window_content_view(t->window);
  if (path == NULL || content == NULL) {
    return;
  }

  /* The capture script greps this file while the twin rewrites it, so a run
   * writes beside it and renames, the way Swift's atomic write does. */
  char temporary[1024];
  snprintf(temporary, sizeof temporary, "%s.tmp", path);
  FILE *out = fopen(temporary, "w");
  if (out == NULL) {
    return;
  }

  CGRect frame = ns_window_frame(t->window);
  CGRect bounds = ns_view_bounds(content);
  fprintf(out, "window.frame=%.1f,%.1f,%.1f,%.1f\n", frame.origin.x,
          frame.origin.y, frame.size.width, frame.size.height);
  fprintf(out, "window.content=%.1f,%.1f\n", bounds.size.width,
          bounds.size.height);
  ns_split_view_item *sidebar_item =
      ns_split_view_controller_split_view_item_at_index(t->split_controller, 0);
  fprintf(out, "sidebar.collapsed=%s\n",
          ns_split_view_item_collapsed(sidebar_item) ? "true" : "false");
  fprintf(out, "sidebar.width=%.1f\n",
          ns_view_frame(t->sidebar_root).size.width);
  twin_frame_line(out, "sidebar", t->sidebar_root, content);
  twin_frame_line(out, "list", t->list_root, content);
  twin_frame_line(out, "detail", t->detail_root, content);
  fprintf(out, "list.rows=%ld\n", t->visible_count);
  fprintf(out, "list.selectedRow=%ld\n",
          ns_table_view_selected_row(t->list_table));

  twin_frame_line(out, "detail.grid", ns_grid_view_as_view(t->detail_grid),
                  content);
  twin_describe(out, "detail.header", ns_text_field_as_view(t->detail_header),
                content);
  twin_describe(out, "detail.title", ns_text_field_as_view(t->detail_title),
                content);
  twin_describe(out, "detail.owner", ns_text_field_as_view(t->detail_owner),
                content);
  twin_describe(out, "detail.category",
                ns_pop_up_button_as_view(t->detail_category), content);
  twin_describe(out, "detail.flagged", ns_button_as_view(t->detail_flagged),
                content);
  twin_describe(out, "detail.delete", ns_button_as_view(t->detail_delete),
                content);

  char *header = ns_control_copy_string_value(
      ns_text_field_as_control(t->detail_header));
  char *title =
      ns_control_copy_string_value(ns_text_field_as_control(t->detail_title));
  char *owner =
      ns_control_copy_string_value(ns_text_field_as_control(t->detail_owner));
  char *category =
      ns_pop_up_button_copy_title_of_selected_item(t->detail_category);
  fprintf(out, "detail.header.value=%s\n", header == NULL ? "" : header);
  fprintf(out, "detail.title.value=%s\n", title == NULL ? "" : title);
  fprintf(out, "detail.owner.value=%s\n", owner == NULL ? "" : owner);
  fprintf(out, "detail.category.value=%s\n", category == NULL ? "" : category);
  fprintf(out, "detail.flagged.value=%s\n",
          ns_button_state(t->detail_flagged) == NS_CONTROL_STATE_VALUE_ON
              ? "true"
              : "false");
  fprintf(out, "detail.delete.enabled=%s\n",
          ns_control_enabled(ns_button_as_control(t->detail_delete)) ? "true"
                                                                    : "false");
  ns_string_free(header);
  ns_string_free(title);
  ns_string_free(owner);
  ns_string_free(category);

  fclose(out);
  rename(temporary, path);
}

/* MARK: - Window and toolbar (twins/swift/MainWindowController.swift) */

/* The toolbar's Add item and File - New Item both land here (R16). */
static void twin_add_new_item(void *context, const void *sender) {
  (void)sender;
  twin *t = context;
  if (t->search_field != NULL) {
    ns_control_set_string_value(ns_search_field_as_control(t->search_field), "");
  }
  t->search_text[0] = '\0';
  twin_refilter(t, true);
  twin_add_item(t);
  twin_write_layout_report(t);
}

static void twin_search_changed(void *context, const void *sender) {
  (void)sender;
  twin *t = context;
  char *value =
      ns_control_copy_string_value(ns_search_field_as_control(t->search_field));
  snprintf(t->search_text, sizeof t->search_text, "%s",
           value == NULL ? "" : value);
  ns_string_free(value);
  twin_refilter(t, true);
  twin_write_layout_report(t);
}

/* Edit - Find moves focus into the toolbar's search field, the way every macOS
 * app with a search toolbar item does. */
static void twin_begin_search(void *context, const void *sender) {
  (void)sender;
  twin *t = context;
  if (t->search_item != NULL) {
    ns_search_toolbar_item_begin_search_interaction(t->search_item);
  }
}

static ns_toolbar_item *twin_toolbar_item_for(void *context,
                                              ns_toolbar *sender,
                                              const char *identifier,
                                              bool inserted) {
  (void)sender;
  (void)inserted;
  twin *t = context;

  if (strcmp(identifier, TWIN_ADD_ITEM_IDENTIFIER) == 0) {
    ns_toolbar_item *item =
        ns_toolbar_item_create_with_item_identifier(identifier);
    ns_toolbar_item_set_label(item, "Add");
    ns_toolbar_item_set_palette_label(item, "Add");
    ns_toolbar_item_set_tool_tip(item, "Add an item");
    ns_image *image =
        ns_image_create_with_system_symbol_name_accessibility_description(
            "plus", "Add");
    ns_toolbar_item_set_image(item, image);
    ns_release(image);
    ns_toolbar_item_set_bordered(item, true);
    ns_toolbar_item_set_action(item, twin_add_new_item, t);
    return item;
  }

  if (strcmp(identifier, TWIN_SEARCH_ITEM_IDENTIFIER) == 0) {
    ns_search_toolbar_item *item =
        ns_search_toolbar_item_create_with_item_identifier(identifier);
    ns_search_toolbar_item_set_resigns_first_responder_with_cancel(item, true);
    ns_search_field *field = ns_search_toolbar_item_search_field(item);
    ns_search_field_set_sends_whole_search_string(field, false);
    ns_search_field_set_sends_search_string_immediately(field, true);
    ns_control_set_action(ns_search_field_as_control(field), twin_search_changed,
                          t);
    t->search_field = field;
    /* Edit - Find needs the item long after this call, and the library drops
     * the reference it returns once AppKit has taken one (R7, R9). */
    t->search_item = (ns_search_toolbar_item *)ns_retain(item);
    return ns_search_toolbar_item_as_toolbar_item(item);
  }

  return NULL;
}

/* Without a nib AppKit rebuilds the key view loop in geometric order, which is
 * not guaranteed to match construction order, so both twins turn the rebuild
 * off and chain Tab by hand. */
static void twin_chain_key_views(twin *t) {
  ns_view *chain[] = {
      ns_outline_view_as_view(t->sidebar_outline),
      ns_table_view_as_view(t->list_table),
      ns_text_field_as_view(t->detail_title),
      ns_text_field_as_view(t->detail_owner),
      ns_pop_up_button_as_view(t->detail_category),
      ns_button_as_view(t->detail_flagged),
      ns_button_as_view(t->detail_delete),
  };
  twin_chain(chain, 7);
  ns_window_set_initial_first_responder(t->window,
                                        ns_table_view_as_view(t->list_table));
}

static void twin_place_dividers(twin *t) {
  if (t->dividers_placed) {
    return;
  }
  t->dividers_placed = true;
  ns_split_view *split =
      ns_split_view_controller_split_view(t->split_controller);
  ns_split_view_set_position_of_divider_at_index(split, TWIN_SIDEBAR_WIDTH, 0);
  ns_split_view_set_position_of_divider_at_index(
      split, TWIN_SIDEBAR_WIDTH + TWIN_LIST_WIDTH, 1);
}

/* Moving a window does not relayout its views, so the report is refreshed by
 * hand: the capture script waits on these numbers. */
static void twin_window_did_move(void *context, ns_window *sender) {
  (void)sender;
  twin_write_layout_report(context);
}

static void twin_window_did_resize(void *context, ns_window *sender) {
  (void)sender;
  twin_write_layout_report(context);
}

static void twin_build_window(twin *t) {
  t->window = ns_window_create_with_content_rect_style_mask_backing_defer(
      CGRectMake(0, 0, TWIN_CONTENT_WIDTH, TWIN_CONTENT_HEIGHT),
      NS_WINDOW_STYLE_MASK_TITLED | NS_WINDOW_STYLE_MASK_CLOSABLE |
          NS_WINDOW_STYLE_MASK_MINIATURIZABLE | NS_WINDOW_STYLE_MASK_RESIZABLE,
      NS_BACKING_STORE_BUFFERED, false);
  ns_window_set_restorable(t->window, false);
  ns_window_set_autorecalculates_key_view_loop(t->window, false);
  ns_window_set_title(t->window, TWIN_APP_NAME);
  ns_window_set_content_min_size(
      t->window, CGSizeMake(TWIN_MIN_CONTENT_WIDTH, TWIN_MIN_CONTENT_HEIGHT));

  static const ns_window_callbacks window_callbacks = {
      .did_move = twin_window_did_move,
      .did_resize = twin_window_did_resize,
  };
  ns_window_set_callbacks(t->window, &window_callbacks, t);

  t->split_controller = ns_split_view_controller_create();
  ns_split_view_item *sidebar_item =
      ns_split_view_item_create_sidebar_with_view_controller(
          t->sidebar_controller);
  ns_split_view_item_set_minimum_thickness(sidebar_item, 180);
  ns_split_view_item_set_maximum_thickness(sidebar_item, 320);
  ns_split_view_item_set_can_collapse(sidebar_item, true);
  ns_split_view_controller_add_split_view_item(t->split_controller,
                                               sidebar_item);
  ns_release(sidebar_item);

  ns_split_view_item *list_item =
      ns_split_view_item_create_content_list_with_view_controller(
          t->list_controller);
  ns_split_view_item_set_minimum_thickness(list_item, 280);
  ns_split_view_controller_add_split_view_item(t->split_controller, list_item);
  ns_release(list_item);

  ns_split_view_item *detail_item =
      ns_split_view_item_create_with_view_controller(t->detail_controller);
  ns_split_view_item_set_minimum_thickness(detail_item, 280);
  ns_split_view_controller_add_split_view_item(t->split_controller, detail_item);
  ns_release(detail_item);

  ns_window_set_content_view_controller(
      t->window, ns_split_view_controller_as_view_controller(t->split_controller));

  t->toolbar = ns_toolbar_create_with_identifier(TWIN_TOOLBAR_IDENTIFIER);
  static const ns_toolbar_callbacks toolbar_callbacks = {
      .item_for_item_identifier_will_be_inserted_into_toolbar =
          twin_toolbar_item_for,
  };
  const char *const identifiers[] = {
      NS_TOOLBAR_TOGGLE_SIDEBAR_ITEM_IDENTIFIER,
      NS_TOOLBAR_SIDEBAR_TRACKING_SEPARATOR_ITEM_IDENTIFIER,
      TWIN_ADD_ITEM_IDENTIFIER,
      NS_TOOLBAR_FLEXIBLE_SPACE_ITEM_IDENTIFIER,
      TWIN_SEARCH_ITEM_IDENTIFIER,
  };
  ns_toolbar_set_callbacks(t->toolbar, &toolbar_callbacks, identifiers, 5,
                           identifiers, 5, t);
  ns_toolbar_set_allows_user_customization(t->toolbar, false);
  ns_toolbar_set_display_mode(t->toolbar, NS_TOOLBAR_DISPLAY_MODE_ICON_ONLY);
  ns_window_set_toolbar(t->window, t->toolbar);
  ns_window_set_toolbar_style(t->window, NS_WINDOW_TOOLBAR_STYLE_UNIFIED);
  ns_window_set_content_size(
      t->window, CGSizeMake(TWIN_CONTENT_WIDTH, TWIN_CONTENT_HEIGHT));
  ns_window_set_frame_origin(t->window,
                             CGPointMake(TWIN_FRAME_ORIGIN_X,
                                         TWIN_FRAME_ORIGIN_Y));
}

static void twin_show_window(twin *t) {
  ns_window_make_key_and_order_front(t->window);
  ns_window_set_content_size(
      t->window, CGSizeMake(TWIN_CONTENT_WIDTH, TWIN_CONTENT_HEIGHT));
  ns_window_set_frame_origin(t->window,
                             CGPointMake(TWIN_FRAME_ORIGIN_X,
                                         TWIN_FRAME_ORIGIN_Y));
  twin_place_dividers(t);
  twin_chain_key_views(t);
  ns_window_make_first_responder(t->window,
                                 ns_table_view_as_view(t->list_table));
  twin_write_layout_report(t);
  char *title = ns_window_copy_title(t->window);
  printf("SYNTONIC_TWIN_WINDOW %ld %s\n", ns_window_window_number(t->window),
         title == NULL ? "" : title);
  ns_string_free(title);
}

/* MARK: - Menu bar (twins/swift/AppDelegate.swift, R18, KTD16)
 *
 * ns_menu_bar_install_standard builds the six standard menus; the two items
 * the twin defines itself go in afterwards.
 */

static ns_menu *twin_submenu_at(ns_menu *menu_bar, long index) {
  return ns_menu_item_submenu(ns_menu_item_at_index(menu_bar, index));
}

/* The index of the item with this title, or -1. */
static long twin_index_of(ns_menu *menu, const char *title) {
  long count = ns_menu_number_of_items(menu);
  for (long index = 0; index < count; index++) {
    char *found = ns_menu_item_copy_title(ns_menu_item_at_index(menu, index));
    bool matched = found != NULL && strcmp(found, title) == 0;
    ns_string_free(found);
    if (matched) {
      return index;
    }
  }
  return -1;
}

/* AppKit puts AutoFill, Start Dictation and Emoji & Symbols at the end of any
 * menu titled "Edit" the moment the menu bar is installed, so an app's own
 * Edit item is inserted after the standard item it follows rather than
 * appended - appending would land it below the system's. */
static long twin_index_after(ns_menu *menu, const char *title) {
  long found = twin_index_of(menu, title);
  return found < 0 ? ns_menu_number_of_items(menu) : found + 1;
}

/* AppKit sends the standard View item's toggleSidebar: down the responder
 * chain, so nothing in this twin hears a sidebar toggle. The Swift twin
 * rewrites its report from viewDidLayout on every layout pass; Syntonic wraps
 * no layout callback, so the item is retargeted here instead and this is where
 * the toggle refreshes the report (R22). The toolbar's own toggle item still
 * goes through the chain and does not. */
static void twin_toggle_sidebar(void *context, const void *sender) {
  (void)sender;
  twin *t = context;
  ns_split_view_controller_toggle_sidebar(t->split_controller);
  twin_write_layout_report(t);
}

/* The menu bar is kept because Syntonic has no getter for the application's
 * main menu: what the installer returns is the only handle on it. */
static void twin_install_menu_bar(twin *t, ns_application *application) {
  ns_menu *menu_bar = ns_menu_bar_install_standard(application, TWIN_APP_NAME,
                                                   twin_show_settings, t);
  t->menu_bar = menu_bar;

  ns_menu *file_menu = twin_submenu_at(menu_bar, 1);
  ns_menu_item *new_item =
      ns_menu_item_create_with_title_key_equivalent("New Item", "n");
  ns_menu_item_set_action(new_item, twin_add_new_item, t);
  ns_menu_insert_item_at_index(file_menu, new_item, 0);
  ns_release(new_item);
  ns_menu_item *file_separator = ns_menu_item_create_separator_item();
  ns_menu_insert_item_at_index(file_menu, file_separator, 1);
  ns_release(file_separator);

  ns_menu *edit_menu = twin_submenu_at(menu_bar, 2);
  long after_select_all = twin_index_after(edit_menu, "Select All");
  ns_menu_item *edit_separator = ns_menu_item_create_separator_item();
  ns_menu_insert_item_at_index(edit_menu, edit_separator, after_select_all);
  ns_release(edit_separator);
  ns_menu_item *find =
      ns_menu_item_create_with_title_key_equivalent("Find", "f");
  ns_menu_item_set_action(find, twin_begin_search, t);
  ns_menu_insert_item_at_index(edit_menu, find, after_select_all + 1);
  ns_release(find);

  ns_menu *view_menu = twin_submenu_at(menu_bar, 3);
  long toggle = twin_index_of(view_menu, "Toggle Sidebar");
  if (toggle >= 0) {
    ns_menu_item_set_action(ns_menu_item_at_index(view_menu, toggle),
                            twin_toggle_sidebar, t);
  }
}

/* MARK: - Application callbacks (twins/swift/AppDelegate.swift) */

static void twin_did_finish_launching(void *context,
                                      ns_application *application) {
  twin *t = context;
  twin_install_menu_bar(t, application);
  twin_show_window(t);
  ns_application_activate(application);
}

/* Closing the last window only orders it out; the app keeps running with its
 * menu bar until Quit (F1, KTD7). */
static bool twin_should_terminate_after_last_window_closed(
    void *context, ns_application *application) {
  (void)context;
  (void)application;
  return false;
}

/* The model outlives every window, so it is freed here and never in a window's
 * will_close. */
static void twin_will_terminate(void *context, ns_application *application) {
  (void)application;
  twin *t = context;

  ns_release(t->search_item);
  ns_release(t->toolbar);
  ns_release(t->split_controller);
  ns_release(t->window);
  for (long pane = 0; pane < 3; pane++) {
    ns_release(t->settings_grids[pane]);
    ns_release(t->settings_pane_views[pane]);
    ns_release(t->settings_panes[pane]);
  }
  ns_release(t->settings_tabs);
  ns_release(t->settings_window);
  ns_release(t->detail_grid);
  ns_release(t->detail_header);
  ns_release(t->detail_title);
  ns_release(t->detail_owner);
  ns_release(t->detail_category);
  ns_release(t->detail_flagged);
  ns_release(t->detail_delete);
  ns_release(t->detail_root);
  ns_release(t->detail_controller);
  ns_release(t->list_scroll);
  ns_release(t->list_table);
  ns_release(t->list_root);
  ns_release(t->list_controller);
  ns_release(t->sidebar_scroll);
  ns_release(t->sidebar_outline);
  ns_release(t->sidebar_root);
  ns_release(t->sidebar_controller);

  free(t->items);
  free(t->visible);
  free(t);
}

/* MARK: - main (twins/swift/main.swift) */

int main(void) {
  /* The capture script reads the window numbers this process prints, so stdout
   * must not sit in a block buffer when it is redirected to a log file. */
  setvbuf(stdout, NULL, _IONBF, 0);

  twin *t = calloc(1, sizeof *t);
  if (t == NULL) {
    return 1;
  }
  t->item_capacity = TWIN_SEED_ITEM_COUNT * 2;
  t->items = malloc((size_t)t->item_capacity * sizeof *t->items);
  t->visible = malloc((size_t)t->item_capacity * sizeof *t->visible);
  if (t->items == NULL || t->visible == NULL) {
    return 1;
  }
  memcpy(t->items, TWIN_SEED_ITEMS, sizeof TWIN_SEED_ITEMS);
  t->item_count = TWIN_SEED_ITEM_COUNT;
  t->collection = TWIN_ALL_ITEMS;
  t->detail_item = -1;

  ns_application *application = ns_application_shared();
  ns_application_set_activation_policy(application,
                                       NS_APPLICATION_ACTIVATION_POLICY_REGULAR);

  /* Construction order, not file order: the sidebar's first selection and the
   * list's first filter both reach the detail pane, so it is built first. */
  twin_build_detail(t);
  twin_build_list(t);
  twin_build_sidebar(t);
  twin_build_window(t);

  static const ns_application_callbacks callbacks = {
      .did_finish_launching = twin_did_finish_launching,
      .should_terminate_after_last_window_closed =
          twin_should_terminate_after_last_window_closed,
      .will_terminate = twin_will_terminate,
  };
  ns_application_set_callbacks(application, &callbacks, t);

  ns_application_run(application); /* does not return; will_terminate frees t */
  return 0;
}

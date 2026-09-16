/*
 * NSOutlineView - the sidebar's source list, view-based and driven by one
 * merged callbacks struct (R9, R15, AE1, KTD6).
 *
 * AN ITEM IS A C POINTER. AppKit walks an outline by asking for the children
 * of an item and handing that item back later, and it requires an item that
 * keeps the same pointer and stays equal to itself across a reload. So the
 * struct traffics in plain C pointers - your own nodes, whatever they are -
 * and the wrapper boxes each pointer **once** into an object it caches by that
 * pointer. The cache lives with the outline view and survives a reload, which
 * is what makes an expanded group stay expanded (KTD6). A null item is the
 * root: `number_of_children_of_item` with a null item is asked for the number
 * of top-level rows. The wrapper never dereferences an item and never frees
 * one; keeping the nodes alive is yours.
 *
 * THE MERGED STRUCT (KTD6). Counts and children come from
 * NSOutlineViewDataSource, cells and selection from NSOutlineViewDelegate, and
 * Syntonic merges the pair into one ns_outline_view_callbacks so that one call
 * fills both AppKit slots with one shim.
 *
 * GROUP ROWS (R15). A sidebar's headings are group rows: `is_group_item`
 * answers which items are drawn that way, and `should_select_item` answers
 * false for them so a click falls through to a real row. Both are optional,
 * and an outline that sets neither is a flat selectable list. AppKit asks
 * `should_select_item` about a selection the **user** drives; a selection the
 * program makes with ns_outline_view_select_item bypasses the question, which
 * is AppKit's own rule for selecting by index. Select a row you meant to
 * select.
 *
 *     static long children(void *context, ns_outline_view *outline,
 *                          const void *item) {
 *       (void)outline;
 *       return item == NULL ? group_count : ((struct node *)item)->count;
 *     }
 *     ns_outline_view_callbacks callbacks = {
 *         .number_of_children_of_item = children, .child_of_item = child,
 *         .is_item_expandable = expandable, .cell_string = cell,
 *         .is_group_item = is_group, .should_select_item = selectable};
 *     ns_outline_view_set_callbacks(outline, &callbacks, &model);
 *     ns_table_view_reload_data(ns_outline_view_as_table_view(outline));
 *
 * An outline is an NSTableView, so its rows, its selection, its reload and its
 * style are the table's: reach them through ns_outline_view_as_table_view. The
 * style starts on the source list (R15) rather than AppKit's automatic, which
 * is the one property this wrapper sets for you.
 */

#ifndef SYNTONIC_NS_OUTLINE_VIEW_H
#define SYNTONIC_NS_OUTLINE_VIEW_H

#include <CoreGraphics/CGGeometry.h>
#include <os/availability.h>
#include <stdbool.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_outline_view ns_outline_view;

/* Declared by their own headers and repeated here so this one stands alone. C
 * has allowed a repeated typedef of the same type since C11. */
typedef struct ns_table_column ns_table_column;
typedef struct ns_table_view ns_table_view;
typedef struct ns_control ns_control;
typedef struct ns_view ns_view;

/*
 * NSOutlineViewDataSource and NSOutlineViewDelegate, merged into one struct of
 * function pointers (KTD6). `number_of_children_of_item`, `child_of_item`,
 * `is_item_expandable` and `cell_string` are required; `should_expand_item`,
 * `is_group_item`, `should_select_item` and `selection_did_change` are
 * optional. docs/conventions.md's per-protocol table is what says so, because
 * both protocols are entirely @optional in the SDK. An unset required member
 * is reported at install time in a debug build, before AppKit ever asks for a
 * child; an unset optional member behaves exactly as a method the object does
 * not implement, so an outline with no `should_expand_item` expands (AE1, R9,
 * KTD8).
 *
 * `context` is what you passed to ns_outline_view_set_callbacks, untouched and
 * never freed by the library; `sender` is the outline, borrowed for the
 * duration of the call; `item` is one of your own pointers, or null for the
 * root. Every member runs on the main thread. syntonic-owned.
 */
typedef struct ns_outline_view_callbacks {
  /* required. -[NSOutlineViewDataSource outlineView:numberOfChildrenOfItem:] -
   * a null `item` asks for the number of top-level rows. A negative count is
   * reported in a debug build (KTD8). */
  long (*_Nullable number_of_children_of_item)(
      void *_Nullable context, ns_outline_view *_Nonnull sender,
      const void *_Nullable item);
  /* required. -[NSOutlineViewDataSource outlineView:child:ofItem:] - a null
   * `item` asks for a top-level row. The returned pointer is the identity
   * AppKit addresses that row by from now on, so the same child has to come
   * back as the same pointer every time; null is reported in a debug build. */
  const void *_Nonnull (*_Nullable child_of_item)(
      void *_Nullable context, ns_outline_view *_Nonnull sender, long index,
      const void *_Nullable item);
  /* required. -[NSOutlineViewDataSource outlineView:isItemExpandable:] */
  bool (*_Nullable is_item_expandable)(void *_Nullable context,
                                       ns_outline_view *_Nonnull sender,
                                       const void *_Nonnull item);
  /* required. syntonic-owned: the one string this row shows, which the wrapper
   * copies into the cell view it built and reuses for
   * -[NSOutlineViewDelegate outlineView:viewForTableColumn:item:] (KTD6). The
   * returned pointer is borrowed by the library for the duration of the
   * callback and copied at once; null is reported in a debug build. */
  const char *_Nonnull (*_Nullable cell_string)(
      void *_Nullable context, ns_outline_view *_Nonnull sender,
      const void *_Nonnull item);
  /* optional. -[NSOutlineViewDelegate outlineView:shouldExpandItem:] - unset
   * lets every expandable item expand, which is AppKit's own answer for a
   * delegate that does not implement it (AE1). */
  bool (*_Nullable should_expand_item)(void *_Nullable context,
                                       ns_outline_view *_Nonnull sender,
                                       const void *_Nonnull item);
  /* optional. -[NSOutlineViewDelegate outlineView:shouldCollapseItem:] - false
   * pins a group open, which is what a sidebar whose headings never close
   * answers (R15). Unset lets every expanded item collapse. */
  bool (*_Nullable should_collapse_item)(void *_Nullable context,
                                         ns_outline_view *_Nonnull sender,
                                         const void *_Nonnull item);
  /* optional. -[NSOutlineViewDelegate outlineView:isGroupItem:] - true draws
   * the row as a source list heading (R15). */
  bool (*_Nullable is_group_item)(void *_Nullable context,
                                  ns_outline_view *_Nonnull sender,
                                  const void *_Nonnull item);
  /* optional. -[NSOutlineViewDelegate outlineView:shouldSelectItem:] - false
   * makes the row unselectable, which is what a group heading is (R15). */
  bool (*_Nullable should_select_item)(void *_Nullable context,
                                       ns_outline_view *_Nonnull sender,
                                       const void *_Nonnull item);
  /* optional. -[NSOutlineViewDelegate outlineViewSelectionDidChange:] - `item`
   * is the newly selected item, or null when the selection was cleared. */
  void (*_Nullable selection_did_change)(void *_Nullable context,
                                         ns_outline_view *_Nonnull sender,
                                         const void *_Nullable item);
} ns_outline_view_callbacks;

/* -[NSTableView initWithFrame:], on an NSOutlineView - owned (+1), released
 * with ns_release (R7). A constructor is the one inherited member a subclass
 * redeclares, because an upcast needs an object to upcast. The outline starts
 * on the source list style (R15). */
ns_outline_view *_Nonnull ns_outline_view_create_with_frame(CGRect frame)
    API_AVAILABLE(macos(26.0));

/* -[NSOutlineView setOutlineTableColumn:] - the column the disclosure
 * triangles are drawn in. Add the column to the outline first. */
void ns_outline_view_set_outline_table_column(
    ns_outline_view *_Nonnull outline_view,
    ns_table_column *_Nullable outline_table_column)
    API_AVAILABLE(macos(26.0));

/* -[NSOutlineView outlineTableColumn] - an assign property, so this is
 * borrowed (R7). */
ns_table_column *_Nullable ns_outline_view_outline_table_column(
    ns_outline_view *_Nonnull outline_view) API_AVAILABLE(macos(26.0));

/* -[NSOutlineView expandItem:] - a null item expands the root's children. */
void ns_outline_view_expand_item(ns_outline_view *_Nonnull outline_view,
                                 const void *_Nullable item)
    API_AVAILABLE(macos(26.0));

/* -[NSOutlineView expandItem:expandChildren:] - a null item with
 * `expand_children` true expands the whole tree, which is how a sidebar opens
 * every group at once (R15). */
void ns_outline_view_expand_item_expand_children(
    ns_outline_view *_Nonnull outline_view, const void *_Nullable item,
    bool expand_children) API_AVAILABLE(macos(26.0));

/* -[NSOutlineView collapseItem:] */
void ns_outline_view_collapse_item(ns_outline_view *_Nonnull outline_view,
                                   const void *_Nullable item)
    API_AVAILABLE(macos(26.0));

/* -[NSOutlineView isItemExpanded:] - a BOOL getter drops AppKit's `is` (R5). */
bool ns_outline_view_item_expanded(ns_outline_view *_Nonnull outline_view,
                                   const void *_Nullable item)
    API_AVAILABLE(macos(26.0));

/* -[NSOutlineView rowForItem:] - -1 when the item is not in a displayed row,
 * which includes an item inside a collapsed group (R11). */
long ns_outline_view_row_for_item(ns_outline_view *_Nonnull outline_view,
                                  const void *_Nullable item)
    API_AVAILABLE(macos(26.0));

/* -[NSOutlineView itemAtRow:] - your own pointer back, null when the row has
 * none. This is not an owned reference: it is the pointer `child_of_item`
 * handed over, and the wrapper neither retains nor frees it (R7). */
const void *_Nullable ns_outline_view_item_at_row(
    ns_outline_view *_Nonnull outline_view, long row)
    API_AVAILABLE(macos(26.0));

/* The selected item, or null when nothing is selected: -[NSTableView
 * selectedRow] followed by -[NSOutlineView itemAtRow:]. The same pointer rule
 * as ns_outline_view_item_at_row. syntonic-owned. */
const void *_Nullable ns_outline_view_selected_item(
    ns_outline_view *_Nonnull outline_view) API_AVAILABLE(macos(26.0));

/* Selects the row an item is displayed in: -[NSOutlineView rowForItem:]
 * followed by -[NSTableView selectRowIndexes:byExtendingSelection:] with that
 * one index (KTD17). An item in no displayed row, or one the struct's
 * `should_select_item` refuses, leaves the selection where it was. Selecting
 * does not scroll; ns_outline_view_scroll_item_to_visible does.
 * syntonic-owned. */
void ns_outline_view_select_item(ns_outline_view *_Nonnull outline_view,
                                 const void *_Nullable item)
    API_AVAILABLE(macos(26.0));

/* Scrolls the row an item is displayed in into view: -[NSOutlineView
 * rowForItem:] followed by -[NSTableView scrollRowToVisible:]. An item in no
 * displayed row scrolls nothing (R16). syntonic-owned. */
void ns_outline_view_scroll_item_to_visible(
    ns_outline_view *_Nonnull outline_view, const void *_Nullable item)
    API_AVAILABLE(macos(26.0));

/* Installs the merged struct as both the outline's data source and its
 * delegate (KTD6). The struct is copied; `context` is not, and has to stay
 * valid until you uninstall or the outline is deallocated - ns_release does
 * not uninstall. Installing again replaces the previous struct and context and
 * keeps the item cache, so identity and expansion survive it; a null
 * `callbacks` uninstalls. An unset required member is reported in a debug
 * build before either slot is assigned (R9, AE1, KTD8). syntonic-owned. */
void ns_outline_view_set_callbacks(
    ns_outline_view *_Nonnull outline_view,
    const ns_outline_view_callbacks *_Nullable callbacks,
    void *_Nullable context) API_AVAILABLE(macos(26.0));

/* The upcast to ns_table_view: the same pointer, checked in a debug build (R4,
 * KTD9). Rows, selection, reload and style live there. syntonic-owned. */
ns_table_view *_Nonnull ns_outline_view_as_table_view(
    ns_outline_view *_Nonnull outline_view) API_AVAILABLE(macos(26.0));

/* The upcast to ns_control: the same pointer, checked in a debug build (R4,
 * KTD9). syntonic-owned. */
ns_control *_Nonnull ns_outline_view_as_control(
    ns_outline_view *_Nonnull outline_view) API_AVAILABLE(macos(26.0));

/* The upcast to ns_view: the same pointer, checked in a debug build (R4,
 * KTD9). This is what a scroll view takes as its document view.
 * syntonic-owned. */
ns_view *_Nonnull ns_outline_view_as_view(
    ns_outline_view *_Nonnull outline_view) API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_OUTLINE_VIEW_H */

/*
 * NSOutlineView, view-based, driven by one merged callbacks struct, with items
 * that are plain C pointers (R9, R15, AE1, KTD6, KTD8).
 *
 * WHY THERE IS A BOX. AppKit addresses an outline row by the object the data
 * source handed it, and it requires that object to keep the same pointer and
 * stay equal to itself across a reload - otherwise expansion state silently
 * resets. A C pointer is not an object, so each one is boxed once into a
 * SynOutlineItem and the box is cached by that pointer for the life of the
 * outline view. The cache survives a reload and survives replacing the
 * callbacks struct, which is exactly what makes an expanded group stay
 * expanded. Neither the box nor the cache retains the outline view (KTD8).
 *
 * The cache never evicts on its own, because it cannot tell a freed node from
 * a live one: an address is permanent identity here, and
 * ns_outline_view_forget_item is what the caller drops one with before freeing
 * the node or handing its address to another.
 */

#import <AppKit/AppKit.h>
#import <objc/runtime.h>

#include "ns_internal.h"
#include "syn_cell_views.h"
#include "syn_shims.h"
#include "syntonic/ns_outline_view.h"
#include "syntonic/ns_table_column.h"

/* ---------------------------------------------------------------------------
 * The item box and its cache
 * ------------------------------------------------------------------------- */

@interface SynOutlineItem : NSObject
@property(readonly, nonatomic) const void *synPointer;
- (instancetype)initWithPointer:(const void *)pointer;
@end

@implementation SynOutlineItem

- (instancetype)initWithPointer:(const void *)pointer {
  self = [super init];
  if (self == nil) return nil;
  _synPointer = pointer;
  return self;
}

/* Default identity equality is what AppKit needs, and the cache below is what
 * makes it hold: one pointer is one box, for as long as the outline lives. */

@end

/* One cache per outline view, keyed by the caller's pointer. The association
 * holds the cache; the cache holds the boxes; nothing here holds the outline
 * view or the caller's own nodes. */
static const char syn_outline_items_key;

static NSMutableDictionary *syn_outline_items(NSOutlineView *outline) {
  NSMutableDictionary *items =
      objc_getAssociatedObject(outline, &syn_outline_items_key);
  if (items == nil) {
    items = [NSMutableDictionary dictionary];
    objc_setAssociatedObject(outline, &syn_outline_items_key, items,
                             OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  }
  return items;
}

/* The one box for `pointer`, made on first sight and handed back unchanged
 * afterwards. Null is the root, which AppKit spells as a nil item. */
static SynOutlineItem *syn_outline_box(NSOutlineView *outline,
                                       const void *pointer) {
  if (pointer == NULL) return nil;
  NSMutableDictionary *items = syn_outline_items(outline);
  NSValue *key = [NSValue valueWithPointer:pointer];
  SynOutlineItem *box = items[key];
  if (box == nil) {
    box = [[SynOutlineItem alloc] initWithPointer:pointer];
    items[key] = box;
  }
  return box;
}

/* The caller's pointer back out of a box; null for the root and for anything
 * that is not a box. */
static const void *syn_outline_pointer(id item) {
  if (![item isKindOfClass:[SynOutlineItem class]]) return NULL;
  return ((SynOutlineItem *)item).synPointer;
}

/* ---------------------------------------------------------------------------
 * The merged shim
 * ------------------------------------------------------------------------- */

/* Both protocols are entirely @optional in the SDK; docs/conventions.md's
 * per-protocol table is what makes four of these required (R9, KTD8). */
SYN_SHIM_TABLE(syn_outline_view_table, ns_outline_view_callbacks,
               "NSOutlineViewDataSource + NSOutlineViewDelegate",
               SYN_SHIM_REQUIRED(ns_outline_view_callbacks,
                                 number_of_children_of_item,
                                 "outlineView:numberOfChildrenOfItem:"),
               SYN_SHIM_REQUIRED(ns_outline_view_callbacks, child_of_item,
                                 "outlineView:child:ofItem:"),
               SYN_SHIM_REQUIRED(ns_outline_view_callbacks, is_item_expandable,
                                 "outlineView:isItemExpandable:"),
               SYN_SHIM_REQUIRED(ns_outline_view_callbacks, cell_string,
                                 "outlineView:viewForTableColumn:item:"),
               /* The second member feeding the cell selector: the shim answers
                * respondsToSelector: yes when either is set, and cell_string
                * is required, so an outline with no icons is unchanged. */
               SYN_SHIM_OPTIONAL(ns_outline_view_callbacks, cell_symbol_name,
                                 "outlineView:viewForTableColumn:item:"),
               SYN_SHIM_OPTIONAL(ns_outline_view_callbacks, should_expand_item,
                                 "outlineView:shouldExpandItem:"),
               SYN_SHIM_OPTIONAL(ns_outline_view_callbacks,
                                 should_collapse_item,
                                 "outlineView:shouldCollapseItem:"),
               SYN_SHIM_OPTIONAL(ns_outline_view_callbacks, is_group_item,
                                 "outlineView:isGroupItem:"),
               SYN_SHIM_OPTIONAL(ns_outline_view_callbacks, should_select_item,
                                 "outlineView:shouldSelectItem:"),
               SYN_SHIM_OPTIONAL(ns_outline_view_callbacks,
                                 selection_did_change,
                                 "outlineViewSelectionDidChange:"));

@interface SynOutlineViewShim : SynShim <NSOutlineViewDataSource,
                                         NSOutlineViewDelegate>
@end

@implementation SynOutlineViewShim

- (NSInteger)outlineView:(NSOutlineView *)outlineView
    numberOfChildrenOfItem:(id)item {
  SYN_SHIM_ENTER(NSOutlineView, outlineView);
  long (*callback)(void *, ns_outline_view *, const void *) =
      SYN_SHIM_FN(ns_outline_view_callbacks, number_of_children_of_item);
  if (callback == NULL) return 0;
  long count = callback(syn_context, NS_OUT(ns_outline_view, syn_sender),
                        syn_outline_pointer(item));
  SYN_SHIM_CHECK_COUNT(syn_outline_view_table, number_of_children_of_item,
                       count);
  return (NSInteger)count;
  SYN_SHIM_LEAVE();
}

/* The returned pointer is the identity AppKit addresses this row by from now
 * on, so it goes through the cache and comes back as the same box (KTD6). */
- (id)outlineView:(NSOutlineView *)outlineView
            child:(NSInteger)index
           ofItem:(id)item {
  SYN_SHIM_ENTER(NSOutlineView, outlineView);
  const void *(*callback)(void *, ns_outline_view *, long, const void *) =
      SYN_SHIM_FN(ns_outline_view_callbacks, child_of_item);
  if (callback == NULL) return nil;
  const void *child =
      callback(syn_context, NS_OUT(ns_outline_view, syn_sender), (long)index,
               syn_outline_pointer(item));
  SYN_SHIM_CHECK_NONNULL(syn_outline_view_table, child_of_item, child);
  return syn_outline_box(syn_sender, child);
  SYN_SHIM_LEAVE();
}

- (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item {
  SYN_SHIM_ENTER(NSOutlineView, outlineView);
  bool (*callback)(void *, ns_outline_view *, const void *) =
      SYN_SHIM_FN(ns_outline_view_callbacks, is_item_expandable);
  if (callback == NULL) return NO;
  return callback(syn_context, NS_OUT(ns_outline_view, syn_sender),
                  syn_outline_pointer(item));
  SYN_SHIM_LEAVE();
}

/* KTD6: the struct supplies one borrowed string, optionally a second one for
 * the row's icon, and the wrapper owns the view. Each string is copied before
 * the next call into caller code, so neither pointer outlives its callback. */
- (NSView *)outlineView:(NSOutlineView *)outlineView
     viewForTableColumn:(NSTableColumn *)tableColumn
                   item:(id)item {
  SYN_SHIM_ENTER(NSOutlineView, outlineView);
  const char *(*callback)(void *, ns_outline_view *, const void *) =
      SYN_SHIM_FN(ns_outline_view_callbacks, cell_string);
  if (callback == NULL) return nil;
  const char *text = callback(syn_context, NS_OUT(ns_outline_view, syn_sender),
                              syn_outline_pointer(item));
  SYN_SHIM_CHECK_NONNULL(syn_outline_view_table, cell_string, text);
  SYN_SHIM_CHECK_UTF8(syn_outline_view_table, cell_string, text);
  /* Copied here rather than in syn_cell_view: cell_symbol_name below re-enters
   * caller code, which is free to hand back the same buffer cell_string just
   * returned, and the borrowed pointer is only good until then (R11). */
  NSString *label = text != NULL ? @(text) : nil;

  /* Null is an answer here, not a report: a row with no icon - a group
   * heading - is what the member says so with. */
  const char *(*symbol_callback)(void *, ns_outline_view *, const void *) =
      SYN_SHIM_FN(ns_outline_view_callbacks, cell_symbol_name);
  const char *symbol =
      symbol_callback != NULL
          ? symbol_callback(syn_context, NS_OUT(ns_outline_view, syn_sender),
                            syn_outline_pointer(item))
          : NULL;
  SYN_SHIM_CHECK_UTF8(syn_outline_view_table, cell_symbol_name, symbol);
  return syn_cell_view(outlineView, tableColumn, label, symbol);
  SYN_SHIM_LEAVE();
}

/* AE1: this method exists only while the member is set. With the member unset
 * respondsToSelector: reports it absent, AppKit takes its own default, and the
 * item expands. */
- (BOOL)outlineView:(NSOutlineView *)outlineView shouldExpandItem:(id)item {
  SYN_SHIM_ENTER(NSOutlineView, outlineView);
  bool (*callback)(void *, ns_outline_view *, const void *) =
      SYN_SHIM_FN(ns_outline_view_callbacks, should_expand_item);
  if (callback == NULL) return YES;
  return callback(syn_context, NS_OUT(ns_outline_view, syn_sender),
                  syn_outline_pointer(item));
  SYN_SHIM_LEAVE();
}

- (BOOL)outlineView:(NSOutlineView *)outlineView shouldCollapseItem:(id)item {
  SYN_SHIM_ENTER(NSOutlineView, outlineView);
  bool (*callback)(void *, ns_outline_view *, const void *) =
      SYN_SHIM_FN(ns_outline_view_callbacks, should_collapse_item);
  if (callback == NULL) return YES;
  return callback(syn_context, NS_OUT(ns_outline_view, syn_sender),
                  syn_outline_pointer(item));
  SYN_SHIM_LEAVE();
}

- (BOOL)outlineView:(NSOutlineView *)outlineView isGroupItem:(id)item {
  SYN_SHIM_ENTER(NSOutlineView, outlineView);
  bool (*callback)(void *, ns_outline_view *, const void *) =
      SYN_SHIM_FN(ns_outline_view_callbacks, is_group_item);
  if (callback == NULL) return NO;
  return callback(syn_context, NS_OUT(ns_outline_view, syn_sender),
                  syn_outline_pointer(item));
  SYN_SHIM_LEAVE();
}

- (BOOL)outlineView:(NSOutlineView *)outlineView shouldSelectItem:(id)item {
  SYN_SHIM_ENTER(NSOutlineView, outlineView);
  bool (*callback)(void *, ns_outline_view *, const void *) =
      SYN_SHIM_FN(ns_outline_view_callbacks, should_select_item);
  if (callback == NULL) return YES;
  return callback(syn_context, NS_OUT(ns_outline_view, syn_sender),
                  syn_outline_pointer(item));
  SYN_SHIM_LEAVE();
}

- (void)outlineViewSelectionDidChange:(NSNotification *)notification {
  SYN_SHIM_ENTER(NSOutlineView, notification.object);
  void (*callback)(void *, ns_outline_view *, const void *) =
      SYN_SHIM_FN(ns_outline_view_callbacks, selection_did_change);
  if (callback == NULL) return;
  NSInteger row = syn_sender.selectedRow;
  id item = row >= 0 ? [syn_sender itemAtRow:row] : nil;
  callback(syn_context, NS_OUT(ns_outline_view, syn_sender),
           syn_outline_pointer(item));
  SYN_SHIM_LEAVE();
}

@end

/* ---------------------------------------------------------------------------
 * The wrapper
 * ------------------------------------------------------------------------- */

/* Owned (+1): an initWith… returns a fresh object the caller ends with
 * ns_release (R7, KTD7). The source list style is the one property this
 * constructor sets, because a sidebar outline is what R15 asks for and
 * AppKit's own default is the automatic style. */
ns_outline_view *ns_outline_view_create_with_frame(CGRect frame) {
  NS_ENTER();
  NSOutlineView *outline = [[NSOutlineView alloc] initWithFrame:frame];
  outline.style = NSTableViewStyleSourceList;
  return NS_OUT_OWNED(ns_outline_view, outline);
  NS_LEAVE();
}

void ns_outline_view_set_outline_table_column(
    ns_outline_view *outline_view, ns_table_column *outline_table_column) {
  NS_ENTER();
  NS_IN(NSOutlineView, outline_view).outlineTableColumn =
      NS_IN_OPT(NSTableColumn, outline_table_column);
  NS_LEAVE();
}

/* `outlineTableColumn` is an assign property, so the return is borrowed (R7). */
ns_table_column *ns_outline_view_outline_table_column(
    ns_outline_view *outline_view) {
  NS_ENTER();
  return NS_OUT(ns_table_column,
                NS_IN(NSOutlineView, outline_view).outlineTableColumn);
  NS_LEAVE();
}

void ns_outline_view_expand_item(ns_outline_view *outline_view,
                                 const void *item) {
  NS_ENTER();
  NSOutlineView *target = NS_IN(NSOutlineView, outline_view);
  [target expandItem:syn_outline_box(target, item)];
  NS_LEAVE();
}

void ns_outline_view_expand_item_expand_children(ns_outline_view *outline_view,
                                                 const void *item,
                                                 bool expand_children) {
  NS_ENTER();
  NSOutlineView *target = NS_IN(NSOutlineView, outline_view);
  [target expandItem:syn_outline_box(target, item)
       expandChildren:expand_children];
  NS_LEAVE();
}

void ns_outline_view_collapse_item(ns_outline_view *outline_view,
                                   const void *item) {
  NS_ENTER();
  NSOutlineView *target = NS_IN(NSOutlineView, outline_view);
  [target collapseItem:syn_outline_box(target, item)];
  NS_LEAVE();
}

bool ns_outline_view_item_expanded(ns_outline_view *outline_view,
                                   const void *item) {
  NS_ENTER();
  NSOutlineView *target = NS_IN(NSOutlineView, outline_view);
  return [target isItemExpanded:syn_outline_box(target, item)];
  NS_LEAVE();
}

long ns_outline_view_row_for_item(ns_outline_view *outline_view,
                                  const void *item) {
  NS_ENTER();
  NSOutlineView *target = NS_IN(NSOutlineView, outline_view);
  return (long)[target rowForItem:syn_outline_box(target, item)];
  NS_LEAVE();
}

/* The caller's own pointer back out of the box: not a reference, so there is
 * nothing to release (R7). */
const void *ns_outline_view_item_at_row(ns_outline_view *outline_view,
                                        long row) {
  NS_ENTER();
  NSOutlineView *target = NS_IN(NSOutlineView, outline_view);
  return syn_outline_pointer([target itemAtRow:(NSInteger)row]);
  NS_LEAVE();
}

const void *ns_outline_view_selected_item(ns_outline_view *outline_view) {
  NS_ENTER();
  NSOutlineView *target = NS_IN(NSOutlineView, outline_view);
  NSInteger row = target.selectedRow;
  if (row < 0) return NULL;
  return syn_outline_pointer([target itemAtRow:row]);
  NS_LEAVE();
}

/* An item in no displayed row has row -1, and selecting nothing is what that
 * has to mean: an index set built from -1 would raise instead. */
void ns_outline_view_select_item(ns_outline_view *outline_view,
                                 const void *item) {
  NS_ENTER();
  NSOutlineView *target = NS_IN(NSOutlineView, outline_view);
  NSInteger row = [target rowForItem:syn_outline_box(target, item)];
  if (row < 0) return;
  [target selectRowIndexes:[NSIndexSet indexSetWithIndex:(NSUInteger)row]
      byExtendingSelection:NO];
  NS_LEAVE();
}

void ns_outline_view_scroll_item_to_visible(ns_outline_view *outline_view,
                                            const void *item) {
  NS_ENTER();
  NSOutlineView *target = NS_IN(NSOutlineView, outline_view);
  NSInteger row = [target rowForItem:syn_outline_box(target, item)];
  if (row < 0) return;
  [target scrollRowToVisible:row];
  NS_LEAVE();
}

/* The one way out of the cache: the next sight of `item` boxes it afresh, so
 * AppKit sees a new row identity and none of the old one's state. */
void ns_outline_view_forget_item(ns_outline_view *outline_view,
                                 const void *item) {
  NS_ENTER();
  NSOutlineView *target = NS_IN(NSOutlineView, outline_view);
  if (item != NULL)
    [syn_outline_items(target)
        removeObjectForKey:[NSValue valueWithPointer:item]];
  NS_LEAVE();
}

/* KTD6: one install, both slots, one shim conforming to both protocols. The
 * item cache is untouched, so identity - and with it every expanded group -
 * survives a replacement. */
void ns_outline_view_set_callbacks(ns_outline_view *outline_view,
                                   const ns_outline_view_callbacks *callbacks,
                                   void *context) {
  NS_ENTER();
  NSOutlineView *target = NS_IN(NSOutlineView, outline_view);
  SYN_SHIM_INSTALL(target, SynOutlineViewShim, syn_outline_view_table,
                   callbacks, context, ^(id shim) {
                     target.delegate = shim;
                     target.dataSource = shim;
                   });
  NS_LEAVE();
}

/* An upcast is the identity, with the debug class check NS_IN carries (KTD9). */
ns_table_view *ns_outline_view_as_table_view(ns_outline_view *outline_view) {
  NS_ENTER();
  return NS_OUT(ns_table_view, NS_IN(NSOutlineView, outline_view));
  NS_LEAVE();
}

ns_control *ns_outline_view_as_control(ns_outline_view *outline_view) {
  NS_ENTER();
  return NS_OUT(ns_control, NS_IN(NSOutlineView, outline_view));
  NS_LEAVE();
}

ns_view *ns_outline_view_as_view(ns_outline_view *outline_view) {
  NS_ENTER();
  return NS_OUT(ns_view, NS_IN(NSOutlineView, outline_view));
  NS_LEAVE();
}

/*
 * Build-and-tag cell views for the view-based table and outline (KTD6).
 *
 * Read src/syn_cell_views.h first: it carries the reason this is not a nib
 * lookup. Nullability annotations belong to the public headers, not here.
 */

#import <AppKit/AppKit.h>

#include "syn_cell_views.h"

/* The identifier a cell view is tagged with when it belongs to no column -
 * AppKit passes a nil column for a group row. */
static NSString *const syn_cell_view_identifier = @"syn.cell";

static NSTableCellView *syn_build_cell_view(NSUserInterfaceItemIdentifier
                                                identifier) {
  NSTableCellView *cell =
      [[NSTableCellView alloc] initWithFrame:NSMakeRect(0, 0, 100, 17)];
  /* The tag is what makes AppKit's own reuse find this view next time. */
  cell.identifier = identifier;

  NSTextField *label = [NSTextField labelWithString:@""];
  label.lineBreakMode = NSLineBreakByTruncatingTail;
  label.translatesAutoresizingMaskIntoConstraints = NO;
  [cell addSubview:label];
  /* NSTableCellView.textField is an assign property; the subview above is what
   * holds the label. */
  cell.textField = label;

  [NSLayoutConstraint activateConstraints:@[
    [label.leadingAnchor constraintEqualToAnchor:cell.leadingAnchor],
    [label.trailingAnchor constraintEqualToAnchor:cell.trailingAnchor],
    [label.centerYAnchor constraintEqualToAnchor:cell.centerYAnchor],
  ]];
  return cell;
}

NSTableCellView *syn_cell_view(NSTableView *table, NSTableColumn *column,
                               const char *utf8) {
  NSUserInterfaceItemIdentifier identifier =
      column.identifier ?: syn_cell_view_identifier;

  NSView *reused = [table makeViewWithIdentifier:identifier owner:nil];
  NSTableCellView *cell = [reused isKindOfClass:[NSTableCellView class]]
                              ? (NSTableCellView *)reused
                              : nil;
  if (cell == nil) cell = syn_build_cell_view(identifier);

  /* The string is borrowed for the callback that produced it, so it is copied
   * now and the pointer is not kept (R11, KTD17). */
  NSString *text = utf8 != NULL ? @(utf8) : nil;
  cell.textField.stringValue = text != nil ? text : @"";
  return cell;
}

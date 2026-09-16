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

/* What separates the two reuse pools: a cell with an icon and a cell without
 * one are laid out differently and neither can turn into the other. */
static NSString *const syn_icon_cell_view_suffix = @".syn.icon";

/* The gap between the icon and the label, and the two priorities that keep the
 * icon at its own width while the label takes the rest. The twins are compared
 * pixel by pixel, so these are the Swift sidebar's numbers, not new ones. */
static const CGFloat syn_icon_cell_spacing = 6;

static NSTextField *syn_build_label(void) {
  NSTextField *label = [NSTextField labelWithString:@""];
  label.lineBreakMode = NSLineBreakByTruncatingTail;
  label.translatesAutoresizingMaskIntoConstraints = NO;
  return label;
}

/* The text-only shape: the label pinned to all four of the cell's edges, which
 * is what both Swift builders do. A row taller than the label's intrinsic
 * height would otherwise place the two twins' text differently. */
static NSTableCellView *syn_build_cell_view(
    NSUserInterfaceItemIdentifier identifier) {
  NSTableCellView *cell =
      [[NSTableCellView alloc] initWithFrame:NSMakeRect(0, 0, 100, 17)];
  /* The tag is what makes AppKit's own reuse find this view next time. */
  cell.identifier = identifier;

  NSTextField *label = syn_build_label();
  [cell addSubview:label];
  /* NSTableCellView.textField is an assign property; the subview above is what
   * holds the label. */
  cell.textField = label;

  [NSLayoutConstraint activateConstraints:@[
    [label.leadingAnchor constraintEqualToAnchor:cell.leadingAnchor],
    [label.trailingAnchor constraintEqualToAnchor:cell.trailingAnchor],
    [label.topAnchor constraintEqualToAnchor:cell.topAnchor],
    [label.bottomAnchor constraintEqualToAnchor:cell.bottomAnchor],
  ]];
  return cell;
}

/* The icon shape: an image view and the label side by side in a horizontal
 * stack pinned to the cell's edges, which is what the Swift sidebar builds. */
static NSTableCellView *syn_build_icon_cell_view(
    NSUserInterfaceItemIdentifier identifier) {
  NSTableCellView *cell =
      [[NSTableCellView alloc] initWithFrame:NSMakeRect(0, 0, 100, 17)];
  cell.identifier = identifier;

  NSTextField *label = syn_build_label();
  NSImageView *icon = [[NSImageView alloc] init];
  icon.contentTintColor = NSColor.controlAccentColor;
  [icon setContentHuggingPriority:NSLayoutPriorityRequired
                   forOrientation:NSLayoutConstraintOrientationHorizontal];

  NSStackView *row = [NSStackView stackViewWithViews:@[ icon, label ]];
  row.orientation = NSUserInterfaceLayoutOrientationHorizontal;
  row.alignment = NSLayoutAttributeCenterY;
  row.spacing = syn_icon_cell_spacing;
  row.translatesAutoresizingMaskIntoConstraints = NO;
  [cell addSubview:row];
  /* Both are assign properties on NSTableCellView; the stack is what holds
   * them. */
  cell.textField = label;
  cell.imageView = icon;

  [NSLayoutConstraint activateConstraints:@[
    [row.leadingAnchor constraintEqualToAnchor:cell.leadingAnchor],
    [row.trailingAnchor constraintEqualToAnchor:cell.trailingAnchor],
    [row.topAnchor constraintEqualToAnchor:cell.topAnchor],
    [row.bottomAnchor constraintEqualToAnchor:cell.bottomAnchor],
  ]];
  return cell;
}

NSTableCellView *syn_cell_view(NSTableView *table, NSTableColumn *column,
                               const char *utf8, const char *symbol) {
  NSUserInterfaceItemIdentifier identifier =
      column.identifier ?: syn_cell_view_identifier;
  /* Composed per call rather than memoised: AppKit's reuse compares the
   * identifier by value, so a fresh equal string finds the same pool, and a
   * memo keyed on caller-supplied identifiers would never be evicted. */
  if (symbol != NULL)
    identifier = [identifier stringByAppendingString:syn_icon_cell_view_suffix];

  NSView *reused = [table makeViewWithIdentifier:identifier owner:nil];
  NSTableCellView *cell = [reused isKindOfClass:[NSTableCellView class]]
                              ? (NSTableCellView *)reused
                              : nil;
  if (cell == nil)
    cell = symbol != NULL ? syn_build_icon_cell_view(identifier)
                          : syn_build_cell_view(identifier);

  /* Both strings are borrowed for the callback that produced them, so both are
   * copied now and neither pointer is kept (R11, KTD17). */
  NSString *text = utf8 != NULL ? @(utf8) : nil;
  cell.textField.stringValue = text != nil ? text : @"";
  if (symbol != NULL) {
    /* The row's own text is what a screen reader reads for the icon, which is
     * what the Swift sidebar passes too (R23). An unknown symbol name is nil
     * here, not an error. */
    cell.imageView.image = [NSImage imageWithSystemSymbolName:@(symbol)
                                     accessibilityDescription:text];
  }
  return cell;
}

/*
 * The cell view a view-based table or outline shows, built and tagged by the
 * wrapper rather than unarchived from a nib (KTD6).
 *
 * This header never ships. Like src/ns_internal.h it is internal, and like it
 * it carries no nullability annotations: -Wnullability-completeness is
 * file-scoped.
 */

#ifndef SYNTONIC_SRC_SYN_CELL_VIEWS_H
#define SYNTONIC_SRC_SYN_CELL_VIEWS_H

#import <AppKit/AppKit.h>

/* The cell view for one cell of `table`, carrying `utf8` as its label's text.
 *
 * -[NSTableView makeViewWithIdentifier:owner:] returns nil until something has
 * been built with that identifier, because there is no nib to unarchive one
 * from. So this builds an NSTableCellView with a label the first time, tags it
 * with the column's identifier, and lets AppKit's own reuse hand the same view
 * back afterwards - the build-and-tag pattern (KTD6).
 *
 * `utf8` is the borrowed string a callbacks struct just returned, valid only
 * until that callback returns, so it is copied into the label here and the
 * pointer is never kept (R11, KTD17). `column` may be nil, which is what
 * AppKit passes for a group row; the fallback identifier is used then.
 *
 * The returned view is autoreleased, exactly as AppKit's own delegate methods
 * return one, and the caller hands it straight back to AppKit.
 */
NSTableCellView *syn_cell_view(NSTableView *table, NSTableColumn *column,
                              const char *utf8);

#endif /* SYNTONIC_SRC_SYN_CELL_VIEWS_H */

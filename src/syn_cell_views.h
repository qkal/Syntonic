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

/* The cell view for one cell of `table`, carrying `utf8` as its label's text
 * and, when `symbol` is not null, that SF Symbol as an icon ahead of the
 * label.
 *
 * -[NSTableView makeViewWithIdentifier:owner:] returns nil until something has
 * been built with that identifier, because there is no nib to unarchive one
 * from. So this builds an NSTableCellView the first time, tags it with the
 * column's identifier, and lets AppKit's own reuse hand the same view back
 * afterwards - the build-and-tag pattern (KTD6).
 *
 * The two shapes are two reuse pools, because a cell laid out one way cannot
 * become the other: a cell with an icon is tagged with the column's identifier
 * plus a suffix, and a cell without one is tagged exactly as it was before the
 * icon existed. An outline whose struct leaves `cell_symbol_name` unset
 * therefore gets the identical view it got before.
 *
 * `utf8` is the borrowed string a callbacks struct just returned, valid only
 * until that callback returns, so it is copied into the label here and the
 * pointer is never kept (R11, KTD17); `symbol` is borrowed the same way.
 * `column` may be nil, which is what AppKit passes for a group row; the
 * fallback identifier is used then.
 *
 * The returned view is autoreleased, exactly as AppKit's own delegate methods
 * return one, and the caller hands it straight back to AppKit.
 */
NSTableCellView *syn_cell_view(NSTableView *table, NSTableColumn *column,
                               const char *utf8, const char *symbol);

#endif /* SYNTONIC_SRC_SYN_CELL_VIEWS_H */

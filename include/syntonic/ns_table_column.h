/*
 * NSTableColumn - one column of a table or of an outline (R17).
 *
 * A column is made from its identifier and given a title and a width; the
 * table it is added to retains it, so releasing your own reference afterwards
 * is correct (R7).
 *
 *     ns_table_column *title = ns_table_column_create_with_identifier("title");
 *     ns_table_column_set_title(title, "Title");
 *     ns_table_column_set_width(title, 170);
 *     ns_table_view_add_table_column(table, title);
 *     ns_release(title);
 *
 * The identifier is what a table's cell callback is keyed by in AppKit. In
 * Syntonic the cell callback is handed the column's index instead, because an
 * index pairs with the row the same callback already carries (KTD17); the
 * identifier is still here, because AppKit's own reuse of a built cell view is
 * keyed by it.
 */

#ifndef SYNTONIC_NS_TABLE_COLUMN_H
#define SYNTONIC_NS_TABLE_COLUMN_H

#include <CoreGraphics/CGBase.h>
#include <os/availability.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_table_column ns_table_column;

/* -[NSTableColumn initWithIdentifier:] - owned (+1), released with ns_release
 * (R7). */
ns_table_column *_Nonnull ns_table_column_create_with_identifier(
    const char *_Nonnull identifier) API_AVAILABLE(macos(26.0));

/* -[NSTableColumn identifier] - a copy property, so this is an owned copy the
 * caller frees with ns_string_free (R7, R11). */
char *_Nonnull ns_table_column_copy_identifier(
    ns_table_column *_Nonnull table_column) API_AVAILABLE(macos(26.0));

/* -[NSTableColumn setTitle:] - what the table's header row shows. */
void ns_table_column_set_title(ns_table_column *_Nonnull table_column,
                               const char *_Nonnull title)
    API_AVAILABLE(macos(26.0));

/* -[NSTableColumn title] - a copy property, so this is an owned copy the
 * caller frees with ns_string_free (R7, R11). */
char *_Nonnull ns_table_column_copy_title(
    ns_table_column *_Nonnull table_column) API_AVAILABLE(macos(26.0));

/* -[NSTableColumn setWidth:] */
void ns_table_column_set_width(ns_table_column *_Nonnull table_column,
                               CGFloat width) API_AVAILABLE(macos(26.0));

/* -[NSTableColumn width] */
CGFloat ns_table_column_width(ns_table_column *_Nonnull table_column)
    API_AVAILABLE(macos(26.0));

/* -[NSTableColumn setMinWidth:] - the width the column stops shrinking at. */
void ns_table_column_set_min_width(ns_table_column *_Nonnull table_column,
                                   CGFloat min_width)
    API_AVAILABLE(macos(26.0));

/* -[NSTableColumn minWidth] */
CGFloat ns_table_column_min_width(ns_table_column *_Nonnull table_column)
    API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_TABLE_COLUMN_H */

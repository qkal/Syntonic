/*
 * NSPopUpButton - the one-of-many control the detail pane picks a category
 * with and the settings panes pick a theme with (R17).
 *
 * The class name is a run of capitals per word, so NSPopUpButton is
 * `ns_pop_up_button` (R5).
 *
 * Items are titles, added by pointer plus count or one at a time, and read
 * back as a count plus an index accessor (KTD17). The selection is an index,
 * with -1 for no selection, exactly as AppKit spells it (R11).
 *
 * COMMITTING A CHOICE (R17). A pop-up's action - installed with
 * ns_control_set_action on the control upcast - fires the moment the user
 * picks an item, so the choice commits immediately. Picking an item
 * programmatically with ns_pop_up_button_select_item_at_index does not fire
 * it, which is AppKit's own behaviour: a program that changes the selection
 * already knows it did. ns_control_send_action fires it if you want both.
 *
 * ACCESSIBILITY (R23). A pop-up has no title of its own to be named after, so
 * a form row's pop-up is one of the places to call
 * ns_view_set_accessibility_label - with the label of the row it sits in,
 * exactly as a nib would have.
 */

#ifndef SYNTONIC_NS_POP_UP_BUTTON_H
#define SYNTONIC_NS_POP_UP_BUTTON_H

#include <CoreGraphics/CGGeometry.h>
#include <os/availability.h>
#include <stdbool.h>

#include <syntonic/ns_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_pop_up_button ns_pop_up_button;

/* Declared by ns_button.h, ns_control.h and ns_view.h and repeated here so
 * this header stands alone. C has allowed a repeated typedef of the same type
 * since C11. */
typedef struct ns_button ns_button;
typedef struct ns_control ns_control;
typedef struct ns_view ns_view;

/* -[NSPopUpButton initWithFrame:pullsDown:] - owned (+1), released with
 * ns_release (R7). False for a pop-up, which shows the current selection and
 * presents its menu over itself; true for a pull-down, which keeps its own
 * title and presents its menu below. */
ns_pop_up_button *_Nonnull ns_pop_up_button_create_with_frame_pulls_down(
    CGRect frame, bool pulls_down) API_AVAILABLE(macos(26.0));

/* -[NSPopUpButton addItemWithTitle:] - appends. A pop-up with no selection
 * selects the first item it is given. */
void ns_pop_up_button_add_item_with_title(
    ns_pop_up_button *_Nonnull pop_up_button, const char *_Nonnull title)
    API_AVAILABLE(macos(26.0));

/* -[NSPopUpButton addItemsWithTitles:] - an array in is a pointer plus a
 * count, borrowed for the call (R11, KTD17). A count of zero adds nothing and
 * `titles` is not read. */
void ns_pop_up_button_add_items_with_titles(
    ns_pop_up_button *_Nonnull pop_up_button,
    const char *_Nonnull const *_Nullable titles, long count)
    API_AVAILABLE(macos(26.0));

/* -[NSPopUpButton numberOfItems] - the count half of the array shape (KTD17). */
long ns_pop_up_button_number_of_items(
    ns_pop_up_button *_Nonnull pop_up_button) API_AVAILABLE(macos(26.0));

/* -[NSPopUpButton itemTitleAtIndex:] - the index half. A string Syntonic
 * returns is always an owned copy, freed with ns_string_free (R11). */
char *_Nullable ns_pop_up_button_copy_item_title_at_index(
    ns_pop_up_button *_Nonnull pop_up_button, long index)
    API_AVAILABLE(macos(26.0));

/* -[NSPopUpButton selectItemAtIndex:] - -1 selects nothing, as AppKit spells
 * it. Any other index outside the items is reported and stops a debug build:
 * AppKit takes it silently and the caller would get a wrong selection rather
 * than a report (R12). */
void ns_pop_up_button_select_item_at_index(
    ns_pop_up_button *_Nonnull pop_up_button, long index)
    API_AVAILABLE(macos(26.0));

/* -[NSPopUpButton selectItemWithTitle:] - a title no item carries selects
 * nothing, which is AppKit's own behaviour and not misuse. */
void ns_pop_up_button_select_item_with_title(
    ns_pop_up_button *_Nonnull pop_up_button, const char *_Nonnull title)
    API_AVAILABLE(macos(26.0));

/* -[NSPopUpButton indexOfSelectedItem] - -1 when nothing is selected. */
long ns_pop_up_button_index_of_selected_item(
    ns_pop_up_button *_Nonnull pop_up_button) API_AVAILABLE(macos(26.0));

/* -[NSPopUpButton titleOfSelectedItem] - a copy property, so this is an owned
 * copy the caller frees with ns_string_free (R7, R11). Null when nothing is
 * selected. */
char *_Nullable ns_pop_up_button_copy_title_of_selected_item(
    ns_pop_up_button *_Nonnull pop_up_button) API_AVAILABLE(macos(26.0));

/* The upcast to ns_button: the same pointer, checked in a debug build (R4,
 * KTD9). syntonic-owned. */
ns_button *_Nonnull ns_pop_up_button_as_button(
    ns_pop_up_button *_Nonnull pop_up_button) API_AVAILABLE(macos(26.0));

/* The upcast to ns_control: the same pointer, checked in a debug build (R4,
 * KTD9). This is where the pop-up's action and its enabled flag live.
 * syntonic-owned. */
ns_control *_Nonnull ns_pop_up_button_as_control(
    ns_pop_up_button *_Nonnull pop_up_button) API_AVAILABLE(macos(26.0));

/* The upcast to ns_view: the same pointer, checked in a debug build (R4,
 * KTD9). syntonic-owned. */
ns_view *_Nonnull ns_pop_up_button_as_view(
    ns_pop_up_button *_Nonnull pop_up_button) API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_POP_UP_BUTTON_H */

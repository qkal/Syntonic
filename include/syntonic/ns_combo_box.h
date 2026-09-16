/*
 * NSComboBox - a text field with a list attached: the caller either hands it a
 * list of values or answers for them from a data source (R9, R11, R17, KTD6,
 * KTD17).
 *
 * TWO MODES, AND AppKit KEEPS THEM APART. `usesDataSource` is false on a new
 * combo box and the list is the static one: ns_combo_box_add_item_with_object_value
 * and the seven calls beside it own it. Set it true and the list comes from
 * the callbacks struct instead, and those eight calls are misuse - AppKit
 * answers them from the static list it is no longer showing, logs a line to
 * the console and hands back a wrong value or raises out of an empty array.
 * The wrapper reports which mode each call needs and stops a debug build
 * (R12), and every function below says which mode it belongs to.
 *
 * An object value crosses as a UTF-8 string (R11): AppKit takes any `id`, and
 * a string is the one shape the boundary has for it. A value Syntonic returns
 * is an owned copy freed with ns_string_free.
 *
 * THE MERGED STRUCT (KTD6). NSComboBoxDataSource answers for the list and
 * NSComboBoxDelegate reports the pop-up and the selection, and Syntonic merges
 * the pair into one ns_combo_box_callbacks that one call installs into both
 * AppKit slots. NSComboBoxDelegate inherits NSTextFieldDelegate, so the text
 * field's own struct is this one's first member (R5) and the field's two
 * editing members are reached as `callbacks.text_field.did_change`.
 *
 * Installing the struct is installing a data source, because
 * `number_of_items` and `object_value_for_item_at_index` are required
 * (docs/conventions.md's per-protocol table). So it needs `usesDataSource`
 * true, set before the install: AppKit's combo box cell drops a data source
 * assigned while it is false, and the list would stay empty with nothing but a
 * console line to say why. A static combo box hears about a pick through its
 * action instead - ns_control_set_action on the control upcast - which fires
 * when the user chooses a value or ends an edit, exactly as a text field's
 * does (R17).
 *
 * THE DATA SOURCE SLOT IS `assign`, NOT `weak` (R9, KTD8). AppKit will not
 * zero it and will not retain it, so a data source that dies first leaves a
 * dangling pointer behind. Syntonic's shim cannot: it is held by an
 * associated reference on the combo box, so it lives exactly as long as the
 * combo box, and install, replace and uninstall each assign the AppKit slot
 * before releasing the shim that was in it. Releasing the combo box needs no
 * uninstall first, and uninstalling leaves both slots nil rather than
 * dangling. `tests/test_combo_box.c` pins both halves.
 *
 * ACCESSIBILITY (R23). A combo box has no title of its own, so a form row's
 * combo box is one of the places to call ns_view_set_accessibility_label with
 * the label of the row it sits in, exactly as a nib would have.
 */

#ifndef SYNTONIC_NS_COMBO_BOX_H
#define SYNTONIC_NS_COMBO_BOX_H

#include <CoreGraphics/CGGeometry.h>
#include <os/availability.h>
#include <stdbool.h>

#include <syntonic/ns_base.h>
/* Not a repeated typedef: the struct below embeds ns_text_field_callbacks by
 * value, so this header needs the parent protocol's definition (R5). */
#include <syntonic/ns_text_field.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The handle. One opaque struct type per AppKit class (R4, KTD9). */
typedef struct ns_combo_box ns_combo_box;

/* Declared by ns_control.h and ns_view.h and repeated here so this header
 * stands alone. C has allowed a repeated typedef of the same type since C11. */
typedef struct ns_control ns_control;
typedef struct ns_view ns_view;

/*
 * NSComboBoxDataSource and NSComboBoxDelegate, merged into one struct of
 * function pointers (KTD6), with NSTextFieldDelegate's struct embedded first
 * because NSComboBoxDelegate inherits that protocol (R5).
 *
 * `number_of_items` and `object_value_for_item_at_index` are required and
 * everything else is optional; docs/conventions.md's per-protocol table is
 * what says so, because all three protocols are entirely @optional in the SDK
 * and NSComboBoxDataSource's own header says its first two methods are
 * required when not using bindings. An unset required member is reported at
 * install time in a debug build, before AppKit ever asks for a value; an unset
 * optional member behaves exactly as a method the object does not implement
 * (R9, KTD8).
 *
 * `context` is what you passed to ns_combo_box_set_callbacks, untouched and
 * never freed by the library; `sender` is the combo box, borrowed for the
 * duration of the call. A string a member receives is borrowed for that call;
 * a string a member returns is borrowed by the library, which copies it before
 * AppKit sees it (R11, KTD17). Every member runs on the main thread.
 * syntonic-owned.
 */
typedef struct ns_combo_box_callbacks {
  /* The parent protocol's struct, first (R5): NSComboBoxDelegate inherits
   * NSTextFieldDelegate, so `text_field.did_change` and
   * `text_field.did_end_editing` are this combo box's editing members. Their
   * `sender` is the combo box behind an ns_text_field handle, which is what
   * its text field upcast hands back. */
  ns_text_field_callbacks text_field;

  /* required. -[NSComboBoxDataSource numberOfItemsInComboBox:] - how many
   * values the list has. A negative count is reported in a debug build
   * (KTD8). */
  long (*_Nullable number_of_items)(void *_Nullable context,
                                    ns_combo_box *_Nonnull sender);

  /* required. -[NSComboBoxDataSource comboBox:objectValueForItemAtIndex:] -
   * the value at `index`, as a UTF-8 string the library copies at once (R11).
   * The pointer is borrowed by the library for the duration of the callback,
   * so the caller may reuse its buffer the moment the callback ends. Null is
   * an answer and not misuse: the SDK declares this return nullable. */
  const char *_Nullable (*_Nullable object_value_for_item_at_index)(
      void *_Nullable context, ns_combo_box *_Nonnull sender, long index);

  /* optional. -[NSComboBoxDataSource comboBox:indexOfItemWithStringValue:] -
   * the index of the value spelled `string_value`, or -1 when the list has
   * none, which is the NSNotFound AppKit expects (R11). AppKit asks while it
   * matches what the user typed against the list. */
  long (*_Nullable index_of_item_with_string_value)(
      void *_Nullable context, ns_combo_box *_Nonnull sender,
      const char *_Nonnull string_value);

  /* optional. -[NSComboBoxDataSource comboBox:completedString:] - the whole
   * value `string` is the beginning of, or null for no completion, which the
   * SDK declares as this return's nullable answer. AppKit asks only while
   * `completes` is true. The returned pointer is borrowed by the library and
   * copied at once (R11). */
  const char *_Nullable (*_Nullable completed_string)(
      void *_Nullable context, ns_combo_box *_Nonnull sender,
      const char *_Nonnull string);

  /* optional. -[NSComboBoxDelegate comboBoxSelectionDidChange:] - the
   * selection settled; read it with ns_combo_box_index_of_selected_item on the
   * sender. The notification carries nothing beyond the combo box, so the
   * combo box is what crosses (R5). */
  void (*_Nullable selection_did_change)(void *_Nullable context,
                                         ns_combo_box *_Nonnull sender);

  /* optional. -[NSComboBoxDelegate comboBoxSelectionIsChanging:] - the
   * selection is moving under the pointer and has not settled. */
  void (*_Nullable selection_is_changing)(void *_Nullable context,
                                          ns_combo_box *_Nonnull sender);

  /* optional. -[NSComboBoxDelegate comboBoxWillPopUp:] - the list is about to
   * appear. */
  void (*_Nullable will_pop_up)(void *_Nullable context,
                                ns_combo_box *_Nonnull sender);

  /* optional. -[NSComboBoxDelegate comboBoxWillDismiss:] - the list is about
   * to close. */
  void (*_Nullable will_dismiss)(void *_Nullable context,
                                 ns_combo_box *_Nonnull sender);
} ns_combo_box_callbacks;

/* -[NSControl initWithFrame:], on an NSComboBox - owned (+1), released with
 * ns_release (R7). NSComboBox.h declares no initializer of its own, and a
 * constructor is the one inherited member a subclass redeclares, because an
 * upcast needs an object to upcast. */
ns_combo_box *_Nonnull ns_combo_box_create_with_frame(CGRect frame)
    API_AVAILABLE(macos(26.0));

/* -[NSComboBox setUsesDataSource:] - false is the static list, true is the
 * callbacks struct. Set it before ns_combo_box_set_callbacks: AppKit's cell
 * drops a data source assigned while this is false. */
void ns_combo_box_set_uses_data_source(ns_combo_box *_Nonnull combo_box,
                                       bool uses_data_source)
    API_AVAILABLE(macos(26.0));

/* -[NSComboBox usesDataSource] */
bool ns_combo_box_uses_data_source(ns_combo_box *_Nonnull combo_box)
    API_AVAILABLE(macos(26.0));

/* -[NSComboBox addItemWithObjectValue:] - appends to the static list. The
 * static list only: with `usesDataSource` true this call is reported and stops
 * a debug build (R12). */
void ns_combo_box_add_item_with_object_value(ns_combo_box *_Nonnull combo_box,
                                             const char *_Nonnull value)
    API_AVAILABLE(macos(26.0));

/* -[NSComboBox addItemsWithObjectValues:] - an array in is a pointer plus a
 * count, borrowed for the call (R11, KTD17). A count of zero adds nothing and
 * `values` is not read. The static list only. */
void ns_combo_box_add_items_with_object_values(
    ns_combo_box *_Nonnull combo_box,
    const char *_Nonnull const *_Nullable values, long count)
    API_AVAILABLE(macos(26.0));

/* -[NSComboBox insertItemWithObjectValue:atIndex:] - the count appends. An
 * index past the count raises, and the entry macro reports that exception with
 * this function's name (KTD4). The static list only. */
void ns_combo_box_insert_item_with_object_value_at_index(
    ns_combo_box *_Nonnull combo_box, const char *_Nonnull value, long index)
    API_AVAILABLE(macos(26.0));

/* -[NSComboBox removeItemWithObjectValue:] - a value the list does not carry
 * removes nothing, which is AppKit's own behaviour and not misuse. The static
 * list only. */
void ns_combo_box_remove_item_with_object_value(
    ns_combo_box *_Nonnull combo_box, const char *_Nonnull value)
    API_AVAILABLE(macos(26.0));

/* -[NSComboBox removeItemAtIndex:] - an index past the end raises, and the
 * entry macro reports that exception with this function's name (KTD4). The
 * static list only. */
void ns_combo_box_remove_item_at_index(ns_combo_box *_Nonnull combo_box,
                                       long index) API_AVAILABLE(macos(26.0));

/* -[NSComboBox removeAllItems] - the static list only. */
void ns_combo_box_remove_all_items(ns_combo_box *_Nonnull combo_box)
    API_AVAILABLE(macos(26.0));

/* -[NSComboBox itemObjectValueAtIndex:] - the index half of the list, as an
 * owned copy the caller frees with ns_string_free (R7, R11). An index past the
 * end raises and the entry macro reports it (KTD4). The static list only: with
 * `usesDataSource` true, ask your own model instead. */
char *_Nullable ns_combo_box_copy_item_object_value_at_index(
    ns_combo_box *_Nonnull combo_box, long index) API_AVAILABLE(macos(26.0));

/* -[NSComboBox indexOfItemWithObjectValue:] - -1 when the list carries no such
 * value, which is AppKit's own NSNotFound (R11). The static list only. */
long ns_combo_box_index_of_item_with_object_value(
    ns_combo_box *_Nonnull combo_box, const char *_Nonnull value)
    API_AVAILABLE(macos(26.0));

/* -[NSComboBox reloadData] - asks the callbacks struct for the count and for
 * every value again. */
void ns_combo_box_reload_data(ns_combo_box *_Nonnull combo_box)
    API_AVAILABLE(macos(26.0));

/* -[NSComboBox noteNumberOfItemsChanged] - asks the struct for the count
 * alone, for a list that grew or shrank but did not otherwise change. */
void ns_combo_box_note_number_of_items_changed(
    ns_combo_box *_Nonnull combo_box) API_AVAILABLE(macos(26.0));

/* -[NSComboBox numberOfItems] - the count half of the list, in either mode
 * (KTD17): the static list's own count, or what the struct last answered. */
long ns_combo_box_number_of_items(ns_combo_box *_Nonnull combo_box)
    API_AVAILABLE(macos(26.0));

/* -[NSComboBox selectItemAtIndex:] - -1 selects nothing, as AppKit spells it.
 * Any other index outside the list is reported and stops a debug build: AppKit
 * takes it silently and the caller would get an empty selection rather than a
 * report (R12). Selecting programmatically still fires `selection_did_change`,
 * which is AppKit's own behaviour. */
void ns_combo_box_select_item_at_index(ns_combo_box *_Nonnull combo_box,
                                       long index) API_AVAILABLE(macos(26.0));

/* -[NSComboBox deselectItemAtIndex:] - deselects that index if it is the
 * selected one. An index outside the list is reported and stops a debug build,
 * for the same reason as the call above (R12). */
void ns_combo_box_deselect_item_at_index(ns_combo_box *_Nonnull combo_box,
                                         long index)
    API_AVAILABLE(macos(26.0));

/* -[NSComboBox selectItemWithObjectValue:] - a value the list does not carry
 * selects nothing, which is AppKit's own behaviour and not misuse; null
 * clears the selection. The static list only. */
void ns_combo_box_select_item_with_object_value(
    ns_combo_box *_Nonnull combo_box, const char *_Nullable value)
    API_AVAILABLE(macos(26.0));

/* -[NSComboBox indexOfSelectedItem] - -1 when nothing is selected. */
long ns_combo_box_index_of_selected_item(ns_combo_box *_Nonnull combo_box)
    API_AVAILABLE(macos(26.0));

/* -[NSComboBox objectValueOfSelectedItem] - the selected value as an owned
 * copy the caller frees with ns_string_free (R7, R11); null when nothing is
 * selected. The static list only: with `usesDataSource` true, read
 * ns_combo_box_index_of_selected_item and ask your own model. */
char *_Nullable ns_combo_box_copy_object_value_of_selected_item(
    ns_combo_box *_Nonnull combo_box) API_AVAILABLE(macos(26.0));

/* -[NSComboBox scrollItemAtIndexToTop:] - scrolls the open list so that item
 * is its first visible row. */
void ns_combo_box_scroll_item_at_index_to_top(ns_combo_box *_Nonnull combo_box,
                                              long index)
    API_AVAILABLE(macos(26.0));

/* -[NSComboBox scrollItemAtIndexToVisible:] - scrolls the open list by as
 * little as it takes to show that item. */
void ns_combo_box_scroll_item_at_index_to_visible(
    ns_combo_box *_Nonnull combo_box, long index) API_AVAILABLE(macos(26.0));

/* -[NSComboBox setHasVerticalScroller:] */
void ns_combo_box_set_has_vertical_scroller(ns_combo_box *_Nonnull combo_box,
                                            bool has_vertical_scroller)
    API_AVAILABLE(macos(26.0));

/* -[NSComboBox hasVerticalScroller] */
bool ns_combo_box_has_vertical_scroller(ns_combo_box *_Nonnull combo_box)
    API_AVAILABLE(macos(26.0));

/* -[NSComboBox setNumberOfVisibleItems:] - how many rows the list shows before
 * it scrolls. */
void ns_combo_box_set_number_of_visible_items(ns_combo_box *_Nonnull combo_box,
                                              long number_of_visible_items)
    API_AVAILABLE(macos(26.0));

/* -[NSComboBox numberOfVisibleItems] */
long ns_combo_box_number_of_visible_items(ns_combo_box *_Nonnull combo_box)
    API_AVAILABLE(macos(26.0));

/* -[NSComboBox setItemHeight:] - the height of one row of the list. */
void ns_combo_box_set_item_height(ns_combo_box *_Nonnull combo_box,
                                  CGFloat item_height)
    API_AVAILABLE(macos(26.0));

/* -[NSComboBox itemHeight] */
CGFloat ns_combo_box_item_height(ns_combo_box *_Nonnull combo_box)
    API_AVAILABLE(macos(26.0));

/* -[NSComboBox setIntercellSpacing:] - the gap around one row of the list. An
 * NSSize crosses unchanged as a CGSize (R11). */
void ns_combo_box_set_intercell_spacing(ns_combo_box *_Nonnull combo_box,
                                        CGSize intercell_spacing)
    API_AVAILABLE(macos(26.0));

/* -[NSComboBox intercellSpacing] */
CGSize ns_combo_box_intercell_spacing(ns_combo_box *_Nonnull combo_box)
    API_AVAILABLE(macos(26.0));

/* -[NSComboBox setButtonBordered:] - whether the arrow at the right edge is
 * drawn with a border. */
void ns_combo_box_set_button_bordered(ns_combo_box *_Nonnull combo_box,
                                      bool button_bordered)
    API_AVAILABLE(macos(26.0));

/* -[NSComboBox isButtonBordered] - a BOOL getter drops AppKit's `is` (R5). */
bool ns_combo_box_button_bordered(ns_combo_box *_Nonnull combo_box)
    API_AVAILABLE(macos(26.0));

/* -[NSComboBox setCompletes:] - whether AppKit completes what the user types
 * from the list. True is what makes AppKit ask `completed_string`. */
void ns_combo_box_set_completes(ns_combo_box *_Nonnull combo_box,
                                bool completes) API_AVAILABLE(macos(26.0));

/* -[NSComboBox completes] */
bool ns_combo_box_completes(ns_combo_box *_Nonnull combo_box)
    API_AVAILABLE(macos(26.0));

/* Installs the callbacks struct as the combo box's data source and its
 * delegate in one call (KTD6). The struct is copied; `context` is not, and has
 * to stay valid until you uninstall or the combo box is deallocated -
 * ns_release does not uninstall. Installing again replaces the previous struct
 * and context; a null `callbacks` uninstalls and leaves both AppKit slots nil,
 * which is what the `assign` data source slot needs (R9, KTD8). A missing
 * required member is reported here, in a debug build, before AppKit ever asks.
 * `usesDataSource` has to be true first, or the install is reported and stops
 * a debug build (R12). syntonic-owned. */
void ns_combo_box_set_callbacks(ns_combo_box *_Nonnull combo_box,
                                const ns_combo_box_callbacks *_Nullable callbacks,
                                void *_Nullable context)
    API_AVAILABLE(macos(26.0));

/* The upcast to ns_text_field: the same pointer, checked in a debug build (R4,
 * KTD9). This is where the placeholder and the text colour live.
 * syntonic-owned. */
ns_text_field *_Nonnull ns_combo_box_as_text_field(
    ns_combo_box *_Nonnull combo_box) API_AVAILABLE(macos(26.0));

/* The upcast to ns_control: the same pointer, checked in a debug build (R4,
 * KTD9). This is where the combo box's text, its action and its enabled flag
 * live. syntonic-owned. */
ns_control *_Nonnull ns_combo_box_as_control(ns_combo_box *_Nonnull combo_box)
    API_AVAILABLE(macos(26.0));

/* The upcast to ns_view: the same pointer, checked in a debug build (R4,
 * KTD9). syntonic-owned. */
ns_view *_Nonnull ns_combo_box_as_view(ns_combo_box *_Nonnull combo_box)
    API_AVAILABLE(macos(26.0));

#ifdef __cplusplus
}
#endif

#endif /* SYNTONIC_NS_COMBO_BOX_H */

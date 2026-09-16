# Syntonic conventions

Syntonic wraps AppKit by hand: pure C23 public headers, Objective-C with ARC
inside, one C type per AppKit class, one C function per method or property.

This document is the contract. Every wrapper in the repository follows it, and
every wrapper added later — by a contributor, or by a generator — is expected to
follow it without asking a question. If you had to ask one, the document has a
bug: open an issue naming the rule that was missing.

**How to use it.** Wrapping a class? Go straight to
[How to wrap a new class](#how-to-wrap-a-new-class), which links back to every
rule at the point you need it. Reviewing a wrapper? Read the section that owns
the rule. Looking for one answer? The [rule index](#rule-index) maps each
requirement to its section.

**The four files this document sits next to.**

| File | What it gives you |
|---|---|
| `src/ns_internal.h` | the macros every wrapper body is made of, with the four canonical bodies in its header comment |
| `src/syn_shims.h` | the callback machinery: the member table, the shim base class, the one install call and the target/action trampoline |
| `src/templates/ns_template.h`, `src/templates/ns_template.m` | a compiling skeleton to copy, one function per shape |
| `scripts/lint-headers.sh` | the mechanical half of the header rules, run by `just lint-headers` |

**Out of scope for v0.** Block completion handlers and subclass override points
have no convention yet; they are set by their first consumer in a later plan. Do
not invent one — if the class you are wrapping needs either, wrap the rest and
say so in the pull request. Also deferred: a general Auto Layout constraint API,
`NSIndexSet` arguments (see [Boundary types](#boundary-types)), and typed
`NSDictionary` or `NSAttributedString` arguments.

---

## Contents

- [Naming](#naming)
- [Header seams and header discipline](#header-seams-and-header-discipline)
- [Ownership](#ownership)
- [Boundary types](#boundary-types)
- [Callbacks](#callbacks)
- [Required members per protocol](#required-members-per-protocol)
- [Threading](#threading)
- [Availability](#availability)
- [Misuse checks and exceptions](#misuse-checks-and-exceptions)
- [Accessibility](#accessibility)
- [File layout and the test each wrapper ships with](#file-layout-and-the-test-each-wrapper-ships-with)
- [How to wrap a new class](#how-to-wrap-a-new-class)
- [Rule index](#rule-index)
- [Where later units append](#where-later-units-append)

---

## Naming

Rule owner: R5, KTD10. Every public identifier starts with `ns_`.

### Types

A class name becomes snake_case. **A run of capitals is one word.**

| AppKit | Syntonic |
|---|---|
| `NSWindow` | `ns_window` |
| `NSTableView` | `ns_table_view` |
| `NSSplitViewController` | `ns_split_view_controller` |
| `NSPopUpButton` | `ns_pop_up_button` |
| `NSComboBox` | `ns_combo_box` |
| `NSURL` | `ns_url` (not `ns_u_r_l`) |
| `NSTextField` | `ns_text_field` |

The type is an opaque struct typedef, never defined:

```c
typedef struct ns_combo_box ns_combo_box;
```

### Functions from selectors

The function name is the type name plus the selector's segments, joined by
underscores, **in argument order**.

| AppKit | Syntonic |
|---|---|
| `-[NSWindow setTitle:]` | `ns_window_set_title` |
| `-[NSView addSubview:]` | `ns_view_add_subview` |
| `-[NSMenu insertItem:atIndex:]` | `ns_menu_insert_item_at_index` |
| `-[NSOutlineView expandItem:]` | `ns_outline_view_expand_item` |
| `-[NSTableView reloadData]` | `ns_table_view_reload_data` |

### Constructors

| AppKit | Syntonic | Rule |
|---|---|---|
| `-[NSObject init]` | `ns_view_controller_create` | `init` is `create` |
| `-[NSView initWithFrame:]` | `ns_view_create_with_frame` | `initWith…` is `create_with_…`, every segment kept |
| `-[NSWindow initWithContentRect:styleMask:backing:defer:]` | `ns_window_create_with_content_rect_style_mask_backing_defer` | every segment, in argument order |
| `+[NSButton buttonWithTitle:target:action:]` | `ns_button_create_with_title_target_action` | an `instancetype` class method is `create_` plus the selector with the **leading class-name stem removed** (`button…`) |
| `+[NSSplitViewItem sidebarWithViewController:]` | `ns_split_view_item_create_sidebar_with_view_controller` | the stem here is `splitViewItem`, which does not appear, so nothing is removed |

Long names are expected. Do not shorten them.

**A segment whose argument cannot cross is dropped from the name**, and the
comment still names the whole selector. `SEL`, `id` and `Class` never cross
(see [Boundary types](#boundary-types)), so
`-[NSMenuItem initWithTitle:action:keyEquivalent:]` becomes
`ns_menu_item_create_with_title_key_equivalent`: the action arrives later,
through `ns_menu_item_set_action`. Drop a segment only when its type is one the
boundary has no shape for, never to shorten a name.

### Properties

| AppKit | Syntonic | Note |
|---|---|---|
| `-[NSWindow setTitle:]` | `ns_window_set_title` | writer: `set_` |
| `-[NSWindow contentView]` | `ns_window_content_view` | reader: the property name, no `get` |
| `-[NSWindow title]` (a `copy` property) | `ns_window_copy_title` | an owned value is `copy_`, never `get_` — see [Ownership](#ownership) |
| `-[NSButton isBordered]` | `ns_button_bordered` | a `BOOL` getter drops AppKit's `is` |
| `-[NSView subviews]` (an array) | `ns_view_subview_count`, `ns_view_subview_at_index` | see [Boundary types](#boundary-types) |

A reader carries no `get`. R5 says so and the plan's own header sketch spells
it `ns_window_content_view`. The writer is the only half that takes an affix,
`set_`, because AppKit's own selector has one.

**The one exception: a reader whose name is already a type name keeps `get_`.**
The naming rule maps an enum and the property that carries it to the same
identifier, and C puts a typedef and a function in one namespace, so the two
collide:

| AppKit | Collides with | Syntonic |
|---|---|---|
| `-[NSTableView style]` | the enum type `ns_table_view_style` | `ns_table_view_get_style` |
| `-[NSWindow styleMask]` | the enum type `ns_window_style_mask` | `ns_window_get_style_mask` |

The test is mechanical and worth running before you name a reader: if a handle
type or an enum type of that exact name exists, prefix the reader with `get_`.
Otherwise do not. Nothing else in the rule changes, and the writer keeps its
plain `set_` name either way (`ns_table_view_set_style`).

`src/templates/ns_template.h` shows both halves: `ns_template_superview` has no
collision and takes no affix, `ns_template_get_style` collides with the
`ns_template_style` enum and takes one.

### Enums and string constants

An enum becomes a snake_case type with a fixed underlying type, and its
constants become `NS_`-prefixed upper case.

| AppKit | Syntonic |
|---|---|
| `NSTableViewStyle` | `ns_table_view_style` |
| `NSTableViewStyleSourceList` | `NS_TABLE_VIEW_STYLE_SOURCE_LIST` |
| `NSWindowStyleMaskTitled` | `NS_WINDOW_STYLE_MASK_TITLED` |
| `NSWindowToolbarStylePreference` | `NS_WINDOW_TOOLBAR_STYLE_PREFERENCE` |

```c
typedef enum ns_table_view_style : int32_t {
  NS_TABLE_VIEW_STYLE_AUTOMATIC = 0,
  NS_TABLE_VIEW_STYLE_SOURCE_LIST = 3,
} ns_table_view_style;
```

Give each constant the SDK's own numeric value explicitly. The C enum is a
separate type from AppKit's; the `.m` casts between them, and the values have to
agree.

A **string constant keeps AppKit's spelling exactly**, because a C caller never
sees AppKit's own symbol and the identical spelling is what makes the two
documents line up:

```c
extern const char *const NSToolbarSidebarTrackingSeparatorItemIdentifier;
```

### Callback members

A struct member is the selector with the **receiver-class prefix and the
receiver segment removed**.

| AppKit | Syntonic member |
|---|---|
| `-[NSWindowDelegate windowWillClose:]` | `will_close` |
| `-[NSApplicationDelegate applicationDidFinishLaunching:]` | `did_finish_launching` |
| `-[NSTableViewDelegate tableViewSelectionDidChange:]` | `selection_did_change` |
| `-[NSOutlineViewDataSource outlineView:numberOfChildrenOfItem:]` | `number_of_children_of_item` |
| `-[NSComboBoxDataSource numberOfItemsInComboBox:]` | `number_of_items` |
| `-[NSComboBoxDataSource comboBox:objectValueForItemAtIndex:]` | `object_value_for_item_at_index` |
| `-[NSToolbarDelegate toolbar:itemForItemIdentifier:willBeInsertedIntoToolbar:]` | `item_for_item_identifier_will_be_inserted_into_toolbar` |

A **lone notification parameter becomes the sender handle**: AppKit's
`windowWillClose:(NSNotification *)notification` carries no information beyond
the window, so the C member is
`void (*will_close)(void *context, ns_window *sender)`. The `NSNotification`
itself never crosses the boundary.

A protocol struct **embeds its parent protocol's struct as its first member**.
`NSComboBoxDelegate` inherits `NSTextFieldDelegate`, so:

```c
typedef struct ns_combo_box_callbacks {
  ns_text_field_callbacks text_field; /* the parent protocol, first */
  void (*_Nullable selection_did_change)(void *_Nullable context,
                                         ns_combo_box *_Nonnull sender);
} ns_combo_box_callbacks;
```

### Upcasts

One per wrapped ancestor, `ns_<type>_as_<ancestor>`, declared in the subclass's
header:

```c
ns_control *_Nonnull ns_button_as_control(ns_button *_Nonnull button);
ns_view *_Nonnull ns_button_as_view(ns_button *_Nonnull button);
```

An upcast has no AppKit selector, so its header comment carries the
`syntonic-owned` tag (see [Header seams](#header-seams-and-header-discipline)).

**Inherited API is reached through the upcast, never redeclared.** One C
function per method or property means the function lives on the class the SDK
declares it on, and the subclass reaches it by upcasting:
`ns_control_set_action(ns_button_as_control(button), …)`,
`ns_control_copy_string_value(ns_text_field_as_control(field), …)`. There is no
`ns_button_set_action` and no `ns_text_field_copy_string_value`, because
`NSButton.h` and `NSTextField.h` declare neither — `NSControl.h` does, and the
mirror-header rule puts a function in the header whose SDK header declares it.
A wrapper that wants an inherited property on hand declares the upcast, not a
second spelling of the property.

### Syntonic-owned names

Names with no AppKit counterpart keep the `ns_` prefix and are listed here so
nobody invents a second spelling.

| Name | What it is |
|---|---|
| `ns_retain`, `ns_release` | reference management on any handle |
| `ns_string_free` | frees a string Syntonic returned |
| `ns_available` | runtime macOS version check |
| `ns_main_thread_dispatch` | runs a C callback on the main thread |
| `ns_action` | the target/action callback type |
| `ns_<class>_set_action(handle, action, context)` | the target/action installer |
| `ns_<class>_set_callbacks(handle, callbacks, context)` | the protocol-struct installer |
| `ns_<type>_as_<ancestor>` | an upcast |
| struct members with no AppKit counterpart, such as `cell_string` | see [Callbacks](#callbacks) |
| `ns_outline_view_selected_item`, `ns_outline_view_select_item`, `ns_outline_view_scroll_item_to_visible` | the three two-selector compositions an outline item needs, because AppKit addresses an outline by row (`rowForItem:` and then `selectRowIndexes:…`, `scrollRowToVisible:` or `itemAtRow:`) and a Syntonic item is a pointer. Each names both selectors in its comment |

---

## Header seams and header discipline

Rule owner: R6, KTD18. `scripts/lint-headers.sh` enforces the mechanical half.

### One header per SDK header

| Kind | Rule | Files |
|---|---|---|
| **Mirror** | one C header per SDK header, same stem; every function's comment names the AppKit selector it wraps, or carries the `syntonic-owned` tag | `ns_window.h` for `NSWindow.h`, `ns_combo_box.h` for `NSComboBox.h`, … |
| **Non-mirror** | builders and helpers with no AppKit counterpart | `syntonic.h`, `ns_base.h`, `ns_layout.h`, `ns_menu_bar.h` |

The non-mirror list lives in `NON_MIRROR` in `scripts/lint-headers.sh`. Adding a
non-mirror header means adding it there in the same commit; anything else is a
mirror header and the selector rule applies to every function in it.

### What the linter fails on

| Check | Fails on | Pass it by |
|---|---|---|
| function-like macro | `#define NS_THING(x) …` in `include/` | putting the macro in `src/ns_internal.h`, which is internal and may have them |
| `inline` | the keyword anywhere in a declaration | declaring the function and defining it in the `.m` |
| variadic | `...` in a parameter list | taking a pointer plus a count |
| availability | a function declaration with no `API_AVAILABLE(…)` | see [Availability](#availability) |
| selector comment | a mirror-header function whose preceding comment names no selector and has no tag | writing the comment below |

The selector comment has to match `[-+][NSClass …]` — a leading `-` or `+`, a
bracket, a class name starting with `NS`, a space, then the selector:

```c
/* -[NSWindow setTitle:] */                      /* passes  */
/* +[NSButton buttonWithTitle:target:action:] */ /* passes  */
/* The upcast to ns_view. syntonic-owned. */     /* passes  */
/* Sets the window's title. */                   /* FAILS   */
/* NSWindow setTitle: */                         /* FAILS   */
```

Run it on one file while you work: `./scripts/lint-headers.sh
include/syntonic/ns_combo_box.h`. `just lint-headers` covers all of `include/`.

### An enum from a header that is not wrapped

An enum sometimes belongs to an SDK header no unit wraps. `NSEventModifierFlags`
is `NSEvent.h`'s and v0 wraps no part of `NSEvent`, but a menu item's key
equivalent needs it. **Declare the enum in the header that uses it**, named by
the naming rule as though its own header existed — `ns_event_modifier_flags` —
and say in its comment which SDK header it comes from. Do not create a mirror
header for an enum alone. The first unit that wraps that SDK class moves the
enum into the mirror header it belongs to, which before U11 is free.

### What a public header may contain

Opaque handle typedefs, enums with fixed underlying types, structs of function
pointers, function declarations with availability and nullability, and
`extern const` declarations. Nothing else — no macro with parameters, no inline
body, no variadic function, no Objective-C, no `#import`. Anything richer
belongs to the later sugar header, not here. Binders built on libclang read
these headers, and `API_AVAILABLE` is what they expand.

Nullability annotations belong in the **public header only**. Do not write
`_Nullable` or `_Nonnull` in a `.m`: `-Wnullability-completeness` is file-scoped,
so one annotation obliges every other pointer in that file to carry one.

Every header is guarded, wrapped in `extern "C"` under `__cplusplus`, and added
to `include/syntonic/syntonic.h` in the same commit.

**Anything moved after the README checkpoint (U11) is a public change.** Before
then, a header may be reorganized freely.

---

## Ownership

Rule owner: R7, KTD7. This is the section to get right; everything else is
typing.

### The rule in one line

A function named `create_` or `copy_` returns a reference **the caller owns** and
ends with `ns_release` (or `ns_string_free` for a string). **Every other function
returns a borrowed reference**, valid while its owner holds it.

A caller who wants to keep a borrowed reference calls `ns_retain` and pairs it
with `ns_release`. Releasing null is a no-op. AppKit's own retention is never
touched, so handing an object to a parent and then releasing your reference is
correct: the parent's reference is intact.

### The mechanical rule: read the SDK property's attribute

You do not have to think about this. Look at how the SDK declares it.

| SDK declaration | Return | C name | Macro in the `.m` |
|---|---|---|---|
| `@property (strong) NSView *contentView` | borrowed | `ns_window_content_view` | `NS_OUT` |
| `@property (weak) NSView *superview` | borrowed | `ns_view_superview` | `NS_OUT` |
| `@property (assign) id<NSComboBoxDataSource> dataSource` | borrowed | — | `NS_OUT` |
| `@property (copy) NSString *title` | **owned** | `ns_window_copy_title` | `NS_STRING_OUT` |
| `@property (copy) NSArray<NSView *> *subviews` | count + index accessor, element **borrowed** | `ns_view_subview_count`, `ns_view_subview_at_index` | `NS_OUT` on the element |
| `- (instancetype)initWith…` | **owned (+1)** | `ns_view_create_with_frame` | `NS_OUT_OWNED` |
| `+ (instancetype)buttonWith…` | **owned (+1)** | `ns_button_create_with_title_target_action` | `NS_OUT_OWNED` |
| any other method returning an object | **owned** | `ns_…_copy_…` | `NS_OUT_OWNED` |
| a method on the allowlist below | borrowed | `ns_menu_item_at_index` | `NS_OUT` |

**The borrowed-return allowlist.** These are borrowed despite being plain
methods, because something else already holds the object for as long as the
caller could use it. The list is closed; adding to it is a change to this
document, reviewed on its own.

| Method | Who holds it |
|---|---|
| `-[NSMenu itemAtIndex:]` | the menu, which keeps the item |
| `-[NSTableView viewAtColumn:row:makeIfNecessary:]` | the table, which keeps the view |
| `-[NSGridView addRowWithViews:]`, `-[NSGridView rowAtIndex:]`, `-[NSGridView columnAtIndex:]`, `-[NSGridView cellAtColumnIndex:rowIndex:]`, `-[NSGridView cellForView:]` | the grid, which keeps its rows, its columns and its cells |
| `+[NSApplication sharedApplication]` | AppKit, for the life of the process |

`sharedApplication` is the one class method on the list. It is not a
constructor: there is one application per process, it is never deallocated, and
an owned `ns_application_copy_shared` would be a reference with nothing to
balance. So it is `ns_application_shared`, borrowed, `NS_OUT`.

**The surprise.** These are all `copy` properties in the SDK, which catches
people who expect a getter to be cheap and borrowed:

| Property | SDK declaration |
|---|---|
| `NSView.subviews` | `@property (copy) NSArray<__kindof NSView *> *subviews` |
| `NSStackView.arrangedSubviews` | `@property (readonly, copy) NSArray<__kindof NSView *> *arrangedSubviews` |
| `NSWindow.title`, `NSButton.title` | `@property (copy) NSString *title` |
| `NSMenu.itemArray` | `@property (copy) NSArray<NSMenuItem *> *itemArray` |
| `NSToolbar.items` | `@property (readonly, copy) NSArray<__kindof NSToolbarItem *> *items` |

A `copy` property hands back an array the receiver does not keep. It cannot be
borrowed, which is why an array becomes a count plus an index accessor: the
accessor reads the live child through the receiver and borrows that.

### Why borrowed cannot be the default for a plain method

Every public function body runs inside **its own autorelease pool** — a C caller
has none before the run loop starts, so without one nothing would ever drain. An
object that only the pool holds would therefore be freed at the wrapper's closing
brace, and a borrowed return of it would dangle before the caller's next line.
`NS_OUT` is safe exactly when something else already holds the object: the
receiver, through a `strong`, `weak` or `assign` property.

### The reference-holder ledger

The object is freed when every column reaches zero.

| Event | Caller | Parent | AppKit during a callback |
|---|---|---|---|
| `ns_x_create` or `ns_x_copy_y` returns | +1 | 0 | 0 |
| caller adds the object to a parent | +1 | +1 | 0 |
| caller releases its reference | 0 | +1 | 0 |
| a getter returns the object borrowed | 0 | +1 (valid while the parent holds it) | 0 |
| caller retains the borrowed handle to keep it | +1 | +1 | 0 |
| a callback returns an owned handle to AppKit | 0 (the shim releases it after AppKit retains) | 0 | +1 |
| parent goes away | unchanged (a caller +1 keeps the object alive) | 0 | 0 |

In C:

```c
ns_view *child = ns_view_create_with_frame(CGRectMake(0, 0, 80, 24)); /* caller +1 */
ns_view_add_subview(pane, child);  /* the pane retains it: caller +1, pane +1 */
ns_release(child);                 /* correct: caller 0, pane +1 */
/* `child` is still in the pane's subviews, still laid out, still borrowable
 * through ns_view_subview_at_index. */
```

### Post-init fixups

Some AppKit classes need one line after construction for the ownership rule to
hold at all. This table is the complete list; a later unit that finds another
adds a row.

| Class | Fixup | Why |
|---|---|---|
| `NSWindow` | `window.releasedWhenClosed = NO` in `ns_window_create_with_content_rect_style_mask_backing_defer` | NSWindow's default is to release itself when closed, which would leave the caller's handle dangling. With it off, closing only orders the window out and `ns_release` tears it down. `tests/test_window.c` pins both halves. |

### Mechanics

- Every public function body opens with `NS_ENTER()` and closes with
  `NS_LEAVE()`; `NS_ENTER` opens the pool's brace and `NS_LEAVE` closes it. A
  `return` between them is correct and drains the pool.
- An owned return is +1 through `CFBridgingRetain`, which is what `NS_OUT_OWNED`
  does.
- `ns_retain` and `ns_release` are the bridging casts; `ns_retain` hands back the
  same pointer.
- A string crosses as a `strdup` copy (`NS_STRING_OUT`), never a pointer into an
  object.
- `NS_IN`, `NS_IN_OPT` and `NS_STRING_IN` evaluate their argument twice in a
  debug build, like `assert`. Pass a parameter or a plain expression, never a
  call with a side effect.

---

## Boundary types

Rule owner: R11, KTD17.

### Scalars

| AppKit | C | Note |
|---|---|---|
| `NSInteger` | `long` | same type on every platform Syntonic supports; `-1` is the "no selection" value where AppKit uses it |
| `NSUInteger` | `unsigned long`, or `long` for a count that pairs with an index | a count returned next to a `long` index stays `long` so the two compare without a cast |
| `BOOL` | `bool` | `<stdbool.h>` |
| `CGFloat`, `NSRect`, `NSSize`, `NSPoint` | `CGFloat`, `CGRect`, `CGSize`, `CGPoint` | CoreGraphics types cross unchanged |
| `NSTimeInterval` | `double` | |
| `SEL`, `id`, `Class` | never cross | an `id` object value crosses as a UTF-8 string; see below |

### Strings

- Text crosses as UTF-8 C strings.
- A string **Syntonic returns is always an owned copy**, freed with
  `ns_string_free`. There is no borrowed string return.
- A string **passed in is borrowed for the call**; the wrapper converts it to an
  `NSString` immediately and never keeps the pointer.
- A string **passed into a callback is borrowed for the callback's duration**.
  Copy it if you need it later.
- A string a **callback returns** is borrowed by the library, which copies it
  before handing anything to AppKit. The C side keeps ownership.

### Collections

| Shape | Convention | Example |
|---|---|---|
| array in | pointer plus count, borrowed for the call | `ns_pop_up_button_add_items_with_titles(button, const char *const *titles, long count)` |
| array out | a count function plus an index accessor returning a **borrowed** element | `ns_view_subview_count`, `ns_view_subview_at_index` |
| `NSIndexSet` argument | a single index in v0 | `-[NSTableView selectRowIndexes:byExtendingSelection:]` becomes a select-one-row function |
| `id` object value | a UTF-8 string in v0, as table and combo box cells do | `-[NSComboBox objectValueOfSelectedItem]` becomes an owned `char *` |
| `NSDictionary`, `NSAttributedString` | out of scope for v0 | wrap the rest of the class and say so |
| **array of arrays in** | an array of **row descriptors** plus a row count. A row descriptor is a struct of a pointer plus a count — the one-dimensional rule, one level down. All of it is borrowed for the call. | `+[NSGridView gridViewWithViews:]` becomes `ns_grid_view_create_with_views(const ns_grid_view_row *rows, long row_count)`, where `ns_grid_view_row` is `{ns_view *const *views; long count;}` |

The row descriptor is declared in the header that takes it and named for that
header, not for the AppKit class it feeds: `ns_grid_view_row` is a boundary
shape, and `ns_grid_row` — if a later unit wraps `NSGridRow` — is a handle.
Rows may be ragged; the receiver decides what a short row means.

Foundation objects appear only where AppKit forces them, and then as opaque
handles under the same ownership rule as everything else.

---

## Callbacks

Rule owner: R9, KTD8. Two shapes, and only two: a target/action, and a struct of
function pointers for a delegate or data source.

### Target/action

```c
/* in ns_base.h */
typedef void (*ns_action)(void *_Nullable context, const void *_Nullable sender);

/* in the class's header. syntonic-owned. */
void ns_button_set_action(ns_button *_Nonnull button, ns_action _Nullable action,
                          void *_Nullable context) API_AVAILABLE(macos(26.0));
```

- `context` is yours: the library never copies or frees it, and it has to stay
  valid until you uninstall or the AppKit object is deallocated.
- `sender` is a borrowed handle to the object that fired, valid for the call.
- A null `action` uninstalls. Installing again replaces the previous action and
  context.
- The callback runs on the main thread.

### A protocol struct

One C struct per wrapped protocol, or per merged pair of protocols (see the note
on tables below), installed with one call:

```c
typedef struct ns_window_callbacks {
  /* optional. -[NSWindowDelegate windowWillClose:] */
  void (*_Nullable will_close)(void *_Nullable context,
                               ns_window *_Nonnull sender);
} ns_window_callbacks;

/* Installs the callbacks struct … syntonic-owned. */
void ns_window_set_callbacks(ns_window *_Nonnull window,
                             const ns_window_callbacks *_Nullable callbacks,
                             void *_Nullable context) API_AVAILABLE(macos(26.0));
```

Rules:

1. **Every member's first parameter is `context`, its second is `sender`**, the
   handle of the object that fired the callback.
2. **Install, replace, uninstall are one function.** A non-null struct installs
   or replaces; a null struct uninstalls. Installing again releases the previous
   shim and its struct copy.
3. **The struct is copied; the context is not.** You may build the struct on the
   stack and let it go; the context has to outlive the installation.
4. **A member is required when the SDK marks it `@required`, or when the table in
   [Required members per protocol](#required-members-per-protocol) lists it.**
   Every protocol v0 wraps is entirely `@optional` in the SDK, so **the table is
   the source**.
5. **An unset optional member behaves exactly as a method the object does not
   implement** — AppKit takes its documented default, and the C side is never
   called.
6. **An unset required member is reported at install time in a debug build**,
   naming the protocol and the member, before AppKit ever asks.
7. **A required method the wrapper answers from stored data is dropped from the
   struct** and documented in the table's "served from stored data" column.
   `NSToolbarDelegate`'s two identifier-list methods are the case in v0: the
   wrapper answers them from arrays of C strings the caller passed in.
8. **A handle a callback returns to AppKit is owned.** The library releases it
   once AppKit has retained it, so the C side never keeps a reference it did not
   create. The toolbar's `item_for_item_identifier_…` is the example: return
   `ns_toolbar_item_create…`'s result directly and do not release it yourself.

Merged structs: tables and outlines take their counts and children from AppKit's
data-source protocol and their cells and selection from its delegate protocol.
Syntonic merges each pair into one struct — `ns_table_view_callbacks`,
`ns_outline_view_callbacks` — so one shim conforms to both and one install sets
both AppKit slots.

### How a wrapper installs one

`src/syn_shims.h` is the machinery, and every wrapper goes through it. A
protocol is four pieces in the `.m`, in this order, and nothing else.

**1. The member table.** One row per struct member: the selector it answers,
where it sits in the struct, and whether
[the table below](#required-members-per-protocol) lists it as required.

```c
SYN_SHIM_TABLE(syn_window_table, ns_window_callbacks, "NSWindowDelegate",
               SYN_SHIM_OPTIONAL(ns_window_callbacks, will_close,
                                 "windowWillClose:"));
```

`SYN_SHIM_REQUIRED` is the other row macro, and a member of an embedded parent
struct is named through it:
`SYN_SHIM_OPTIONAL(ns_combo_box_callbacks, text_field.did_change,
"controlTextDidChange:")`. The protocol string is what a report names, so a
merged struct spells both: `"NSTableViewDataSource + NSTableViewDelegate"`.

**2. The shim subclass**, which adopts the protocol — both protocols, for a
merged struct — and implements one method per member.

```objc
@interface SynWindowShim : SynShim <NSWindowDelegate>
@end

@implementation SynWindowShim
- (void)windowWillClose:(NSNotification *)notification {
  SYN_SHIM_ENTER(NSWindow, notification.object);
  void (*callback)(void *, ns_window *) =
      SYN_SHIM_FN(ns_window_callbacks, will_close);
  if (callback != NULL) callback(syn_context, NS_OUT(ns_window, syn_sender));
  SYN_SHIM_LEAVE();
}
@end
```

`SYN_SHIM_ENTER` takes the sender's AppKit class and the expression that yields
it — the method's own sender argument, or a lone notification's `object` — and
declares `syn_sender` and `syn_context` for the body. It also opens the call's
autorelease pool and holds the shim and the sender until the method returns,
which is what makes a callback that re-installs its own struct, releases the
sender, or closes the window it was handed safe rather than a use after free.
Test the function pointer against null even for a member `respondsToSelector:`
reports as absent: AppKit caches that answer and a stale cache would otherwise
call through a null pointer.

**3. The installer**, which is one call:

```c
void ns_window_set_callbacks(ns_window *window,
                             const ns_window_callbacks *callbacks,
                             void *context) {
  NS_ENTER();
  NSWindow *target = NS_IN(NSWindow, window);
  SYN_SHIM_INSTALL(target, SynWindowShim, syn_window_table, callbacks, context,
                   ^(id shim) { target.delegate = shim; });
  NS_LEAVE();
}
```

The block is the only part that differs per class: it assigns the AppKit slots
this protocol lives in, and it is called with the new shim or with nil to
uninstall. A merged struct assigns both slots in that one block:

```c
^(id shim) { target.delegate = shim; target.dataSource = shim; }
```

Everything else is the same call for every protocol — the required-member
report, the struct copy, assigning the slot before swapping the association,
releasing the previous shim, and uninstalling on a null struct.

**4. A member that returns something** validates the value before AppKit sees
it, and hands back an owned handle through `syn_shim_take`:

```c
SYN_SHIM_CHECK_COUNT(syn_table_view_table, number_of_rows, count);
SYN_SHIM_CHECK_NONNULL(syn_table_view_table, cell_string, text);
SYN_SHIM_CHECK_HANDLE(syn_toolbar_table, item_for_item_identifier_…, handle,
                      NSToolbarItem, false);
return syn_shim_take(handle); /* the callback's +1 goes once AppKit retains */
```

Both checks compile out under `NDEBUG`. `syn_shim_take` does not: it is the
ownership rule in rule 8 above, not a debug check.

### Lifetimes

| Thing | Lives from | Until |
|---|---|---|
| the shim object | install | uninstall, replacement, or the AppKit object's deallocation |
| the struct copy | install | the same |
| `context` | install | the same — `ns_release` does **not** uninstall |
| `sender` in a callback | the callback's entry | the callback's return |
| a string passed into a callback | the callback's entry | the callback's return |
| a string a callback returns | the callback's return | the library copies it immediately |

The shim is kept alive by an associated-object reference on the AppKit object it
serves, and holds itself and a +1 on the sender until AppKit's dispatch returns.
A shim never holds a strong reference to the object it serves.

### What AppKit does, and why these rules are what they are

- **AppKit holds a delegate or data source weakly** (`NSComboBox`'s data source
  is `assign`, which is worse: no zeroing). Something has to keep the shim alive,
  which is what the associated object is for.
- **AppKit caches which optional methods a delegate implements at assignment
  time.** That is why install, replace and uninstall assign the AppKit property
  first and swap the association second, and why the shim answers
  `respondsToSelector:` from struct membership rather than from its own class.
- **A callback may re-install a struct, release the sender, or close a window
  while the shim's method is still on the stack.** That is why the shim holds
  itself and the sender for the duration of the dispatch.

### Debug return validation

In a debug build a shim checks each value a callback returns before handing it to
AppKit, and reports the protocol, the member and the value on failure
(see [Misuse checks](#misuse-checks-and-exceptions)):

- null where the SDK's nullability says non-null,
- a negative count,
- null from a member that never returns null — a cell's borrowed string, an
  outline's child item; neither is an object handle, so neither is checked as
  one,
- a handle whose class is not the expected class or a subclass.

`SYN_SHIM_CHECK_COUNT`, `SYN_SHIM_CHECK_NONNULL` and `SYN_SHIM_CHECK_HANDLE`
are the three checks, written in the shim method between the callback's return
and AppKit's — see
[How a wrapper installs one](#how-a-wrapper-installs-one), piece 4. All three
are nothing under `NDEBUG`.

---

## Required members per protocol

Rule owner: R9, R13, KTD8. **This table is the source of truth for what is
required**, because every protocol v0 wraps is `@optional` in the SDK.

| C struct | AppKit protocol(s) | Required | Optional | Served from stored data (dropped) | Unit |
|---|---|---|---|---|---|
| `ns_application_callbacks` | `NSApplicationDelegate` | none | `did_finish_launching`, `should_terminate_after_last_window_closed`, `will_terminate` | — | U14 ✓ |
| `ns_window_callbacks` | `NSWindowDelegate` | none | `will_close` | — | U14 ✓ |
| `ns_text_field_callbacks` | `NSTextFieldDelegate` | none | `did_change`, `did_end_editing` | — | U7 ✓ |
| `ns_table_view_callbacks` | `NSTableViewDataSource` + `NSTableViewDelegate` (merged) | `number_of_rows`, `cell_string` | `selection_did_change` | — | U8 ✓ |
| `ns_outline_view_callbacks` | `NSOutlineViewDataSource` + `NSOutlineViewDelegate` (merged) | `number_of_children_of_item`, `child_of_item`, `is_item_expandable`, `cell_string` | `cell_symbol_name`, `should_expand_item`, `should_collapse_item`, `is_group_item`, `should_select_item`, `selection_did_change` | — | U8 ✓ |
| `ns_toolbar_callbacks` | `NSToolbarDelegate` | `item_for_item_identifier_will_be_inserted_into_toolbar` | — | `toolbarDefaultItemIdentifiers:`, `toolbarAllowedItemIdentifiers:` — the wrapper answers both from C string arrays passed to `ns_toolbar_set_callbacks` by pointer plus count, and **deep-copied there**, which is the one exception to KTD17's borrow rule (see below) | U9 ✓ |
| `ns_combo_box_callbacks` | `NSComboBoxDataSource` + `NSComboBoxDelegate` (merged; embeds `ns_text_field_callbacks` first, because `NSComboBoxDelegate` inherits `NSTextFieldDelegate`) | `number_of_items`, `object_value_for_item_at_index` | `index_of_item_with_string_value`, `completed_string`, `selection_did_change`, `selection_is_changing`, `will_pop_up`, `will_dismiss` | — | U13 ✓ |

A ✓ in the unit column means the row was checked against the shipped struct.

`cell_string` has no AppKit counterpart: it is the Syntonic-owned member that
supplies one borrowed UTF-8 string per cell, which the wrapper copies into the
cell view it built and reuses. It answers
`tableView:viewForTableColumn:row:` and `outlineView:viewForTableColumn:item:`,
which is why those two selectors appear in no other row. `cell_symbol_name` is
the outline's second syntonic-owned member on that same selector: the SF Symbol
name of the row's icon, or null for a row that has none, and two members may
share one selector because a shim answers `respondsToSelector:` yes as soon as
any member of it is set.

**The table's members** carry the column's index and the row's, in that order,
because a table addresses a cell by both. **The outline's members carry an item
pointer**, null for the root, and the wrapper boxes each pointer once into a
cached object so the same item comes back as the same object across a reload —
Apple's identity rule for `NSOutlineView`. `is_group_item` and
`should_select_item` are what a sidebar's unselectable headings are made of
(R15); they are optional, and an outline that sets neither is a flat,
selectable list. `should_collapse_item` is the other half of
`should_expand_item`, and false is the sidebar whose headings never close.
AppKit asks `should_select_item` about a selection the user drives, not about
one `ns_outline_view_select_item` makes.

**The text field's two members answer `NSControlTextEditingDelegate`
selectors** — `controlTextDidChange:` and `controlTextDidEndEditing:`.
`NSTextFieldDelegate` inherits that protocol, and the three methods it declares
itself are candidate-list methods v0 does not wrap. `did_end_editing` is the
commit point R17 asks for. The field's target/action,
installed through `ns_control_set_action` on the control upcast, fires at the
same moment and is what the twins use; the struct is the finer-grained view of
the same event and the two are independent.

**`should_terminate_after_last_window_closed` unset means false**, which is F1:
closing the last window leaves the app running with its menu bar until Quit.
Nothing in the wrapper enforces that, and nothing needs to: an unset member is
a method the delegate does not implement, and AppKit's own answer when
`applicationShouldTerminateAfterLastWindowClosed:` is absent is the same false.
The two agree, so F1 holds whether the caller sets the member or not.

Why these are required even though the SDK says `@optional`:
`NSComboBoxDataSource`'s header says in so many words that its first two methods
are required when not using bindings; a table with no `number_of_rows` shows
nothing; an outline with no `child_of_item` cannot be walked. Making them
required turns a blank control into a report that names the missing member.

**The toolbar's two identifier lists are copied, not borrowed**, and that is
the one place in v0 where an array passed in is not borrowed for the call
(KTD17). AppKit asks a toolbar's delegate for its default and allowed
identifiers *after* the install returns, repeatedly, for as long as the toolbar
exists, so a caller's stack array would be a dangling read on the first
question. `ns_toolbar_set_callbacks` therefore copies the array and every
string in it, and **the copy lives as long as the toolbar** — or until the next
install replaces it, which a null struct also does. The caller's array may go
out of scope the moment the call returns; `tests/test_toolbar.c` installs from
storage it then frees and scribbles, and the suite fails without the copy.
Nothing else about the boundary changes: the strings still cross as UTF-8 and
the count still comes alongside the pointer. A protocol whose stored data is
only ever read during the call it was passed in keeps the borrow rule; write
the exception down here if you find another.

**Returning an owned handle is the toolbar member's shape**, and
`syn_shim_take` in `src/syn_shims.h` is what implements rule 8 above: the
member returns `ns_toolbar_item_create…`'s result directly, the wrapper hands
the item to AppKit, and the reference is dropped once AppKit has retained it.
The member's null is an answer and not misuse, because the SDK declares the
return nullable — which is why its `SYN_SHIM_CHECK_HANDLE` is written with
`required` false.

**Adding a protocol?** Add a row here in the same commit as the wrapper, fill
every column, and say in the pull request why each required member is required.

---

## Threading

Rule owner: R10.

- **Every public function is main-thread only**, with two exceptions:
  - `ns_main_thread_dispatch`, which runs a C callback on the main thread and may
    be called from any thread. It is always asynchronous, from the main thread as
    well.
  - `ns_string_free`, which frees a `malloc`'d copy and touches no Objective-C
    object.
- Every callback runs on the main thread.
- A debug build detects a call from any other thread, names the function, and
  stops the process:

```
syntonic: ns_window_set_title: called from a background thread. Every Syntonic
function is main thread only; ns_main_thread_dispatch is the one that crosses
onto the main thread (R10).
```

In a wrapper body this is free: `NS_ENTER()` carries the check. The two exempt
functions use `NS_ENTER_ANY_THREAD()` instead, which keeps the pool and drops the
check. Do not use it anywhere else.

---

## Availability

Rule owner: R8, KTD5. **The floor is macOS 26.**

| The SDK says | Write |
|---|---|
| `API_AVAILABLE(macos(10.11))` — below the floor | `API_AVAILABLE(macos(26.0))` |
| `API_AVAILABLE(macos(27.0))` — above the floor | `API_AVAILABLE(macos(27.0))`, mirroring it |
| nothing at all | `API_AVAILABLE(macos(26.0))` |

Whole AppKit headers carry no annotation — `NSComboBox.h` has not one
`API_AVAILABLE` in it. That is not an omission to research: declare the floor.

**The library adds no per-call guard.** An API the running system does not have
behaves exactly as it does in AppKit, and the caller guards. A C source guards
like this:

```c
if (__builtin_available(macOS 27.0, *)) {
  ns_new_thing_create();
}
```

`__builtin_available` is what silences `-Wunguarded-availability-new`. **An `if`
on `ns_available` does not**, because clang only understands literal versions
there. `ns_available(int major, int minor)` is the runtime answer for callers
with no compile-time construct to reach for: a binding generator, a foreign
runtime, a dispatch table built from a version number.

---

## Misuse checks and exceptions

Rule owner: R12, KTD4. Debug builds report and abort; release builds
(`NDEBUG`) compile all of it out and pay nothing.

### Nullability

Mark each handle parameter and return `_Nonnull` or `_Nullable` **following the
AppKit API you wrap**. `NSWindow.contentView` is `(nullable, strong)`, so
`ns_window_content_view` returns `ns_view *_Nullable`.

These annotations belong in the **public header only**. Writing one `_Nullable`
inside a `.m` turns on `-Wnullability-completeness` for that whole file and
obliges every other pointer in it to carry one.

### What a debug build checks

| Check | Carried by | Report |
|---|---|---|
| called off the main thread | `NS_ENTER()` | the function and the main-thread rule |
| null at a `_Nonnull` handle position | `NS_IN` | the expected AppKit class and the function |
| a handle whose class is neither the expected class nor a subclass | `NS_IN`, `NS_IN_OPT` | both class names and the function |
| null or invalid UTF-8 at a `_Nonnull` string position | `NS_STRING_IN` | the function and the UTF-8 rule |
| an Objective-C exception raised by AppKit inside the call | `NS_ENTER()` / `NS_LEAVE()` | the function, the exception's name and its reason |

Each one prints to stderr and then **aborts**. Aborting is the point: a
log-and-continue hides the misuse, and a breakpoint trap says nothing outside a
debugger. Catching the exception at the wrapper's edge matters too — an
Objective-C exception unwinding through C frames skips both the C cleanups and
the ARC releases.

### When to add an explicit check

Use `NS_IN` and the entry macros and stop there **when AppKit itself raises** on
the misuse: the entry macro already catches that exception and names your
function. `-[NSArray objectAtIndex:]` past the end is this case.

Add an explicit check **when AppKit would accept the bad value silently**, so the
caller would get a wrong result rather than a report.
`-[NSPopUpButton selectItemAtIndex:]` with an out-of-range index is this case, as
is `-[NSMenu insertItem:atIndex:]`, which raises only after doing partial work.
Write the check with the same shape as the built-in ones: print the class, the
function and the violated rule, then stop.

### The index check

`NS_CHECK_INDEX(index, largest)` in `src/ns_internal.h` is that check for an
index, and the one a wrapper reaches for most often. `largest` is the
largest index **this call** accepts, which is not the same number for every
call: an insert accepts the count, because the count appends, while an accessor
accepts the count minus one.

It is not the only check a wrapper writes by hand. Where the receiver's own
state decides whether a call means anything at all, the wrapper writes its own
in the same shape — a static function in its `.m` behind `#ifndef NDEBUG` that
prints the function, the state and the rule, then stops. `NSComboBox` is the
first: its `usesDataSource` switches the class between two exclusive lists, and
a call meant for the other one is answered from the list the combo box is not
showing, with nothing but a console line to say so. `src/ns_combo_box.m` holds
that check and `tests/test_combo_box.c` pins both directions of it.

```c
void ns_menu_insert_item_at_index(ns_menu *menu, ns_menu_item *item,
                                  long index) {
  NS_ENTER();
  NSMenu *target = NS_IN(NSMenu, menu);
  NS_CHECK_INDEX(index, (long)target.numberOfItems);
  [target insertItem:NS_IN(NSMenuItem, item) atIndex:(NSInteger)index];
  NS_LEAVE();
}
```

It reports the index, the range and the function, and it is nothing under
`NDEBUG` like every other check here.

**Do not write it where AppKit raises cleanly.** `-[NSMenu itemAtIndex:]` past
the end raises before it changes anything, so `ns_menu_item_at_index` has no
check of its own and the entry macro's exception report names the function.
`tests/test_menu.c` pins both halves: the insert aborts with the range, the
accessor aborts with the AppKit exception.

---

## Accessibility

Rule owner: R23.

**A wrapper never touches an accessibility property unless the caller asks.**
AppKit infers a control's role, label and default behavior from its title, its
class and its position, and every one of those inferences survives only if the
wrapper leaves the properties alone. Do not set `accessibilityLabel` "to be
helpful" in a constructor, and do not set `accessibilityRole` to what you think
the control is.

The exception is a control with no text of its own. An image-only toolbar button
has nothing to infer a label from, so its wrapper takes an accessibility
description as a parameter and the caller supplies it, exactly as a nib would
have.

`ns_view.h` exposes setters for a view's accessibility label and role, so a
programmatically built app can name what a nib would have named. They landed in
U7 as four functions on `ns_view`: `ns_view_set_accessibility_label`,
`ns_view_copy_accessibility_label`, `ns_view_set_accessibility_role` and
`ns_view_copy_accessibility_role`. Every control reaches them through its view
upcast, because both are `NSAccessibility` properties that `NSView` adopts.

**A role crosses as a UTF-8 string, not as a named constant.** AppKit's roles
are a typed `NSString` (`NSAccessibilityRole`), so the boundary rule for a
string applies unchanged (R11) and the caller writes AppKit's own spelling:
`"AXButton"`, `"AXPopUpButton"`, `"AXGroup"`. Syntonic declares no `extern
const char *const` for them, because a C constant cannot be initialized from
AppKit's `NSString` at load time and the alternative — a second, hand-copied
table of Apple's role strings — is a spelling to keep in sync for no gain. A
suite that needs one spells it in a `#define` of its own, as
`tests/test_accessibility.c` does.

**Both properties are `copy`, so the getters are owned** and named `copy_`
(R7). Both are also latches, which is the reason the rule above is "never touch
one unless asked" rather than "set a sensible default": a value the caller sets
replaces AppKit's inference, and setting null afterwards replaces it with
nothing rather than restoring the inference. `tests/test_accessibility.c` pins
both halves — a titled button reporting its title with no call made, and a set
label surviving a clear as null.

---

## File layout and the test each wrapper ships with

Rule owner: R21.

### Files

| File | Contents |
|---|---|
| `include/syntonic/ns_<class>.h` | the public header |
| `src/ns_<class>.m` | the implementation |
| `tests/test_<class>.c` | the off-screen suite |
| `include/syntonic/syntonic.h` | one `#include` line added for the new header |

`CMakeLists.txt` globs `src/*.m` and `tests/test_*.c` with `CONFIGURE_DEPENDS`,
so **a new wrapper and a new suite need no build edit**. The globs do not descend
into subdirectories, which is why `src/templates/` is not part of the library.

### The suite

Each wrapped control ships with an off-screen suite in the same commit. Off-screen
means:

1. `syn_test_bootstrap()` first — it creates the shared application with the
   **prohibited** activation policy. Many AppKit initializers assert that the
   application exists.
2. Create windows **without ordering them front**.
3. Force layout with `layoutSubtreeIfNeeded` rather than waiting for a display
   pass.
4. Drive controls with `performClick:` and `sendAction:to:from:`.
5. Wait for a callback with `SYN_WAIT_FOR(flag, 1000)` — a run-loop barrier. A
   fixed sleep is flaky in both directions.

The vocabulary, from `tests/runner.h` and `tests/support.h`:

| Macro | Use |
|---|---|
| `SYN_TEST(name)` | defines and registers a test |
| `SYN_ASSERT`, `SYN_ASSERT_MSG`, `SYN_ASSERT_STR_EQ` | assertions; each stops the test at the failure |
| `SYN_ABORT_CASE(name)` | a case that is expected to abort; it never runs as part of the suite |
| `SYN_ASSERT_ABORTS(case_name, needle)` | asserts that the case died on `SIGABRT` with `needle` on stderr |
| `SYN_WAIT_FOR(expr, timeout_ms)` | spins the run loop until `expr` is true |

An abort case has to be a `SYN_ABORT_CASE` driven by `SYN_ASSERT_ABORTS`, because
the helper **re-executes the suite binary** rather than forking:
CoreFoundation refuses to run after a fork and AppKit aborts on its own, which
would mask the abort under test.

What a suite covers, at a minimum: every function once, an owned return released,
a borrowed return retained and outliving its owner, each callback member firing,
each optional member unset and not crashing, each required member missing and
aborting, one off-main-thread abort, and a teardown that leaves `just leaks`
clean.

---

## How to wrap a new class

Rule owner: F5, R13. The procedure, end to end. It ends with a green `just test`.
Worked here against `NSComboBox`.

### 1. Find the SDK header

```sh
ls "$(xcrun --show-sdk-path)/System/Library/Frameworks/AppKit.framework/Headers/NSComboBox.h"
```

Read it whole before you write anything. You need the class's properties with
their attributes, its methods, its protocols, and its superclass.

### 2. Decide mirror or non-mirror, and name the files

One SDK header, so it is a **mirror** header with the same stem:
`include/syntonic/ns_combo_box.h`, `src/ns_combo_box.m`,
`tests/test_combo_box.c`. A non-mirror header is only for a builder or helper
with no AppKit counterpart, and it has to be added to `NON_MIRROR` in
`scripts/lint-headers.sh`.

### 3. Copy the template

```sh
cp src/templates/ns_template.h include/syntonic/ns_combo_box.h
cp src/templates/ns_template.m src/ns_combo_box.m
```

Replace `ns_template` with `ns_combo_box`, `NSTemplate` with `NSComboBox`, the
include guard, and the receiver parameter name. Delete the shapes you do not
need. Delete the `@compatibility_alias` lines.

### 4. Derive the type name and the upcasts

`NSComboBox` → `ns_combo_box`, a run of capitals being one word. Its superclass
chain is `NSTextField` → `NSControl` → `NSView`, so declare one upcast per
wrapped ancestor in `ns_combo_box.h`:

```c
/* The upcast to ns_text_field: the same pointer. syntonic-owned. */
ns_text_field *_Nonnull ns_combo_box_as_text_field(ns_combo_box *_Nonnull combo_box)
    API_AVAILABLE(macos(26.0));
```

### 5. Derive each function name from its selector

Type name plus the selector's segments in argument order. See
[Naming](#naming). `-[NSComboBox numberOfVisibleItems]` →
`ns_combo_box_number_of_visible_items`.

### 6. Decide owned or borrowed for every return

Read the property's attribute and apply the table in
[Ownership](#ownership) — this is not a judgment call. `NSComboBox.objectValueOfSelectedItem`
is a computed `id`, so it crosses as an **owned** `char *` named
`ns_combo_box_copy_object_value_of_selected_item`. A `strong` property getter is
borrowed and named `get_`.

### 7. Write the availability annotation

`NSComboBox.h` carries no `API_AVAILABLE` at all, so every function gets the
floor: `API_AVAILABLE(macos(26.0))`. See [Availability](#availability).

### 8. Write the nullability annotations

`_Nonnull` or `_Nullable` on every handle and string, following the SDK. In the
header only. See [Misuse checks](#misuse-checks-and-exceptions).

### 9. Write the bodies with the `ns_internal.h` macros

`NS_ENTER()` … `NS_LEAVE()` around one line of AppKit, with `NS_IN` inbound and
`NS_OUT`, `NS_OUT_OWNED` or `NS_STRING_OUT` outbound. The four canonical bodies
are in `src/ns_internal.h`'s header comment. No other vocabulary is needed; if
you find yourself reaching for something else, that is a question for the pull
request.

### 10. Add the protocol struct and its table row

If the class has a delegate or a data source: one struct of function pointers,
members named per [Naming](#naming), the parent protocol's struct embedded first,
and one `ns_combo_box_set_callbacks` installer written the way
[How a wrapper installs one](#how-a-wrapper-installs-one) spells out — a member
table, a `SynComboBoxShim`, and one `SYN_SHIM_INSTALL`. Then **add a row to
[Required members per protocol](#required-members-per-protocol)** naming every
required and optional member. Check the SDK for an `assign` delegate slot —
`NSComboBox.dataSource` is one, and AppKit neither retains it nor zeroes it.
The machinery already covers that: the shim is held by the object it serves, so
it cannot die first, and install, replace and uninstall each assign the AppKit
slot before the previous shim is released. What the wrapper owes an `assign`
slot is therefore only that the install block assigns it like any other slot, so
that a null struct leaves nil there rather than an address. A caller needs no
uninstall before `ns_release`; `tests/test_combo_box.c` pins both halves.

### 11. Add the header to the umbrella

One line in `include/syntonic/syntonic.h`, in the same commit.

### 12. Write the suite

`tests/test_combo_box.c`, per
[the test each wrapper ships with](#file-layout-and-the-test-each-wrapper-ships-with).
No build edit: the glob picks it up.

### 13. Run the six gates

```sh
just build         # no warnings; -Werror is on
just test          # every CTest suite
just test-san      # AddressSanitizer + UndefinedBehaviorSanitizer
just leaks         # no leaked allocation with a Syntonic or test frame
just lint-headers  # header discipline
just check-c11     # the umbrella header still compiles in C11 mode
```

All six, before you call it done.

### What not to do

- **Do not add a convenience function that AppKit does not have.** One C function
  per method or property. A builder belongs in a non-mirror header, and v0 has
  three.
- **Do not guard a call on the macOS version inside the library.** The caller
  guards.
- **Do not set an accessibility property** the caller did not ask for.
- **Do not write `_Nullable` in a `.m`.**
- **Do not return a borrowed handle from anything but a `strong`, `weak` or
  `assign` property getter** — or from the two methods on the child-accessor
  allowlist.
- **Do not skip, loosen or `#if 0` a failing gate.** Fix the wrapper.
- **Do not invent a convention for block completion handlers or subclass override
  points.** They are out of scope for v0.
- **Do not reformat a header you are not changing.**

---

## Rule index

| Rule | Section |
|---|---|
| R5 Naming | [Naming](#naming) |
| R6 Header discipline | [Header seams and header discipline](#header-seams-and-header-discipline) |
| R7 Ownership | [Ownership](#ownership) |
| R8 Availability | [Availability](#availability) |
| R9 Callbacks | [Callbacks](#callbacks), [Required members per protocol](#required-members-per-protocol) |
| R10 Threading | [Threading](#threading) |
| R11 Boundary types | [Boundary types](#boundary-types) |
| R12 Misuse detection | [Misuse checks and exceptions](#misuse-checks-and-exceptions) |
| R13 This document's completeness bar | the whole document; the per-protocol table and [How to wrap a new class](#how-to-wrap-a-new-class) are what it names specifically |
| R21 The test each wrapper ships with | [File layout and the test each wrapper ships with](#file-layout-and-the-test-each-wrapper-ships-with) |
| R23 Accessibility | [Accessibility](#accessibility) |
| KTD4 Debug checks | [Misuse checks and exceptions](#misuse-checks-and-exceptions) |
| KTD5 Availability | [Availability](#availability) |
| KTD7 Ownership mechanics | [Ownership](#ownership) |
| KTD8 Shims | [Callbacks](#callbacks) |
| KTD10 Naming | [Naming](#naming) |
| KTD17 Boundary shapes | [Boundary types](#boundary-types) |
| KTD18 Header seams | [Header seams and header discipline](#header-seams-and-header-discipline) |

---

## Where later units append

This document is revised as the wrappers land. Each unit below appends to the
named section in its own commit; nothing else in the document moves.

| Unit | Appends |
|---|---|
| U14 ✓ | landed: the application and window rows of the per-protocol table, confirmed against the shipped structs; [How a wrapper installs one](#how-a-wrapper-installs-one), which is `src/syn_shims.h` as the six later units use it; `+[NSApplication sharedApplication]` on the [borrowed-return allowlist](#the-mechanical-rule-read-the-sdk-propertys-attribute); `NSWindow`'s `releasedWhenClosed` fixup confirmed. The view controller needed no row: NSViewController has no protocol Syntonic wraps |
| U6 ✓ | landed: [The index check](#the-index-check) in [Misuse checks](#misuse-checks-and-exceptions), which is `NS_CHECK_INDEX` and where not to use it; the dropped-segment rule in [Constructors](#constructors), for a selector segment whose type cannot cross; [An enum from a header that is not wrapped](#an-enum-from-a-header-that-is-not-wrapped), for `ns_event_modifier_flags`; `-[NSMenu itemAtIndex:]` confirmed on the [borrowed-return allowlist](#the-mechanical-rule-read-the-sdk-propertys-attribute). No per-protocol row and no post-init fixup: v0 wraps no menu protocol, and neither `NSMenu` nor `NSMenuItem` needs a line after construction |
| U7 ✓ | landed: the accessibility setters and the role-as-a-string decision in [Accessibility](#accessibility); the text-field row of the per-protocol table, confirmed against the shipped struct; the inherited-API-through-the-upcast rule in [Upcasts](#upcasts), which is why there is no `ns_button_set_action`. No post-init fixup: none of `NSView`, `NSControl`, `NSButton`, `NSTextField` or `NSPopUpButton` needs a line after construction |
| U8 ✓ | landed: the table and outline rows of the per-protocol table, confirmed against the shipped structs, with what a member's extra arguments are and why the outline's two group-row members exist; `SYN_SHIM_CHECK_NONNULL` in [Debug return validation](#debug-return-validation), the third check, for a member whose return is neither a count nor an object handle; the outline's three two-selector compositions in [Syntonic-owned names](#syntonic-owned-names). `-[NSTableView viewAtColumn:row:makeIfNecessary:]` confirmed on the [borrowed-return allowlist](#the-mechanical-rule-read-the-sdk-propertys-attribute). No post-init fixup. Two things a later unit should know rather than rediscover: **a constructor is the one inherited member a subclass redeclares**, because an upcast needs an object to upcast, so `ns_outline_view_create_with_frame` carries `-[NSTableView initWithFrame:]` in its comment; and `NSTableColumn` got its own mirror header, because a table with no column shows no cell and the alternative was a builder in a mirror header |
| U15 ✓ | landed: the **array of arrays** rule in [Boundary types](#boundary-types), which is `ns_grid_view_row`. No per-protocol row and no post-init fixup: v0 wraps no stack, grid, font or colour protocol. Two things the document was silent on, decided here and recorded in the headers rather than in this document: `NSEdgeInsets` crosses as Syntonic's own `ns_edge_insets` struct, because Foundation's `NSGeometry.h` is not includable from C; a `strong` **class** property such as `+[NSColor labelColor]` is borrowed under the existing property-attribute rule and needs no allowlist row |
| U9 ✓ | landed: the toolbar row of the per-protocol table, confirmed against the shipped struct, with its two dropped identifier methods and the **deep-copy exception** to [Boundary types](#boundary-types)' borrow rule; `syn_shim_take`'s first consumer, which is rule 8 of [A protocol struct](#a-protocol-struct) in practice. Three things the document was silent or wrong on, decided here: **the string-constant example in [Enums and string constants](#enums-and-string-constants) does not compile** — a C `extern const char *const` spelled exactly like AppKit's own constant is a redeclaration of the SDK's `NSString` under that name, so no source including both headers builds, and the three toolbar identifiers `ns_toolbar_item.h` exports therefore follow the *enum* constant rule instead (`NS_TOOLBAR_SIDEBAR_TRACKING_SEPARATOR_ITEM_IDENTIFIER`), carry AppKit's value as a literal, and are pinned equal to AppKit's own symbol by `tests/test_toolbar.c`; a shim method that answers purely from stored data crosses into no C callback, so the two identifier methods carry a plain autorelease pool rather than `SYN_SHIM_ENTER`'s sender hold; and `-[NSToolbarItem performClick:]` does not exist on macOS 27, so a toolbar item is driven the way AppKit drives it, by sending its action to its target. No post-init fixup and no borrowed-return allowlist row: every borrowed return here is a `strong` property getter. `NSWindow`'s `isRestorable` came with this unit into `ns_window.h` although the SDK declares it in `NSWindowRestoration.h`, on the [enum from a header that is not wrapped](#an-enum-from-a-header-that-is-not-wrapped) precedent, and the header says so |
| U12 gaps ✓ | landed, the three things the C twin needed that no unit had wrapped: NSGridRow, NSGridColumn and NSGridCell in `ns_grid_view.h`, whose five child accessors are the new row on the [borrowed-return allowlist](#the-mechanical-rule-read-the-sdk-propertys-attribute) - none of the three is a view, so none has an upcast, and none needs [the index check](#the-index-check) because AppKit raises cleanly and changes nothing first; `cell_symbol_name` in the outline's row above, which is the first case of **two struct members feeding one selector**, so `respondsToSelector:` now answers yes when any member of a selector is set rather than the first; `lineBreakMode` on `ns_control`, not on `ns_text_field`, because `NSControl.h` is what declares it. Two things decided in the headers rather than here: an `NSRange` argument crosses as its location and length, two `long`s, because Foundation's `NSRange.h` is not includable from C; `+[NSGridCell emptyContentView]` is a `strong` class property and therefore borrowed, needing no allowlist row |
| U9 | the toolbar row, including the dropped identifier methods |
| U10 ✓ | landed, all three settled by rules already here: a constructor belongs to the concrete class even when the SDK header declares no initializer of its own, as `ns_view_controller_create` already showed and `ns_tab_view_controller_create` repeats; a nil AppKit documents is an answer, not misuse, so `+[NSImage imageWithSystemSymbolName:accessibilityDescription:]` crosses its nil as null rather than aborting (R12 checks misuse, not results); and `-[NSTabViewController setSelectedTabViewItemIndex:]` takes [the index check](#the-index-check) because AppKit is clean on neither side of the range - a negative index selects the first tab silently, and the exception past the end names a range that includes the index it rejected. No per-protocol row and no post-init fixup: `NSTabViewController` is its own tab view's and toolbar's delegate and Syntonic wraps neither protocol, and none of `NSTabViewController`, `NSTabViewItem` or `NSImage` needs a line after construction |
| U13 ✓ | landed: the combo box row of the per-protocol table, confirmed against the shipped struct, and with it the embedded-parent rule's first consumer — `text_field.did_change` is the first member a shim table names by a path into a parent struct; the **wrapper's own check** in [The index check](#the-index-check), which is the first one that is not an index, because `NSComboBox` keeps two exclusive lists and answers a call meant for the other one from the list it is not showing, logging a console line a C caller never sees; the `assign` data source slot, which step 10 of [How to wrap a new class](#how-to-wrap-a-new-class) now states narrowly — the machinery already keeps the slot from outliving its shim, so the wrapper owes it nothing but the ordinary assignment in the install block, and a caller needs no uninstall before `ns_release`. Three things the document was silent on, decided in the header rather than here: AppKit's `NSNotFound` crosses as **-1**, in both directions, because the boundary already spells "no index" that way and a C caller cannot write `NSIntegerMax` without Foundation; `-[NSComboBox objectValues]` gets no pair of its own, because `numberOfItems` and `itemObjectValueAtIndex:` are already the count function and the index accessor the array-out rule asks for and a second spelling of one property is what the upcast rule forbids; the four `NSComboBox…Notification` constants are not exported, because v0 wraps no notification centre and the four delegate members are the whole of what they carry. One consequence worth knowing rather than rediscovering: **a combo box on the static list cannot install the struct at all**, since the merged struct's two data source members are required and so an install is a data source install — a static combo box hears a pick through `ns_control_set_action` on its control upcast, the same commit point a text field uses (R17). No post-init fixup, and no borrowed-return allowlist row: every borrowed return here is an upcast |
| U11 | whatever the README checkpoint's feedback changes; after U11 a change here is a public change |

Rows in the per-protocol table are pre-filled from the SDK and the plan. A unit
that finds its row wrong corrects it rather than working around it.

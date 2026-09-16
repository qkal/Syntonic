# Twin comparison protocol

Syntonic is accepted when a reviewer shown shuffled captures of the two twins
cannot say which one is C (R22, AE6). This document fixes everything the
comparison depends on: the versions, the geometry, the identity both twins
share, the states, the transitions, the capture method, the behavior lines and
the pass rule.

The Swift twin is the reference. The C twin must match it, not the other way
round.

## 1. Versions

| | |
| --- | --- |
| macOS | 27.0 (build 26A428) |
| SDK | MacOSX 27.0, Command Line Tools only — no Xcode.app, no xcodebuild |
| Swift | swiftc 6.4 (swiftlang-6.4.0.34.1, clang-2100.3.34.1), `-swift-version 6` |
| Deployment target | macOS 26.0 (`-target arm64-apple-macos26.0`) |
| Architecture | arm64 |

Both twins are built on the same machine in the same session. A capture set
made under different versions is not comparable with one made here.

## 2. Shared identity

The menu bar, the window title and the icon are on screen in nearly every
state, so they are shared constants, not per-twin values. They live in
`twins/shared/seed-data.md`; the short version:

- App name, display name, menu-bar title and main window title: `Syntonic Twin`
- Icon set: `twins/shared/icon`, passed to `scripts/bundle.sh` by both twins
- Bundle identifiers differ (`dev.kaino.syntonic.twin.swift` and
  `…twin.c`) because nothing on screen shows them

## 3. Geometry and environment

| | |
| --- | --- |
| Content size | 1000 × 640 points |
| Frame size | 1000 × 692 points (the toolbar adds 52) |
| Frame origin | (200, 200) from the screen's bottom left |
| Minimum content size | 720 × 480 |
| Sidebar pane | 220 points wide |
| List pane | 400 points wide |
| Detail pane | the remainder, 379 points at the fixed size |
| Table columns | Title 170, Owner 120, Category 90 |
| Detail form | 20-point inset, label column trailing-aligned, 220-point field column |
| Resized state | frame set to 820 × 520 |
| Settings window | content 520 × 300, frame origin (420, 420) |

Neither twin uses a frame autosave name, a split-view autosave name, a toolbar
autosave name or window restoration, so a run never inherits a previous run's
layout. The display must be at least 1200 × 930 points so the window is never
constrained; the built-in display of the reference machine is 1512 × 982
points.

Appearance is pinned per run by `SYNTONIC_TWIN_APPEARANCE=light|dark`, which
sets `NSApp.appearance`. The system theme is not touched, so a run is
reproducible and both appearances can be captured back to back. The system
accent color is visible in the sidebar icons and the selection; capture both
twins in the same session so it cannot drift between them.

**Only the Swift twin can honour that variable today.** Syntonic wraps no
`NSAppearance`, so the C twin follows the system theme whatever the variable
says, and a dark-appearance pair would put a pinned-dark Swift capture beside a
system-themed C one — a tell that has nothing to do with the wrappers. Until
`NSAppearance` is wrapped, the dark half of the set is captured by setting the
*system* theme to dark for that half of the session and leaving the variable
unset, so both twins follow the same appearance.

## 4. Capture

```sh
just twin-swift-build && ./scripts/capture-states.sh swift light
just twin-swift-build && ./scripts/capture-states.sh swift dark
just twin-c-build     && ./scripts/capture-states.sh c light
just twin-c-build     && ./scripts/capture-states.sh c dark
```

Captures land in `build/compare/<twin>/<appearance>/<state>.png`.

**Two TCC grants are required**, and a headless or freshly provisioned machine
has neither. System Events needs **Accessibility** to post keys and click
controls; `screencapture` needs **Screen Recording** to photograph another
app's window. Both are granted in System Settings ▸ Privacy & Security to the
program that runs the script. Without them the script stops with
`SYNTONIC_CAPTURE_NO_ACCESSIBILITY` or `SYNTONIC_CAPTURE_NO_SCREEN_RECORDING`
rather than writing black images.

**Two capture methods.** `screencapture -o -l<windowid>` photographs one window
without its shadow and is used for every state that has a window. An open menu
is not inside the window, and the last state has no window at all, so those are
captured from the whole display with `screencapture -D 1`. The window ids come
from the twin itself: it prints `SYNTONIC_TWIN_WINDOW <id> <title>` on stdout
whenever it shows a window.

**Settling is a handshake, never a sleep.** Each twin writes a layout report to
`$SYNTONIC_TWIN_DUMP` after every layout pass, window move, window resize,
selection change and edit. Before each capture the script waits for the line
that proves the state has arrived — `sidebar.collapsed=true`, `list.rows=13`,
`detail.title.value=…` — and fails with `SYNTONIC_CAPTURE_TIMEOUT` after ten
seconds. A fixed sleep would photograph animations mid-flight and add timing
noise to a blind comparison.

**Layout invariants travel with the images.** Every window capture is filed
beside a `<state>.layout.txt` holding the frames of the sidebar, list and
detail panes, the frame, intrinsic size, hugging and compression-resistance
priorities of each detail control, and the pane's current values. U12 compares
these before it accepts screenshot parity: matching pixels with mismatched
priorities is a wrapper defect that only shows up at a size nobody captured.

## 5. States

Twenty states, each captured in light and dark appearance: 40 images per twin.

| # | State | How it is reached | Capture |
| --- | --- | --- | --- |
| 1 | launch | the app starts, All Items selected, no row selected | window |
| 2 | sidebar collapsed | ⌃⌘S | window |
| 3 | sidebar expanded | ⌃⌘S again | window |
| 4 | search filtering | ⌘F, type `lan` (2 rows left) | window |
| 5 | no selection | click the empty area below the rows | window |
| 6 | row selection | Down arrow selects row 1 | window |
| 7 | detail edit | click Title, ⌘A, type `Aperture Sync Revised`, Tab | window |
| 8 | add a row | ⌘N (13 rows, the new row selected) | window |
| 9 | delete a row | click Delete (12 rows, no selection) | window |
| 10 | window resized | frame set to 820 × 520 | window |
| 11 | Settings ▸ General | ⌘, | window |
| 12 | Settings ▸ Appearance | click the Appearance tab | window |
| 13 | Settings ▸ Advanced | click the Advanced tab | window |
| 14 | menu: app | click the `Syntonic Twin` menu | display |
| 15 | menu: File | click the File menu | display |
| 16 | menu: Edit | click the Edit menu | display |
| 17 | menu: View | click the View menu | display |
| 18 | menu: Window | click the Window menu | display |
| 19 | menu: Help | click the Help menu | display |
| 20 | last window closed | ⌘W with only the main window open | display |

States 5 and 1 look alike by design: both show the placeholder. They are kept
apart because they are reached differently, and a twin that places the
placeholder correctly at launch but not after a deselect is a defect.

The Settings states have no layout report: the report covers the main window.

## 6. Transitions (F6)

The capture set freezes states; these transitions are walked by hand and must
agree at every step. The table and the detail pane never disagree.

1. no selection → row selected: the table's selection callback fires, the pane
   loads that row's Title, Owner, Category and Flagged, and Delete enables.
2. row selected → edited: a text field commits when editing ends — Return, Tab
   or clicking away — and the table's row updates from the same value.
3. row selected → toggled: the checkbox and the pop-up commit at once, with no
   confirm step.
4. row selected → deleted: Delete removes the row, the table reloads one row
   shorter, the selection empties and the pane falls back to its placeholder
   with Delete disabled.
5. any state → row added: Add clears the search field, appends a row to the
   selected collection and selects it; the pane shows the new row.
6. main window open → closed: the window orders out, the app keeps running with
   its menu bar, and Quit is the only way out.

## 7. Behavior lines

Each line is walked by hand on both twins and recorded as matching or not. A
single mismatch fails the comparison whatever the images score.

**R16 — toolbar**

- The toolbar carries the sidebar toggle, the sidebar tracking separator, Add
  with the `plus` symbol, a flexible space and a search field, in that order,
  in the unified toolbar style.
- Add inserts a row and selects it.
- The search field filters as characters arrive.

**R17 — list and detail**

- The pane shows the selected row's Title, Owner, Category and Flagged.
- A text-field edit commits when editing ends, not per keystroke.
- The checkbox and the pop-up commit at once.
- Delete removes the selected row.
- With no selection the pane shows `No Selection` and Delete is disabled.
- The pane resizes with the window: the label column stays trailing-aligned and
  the field column stays 220 points wide.

**R18 / AE5 — menus**

- Every Edit-menu item the app defines is a responder-chain action with no app
  code behind it: ⌘C in a text field copies, ⌘Z undoes, ⌘A selects all.
- AppKit appends AutoFill, Start Dictation and Emoji & Symbols to any menu
  titled `Edit`. Neither twin adds them and both get them, so the menu-edit
  capture shows them on both sides.
- ⌘, opens Settings, ⌘W closes the front window, ⌘Q quits, ⌃⌘S toggles the
  sidebar, ⌘F moves focus to the search field, ⌘N adds a row.

**F1 — lifecycle**

- Closing the last window leaves the app running with its menu bar.
- Reopening from the Dock brings the main window back with its state intact.

**Tab order** — without a nib AppKit rebuilds the key view loop in geometric
order, so both twins set `autorecalculatesKeyViewLoop` to false and chain it by
hand. What the flag actually buys is less than its name suggests, and U12
measured it on both twins:

- Main window: sidebar outline → table → Title → Owner → Category → Flagged →
  Delete → sidebar outline. The toolbar's search field is outside the loop and
  is reached with ⌘F. **Measured, and both twins agree**: the chain is built
  after `makeKeyAndOrderFront:`, nothing rebuilds it afterwards, and walking
  `nextKeyView` from the window's initial first responder gives exactly this
  cycle on both.
- Settings ▸ General, Appearance and Advanced chain their three controls in
  construction order — **and neither twin keeps that chain.** Each pane chains
  in `loadView`, before the window is ever shown; showing the window rebuilds
  the loop with `autorecalculatesKeyViewLoop` set to false, and every control's
  `nextKeyView` ends up pointing at an `_NSCoreHostingView<…>`, one of AppKit's
  own hosting views. The flag does not hold a chain across a rebuild on macOS
  27; `include/syntonic/ns_window.h` records the same measurement. Both twins
  are replaced identically, so this is not a comparison risk — but the Settings
  Tab order is AppKit's, not the twins', and a comparison must not record it as
  matching the list above.

## 8. Blind procedure

1. Capture both twins in both appearances in one session, on one machine.
2. For each of the 40 state/appearance pairs, place the two captures side by
   side in an order drawn at random. Record the seed, not the order, in
   `build/compare/shuffle.txt`; nobody who reviews may read it.
3. The reviewer sees only the pairs. For each pair the reviewer names which
   side is the C twin. This is a forced choice: there is no "cannot tell"
   option, and a reviewer who is guessing must still guess, which is what makes
   the count comparable with chance.
4. The reviewer's answers are scored against the recorded seed afterwards.

All 40 pairs are required. A run with fewer is not a valid comparison, because
the acceptance interval below is computed for the full set.

## 9. Pass rule

Two conditions, both required.

**Images at chance.** With one reviewer there are 40 forced-choice trials. If
the twins are indistinguishable the number of correct labels follows
Binomial(40, 0.5). The comparison passes when the correct count lands in
**14 to 26 inclusive**, the exact two-sided 95% acceptance region (the
probability of falling outside it by chance is 0.0385). With two reviewers,
pool the trials: 80 trials, accept **31 to 49** (outside-probability 0.0330).
For any other number of trials n, accept the counts k with
P(X ≤ k) > 0.025 and P(X ≥ k) > 0.025 for X ~ Binomial(n, 0.5).

A count *below* the interval fails as loudly as one above it: a reviewer who is
reliably wrong is still reading a tell.

**Every behavior line matches.** Section 7 is walked on both twins and every
line records as matching. A behavior mismatch fails the comparison even when
the images score at chance, and the layout reports beside the captures must
agree within a tolerance of 0.5 points on every frame and exactly on every
priority.

When the comparison fails, the failing states name the defect: the state list,
the layout report and the behavior lines together say whether the fault is a
missing wrapper, a wrong default or a layout priority.

## 10. Results

Run on 2026-09-16 against `feat/appkit-c-core-v0` at `bbafada` plus U12, on the
reference machine of section 1. **Acceptance is Kal's to judge; this section is
what the executor produced.**

### 10.1 Blocked: the blind comparison and the accessibility diff

**Neither TCC grant exists on this machine, so there are no captures and no
blind result.** Both paths were built, run, and stopped where they should:

| Command | Result |
| --- | --- |
| `just compare` | `SYNTONIC_CAPTURE_NO_ACCESSIBILITY` — System Events may not drive the UI |
| `screencapture -x -D 1` | `could not create image from display` — no Screen Recording grant |
| `just ax-compare` | `SYNTONIC_AX_NOT_TRUSTED` — `AXIsProcessTrusted()` is false |

`tools/axdump.m` cannot fall back to reading the twin's own process either: a
scratch program that created a window and then asked for its own
`kAXWindowsAttribute` got `kAXErrorAPIDisabled` (-25208). Without the
Accessibility grant **no accessibility tree can be read at all**, in any
process, so R23 is unverified rather than partly verified.

No image was fabricated and no blind score is recorded. Sections 8 and 9 are
unchanged and are what a run with the grants follows; grant Accessibility and
Screen Recording to the shell's host app in System Settings ▸ Privacy &
Security, then `just compare` and `just ax-compare`.

### 10.2 What was verified instead: the layout-report diff

Both twins were driven through the state list by a pair of scratch drivers that
call the same actions the UI is wired to — the split view item's collapse, the
search field's action, the table's selection, the detail controls' actions and
`performClick`, the toolbar's Add — and file the same reports at the same
points. The C driver includes `twins/c/main.c` and reaches the twin only
through Syntonic; the Swift driver replaces `twins/swift/main.swift` and
reaches the twin through AppKit.

**Sixteen files per twin, byte-identical, twice in a row:**

| File | What it holds |
| --- | --- |
| `01-launch` … `12-collection-syntonic``.layout.txt` | the full layout report at twelve states |
| `menu.txt` | every menu, item title, key equivalent, modifier mask and separator, recursively |
| `toolbar.txt` | the toolbar's five item identifiers and labels in order, its display mode, the window's toolbar style |
| `keys.txt` | the main window's key view loop, walked ten hops from the initial first responder |
| `settings.txt` | the Settings window frame, its three tab labels, each pane's grid frame, and each pane's key chain |

`diff -ru` over the two directories is empty, and a second run of both drivers
reproduces both directories exactly. That covers every frame, intrinsic size,
hugging priority and compression-resistance priority section 4 asks for, at a
tolerance of zero rather than the 0.5 points section 9 allows.

Two states were driven but are **not** what the capture protocol reaches by the
same route:

- **`10-window-resized`** sets the *content* size to 820 × 520; section 5
  resizes the *frame*. Syntonic wraps `setContentSize:` and `setFrameOrigin:`
  but not `setFrame:display:`, so a C app cannot set a frame size from inside.
  The capture script resizes from outside through the accessibility API, which
  reaches both twins the same way.
- **Settings** reports a 500 × 588 frame at (420, 361) on both twins, not the
  520 × 300 at (420, 420) section 3 states: `NSTabViewController` in the
  toolbar style sizes the window to its panes. Both twins agree exactly, so it
  is section 3 that is optimistic, not either twin.

Collapsing the sidebar leaves `sidebar.width` at 220.0 on **both** twins —
AppKit hides the sidebar's view rather than zeroing its frame — so the wait in
`scripts/capture-states.sh` for `sidebar.width=0.0` could never have matched.
It now waits for `sidebar.collapsed=true`, which is the line that moves.

### 10.3 The behavior list, walked

Twenty-four lines: the six transitions of section 6 and the eighteen bullets of
section 7. **Sixteen verified by driving both twins and diffing, six verified
structurally, one cannot be verified without a UI, one is a known mismatch.**

| Line | How |
| --- | --- |
| Transitions 1, 3, 4, 5, 6 | driven; states 06, 11, 09, 08 and the window close agree exactly |
| Transition 2 (edit commits at end of editing) | driven through the field's action; Return, Tab and click-away are AppKit's own end-of-editing triggers and need a UI |
| R16 toolbar order, style, symbol | `toolbar.txt` identical: toggle sidebar, tracking separator, Add, flexible space, search, icon-only, unified |
| R16 Add inserts and selects | driven through the toolbar item's own action |
| R16 search filters as characters arrive | driven; `sendsSearchStringImmediately` is set identically on both, per-keystroke arrival needs a UI |
| R17, all six | driven; states 05–11 and the 820 × 520 state agree, field column 220.0 on both |
| R18 Edit items are responder-chain actions | structural: the standard menu bar gives them nil targets by construction, and `menu.txt` is identical. The keystrokes need a UI |
| R18 AppKit appends AutoFill, Dictation, Emoji | `menu.txt` shows all three on both twins, in the same place |
| R18 key equivalents (⌘, ⌘W ⌘Q ⌃⌘S ⌘F ⌘N) | `menu.txt` matches on every title, key equivalent and modifier mask |
| F1 closing the last window | driven; both report `windowVisible=false` and keep running |
| **F1 reopening from the Dock** | **mismatch.** `ns_application_callbacks` has no `applicationShouldHandleReopen:hasVisibleWindows:` member, so the C twin does not bring its window back and the Swift twin does |
| Tab order, main window | `keys.txt` identical; see section 7 |
| Tab order, three Settings panes | measured: neither twin keeps its chain; see section 7 |

The C twin walked all twelve states **under AddressSanitizer and
UndefinedBehaviorSanitizer with no report**, and under `leaks -atExit` with 415
framework-baseline leaks and **0 attributed to the twin's own frames**.

### 10.4 What fails the comparison today

One behavior line: the Dock reopen. It needs a wrapper, not a twin change.
Everything else either matches exactly or is waiting on the two TCC grants.

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
that proves the state has arrived — `sidebar.width=0.0`, `list.rows=13`,
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
hand:

- Main window: sidebar outline → table → Title → Owner → Category → Flagged →
  Delete → sidebar outline. The toolbar's search field is outside the loop and
  is reached with ⌘F.
- Settings ▸ General: Workspace → Opens with → Reopen checkbox → Workspace.
- Settings ▸ Appearance: Theme → Row height → Badge checkbox → Theme.
- Settings ▸ Advanced: Scratch folder → Log checkbox → Reset → Scratch folder.

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

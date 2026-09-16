# Checkpoint log

The README is Syntonic's first demand test (U11): it goes public before the twin
app is built, carrying the problem framing, the conventions and the headers of
the first wrapped classes. This file records when that happened and what came
back.

It is a log, not a report. Append; do not rewrite. It is input for the sugar and
generator plans — nothing downstream waits on it.

## Publication

| | |
|---|---|
| Date | _not published yet_ |
| Commit | _fill in at push_ |
| Repository | _fill in at push_ |
| Announced at | _none, or the threads the link was posted to_ |

Before pushing, replace `<owner>` in the README's `git clone` line with the real
repository owner.

## Responses

Newest last. One entry per response, in this shape:

```
### YYYY-MM-DD — issue #12 — wants NSStatusItem for a menu bar app
- Who: (handle, and the language they came from if they said)
- What they want: the class, the app, or the binding they named
- Friction they hit: what in "What it costs you today" they ran into, if any
- Answered: yes/no, and what was promised
```

Signals worth an entry: issues, discussions, someone starting a binding from
another language, a question about a class that is not wrapped yet, and someone
reporting the build recipe failing on their machine. Build failures matter most
— they mean the checkpoint measured the pitch instead of the product.

_(no entries yet)_

---

# Independent-developer extension exercise (U12)

Appended by U12. Separate from the README checkpoint above: that one measures
the pitch, this one measures whether the headers and `docs/conventions.md` are
enough to write against without asking anyone.

## The exercise

**Status: set up, not yet run. It is waiting on an actual independent author.**

The person who runs it must not have written any of Syntonic and must not read
this file's answers, `twins/c/main.c`, or the plan. They get the repository,
`README.md` and `docs/conventions.md`, and nothing else — no walkthrough, no
pairing, no questions answered ahead of time.

**Task.** Starting from the C twin at the commit this exercise is run against:

1. Add one control to the detail pane: a **Notes** text field, below Category,
   editable, labelled `Notes:`, committing the way Title and Owner do, disabled
   when nothing is selected, and carried in the row's model.
2. Add one menu item: **View ▸ Flag Selected**, ⌘L, which toggles the selected
   row's Flagged checkbox and leaves the table and the pane agreeing.

Both must build with `just build`, pass `just lint-twin-c`, and leave
`just leaks` clean.

## The criterion

**Zero questions.** Every question the author asks — of a person, of an issue,
of a previous author, of a model — is logged below with what they were trying to
find out and where they looked first. A question that had to be asked is a hole
in the README or in `docs/conventions.md`, and the fix goes there, not into an
answer.

Time is recorded but is not the criterion.

## The log

| # | Question | What they were trying to do | Where they looked first | Where the answer should have been |
|---|---|---|---|---|
| | | | | |

_(no entries yet — the exercise has not been run)_

## What U12 already knows the author will hit

Written down before the run so it cannot be rationalised afterwards. If the
author asks about any of these, the README's sugar backlog is where the fix
goes.

1. **Adding a row to the detail grid** is four calls, not one, and the twin's
   two helpers are the only example of the shape.
2. **Reaching a standard menu** means `ns_menu_item_at_index` on the menu bar
   then `ns_menu_item_submenu`; `ns_menu_bar.h` says so, `README.md` does not.
3. **Inserting into the View menu** has the trap the Edit menu has: AppKit
   appends its own items to some standard menus as soon as the bar is
   installed, so an appended item can land below them.
4. **A menu item's action is `ns_menu_item_set_action`**, not a selector, and
   the context pointer's lifetime is the author's.
5. **The commit path** for a text field is `ns_control_set_action` on the
   field's control upcast, not `ns_text_field_set_callbacks` — both exist and
   only one is what Title and Owner use.

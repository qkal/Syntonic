# Twin seed data

The twin app's data is a fixed in-memory seed list; nothing persists across a
relaunch (KTD14). Both twins repeat these values verbatim — the Swift twin in
`twins/swift/SeedData.swift`, the C twin in its own seed source. A blind
comparison is only meaningful while the two lists are identical, so change this
file first and both twins after it.

## Identity

| Thing | Value |
| --- | --- |
| App name, display name and menu-bar title | `Syntonic Twin` |
| Main window title | `Syntonic Twin` |
| Settings window title | the selected tab's label |
| Icon set | `twins/shared/icon` |
| Bundle id, Swift twin | `dev.kaino.syntonic.twin.swift` |
| Bundle id, C twin | `dev.kaino.syntonic.twin.c` |

The bundle identifiers differ because nothing on screen shows them. Everything
a reviewer can see is shared.

## Sidebar

Three groups. A group is an unselectable source-list header, not a row; only
the leaves select.

| Group | Leaves (symbol) |
| --- | --- |
| Library | All Items (`tray.full`), Inbox (`tray`), Favorites (`star`) |
| Projects | Syntonic (`hammer`), Twin App (`square.on.square`), Toolchain (`wrench.and.screwdriver`) |
| Archive | 2024 (`calendar`), 2025 (`calendar`) |

All three groups start expanded and cannot be collapsed. `All Items` is
selected at launch and shows every row; every other leaf shows the rows whose
collection is that leaf.

## Rows

Twelve rows, in this order. The table shows Title, Owner and Category; Flagged
and Collection show only in the detail pane and the sidebar filter.

| # | Title | Owner | Category | Flagged | Collection |
| --- | --- | --- | --- | --- | --- |
| 1 | Aperture Sync | Dana Wu | Engineering | no | Inbox |
| 2 | Brass Lantern | Rafi Okoye | Design | yes | Inbox |
| 3 | Cedar Rollout | Mira Halvorsen | Research | no | Favorites |
| 4 | Driftwood Notes | Tomas Brandt | General | no | Favorites |
| 5 | Ember Protocol | Yuki Saito | Engineering | yes | Syntonic |
| 6 | Foxglove Audit | Priya Nandi | Research | no | Syntonic |
| 7 | Granite Handoff | Luis Ferrer | Design | no | Twin App |
| 8 | Harbor Checklist | Anja Vogel | General | yes | Twin App |
| 9 | Ivory Baseline | Sam Oduya | Engineering | no | Toolchain |
| 10 | Juniper Rewrite | Elena Marchetti | Design | no | Toolchain |
| 11 | Kettle Report | Noor Haddad | Research | yes | 2024 |
| 12 | Lantern Archive | Gus Lindqvist | General | no | 2025 |

The Category pop-up offers, in this order: General, Design, Engineering,
Research.

## Added rows

Add appends `New Item` / `Unassigned` / `General`, not flagged, in the selected
collection — or in `Inbox` when the selection is `All Items`. Add clears the
search field first, so the new row is always visible and selectable.

## Search

The search field filters the rows of the selected collection by a
case-insensitive substring of Title or Owner. The capture protocol searches
`lan`, which leaves `Brass Lantern` and `Lantern Archive`.

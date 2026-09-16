/*
 * The C twin's seed data: twins/shared/seed-data.md, verbatim, and the mirror
 * of twins/swift/SeedData.swift (KTD14).
 *
 * The data is a fixed in-memory seed list; nothing persists across a relaunch.
 * twins/shared/seed-data.md is the source of these values and the Swift twin
 * repeats them too, so change that file first and both twins after it.
 */

#ifndef SYNTONIC_TWIN_SEED_DATA_H
#define SYNTONIC_TWIN_SEED_DATA_H

#include <stdbool.h>

/* Title and Owner are edited in the detail pane, so a row carries its own
 * storage for them; Category and Collection are always one of the fixed
 * strings below and are held by pointer. */
enum { TWIN_TEXT_MAX = 64 };

typedef struct twin_item {
  char title[TWIN_TEXT_MAX];
  char owner[TWIN_TEXT_MAX];
  const char *category;
  bool flagged;
  const char *collection;
} twin_item;

/* The outline view addresses its rows by the pointers the data source hands
 * back, so the node tree is static: it is built once, never moves, and the
 * same child always comes back as the same pointer. */
typedef struct twin_sidebar_node {
  const char *title;
  const char *symbol;
  const struct twin_sidebar_node *children;
  long child_count;
} twin_sidebar_node;

#define TWIN_ALL_ITEMS "All Items"

/* The collection a row added while "All Items" is selected lands in. */
#define TWIN_DEFAULT_COLLECTION "Inbox"
#define TWIN_NEW_ITEM_TITLE "New Item"
#define TWIN_NEW_ITEM_OWNER "Unassigned"
#define TWIN_NEW_ITEM_CATEGORY "General"

static const twin_sidebar_node TWIN_LIBRARY_CHILDREN[] = {
    {TWIN_ALL_ITEMS, "tray.full", NULL, 0},
    {"Inbox", "tray", NULL, 0},
    {"Favorites", "star", NULL, 0},
};

static const twin_sidebar_node TWIN_PROJECTS_CHILDREN[] = {
    {"Syntonic", "hammer", NULL, 0},
    {"Twin App", "square.on.square", NULL, 0},
    {"Toolchain", "wrench.and.screwdriver", NULL, 0},
};

static const twin_sidebar_node TWIN_ARCHIVE_CHILDREN[] = {
    {"2024", "calendar", NULL, 0},
    {"2025", "calendar", NULL, 0},
};

static const twin_sidebar_node TWIN_GROUPS[] = {
    {"Library", "folder", TWIN_LIBRARY_CHILDREN, 3},
    {"Projects", "folder", TWIN_PROJECTS_CHILDREN, 3},
    {"Archive", "folder", TWIN_ARCHIVE_CHILDREN, 2},
};

static const long TWIN_GROUP_COUNT = 3;

static const char *const TWIN_CATEGORIES[] = {"General", "Design",
                                              "Engineering", "Research"};
static const long TWIN_CATEGORY_COUNT = 4;

/* Twelve rows, in this order. The table shows Title, Owner and Category;
 * Flagged and Collection show only in the detail pane and the sidebar
 * filter. */
static const twin_item TWIN_SEED_ITEMS[] = {
    {"Aperture Sync", "Dana Wu", "Engineering", false, "Inbox"},
    {"Brass Lantern", "Rafi Okoye", "Design", true, "Inbox"},
    {"Cedar Rollout", "Mira Halvorsen", "Research", false, "Favorites"},
    {"Driftwood Notes", "Tomas Brandt", "General", false, "Favorites"},
    {"Ember Protocol", "Yuki Saito", "Engineering", true, "Syntonic"},
    {"Foxglove Audit", "Priya Nandi", "Research", false, "Syntonic"},
    {"Granite Handoff", "Luis Ferrer", "Design", false, "Twin App"},
    {"Harbor Checklist", "Anja Vogel", "General", true, "Twin App"},
    {"Ivory Baseline", "Sam Oduya", "Engineering", false, "Toolchain"},
    {"Juniper Rewrite", "Elena Marchetti", "Design", false, "Toolchain"},
    {"Kettle Report", "Noor Haddad", "Research", true, "2024"},
    {"Lantern Archive", "Gus Lindqvist", "General", false, "2025"},
};

static const long TWIN_SEED_ITEM_COUNT = 12;

#endif /* SYNTONIC_TWIN_SEED_DATA_H */

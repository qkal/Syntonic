import Foundation

// The twin's data is a fixed in-memory seed list; nothing persists across a
// relaunch (KTD14). twins/shared/seed-data.md is the source of these values and
// the C twin repeats them verbatim.

final class Item {
    var title: String
    var owner: String
    var category: String
    var flagged: Bool
    var collection: String

    init(title: String, owner: String, category: String, flagged: Bool, collection: String) {
        self.title = title
        self.owner = owner
        self.category = category
        self.flagged = flagged
        self.collection = collection
    }
}

struct SidebarGroup {
    let title: String
    let children: [String]
    let symbols: [String]
}

enum SeedData {
    static let allItems = "All Items"

    static let groups = [
        SidebarGroup(title: "Library",
                     children: [allItems, "Inbox", "Favorites"],
                     symbols: ["tray.full", "tray", "star"]),
        SidebarGroup(title: "Projects",
                     children: ["Syntonic", "Twin App", "Toolchain"],
                     symbols: ["hammer", "square.on.square", "wrench.and.screwdriver"]),
        SidebarGroup(title: "Archive",
                     children: ["2024", "2025"],
                     symbols: ["calendar", "calendar"]),
    ]

    static let categories = ["General", "Design", "Engineering", "Research"]

    // The collection a row added while "All Items" is selected lands in.
    static let defaultCollection = "Inbox"
    static let newItemTitle = "New Item"
    static let newItemOwner = "Unassigned"
    static let newItemCategory = "General"

    static func items() -> [Item] {
        [
            Item(title: "Aperture Sync", owner: "Dana Wu", category: "Engineering", flagged: false, collection: "Inbox"),
            Item(title: "Brass Lantern", owner: "Rafi Okoye", category: "Design", flagged: true, collection: "Inbox"),
            Item(title: "Cedar Rollout", owner: "Mira Halvorsen", category: "Research", flagged: false, collection: "Favorites"),
            Item(title: "Driftwood Notes", owner: "Tomas Brandt", category: "General", flagged: false, collection: "Favorites"),
            Item(title: "Ember Protocol", owner: "Yuki Saito", category: "Engineering", flagged: true, collection: "Syntonic"),
            Item(title: "Foxglove Audit", owner: "Priya Nandi", category: "Research", flagged: false, collection: "Syntonic"),
            Item(title: "Granite Handoff", owner: "Luis Ferrer", category: "Design", flagged: false, collection: "Twin App"),
            Item(title: "Harbor Checklist", owner: "Anja Vogel", category: "General", flagged: true, collection: "Twin App"),
            Item(title: "Ivory Baseline", owner: "Sam Oduya", category: "Engineering", flagged: false, collection: "Toolchain"),
            Item(title: "Juniper Rewrite", owner: "Elena Marchetti", category: "Design", flagged: false, collection: "Toolchain"),
            Item(title: "Kettle Report", owner: "Noor Haddad", category: "Research", flagged: true, collection: "2024"),
            Item(title: "Lantern Archive", owner: "Gus Lindqvist", category: "General", flagged: false, collection: "2025"),
        ]
    }
}

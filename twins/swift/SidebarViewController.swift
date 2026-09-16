import AppKit

// The outline view addresses its rows by object identity, so the node tree is
// built once and handed back unchanged.
final class SidebarNode: NSObject {
    let title: String
    let symbol: String
    let children: [SidebarNode]

    init(title: String, symbol: String, children: [SidebarNode] = []) {
        self.title = title
        self.symbol = symbol
        self.children = children
    }

    var isGroup: Bool { !children.isEmpty }
}

// A source-list outline view inside the split view's sidebar item. The sidebar
// item supplies the material, so there is no NSVisualEffectView here (R15).
final class SidebarViewController: NSViewController, NSOutlineViewDataSource, NSOutlineViewDelegate {
    var onSelectCollection: ((String) -> Void)?

    private let outlineView = NSOutlineView()
    private let scrollView = NSScrollView()
    var keyViewChain: [NSView] { [outlineView] }

    private let nodes: [SidebarNode] = SeedData.groups.map { group in
        SidebarNode(title: group.title,
                    symbol: "folder",
                    children: zip(group.children, group.symbols).map { SidebarNode(title: $0, symbol: $1) })
    }

    override func loadView() {
        view = NSView(frame: NSRect(x: 0, y: 0,
                                    width: TwinGeometry.sidebarWidth,
                                    height: TwinGeometry.contentSize.height))

        let column = NSTableColumn(identifier: NSUserInterfaceItemIdentifier("collection"))
        column.resizingMask = .autoresizingMask
        outlineView.addTableColumn(column)
        outlineView.outlineTableColumn = column
        outlineView.headerView = nil
        outlineView.style = .sourceList
        outlineView.floatsGroupRows = false
        outlineView.allowsMultipleSelection = false
        outlineView.allowsEmptySelection = false
        outlineView.dataSource = self
        outlineView.delegate = self

        scrollView.documentView = outlineView
        scrollView.hasVerticalScroller = true
        scrollView.drawsBackground = false
        scrollView.autohidesScrollers = true
        pinEdges(scrollView, to: view)
    }

    override func viewDidLoad() {
        super.viewDidLoad()
        outlineView.reloadData()
        outlineView.expandItem(nil, expandChildren: true)
        // Row 0 is the first group header; "All Items" is row 1.
        outlineView.selectRowIndexes(IndexSet(integer: 1), byExtendingSelection: false)
    }

    // MARK: - NSOutlineViewDataSource

    func outlineView(_ outlineView: NSOutlineView, numberOfChildrenOfItem item: Any?) -> Int {
        (item as? SidebarNode)?.children.count ?? nodes.count
    }

    func outlineView(_ outlineView: NSOutlineView, child index: Int, ofItem item: Any?) -> Any {
        (item as? SidebarNode)?.children[index] ?? nodes[index]
    }

    func outlineView(_ outlineView: NSOutlineView, isItemExpandable item: Any) -> Bool {
        (item as? SidebarNode)?.isGroup ?? false
    }

    // MARK: - NSOutlineViewDelegate

    // The three groups are unselectable source-list headers, not rows.
    func outlineView(_ outlineView: NSOutlineView, isGroupItem item: Any) -> Bool {
        (item as? SidebarNode)?.isGroup ?? false
    }

    func outlineView(_ outlineView: NSOutlineView, shouldSelectItem item: Any) -> Bool {
        !((item as? SidebarNode)?.isGroup ?? false)
    }

    func outlineView(_ outlineView: NSOutlineView, shouldCollapseItem item: Any) -> Bool {
        false
    }

    func outlineView(_ outlineView: NSOutlineView, viewFor tableColumn: NSTableColumn?, item: Any) -> NSView? {
        guard let node = item as? SidebarNode else { return nil }
        let cell = NSTableCellView()
        let label = NSTextField(labelWithString: node.title)
        label.lineBreakMode = .byTruncatingTail
        cell.textField = label

        if node.isGroup {
            pinEdges(label, to: cell)
            return cell
        }

        let icon = NSImageView()
        icon.image = NSImage(systemSymbolName: node.symbol, accessibilityDescription: node.title)
        icon.contentTintColor = .controlAccentColor
        icon.setContentHuggingPriority(.required, for: .horizontal)
        cell.imageView = icon

        let row = NSStackView(views: [icon, label])
        row.orientation = .horizontal
        row.alignment = .centerY
        row.spacing = 6
        pinEdges(row, to: cell)
        return cell
    }

    func outlineViewSelectionDidChange(_ notification: Notification) {
        let row = outlineView.selectedRow
        guard row >= 0, let node = outlineView.item(atRow: row) as? SidebarNode, !node.isGroup else { return }
        onSelectCollection?(node.title)
    }
}

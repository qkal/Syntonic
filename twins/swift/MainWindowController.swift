import AppKit

// Both twins share these numbers; docs/twin-comparison.md records them as the
// comparison's fixed geometry.
enum TwinGeometry {
    static let appName = "Syntonic Twin"
    static let contentSize = NSSize(width: 1000, height: 640)
    static let frameOrigin = NSPoint(x: 200, y: 200)
    static let minContentSize = NSSize(width: 720, height: 480)
    static let resizedContentSize = NSSize(width: 820, height: 520)
    static let sidebarWidth: CGFloat = 220
    static let listWidth: CGFloat = 400
    static let settingsContentSize = NSSize(width: 520, height: 300)
    static let settingsFrameOrigin = NSPoint(x: 420, y: 420)
    static let detailInset: CGFloat = 20
    static let detailFieldWidth: CGFloat = 220
}

// Edge pinning is the only constraint API the twins use; everything else is
// NSStackView, NSGridView or split-view geometry (KTD3).
@MainActor
func pinEdges(_ view: NSView, to container: NSView, inset: CGFloat = 0) {
    view.translatesAutoresizingMaskIntoConstraints = false
    container.addSubview(view)
    NSLayoutConstraint.activate([
        view.leadingAnchor.constraint(equalTo: container.leadingAnchor, constant: inset),
        view.trailingAnchor.constraint(equalTo: container.trailingAnchor, constant: -inset),
        view.topAnchor.constraint(equalTo: container.topAnchor, constant: inset),
        view.bottomAnchor.constraint(equalTo: container.bottomAnchor, constant: -inset),
    ])
}

// The pinned view keeps its own height and sits against the top edge.
@MainActor
func pinTopEdges(_ view: NSView, to container: NSView, inset: CGFloat = 0) {
    view.translatesAutoresizingMaskIntoConstraints = false
    container.addSubview(view)
    NSLayoutConstraint.activate([
        view.leadingAnchor.constraint(equalTo: container.leadingAnchor, constant: inset),
        view.trailingAnchor.constraint(lessThanOrEqualTo: container.trailingAnchor, constant: -inset),
        view.topAnchor.constraint(equalTo: container.topAnchor, constant: inset),
    ])
}

final class TwinSplitViewController: NSSplitViewController {
    var onLayout: (() -> Void)?

    override func viewDidLayout() {
        super.viewDidLayout()
        onLayout?()
    }
}

final class MainWindowController: NSWindowController, NSToolbarDelegate, NSWindowDelegate {
    private static let addItemID = NSToolbarItem.Identifier("dev.kaino.syntonic.twin.add")
    private static let searchItemID = NSToolbarItem.Identifier("dev.kaino.syntonic.twin.search")

    private let splitViewController = TwinSplitViewController()
    private let sidebarViewController = SidebarViewController()
    private let listViewController = ListViewController()
    private let detailViewController = DetailViewController()
    private weak var searchField: NSSearchField?
    private var dividersPlaced = false

    init() {
        let window = NSWindow(contentRect: NSRect(origin: .zero, size: TwinGeometry.contentSize),
                              styleMask: [.titled, .closable, .miniaturizable, .resizable],
                              backing: .buffered,
                              defer: false)
        window.isReleasedWhenClosed = false // KTD7
        window.isRestorable = false
        window.autorecalculatesKeyViewLoop = false
        window.title = TwinGeometry.appName
        window.contentMinSize = TwinGeometry.minContentSize
        super.init(window: window)

        window.delegate = self
        buildSplitViewController()
        window.contentViewController = splitViewController

        let toolbar = NSToolbar(identifier: "dev.kaino.syntonic.twin.toolbar")
        toolbar.delegate = self
        toolbar.allowsUserCustomization = false
        toolbar.displayMode = .iconOnly
        window.toolbar = toolbar
        window.toolbarStyle = .unified
        window.setContentSize(TwinGeometry.contentSize)
        window.setFrameOrigin(TwinGeometry.frameOrigin)
    }

    @available(*, unavailable)
    required init?(coder: NSCoder) {
        fatalError("the twins build their windows in code, never from a nib")
    }

    override func showWindow(_ sender: Any?) {
        super.showWindow(sender)
        guard let window else { return }
        window.setContentSize(TwinGeometry.contentSize)
        window.setFrameOrigin(TwinGeometry.frameOrigin)
        placeDividers()
        chainKeyViews()
        window.makeFirstResponder(listViewController.firstResponderView)
        writeLayoutReport()
        print("SYNTONIC_TWIN_WINDOW \(window.windowNumber) \(window.title)")
    }

    // Moving a window does not relayout its views, so the report is refreshed
    // by hand: the capture script waits on these numbers.
    func windowDidMove(_ notification: Notification) {
        writeLayoutReport()
    }

    func windowDidResize(_ notification: Notification) {
        writeLayoutReport()
    }

    private func buildSplitViewController() {
        let sidebarItem = NSSplitViewItem(sidebarWithViewController: sidebarViewController)
        sidebarItem.minimumThickness = 180
        sidebarItem.maximumThickness = 320
        sidebarItem.canCollapse = true

        let listItem = NSSplitViewItem(contentListWithViewController: listViewController)
        listItem.minimumThickness = 280

        let detailItem = NSSplitViewItem(viewController: detailViewController)
        detailItem.minimumThickness = 280

        splitViewController.addSplitViewItem(sidebarItem)
        splitViewController.addSplitViewItem(listItem)
        splitViewController.addSplitViewItem(detailItem)
        splitViewController.onLayout = { [weak self] in self?.writeLayoutReport() }

        sidebarViewController.onSelectCollection = { [weak self] collection in
            self?.listViewController.collection = collection
            self?.writeLayoutReport()
        }
        listViewController.onSelectionChanged = { [weak self] item in
            self?.detailViewController.show(item)
            self?.writeLayoutReport()
        }
        detailViewController.onCommit = { [weak self] in
            self?.listViewController.reloadSelectedRow()
            self?.writeLayoutReport()
        }
        detailViewController.onDelete = { [weak self] in
            self?.listViewController.deleteSelectedItem()
            self?.writeLayoutReport()
        }
    }

    // Without a nib AppKit rebuilds the key view loop in geometric order, which
    // is not guaranteed to match construction order, so both twins turn the
    // rebuild off and chain Tab by hand.
    private func chainKeyViews() {
        let chain = sidebarViewController.keyViewChain
            + listViewController.keyViewChain
            + detailViewController.keyViewChain
        for (index, view) in chain.enumerated() {
            view.nextKeyView = chain[(index + 1) % chain.count]
        }
        window?.initialFirstResponder = listViewController.firstResponderView
    }

    private func placeDividers() {
        guard !dividersPlaced else { return }
        dividersPlaced = true
        let splitView = splitViewController.splitView
        splitView.setPosition(TwinGeometry.sidebarWidth, ofDividerAt: 0)
        splitView.setPosition(TwinGeometry.sidebarWidth + TwinGeometry.listWidth, ofDividerAt: 1)
    }

    // MARK: - Actions

    // The toolbar's Add item and File ▸ New Item both land here (R16).
    @objc func addNewItem(_ sender: Any?) {
        searchField?.stringValue = ""
        listViewController.searchText = ""
        listViewController.addItem()
    }

    @objc private func searchChanged(_ sender: NSSearchField) {
        listViewController.searchText = sender.stringValue
        writeLayoutReport()
    }

    // Edit ▸ Find moves focus into the toolbar's search field, the way every
    // macOS app with a search toolbar item does.
    @objc func beginSearch(_ sender: Any?) {
        guard let item = window?.toolbar?.items.first(where: { $0.itemIdentifier == Self.searchItemID })
                as? NSSearchToolbarItem else { return }
        item.beginSearchInteraction()
    }

    // MARK: - NSToolbarDelegate (R16)

    func toolbarDefaultItemIdentifiers(_ toolbar: NSToolbar) -> [NSToolbarItem.Identifier] {
        [.toggleSidebar, .sidebarTrackingSeparator, Self.addItemID, .flexibleSpace, Self.searchItemID]
    }

    func toolbarAllowedItemIdentifiers(_ toolbar: NSToolbar) -> [NSToolbarItem.Identifier] {
        toolbarDefaultItemIdentifiers(toolbar)
    }

    func toolbar(_ toolbar: NSToolbar,
                 itemForItemIdentifier itemIdentifier: NSToolbarItem.Identifier,
                 willBeInsertedIntoToolbar flag: Bool) -> NSToolbarItem? {
        switch itemIdentifier {
        case Self.addItemID:
            let item = NSToolbarItem(itemIdentifier: itemIdentifier)
            item.label = "Add"
            item.paletteLabel = "Add"
            item.toolTip = "Add an item"
            item.image = NSImage(systemSymbolName: "plus", accessibilityDescription: "Add")
            item.isBordered = true
            item.target = self
            item.action = #selector(addNewItem(_:))
            return item
        case Self.searchItemID:
            let item = NSSearchToolbarItem(itemIdentifier: itemIdentifier)
            item.resignsFirstResponderWithCancel = true
            item.searchField.sendsWholeSearchString = false
            item.searchField.sendsSearchStringImmediately = true
            item.searchField.target = self
            item.searchField.action = #selector(searchChanged(_:))
            searchField = item.searchField
            return item
        default:
            return nil
        }
    }

    // MARK: - Layout report (the text half of the U12 comparison)

    // With SYNTONIC_TWIN_DUMP set, every layout pass overwrites that file with
    // the frames and layout priorities the capture script files beside the
    // screenshot.
    private func writeLayoutReport() {
        guard let path = ProcessInfo.processInfo.environment["SYNTONIC_TWIN_DUMP"],
              let window,
              let contentView = window.contentView
        else { return }

        var lines = [
            String(format: "window.frame=%.1f,%.1f,%.1f,%.1f",
                   window.frame.origin.x, window.frame.origin.y,
                   window.frame.size.width, window.frame.size.height),
            String(format: "window.content=%.1f,%.1f", contentView.bounds.width, contentView.bounds.height),
            "sidebar.collapsed=\(splitViewController.splitViewItems[0].isCollapsed)",
            String(format: "sidebar.width=%.1f", sidebarViewController.view.frame.width),
            frameLine("sidebar", sidebarViewController.view, in: contentView),
            frameLine("list", listViewController.view, in: contentView),
            frameLine("detail", detailViewController.view, in: contentView),
            "list.rows=\(listViewController.visibleCount)",
            "list.selectedRow=\(listViewController.selectedRow)",
        ]
        lines.append(contentsOf: detailViewController.layoutReport(in: contentView))
        try? (lines.joined(separator: "\n") + "\n").write(toFile: path, atomically: true, encoding: .utf8)
    }
}

@MainActor
func frameLine(_ key: String, _ view: NSView, in reference: NSView) -> String {
    let rect = view.convert(view.bounds, to: reference)
    return String(format: "%@.frame=%.1f,%.1f,%.1f,%.1f",
                  key, rect.origin.x, rect.origin.y, rect.size.width, rect.size.height)
}

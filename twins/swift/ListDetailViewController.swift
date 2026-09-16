import AppKit

// The list pane: a view-based table fed by a data source (R17).
final class ListViewController: NSViewController, NSTableViewDataSource, NSTableViewDelegate {
    var onSelectionChanged: ((Item?) -> Void)?

    private enum Column {
        static let title = NSUserInterfaceItemIdentifier("title")
        static let owner = NSUserInterfaceItemIdentifier("owner")
        static let category = NSUserInterfaceItemIdentifier("category")
    }

    private var items = SeedData.items()
    private var visibleItems: [Item] = []
    private let tableView = NSTableView()
    private let scrollView = NSScrollView()

    var collection = SeedData.allItems {
        didSet { refilter(keepingSelection: false) }
    }

    var searchText = "" {
        didSet { refilter(keepingSelection: true) }
    }

    var firstResponderView: NSView { tableView }
    var keyViewChain: [NSView] { [tableView] }
    var visibleCount: Int { visibleItems.count }
    var selectedRow: Int { tableView.selectedRow }

    override func loadView() {
        view = NSView(frame: NSRect(x: 0, y: 0,
                                    width: TwinGeometry.listWidth,
                                    height: TwinGeometry.contentSize.height))

        addColumn(Column.title, "Title", width: 170)
        addColumn(Column.owner, "Owner", width: 120)
        addColumn(Column.category, "Category", width: 90)
        tableView.style = .inset
        tableView.usesAlternatingRowBackgroundColors = true
        tableView.allowsMultipleSelection = false
        tableView.allowsEmptySelection = true
        tableView.columnAutoresizingStyle = .lastColumnOnlyAutoresizingStyle
        tableView.headerView = NSTableHeaderView()
        tableView.dataSource = self
        tableView.delegate = self

        scrollView.documentView = tableView
        scrollView.hasVerticalScroller = true
        scrollView.autohidesScrollers = true
        pinEdges(scrollView, to: view)
    }

    override func viewDidLoad() {
        super.viewDidLoad()
        refilter(keepingSelection: false)
    }

    private func addColumn(_ identifier: NSUserInterfaceItemIdentifier, _ title: String, width: CGFloat) {
        let column = NSTableColumn(identifier: identifier)
        column.title = title
        column.width = width
        column.minWidth = 60
        tableView.addTableColumn(column)
    }

    // MARK: - Model

    private func matches(_ item: Item) -> Bool {
        guard collection == SeedData.allItems || item.collection == collection else { return false }
        guard !searchText.isEmpty else { return true }
        return item.title.localizedCaseInsensitiveContains(searchText)
            || item.owner.localizedCaseInsensitiveContains(searchText)
    }

    private func refilter(keepingSelection: Bool) {
        guard isViewLoaded else { return }
        let selected = keepingSelection ? selectedItem : nil
        visibleItems = items.filter(matches)
        tableView.reloadData()
        if let selected, let row = visibleItems.firstIndex(where: { $0 === selected }) {
            tableView.selectRowIndexes(IndexSet(integer: row), byExtendingSelection: false)
        } else {
            tableView.deselectAll(nil)
            onSelectionChanged?(nil)
        }
    }

    private var selectedItem: Item? {
        let row = tableView.selectedRow
        return row >= 0 && row < visibleItems.count ? visibleItems[row] : nil
    }

    // Add appends a row to the current collection and selects it (R16).
    func addItem() {
        let target = collection == SeedData.allItems ? SeedData.defaultCollection : collection
        let item = Item(title: SeedData.newItemTitle,
                        owner: SeedData.newItemOwner,
                        category: SeedData.newItemCategory,
                        flagged: false,
                        collection: target)
        items.append(item)
        visibleItems = items.filter(matches)
        tableView.reloadData()
        guard let row = visibleItems.firstIndex(where: { $0 === item }) else { return }
        tableView.selectRowIndexes(IndexSet(integer: row), byExtendingSelection: false)
        tableView.scrollRowToVisible(row)
    }

    func deleteSelectedItem() {
        guard let selected = selectedItem else { return }
        items.removeAll { $0 === selected }
        visibleItems = items.filter(matches)
        tableView.reloadData()
        tableView.deselectAll(nil)
        onSelectionChanged?(nil)
    }

    func reloadSelectedRow() {
        let row = tableView.selectedRow
        guard row >= 0 else { return }
        tableView.reloadData(forRowIndexes: IndexSet(integer: row),
                             columnIndexes: IndexSet(integersIn: 0 ..< tableView.numberOfColumns))
    }

    // Edit ▸ Delete reaches the list through the responder chain.
    @objc func delete(_ sender: Any?) {
        deleteSelectedItem()
    }

    // MARK: - NSTableViewDataSource / NSTableViewDelegate

    func numberOfRows(in tableView: NSTableView) -> Int {
        visibleItems.count
    }

    func tableView(_ tableView: NSTableView, viewFor tableColumn: NSTableColumn?, row: Int) -> NSView? {
        guard let identifier = tableColumn?.identifier, row < visibleItems.count else { return nil }
        let item = visibleItems[row]
        let cell = tableView.makeView(withIdentifier: identifier, owner: self) as? NSTableCellView
            ?? makeCell(identifier)
        switch identifier {
        case Column.title: cell.textField?.stringValue = item.title
        case Column.owner: cell.textField?.stringValue = item.owner
        default: cell.textField?.stringValue = item.category
        }
        return cell
    }

    private func makeCell(_ identifier: NSUserInterfaceItemIdentifier) -> NSTableCellView {
        let cell = NSTableCellView()
        cell.identifier = identifier
        let label = NSTextField(labelWithString: "")
        label.lineBreakMode = .byTruncatingTail
        cell.textField = label
        pinEdges(label, to: cell)
        return cell
    }

    func tableViewSelectionDidChange(_ notification: Notification) {
        onSelectionChanged?(selectedItem)
    }
}

// The detail pane: an NSGridView form over the selected row (R17, F6).
final class DetailViewController: NSViewController {
    var onCommit: (() -> Void)?
    var onDelete: (() -> Void)?

    private let headerLabel = NSTextField(labelWithString: "")
    private let titleField = NSTextField(string: "")
    private let ownerField = NSTextField(string: "")
    private let categoryPopUp = NSPopUpButton(frame: .zero, pullsDown: false)
    private let flaggedCheckbox = NSButton(checkboxWithTitle: "Flagged", target: nil, action: nil)
    private let deleteButton = NSButton(title: "Delete", target: nil, action: nil)
    private var grid = NSGridView(numberOfColumns: 2, rows: 0)
    private weak var item: Item?

    override func loadView() {
        view = NSView(frame: NSRect(x: 0, y: 0, width: 360, height: TwinGeometry.contentSize.height))

        headerLabel.font = .boldSystemFont(ofSize: NSFont.systemFontSize)
        titleField.target = self
        titleField.action = #selector(commitText(_:))
        ownerField.target = self
        ownerField.action = #selector(commitText(_:))
        categoryPopUp.addItems(withTitles: SeedData.categories)
        categoryPopUp.target = self
        categoryPopUp.action = #selector(commitCategory(_:))
        flaggedCheckbox.target = self
        flaggedCheckbox.action = #selector(commitFlagged(_:))
        deleteButton.bezelStyle = .rounded
        deleteButton.target = self
        deleteButton.action = #selector(deleteItem(_:))
        titleField.setAccessibilityLabel("Title")
        ownerField.setAccessibilityLabel("Owner")
        categoryPopUp.setAccessibilityLabel("Category")

        grid.rowSpacing = 10
        grid.columnSpacing = 12
        grid.addRow(with: [headerLabel])
        grid.row(at: 0).mergeCells(in: NSRange(location: 0, length: 2))
        grid.cell(atColumnIndex: 0, rowIndex: 0).xPlacement = .leading
        grid.addRow(with: [NSTextField(labelWithString: "Title:"), titleField])
        grid.addRow(with: [NSTextField(labelWithString: "Owner:"), ownerField])
        grid.addRow(with: [NSTextField(labelWithString: "Category:"), categoryPopUp])
        grid.addRow(with: [NSGridCell.emptyContentView, flaggedCheckbox])
        grid.addRow(with: [NSGridCell.emptyContentView, deleteButton])
        grid.column(at: 0).xPlacement = .trailing
        grid.column(at: 1).xPlacement = .leading
        grid.column(at: 1).width = TwinGeometry.detailFieldWidth
        grid.cell(for: titleField)?.xPlacement = .fill
        grid.cell(for: ownerField)?.xPlacement = .fill
        grid.cell(for: categoryPopUp)?.xPlacement = .fill
        pinTopEdges(grid, to: view, inset: TwinGeometry.detailInset)
        show(nil)
    }

    // The window chains these in this order; see MainWindowController.
    var keyViewChain: [NSView] {
        [titleField, ownerField, categoryPopUp, flaggedCheckbox, deleteButton]
    }

    func show(_ item: Item?) {
        self.item = item
        let enabled = item != nil
        headerLabel.stringValue = enabled ? "Details" : "No Selection"
        headerLabel.textColor = enabled ? .labelColor : .secondaryLabelColor
        titleField.stringValue = item?.title ?? ""
        ownerField.stringValue = item?.owner ?? ""
        categoryPopUp.selectItem(withTitle: item?.category ?? SeedData.categories[0])
        flaggedCheckbox.state = (item?.flagged ?? false) ? .on : .off
        titleField.isEnabled = enabled
        ownerField.isEnabled = enabled
        categoryPopUp.isEnabled = enabled
        flaggedCheckbox.isEnabled = enabled
        deleteButton.isEnabled = enabled
    }

    // Text fields commit when editing ends: the field's action fires on Return,
    // on Tab and when the field loses focus (F6).
    @objc private func commitText(_ sender: NSTextField) {
        guard let item else { return }
        if sender === titleField {
            item.title = sender.stringValue
        } else {
            item.owner = sender.stringValue
        }
        onCommit?()
    }

    @objc private func commitCategory(_ sender: NSPopUpButton) {
        item?.category = sender.titleOfSelectedItem ?? SeedData.categories[0]
        onCommit?()
    }

    @objc private func commitFlagged(_ sender: NSButton) {
        item?.flagged = sender.state == .on
        onCommit?()
    }

    @objc private func deleteItem(_ sender: Any?) {
        onDelete?()
    }

    // U12 compares these numbers before it accepts screenshot parity.
    func layoutReport(in reference: NSView) -> [String] {
        var lines = [frameLine("detail.grid", grid, in: reference)]
        for (name, control) in [("header", headerLabel as NSView),
                                ("title", titleField),
                                ("owner", ownerField),
                                ("category", categoryPopUp),
                                ("flagged", flaggedCheckbox),
                                ("delete", deleteButton)] {
            lines.append(contentsOf: describe("detail.\(name)", control, in: reference))
        }
        lines.append("detail.header.value=\(headerLabel.stringValue)")
        lines.append("detail.title.value=\(titleField.stringValue)")
        lines.append("detail.owner.value=\(ownerField.stringValue)")
        lines.append("detail.category.value=\(categoryPopUp.titleOfSelectedItem ?? "")")
        lines.append("detail.flagged.value=\(flaggedCheckbox.state == .on)")
        lines.append("detail.delete.enabled=\(deleteButton.isEnabled)")
        return lines
    }

    private func describe(_ key: String, _ control: NSView, in reference: NSView) -> [String] {
        let intrinsic = control.intrinsicContentSize
        return [
            frameLine(key, control, in: reference),
            String(format: "%@.intrinsic=%.1f,%.1f", key, intrinsic.width, intrinsic.height),
            String(format: "%@.hug=%.0f,%.0f", key,
                   control.contentHuggingPriority(for: .horizontal).rawValue,
                   control.contentHuggingPriority(for: .vertical).rawValue),
            String(format: "%@.resist=%.0f,%.0f", key,
                   control.contentCompressionResistancePriority(for: .horizontal).rawValue,
                   control.contentCompressionResistancePriority(for: .vertical).rawValue),
        ]
    }
}

import AppKit

// A separate window with toolbar-style tabs, opened by the application menu's
// Settings item (R19).
final class SettingsWindowController: NSWindowController {
    private let tabViewController = SettingsTabViewController()

    init() {
        let window = NSWindow(contentRect: NSRect(origin: .zero, size: TwinGeometry.settingsContentSize),
                              styleMask: [.titled, .closable],
                              backing: .buffered,
                              defer: false)
        window.isReleasedWhenClosed = false // KTD7
        window.isRestorable = false
        window.autorecalculatesKeyViewLoop = false
        window.contentViewController = tabViewController
        window.toolbarStyle = .preference
        super.init(window: window)
        window.setFrameOrigin(TwinGeometry.settingsFrameOrigin)
    }

    @available(*, unavailable)
    required init?(coder: NSCoder) {
        fatalError("the twins build their windows in code, never from a nib")
    }

    override func showWindow(_ sender: Any?) {
        super.showWindow(sender)
        guard let window else { return }
        window.setFrameOrigin(TwinGeometry.settingsFrameOrigin)
        print("SYNTONIC_TWIN_WINDOW \(window.windowNumber) Settings")
    }
}

final class SettingsTabViewController: NSTabViewController {
    override func viewDidLoad() {
        super.viewDidLoad()
        tabStyle = .toolbar

        addPane(SettingsPaneViewController(pane: .general), "General", "gearshape")
        addPane(SettingsPaneViewController(pane: .appearance), "Appearance", "paintpalette")
        addPane(SettingsPaneViewController(pane: .advanced), "Advanced", "slider.horizontal.3")
    }

    private func addPane(_ controller: NSViewController, _ label: String, _ symbol: String) {
        let tab = NSTabViewItem(viewController: controller)
        tab.label = label
        tab.image = NSImage(systemSymbolName: symbol, accessibilityDescription: label)
        addTabViewItem(tab)
    }
}

final class SettingsPaneViewController: NSViewController {
    enum Pane {
        case general, appearance, advanced
    }

    private let pane: Pane

    init(pane: Pane) {
        self.pane = pane
        super.init(nibName: nil, bundle: nil)
    }

    @available(*, unavailable)
    required init?(coder: NSCoder) {
        fatalError("the twins build their panes in code, never from a nib")
    }

    override func loadView() {
        view = NSView(frame: NSRect(origin: .zero, size: TwinGeometry.settingsContentSize))

        let grid = NSGridView(numberOfColumns: 2, rows: 0)
        grid.rowSpacing = 10
        grid.columnSpacing = 12
        var chain: [NSView] = []

        switch pane {
        case .general:
            let nameField = NSTextField(string: "Syntonic")
            let popUp = NSPopUpButton(frame: .zero, pullsDown: false)
            popUp.addItems(withTitles: ["All Items", "Inbox", "Favorites"])
            let checkbox = NSButton(checkboxWithTitle: "Reopen the last collection", target: nil, action: nil)
            checkbox.state = .on
            grid.addRow(with: [NSTextField(labelWithString: "Workspace:"), nameField])
            grid.addRow(with: [NSTextField(labelWithString: "Opens with:"), popUp])
            grid.addRow(with: [NSGridCell.emptyContentView, checkbox])
            grid.cell(for: nameField)?.xPlacement = .fill
            chain = [nameField, popUp, checkbox]
        case .appearance:
            let popUp = NSPopUpButton(frame: .zero, pullsDown: false)
            popUp.addItems(withTitles: ["System", "Light", "Dark"])
            let density = NSPopUpButton(frame: .zero, pullsDown: false)
            density.addItems(withTitles: ["Comfortable", "Compact"])
            let checkbox = NSButton(checkboxWithTitle: "Show the flagged badge", target: nil, action: nil)
            grid.addRow(with: [NSTextField(labelWithString: "Theme:"), popUp])
            grid.addRow(with: [NSTextField(labelWithString: "Row height:"), density])
            grid.addRow(with: [NSGridCell.emptyContentView, checkbox])
            chain = [popUp, density, checkbox]
        case .advanced:
            let pathField = NSTextField(string: "~/Library/Syntonic")
            let checkbox = NSButton(checkboxWithTitle: "Log layout reports", target: nil, action: nil)
            let resetButton = NSButton(title: "Reset Settings", target: nil, action: nil)
            resetButton.bezelStyle = .rounded
            grid.addRow(with: [NSTextField(labelWithString: "Scratch folder:"), pathField])
            grid.addRow(with: [NSGridCell.emptyContentView, checkbox])
            grid.addRow(with: [NSGridCell.emptyContentView, resetButton])
            grid.cell(for: pathField)?.xPlacement = .fill
            chain = [pathField, checkbox, resetButton]
        }

        grid.column(at: 0).xPlacement = .trailing
        grid.column(at: 1).xPlacement = .leading
        grid.column(at: 1).width = TwinGeometry.detailFieldWidth
        pinTopEdges(grid, to: view, inset: TwinGeometry.detailInset)

        // Same reason as the detail pane: the Tab order is chained by hand so
        // both twins walk the controls in construction order.
        for (index, control) in chain.enumerated() {
            control.nextKeyView = chain[(index + 1) % chain.count]
        }
    }
}

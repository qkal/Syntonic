import AppKit

@MainActor
final class AppDelegate: NSObject, NSApplicationDelegate {
    let mainWindowController = MainWindowController()
    private lazy var settingsWindowController = SettingsWindowController()

    func applicationDidFinishLaunching(_ notification: Notification) {
        applyAppearanceOverride()
        NSApp.mainMenu = makeMainMenu()
        mainWindowController.showWindow(nil)
        NSApp.activate()
    }

    // Closing the last window only orders it out; the app keeps running with
    // its menu bar until Quit (F1, KTD7).
    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        false
    }

    func applicationShouldHandleReopen(_ sender: NSApplication, hasVisibleWindows flag: Bool) -> Bool {
        if !flag {
            mainWindowController.showWindow(nil)
        }
        return true
    }

    // AppKit has no responder-chain action for Settings, so the menu item calls
    // back into the app (KTD16).
    @objc func showSettings(_ sender: Any?) {
        settingsWindowController.showWindow(nil)
        settingsWindowController.window?.makeKeyAndOrderFront(nil)
    }

    // The capture script pins one appearance per run instead of switching the
    // system theme, so a run is reproducible.
    private func applyAppearanceOverride() {
        switch ProcessInfo.processInfo.environment["SYNTONIC_TWIN_APPEARANCE"] {
        case "dark": NSApp.appearance = NSAppearance(named: .darkAqua)
        case "light": NSApp.appearance = NSAppearance(named: .aqua)
        default: break
        }
    }

    // MARK: - Menu bar (R18, KTD16)

    private func makeMainMenu() -> NSMenu {
        let name = TwinGeometry.appName
        let mainMenu = NSMenu()

        let appMenu = NSMenu()
        appMenu.addItem(item("About \(name)", #selector(NSApplication.orderFrontStandardAboutPanel(_:))))
        appMenu.addItem(.separator())
        let settings = item("Settings…", #selector(AppDelegate.showSettings(_:)), ",")
        settings.target = self
        appMenu.addItem(settings)
        appMenu.addItem(.separator())
        let services = NSMenuItem(title: "Services", action: nil, keyEquivalent: "")
        let servicesMenu = NSMenu()
        services.submenu = servicesMenu
        NSApp.servicesMenu = servicesMenu
        appMenu.addItem(services)
        appMenu.addItem(.separator())
        appMenu.addItem(item("Hide \(name)", #selector(NSApplication.hide(_:)), "h"))
        let hideOthers = item("Hide Others", #selector(NSApplication.hideOtherApplications(_:)), "h")
        hideOthers.keyEquivalentModifierMask = [.command, .option]
        appMenu.addItem(hideOthers)
        appMenu.addItem(item("Show All", #selector(NSApplication.unhideAllApplications(_:))))
        appMenu.addItem(.separator())
        appMenu.addItem(item("Quit \(name)", #selector(NSApplication.terminate(_:)), "q"))
        mainMenu.addItem(submenu(name, appMenu))

        let fileMenu = NSMenu(title: "File")
        fileMenu.addItem(item("New Item", #selector(MainWindowController.addNewItem(_:)), "n"))
        fileMenu.addItem(.separator())
        fileMenu.addItem(item("Close", #selector(NSWindow.performClose(_:)), "w"))
        mainMenu.addItem(submenu("File", fileMenu))

        // Every item here is a responder-chain action with no app code behind
        // it (AE5).
        let editMenu = NSMenu(title: "Edit")
        editMenu.addItem(item("Undo", Selector(("undo:")), "z"))
        let redo = item("Redo", Selector(("redo:")), "z")
        redo.keyEquivalentModifierMask = [.command, .shift]
        editMenu.addItem(redo)
        editMenu.addItem(.separator())
        editMenu.addItem(item("Cut", #selector(NSText.cut(_:)), "x"))
        editMenu.addItem(item("Copy", #selector(NSText.copy(_:)), "c"))
        editMenu.addItem(item("Paste", #selector(NSText.paste(_:)), "v"))
        editMenu.addItem(item("Delete", #selector(NSText.delete(_:))))
        editMenu.addItem(.separator())
        editMenu.addItem(item("Select All", #selector(NSText.selectAll(_:)), "a"))
        editMenu.addItem(.separator())
        editMenu.addItem(item("Find", #selector(MainWindowController.beginSearch(_:)), "f"))
        mainMenu.addItem(submenu("Edit", editMenu))

        let viewMenu = NSMenu(title: "View")
        let toggleSidebar = item("Toggle Sidebar", #selector(NSSplitViewController.toggleSidebar(_:)), "s")
        toggleSidebar.keyEquivalentModifierMask = [.command, .control]
        viewMenu.addItem(toggleSidebar)
        viewMenu.addItem(.separator())
        let fullScreen = item("Enter Full Screen", #selector(NSWindow.toggleFullScreen(_:)), "f")
        fullScreen.keyEquivalentModifierMask = [.command, .control]
        viewMenu.addItem(fullScreen)
        mainMenu.addItem(submenu("View", viewMenu))

        let windowMenu = NSMenu(title: "Window")
        windowMenu.addItem(item("Minimize", #selector(NSWindow.performMiniaturize(_:)), "m"))
        windowMenu.addItem(item("Zoom", #selector(NSWindow.performZoom(_:))))
        windowMenu.addItem(.separator())
        windowMenu.addItem(item("Bring All to Front", #selector(NSApplication.arrangeInFront(_:))))
        mainMenu.addItem(submenu("Window", windowMenu))
        NSApp.windowsMenu = windowMenu

        let helpMenu = NSMenu(title: "Help")
        let help = item("\(name) Help", #selector(NSApplication.showHelp(_:)), "?")
        helpMenu.addItem(help)
        mainMenu.addItem(submenu("Help", helpMenu))
        NSApp.helpMenu = helpMenu

        return mainMenu
    }

    private func item(_ title: String, _ action: Selector?, _ key: String = "") -> NSMenuItem {
        NSMenuItem(title: title, action: action, keyEquivalent: key)
    }

    private func submenu(_ title: String, _ menu: NSMenu) -> NSMenuItem {
        let holder = NSMenuItem(title: title, action: nil, keyEquivalent: "")
        holder.submenu = menu
        return holder
    }
}

import AppKit

// The capture script reads the window numbers this process prints, so stdout
// must not sit in a block buffer when it is redirected to a log file.
setvbuf(stdout, nil, _IONBF, 0)

let application = NSApplication.shared
let twinDelegate = AppDelegate()
application.delegate = twinDelegate
application.setActivationPolicy(.regular)
application.run()

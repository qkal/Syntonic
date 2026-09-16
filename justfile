# Syntonic - one-command recipes (KTD1, R20).
#
# Everything here runs with the Command Line Tools, CMake and just. Nothing
# needs Xcode.app.

set shell := ["bash", "-euo", "pipefail", "-c"]

build_dir := "build"
san_dir := "build-san"
twin_name := "Syntonic Twin"
swift_twin_dir := "build/twin-swift"
c_twin_dir := "build/twin-c"
macos_floor := "26.0"
swift_twin_sources := "twins/swift/main.swift twins/swift/AppDelegate.swift twins/swift/MainWindowController.swift twins/swift/SidebarViewController.swift twins/swift/ListDetailViewController.swift twins/swift/SettingsViewController.swift twins/swift/SeedData.swift"

# List the recipes.
default:
    @just --list

# Configure and build the library and its tests.
build:
    cmake -S . -B {{build_dir}} -G "Unix Makefiles"
    cmake --build {{build_dir}} --parallel

# Run every CTest suite.
test: build
    ctest --test-dir {{build_dir}} --output-on-failure

# Rebuild under AddressSanitizer and UndefinedBehaviorSanitizer, then run the suite.
test-san:
    cmake -S . -B {{san_dir}} -G "Unix Makefiles" -DSYNTONIC_SANITIZE=ON
    cmake --build {{san_dir}} --parallel
    ctest --test-dir {{san_dir}} --output-on-failure

# A non-instrumented `leaks -atExit` run that fails only on allocations
# attributed to Syntonic's own frames; the framework baseline is printed.
# Leak gate (KTD12).
leaks: build
    ./scripts/leaks-check.sh {{build_dir}}

# Public header discipline: macros, inline, variadics, availability, selectors.
lint-headers:
    ./scripts/lint-headers.sh include

# Uses clang's fixed-enum extension and no pedantic flag, the same terms
# Apple's own C headers rely on.
# Check that the public headers also compile in C11 mode (R2).
check-c11:
    printf '#include <syntonic/syntonic.h>\nbool syntonic_c11_probe(void) { return ns_version_string()[0] != 0; }\n' | clang -x c -std=c11 -Iinclude -Wall -Wextra -Werror -fsyntax-only -

#   just bundle build/test_smoke Smoke dev.kaino.syntonic.smoke
# Pass a Developer ID identity as the last argument to sign for distribution.
# Turn a built executable into a signed .app (R20, AE7).
bundle exe name id icons="twins/shared/icon" identity="":
    ./scripts/bundle.sh "{{exe}}" "{{name}}" "{{id}}" "{{icons}}" "{{identity}}"

# CMake cannot build Swift with the Makefile generator, so the Swift twin
# compiles by calling swiftc directly (KTD1).
# Compile and bundle the Swift reference twin.
twin-swift-build:
    mkdir -p {{swift_twin_dir}}
    xcrun --sdk macosx swiftc -O -swift-version 6 -target arm64-apple-macos{{macos_floor}} -o {{swift_twin_dir}}/twin-swift {{swift_twin_sources}}
    ./scripts/bundle.sh {{swift_twin_dir}}/twin-swift "{{twin_name}}" dev.kaino.syntonic.twin.swift twins/shared/icon
    codesign --verify --strict "{{swift_twin_dir}}/{{twin_name}}.app"

# Build, sign and launch the Swift twin.
twin-swift: twin-swift-build
    open "{{swift_twin_dir}}/{{twin_name}}.app"

# Runs the bare executable, so the quality floor covers both twins, not only
# the C one. Quit it with Command-Q.
# Build and run the Swift twin under AddressSanitizer and UndefinedBehaviorSanitizer.
twin-swift-san:
    mkdir -p {{swift_twin_dir}}
    xcrun --sdk macosx swiftc -g -swift-version 6 -sanitize=address,undefined -target arm64-apple-macos{{macos_floor}} -o {{swift_twin_dir}}/twin-swift-san {{swift_twin_sources}}
    {{swift_twin_dir}}/twin-swift-san

# Capture the Swift twin in every checklist state, in one appearance.
capture-swift appearance="light": twin-swift-build
    ./scripts/capture-states.sh swift {{appearance}}

# Same app name and icon set as the Swift twin, so nothing on screen tells the
# two apart; only the bundle id differs, and nothing shows it.
# Compile and bundle the C twin.
twin-c-build: lint-twin-c build
    mkdir -p {{c_twin_dir}}
    SYNTONIC_BUNDLE_DIR={{c_twin_dir}} ./scripts/bundle.sh {{build_dir}}/twin_c "{{twin_name}}" dev.kaino.syntonic.twin.c twins/shared/icon
    codesign --verify --strict "{{c_twin_dir}}/{{twin_name}}.app"

# Build, sign and launch the C twin.
twin-c: twin-c-build
    open "{{c_twin_dir}}/{{twin_name}}.app"

# No AppKit header, no Foundation header, no Objective-C runtime call.
# The C twin is written against Syntonic and nothing else (AE7).
lint-twin-c:
    @if grep -nE '#[[:space:]]*include[[:space:]]*[<"](objc|Foundation|AppKit|Cocoa)/' twins/c/*.c twins/c/*.h; then echo "lint-twin-c: the C twin may include syntonic.h and the C standard headers only." >&2; exit 1; fi
    @if grep -nE 'objc_msgSend|objc_getClass|objc_lookUpClass|sel_registerName|sel_getUid|@selector' twins/c/*.c twins/c/*.h; then echo "lint-twin-c: the C twin may not call the Objective-C runtime." >&2; exit 1; fi

# Every wrapper is ARC Objective-C against the SDK's own API.
# No wrapper reaches for the Objective-C runtime by hand (R3).
lint-no-runtime:
    @if grep -nE 'objc_msgSend|objc_getClass|objc_lookUpClass|sel_registerName|sel_getUid' src/*.m include/syntonic/*.h; then echo "lint-no-runtime: a wrapper calls the Objective-C runtime by hand." >&2; exit 1; fi

# Needs the Screen Recording and Accessibility grants; without them
# capture-states.sh stops with a named message instead of writing black images.
# Capture both twins in every state and both appearances for the blind comparison.
compare: twin-swift-build twin-c-build
    ./scripts/capture-states.sh swift light
    ./scripts/capture-states.sh swift dark
    ./scripts/capture-states.sh c light
    ./scripts/capture-states.sh c dark
    @echo "compare: 40 states per twin in {{build_dir}}/compare. Shuffle, score and record the result in docs/twin-comparison.md section 10."

# Compare the accessibility role and label trees of the two twins (R23).
ax-compare: twin-swift-build twin-c-build
    #!/usr/bin/env bash
    set -euo pipefail
    out="{{build_dir}}/compare/ax"
    mkdir -p "$out"
    for twin in swift c; do
      app="{{build_dir}}/twin-$twin/{{twin_name}}.app"
      exe=$(plutil -extract CFBundleExecutable raw "$app/Contents/Info.plist")
      open -n "$app"
      for _ in $(seq 100); do
        pid=$(pgrep -n -x "$exe" || true)
        [[ -n "${pid:-}" ]] && break
        sleep 0.1
      done
      [[ -n "${pid:-}" ]] || { echo "ax-compare: $twin never appeared in the process list." >&2; exit 1; }
      status=0
      {{build_dir}}/axdump "$pid" > "$out/$twin.txt" || status=$?
      kill "$pid" 2>/dev/null || true
      [[ $status -eq 0 ]] || { echo "ax-compare: axdump could not read the $twin twin; see the message above." >&2; exit 1; }
    done
    diff -u "$out/swift.txt" "$out/c.txt" && echo "ax-compare: the two trees match."

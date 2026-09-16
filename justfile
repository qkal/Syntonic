# Syntonic - one-command recipes (KTD1, R20).
#
# Everything here runs with the Command Line Tools, CMake and just. Nothing
# needs Xcode.app.

set shell := ["bash", "-euo", "pipefail", "-c"]

build_dir := "build"
san_dir := "build-san"
twin_name := "Syntonic Twin"
swift_twin_dir := "build/twin-swift"
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

# Build, sign and launch the C twin.
twin-c:
    @echo "just twin-c: not built yet - unit U12 adds the C twin under twins/c." >&2; exit 1

# Capture both twins in every state and both appearances for the blind comparison.
compare:
    @echo "just compare: not built yet - unit U12 adds the twin comparison harness." >&2; exit 1

# Compare the accessibility role and label trees of the two twins.
ax-compare:
    @echo "just ax-compare: not built yet - unit U12 adds the accessibility comparison." >&2; exit 1

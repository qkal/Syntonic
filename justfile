# Syntonic - one-command recipes (KTD1, R20).
#
# Everything here runs with the Command Line Tools, CMake and just. Nothing
# needs Xcode.app.

set shell := ["bash", "-euo", "pipefail", "-c"]

build_dir := "build"
san_dir := "build-san"

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

# Build, sign and launch the Swift twin.
twin-swift:
    @echo "just twin-swift: not built yet - unit U2 adds the Swift twin under twins/swift." >&2; exit 1

# Build, sign and launch the C twin.
twin-c:
    @echo "just twin-c: not built yet - unit U12 adds the C twin under twins/c." >&2; exit 1

# Capture both twins in every state and both appearances for the blind comparison.
compare:
    @echo "just compare: not built yet - unit U12 adds the twin comparison harness." >&2; exit 1

# Compare the accessibility role and label trees of the two twins.
ax-compare:
    @echo "just ax-compare: not built yet - unit U12 adds the accessibility comparison." >&2; exit 1

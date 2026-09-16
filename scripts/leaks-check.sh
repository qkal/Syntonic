#!/bin/bash
#
# Leak gate (KTD12).
#
# An empty AppKit process already reports hundreds of framework-internal
# leaks, so a zero count is not a usable gate and LeakSanitizer is unreliable
# on arm64 macOS. Instead this runs each binary under a non-instrumented
# `leaks -atExit` and fails only on allocations whose *innermost non-allocator*
# frame belongs to the binary under test - that is, to Syntonic's own code, a
# test, or a twin.
#
# "Innermost non-allocator" rather than "mentions Syntonic anywhere": an AppKit
# framework cache filled during a Syntonic call has a Syntonic frame somewhere
# in its backtrace, so a contains-check fires on the framework baseline.
#
# The trade-off: an object that AppKit allocates on our behalf and then leaks
# is attributed to AppKit, not to us. That is deliberate - false negatives are
# survivable here, a permanently red gate is not.
#
# Usage:
#   scripts/leaks-check.sh [build-dir]
#   scripts/leaks-check.sh <binary> [binary ...]
#
# Build the binaries with debug info and a .dSYM beside them (CMake does this
# with a dsymutil post-build step) so the backtraces symbolize.

set -uo pipefail

targets=()
if [[ $# -eq 0 ]]; then
  set -- build
fi

if [[ $# -eq 1 && -d "$1" ]]; then
  while IFS= read -r candidate; do
    targets+=("$candidate")
  done < <(find "$1" -maxdepth 1 -type f -perm -111 -name 'test_*' | sort)
  if [[ ${#targets[@]} -eq 0 ]]; then
    echo "leaks-check: no test binaries in '$1'; build first." >&2
    exit 1
  fi
else
  targets=("$@")
fi

status=0

for target in "${targets[@]}"; do
  if [[ ! -x "$target" ]]; then
    echo "leaks-check: '$target' is not an executable." >&2
    status=1
    continue
  fi

  image=$(basename "$target")
  echo "== leaks: $image"

  report=$(MallocScribble=1 MallocStackLogging=1 \
    leaks -atExit -- "$target" 2>/dev/null)

  # `leaks -atExit` prints nothing and still exits 0 when the target crashes
  # before its atExit handlers run, which would read as a clean pass.
  if [[ "$report" != *"leaks Report Version"* ]]; then
    echo "  no leaks report - '$image' probably crashed or exited early." >&2
    status=1
    continue
  fi

  echo "$report" | awk -v image="$image" '
    # Frames print innermost-last within a block but carry an explicit index,
    # so frame 0 is the allocator and the first non-allocator frame is the
    # lowest index we have not filtered out.
    function is_allocator(name) {
      return name == "libsystem_malloc.dylib" \
          || name == "libsystem_c.dylib" \
          || name == "libc++abi.dylib" \
          || name == "libobjc.A.dylib" \
          || name == "libsystem_blocks.dylib" \
          || name ~ /^libmalloc/
    }
    function flush_block(  i, best, best_image) {
      if (!header) return
      best = -1
      for (i = 0; i < nframes; i++) {
        if (is_allocator(frame_image[i])) continue
        if (best < 0 || frame_index[i] < best) {
          best = frame_index[i]
          best_image = frame_image[i]
        }
      }
      if (best >= 0 && best_image == image) {
        hits++
        print "  " header
        for (i = 0; i < nframes; i++) print "    " frame_text[i]
      }
      header = ""
      nframes = 0
    }
    /^Process [0-9]+: [0-9]+ leak/ { baseline = $3 }
    /^STACK OF /                   { flush_block(); header = $0; nframes = 0; next }
    header && /^====/              { flush_block(); next }
    header && /^[0-9]+[ \t]/ {
      frame_index[nframes] = $1 + 0
      frame_image[nframes] = $2
      frame_text[nframes] = $0
      nframes++
      next
    }
    END {
      flush_block()
      printf "   baseline (all leaks reported by leaks): %s\n", (baseline == "" ? "unknown" : baseline)
      printf "   attributed to %s: %d\n", image, hits
      exit (hits > 0) ? 1 : 0
    }
  '
  if [[ $? -ne 0 ]]; then
    status=1
  fi
done

exit $status

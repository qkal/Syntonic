#!/bin/bash
#
# Drive a twin through the state checklist of docs/twin-comparison.md and
# capture every state (R22).
#
# Usage:
#   scripts/capture-states.sh [swift|c] [light|dark]
#
# Captures land in build/compare/<twin>/<appearance>/<state>.png, each window
# state beside a <state>.layout.txt with the frames and layout priorities the
# app reports. States that show an open menu, and the state with no window
# left, are captured from the whole display: screencapture -l needs a window.
#
# Two TCC grants are required and a headless run does not have them:
#   - Accessibility, so System Events may post keys and click controls
#   - Screen Recording, so screencapture may photograph another app's window
# Both are granted in System Settings ▸ Privacy & Security to the program that
# runs this script (Terminal, iTerm, or whatever shell host you use). Without
# them this script stops with a named message instead of writing black images.

set -euo pipefail

twin=${1:-swift}
appearance=${2:-light}

case "$twin" in swift | c) ;; *) echo "capture-states: twin must be 'swift' or 'c'" >&2; exit 2 ;; esac
case "$appearance" in light | dark) ;; *) echo "capture-states: appearance must be 'light' or 'dark'" >&2; exit 2 ;; esac

repo=$(cd "$(dirname "$0")/.." && pwd)
app="$repo/build/twin-$twin/Syntonic Twin.app"
out="$repo/build/compare/$twin/$appearance"
log="$out/.app.log"
state="$out/.state.txt"

# The seed list and the fixed geometry, repeated from docs/twin-comparison.md.
seed_rows=12
content_w=1000.0
content_h=640.0
frame_w=1000
frame_h=692
resized_w=820
resized_h=520
sidebar_w=220.0

fail() {
  echo "capture-states: $*" >&2
  exit 1
}

# MARK: - Grants

require_grants() {
  if [[ "$(osascript -e 'tell application "System Events" to get UI elements enabled' 2>/dev/null)" != "true" ]]; then
    fail "SYNTONIC_CAPTURE_NO_ACCESSIBILITY: System Events may not drive the UI.
  Grant Accessibility to this shell's host app in
  System Settings ▸ Privacy & Security ▸ Accessibility, then run this again."
  fi
  local probe="$out/.probe.png"
  if ! screencapture -x -D 1 "$probe" 2>/dev/null || [[ ! -s "$probe" ]]; then
    rm -f "$probe"
    fail "SYNTONIC_CAPTURE_NO_SCREEN_RECORDING: screencapture cannot read the display.
  Grant Screen Recording to this shell's host app in
  System Settings ▸ Privacy & Security ▸ Screen Recording, then run this again."
  fi
  rm -f "$probe"
}

# MARK: - Talking to the twin

# Run an AppleScript body with `proc` bound to the twin's process.
sys() {
  osascript <<APPLESCRIPT
tell application "System Events"
  set proc to first application process whose unix id is $pid
  tell proc
$1
  end tell
end tell
APPLESCRIPT
}

# The settling handshake: every state waits for a line the app itself reports,
# never for a fixed sleep, so no capture records an animation mid-flight.
wait_for() { # <extended regex> <what>
  local tries=0
  until grep -Eq "$1" "$state" 2>/dev/null; do
    tries=$((tries + 1))
    if ((tries > 100)); then
      fail "SYNTONIC_CAPTURE_TIMEOUT: $2 (10s without '$1' in the layout report)"
    fi
    sleep 0.1
  done
}

wait_ax() { # <applescript expression> <expected> <what>
  local tries=0
  until [[ "$(sys "$1" 2>/dev/null || true)" == "$2" ]]; do
    tries=$((tries + 1))
    if ((tries > 100)); then
      fail "SYNTONIC_CAPTURE_TIMEOUT: $3 (10s without '$2')"
    fi
    sleep 0.1
  done
}

front() {
  sys "set frontmost to true"
  wait_ax "get frontmost" "true" "the twin never came to the front"
}

key() { # <key code> [modifier list]
  front
  if [[ $# -gt 1 ]]; then sys "key code $1 using {$2}"; else sys "key code $1"; fi
}

shortcut() { # <character> <modifier list>
  front
  sys "keystroke \"$1\" using {$2}"
}

type_text() {
  front
  sys "keystroke \"$1\""
}

# MARK: - Clicking by the app's own layout report

# Turn a point in the window's content coordinates (origin bottom left) into a
# screen point and click it.
click_content() { # <x> <y>
  front
  local geometry px py ph
  geometry=$(sys "get {position of window 1, size of window 1}")
  IFS=', ' read -r px py _ ph <<<"$geometry"
  local chrome=$((ph - ${content_h%.*}))
  local sx sy
  sx=$(awk -v p="$px" -v x="$1" 'BEGIN { printf "%.0f", p + x }')
  sy=$(awk -v p="$py" -v c="$chrome" -v h="$content_h" -v y="$2" 'BEGIN { printf "%.0f", p + c + (h - y) }')
  sys "click at {$sx, $sy}"
}

click_control() { # <layout report key, e.g. detail.title>
  local centre
  centre=$(awk -F'[=,]' -v k="$1.frame" '$1 == k { printf "%.0f %.0f\n", $2 + $4 / 2, $3 + $5 / 2; found = 1 }
                                         END { if (!found) exit 1 }' "$state") ||
    fail "SYNTONIC_CAPTURE_STATE: the layout report has no frame for '$1'"
  click_content ${centre}
}

# Clicking below the last row is what empties the table's selection.
click_below_rows() {
  local point
  point=$(awk -F'[=,]' '$1 == "list.frame" { printf "%.0f %.0f\n", $2 + $4 / 2, $3 + 40; found = 1 }
                        END { if (!found) exit 1 }' "$state") ||
    fail "SYNTONIC_CAPTURE_STATE: the layout report has no frame for the list pane"
  click_content ${point}
}

# MARK: - Captures

window_id() { # <window title as the twin logged it>
  awk -v want="$1" '$1 == "SYNTONIC_TWIN_WINDOW" && substr($0, index($0, $3)) == want { id = $2 } END { print id }' "$log"
}

verify_png() {
  [[ -s "$1" ]] || fail "SYNTONIC_CAPTURE_BLOCKED: $1 is empty; the Screen Recording grant is missing."
  local width
  width=$(sips -g pixelWidth "$1" 2>/dev/null | awk '/pixelWidth/ { print $2 }')
  [[ -n "$width" && "$width" -gt 0 ]] || fail "SYNTONIC_CAPTURE_BLOCKED: $1 holds no image."
}

capture_window() { # <state> [window title]
  local id
  id=$(window_id "${2:-Syntonic Twin}")
  [[ -n "$id" ]] || fail "SYNTONIC_CAPTURE_STATE: the twin never reported a window titled '${2:-Syntonic Twin}'"
  screencapture -x -o -l"$id" "$out/$1.png" ||
    fail "SYNTONIC_CAPTURE_NO_SCREEN_RECORDING: screencapture could not read window $id."
  verify_png "$out/$1.png"
  cp "$state" "$out/$1.layout.txt"
  echo "captured $1"
}

capture_display() { # <state>
  screencapture -x -D 1 "$out/$1.png" ||
    fail "SYNTONIC_CAPTURE_NO_SCREEN_RECORDING: screencapture could not read the display."
  verify_png "$out/$1.png"
  echo "captured $1 (whole display)"
}

capture_menu() { # <state> <menu bar item specifier>
  front
  sys "click $2 of menu bar 1"
  wait_ax "get value of attribute \"AXSelected\" of $2 of menu bar 1" "true" "the $1 menu never opened"
  capture_display "$1"
  key 53 # escape
}

# MARK: - Run

[[ -d "$app" ]] || fail "no bundle at $app; run 'just twin-$twin-build' first."
rm -rf "$out"
mkdir -p "$out"
require_grants

exe=$(plutil -extract CFBundleExecutable raw "$app/Contents/Info.plist")
open -n --stdout "$log" --stderr "$log" \
  --env SYNTONIC_TWIN_APPEARANCE="$appearance" \
  --env SYNTONIC_TWIN_DUMP="$state" \
  "$app"

for _ in $(seq 100); do
  pid=$(pgrep -n -x "$exe" || true)
  [[ -n "${pid:-}" ]] && break
  sleep 0.1
done
[[ -n "${pid:-}" ]] || fail "SYNTONIC_CAPTURE_LAUNCH: $exe never appeared in the process list."
trap 'kill "$pid" 2>/dev/null || true' EXIT

# launch
wait_for "^window\.content=$content_w,$content_h$" "the window never reached its fixed size"
wait_for "^list\.rows=$seed_rows$" "the seed list never loaded"
wait_for "^list\.selectedRow=-1$" "the list started with a selection"
front
capture_window launch

# sidebar collapsed / expanded
# Collapsing hides the sidebar's view rather than zeroing its frame, so both
# twins still report sidebar.width=220.0 here; the collapsed flag is the line
# that moves, and list.frame sliding to x=0 is what the capture shows.
shortcut s "command down, control down"
wait_for "^sidebar\.collapsed=true$" "the sidebar never collapsed"
capture_window sidebar-collapsed
shortcut s "command down, control down"
wait_for "^sidebar\.collapsed=false$" "the sidebar never came back"
wait_for "^sidebar\.width=$sidebar_w$" "the sidebar never came back to its width"
capture_window sidebar-expanded

# search filtering
shortcut f "command down"
type_text lan
wait_for "^list\.rows=2$" "the search never filtered the list"
capture_window search-filtering
shortcut a "command down"
key 51 # delete
wait_for "^list\.rows=$seed_rows$" "the search filter never cleared"

# no selection, then row selection
click_below_rows
wait_for "^list\.selectedRow=-1$" "the list kept its selection"
wait_for "^detail\.delete\.enabled=false$" "Delete stayed enabled with no selection"
capture_window no-selection
key 125 # down arrow
wait_for "^list\.selectedRow=0$" "the first row never got selected"
capture_window row-selection

# detail edit: type into the title field and tab out to commit
click_control detail.title
shortcut a "command down"
type_text "Aperture Sync Revised"
key 48 # tab
wait_for "^detail\.title\.value=Aperture Sync Revised$" "the title edit never committed"
capture_window detail-edit

# add a row
shortcut n "command down"
wait_for "^list\.rows=$((seed_rows + 1))$" "Add never inserted a row"
wait_for "^detail\.title\.value=New Item$" "Add never selected the new row"
capture_window add-row

# delete a row
click_control detail.delete
wait_for "^list\.rows=$seed_rows$" "Delete never removed the row"
wait_for "^detail\.delete\.enabled=false$" "the pane never fell back to its placeholder"
capture_window delete-row

# resized window
sys "set size of window 1 to {$resized_w, $resized_h}"
wait_for "^window\.frame=[-0-9.]+,[-0-9.]+,$resized_w\.0,$resized_h\.0$" "the window never resized"
capture_window window-resized
sys "set size of window 1 to {$frame_w, $frame_h}"
wait_for "^window\.frame=[-0-9.]+,[-0-9.]+,$frame_w\.0,$frame_h\.0$" "the window never went back to its fixed size"

# settings tabs
shortcut , "command down"
wait_ax "get title of window 1" "General" "the Settings window never opened"
capture_window settings-general Settings
for tab in Appearance Advanced; do
  sys "tell window 1 to click (first radio button of (entire contents of toolbar 1) whose name is \"$tab\")"
  wait_ax "get title of window 1" "$tab" "the $tab tab never came forward"
  capture_window "settings-$(echo "$tab" | tr '[:upper:]' '[:lower:]')" Settings
done
shortcut w "command down"
wait_ax "get title of window 1" "Syntonic Twin" "the Settings window never closed"

# open menus, captured from the display: a menu is not inside the window
capture_menu menu-app "menu bar item 2"
capture_menu menu-file "menu bar item \"File\""
capture_menu menu-edit "menu bar item \"Edit\""
capture_menu menu-view "menu bar item \"View\""
capture_menu menu-window "menu bar item \"Window\""
capture_menu menu-help "menu bar item \"Help\""

# last window closed: the app keeps running with its menu bar (F1)
shortcut w "command down"
wait_ax "count of windows" "0" "the last window never closed"
kill -0 "$pid" || fail "SYNTONIC_CAPTURE_LIFECYCLE: the twin quit when its last window closed."
capture_display last-window-closed

shortcut q "command down"
for _ in $(seq 50); do
  kill -0 "$pid" 2>/dev/null || break
  sleep 0.1
done
trap - EXIT
kill "$pid" 2>/dev/null || true
rm -f "$state"

echo "capture-states: $twin/$appearance captured into $out"

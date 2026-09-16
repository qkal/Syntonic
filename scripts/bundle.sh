#!/bin/bash
#
# Turn any built executable into a signed .app (R20, KTD11, AE7).
#
# Needs only the Command Line Tools: iconutil and codesign ship with them, so
# this works on a Mac with no Xcode.app. The bare executable still runs
# directly during development; this is only for shipping and for Finder.
#
# Usage:
#   scripts/bundle.sh <executable> <app name> <bundle id> [icon png dir] [signing identity]
#
# The icon directory holds a PNG set named the way iconutil expects
# (icon_16x16.png, icon_16x16@2x.png, ... icon_512x512@2x.png).
# With no signing identity the bundle is signed ad hoc, which is enough to
# launch locally; pass a Developer ID identity to sign for distribution.
#
# The .app is written next to the executable. Override with SYNTONIC_BUNDLE_DIR.

set -euo pipefail

if [[ $# -lt 3 ]]; then
  sed -n '3,20p' "$0" >&2
  exit 2
fi

executable=$1
app_name=$2
bundle_id=$3
icon_dir=${4:-}
identity=${5:-}
version=${SYNTONIC_BUNDLE_VERSION:-0.0.0}

script_dir=$(cd "$(dirname "$0")" && pwd)
template="$script_dir/Info.plist.in"

if [[ ! -x "$executable" ]]; then
  echo "bundle: '$executable' is not an executable." >&2
  exit 1
fi
if [[ ! -f "$template" ]]; then
  echo "bundle: missing $template" >&2
  exit 1
fi

out_dir=${SYNTONIC_BUNDLE_DIR:-$(cd "$(dirname "$executable")" && pwd)}
app="$out_dir/$app_name.app"
exec_name=$(basename "$executable")

rm -rf "$app"
mkdir -p "$app/Contents/MacOS" "$app/Contents/Resources"
cp "$executable" "$app/Contents/MacOS/$exec_name"
printf 'APPL????' > "$app/Contents/PkgInfo"

sed -e "s|@EXECUTABLE@|$exec_name|g" \
    -e "s|@IDENTIFIER@|$bundle_id|g" \
    -e "s|@NAME@|$app_name|g" \
    -e "s|@VERSION@|$version|g" \
    "$template" > "$app/Contents/Info.plist"
plutil -lint "$app/Contents/Info.plist" > /dev/null

if [[ -n "$icon_dir" ]]; then
  if [[ ! -f "$icon_dir/icon_512x512@2x.png" ]]; then
    echo "bundle: '$icon_dir' has no icon_512x512@2x.png; not an iconutil PNG set." >&2
    exit 1
  fi
  iconset=$(mktemp -d)/AppIcon.iconset
  mkdir -p "$iconset"
  cp "$icon_dir"/icon_*.png "$iconset/"
  iconutil --convert icns --output "$app/Contents/Resources/AppIcon.icns" "$iconset"
  rm -rf "$(dirname "$iconset")"
fi

if [[ -n "$identity" ]]; then
  codesign --force --timestamp --options runtime --sign "$identity" "$app"
else
  # Ad-hoc: launches locally, is not distributable.
  codesign --force --sign - "$app"
fi
codesign --verify --strict --verbose=2 "$app"

echo "bundle: $app"

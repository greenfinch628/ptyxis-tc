#!/bin/sh
# Install the custom variants and the original upstream icons in a hicolor theme.
# Pass a share directory, optionally including a package staging root.
set -eu
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 SHARE_DIRECTORY" >&2
    exit 2
fi
icon_source=$(CDPATH= cd "$(dirname "$0")/../data/icons" && pwd)
scalable="$1/icons/hicolor/scalable/apps"
symbolic="$1/icons/hicolor/symbolic/apps"
install -d "$scalable" "$symbolic"
for icon in "$icon_source"/ptyxis-tc-*.svg; do
    install -m644 "$icon" "$scalable/"
done
# Keep the upstream artwork available under the existing application icon names.
install -m644 "$icon_source/ptyxis.svg" "$scalable/org.gnome.Ptyxis.TabColors.svg"
install -m644 "$icon_source/ptyxis-symbolic.svg" "$symbolic/org.gnome.Ptyxis.TabColors-symbolic.svg"

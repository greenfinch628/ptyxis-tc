# Ptyxis-TC

Unofficial modification of Ptyxis 50.1 by Bob Czys, October 5, 2026.
Upstream: https://gitlab.gnome.org/chergert/ptyxis

## Features

- Automatic tab background colors based on hostnames and SSH aliases.
- Manual tab colors through the tab context menu.
- Manual colors retained during session restoration.
- Right-click copies selected text; without a selection, it pastes.
- Shift+right-click opens the original terminal context menu.

## License

GPL version 3 or later; see ../COPYING.
Original copyright and license notices are retained.

## Build on Fedora

Install dependencies:

    sudo dnf install meson ninja-build gcc gettext-devel glib2-devel gtk4-devel libadwaita-devel vte291-gtk4-devel json-glib-devel libportal-devel libportal-gtk4-devel desktop-file-utils

Run these commands from the repository directory:

    meson setup build-tc --prefix=/opt/ptyxis-tab-colors-50.1 --buildtype=debugoptimized -Dapp-id=org.gnome.Ptyxis.TabColors -Dgschema-id=org.gnome.Ptyxis.TabColors -Dgschema-path=/org/gnome/Ptyxis/TabColors/
    meson compile -C build-tc
    meson test -C build-tc --print-errorlogs

## Installation

Close Ptyxis-TC before installing or updating:

    sudo meson install -C build-tc
    sudo install -m 755 custom/ptyxis-tab-colors /opt/ptyxis-tab-colors-50.1/bin/ptyxis-tab-colors
    mkdir -p ~/.local/share/applications
    install -m 644 custom/org.gnome.Ptyxis.TabColors.desktop ~/.local/share/applications/
    mkdir -p ~/.config/org.gnome.Ptyxis.TabColors

Copy custom/tab-colors.ini into ~/.config/org.gnome.Ptyxis.TabColors/
if that file does not already exist. Edit it to match your hosts and aliases.

Launch Ptyxis-TC from the application menu or run:

    /opt/ptyxis-tab-colors-50.1/bin/ptyxis-tab-colors

The custom app uses separate settings from Fedora's regular Ptyxis.

## Compatibility and validation

Originally tested on Fedora 44 x86_64 with GTK 4.22.5 and libadwaita 1.9.4.
Tab styling uses libadwaita internals and deprecated GTK style APIs;
other versions require verification.

See ARCHIVE-NOTES.md for detailed behavior and original validation.
Its archive installation instructions do not apply to this Git checkout.

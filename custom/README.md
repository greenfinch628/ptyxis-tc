# Ptyxis-TC

Unofficial modification by Bob Czys, October 5, 2026.
Started on Ptyxis 50.1; main includes newer upstream history and currently
reports version 50.2. The installation prefix retains its original name.
Upstream: https://gitlab.gnome.org/chergert/ptyxis

## Features

- Automatic tab background colors based on hostnames and SSH aliases.
- Manual tab colors through the tab context menu.
- Manual colors retained during session restoration.
- Right-click copies selected text; without a selection, it pastes.
- Enable or disable right-click copy/paste in Preferences → Behavior → Mouse.
  It is enabled by default; middle-click in the terminal text area opens the context menu
  instead of pasting the primary selection while enabled.
  Disabling it restores normal right-click context menu and middle-click behavior.
  Left-button text selection, dragging, and highlighting use normal mouse behavior.
  The preference is saved across restarts.
- Shift+right-click opens the original terminal context menu.

## License

GPL version 3 or later; see ../COPYING.
Original copyright and license notices are retained.

## Build on Fedora

Install dependencies:

    sudo dnf install meson ninja-build gcc gettext-devel glib2-devel gtk4-devel libadwaita-devel vte291-gtk4-devel json-glib-devel libportal-devel libportal-gtk4-devel desktop-file-utils

Clone the custom repository:

    git clone git@github.com:greenfinch628/ptyxis-tc.git
    cd ptyxis-tc

Requires Meson >= 1.0, GLib >= 2.80, GTK >= 4.14, libadwaita >= 1.8,
JSON-GLib >= 1.6, and VTE GTK4 >= 0.79.

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

The Meson test command runs the upstream test suite. The custom test C files
and helper scripts are archived validation fixtures; the helper scripts expect
the original archive layout (`build/`, `extension/`, and `link-command.txt`)
and are not directly runnable from this checkout.

See ARCHIVE-NOTES.md for detailed behavior and original validation.
Its archive installation instructions do not apply to this Git checkout.

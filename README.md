# Ptyxis-TC

An unofficial [Ptyxis](https://gitlab.gnome.org/chergert/ptyxis) fork with
hostname-based tab colors, manual tab colors, and right-click copy/paste.

![Ptyxis-TC showing blue and green tab backgrounds](custom/screenshots/tab-colors.png)

## Features

- Automatic tab background colors based on hostnames and SSH aliases.
- Manual tab colors through the tab context menu.
- Manual colors retained during session restoration.
- Right-click copies selected text and clears the selection; with no selection, it pastes.
- Shift+right-click opens the original terminal context menu.
- Separate application ID and settings, so it can coexist with Fedora's regular Ptyxis.

Custom changes started on Ptyxis 50.1. The `main` branch preserves both the
custom changes and newer upstream history, currently reporting version 50.2.

## Build on Fedora

Clone the repository:

```bash
git clone git@github.com:greenfinch628/ptyxis-tc.git
cd ptyxis-tc
```

Install build dependencies:

```bash
sudo dnf install meson ninja-build gcc gettext-devel glib2-devel gtk4-devel \
  libadwaita-devel vte291-gtk4-devel json-glib-devel libportal-devel \
  libportal-gtk4-devel desktop-file-utils
```

Configure, build, and run the upstream checks:

```bash
meson setup build-tc --prefix=/opt/ptyxis-tab-colors-50.1 \
  --buildtype=debugoptimized \
  -Dapp-id=org.gnome.Ptyxis.TabColors \
  -Dgschema-id=org.gnome.Ptyxis.TabColors \
  -Dgschema-path=/org/gnome/Ptyxis/TabColors/
meson compile -C build-tc
meson test -C build-tc --print-errorlogs
```

The installation prefix retains its original `50.1` name for compatibility
with existing launchers. See [custom/README.md](custom/README.md) for source
installation, dependency versions, configuration, and compatibility details.

## Local RPM on Scopuli

The locally built Fedora 44 x86_64 package is stored under `local-rpm/`.
RPM artifacts and build inputs in that directory are excluded from Git and
are not included in a fresh clone.

```bash
sudo dnf install ~/ptyxis-tc/local-rpm/RPMS/x86_64/ptyxis-tc-50.2-1.fc44.x86_64.rpm
```

Close and reopen Ptyxis-TC after upgrading. Launch it from the application
menu or run `ptyxis-tc` (also available as `ptyxis-tab-colors`).
The local `local-rpm/README.md` contains RPM rebuild instructions.

## Hostname colors

Create or edit `~/.config/org.gnome.Ptyxis.TabColors/tab-colors.ini`.
Use [custom/tab-colors.ini](custom/tab-colors.ini) as an example and replace
its hosts and aliases with your own. Changes are read automatically.

This source reads the user configuration file; the legacy RPM configuration
at `/etc/ptyxis-tc/tab-colors.ini` is not read. Copy any desired mappings from
that file into your user configuration.

## Validation and compatibility

The current source built successfully on Scopuli (Fedora 44 x86_64) with
GTK 4.22.5, libadwaita 1.9.4, and VTE 0.84.1. All four Meson checks and the
custom tab-color integration test passed. The local RPM also passed digest
verification and an upgrade dry run.

Tab styling uses libadwaita internals and deprecated GTK style APIs, so other
library versions require verification. See
[custom/ARCHIVE-NOTES.md](custom/ARCHIVE-NOTES.md) for the original build and
validation history.

## License and attribution

Licensed under GPL version 3 or later; see [COPYING](COPYING).
Original copyright and license notices are retained.

Ptyxis is developed upstream by Christian Hergert and contributors.
Modified by Bob Czys, October 5, 2026.
The original upstream README is preserved in [README-upstream.md](README-upstream.md).

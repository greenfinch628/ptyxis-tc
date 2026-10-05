# Ptyxis 50.1 with tab background colors

Built on scopuli-dr1 (Fedora 44 x86_64) for scopuli, whose GTK, VTE,
libadwaita and glibc versions were verified to match. Upstream source:
https://download.gnome.org/sources/ptyxis/50/ptyxis-50.1.tar.xz
SHA256: 73f4b76480644b2840415859a51d20cd7487b0619714f8defcd38212c3ddcffe

## Install on scopuli

Extract the archive, enter its directory, and run:

```sh
sha256sum -c MANIFEST.sha256
./install.sh
~/.local/bin/ptyxis-tab-colors
```

The installer requires sudo only for the custom `/opt/ptyxis-tab-colors-50.1`
prefix. It refuses an existing custom prefix. The Fedora Ptyxis package,
`/usr/bin/ptyxis`, its desktop entry, profiles and settings are retained.
The new app uses `org.gnome.Ptyxis.TabColors`, its own settings path and
session data, and appears as **Ptyxis Tab Colors**. Existing profiles are
not imported automatically. Installer retains an existing tab-colors.ini.
The bundle uses system libraries; it is not a static or cross-distro build.

## Coloring

Edit `~/.config/org.gnome.Ptyxis.TabColors/tab-colors.ini` (or the same path
under XDG_CONFIG_HOME). The included colors are editable defaults for your
SSH hosts. Changes are read every 500 ms while the window is visible.

`[Hosts]` maps lower-case exact hostnames to GTK colors such as `#3584e4`.
Matching tries the full hostname, then the short hostname, then an optional
`default` key. Unmatched hosts keep the normal theme background. Invalid
colors also leave the theme background. `[Aliases]` maps SSH names like
`s` and `dr1` to hostnames. Add aliases here when your SSH configuration changes.

The app uses the hostname from the shell's OSC 7 working-directory URI.
For foreground `ssh` commands, it falls back to parsing the destination
and the configured aliases, so ordinary `ssh s` works without remote shell
integration. OSC 7 permits hostname changes inside nested SSH sessions.
No title-string guessing, DNS requests, or external commands are used.
For reliable nested sessions and local resets, shells should emit OSC 7 at
prompts. An optional Bash example (add on each relevant host):

```bash
ptyxis_osc7() { printf '\033]7;file://%s%s\033\\' "$HOSTNAME" "$PWD"; }
PROMPT_COMMAND+=(ptyxis_osc7)
```

This simple example assumes a URI-safe working-directory path. Use your
existing shell integration when it already emits OSC 7. Remote programs
that do not emit OSC 7 and cannot be observed as foreground SSH commands
need a manual override.

Right-click a tab and choose **Tab Background Color…** to pick any opaque
color. **Automatic Tab Color** clears that tab's override. Overrides belong
to the tab, survive reordering and moving it to another window, and are saved with the tab when session restoration is enabled. Restored
tabs retain their manual overrides across application restarts. The color
picker starts with the tab's current manual or automatic color.
Foreground text switches between white and dark for contrast, and the
selected tab has an underline. Terminal content/palette is unchanged.

## Right-click clipboard

In the terminal content area, right-click copies selected text to the standard
clipboard and clears the selection. With no selection, right-click pastes the
clipboard into the terminal. Copied text stays available for other applications.
Shift+right-click opens the original terminal context menu. Tab context menus
are unchanged. This behavior also applies inside programs using mouse input;
hold Shift to bypass the new clipboard shortcut.

## Build and verification

The archive contains the complete installed payload, modified source,
original source tarball, patch, original Meson build tree, extension and
integration test source, build logs and a rendered test preview. Build tree
paths refer to scopuli-dr1; use `./rebuild.sh /absolute/new/build-directory`
for a fresh build elsewhere.

Fedora dependencies:

```sh
sudo dnf install meson ninja-build gcc gettext-devel glib2-devel gtk4-devel \
  libadwaita-devel vte291-gtk4-devel json-glib-devel libportal-devel \
  libportal-gtk4-devel desktop-file-utils
```

The tab-bar implementation introspects libadwaita's internal `page`
property. This was tested with libadwaita 1.9.4; recheck the integration test
after major GTK/libadwaita changes. GTK style-context APIs used for per-tab
CSS are deprecated but supported by the tested GTK 4.22.5.

Headless integration test (requires xorg-x11-server-Xvfb):

```sh
cc -Wall -Wextra -Wno-deprecated-declarations -Wno-unused-parameter \
  -Wno-unused-function extension/test-tab-colors.c -o test-tab-colors \
  $(pkg-config --cflags --libs gtk4 libadwaita-1 vte-2.91-gtk4)
GTK_A11Y=none GSK_RENDERER=cairo dbus-run-session -- xvfb-run -a ./test-tab-colors
```

See `validation/` for results. These verify automatic mapping, SSH aliases,
manual overrides, real AdwTab CSS attachment, restoring automatic mode,
clearing unmapped colors, and the four upstream tests. A separate full-app
smoke test executes a shell command through the installed terminal agent.

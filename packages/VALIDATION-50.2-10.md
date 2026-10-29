# Fedora 44 — Ptyxis-TC 50.2-10

Installed on Scopuli on October 29, 2026. Other hosts and Ubuntu packages
are unchanged. Application version remains 50.2; Fedora package revision is 10.
Source commit: 2e87464. The source archive excludes prior binary release artifacts.

## Rejected-drop handling

GTK 4.22.5's Wayland data_source_cancelled callback emits GDK_DRAG_CANCEL_ERROR,
including rejection by another application. The bundled libadwaita previously
reattached this tab to its original window. The private patch now sends ERROR
on a Wayland display through its existing detach/create-window path.
NO_TARGET continues to detach. Explicit USER_CANCELLED and errors on other
backends retain their original restore behavior. Accepted transfers are unchanged.

Wayland uses the same generic error for compositor cancellation, including Escape.
The user explicitly approved prioritizing a new window for rejected drops over
preserving Escape cancellation on this ambiguous path. No compositor or system
GTK/libadwaita changes are installed.

GTK source reference:
https://github.com/GNOME/gtk/blob/4.22.5/gdk/wayland/gdkdrag-wayland.c

## Automated validation

- Pinned dependency patch applied in a fresh RPM build; application and private
  library build succeeded. All four Meson checks passed in both builds.
- Callback dispatch test compiles the actual patched handler: rejected Wayland
  drop detaches; explicit cancellation restores; X11 errors restore; no-target
  detaches; teardown executes once; also compiles without Wayland support.
- Native window transfer integration passed: single/multiple tabs, insertion,
  reordering, reattachment and empty-window cleanup. Same live shell/process,
  PTY, history, profile, manual/automatic colors and heartbeat retained.
- Appearance, session/color persistence, native chooser and backed-up reset
  regression fixtures passed on an isolated X11 virtual display.
- Right-click clipboard copy, selection clearing and subsequent PTY paste passed.
- RPM digest and upgrade dry run passed; installed 50.2-10 locally.
- Settings, config/session file fingerprints and system libadwaita SHA-256 were
  identical before/after installation. Private libadwaita resolves via RUNPATH.
- Packaged files verify; only the generated ghost schema-cache mode differs.

## Interactive validation

A separate instance titled “Drop test 50.2-10” runs the installed package on
Wayland with isolated config/state. Physical rejected-drop verification is
pending the user's result. Existing terminal processes have not been closed.

## Artifacts

- fedora-44/ptyxis-tc-50.2-10.fc44.x86_64.rpm
- source/ptyxis-tc-50.2-10.fc44.src.rpm
- source/ptyxis-tc-50.2-10-source.tar.gz

Checksums are in SHA256SUMS. Packages are unsigned local builds.

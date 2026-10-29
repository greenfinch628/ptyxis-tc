# Fedora 44 — Ptyxis-TC 50.2-9

Installed on Scopuli on October 28, 2026. Other hosts and Ubuntu packages are
unchanged. The upstream app version remains 50.2; the Fedora package revision is 9.

## Native-gesture diagnosis

User mouse testing established that multi-tab detachment works when dropped on
empty desktop space. Dropping back into the same terminal view is accepted as an
intra-window move by the upstream library. Debugger observation confirmed GTK’s
native drag entry point, libadwaita’s detach handler and the app’s create-window
callback on the successful outside-window drop.

The installed libadwaita 1.9.4 explicitly blocks starting native tab DnD with only
one page. Making the one-tab bar visible in revision 8 did not remove that separate
condition. Revision 9 builds the official pinned source privately and removes only
that condition. Existing drag thresholds, cancellation and insertion remain native.
Pinned-tab and standalone-command behavior remain upstream-compatible.

The private library installs beneath the application’s /opt prefix. A relative
RUNPATH selects it for Ptyxis-TC only. Fedora’s /usr/lib64/libadwaita is unchanged,
verified by before/after SHA-256 comparison. The pinned archive, wrap hash, patch,
license and provenance are tracked in subprojects/ and vendor/.

Empty-window destruction is deferred until the library finishes its global
is-transferring-page notification iteration, avoiding reentrant list destruction.

## Automated checks

- Private library and application build succeeded; all four Meson checks passed.
- Actual Ptyxis windows/native API transfers: detach/create window, reattach,
  one/multiple-tab destinations, insertion order, reorder and empty-window cleanup.
- Same live process proxy, PTY, terminal object and retained terminal text across
  transfers; the running shell heartbeat continues.
- Arthur profile identity/label, manual active/inactive colors and hostname-derived
  colors retained.
- Existing right-click clipboard and session-color persistence fixtures passed.
- Xvfb tests explicitly force GDK_BACKEND=x11 and temporary settings/config/data.
- RPM installed with unchanged user settings/profile and tab-color fingerprints.
- Packaged non-ghost files verify; generated schema-cache metadata is excluded.
- Loader resolves the application-private libadwaita; installed terminal-command
  smoke test passes in an isolated display/session.

## Interactive verification

The user confirmed successful physical single-tab reattachment in the installed
revision 9 on Scopuli’s Wayland desktop. Earlier multi-tab outside-window
detachment was also confirmed during the diagnostic session. Automated checks
cover order, terminal continuity, profiles/colors and empty-window cleanup.

## Release files

- fedora-44/ptyxis-tc-50.2-9.fc44.x86_64.rpm
- source/ptyxis-tc-50.2-9.fc44.src.rpm
- source/ptyxis-tc-50.2-9-source.tar.gz

All checksums are listed in SHA256SUMS. Source includes the pinned dependency
archive and patch, and excludes prior binary release artifacts.

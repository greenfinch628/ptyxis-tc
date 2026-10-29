# Private libadwaita

Official source: https://download.gnome.org/sources/libadwaita/1.9/libadwaita-1.9.4.tar.xz
SHA-256: f5ab3453ee743dc293cdf96d6821789cd20e8d125d9b41637703c12bfa4162ee
License: LGPL-2.1-or-later; see libadwaita-COPYING and the original archive.

The checked-in archive is in subprojects/packagecache so builds need no download.
Meson applies subprojects/packagefiles/libadwaita-single-tab-drag.patch: remove
the native reorder handler's n-pages > 1 restriction and detach on Wayland
GDK_DRAG_CANCEL_ERROR, used for rejected drops. Wayland compositor cancellation
(including Escape) can report the same error and therefore also detach; this
tradeoff is intentional. Explicit USER_CANCELLED and non-Wayland errors retain
the original restore behavior. The existing library handles drag icons, data
transfer and ordered insertion.
Its drag-distance threshold and pinned-tab behavior are unchanged.

Library and development files install only below the application's private /opt
prefix, in a ptyxis-tc subdirectory. The executable's RUNPATH selects that copy.
No system libadwaita files or configuration are replaced. This pinned dependency
needs maintenance alongside GTK/libadwaita security and compatibility updates.

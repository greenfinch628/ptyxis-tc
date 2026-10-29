#!/usr/bin/env python3
"""Exercise the bundled cancellation handler's dispatch without a live drag.

Compile the actual patched callback with stand-ins for window/drag teardown.
Native window/session continuity is covered by test-tab-transfer.c.
"""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = root / 'subprojects/libadwaita-1.9.4/src/adw-tab-box.c'
callback = re.search(r'static void\ntab_drag_cancel_cb .*?\n}\n', source.read_text(), re.S).group()
preamble = r'''
#include <glib.h>
typedef enum { GDK_DRAG_CANCEL_NO_TARGET, GDK_DRAG_CANCEL_USER_CANCELLED,
               GDK_DRAG_CANCEL_ERROR } GdkDragCancelReason;
typedef struct { gboolean should_detach_into_new_window; int windows; int ends; gboolean success; } AdwTabBox;
typedef struct { gboolean wayland; } GdkDrag;
#define gdk_drag_get_display(drag) (drag)
#define GDK_IS_WAYLAND_DISPLAY(display) ((display)->wayland)
static void detach_into_new_window (AdwTabBox *box) { box->windows++; box->should_detach_into_new_window = FALSE; }
static void drag_end (AdwTabBox *box, GdkDrag *drag, gboolean success) { box->ends++; box->success = success; }
'''
tests = r'''
int main (void) {
  for (int wayland = 0; wayland <= 1; wayland++) {
    for (int reason = GDK_DRAG_CANCEL_NO_TARGET; reason <= GDK_DRAG_CANCEL_ERROR; reason++) {
      AdwTabBox box = { TRUE, 0, 0, FALSE };
      GdkDrag drag = { wayland };
      gboolean expected = reason == GDK_DRAG_CANCEL_NO_TARGET;
#ifdef GDK_WINDOWING_WAYLAND
      expected |= wayland && reason == GDK_DRAG_CANCEL_ERROR;
#endif
      tab_drag_cancel_cb (&box, reason, &drag);
      g_assert_cmpint (box.windows, ==, expected);
      g_assert_cmpint (box.ends, ==, 1);
      g_assert_cmpint (box.success, ==, expected);
      g_assert_false (box.should_detach_into_new_window);
    }
  }
  return 0;
}
'''
flags = subprocess.check_output(['pkg-config', '--cflags', '--libs', 'glib-2.0'], text=True).split()
with tempfile.TemporaryDirectory(prefix='ptyxis-drag-test-') as tmp:
    src = Path(tmp) / 'cancel.c'
    src.write_text(preamble + callback + tests)
    for defines in [[], ['-DGDK_WINDOWING_WAYLAND']]:
        exe = str(Path(tmp) / 'cancel-test')
        subprocess.run(['cc', '-Wall', '-Werror', *defines, str(src), '-o', exe, *flags], check=True)
        subprocess.run([exe], check=True)
print('PASS: no-target detaches; Wayland rejection detaches; explicit user cancel restores; X11 errors restore; builds without Wayland; teardown occurs once.')

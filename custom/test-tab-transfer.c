/* Integration fixture: real Ptyxis windows, profiles, terminal and native transfers.
 * Include window.c to inspect its private widgets without adding production API.
 */
#define main ptyxis_original_main
#include "main.c"
#undef main
#include "ptyxis-window.c"
#include <signal.h>

static void spin (guint milliseconds)
{
  gint64 until = g_get_monotonic_time () + milliseconds * 1000;
  do {
    while (g_main_context_iteration (NULL, FALSE));
    g_usleep (1000);
  } while (g_get_monotonic_time () < until);
}

static gsize file_size (const char *path)
{
  g_autofree char *contents = NULL;
  gsize length = 0;
  g_file_get_contents (path, &contents, &length, NULL);
  return length;
}

int main (int argc, char **argv)
{
  gtk_init ();
  g_autoptr(PtyxisApplication) app = ptyxis_application_new (APP_ID, G_APPLICATION_NON_UNIQUE);
  g_assert_true (g_application_register (G_APPLICATION (app), NULL, NULL));
  g_autoptr(PtyxisProfile) profile = ptyxis_application_dup_default_profile (app);
  g_autoptr(PtyxisProfile) arthur = ptyxis_profile_new (NULL);
  ptyxis_profile_set_label (arthur, "Arthur");
  ptyxis_application_add_profile (app, arthur);
  PtyxisWindow *first = g_object_ref (ptyxis_window_new_empty ());
  PtyxisWindow *second = g_object_ref (ptyxis_window_new_empty ());
  PtyxisTab *moving = ptyxis_tab_new (arthur);
  const char *heartbeat = g_getenv ("PTYXIS_TEST_HEARTBEAT");
  g_assert_nonnull (heartbeat);
  const char *command[] = {"/bin/sh", "-c", "printf 'SESSION_MARKER %s\\n' $$; while :; do printf x >> \"$1\"; sleep 0.1; done", "sh", heartbeat, NULL};
  ptyxis_tab_set_command (moving, command);
  g_object_set_data_full (G_OBJECT (moving), "tab-color-override", g_strdup ("#3584e4"), g_free);
  g_object_set_data_full (G_OBJECT (moving), "tab-color-inactive-override", g_strdup ("#123456"), g_free);
  ptyxis_window_append_tab (first, moving);
  /* This assertion reproduces the one-tab detach/reattach failure on the old code. */
  g_assert_true (gtk_widget_get_visible (GTK_WIDGET (first->tab_bar)));
  PtyxisTab *other = ptyxis_tab_new (profile);
  ptyxis_window_append_tab (first, other);
  ptyxis_window_set_active_tab (first, moving);
  ptyxis_window_append_tab (second, ptyxis_tab_new (profile));
  gtk_window_present (GTK_WINDOW (first));
  gtk_window_present (GTK_WINDOW (second));
  for (guint i = 0; i < 100 && (ptyxis_tab_get_process (moving) == NULL || file_size (heartbeat) < 2); i++) spin (100);
  g_assert_nonnull (ptyxis_tab_get_process (moving));
  g_assert_cmpuint (file_size (heartbeat), >=, 2);
  g_autoptr(PtyxisIpcProcess) process = g_object_ref (ptyxis_tab_get_process (moving));
  VteTerminal *terminal = VTE_TERMINAL (ptyxis_tab_get_terminal (moving));
  VtePty *pty = vte_terminal_get_pty (terminal);
  AdwTabPage *page = adw_tab_view_get_page (first->tab_view, GTK_WIDGET (moving));
  /* Detach using the exact create-window handler and libadwaita transfer path. */
  AdwTabView *detached = ptyxis_window_create_window_cb (first, first->tab_view);
  PtyxisWindow *detached_window = PTYXIS_WINDOW (gtk_widget_get_root (GTK_WIDGET (detached)));
  g_object_ref (detached_window);
  adw_tab_view_transfer_page (first->tab_view, page, detached, 0);
  g_assert_true (gtk_widget_get_visible (GTK_WIDGET (first->tab_bar)));
  g_assert_true (gtk_widget_get_visible (GTK_WIDGET (detached_window->tab_bar)));
  g_assert_true (adw_tab_page_get_child (page) == GTK_WIDGET (moving));
  /* Reattach into a one-tab destination, inserting at the requested position. */
  adw_tab_view_transfer_page (detached, page, first->tab_view, 1);
  spin (50);
  g_assert_false (gtk_widget_get_visible (GTK_WIDGET (detached_window)));
  g_assert_null (g_list_find (gtk_application_get_windows (GTK_APPLICATION (app)), detached_window));
  g_assert_cmpint (adw_tab_view_get_page_position (first->tab_view, page), ==, 1);
  /* Move between existing multi-tab windows and reorder within a window. */
  ptyxis_window_append_tab (second, ptyxis_tab_new (profile));
  adw_tab_view_transfer_page (first->tab_view, page, second->tab_view, 1);
  g_assert_cmpint (adw_tab_view_get_page_position (second->tab_view, page), ==, 1);
  g_assert_true (adw_tab_view_reorder_page (second->tab_view, page, 0));
  g_assert_true (ptyxis_tab_get_profile (moving) == arthur);
  g_autofree char *label = ptyxis_profile_dup_label (ptyxis_tab_get_profile (moving));
  g_assert_cmpstr (label, ==, "Arthur");
  g_assert_cmpstr (g_object_get_data (G_OBJECT (moving), "tab-color-override"), ==, "#3584e4");
  g_assert_cmpstr (g_object_get_data (G_OBJECT (moving), "tab-color-inactive-override"), ==, "#123456");
  g_assert_true (ptyxis_tab_get_process (moving) == process);
  g_assert_true (vte_terminal_get_pty (terminal) == pty);
  gsize before = file_size (heartbeat);
  spin (350);
  g_assert_cmpuint (file_size (heartbeat), >, before);
  g_autofree char *text = vte_terminal_get_text (terminal, NULL, NULL, NULL);
  g_assert_nonnull (strstr (text, "SESSION_MARKER"));
  /* Automatic host color still derives from the same tab after transfers. */
  g_autoptr(GKeyFile) colors = g_key_file_new ();
  g_autofree char *hostname = color_tab_hostname (other, colors);
  g_key_file_set_string (colors, "Hosts", hostname, "#26a269");
  g_autofree char *before_color = color_tab_value (other, colors);
  AdwTabPage *automatic = adw_tab_view_get_page (first->tab_view, GTK_WIDGET (other));
  adw_tab_view_transfer_page (first->tab_view, automatic, second->tab_view, 2);
  g_autofree char *after_color = color_tab_value (other, colors);
  g_assert_cmpstr (before_color, ==, after_color);
  g_assert_cmpstr (after_color, ==, "#26a269");
  spin (50);
  g_assert_false (gtk_widget_get_visible (GTK_WIDGET (first)));
  g_assert_null (g_list_find (gtk_application_get_windows (GTK_APPLICATION (app)), first));
  /* End-of-drag notification must close an empty window which was kept alive
   * during a native drag (including cancelled new-window transfers). */
  PtyxisWindow *empty = g_object_ref (ptyxis_window_new_empty ());
  g_object_notify (G_OBJECT (empty->tab_view), "is-transferring-page");
  spin (50);
  g_assert_null (g_list_find (gtk_application_get_windows (GTK_APPLICATION (app)), empty));
  g_object_unref (empty);
  ptyxis_ipc_process_call_send_signal (process, SIGTERM, NULL, NULL, NULL);
  spin (100);
  gtk_window_destroy (GTK_WINDOW (second));
  g_object_unref (first);
  g_object_unref (second);
  g_object_unref (detached_window);
  g_print ("PASS: single/multi-tab bars; detach/reattach/native ordered transfers; reorder; live process, PTY and history; Arthur profile; manual/automatic colors; empty-window cleanup.\n");
  return 0;
}

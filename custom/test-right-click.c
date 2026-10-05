#define main ptyxis_original_main
#include "main.c"
#undef main
#include "ptyxis-terminal.c"
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

static gboolean clipboard_read_done;
static char *clipboard_text;
static void read_clipboard (GObject *object, GAsyncResult *result, gpointer data)
{
  GError *error = NULL;
  clipboard_text = gdk_clipboard_read_text_finish (GDK_CLIPBOARD (object), result, &error);
  g_assert_no_error (error);
  clipboard_read_done = TRUE;
}
static void pump (void)
{
  while (g_main_context_iteration (NULL, FALSE));
  g_usleep (10000);
}
int main (void)
{
  gtk_init ();
  g_autoptr(PtyxisApplication) app = ptyxis_application_new (APP_ID, G_APPLICATION_NON_UNIQUE);
  g_assert_true (g_application_register (G_APPLICATION (app), NULL, NULL));
  GtkWidget *window = gtk_window_new ();
  PtyxisTerminal *terminal = g_object_new (PTYXIS_TYPE_TERMINAL, NULL);
  GtkWidget *scroller = gtk_scrolled_window_new ();
  gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scroller), GTK_WIDGET (terminal));
  gtk_window_set_child (GTK_WINDOW (window), scroller);
  gtk_window_present (GTK_WINDOW (window));
  vte_terminal_feed (VTE_TERMINAL (terminal), "clipboard sample", -1);
  for (int i = 0; i < 50; i++) pump ();
  vte_terminal_select_all (VTE_TERMINAL (terminal));
  g_assert_true (vte_terminal_get_has_selection (VTE_TERMINAL (terminal)));
  ptyxis_terminal_right_click_clipboard (terminal);
  g_assert_false (vte_terminal_get_has_selection (VTE_TERMINAL (terminal)));
  GdkClipboard *clipboard = gtk_widget_get_clipboard (GTK_WIDGET (terminal));
  gdk_clipboard_read_text_async (clipboard, NULL, read_clipboard, NULL);
  for (int i = 0; i < 200 && !clipboard_read_done; i++) pump ();
  g_assert_true (clipboard_read_done);
  g_assert_nonnull (clipboard_text);
  g_assert_true (g_str_has_prefix (clipboard_text, "clipboard sample"));
  /* Attach a PTY without a shell: verify paste bytes reach the slave. */
  g_autoptr(VtePty) pty = vte_pty_new_sync (VTE_PTY_DEFAULT, NULL, NULL);
  g_assert_nonnull (pty);
  int master = vte_pty_get_fd (pty);
  char name[256];
  g_assert_cmpint (ptsname_r (master, name, sizeof name), ==, 0);
  int slave = open (name, O_RDWR | O_NOCTTY | O_NONBLOCK);
  g_assert_cmpint (slave, >=, 0);
  struct termios attributes;
  g_assert_cmpint (tcgetattr (slave, &attributes), ==, 0);
  cfmakeraw (&attributes);
  g_assert_cmpint (tcsetattr (slave, TCSANOW, &attributes), ==, 0);
  vte_terminal_set_pty (VTE_TERMINAL (terminal), pty);
  ptyxis_terminal_right_click_clipboard (terminal);
  char received[4096] = {0};
  ssize_t count = -1;
  for (int i = 0; i < 200 && count < 0; i++) { pump (); count = read (slave, received, sizeof received - 1); }
  g_assert_cmpint (count, >, 0);
  g_assert_true (g_str_has_prefix (received, "clipboard sample"));
  g_free (clipboard_text);
  clipboard_text = NULL;
  clipboard_read_done = FALSE;
  gdk_clipboard_read_text_async (clipboard, NULL, read_clipboard, NULL);
  for (int i = 0; i < 200 && !clipboard_read_done; i++) pump ();
  g_assert_true (g_str_has_prefix (clipboard_text, "clipboard sample"));
  g_free (clipboard_text);
  close (slave);
  gtk_window_destroy (GTK_WINDOW (window));
  g_print ("PASS: right-click copies to standard clipboard, clears selection, next click pastes through PTY, and clipboard remains available\n");
  return 0;
}

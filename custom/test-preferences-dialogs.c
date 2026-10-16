/* Exercise native GTK dialog ownership without chooser interception. */
#define main ptyxis_original_main
#include "main.c"
#undef main
#include "ptyxis-preferences-window.h"
static GMainLoop *loop;
static void collect (GtkWidget *w, GPtrArray *colors, GPtrArray *resets)
{
  if (g_object_get_data (G_OBJECT (w), "appearance-key"))
    {
      if (GTK_IS_COLOR_DIALOG_BUTTON (w)) g_ptr_array_add (colors, w);
      else if (GTK_IS_BUTTON (w)) g_ptr_array_add (resets, w);
    }
  for (GtkWidget *c = gtk_widget_get_first_child (w); c; c = gtk_widget_get_next_sibling (c)) collect (c, colors, resets);
}
static void finished (GObject *source, GAsyncResult *result, gpointer data)
{
  g_autoptr(GError) error = NULL;
  GdkRGBA *color = gtk_color_dialog_choose_rgba_finish (GTK_COLOR_DIALOG (source), result, &error);
  if (color) gdk_rgba_free (color);
  g_assert_error (error, GTK_DIALOG_ERROR, GTK_DIALOG_ERROR_CANCELLED);
  g_main_loop_quit (loop);
}
static gboolean cancel_dialog (gpointer data)
{
  g_cancellable_cancel (data);
  return G_SOURCE_REMOVE;
}
int main (void)
{
  gtk_init ();
  g_autoptr(PtyxisApplication) app = ptyxis_application_new (APP_ID, G_APPLICATION_NON_UNIQUE);
  g_assert_true (g_application_register (G_APPLICATION (app), NULL, NULL));
  GtkWindow *window = ptyxis_preferences_window_new (GTK_APPLICATION (app));
  gtk_window_present (window);
  GPtrArray *colors = g_ptr_array_new ();
  GPtrArray *resets = g_ptr_array_new ();
  collect (GTK_WIDGET (window), colors, resets);
  g_assert_cmpuint (colors->len, ==, 4);
  loop = g_main_loop_new (NULL, FALSE);
  for (guint round = 0; round < 2; round++)
    for (guint i = 0; i < colors->len; i++)
      {
        GtkColorDialogButton *button = g_ptr_array_index (colors, i);
        if (round) g_signal_emit_by_name (g_ptr_array_index (resets, i), "clicked");
        GtkColorDialog *dialog = gtk_color_dialog_button_get_dialog (button);
        g_assert_true (GTK_IS_COLOR_DIALOG (dialog));
        g_autoptr(GCancellable) cancel = g_cancellable_new ();
        gtk_color_dialog_choose_rgba (dialog, window, gtk_color_dialog_button_get_rgba (button), cancel, finished, NULL);
        g_timeout_add (150, cancel_dialog, cancel);
        g_main_loop_run (loop);
      }
  g_main_loop_unref (loop);
  gtk_window_destroy (window);
  g_ptr_array_unref (colors); g_ptr_array_unref (resets);
  g_print ("PASS: all four native GTK color choosers open/cancel before and after reset, without mocking\n");
  return 0;
}

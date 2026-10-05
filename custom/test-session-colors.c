#define main ptyxis_original_main
#include "main.c"
#undef main
#include "ptyxis-session.h"
#include "ptyxis-window.h"

static GdkRGBA picker_initial;
static gboolean picker_had_initial;
/* Intercept the chooser boundary to verify the actual window action passes
 * the current color, without requiring manual interaction in headless tests. */
void gtk_color_dialog_choose_rgba (GtkColorDialog *dialog, GtkWindow *parent,
                                  const GdkRGBA *initial, GCancellable *cancellable,
                                  GAsyncReadyCallback callback, gpointer user_data)
{
  picker_had_initial = initial != NULL;
  if (initial) picker_initial = *initial;
  g_object_unref (user_data);
}

static GVariant *first_tabs (GVariant *state)
{
  g_autoptr(GVariant) windows = g_variant_lookup_value (state, "windows", G_VARIANT_TYPE ("aa{sv}"));
  g_autoptr(GVariant) window = g_variant_get_child_value (windows, 0);
  return g_variant_lookup_value (window, "tabs", G_VARIANT_TYPE ("aa{sv}"));
}

int main (int argc, char **argv)
{
  gtk_init ();
  g_autoptr(PtyxisApplication) app = ptyxis_application_new (APP_ID, G_APPLICATION_NON_UNIQUE);
  g_assert_true (g_application_register (G_APPLICATION (app), NULL, NULL));
  g_autoptr(GSettings) settings = g_settings_new (APP_SCHEMA_ID);
  g_settings_set_boolean (settings, "restore-session", TRUE);
  g_autoptr(PtyxisProfile) profile = ptyxis_application_dup_default_profile (app);
  PtyxisWindow *window = ptyxis_window_new_empty ();
  PtyxisTab *blue = ptyxis_tab_new (profile);
  PtyxisTab *green = ptyxis_tab_new (profile);
  g_object_set_data_full (G_OBJECT (blue), "tab-color-override", g_strdup ("#3584e4"), g_free);
  g_object_set_data_full (G_OBJECT (green), "tab-color-override", g_strdup ("#26a269"), g_free);
  ptyxis_window_append_tab (window, blue);
  ptyxis_window_append_tab (window, green);
  GdkRGBA expected;
  ptyxis_window_set_active_tab (window, blue);
  g_assert_true (gtk_widget_activate_action (GTK_WIDGET (window), "win.tab-color", NULL));
  g_assert_true (picker_had_initial);
  gdk_rgba_parse (&expected, "#3584e4");
  g_assert_true (gdk_rgba_equal (&picker_initial, &expected));
  ptyxis_window_set_active_tab (window, green);
  g_assert_true (gtk_widget_activate_action (GTK_WIDGET (window), "win.tab-color", NULL));
  gdk_rgba_parse (&expected, "#26a269");
  g_assert_true (gdk_rgba_equal (&picker_initial, &expected));
  g_autoptr(GVariant) saved = ptyxis_session_save (app);
  /* Exercise actual binary session serialization across a file boundary. */
  g_autoptr(GBytes) bytes = g_variant_get_data_as_bytes (saved);
  gsize size;
  const char *data = g_bytes_get_data (bytes, &size);
  g_assert_true (g_file_set_contents ("test-session.gvariant", data, size, NULL));
  g_autofree char *loaded = NULL;
  gsize length;
  g_assert_true (g_file_get_contents ("test-session.gvariant", &loaded, &length, NULL));
  g_autoptr(GBytes) loaded_bytes = g_bytes_new (loaded, length);
  g_autoptr(GVariant) restored_state = g_variant_ref_sink (g_variant_new_from_bytes (G_VARIANT_TYPE ("a{sv}"), loaded_bytes, FALSE));
  gtk_window_destroy (GTK_WINDOW (window));
  g_assert_true (ptyxis_session_restore (app, restored_state));
  window = PTYXIS_WINDOW (gtk_application_get_windows (GTK_APPLICATION (app))->data);
  g_autoptr(GListModel) pages = ptyxis_window_list_pages (window);
  g_assert_cmpuint (g_list_model_get_n_items (pages), ==, 2);
  const char *colors[] = {"#3584e4", "#26a269"};
  for (guint i = 0; i < 2; i++)
    {
      g_autoptr(AdwTabPage) page = g_list_model_get_item (pages, i);
      GtkWidget *tab = adw_tab_page_get_child (page);
      g_assert_cmpstr (g_object_get_data (G_OBJECT (tab), "tab-color-override"), ==, colors[i]);
      g_object_set_data (G_OBJECT (tab), "tab-color-override", NULL);
    }
  g_autofree char *config_dir = g_build_filename (g_get_user_config_dir (), APP_ID, NULL);
  g_mkdir_with_parents (config_dir, 0700);
  g_autofree char *config_path = g_build_filename (config_dir, "tab-colors.ini", NULL);
  g_assert_true (g_file_set_contents (config_path, "[Hosts]\ndefault=#9141ac\n", -1, NULL));
  g_assert_true (gtk_widget_activate_action (GTK_WIDGET (window), "win.tab-color", NULL));
  g_assert_true (picker_had_initial);
  gdk_rgba_parse (&expected, "#9141ac");
  g_assert_true (gdk_rgba_equal (&picker_initial, &expected));
  g_autoptr(GVariant) automatic_state = ptyxis_session_save (app);
  g_autoptr(GVariant) automatic_tabs = first_tabs (automatic_state);
  for (guint i = 0; i < 2; i++)
    {
      g_autoptr(GVariant) tab = g_variant_get_child_value (automatic_tabs, i);
      g_assert_null (g_variant_lookup_value (tab, "tab-color-override", NULL));
    }
  gtk_window_destroy (GTK_WINDOW (window));
  g_assert_true (ptyxis_session_restore (app, automatic_state));
  window = PTYXIS_WINDOW (gtk_application_get_windows (GTK_APPLICATION (app))->data);
  g_autoptr(GListModel) old_pages = ptyxis_window_list_pages (window);
  for (guint i = 0; i < 2; i++)
    {
      g_autoptr(AdwTabPage) page = g_list_model_get_item (old_pages, i);
      g_assert_null (g_object_get_data (G_OBJECT (adw_tab_page_get_child (page)), "tab-color-override"));
    }
  gtk_window_destroy (GTK_WINDOW (window));
  g_print ("PASS: picker receives current manual/automatic colors; blue/green overrides survive binary session save/restore; automatic reset and older sessions without colors remain compatible\n");
  return 0;
}

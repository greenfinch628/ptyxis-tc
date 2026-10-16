#define main ptyxis_original_main
#include "main.c"
#undef main
#include "ptyxis-session.h"
#include "ptyxis-window.h"
#include "ptyxis-preferences-window.h"

static GdkRGBA picker_initial;
static gboolean picker_had_initial;
/* Intercept the chooser boundary to verify the actual window action passes
 * the current color, without requiring manual interaction in headless tests. */
void gtk_color_dialog_choose_rgba (GtkColorDialog *dialog, GtkWindow *parent,
                                  const GdkRGBA *initial, GCancellable *cancellable,
                                  GAsyncReadyCallback callback, gpointer user_data)
{
  g_assert_true (GTK_IS_COLOR_DIALOG (dialog));
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

static void collect_appearance (GtkWidget *widget, GPtrArray *colors, GPtrArray *resets)
{
  if (GTK_IS_COLOR_DIALOG_BUTTON (widget) && g_object_get_data (G_OBJECT (widget), "appearance-key"))
    g_ptr_array_add (colors, widget);
  if (GTK_IS_BUTTON (widget) && g_object_get_data (G_OBJECT (widget), "appearance-key"))
    g_ptr_array_add (resets, widget);
  for (GtkWidget *child = gtk_widget_get_first_child (widget); child; child = gtk_widget_get_next_sibling (child))
    collect_appearance (child, colors, resets);
}

static GtkWidget *find_defaults_button (GtkWidget *widget)
{
  if (g_object_get_data (G_OBJECT (widget), "appearance-defaults"))
    return widget;
  for (GtkWidget *child = gtk_widget_get_first_child (widget); child; child = gtk_widget_get_next_sibling (child))
    {
      GtkWidget *found = find_defaults_button (child);
      if (found) return found;
    }
  return NULL;
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
  g_object_set_data_full (G_OBJECT (blue), "tab-color-inactive-override", g_strdup ("#123456"), g_free);
  ptyxis_window_append_tab (window, blue);
  ptyxis_window_append_tab (window, green);
  ptyxis_window_set_active_tab (window, blue);
  g_assert_true (gtk_widget_activate_action (GTK_WIDGET (window), "page.next", NULL));
  g_assert_true (ptyxis_window_get_active_tab (window) == green);
  g_assert_true (gtk_widget_activate_action (GTK_WIDGET (window), "page.previous", NULL));
  g_assert_true (ptyxis_window_get_active_tab (window) == blue);
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
  ptyxis_window_set_active_tab (window, blue);
  g_assert_true (gtk_widget_activate_action (GTK_WIDGET (window), "win.tab-color-inactive", NULL));
  g_assert_true (picker_had_initial);
  gdk_rgba_parse (&expected, "#123456");
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
      g_assert_cmpstr (ptyxis_profile_get_uuid (ptyxis_tab_get_profile (PTYXIS_TAB (tab))), ==, ptyxis_profile_get_uuid (profile));
      if (i == 0)
        g_assert_cmpstr (g_object_get_data (G_OBJECT (tab), "tab-color-inactive-override"), ==, "#123456");
      ptyxis_window_set_active_tab (window, PTYXIS_TAB (tab));
      g_assert_true (gtk_widget_activate_action (GTK_WIDGET (window), "win.tab-color-auto", NULL));
      g_assert_null (g_object_get_data (G_OBJECT (tab), "tab-color-override"));
      g_assert_null (g_object_get_data (G_OBJECT (tab), "tab-color-inactive-override"));
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
      g_assert_null (g_variant_lookup_value (tab, "tab-color-inactive-override", NULL));
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
  /* Exercise Preferences through its real color buttons and reset controls. */
  g_assert_true (g_file_set_contents (config_path, "# preserve comment\n[Hosts]\ndefault=#9141ac\n[Aliases]\ns=scopuli\n", -1, NULL));
  GtkWindow *preferences = ptyxis_preferences_window_new (GTK_APPLICATION (app));
  gtk_window_set_default_size (preferences, 720, 900);
  gtk_window_present (preferences);
  GPtrArray *color_buttons = g_ptr_array_new ();
  GPtrArray *reset_buttons = g_ptr_array_new ();
  collect_appearance (GTK_WIDGET (preferences), color_buttons, reset_buttons);
  g_assert_cmpuint (color_buttons->len, ==, 4);
  g_assert_cmpuint (reset_buttons->len, ==, 4);
  const char *default_backgrounds[] = { "#e8e8e8", "#c0bfbc" };
  for (guint i = 0; i < 2; i++)
    {
      gdk_rgba_parse (&expected, default_backgrounds[i]);
      g_assert_true (gdk_rgba_equal (gtk_color_dialog_button_get_rgba (g_ptr_array_index (color_buttons, i)), &expected));
    }
  /* Unset text colors show the same pure black used by the tab renderer. */
  gdk_rgba_parse (&expected, "#000000");
  for (guint i = 2; i < 4; i++)
    {
      GtkColorDialogButton *picker = g_ptr_array_index (color_buttons, i);
      g_assert_true (gdk_rgba_equal (gtk_color_dialog_button_get_rgba (picker), &expected));
      AdwActionRow *row = g_object_get_data (G_OBJECT (picker), "appearance-row");
      g_assert_cmpstr (adw_action_row_get_subtitle (row), ==, "Default (Black)");
    }

  /* The constructor takes ownership of its dialog. The dialog must remain
   * alive after setup returns, including when opening after a reset. */
  for (guint i = 0; i < color_buttons->len; i++)
    {
      GtkColorDialogButton *button = g_ptr_array_index (color_buttons, i);
      g_assert_true (GTK_IS_COLOR_DIALOG (gtk_color_dialog_button_get_dialog (button)));
    }
  const char *keys[] = { "active-background", "inactive-background", "active-foreground", "inactive-foreground" };
  for (guint i = 0; i < 4; i++)
    {
      gdk_rgba_parse (&expected, i < 2 ? "#123456" : "#ffffff");
      gtk_color_dialog_button_set_rgba (g_ptr_array_index (color_buttons, i), &expected);
    }
  g_autoptr(GKeyFile) preferences_config = g_key_file_new ();
  g_assert_true (g_key_file_load_from_file (preferences_config, config_path, G_KEY_FILE_NONE, NULL));
  for (guint i = 0; i < 4; i++)
    g_assert_true (g_key_file_has_key (preferences_config, "Appearance", keys[i], NULL));
  /* Reopening Preferences must read saved colors, rather than show placeholders. */
  GtkWindow *reopened = ptyxis_preferences_window_new (GTK_APPLICATION (app));
  GPtrArray *reopened_colors = g_ptr_array_new ();
  GPtrArray *reopened_resets = g_ptr_array_new ();
  collect_appearance (GTK_WIDGET (reopened), reopened_colors, reopened_resets);
  g_assert_cmpuint (reopened_colors->len, ==, 4);
  for (guint i = 0; i < 4; i++)
    {
      gdk_rgba_parse (&expected, i < 2 ? "#123456" : "#ffffff");
      g_assert_true (gdk_rgba_equal (gtk_color_dialog_button_get_rgba (g_ptr_array_index (reopened_colors, i)), &expected));
    }
  gtk_window_destroy (reopened);
  g_ptr_array_unref (reopened_colors);
  g_ptr_array_unref (reopened_resets);
  GtkWidget *defaults_button = find_defaults_button (GTK_WIDGET (preferences));
  g_assert_nonnull (defaults_button);
  g_signal_emit_by_name (defaults_button, "clicked");
  g_assert_true (g_key_file_load_from_file (preferences_config, config_path, G_KEY_FILE_NONE, NULL));
  for (guint i = 0; i < 2; i++)
    {
      g_autofree char *saved_background = g_key_file_get_string (preferences_config, "Appearance", keys[i], NULL);
      g_assert_cmpstr (saved_background, ==, default_backgrounds[i]);
      gdk_rgba_parse (&expected, default_backgrounds[i]);
      g_assert_true (gdk_rgba_equal (gtk_color_dialog_button_get_rgba (g_ptr_array_index (color_buttons, i)), &expected));
    }
  gdk_rgba_parse (&expected, "#ffffff");
  for (guint i = 2; i < 4; i++)
    g_assert_true (gdk_rgba_equal (gtk_color_dialog_button_get_rgba (g_ptr_array_index (color_buttons, i)), &expected));
  reopened = ptyxis_preferences_window_new (GTK_APPLICATION (app));
  reopened_colors = g_ptr_array_new ();
  reopened_resets = g_ptr_array_new ();
  collect_appearance (GTK_WIDGET (reopened), reopened_colors, reopened_resets);
  for (guint i = 0; i < 2; i++)
    {
      gdk_rgba_parse (&expected, default_backgrounds[i]);
      g_assert_true (gdk_rgba_equal (gtk_color_dialog_button_get_rgba (g_ptr_array_index (reopened_colors, i)), &expected));
    }
  gtk_window_destroy (reopened);
  g_ptr_array_unref (reopened_colors);
  g_ptr_array_unref (reopened_resets);
  g_autofree char *mapping = g_key_file_get_string (preferences_config, "Hosts", "default", NULL);
  g_assert_cmpstr (mapping, ==, "#9141ac");
  g_autofree char *alias = g_key_file_get_string (preferences_config, "Aliases", "s", NULL);
  g_assert_cmpstr (alias, ==, "scopuli");
  for (int i=0; i<30; i++) { while(g_main_context_iteration(NULL,FALSE)); g_usleep(10000); }
  GtkWidget *group = gtk_widget_get_ancestor (g_ptr_array_index (color_buttons, 0), ADW_TYPE_PREFERENCES_GROUP);
  GtkWidget *scroller = gtk_widget_get_ancestor (group, GTK_TYPE_SCROLLED_WINDOW);
  graphene_rect_t bounds;
  g_assert_true (gtk_widget_compute_bounds (group, gtk_scrolled_window_get_child (GTK_SCROLLED_WINDOW (scroller)), &bounds));
  gtk_adjustment_set_value (gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scroller)), bounds.origin.y - 12);
  for (int i=0; i<30; i++) { while(g_main_context_iteration(NULL,FALSE)); g_usleep(10000); }
  GdkPaintable *paintable = gtk_widget_paintable_new (GTK_WIDGET (preferences));
  GtkSnapshot *snapshot = gtk_snapshot_new ();
  gdk_paintable_snapshot (paintable, GDK_SNAPSHOT (snapshot), gtk_widget_get_width (GTK_WIDGET (preferences)), gtk_widget_get_height (GTK_WIDGET (preferences)));
  GskRenderNode *node = gtk_snapshot_free_to_node (snapshot);
  GdkTexture *texture = gsk_renderer_render_texture (gtk_native_get_renderer (GTK_NATIVE (preferences)), node, NULL);
  gdk_texture_save_to_png (texture, "preferences.png");
  g_object_unref (texture); gsk_render_node_unref (node); g_object_unref (paintable);
  for (guint i = 0; i < 4; i++)
    g_signal_emit_by_name (g_ptr_array_index (reset_buttons, i), "clicked");
  g_assert_true (g_key_file_load_from_file (preferences_config, config_path, G_KEY_FILE_NONE, NULL));
  for (guint i = 0; i < 4; i++)
    g_assert_false (g_key_file_has_key (preferences_config, "Appearance", keys[i], NULL));
  for (guint i = 0; i < color_buttons->len; i++)
    {
      GtkColorDialogButton *button = g_ptr_array_index (color_buttons, i);
      g_assert_true (GTK_IS_COLOR_DIALOG (gtk_color_dialog_button_get_dialog (button)));
    }
  gdk_rgba_parse (&expected, "#000000");
  for (guint i = 2; i < 4; i++)
    {
      GtkColorDialogButton *picker = g_ptr_array_index (color_buttons, i);
      g_assert_true (gdk_rgba_equal (gtk_color_dialog_button_get_rgba (picker), &expected));
      AdwActionRow *row = g_object_get_data (G_OBJECT (picker), "appearance-row");
      g_assert_cmpstr (adw_action_row_get_subtitle (row), ==, "Default (Black)");
    }
  for (guint i = 0; i < 2; i++)
    {
      gdk_rgba_parse (&expected, default_backgrounds[i]);
      g_assert_true (gdk_rgba_equal (gtk_color_dialog_button_get_rgba (g_ptr_array_index (color_buttons, i)), &expected));
    }
  g_autofree char *preserved = NULL;
  g_assert_true (g_file_get_contents (config_path, &preserved, NULL, NULL));
  g_assert_nonnull (strstr (preserved, "# preserve comment"));
  /* A malformed external file is reported and must never be overwritten. */
  const char *malformed = "[Hosts\ndefault=#123456\n";
  g_assert_true (g_file_set_contents (config_path, malformed, -1, NULL));
  gdk_rgba_parse (&expected, "#abcdef");
  gtk_color_dialog_button_set_rgba (g_ptr_array_index (color_buttons, 0), &expected);
  g_autofree char *unchanged = NULL;
  g_assert_true (g_file_get_contents (config_path, &unchanged, NULL, NULL));
  g_assert_cmpstr (unchanged, ==, malformed);
  gtk_window_destroy (preferences);
  g_ptr_array_unref (color_buttons);
  g_ptr_array_unref (reset_buttons);
  g_print ("PASS: built-in background pair and Default button persistence/mapping preservation; black default/reset swatches and saved colors on reopening; Preferences save/reset preserves host/alias mappings and comments; picker receives current manual/automatic colors; active/inactive overrides survive binary session save/restore; automatic reset and older sessions without colors remain compatible\n");
  return 0;
}

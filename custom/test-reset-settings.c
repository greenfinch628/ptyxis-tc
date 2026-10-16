/* Master reset tests use private XDG directories and an in-memory backend. */
#define main ptyxis_original_main
#include "main.c"
#undef main
#include "ptyxis-preferences-window.h"
#include "ptyxis-window.h"
#include "ptyxis-tab.h"

static void pump (void)
{
  for (int i = 0; i < 20; i++)
    { while (g_main_context_iteration (NULL, FALSE)); g_usleep (10000); }
}

static GtkWidget *find_response_button (GtkWidget *widget, const char *label)
{
  if (GTK_IS_BUTTON (widget) && g_strcmp0 (gtk_button_get_label (GTK_BUTTON (widget)), label) == 0)
    return widget;
  for (GtkWidget *child = gtk_widget_get_first_child (widget); child; child = gtk_widget_get_next_sibling (child))
    {
      GtkWidget *found = find_response_button (child, label);
      if (found) return found;
    }
  return NULL;
}

static void respond (AdwDialog *dialog, const char *response)
{
  pump ();
  GtkWidget *button = find_response_button (GTK_WIDGET (dialog),
    adw_alert_dialog_get_response_label (ADW_ALERT_DIALOG (dialog), response));
  g_assert_nonnull (button);
  g_signal_emit_by_name (button, "clicked");
}

static void capture (GtkWindow *window, const char *filename)
{
  GdkPaintable *paintable = gtk_widget_paintable_new (GTK_WIDGET (window));
  GtkSnapshot *snapshot = gtk_snapshot_new ();
  gdk_paintable_snapshot (paintable, GDK_SNAPSHOT (snapshot), gtk_widget_get_width (GTK_WIDGET (window)), gtk_widget_get_height (GTK_WIDGET (window)));
  GskRenderNode *node = gtk_snapshot_free_to_node (snapshot);
  GdkTexture *texture = gsk_renderer_render_texture (gtk_native_get_renderer (GTK_NATIVE (window)), node, NULL);
  gdk_texture_save_to_png (texture, filename);
  g_object_unref (texture); gsk_render_node_unref (node); g_object_unref (paintable);
}

static void assert_no_custom_keys (GSettings *settings, gboolean allow_profile_ids)
{
  g_autoptr(GSettingsSchema) schema = NULL;
  g_auto(GStrv) keys = NULL;
  g_object_get (settings, "settings-schema", &schema, NULL);
  keys = g_settings_schema_list_keys (schema);
  for (guint i = 0; keys[i]; i++)
    {
      if (allow_profile_ids && (g_str_equal (keys[i], "profile-uuids") ||
                                g_str_equal (keys[i], "default-profile-uuid")))
        continue;
      g_autoptr(GVariant) actual = g_settings_get_value (settings, keys[i]);
      g_autoptr(GVariant) initial = g_settings_get_default_value (settings, keys[i]);
      if (!g_variant_equal (actual, initial))
        {
          g_autofree char *value = g_variant_print (actual, TRUE);
          g_printerr ("Non-default reset key %s: %s\n", keys[i], value);
        }
      g_assert_true (g_variant_equal (actual, initial));
    }
}

int main (void)
{
  gtk_init ();
  g_object_set (gtk_settings_get_default (), "gtk-enable-animations", FALSE, NULL);
  g_autoptr(PtyxisApplication) app = ptyxis_application_new (APP_ID, G_APPLICATION_NON_UNIQUE);
  g_assert_true (g_application_register (G_APPLICATION (app), NULL, NULL));
  g_autoptr(GSettings) settings = g_settings_new (APP_SCHEMA_ID);
  g_autoptr(GSettings) shortcuts = g_settings_new (APP_SCHEMA_SHORTCUTS_ID);
  g_autoptr(PtyxisProfile) original = ptyxis_application_dup_default_profile (app);
  g_autoptr(PtyxisProfile) custom = ptyxis_profile_new (NULL);
  ptyxis_application_add_profile (app, custom);
  ptyxis_application_set_default_profile (app, custom);
  g_autofree char *custom_path = g_strdup_printf (APP_SCHEMA_PATH "Profiles/%s/", ptyxis_profile_get_uuid (custom));
  g_autoptr(GSettings) profile_settings = g_settings_new_with_path (APP_SCHEMA_PROFILE_ID, custom_path);
  g_settings_set_string (profile_settings, "label", "Reset custom profile");
  g_settings_set_string (profile_settings, "palette", "dracula");
  g_settings_set_string (settings, "font-name", "Monospace 19");
  g_settings_set_boolean (settings, "use-system-font", FALSE);
  g_settings_set_boolean (settings, "audible-bell", FALSE);
  g_settings_set_string (shortcuts, "copy-clipboard", "<Control><Alt>c");
  g_autoptr(GSettings) regular_sentinel = g_settings_new_with_path (APP_SCHEMA_PROFILE_ID,
    "/org/gnome/Ptyxis/Profiles/reset-sentinel/");
  g_settings_set_string (regular_sentinel, "label", "Regular Ptyxis sentinel");

  PtyxisWindow *window = ptyxis_window_new_empty ();
  PtyxisTab *tab = ptyxis_tab_new (custom);
  ptyxis_window_append_tab (window, tab);
  g_object_set_data_full (G_OBJECT (tab), "tab-color-override", g_strdup ("#ff0000"), g_free);
  g_object_set_data_full (G_OBJECT (tab), "tab-color-inactive-override", g_strdup ("#001122"), g_free);
  g_autofree char *config_dir = g_build_filename (g_get_user_config_dir (), APP_ID, NULL);
  g_autofree char *colors_path = g_build_filename (config_dir, "tab-colors.ini", NULL);
  g_autofree char *session_path = g_build_filename (config_dir, "session.gvariant", NULL);
  g_mkdir_with_parents (config_dir, 0700);
  const char *colors = "[Hosts]\nexample=#ff0000\n[Aliases]\ne=example\n[Appearance]\nactive-foreground=#ffffff\n";
  g_assert_true (g_file_set_contents (colors_path, colors, -1, NULL));
  g_assert_true (g_file_set_contents (session_path, "old session", -1, NULL));

  GtkWindow *preferences = ptyxis_preferences_window_new (GTK_APPLICATION (app));
  gtk_application_add_window (GTK_APPLICATION (app), preferences);
  gtk_window_set_transient_for (preferences, GTK_WINDOW (window));
  gtk_window_present (preferences);
  pump ();
  GtkWidget *reset_button = find_response_button (GTK_WIDGET (preferences), "Reset All Settings…");
  g_assert_nonnull (reset_button);
  GtkWidget *scroller = gtk_widget_get_ancestor (reset_button, GTK_TYPE_SCROLLED_WINDOW);
  GtkAdjustment *adjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scroller));
  gtk_adjustment_set_value (adjustment, gtk_adjustment_get_upper (adjustment));
  pump ();
  capture (preferences, "master-reset.png");
  g_assert_true (gtk_widget_activate_action (GTK_WIDGET (preferences), "settings.reset-all", NULL));
  AdwDialog *confirmation = adw_window_get_visible_dialog (ADW_WINDOW (preferences));
  g_assert_true (ADW_IS_ALERT_DIALOG (confirmation));
  pump ();
  capture (preferences, "master-reset-confirmation.png");
  respond (confirmation, "cancel");
  pump ();
  g_autofree char *font = g_settings_get_string (settings, "font-name");
  g_assert_cmpstr (font, ==, "Monospace 19");
  g_assert_true (g_file_test (colors_path, G_FILE_TEST_EXISTS));
  g_assert_cmpstr (g_object_get_data (G_OBJECT (tab), "tab-color-override"), ==, "#ff0000");

  /* If a backup cannot be made, no settings or files may be reset. */
  g_autofree char *state_dir = g_build_filename (g_get_user_state_dir (), APP_ID, NULL);
  g_autofree char *backup_root = g_build_filename (state_dir, "reset-backups", NULL);
  if (!g_file_test (backup_root, G_FILE_TEST_EXISTS))
    {
      g_mkdir_with_parents (state_dir, 0700);
      g_assert_true (g_file_set_contents (backup_root, "block backup directory", -1, NULL));
      g_autoptr(GError) backup_error = NULL;
      g_autofree char *failed = ptyxis_application_reset_preferences (app, &backup_error);
      g_assert_null (failed);
      g_assert_nonnull (backup_error);
      g_assert_true (g_file_test (colors_path, G_FILE_TEST_EXISTS));
      g_autofree char *unchanged = g_settings_get_string (settings, "font-name");
      g_assert_cmpstr (unchanged, ==, "Monospace 19");
      g_autoptr(GFile) blocker = g_file_new_for_path (backup_root);
      g_assert_true (g_file_delete (blocker, NULL, NULL));
    }
  /* Force a save/reset race: old asynchronous writes must not recreate tabs. */
  ptyxis_application_save_session (app);
  g_autoptr(GError) error = NULL;
  g_autofree char *backup = ptyxis_application_reset_preferences (app, &error);
  g_assert_no_error (error);
  g_assert_nonnull (backup);
  pump ();
  g_assert_false (g_file_test (colors_path, G_FILE_TEST_EXISTS));
  g_assert_false (g_file_test (session_path, G_FILE_TEST_EXISTS));
  ptyxis_application_save_session (app);
  pump ();
  g_assert_false (g_file_test (session_path, G_FILE_TEST_EXISTS));
  assert_no_custom_keys (settings, TRUE);
  assert_no_custom_keys (shortcuts, FALSE);
  assert_no_custom_keys (profile_settings, FALSE);
  g_autofree char *sentinel = g_settings_get_string (regular_sentinel, "label");
  g_assert_cmpstr (sentinel, ==, "Regular Ptyxis sentinel");
  g_assert_null (g_object_get_data (G_OBJECT (tab), "tab-color-override"));
  g_assert_null (g_object_get_data (G_OBJECT (tab), "tab-color-inactive-override"));
  g_autoptr(GListModel) pages = ptyxis_window_list_pages (window);
  g_assert_cmpuint (g_list_model_get_n_items (pages), ==, 1);
  g_autoptr(AdwTabPage) page = g_list_model_get_item (pages, 0);
  g_assert_true (adw_tab_page_get_child (page) == GTK_WIDGET (tab));
  g_autoptr(PtyxisProfile) fresh = ptyxis_application_dup_default_profile (app);
  g_assert_cmpstr (ptyxis_profile_get_uuid (ptyxis_tab_get_profile (tab)), ==, ptyxis_profile_get_uuid (fresh));
  g_assert_cmpstr (ptyxis_profile_get_uuid (fresh), !=, ptyxis_profile_get_uuid (custom));
  g_autoptr(GListModel) profiles = ptyxis_application_list_profiles (app);
  g_assert_cmpuint (g_list_model_get_n_items (profiles), ==, 1);
  g_autofree char *saved_settings = g_build_filename (backup, "settings.ini", NULL);
  g_autoptr(GKeyFile) saved = g_key_file_new ();
  g_assert_true (g_key_file_load_from_file (saved, saved_settings, G_KEY_FILE_NONE, NULL));
  g_autofree char *saved_font = g_key_file_get_string (saved, "Application", "font-name", NULL);
  g_assert_cmpstr (saved_font, ==, "'Monospace 19'");
  g_autofree char *saved_colors = g_build_filename (backup, "config", "tab-colors.ini", NULL);
  g_autofree char *saved_contents = NULL;
  g_assert_true (g_file_get_contents (saved_colors, &saved_contents, NULL, NULL));
  g_assert_cmpstr (saved_contents, ==, colors);
  g_autofree char *live_session = g_build_filename (backup, "live-session.gvariant", NULL);
  g_assert_true (g_file_test (live_session, G_FILE_TEST_EXISTS));

  /* Also cover a Preferences window opened outside the application's window list. */
  preferences = ptyxis_preferences_window_new (GTK_APPLICATION (app));
  gtk_window_set_transient_for (preferences, GTK_WINDOW (window));
  gtk_window_present (preferences);
  pump ();
  g_settings_set_string (settings, "font-name", "Monospace 22");
  g_assert_true (gtk_widget_activate_action (GTK_WIDGET (preferences), "settings.reset-all", NULL));
  confirmation = adw_window_get_visible_dialog (ADW_WINDOW (preferences));
  g_assert_true (ADW_IS_ALERT_DIALOG (confirmation));
  respond (confirmation, "reset");
  pump ();
  assert_no_custom_keys (settings, TRUE);
  g_assert_false (g_file_test (session_path, G_FILE_TEST_EXISTS));
  gtk_window_destroy (GTK_WINDOW (window));
  g_print ("PASS: reset confirmation/cancel, settings/profiles/shortcuts reset, backups, same open terminal preserved, regular settings untouched, old session saves suppressed, UI reset response\n");
  return 0;
}

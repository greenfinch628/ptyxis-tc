/* Local tab coloring extension for Ptyxis 50.1. SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef COLOR_TAB_TESTING
#include "ptyxis-terminal.h"
#endif

static char *
color_tab_hostname (PtyxisTab *tab, GKeyFile *config)
{
  g_autoptr(GUri) directory = vte_terminal_ref_termprop_uri_by_id (VTE_TERMINAL (ptyxis_tab_get_terminal (tab)), VTE_PROPERTY_ID_CURRENT_DIRECTORY_URI);
  const char *host = directory ? g_uri_get_host (directory) : NULL;
  /* Foreground SSH command provides a fallback when the remote shell does
   * not emit OSC 7. Explicit alias mappings avoid spawning ssh or DNS lookups. */
  const char *command = ptyxis_tab_get_command_line (tab);
  g_auto(GStrv) args = NULL;
  int argc = 0;
  if (command && g_shell_parse_argv (command, &argc, &args, NULL) && argc > 1)
    {
      g_autofree char *program = g_path_get_basename (args[0]);
      if (g_str_equal (program, "ssh"))
        {
          for (int i = 1; i < argc; i++)
            {
              if (g_str_equal (args[i], "--"))
                continue;
              if (args[i][0] == '-')
                {
                  if (strlen (args[i]) == 2 && strchr ("BbcDEeFIiJLlmOopQRSWw", args[i][1]))
                    i++;
                  continue;
                }
              const char *destination = strrchr (args[i], '@');
              destination = destination ? destination + 1 : args[i];
              g_autofree char *lower = g_ascii_strdown (destination, -1);
              g_autofree char *alias = g_key_file_get_string (config, "Aliases", lower, NULL);
              /* OSC 7 is authoritative for nested SSH sessions. */
              if (host && *host && !g_str_equal (host, "localhost") &&
                  g_ascii_strcasecmp (host, g_get_host_name ()) != 0)
                return g_ascii_strdown (host, -1);
              if (alias)
                return g_ascii_strdown (alias, -1);
              return g_steal_pointer (&lower);
            }
        }
    }
  if (!host || !*host || g_str_equal (host, "localhost"))
    host = g_get_host_name ();
  return g_ascii_strdown (host, -1);
}

static char *
color_tab_value (PtyxisTab *tab, GKeyFile *config)
{
  const char *manual = g_object_get_data (G_OBJECT (tab), "tab-color-override");
  g_autofree char *host = NULL;
  g_autofree char *short_host = NULL;
  char *color;
  char *dot;
  if (manual)
    return g_strdup (manual);
  host = color_tab_hostname (tab, config);
  color = g_key_file_get_string (config, "Hosts", host, NULL);
  if (color)
    return color;
  short_host = g_strdup (host);
  if ((dot = strchr (short_host, '.')))
    *dot = 0;
  color = g_key_file_get_string (config, "Hosts", short_host, NULL);
  if (color)
    return color;
  return g_key_file_get_string (config, "Hosts", "default", NULL);
}

/* AdwTab's page property is introspected instead of linking private symbols.
 * If a future libadwaita changes it, coloring safely stops for that widget. */
static void
color_tab_widgets (GtkWidget *widget, GKeyFile *config)
{
  GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (widget), "page");
  if (pspec && g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), ADW_TYPE_TAB_PAGE))
    {
      g_autoptr(AdwTabPage) page = NULL;
      g_autofree char *value = NULL;
      g_object_get (widget, "page", &page, NULL);
      if (page && PTYXIS_IS_TAB (adw_tab_page_get_child (page)))
        value = color_tab_value (PTYXIS_TAB (adw_tab_page_get_child (page)), config);
      if (!g_object_get_data (G_OBJECT (widget), "tab-color-provider") ||
          g_strcmp0 (value, g_object_get_data (G_OBJECT (widget), "tab-color-applied")))
        {
          GtkCssProvider *provider = g_object_get_data (G_OBJECT (widget), "tab-color-provider");
          GdkRGBA rgba;
          if (provider)
            gtk_style_context_remove_provider (gtk_widget_get_style_context (widget), GTK_STYLE_PROVIDER (provider));
          g_object_set_data (G_OBJECT (widget), "tab-color-provider", NULL);
          g_object_set_data_full (G_OBJECT (widget), "tab-color-applied", g_strdup (value), g_free);
          g_autofree char *css = NULL;
          if (value && gdk_rgba_parse (&rgba, value))
            {
              g_autofree char *css_color = gdk_rgba_to_string (&rgba);
              double luminance = .2126 * rgba.red + .7152 * rgba.green + .0722 * rgba.blue;
              const char *foreground = luminance > .5 ? "#141414" : "#ffffff";
              css = g_strdup_printf (
                "tab { background-color: mix(#dddddd, %s, 0.40); "
                "background-image: none; color: #141414; border-bottom: 3px solid transparent; }"
                "tab:selected { background-color: %s; color: %s; "
                "background-image: none; border-bottom-color: #ffffff; box-shadow: none; }",
                css_color, css_color, foreground);
            }
          else
            css = g_strdup ("tab { border-bottom: 3px solid transparent; } tab:selected { background-image: none; border-bottom-color: #ffffff; box-shadow: none; }");
          provider = gtk_css_provider_new ();
          gtk_css_provider_load_from_string (provider, css);
          gtk_style_context_add_provider (gtk_widget_get_style_context (widget), GTK_STYLE_PROVIDER (provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 10);
          g_object_set_data_full (G_OBJECT (widget), "tab-color-provider", provider, g_object_unref);
        }
    }
  for (GtkWidget *child = gtk_widget_get_first_child (widget); child; child = gtk_widget_get_next_sibling (child))
    color_tab_widgets (child, config);
}

static gboolean
color_tabs_refresh (gpointer data)
{
  PtyxisWindow *self = data;
  g_autoptr(GKeyFile) config = g_key_file_new ();
  g_autofree char *path = g_build_filename (g_get_user_config_dir (), APP_ID, "tab-colors.ini", NULL);
  if (gtk_widget_get_mapped (GTK_WIDGET (self)))
    {
      g_key_file_load_from_file (config, path, G_KEY_FILE_NONE, NULL);
      color_tab_widgets (GTK_WIDGET (self->tab_bar), config);
    }
  return G_SOURCE_CONTINUE;
}

static void
color_chosen (GObject *source, GAsyncResult *result, gpointer data)
{
  g_autoptr(PtyxisTab) tab = data;
  g_autoptr(GError) error = NULL;
  GdkRGBA *rgba = gtk_color_dialog_choose_rgba_finish (GTK_COLOR_DIALOG (source), result, &error);
  if (rgba)
    {
      g_object_set_data_full (G_OBJECT (tab), "tab-color-override", gdk_rgba_to_string (rgba), g_free);
      gdk_rgba_free (rgba);
    }
}

static void
color_tab_action (GtkWidget *widget, const char *name, GVariant *parameter)
{
  PtyxisWindow *self = PTYXIS_WINDOW (widget);
  AdwTabPage *page = adw_tab_view_get_selected_page (self->tab_view);
  GtkWidget *tab = page ? adw_tab_page_get_child (page) : NULL;
  if (!tab)
    return;
  if (g_str_equal (name, "win.tab-color-auto"))
    g_object_set_data (G_OBJECT (tab), "tab-color-override", NULL);
  else
    {
      g_autoptr(GtkColorDialog) dialog = gtk_color_dialog_new ();
      g_autoptr(GKeyFile) config = g_key_file_new ();
      g_autofree char *path = g_build_filename (g_get_user_config_dir (), APP_ID, "tab-colors.ini", NULL);
      g_autofree char *value = NULL;
      GdkRGBA initial;
      gboolean has_initial;

      g_key_file_load_from_file (config, path, G_KEY_FILE_NONE, NULL);
      value = color_tab_value (PTYXIS_TAB (tab), config);
      has_initial = value && gdk_rgba_parse (&initial, value);
      gtk_color_dialog_set_title (dialog, "Tab Background Color");
      gtk_color_dialog_set_with_alpha (dialog, FALSE);
      gtk_color_dialog_choose_rgba (dialog, GTK_WINDOW (self), has_initial ? &initial : NULL, NULL, color_chosen, g_object_ref (tab));
    }
}

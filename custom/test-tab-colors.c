#include <adwaita.h>
#include <vte/vte.h>
#include <string.h>
#define COLOR_TAB_TESTING
#define APP_ID "org.gnome.Ptyxis.TabColors"
typedef GtkWidget PtyxisTab;
typedef struct { GtkWidget *tab_bar; AdwTabView *tab_view; } PtyxisWindow;
#define PTYXIS_WINDOW(w) ((PtyxisWindow *) (w))
#define PTYXIS_TAB(w) ((PtyxisTab *) (w))
#define PTYXIS_IS_TAB(w) VTE_IS_TERMINAL(w)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (PtyxisTab, g_object_unref)
static VteTerminal *ptyxis_tab_get_terminal (PtyxisTab *tab) { return VTE_TERMINAL (tab); }
static const char *ptyxis_tab_get_command_line (PtyxisTab *tab) { return g_object_get_data (G_OBJECT (tab), "command"); }
#include "ptyxis-tab-colors.h"
static int colored = 0;
static void inspect (GtkWidget *w)
{
  if (g_object_get_data (G_OBJECT(w), "tab-color-applied"))
    {
      g_assert_nonnull (g_object_get_data (G_OBJECT(w), "tab-color-provider"));
      colored++;
    }
  for (GtkWidget *c = gtk_widget_get_first_child(w); c; c = gtk_widget_get_next_sibling(c)) inspect(c);
}
static GtkWidget *find_page_widget (GtkWidget *widget, AdwTabPage *page)
{
  GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (widget), "page");
  if (pspec && g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), ADW_TYPE_TAB_PAGE))
    {
      g_autoptr(AdwTabPage) candidate = NULL;
      g_object_get (widget, "page", &candidate, NULL);
      if (candidate == page) return widget;
    }
  for (GtkWidget *child = gtk_widget_get_first_child (widget); child; child = gtk_widget_get_next_sibling (child))
    {
      GtkWidget *found = find_page_widget (child, page);
      if (found) return found;
    }
  return NULL;
}

static void assert_foreground (GtkWidget *bar, AdwTabPage *page, const char *expected)
{
  GdkRGBA actual, color;
  GtkWidget *widget = find_page_widget (bar, page);
  g_assert_nonnull (widget);
  gtk_style_context_get_color (gtk_widget_get_style_context (widget), &actual);
  gdk_rgba_parse (&color, expected);
  g_assert_true (gdk_rgba_equal (&actual, &color));
}

static void assert_pixel (GtkWidget *window, GtkWidget *bar, AdwTabPage *page, const char *expected, const char *filename, gboolean underline)
{
  GtkWidget *widget = find_page_widget (bar, page);
  graphene_point_t point = GRAPHENE_POINT_INIT (12, 10), position;
  if (underline)
    {
      GtkBorder padding;
      gtk_style_context_get_padding (gtk_widget_get_style_context (widget), &padding);
      point = (graphene_point_t) { gtk_widget_get_width (widget) / 2.f,
                                  gtk_widget_get_height (widget) + padding.bottom + 1.f };
    }
  g_assert_nonnull (widget);
  g_assert_true (gtk_widget_compute_point (widget, window, &point, &position));
  GdkPaintable *paintable = gtk_widget_paintable_new (window);
  GtkSnapshot *snapshot = gtk_snapshot_new ();
  gdk_paintable_snapshot (paintable, GDK_SNAPSHOT (snapshot), gtk_widget_get_width (window), gtk_widget_get_height (window));
  GskRenderNode *node = gtk_snapshot_free_to_node (snapshot);
  GdkTexture *texture = gsk_renderer_render_texture (gtk_native_get_renderer (GTK_NATIVE (window)), node, NULL);
  GdkTextureDownloader *downloader = gdk_texture_downloader_new (texture);
  gdk_texture_downloader_set_format (downloader, GDK_MEMORY_R8G8B8A8);
  gsize stride;
  GBytes *bytes = gdk_texture_downloader_download_bytes (downloader, &stride);
  const guint8 *pixels = g_bytes_get_data (bytes, NULL);
  const guint8 *pixel = pixels + (int) position.y * stride + (int) position.x * 4;
  GdkRGBA color;
  gdk_rgba_parse (&color, expected);
  g_assert_cmpint (ABS ((int) pixel[0] - (int) (color.red * 255 + .5)), <=, 1);
  g_assert_cmpint (ABS ((int) pixel[1] - (int) (color.green * 255 + .5)), <=, 1);
  g_assert_cmpint (ABS ((int) pixel[2] - (int) (color.blue * 255 + .5)), <=, 1);
  if (filename) gdk_texture_save_to_png (texture, filename);
  g_bytes_unref (bytes);
  gdk_texture_downloader_free (downloader);
  g_object_unref (texture);
  gsk_render_node_unref (node);
  g_object_unref (paintable);
}

static void assert_background (GtkWidget *window, GtkWidget *bar, AdwTabPage *page, const char *expected, const char *filename)
{
  assert_pixel (window, bar, page, expected, filename, FALSE);
}

int main (void)
{
  adw_init ();
  g_object_set (gtk_settings_get_default (), "gtk-enable-animations", FALSE, NULL);
  g_autoptr(GKeyFile) config = g_key_file_new ();
  g_key_file_set_string(config, "Hosts", "scopuli", "#3584e4");
  g_key_file_set_string(config, "Aliases", "s", "scopuli");
  GtkWidget *tab = vte_terminal_new ();
  g_object_ref_sink(tab);
  g_object_set_data(G_OBJECT(tab), "command", "ssh -p 7051 rczys@s");
  g_autofree char *host = color_tab_hostname(tab, config);
  g_assert_cmpstr(host, ==, "scopuli");
  g_autofree char *color = color_tab_value(tab, config);
  g_assert_cmpstr(color, ==, "#3584e4");
  g_key_file_set_string (config, "InactiveHosts", "scopuli", "#123456");
  g_autofree char *ssh_inactive = color_tab_inactive_value (tab, config);
  g_assert_cmpstr (ssh_inactive, ==, "rgb(18,52,86)");
  g_object_set_data_full(G_OBJECT(tab), "tab-color-override", g_strdup("#c01c28"), g_free);
  g_autofree char *override = color_tab_value(tab, config);
  g_assert_cmpstr(override, ==, "#c01c28");
  g_autofree char *manual_css = color_tab_css (tab, config);
  g_assert_null (strstr (manual_css, "rgb(18,52,86)"));
  g_key_file_set_string (config, "Appearance", "inactive-background", "#222222");
  g_key_file_set_string (config, "Appearance", "active-foreground", "#ffeeaa");
  g_autofree char *appearance_css = color_tab_css (tab, config);
  g_assert_nonnull (strstr (appearance_css, "rgb(34,34,34)"));
  g_assert_nonnull (strstr (appearance_css, "rgb(255,238,170)"));
  g_object_set_data_full (G_OBJECT (tab), "tab-color-inactive-override", g_strdup ("#00ffff"), g_free);
  g_autofree char *pair_css = color_tab_css (tab, config);
  g_assert_nonnull (strstr (pair_css, "rgb(0,255,255)"));
  g_key_file_remove_group (config, "Appearance", NULL);
  g_object_set_data (G_OBJECT (tab), "tab-color-inactive-override", NULL);
  GtkWidget *window = gtk_window_new ();
  AdwTabView *view = adw_tab_view_new ();
  GtkWidget *bar = GTK_WIDGET (adw_tab_bar_new ());
  adw_tab_bar_set_view(ADW_TAB_BAR(bar), view);
  adw_tab_bar_set_autohide(ADW_TAB_BAR(bar), FALSE);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_append(GTK_BOX(box), bar);
  gtk_box_append(GTK_BOX(box), GTK_WIDGET(view));
  gtk_window_set_child(GTK_WINDOW(window), box);
  AdwTabPage *page = adw_tab_view_append(view, tab);
  adw_tab_page_set_title(page, "scopuli");
  GtkWidget *preview_tab = vte_terminal_new();
  g_object_set_data_full(G_OBJECT(preview_tab), "tab-color-override", g_strdup("#26a269"), g_free);
  AdwTabPage *preview_page = adw_tab_view_append(view, preview_tab);
  adw_tab_page_set_title(preview_page, "Inactive green");
  adw_tab_page_set_title(page, "Active red — underlined");
  adw_tab_view_set_selected_page(view, page);
  gtk_window_set_default_size(GTK_WINDOW(window), 760, 280);
  gtk_window_present(GTK_WINDOW(window));
  for (int i=0; i<100; i++) { while(g_main_context_iteration(NULL,FALSE)); g_usleep(10000); }
  color_tab_widgets(bar, config);
  inspect(bar);
  for (int i=0; i<30; i++) { while(g_main_context_iteration(NULL,FALSE)); g_usleep(10000); }
  GdkPaintable *paintable = gtk_widget_paintable_new(window);
  GtkSnapshot *snapshot = gtk_snapshot_new();
  int width = gtk_widget_get_width(window), height = gtk_widget_get_height(window);
  gdk_paintable_snapshot(paintable, GDK_SNAPSHOT(snapshot), width, height);
  GskRenderNode *node = gtk_snapshot_free_to_node(snapshot);
  GdkTexture *texture = gsk_renderer_render_texture(gtk_native_get_renderer(GTK_NATIVE(window)), node, NULL);
  gdk_texture_save_to_png(texture, "test-tab-colors.png");
  g_object_unref(texture); gsk_render_node_unref(node); g_object_unref(paintable);
  g_assert_cmpint(colored, >, 0);
  assert_foreground (bar, page, "#000000");
  assert_foreground (bar, preview_page, "#000000");
  g_key_file_set_string (config, "Appearance", "active-foreground", "#ffffff");
  color_tab_widgets (bar, config);
  for (int i=0; i<10; i++) { while(g_main_context_iteration(NULL,FALSE)); g_usleep(10000); }
  assert_foreground (bar, page, "#ffffff");
  assert_foreground (bar, preview_page, "#000000");
  g_key_file_remove_group (config, "Appearance", NULL);

  /* Explicit dark text on orange must not change the white indicator. */
  g_object_set_data_full (G_OBJECT (tab), "tab-color-override", g_strdup ("#ffa500"), g_free);
  g_object_set_data_full (G_OBJECT (tab), "tab-color-inactive-override", g_strdup ("#ff8c00"), g_free);
  g_key_file_set_string (config, "Appearance", "active-foreground", "#000000");
  color_tab_widgets (bar, config);
  for (int i=0; i<10; i++) { while(g_main_context_iteration(NULL,FALSE)); g_usleep(10000); }
  assert_background (window, bar, page, "#ffa500", "orange-active.png");
  assert_pixel (window, bar, page, "#ffffff", NULL, TRUE);
  adw_tab_view_set_selected_page (view, preview_page);
  for (int i=0; i<10; i++) { while(g_main_context_iteration(NULL,FALSE)); g_usleep(10000); }
  assert_background (window, bar, page, "#ff8c00", "orange-inactive.png");
  assert_pixel (window, bar, page, "#ff8c00", NULL, TRUE);
  adw_tab_view_set_selected_page (view, page);
  g_object_set_data_full (G_OBJECT (tab), "tab-color-override", g_strdup ("#c01c28"), g_free);
  g_object_set_data (G_OBJECT (tab), "tab-color-inactive-override", NULL);
  g_key_file_remove_group (config, "Appearance", NULL);

  g_object_set_data_full (G_OBJECT (tab), "tab-color-inactive-override", g_strdup ("#123456"), g_free);
  color_tab_widgets (bar, config);
  for (int i=0; i<10; i++) { while(g_main_context_iteration(NULL,FALSE)); g_usleep(10000); }
  assert_background (window, bar, page, "#c01c28", "active-red.png");
  adw_tab_view_set_selected_page(view, preview_page);
  /* Do not refresh CSS: selection alone must change the rendered colors. */
  for (int i=0; i<10; i++) { while(g_main_context_iteration(NULL,FALSE)); g_usleep(10000); }
  assert_background (window, bar, page, "#123456", "inactive-blue.png");
  assert_background (window, bar, preview_page, "#26a269", NULL);
  g_object_set_data (G_OBJECT (tab), "tab-color-inactive-override", NULL);

  g_assert_false(adw_tab_page_get_selected(page));
  g_assert_true(adw_tab_page_get_selected(preview_page));
  adw_tab_view_close_page(view, preview_page);
  for (int i=0; i<50; i++) { while(g_main_context_iteration(NULL,FALSE)); g_usleep(10000); }
  g_object_set_data(G_OBJECT(tab), "tab-color-override", NULL);
  color_tab_widgets(bar, config);
  g_autofree char *automatic = color_tab_value(tab, config);
  g_assert_cmpstr(automatic, ==, "#3584e4");
  g_key_file_remove_key(config, "Hosts", "scopuli", NULL);
  g_key_file_remove_group (config, "InactiveHosts", NULL);
  color_tab_widgets(bar, config);
  colored = 0; inspect(bar);
  g_assert_cmpint(colored, ==, 0);
  /* Empty settings use the startup pair, including newly appended tabs. */
  g_autoptr(GKeyFile) fresh_config = g_key_file_new ();
  color_tab_widgets (bar, fresh_config);
  for (int i=0; i<10; i++) { while(g_main_context_iteration(NULL,FALSE)); g_usleep(10000); }
  assert_background (window, bar, page, "#e8e8e8", NULL);
  GtkWidget *new_default_tab = vte_terminal_new ();
  AdwTabPage *new_default_page = adw_tab_view_append (view, new_default_tab);
  adw_tab_page_set_title (new_default_page, "New tab defaults");
  for (int i=0; i<10; i++) { while(g_main_context_iteration(NULL,FALSE)); g_usleep(10000); }
  color_tab_widgets (bar, fresh_config);
  for (int i=0; i<10; i++) { while(g_main_context_iteration(NULL,FALSE)); g_usleep(10000); }
  assert_background (window, bar, new_default_page, "#c0bfbc", "default-pair.png");
  adw_tab_view_set_selected_page (view, new_default_page);
  for (int i=0; i<10; i++) { while(g_main_context_iteration(NULL,FALSE)); g_usleep(10000); }
  assert_background (window, bar, new_default_page, "#e8e8e8", "new-tab-active.png");
  assert_background (window, bar, page, "#c0bfbc", NULL);
  adw_tab_view_close_page (view, new_default_page);
  for (int i=0; i<10; i++) { while(g_main_context_iteration(NULL,FALSE)); g_usleep(10000); }
  /* OSC 7 takes precedence over a mapped SSH alias for nested sessions. */
  vte_terminal_feed(VTE_TERMINAL(tab), "\033]7;file://narlikar.example.org/home/test\033\\", -1);
  for (int i=0; i<30; i++) { while(g_main_context_iteration(NULL,FALSE)); g_usleep(10000); }
  g_autofree char *remote = color_tab_hostname(tab, config);
  g_assert_cmpstr(remote, ==, "narlikar.example.org");
  g_key_file_set_string(config, "Hosts", "narlikar", "#26a269");
  g_autofree char *short_match = color_tab_value(tab, config);
  g_assert_cmpstr(short_match, ==, "#26a269");
  g_object_set_data_full(G_OBJECT(tab), "tab-color-override", g_strdup("#c01c28"), g_free);
  GtkWidget *second_tab = vte_terminal_new();
  adw_tab_view_append(view, second_tab);
  adw_tab_view_reorder_page(view, page, 1);
  g_autofree char *reordered = color_tab_value(tab, config);
  g_assert_cmpstr(reordered, ==, "#c01c28");
  AdwTabView *other_view = adw_tab_view_new();
  g_object_ref_sink(other_view);
  adw_tab_view_transfer_page(view, page, other_view, 0);
  g_autofree char *moved = color_tab_value(PTYXIS_TAB(adw_tab_page_get_child(page)), config);
  g_assert_cmpstr(moved, ==, "#c01c28");
  g_object_unref(other_view);
  g_key_file_set_string (config, "Appearance", "active-background", "#010203");
  g_key_file_set_string (config, "Appearance", "inactive-background", "#f8f8f8");
  g_object_set_data (G_OBJECT (tab), "tab-color-override", NULL);
  g_key_file_remove_group (config, "Hosts", NULL);
  g_autofree char *defaults_css = color_tab_css (tab, config);
  g_assert_nonnull (strstr (defaults_css, "rgb(1,2,3)"));
  g_key_file_set_string (config, "Appearance", "active-background", "invalid; }");
  g_key_file_set_string (config, "Appearance", "inactive-foreground", "invalid; }");
  g_autofree char *invalid_css = color_tab_css (tab, config);
  g_assert_null (strstr (invalid_css, "invalid"));
  gtk_window_destroy(GTK_WINDOW(window));
  g_object_unref(tab);
  g_print("PASS: rendered active/inactive pixels and switching without refresh, defaults, explicit text colors, invalid values, SSH alias, mapping, manual override, real AdwTab CSS, reset, clearing, OSC 7, FQDN, reorder and transfer\n");
  return 0;
}

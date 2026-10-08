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
int main (void)
{
  adw_init ();
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
  g_object_set_data_full(G_OBJECT(tab), "tab-color-override", g_strdup("#c01c28"), g_free);
  g_autofree char *override = color_tab_value(tab, config);
  g_assert_cmpstr(override, ==, "#c01c28");
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
  adw_tab_view_set_selected_page(view, preview_page);
  g_assert_false(adw_tab_page_get_selected(page));
  g_assert_true(adw_tab_page_get_selected(preview_page));
  adw_tab_view_close_page(view, preview_page);
  g_object_set_data(G_OBJECT(tab), "tab-color-override", NULL);
  color_tab_widgets(bar, config);
  g_autofree char *automatic = color_tab_value(tab, config);
  g_assert_cmpstr(automatic, ==, "#3584e4");
  g_key_file_remove_key(config, "Hosts", "scopuli", NULL);
  color_tab_widgets(bar, config);
  colored = 0; inspect(bar);
  g_assert_cmpint(colored, ==, 0);
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
  gtk_window_destroy(GTK_WINDOW(window));
  g_object_unref(tab);
  g_print("PASS: SSH alias, mapping, manual override, real AdwTab CSS, reset, clearing, OSC 7, FQDN, reorder and transfer\n");
  return 0;
}

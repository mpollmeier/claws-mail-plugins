/* 
 * Claws Mail -- A GTK+ based, lightweight, and fast e-mail client
 * == Fancy Plugin ==
 * Copyright(C) 1999-2009 the Claws Mail Team
 * This file Copyright (C) 2009 Salvatore De Paolis
 * <iwkse@claws-mail.org>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write tothe Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <fancy_viewer.h>
#include <fancy_prefs.h>

static void 
load_start_cb( WebKitWebView *view, 
		                            gint progress, 
									FancyViewer 
									*viewer );

static void 
load_finished_cb( WebKitWebView *view, 
									 gint progress, 
									 FancyViewer *viewer );

static void 
hovering_over_link_cb( WebKitWebView *view, 
											    const gchar *wtf, 
												const gchar *link, 
												FancyViewer *viewer, 
												void *wtfa);

static void 
load_progress_cb( WebKitWebView *view, 
										  gint progress, 
										  FancyViewer *viewer);

static WebKitNavigationResponse 
navigation_requested_cb( WebKitWebView *view, 
								   WebKitWebFrame *frame, 
								   WebKitNetworkRequest *netreq, 
								   FancyViewer *viewer);

static MimeViewerFactory fancy_viewer_factory;

static gboolean	fancy_text_search(MimeViewer *_viewer, gboolean backward,
				     const gchar *str, gboolean case_sens);

static void viewer_menu_handler(GtkWidget *menuitem, FancyViewer *viewer);

/*FIXME substitute webkitwebsettings.cpp functions with their API when available */
gchar* webkit_web_view_get_selected_text(WebKitWebView* webView);
/*------*/
static GtkWidget *fancy_get_widget(MimeViewer *_viewer)
{
	FancyViewer *viewer = (FancyViewer *) _viewer;
	debug_print("fancy_get_widget: %p\n", viewer->vbox);
	viewer->load_page = FALSE;

	return GTK_WIDGET(viewer->vbox);
}

static gint fancy_show_mimepart_real(MimeViewer *_viewer)
{
	FancyViewer *viewer = (FancyViewer *) _viewer;
	
	MessageView *messageview = ((MimeViewer *)viewer)->mimeview 
					? ((MimeViewer *)viewer)->mimeview->messageview 
					: NULL;
	MimeInfo *partinfo = viewer->to_load;
	
	viewer->loading = 1;
	messageview->updating = TRUE;

	if (viewer->filename != NULL) {
		g_unlink(viewer->filename);
		g_free(viewer->filename);
		viewer->filename = NULL;
	}

	if (messageview) {
		NoticeView *noticeview = messageview->noticeview;
		noticeview_hide(noticeview);
	}
	if (partinfo)
		viewer->filename = procmime_get_tmp_file_name(partinfo);

	if (procmime_get_part(viewer->filename, partinfo) < 0) {
			g_free(viewer->filename);
	} else {
		gchar *tmp = g_filename_to_uri(viewer->filename, NULL, NULL);
		webkit_web_view_open(viewer->view, tmp);
		g_free(tmp);
	}
	return FALSE;
}

static gint fancy_show_mimepart_prepare(MimeViewer *_viewer)
{
	FancyViewer *viewer = (FancyViewer *) _viewer;

	if (viewer->tag > 0) {
		gtk_timeout_remove(viewer->tag);
		viewer->tag = -1;
		if (viewer->loading) {
			viewer->stop_previous = TRUE;
		}
	}
	
	viewer->tag = g_timeout_add(5, (GtkFunction)fancy_show_mimepart_real, viewer);
	return FALSE;
}

static void fancy_show_mimepart(MimeViewer *_viewer,
				const gchar *infile,
				MimeInfo *partinfo)
{
	FancyViewer *viewer = (FancyViewer *) _viewer;
	viewer->to_load = partinfo;
	g_timeout_add(5, (GtkFunction)fancy_show_mimepart_prepare, viewer);
}

static void fancy_clear_viewer(MimeViewer *_viewer)
{
	FancyViewer *viewer = (FancyViewer *) _viewer;
	GtkAdjustment *vadj;
	
	debug_print("fancy_clear_viewer\n");
	viewer->to_load = NULL;
	vadj = gtk_scrolled_window_get_vadjustment(
		GTK_SCROLLED_WINDOW(viewer->scrollwin));
	vadj->value = 0.0;
	g_signal_emit_by_name(G_OBJECT(vadj), "value-changed", 0);
}

static void fancy_destroy_viewer(MimeViewer *_viewer)
{
	FancyViewer *viewer = (FancyViewer *) _viewer;

	debug_print("fancy_destroy_viewer\n");

	g_free(viewer->filename);
   	g_free(viewer);
}

static gboolean	fancy_text_search(MimeViewer *_viewer, gboolean backward, 
								  const gchar *str, gboolean case_sens)
{
	return webkit_web_view_search_text(((FancyViewer*)_viewer)->view,
                                        str,
                                        case_sens,
                                        !backward,
                                         TRUE);
}

static gboolean fancy_scroll_page(MimeViewer *_viewer, gboolean up)
{
	FancyViewer *viewer = (FancyViewer *)_viewer;
	GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(
		GTK_SCROLLED_WINDOW(viewer->scrollwin));

	if (viewer->view == NULL)
		return FALSE;

	return gtkutils_scroll_page(GTK_WIDGET(viewer->view), vadj, up);
}

static void fancy_scroll_one_line(MimeViewer *_viewer, gboolean up)
{
	FancyViewer *viewer = (FancyViewer *)_viewer;
	GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(
		GTK_SCROLLED_WINDOW(viewer->scrollwin));

	if (viewer->view == NULL)
		return;

	gtkutils_scroll_one_line(GTK_WIDGET(viewer->view), vadj, up);
}

static void load_start_cb(WebKitWebView *view, gint progress, FancyViewer *viewer)
{
	gtk_widget_show(GTK_WIDGET(viewer->progress));
}

static void load_finished_cb(WebKitWebView *view, gint progress, FancyViewer *viewer)
{
	gtk_widget_hide(GTK_WIDGET(viewer->progress));
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(viewer->progress), (gdouble) 0.0);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(viewer->progress), "");
}

static void hovering_over_link_cb(WebKitWebView *view, const gchar *wtf, const gchar *link, FancyViewer *viewer, void *wtfa)
{
	gtk_label_set_text(GTK_LABEL(viewer->l_link), link);
	viewer->cur_link = link;
}

static void load_progress_cb(WebKitWebView *view, gint progress, FancyViewer *viewer)
{
	gdouble pbar;
	gchar *label = g_strdup_printf("%d%% Loading...", progress);
	pbar = (gdouble) progress / 100;
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(viewer->progress), pbar);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(viewer->progress), (const gchar*)label);
	g_free(label);
}
static WebKitNavigationResponse navigation_requested_cb(WebKitWebView *view, 
																	WebKitWebFrame *frame, 
																	WebKitNetworkRequest *netreq, 
																	FancyViewer *viewer)
{
	if (!fancy_prefs.auto_load_images) {
		g_object_set(viewer->settings, 
					"auto-load-images", FALSE, NULL);
		webkit_web_view_set_settings(viewer->view, viewer->settings);
	}
	else {
		g_object_set(viewer->settings, 
					"auto-load-images", TRUE, NULL);
		webkit_web_view_set_settings(viewer->view, viewer->settings);
	}

	if (!fancy_prefs.enable_scripts) {
		g_object_set(viewer->settings, 
					"enable-scripts", FALSE, NULL);
		webkit_web_view_set_settings(viewer->view, viewer->settings);
	}
	else {
		g_object_set(viewer->settings, 
					"enable-scripts", TRUE, NULL);
		webkit_web_view_set_settings(viewer->view, viewer->settings);
	}
	
	if (!fancy_prefs.enable_plugins) {
		g_object_set(viewer->settings, 
					"enable-plugins", FALSE, NULL);
		webkit_web_view_set_settings(viewer->view, viewer->settings);
	}
	else {
		g_object_set(viewer->settings, 
					"enable-plugins", TRUE, NULL);
		webkit_web_view_set_settings(viewer->view, viewer->settings);
	}

	if (!fancy_prefs.block_links) {
		if (viewer->load_page){
			return WEBKIT_NAVIGATION_RESPONSE_IGNORE; 
		}
		else {
			viewer->load_page = TRUE;
		}
	}

	return WEBKIT_NAVIGATION_RESPONSE_ACCEPT;
}
static void search_the_web_cb(GtkWidget *widget, FancyViewer *viewer)
{
	debug_print("Clicked on Search on Web\n");
	if (webkit_web_view_has_selection(viewer->view)) {
		gchar *search = g_strconcat(GOOGLE_SEARCH, webkit_web_view_get_selected_text(viewer->view), NULL);
		webkit_web_view_open(viewer->view, search);
		g_free(search);
	}
}

static void open_in_browser_cb(GtkWidget *widget, FancyViewer *viewer)
{
	debug_print("Open in Browser\n");
	open_uri(viewer->cur_link,prefs_common_get_uri_cmd());
}

static void download_link_cb(GtkWidget *widget, FancyViewer *viewer)
{
	debug_print("Not Yet Implemented\n");
}

static void open_image_cb(GtkWidget *widget, FancyViewer *viewer)
{
	debug_print("Not Yet Implemented\n");

}

static void save_image_cb(GtkWidget *widget, FancyViewer *viewer)
{
	debug_print("Not Yet Implemented\n");

}

static void copy_image_cb(GtkWidget *widget, FancyViewer *viewer)
{
	debug_print("Not Yet Implemented\n");

}

static void viewer_menu_handler(GtkWidget *menuitem, FancyViewer *viewer)
{
		const gchar *g_name = gtk_widget_get_name(GTK_WIDGET(menuitem));

		if (!g_ascii_strcasecmp(g_name, "GtkImageMenuItem")) {

			GtkWidget *menul = gtk_bin_get_child(GTK_BIN(menuitem));
		
			if (!g_ascii_strcasecmp(gtk_label_get_text(GTK_LABEL(menul)), 
									"Search the Web")) {
				gtk_label_set_text(GTK_LABEL(menul), _("Search the Web"));
				
				if (!fancy_prefs.block_links)
					gtk_widget_set_sensitive(GTK_WIDGET(menul), FALSE);

				GtkImageMenuItem *m_search = GTK_IMAGE_MENU_ITEM(menuitem);
				g_signal_connect(G_OBJECT(m_search), "activate",
					 G_CALLBACK(search_the_web_cb),
					 (gpointer *) viewer);
			}

			if (!g_ascii_strcasecmp(gtk_label_get_text(GTK_LABEL(menul)), 
									"Open Link in New Window" )) {
				
				gtk_label_set_text(GTK_LABEL(menul), _("Open in Browser"));
				
				GtkImageMenuItem *m_new = GTK_IMAGE_MENU_ITEM(menuitem);
				g_signal_connect(G_OBJECT(m_new), "activate",
					 G_CALLBACK(open_in_browser_cb),
					 (gpointer *) viewer);
			}
			
			if (!g_ascii_strcasecmp(gtk_label_get_text(GTK_LABEL(menul)), 
									"Open Image in New Window" )) {
				
				gtk_label_set_text(GTK_LABEL(menul), _("Open Image"));
				
				GtkImageMenuItem *m_image = GTK_IMAGE_MENU_ITEM(menuitem);
				g_signal_connect(G_OBJECT(m_image), "activate",
					 G_CALLBACK(open_image_cb),
					 (gpointer *) viewer);
			}

			if (!g_ascii_strcasecmp(gtk_label_get_text(GTK_LABEL(menul)), 
									"Copy Link Location" )) {
				gtk_label_set_text(GTK_LABEL(menul), _("Copy Link"));
/*				GtkWidget *rssyl = gtk_image_menu_item_new();
				GtkWidget *img = gtk_image_new_from_stock(GTK_STOCK_ADD, 
														  GTK_ICON_SIZE_MENU);
				gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(rssyl), img);
				GtkWidget *rssyll = gtk_bin_get_child(GTK_BIN(rssyl));
				gtk_label_set_text(GTK_LABEL(rssyll), "Import Feed");
				gtk_widget_show(GTK_WIDGET(rssyl));*/
			}
			
			if (!g_ascii_strcasecmp(gtk_label_get_text(GTK_LABEL(menul)), 
									"Download Linked File" )) {
				
				gtk_label_set_text(GTK_LABEL(menul), _("Download Link"));
				
				gtk_widget_set_sensitive(GTK_WIDGET(menul), FALSE);
				
				GtkImageMenuItem *m_dlink = GTK_IMAGE_MENU_ITEM(menuitem);
				g_signal_connect(G_OBJECT(m_dlink), "activate",
					 G_CALLBACK(download_link_cb),
					 (gpointer *) viewer);
			}

			if (!g_ascii_strcasecmp(gtk_label_get_text(GTK_LABEL(menul)), 
									"Open Image" )) {
				
				gtk_label_set_text(GTK_LABEL(menul), _("Open Image"));
				gtk_widget_set_sensitive(GTK_WIDGET(menul), FALSE);
				
				GtkImageMenuItem *m_oimage = GTK_IMAGE_MENU_ITEM(menuitem);
				g_signal_connect(G_OBJECT(m_oimage), "activate",
					 G_CALLBACK(open_image_cb),
					 (gpointer *) viewer);
			}

			if (!g_ascii_strcasecmp(gtk_label_get_text(GTK_LABEL(menul)), 
									"Save Image As" )) {
				
				gtk_label_set_text(GTK_LABEL(menul), _("Save Image As"));
				gtk_widget_set_sensitive(GTK_WIDGET(menul), FALSE);
				
				GtkImageMenuItem *m_simage = GTK_IMAGE_MENU_ITEM(menuitem);
				g_signal_connect(G_OBJECT(m_simage), "activate",
					 G_CALLBACK(save_image_cb),
					 (gpointer *) viewer);
			}

			if (!g_ascii_strcasecmp(gtk_label_get_text(GTK_LABEL(menul)), 
									"Copy Image" )) {
				
				gtk_label_set_text(GTK_LABEL(menul), _("Copy Image"));
				gtk_widget_set_sensitive(GTK_WIDGET(menul), FALSE);
				
				GtkImageMenuItem *m_cimage = GTK_IMAGE_MENU_ITEM(menuitem);
				g_signal_connect(G_OBJECT(m_cimage), "activate",
					 G_CALLBACK(copy_image_cb),
					 (gpointer *) viewer);
			}

		}
}

static gboolean populate_popup_cb (WebKitWebView *view, GtkWidget *menu, FancyViewer *viewer)
{
	/*FIXME This is an hack..until webkit does not give the proper way to handle it*/
	gtk_container_foreach(GTK_CONTAINER(menu), (GtkCallback)viewer_menu_handler, viewer);
	return TRUE;
}

static gboolean release_button_cb (WebKitWebView *view, GdkEvent *ev, gpointer data)
{
	/* Make the copy/paste works as usual  */
	if (webkit_web_view_can_copy_clipboard(view)) {
		GtkClipboard *wv_clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
		const gchar *sel_text = (const gchar*)webkit_web_view_get_selected_text(view);
		gtk_clipboard_set_text(wv_clipboard,
							   sel_text,
							   -1);
	}
	return FALSE;
}

static void zoom_100_cb(GtkWidget *widget, GdkEvent *ev, FancyViewer *viewer)
{
	gtk_widget_grab_focus(widget);
	webkit_web_view_set_zoom_level(viewer->view, 1);
}

static void zoom_in_cb(GtkWidget *widget, GdkEvent *ev, FancyViewer *viewer)
{
	gtk_widget_grab_focus(widget);
	webkit_web_view_zoom_in(viewer->view);
}
static void zoom_out_cb(GtkWidget *widget, GdkEvent *ev, FancyViewer *viewer)
{
	gtk_widget_grab_focus(widget);
	webkit_web_view_zoom_out(viewer->view);
}

static MimeViewer *fancy_viewer_create(void)
{
	FancyViewer	*viewer;
	GtkAdjustment 	*adj;
	GtkWidget		*hbox;
	
	debug_print("fancy_viewer_create\n");

	viewer = g_new0(FancyViewer, 1);
	viewer->mimeviewer.factory = &fancy_viewer_factory;
	viewer->mimeviewer.get_widget = fancy_get_widget;
	viewer->mimeviewer.show_mimepart = fancy_show_mimepart;
	viewer->mimeviewer.clear_viewer = fancy_clear_viewer;
	viewer->mimeviewer.destroy_viewer = fancy_destroy_viewer;
	viewer->mimeviewer.text_search = fancy_text_search;
	viewer->mimeviewer.scroll_page = fancy_scroll_page;
	viewer->mimeviewer.scroll_one_line = fancy_scroll_one_line;
	viewer->view = WEBKIT_WEB_VIEW(webkit_web_view_new());
	viewer->settings = webkit_web_settings_new();	
	viewer->scrollwin = gtk_scrolled_window_new(NULL, NULL);
	viewer->tag = -1;
	gtk_scrolled_window_set_policy(
			GTK_SCROLLED_WINDOW(viewer->scrollwin), 
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(
			GTK_SCROLLED_WINDOW(viewer->scrollwin),
			GTK_SHADOW_IN);
	gtk_container_add(
		GTK_CONTAINER(viewer->scrollwin),
		GTK_WIDGET(viewer->view));

	adj = gtk_scrolled_window_get_vadjustment(
		GTK_SCROLLED_WINDOW(viewer->scrollwin));
	viewer->vbox = gtk_vbox_new(FALSE, 0);
	hbox = gtk_hbox_new(FALSE, 0);
	viewer->progress = gtk_progress_bar_new();
	
	viewer->zoom_100 = gtk_image_new_from_stock(GTK_STOCK_ZOOM_100, GTK_ICON_SIZE_MENU);
	viewer->zoom_in = gtk_image_new_from_stock(GTK_STOCK_ZOOM_IN, GTK_ICON_SIZE_MENU);
	viewer->zoom_out = gtk_image_new_from_stock(GTK_STOCK_ZOOM_OUT, GTK_ICON_SIZE_MENU);

	viewer->l_link = gtk_label_new("");

	viewer->ev_zoom_100 = gtk_event_box_new();
	viewer->ev_zoom_in = gtk_event_box_new();
	viewer->ev_zoom_out = gtk_event_box_new();

	gtk_event_box_set_visible_window(GTK_EVENT_BOX(viewer->ev_zoom_100), FALSE);
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(viewer->ev_zoom_in), FALSE);
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(viewer->ev_zoom_out), FALSE);

	gtk_container_add(GTK_CONTAINER(viewer->ev_zoom_100), viewer->zoom_100);
	gtk_container_add(GTK_CONTAINER(viewer->ev_zoom_in), viewer->zoom_in);
	gtk_container_add(GTK_CONTAINER(viewer->ev_zoom_out), viewer->zoom_out);

	gtk_box_pack_start(GTK_BOX(hbox), viewer->ev_zoom_100, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(hbox), viewer->ev_zoom_in, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(hbox), viewer->ev_zoom_out, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(hbox), viewer->l_link, FALSE, FALSE, 8);
	gtk_box_pack_end(GTK_BOX(hbox), viewer->progress, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(viewer->vbox), viewer->scrollwin, TRUE, TRUE, 1);
	gtk_box_pack_start(GTK_BOX(viewer->vbox), hbox, FALSE, FALSE, 0);

	gtk_widget_show(viewer->ev_zoom_100);
	gtk_widget_show(viewer->ev_zoom_in);
	gtk_widget_show(viewer->ev_zoom_out);

	gtk_widget_show(viewer->scrollwin);
	gtk_widget_show(viewer->zoom_100);
	gtk_widget_show(viewer->zoom_in);
	gtk_widget_show(viewer->zoom_out);

	gtk_widget_show(viewer->l_link);
	gtk_widget_show(viewer->vbox);
	gtk_widget_show(hbox);
	gtk_widget_show(GTK_WIDGET(viewer->view));
	
	g_signal_connect(G_OBJECT(viewer->view), "load-started", 
					 G_CALLBACK(load_start_cb), 
					 viewer);
	g_signal_connect(G_OBJECT(viewer->view), "load-finished", 
					 G_CALLBACK(load_finished_cb), 
					 viewer);
	g_signal_connect(G_OBJECT(viewer->view), "hovering-over-link", 
					 G_CALLBACK(hovering_over_link_cb), 
					 viewer);
	g_signal_connect(G_OBJECT(viewer->view), "load-progress-changed", 
					 G_CALLBACK(load_progress_cb), 
					 viewer);
	g_signal_connect(G_OBJECT(viewer->view), "navigation-requested",
					 G_CALLBACK(navigation_requested_cb),
					 viewer);
	g_signal_connect(G_OBJECT(viewer->view), "populate-popup",
					 G_CALLBACK(populate_popup_cb),
					 viewer);
	g_signal_connect(G_OBJECT(viewer->view), "button-release-event",
					 G_CALLBACK(release_button_cb),
					 viewer);
	g_signal_connect(G_OBJECT(viewer->ev_zoom_100), "button-press-event",
					 G_CALLBACK(zoom_100_cb),
					 (gpointer*)viewer);
	g_signal_connect(G_OBJECT(viewer->ev_zoom_in), "button-press-event",
					 G_CALLBACK(zoom_in_cb),
					 (gpointer *)viewer);
	g_signal_connect(G_OBJECT(viewer->ev_zoom_out), "button-press-event",
					 G_CALLBACK(zoom_out_cb),
					 (gpointer *)viewer);

	viewer->filename = NULL;

	return (MimeViewer *) viewer;
}

static gchar *content_types[] =
	{"text/html", NULL};

static MimeViewerFactory fancy_viewer_factory =
{
	content_types,
	0,
	fancy_viewer_create,
};

gint plugin_init(gchar **error)
{
/*	fancy_viewer_tmpdir = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S,
				"fancy_viewer", NULL); */

	if (!check_plugin_version(MAKE_NUMERIC_VERSION(2,9,2,72),
				VERSION_NUMERIC, _("Fancy"), error))
		return -1;
	fancy_prefs_init();
	
	mimeview_register_viewer_factory(&fancy_viewer_factory);

/*	if (!is_dir_exist(fancy_viewer_tmpdir))
		make_dir_hier(fancy_viewer_tmpdir);*/

	return 0;
}

gboolean plugin_done(void)
{
	mimeview_unregister_viewer_factory(&fancy_viewer_factory);
	fancy_prefs_done();
	return FALSE;
}

const gchar *plugin_name(void)
{
	return _("Fancy HTML Viewer");
}

const gchar *plugin_desc(void)
{
	return g_strdup_printf("This plugin renders HTML e-mail using the WebKit %d.%d.%d "
		"library.", WEBKIT_MAJOR_VERSION, 
					WEBKIT_MINOR_VERSION, 
					WEBKIT_MICRO_VERSION);
}

const gchar *plugin_type(void)
{
	return "GTK2";
}

const gchar *plugin_licence(void)
{
	return "GPL3";
}

const gchar *plugin_version(void)
{
	return PLUGINVERSION;
}

struct PluginFeature *plugin_provides(void)
{
	static struct PluginFeature features[] = 
		{ {PLUGIN_MIMEVIEWER, "text/html"},
		  {PLUGIN_NOTHING, NULL}};
	return features;
}

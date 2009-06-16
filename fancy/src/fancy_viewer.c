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

#if USE_PRINTUNIX
#include <alertpanel.h>
#include <printing.h>
#if GTK_CHECK_VERSION(2,13,1)
#include <gtk/gtkunixprint.h>
#else
#include <gtk/gtkprintoperation.h>
#include <gtk/gtkprintjob.h>
#include <gtk/gtkprintunixdialog.h>
#endif
#endif

static void 
load_start_cb (WebKitWebView *view, gint progress, FancyViewer *viewer);

static void 
load_finished_cb (WebKitWebView *view, gint progress, FancyViewer *viewer);

static void 
over_link_cb (WebKitWebView *view, const gchar *wtf, const gchar *link, 
              FancyViewer *viewer, void *wtfa);

static void 
load_progress_cb (WebKitWebView *view, gint progress, FancyViewer *viewer);

static WebKitNavigationResponse 
navigation_requested_cb (WebKitWebView *view, WebKitWebFrame *frame, 
                         WebKitNetworkRequest *netreq, FancyViewer *viewer);

static MimeViewerFactory fancy_viewer_factory;

static gboolean    
fancy_text_search(MimeViewer *_viewer, gboolean backward, const gchar *str, 
                  gboolean case_sens);

static void 
viewer_menu_handler(GtkWidget *menuitem, FancyViewer *viewer);

static void
job_complete_cb (GtkPrintJob *print_job, FancyViewer *viewer, GError *error);

static gint keypress_events_cb (GtkWidget *widget, GdkEventKey *event,
                                FancyViewer *viewer);
static void zoom_in_cb(GtkWidget *widget, GdkEvent *ev, FancyViewer *viewer);
static void zoom_out_cb(GtkWidget *widget, GdkEvent *ev, FancyViewer *viewer);
static void zoom_100_cb(GtkWidget *widget, GdkEvent *ev, FancyViewer *viewer);

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
    debug_print("filename: %s\n", viewer->filename);
    if (!viewer->filename) {    
        return FALSE;
    }
    if (procmime_get_part(viewer->filename, partinfo) < 0) {
            g_free(viewer->filename);
    } else {
        gchar *tmp = g_filename_to_uri(viewer->filename, NULL, NULL);
        webkit_web_view_open(viewer->view, tmp);
        g_free(tmp);
    }
    viewer->loading = FALSE;
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

static void fancy_show_mimepart(MimeViewer *_viewer, const gchar *infile,
                                MimeInfo *partinfo)
{
    FancyViewer *viewer = (FancyViewer *) _viewer;
    viewer->to_load = partinfo;
    viewer->loading = TRUE;
    g_timeout_add(5, (GtkFunction)fancy_show_mimepart_prepare, viewer);
}
#if GTK_CHECK_VERSION(2,10,0) && USE_PRINTUNIX

static void
job_complete_cb (GtkPrintJob *print_job, FancyViewer *viewer, GError *error)
{
	if (error) {
		alertpanel_error(_("Printing failed:\n %s"), error->message);
	}
	viewer->printing = FALSE;
}

static void fancy_print(MimeViewer *_viewer)
{
    FancyViewer *viewer = (FancyViewer *) _viewer;
	MainWindow *mainwin = mainwindow_get_mainwindow();
	gchar *program = NULL, *cmd = NULL;
	gchar *outfile = NULL;
	gint result;
	GError *error = NULL;
	GtkWidget *dialog;
	GtkPrintUnixDialog *print_dialog;
	GtkPrinter *printer;
	GtkPrintJob *job;

	gtk_widget_realize(GTK_WIDGET(viewer->view));
    
    while (viewer->loading)
        claws_do_idle();

	debug_print("Preparing print job...\n");

	program = g_find_program_in_path("html2ps");

	if (program == NULL) {
		alertpanel_error(_("Printing HTML is only possible if the program 'html2ps' is installed."));
		return;
	}
    debug_print("filename: %s\n", viewer->filename);
	if (!viewer->filename) {
		alertpanel_error(_("Filename is null."));
		return;
	}

	outfile = get_tmp_file();
	cmd = g_strdup_printf("%s%s -o %s %s", program, 
                          fancy_prefs.auto_load_images?"":" -T", outfile, 
                          viewer->filename);

	g_free(program);

	result = execute_command_line(cmd, FALSE);
	g_free(cmd);

	if (result != 0) {
		alertpanel_error(_("Conversion to postscript failed."));
		g_free(outfile);
		return;
	}

	debug_print("Starting print job...\n");
	
	dialog = gtk_print_unix_dialog_new (_("Print"),
		  	   	 mainwin? GTK_WINDOW (mainwin->window):NULL);
	print_dialog = GTK_PRINT_UNIX_DIALOG (dialog);
	gtk_print_unix_dialog_set_page_setup (print_dialog, printing_get_page_setup());
	gtk_print_unix_dialog_set_settings (print_dialog, printing_get_settings());

	gtk_print_unix_dialog_set_manual_capabilities(print_dialog,
		                                          GTK_PRINT_CAPABILITY_GENERATE_PS);

	result = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_hide (dialog);

	printer = gtk_print_unix_dialog_get_selected_printer (print_dialog);

	if (result != GTK_RESPONSE_OK || !printer) {
		gtk_widget_destroy (dialog);
		g_free(outfile);
		return;
	}

	if (!gtk_printer_accepts_ps(printer)) {
		alertpanel_error(_("Printer %s doesn't accept PostScript files."),
			               gtk_printer_get_name(printer));
        g_free(outfile);
		return;
	}
	
	printing_store_settings(gtk_print_unix_dialog_get_settings(print_dialog));

	job = gtk_print_job_new(viewer->filename, printer, printing_get_settings(),
				            printing_get_page_setup());

	gtk_print_job_set_source_file(job, outfile, &error);

	if (error) {
		alertpanel_error(_("Printing failed:\n%s"), error->message);
		g_error_free(error);
		g_free(outfile);
		return;
	}
	
    viewer->printing = TRUE;
	
	gtk_print_job_send (job, (GtkPrintJobCompleteFunc) job_complete_cb, viewer,
		                NULL);	

	while (viewer->printing) {
		claws_do_idle();
	}

	g_free(outfile);
    
}
#endif
static gchar *fancy_get_selection (MimeViewer *_viewer)
{
    debug_print("fancy_get_selection\n");
    FancyViewer *viewer = (FancyViewer *) _viewer;
    
    if (!viewer->view)
        return NULL;
    return webkit_web_view_get_selected_text(viewer->view); 

}
static void fancy_clear_viewer(MimeViewer *_viewer)
{
    FancyViewer *viewer = (FancyViewer *) _viewer;
    GtkAdjustment *vadj;
    viewer->load_page = FALSE;    
    webkit_web_view_open(viewer->view, "about:blank");
    debug_print("fancy_clear_viewer\n");
    viewer->to_load = NULL;
    g_free(viewer->cur_link);
    vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(viewer->scrollwin));
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

static gboolean fancy_text_search(MimeViewer *_viewer, gboolean backward, 
                                     const gchar *str, gboolean case_sens)
{
    return webkit_web_view_search_text(((FancyViewer*)_viewer)->view, str,
                                         case_sens, !backward, TRUE);
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

static void load_start_cb(WebKitWebView *view, gint progress, 
                          FancyViewer *viewer)
{
    gtk_widget_show(viewer->progress);
    gtk_widget_show(viewer->ev_stop_loading);
}

static void load_finished_cb(WebKitWebView *view, gint progress, 
                             FancyViewer *viewer)
{
    gtk_widget_hide(viewer->progress);
    gtk_widget_hide(viewer->ev_stop_loading);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(viewer->progress), 
                                  (gdouble) 0.0);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(viewer->progress), "");
}

static void over_link_cb(WebKitWebView *view, const gchar *wtf, 
                         const gchar *link, FancyViewer *viewer, void *wtfa)
{
    gtk_label_set_text(GTK_LABEL(viewer->l_link), link);
    debug_print("link: %s\n", link);
    if(link) {
        if (viewer->cur_link) {
            g_free(viewer->cur_link);
            viewer->cur_link = g_strdup(link);
        } else {
            viewer->cur_link = g_strdup(link);
        }
    }
}

static void load_progress_cb(WebKitWebView *view, gint progress, 
                             FancyViewer *viewer)
{
    gdouble pbar;
    gchar *label = g_strdup_printf("%d%% Loading...", progress);
    pbar = (gdouble) progress / 100;
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(viewer->progress), pbar);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(viewer->progress), 
                              (const gchar*)label);
    g_free(label);
}
static void stop_loading_cb(GtkWidget *widget, GdkEvent *ev, 
                            FancyViewer *viewer)
{
    webkit_web_view_stop_loading (viewer->view);
    gtk_widget_hide(viewer->progress);
    gtk_widget_hide(viewer->ev_stop_loading);
}

static WebKitNavigationResponse 
navigation_requested_cb(WebKitWebView *view, WebKitWebFrame *frame, 
                        WebKitNetworkRequest *netreq, FancyViewer *viewer)
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
        gchar *search;
        gchar *tmp = webkit_web_view_get_selected_text(viewer->view);
        search = g_strconcat(GOOGLE_SEARCH, tmp, NULL);
        webkit_web_view_open(viewer->view, search);
        g_free(search);
        g_free(tmp);
    }
}

static void open_in_browser_cb(GtkWidget *widget, FancyViewer *viewer)
{
    debug_print("link: %s\n", viewer->cur_link);
    open_uri(viewer->cur_link, prefs_common_get_uri_cmd());
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
                
            if (!fancy_prefs.block_links) {
                gtk_widget_set_sensitive(GTK_WIDGET(menul), FALSE);

                GtkImageMenuItem *m_search = GTK_IMAGE_MENU_ITEM(menuitem);
                g_signal_connect(G_OBJECT(m_search), "activate",
                                 G_CALLBACK(search_the_web_cb),
                                 (gpointer *) viewer);
            }
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
/*          GtkWidget *rssyl = gtk_image_menu_item_new();
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
                             G_CALLBACK(save_image_cb), (gpointer *) viewer);
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

static gboolean populate_popup_cb (WebKitWebView *view, GtkWidget *menu, 
                                   FancyViewer *viewer)
{
    /*FIXME hack..until webkit does not give the proper way to handle it*/
    gtk_container_foreach(GTK_CONTAINER(menu), 
                          (GtkCallback)viewer_menu_handler, 
                          viewer);
    return TRUE;
}

static gint keypress_events_cb (GtkWidget *widget, GdkEventKey *event,
                                FancyViewer *viewer)
{
    if (event->state == CTRL_KEY) {
    switch (event->keyval) {
    case GDK_plus:
        zoom_in_cb(viewer->ev_zoom_in, NULL, viewer);
        break;
    case GDK_period:
        zoom_100_cb(viewer->ev_zoom_100, NULL, viewer);
        break;
    case GDK_minus:
        zoom_out_cb(viewer->ev_zoom_out, NULL, viewer);
        break;
    }
    }
    return FALSE;
}

static gboolean release_button_cb (WebKitWebView *view, GdkEvent *ev, 
                                   gpointer data)
{
    /* Make the copy/paste works as usual  */
    if (webkit_web_view_can_copy_clipboard(view)) {
        GtkClipboard *wv_clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
        const gchar *sel_text;
        sel_text = (const gchar*)webkit_web_view_get_selected_text(view);
        gtk_clipboard_set_text(wv_clipboard, sel_text, -1);
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
    FancyViewer    *viewer;
    GtkAdjustment  *adj;
    GtkWidget      *hbox;
    
    debug_print("fancy_viewer_create\n");

    viewer = g_new0(FancyViewer, 1);
    viewer->mimeviewer.factory = &fancy_viewer_factory;
    viewer->mimeviewer.get_widget = fancy_get_widget;
    viewer->mimeviewer.get_selection = fancy_get_selection;
    viewer->mimeviewer.show_mimepart = fancy_show_mimepart;
#if GTK_CHECK_VERSION(2,10,0) && USE_PRINTUNIX
    viewer->mimeviewer.print = fancy_print;
#endif
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
    
    viewer->zoom_100 = gtk_image_new_from_stock(GTK_STOCK_ZOOM_100, 
                                                GTK_ICON_SIZE_MENU);
    viewer->zoom_in = gtk_image_new_from_stock(GTK_STOCK_ZOOM_IN, 
                                               GTK_ICON_SIZE_MENU);
    viewer->zoom_out = gtk_image_new_from_stock(GTK_STOCK_ZOOM_OUT, 
                                                GTK_ICON_SIZE_MENU);
    viewer->stop_loading = gtk_image_new_from_stock(GTK_STOCK_CANCEL, 
                                                    GTK_ICON_SIZE_MENU);

    viewer->l_link = gtk_label_new("");

    viewer->ev_zoom_100 = gtk_event_box_new();
    viewer->ev_zoom_in = gtk_event_box_new();
    viewer->ev_zoom_out = gtk_event_box_new();
    viewer->ev_stop_loading = gtk_event_box_new();


    gtk_event_box_set_visible_window(GTK_EVENT_BOX(viewer->ev_zoom_100), 
                                     FALSE);
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(viewer->ev_zoom_in), 
                                     FALSE);
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(viewer->ev_zoom_out), 
                                     FALSE);
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(viewer->ev_stop_loading), 
                                     FALSE);

    gtk_container_add(GTK_CONTAINER(viewer->ev_zoom_100), viewer->zoom_100);
    gtk_container_add(GTK_CONTAINER(viewer->ev_zoom_in), viewer->zoom_in);
    gtk_container_add(GTK_CONTAINER(viewer->ev_zoom_out), viewer->zoom_out);
    gtk_container_add(GTK_CONTAINER(viewer->ev_stop_loading), 
                                    viewer->stop_loading);

    gtk_box_pack_start(GTK_BOX(hbox), viewer->ev_zoom_100, FALSE, FALSE, 1);
    gtk_box_pack_start(GTK_BOX(hbox), viewer->ev_zoom_in, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(hbox), viewer->ev_zoom_out, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(hbox), viewer->l_link, FALSE, FALSE, 8);
    gtk_box_pack_end(GTK_BOX(hbox), viewer->progress, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), viewer->ev_stop_loading, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(viewer->vbox), viewer->scrollwin, TRUE, TRUE, 
                       1);
    gtk_box_pack_start(GTK_BOX(viewer->vbox), hbox, FALSE, FALSE, 0);

    gtk_widget_show(viewer->ev_zoom_100);
    gtk_widget_show(viewer->ev_zoom_in);
    gtk_widget_show(viewer->ev_zoom_out);

    gtk_widget_show(viewer->scrollwin);
    gtk_widget_show(viewer->zoom_100);
    gtk_widget_show(viewer->zoom_in);
    gtk_widget_show(viewer->zoom_out);
    gtk_widget_show(viewer->stop_loading);

    gtk_widget_show(viewer->l_link);
    gtk_widget_show(viewer->vbox);
    gtk_widget_show(hbox);
    gtk_widget_show(GTK_WIDGET(viewer->view));
    
    g_signal_connect(G_OBJECT(viewer->view), "load-started", 
                     G_CALLBACK(load_start_cb), viewer);
    g_signal_connect(G_OBJECT(viewer->view), "load-finished", 
                     G_CALLBACK(load_finished_cb), viewer);
    g_signal_connect(G_OBJECT(viewer->view), "hovering-over-link", 
                     G_CALLBACK(over_link_cb), viewer);
    g_signal_connect(G_OBJECT(viewer->view), "load-progress-changed", 
                     G_CALLBACK(load_progress_cb), viewer);
    g_signal_connect(G_OBJECT(viewer->view), "navigation-requested",
                     G_CALLBACK(navigation_requested_cb), viewer);
    g_signal_connect(G_OBJECT(viewer->view), "populate-popup",
                     G_CALLBACK(populate_popup_cb), viewer);
    g_signal_connect(G_OBJECT(viewer->view), "button-release-event",
                     G_CALLBACK(release_button_cb), viewer);
    g_signal_connect(G_OBJECT(viewer->ev_zoom_100), "button-press-event",
                     G_CALLBACK(zoom_100_cb), (gpointer*)viewer);
    g_signal_connect(G_OBJECT(viewer->ev_zoom_in), "button-press-event",
                     G_CALLBACK(zoom_in_cb), (gpointer *)viewer);
    g_signal_connect(G_OBJECT(viewer->ev_zoom_out), "button-press-event",
                     G_CALLBACK(zoom_out_cb), (gpointer *)viewer);
    g_signal_connect(G_OBJECT(viewer->ev_stop_loading), "button-press-event",
                     G_CALLBACK(stop_loading_cb), viewer);
    g_signal_connect(G_OBJECT(viewer->view), "key_press_event",
                     G_CALLBACK(keypress_events_cb), viewer);

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
/*    fancy_viewer_tmpdir = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S,
                "fancy_viewer", NULL); */

    if (!check_plugin_version(MAKE_NUMERIC_VERSION(2,9,2,72),
                              VERSION_NUMERIC, _("Fancy"), error))
        return -1;
    fancy_prefs_init();
    
    mimeview_register_viewer_factory(&fancy_viewer_factory);

/*    if (!is_dir_exist(fancy_viewer_tmpdir))
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
    return g_strdup_printf("This plugin renders HTML e-mail using the WebKit" 
                           "%d.%d.%d library.", WEBKIT_MAJOR_VERSION, 
                           WEBKIT_MINOR_VERSION, WEBKIT_MICRO_VERSION);
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
        {{PLUGIN_MIMEVIEWER, "text/html"},
         {PLUGIN_NOTHING, NULL}};
    return features;
}

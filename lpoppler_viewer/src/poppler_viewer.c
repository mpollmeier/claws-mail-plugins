/*
 * Claws Mail -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2007 the Claws Mail Team
 * This file Copyright (C) 2007 Salvatore De Paolis 
 * <depaolis.salvatore@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include "gettext.h"
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <poppler.h>
#include "claws.h"
#include "version.h"
#include "main.h"
#include "plugin.h"
#include "utils.h"
#include "mimeview.h"
#include "messageview.h"
#include "prefs_common.h"
#include "log.h"
#include "codeconv.h"
#include "menu.h"
#include "defs.h"
#include "noticeview.h"

#ifdef USE_PTHREAD
#include <pthread.h>
#endif

static void poppler_pdf_view_update(MimeViewer *_viewer, gboolean reload_file, int page_num);
struct _PopplerViewer
{
	MimeViewer	 mimeviewer;
	GtkWidget	*vbox;
	GtkWidget	*pdf_view;
	GtkWidget	*scrollwin;
	PopplerDocument	*pdf_doc;
	PopplerPage	*pdf_page;
	gchar		*filename;
	gchar		*fsname;
	double		width;
	double		height;
	MimeInfo	*mimeinfo;
	MimeInfo	*to_load;
	gint		 cur_page;
	gint		 num_pages;
	NoticeView	*navigation;
};

typedef struct _PopplerViewer PopplerViewer;

static MimeViewerFactory poppler_viewer_factory;

static GtkWidget *poppler_get_widget(MimeViewer *_viewer)
{
	PopplerViewer *viewer = (PopplerViewer *) _viewer;

	debug_print("poppler_get_widget: %p\n", viewer->vbox);

	return GTK_WIDGET(viewer->vbox);
}

static void poppler_prev_page(NoticeView *noticeview, PopplerViewer *viewer)
{
	GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(
		GTK_SCROLLED_WINDOW(viewer->scrollwin));
	if (viewer->cur_page > 1) {
		poppler_pdf_view_update((MimeViewer *)viewer, FALSE, viewer->cur_page + 1);
		vadj = gtk_scrolled_window_get_vadjustment(
			GTK_SCROLLED_WINDOW(viewer->scrollwin));
		vadj->value = 0.0;
		g_signal_emit_by_name(G_OBJECT(vadj), "value-changed", 0);
	}
}

static void poppler_next_page(NoticeView *noticeview, PopplerViewer *viewer)
{
	GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(
		GTK_SCROLLED_WINDOW(viewer->scrollwin));
	if (viewer->cur_page < viewer->num_pages) {
		poppler_pdf_view_update((MimeViewer *)viewer, FALSE, viewer->cur_page + 1);
		vadj = gtk_scrolled_window_get_vadjustment(
			GTK_SCROLLED_WINDOW(viewer->scrollwin));
		vadj->value = 0.0;
		g_signal_emit_by_name(G_OBJECT(vadj), "value-changed", 0);
	}
}

static void poppler_pdf_view_update(MimeViewer *_viewer, gboolean reload_file, int page_num) {

	PopplerViewer *viewer = (PopplerViewer *) _viewer;
	GError *error;
	GdkPixbuf *pb;
	gchar *page_str = g_strdup_printf("%d", page_num);
	gchar *notice_text;

	NoticeView *noticeview = viewer->navigation;
	
	error = NULL;
	
	if (reload_file) {
		viewer->pdf_doc = poppler_document_new_from_file( viewer->fsname, NULL, &error);
	} 

	g_return_if_fail(viewer->pdf_doc != NULL);

	if (reload_file) {
		viewer->num_pages = poppler_document_get_n_pages( viewer->pdf_doc);
	}
	viewer->pdf_page = poppler_document_get_page_by_label (viewer->pdf_doc, page_str);
	g_free(page_str);
	if (viewer->pdf_page == NULL) {
		g_warning ("Page not found");
		return;
	}

	viewer->cur_page = page_num;

	poppler_page_get_size(viewer->pdf_page, &viewer->width, &viewer->height);

	pb = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, viewer->width, viewer->height);
	gdk_pixbuf_fill (pb, 0x00106000);
	poppler_page_render_to_pixbuf (viewer->pdf_page, 0, 0, viewer->width, viewer->height, 1, 0, pb);
	gtk_image_set_from_pixbuf(GTK_IMAGE(viewer->pdf_view),
				  pb);
	
	notice_text = g_strdup_printf(_("PDF document, page %d/%d"), viewer->cur_page, viewer->num_pages);
	
	if (noticeview)
		noticeview_set_text(noticeview, notice_text);
	g_free(notice_text);
	if (noticeview && reload_file) {
		noticeview_set_icon(noticeview, STOCK_PIXMAP_MIME_TEXT_PLAIN);
		noticeview_set_button_text(noticeview, _("Prev page"));
		noticeview_set_button_press_callback(noticeview,
			     G_CALLBACK(poppler_prev_page), (gpointer)viewer);
		noticeview_set_2ndbutton_text(noticeview, _("Next page"));
		noticeview_set_2ndbutton_press_callback(noticeview,
			     G_CALLBACK(poppler_next_page), (gpointer)viewer);
		noticeview_set_icon_clickable(noticeview, FALSE);
		noticeview_show(noticeview);
	} 
}

static gint poppler_show_mimepart_real(MimeViewer *_viewer)
{
	PopplerViewer *viewer = (PopplerViewer *) _viewer;
	gchar buf[4096];
	const gchar *charset = NULL;
	MessageView *messageview = ((MimeViewer *)viewer)->mimeview 
					? ((MimeViewer *)viewer)->mimeview->messageview 
					: NULL;
	MimeInfo *partinfo = viewer->to_load;

	memset(buf, 0, sizeof(buf));

	noticeview_set_icon(viewer->navigation, STOCK_PIXMAP_MIME_TEXT_PLAIN);
	noticeview_set_text(viewer->navigation, _("Loading..."));
	noticeview_set_button_text(viewer->navigation, NULL);
	noticeview_set_2ndbutton_text(viewer->navigation, NULL);
	noticeview_set_icon_clickable(viewer->navigation, FALSE);

	messageview->updating = TRUE;
	debug_print("poppler_show_mimepart\n");

	if (viewer->filename != NULL) {
		g_unlink(viewer->filename);
		g_free(viewer->filename);
		viewer->filename = NULL;
	}

	viewer->mimeinfo = NULL;
	
	if (partinfo) {
	    viewer->filename = procmime_get_tmp_file_name(partinfo);
	    viewer->fsname = g_strconcat("file://", viewer->filename, NULL);
	}
	
	if (partinfo && !(procmime_get_part(viewer->filename, partinfo) < 0)) {

		if (_viewer && _viewer->mimeview &&
		    _viewer->mimeview->messageview->forced_charset)
			charset = _viewer->mimeview->messageview->forced_charset;
		else
			charset = procmime_mimeinfo_get_parameter(partinfo, "charset");
		if (charset == NULL)
			charset = conv_get_locale_charset_str();

		debug_print("using charset %s\n", charset);
		
		viewer->mimeinfo = partinfo;
	}
	
	poppler_pdf_view_update(_viewer, TRUE, 1);

	messageview->updating = FALSE;
	return FALSE;
}
static void poppler_show_mimepart(MimeViewer *_viewer,
				const gchar *infile,
				MimeInfo *partinfo)
{
	PopplerViewer *viewer = (PopplerViewer *) _viewer;
	viewer->to_load = partinfo;
	gtk_timeout_add(5, (GtkFunction)poppler_show_mimepart_real, viewer);
	
}

static void poppler_clear_viewer(MimeViewer *_viewer)
{
	PopplerViewer *viewer = (PopplerViewer *) _viewer;
	GtkAdjustment *vadj;
	
	debug_print("poppler_clear_viewer\n");
	viewer->to_load = NULL;
	
	vadj = gtk_scrolled_window_get_vadjustment(
		GTK_SCROLLED_WINDOW(viewer->scrollwin));
	vadj->value = 0.0;
	g_signal_emit_by_name(G_OBJECT(vadj), "value-changed", 0);
}

static void poppler_destroy_viewer(MimeViewer *_viewer)
{
	PopplerViewer *viewer = (PopplerViewer *) _viewer;

	debug_print("poppler_destroy_viewer\n");

	gtk_widget_unref(GTK_WIDGET(viewer->vbox));
	gtk_widget_unref(GTK_WIDGET(viewer->pdf_view));
	gtk_widget_unref(GTK_WIDGET(viewer->scrollwin));
	g_unlink(viewer->filename);
	g_free(viewer->filename);
    	g_free(viewer);
}

static gboolean poppler_scroll_page(MimeViewer *_viewer, gboolean up)
{
	PopplerViewer *viewer = (PopplerViewer *)_viewer;
	GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(
		GTK_SCROLLED_WINDOW(viewer->scrollwin));

	if (viewer->pdf_view == NULL)
		return FALSE;

	if (!gtkutils_scroll_page(viewer->pdf_view, vadj, up)) {
		if (!up && viewer->cur_page < viewer->num_pages) {
			poppler_pdf_view_update(_viewer, FALSE, viewer->cur_page + 1);
			vadj = gtk_scrolled_window_get_vadjustment(
				GTK_SCROLLED_WINDOW(viewer->scrollwin));
			vadj->value = 0.0;
			g_signal_emit_by_name(G_OBJECT(vadj), "value-changed", 0);
			return TRUE;
		} else if (up && viewer->cur_page > 1) {
			poppler_pdf_view_update(_viewer, FALSE, viewer->cur_page - 1);
			vadj = gtk_scrolled_window_get_vadjustment(
				GTK_SCROLLED_WINDOW(viewer->scrollwin));
			vadj->value = vadj->upper - vadj->page_size;
			g_signal_emit_by_name(G_OBJECT(vadj), "value-changed", 0);
			return TRUE;
		} 
			return FALSE;
	} else
		return TRUE;
}

static void poppler_scroll_one_line(MimeViewer *_viewer, gboolean up)
{
	PopplerViewer *viewer = (PopplerViewer *)_viewer;
	GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(
		GTK_SCROLLED_WINDOW(viewer->scrollwin));

	if (viewer->pdf_view == NULL)
		return;

	gtkutils_scroll_one_line(viewer->pdf_view, vadj, up);
}

static MimeViewer *poppler_viewer_create(void)
{
	PopplerViewer *viewer;
	
	debug_print("poppler_viewer_create\n");

	viewer = g_new0(PopplerViewer, 1);
	viewer->mimeviewer.factory = &poppler_viewer_factory;
	viewer->mimeviewer.get_widget = poppler_get_widget;
	viewer->mimeviewer.show_mimepart = poppler_show_mimepart;
	viewer->mimeviewer.clear_viewer = poppler_clear_viewer;
	viewer->mimeviewer.destroy_viewer = poppler_destroy_viewer;
	viewer->mimeviewer.scroll_page = poppler_scroll_page;
	viewer->mimeviewer.scroll_one_line = poppler_scroll_one_line;
	viewer->scrollwin = gtk_scrolled_window_new(NULL, NULL);
	viewer->mimeinfo  = NULL;
	viewer->navigation  = noticeview_create(mainwindow_get_mainwindow());
	viewer->pdf_view = gtk_image_new();
	viewer->vbox = gtk_vbox_new(FALSE, 4);

	gtk_scrolled_window_set_policy(
			GTK_SCROLLED_WINDOW(viewer->scrollwin), 
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_scrolled_window_set_shadow_type(
			GTK_SCROLLED_WINDOW(viewer->scrollwin),
			GTK_SHADOW_IN);

	gtk_scrolled_window_add_with_viewport(
		GTK_SCROLLED_WINDOW(viewer->scrollwin),
		viewer->pdf_view);

	gtk_box_pack_start(GTK_BOX(viewer->vbox), GTK_WIDGET_PTR(viewer->navigation), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(viewer->vbox), viewer->scrollwin, TRUE, TRUE, 0);
	
	noticeview_set_icon(viewer->navigation, STOCK_PIXMAP_MIME_TEXT_PLAIN);
	noticeview_set_text(viewer->navigation, _("Loading..."));
	noticeview_set_button_text(viewer->navigation, NULL);
	noticeview_set_2ndbutton_text(viewer->navigation, NULL);
	noticeview_set_icon_clickable(viewer->navigation, FALSE);
	noticeview_show(viewer->navigation);

	gtk_widget_show(GTK_WIDGET(viewer->vbox));
	gtk_widget_ref(GTK_WIDGET(viewer->vbox));

	gtk_widget_show(GTK_WIDGET(viewer->scrollwin));
	gtk_widget_ref(GTK_WIDGET(viewer->scrollwin));
	
	gtk_widget_show(GTK_WIDGET(viewer->pdf_view));
	gtk_widget_ref(GTK_WIDGET(viewer->pdf_view));
	
	viewer->filename = NULL;
	return (MimeViewer *) viewer;
}

static gchar *content_types[] =
	{"application/pdf", NULL};

static MimeViewerFactory poppler_viewer_factory =
{
	content_types,
	0,
	poppler_viewer_create,
};

gint plugin_init(gchar **error)
{
	bindtextdomain(TEXTDOMAIN, LOCALEDIR);
	bind_textdomain_codeset(TEXTDOMAIN, "UTF-8");

	if (!check_plugin_version(MAKE_NUMERIC_VERSION(2, 7, 2, 2),
		    VERSION_NUMERIC, "PDF Viewer", error))
		return -1;
	mimeview_register_viewer_factory(&poppler_viewer_factory);
	return 0;
}

void plugin_done(void)
{
	mimeview_unregister_viewer_factory(&poppler_viewer_factory);
}

const gchar *plugin_name(void)
{
	return _("PDF Viewer");
}

const gchar *plugin_desc(void)
{
	return _("This plugin enables the viewing of PDF attachments using "
		 "the Poppler lib.");
}

const gchar *plugin_type(void)
{
	return "GTK2";
}

const gchar *plugin_licence(void)
{
	return "GPL";
}

const gchar *plugin_version(void)
{
	return PLUGINVERSION;
}

struct PluginFeature *plugin_provides(void)
{
	static struct PluginFeature features[] = 
		{ {PLUGIN_MIMEVIEWER, "application/pdf"},
		  {PLUGIN_NOTHING, NULL} };
	return features;
}

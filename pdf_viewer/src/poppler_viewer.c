/*
 * Claws Mail -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2007 the Claws Mail Team
 * This file Copyright (C) 2007 Salvatore De Paolis 
 * <iwkse@claws-mail.org> 
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
#include <gdk/gdkkeysyms.h>
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
#include "gtkutils.h"
#include "alertpanel.h"

#include "left_arrow.xpm"
#include "right_arrow.xpm"
#include "zoom_fit.xpm"
#include "zoom_in.xpm"
#include "zoom_out.xpm"
#include "zoom_width.xpm"
#include "rotate_right.xpm"
#include "rotate_left.xpm"
#include "doc_info.xpm"

#ifdef USE_PTHREAD
#include <pthread.h>
#endif

#define ZOOM_FACTOR 0.3
#define ROTATION 90 

static void poppler_pdf_view_update(MimeViewer *_viewer, gboolean reload_file, int page_num);

static gchar *msg = NULL;

struct _PopplerViewer
{
	MimeViewer	 mimeviewer;
	GtkWidget	*vsep;
	GtkWidget	*vsep1;
	GtkWidget	*vbox;
	GtkWidget	*pdf_view;
	GtkWidget	*scrollwin;
	GtkWidget	*icon_type;
	GdkPixmap	*icon_pixmap;
	GdkBitmap	*icon_bitmap;
	GtkWidget	*doc_label;
	GtkWidget	*cur_page;
	GtkWidget	*num_pages;
	/* begin GtkButtons */
	GtkWidget	*prev;
	GtkWidget	*next;
	GtkWidget	*zoom_in;
	GtkWidget	*zoom_out;
	GtkWidget	*zoom_fit;
	GtkWidget	*zoom_width;
	GtkWidget	*rotate_left;
	GtkWidget	*rotate_right;
	GtkWidget	*doc_info;
	/* end GtkButtons */
	GtkTable	*table_doc_info;
	GtkWidget	*hbox;
	GtkTooltips	*button_bar_tips;
	PopplerDocument	*pdf_doc;
	PopplerPage	*pdf_page;
	gchar		*filename;
	gchar		*fsname;
	gchar		*doc_info_text;
	gint		rotate;
	double		zoom;
	double		width;
	double		height;
	MimeInfo	*mimeinfo;
	MimeInfo	*to_load;
};

typedef enum {
	TYPE_UNKNOWN,
	TYPE_PDF,
	TYPE_PS
} FileType;

typedef struct _PopplerViewer PopplerViewer;

static MimeViewerFactory poppler_viewer_factory;

static GtkWidget *poppler_get_widget(MimeViewer *_viewer)
{
	PopplerViewer *viewer = (PopplerViewer *) _viewer;
	debug_print("poppler_get_widget: %p\n", viewer->vbox);

	return GTK_WIDGET(viewer->vbox);
}

static void poppler_button_prev_page_cb(GtkButton *button, PopplerViewer *viewer)
{
    poppler_pdf_view_update((MimeViewer *)viewer, FALSE, gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(viewer->cur_page)) - 1);
}

static void poppler_button_next_page_cb(GtkButton *button, PopplerViewer *viewer)
{
    poppler_pdf_view_update((MimeViewer *)viewer, FALSE, gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(viewer->cur_page)) + 1);
}

static void poppler_spin_change_page_cb(GtkSpinButton *button, PopplerViewer *viewer)
{
    poppler_pdf_view_update((MimeViewer *)viewer, FALSE, gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(viewer->cur_page)));
}

static void poppler_button_zoom_in_cb(GtkButton *button, PopplerViewer *viewer)
{
    if (viewer->zoom < 4) {
	viewer->zoom += ZOOM_FACTOR;
	poppler_pdf_view_update((MimeViewer *)viewer, FALSE, gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(viewer->cur_page)));
    }
}

static void poppler_button_zoom_out_cb(GtkButton *button, PopplerViewer *viewer)
{
    if (viewer->zoom > 0.3) {
	viewer->zoom -= ZOOM_FACTOR;
	poppler_pdf_view_update((MimeViewer *)viewer, FALSE, gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(viewer->cur_page)));
    }
}

static void poppler_button_zoom_fit_cb(GtkButton *button, PopplerViewer *viewer)
{
    GtkAllocation *allocation;
    double xratio, yratio;
    allocation = &(viewer->scrollwin->allocation);
    debug_print("width: %d\n", allocation->width);
    debug_print("height: %d\n", allocation->height);
    xratio = allocation->width / viewer->width;
    yratio = allocation->height / viewer->height;
    
    if (xratio >= yratio)
    	viewer->zoom = yratio;
    else
    	viewer->zoom = xratio;
    poppler_pdf_view_update((MimeViewer *)viewer, FALSE, gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(viewer->cur_page)));
}

static void poppler_button_zoom_width_cb(GtkButton *button, PopplerViewer *viewer)
{
    GtkAllocation *allocation;
    double xratio;
    allocation = &(viewer->scrollwin->allocation);
    debug_print("width: %d\n", allocation->width);
    xratio = allocation->width / viewer->width;
    viewer->zoom = xratio;
    poppler_pdf_view_update((MimeViewer *)viewer, FALSE, gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(viewer->cur_page)));
}

static void poppler_button_rotate_right_cb(GtkButton *button, PopplerViewer *viewer)
{
    if (viewer->rotate == 360)
	viewer->rotate = 0;
    	
	viewer->rotate += (gint) ROTATION;
	poppler_pdf_view_update((MimeViewer *)viewer, FALSE, gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(viewer->cur_page)));
}

static void poppler_button_rotate_left_cb(GtkButton *button, PopplerViewer *viewer)
{
    if (viewer->rotate == 0)
	viewer->rotate = 360;

    viewer->rotate = abs(viewer->rotate - (gint) ROTATION);
    poppler_pdf_view_update((MimeViewer *)viewer, FALSE, gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(viewer->cur_page)));
}

static const char * poppler_get_document_info_mode(PopplerPageMode mode)
{
    GEnumValue *enum_value;

    enum_value = g_enum_get_value((GEnumClass *) g_type_class_peek (POPPLER_TYPE_PAGE_MODE), mode);
    return (gchar *) enum_value->value_name;
}
static const char * poppler_get_document_info_layout(PopplerPageLayout layout)
{

    GEnumValue *enum_value;

    enum_value = g_enum_get_value((GEnumClass *) g_type_class_peek (POPPLER_TYPE_PAGE_LAYOUT), layout);
    return (gchar *) enum_value->value_name;
}
static char * poppler_get_document_format_data(GTime utime) 
{
	time_t time = (time_t) utime;
	struct tm t;
	char s[256];
	const char *fmt_hack = "%c";
	size_t len;

	if (time == 0 || !localtime_r (&time, &t)) return NULL;

	len = strftime (s, sizeof (s), fmt_hack, &t);
	if (len == 0 || s[0] == '\0') return NULL;

	return g_locale_to_utf8 (s, -1, NULL, NULL, NULL);
}

static GtkTable * poppler_fill_info_table(PopplerViewer *viewer)
{
    GtkWidget *label;
    gchar *title, *format, *author, *subject, *keywords, *creator, *producer, *linearized, *tmp;
    
    GTime creation_date, mod_date;

    PopplerPageLayout layout;
    PopplerPageMode mode;
    PopplerPermissions permissions;
    PopplerViewerPreferences view_prefs;

    g_object_get (viewer->pdf_doc,
	"title", &title,
	"format", &format,
	"author", &author,
	"subject", &subject,
	"keywords", &keywords,
	"creation-date", &creation_date,
	"permissions", &permissions,
	"mod-date", &mod_date,
	"creator", &creator,
	"producer", &producer,	
	"linearized", &linearized,
	"page-mode", &mode,
	"page-layout", &layout,
	"viewer-preferences", &view_prefs,
	NULL);
	
    viewer->table_doc_info = GTK_TABLE(gtk_table_new(12, 2, FALSE));

    label = gtk_label_new(_("Title:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 4, 0);
    gtk_table_attach(viewer->table_doc_info, label, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    label = gtk_label_new(title);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 4, 0);
    gtk_table_attach(viewer->table_doc_info, label, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    label = gtk_label_new(_("Subject:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 4, 0);
    gtk_table_attach(viewer->table_doc_info, label, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    label = gtk_label_new(subject);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 4, 0);
    gtk_table_attach(viewer->table_doc_info, label, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    label = gtk_label_new(_("Author:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 4, 0);
    gtk_table_attach(viewer->table_doc_info, label, 0, 1, 2, 3, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    label = gtk_label_new(author);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 4, 0);
    gtk_table_attach(viewer->table_doc_info, label, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    label = gtk_label_new(_("Keywords:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 4, 0);
    gtk_table_attach(viewer->table_doc_info, label, 0, 1, 3, 4, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    label = gtk_label_new(keywords);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 4, 0);
    gtk_table_attach(viewer->table_doc_info, label, 1, 2, 3, 4, GTK_EXPAND | GTK_FILL, 0, 0, 0);
    
    label = gtk_label_new(_("Creator:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 4, 0);
    gtk_table_attach(viewer->table_doc_info, label, 0, 1, 4, 5, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    label = gtk_label_new(creator);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 4, 0);
    gtk_table_attach(viewer->table_doc_info, label, 1, 2, 4, 5, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    label = gtk_label_new(_("Producer:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 4, 0);
    gtk_table_attach(viewer->table_doc_info, label, 0, 1, 5, 6, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    label = gtk_label_new(producer);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 4, 0);
    gtk_table_attach(viewer->table_doc_info, label, 1, 2, 5, 6, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    label = gtk_label_new(_("Created:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 4, 0);
    gtk_table_attach(viewer->table_doc_info, label, 0, 1, 6, 7, GTK_EXPAND | GTK_FILL, 0, 0, 0);
    
    tmp = poppler_get_document_format_data(creation_date);
    label = gtk_label_new(tmp);
    g_free(tmp);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 4, 0);
    gtk_table_attach(viewer->table_doc_info, label, 1, 2, 6, 7, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    label = gtk_label_new(_("Modified:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 4, 0);
    gtk_table_attach(viewer->table_doc_info, label, 0, 1, 7, 8, GTK_EXPAND | GTK_FILL, 0, 0, 0);
    
    tmp = poppler_get_document_format_data(mod_date);
    label = gtk_label_new(tmp);
    g_free(tmp);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 4, 0);
    gtk_table_attach(viewer->table_doc_info, label, 1, 2, 7, 8, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    label = gtk_label_new(_("Format:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 4, 0);
    gtk_table_attach(viewer->table_doc_info, label, 0, 1, 8, 9, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    label = gtk_label_new(format);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 4, 0);
    gtk_table_attach(viewer->table_doc_info, label, 1, 2, 8, 9, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    label = gtk_label_new(_("Optimized:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 4, 0);
    gtk_table_attach(viewer->table_doc_info, label, 0, 1, 9, 10, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    label = gtk_label_new(linearized);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 4, 0);
    gtk_table_attach(viewer->table_doc_info, label, 1, 2, 9, 10, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    label = gtk_label_new(_("Page Mode:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 4, 0);
    gtk_table_attach(viewer->table_doc_info, label, 0, 1, 10, 11, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    label = gtk_label_new(poppler_get_document_info_mode(mode));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 4, 0);
    gtk_table_attach(viewer->table_doc_info, label, 1, 2, 10, 11, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    label = gtk_label_new(_("Page Layout:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 4, 0);
    gtk_table_attach(viewer->table_doc_info, label, 0, 1, 11, 12, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    label = gtk_label_new(poppler_get_document_info_layout(layout));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 4, 0);
    gtk_table_attach(viewer->table_doc_info, label, 1, 2, 11, 12, GTK_EXPAND | GTK_FILL, 0, 0, 0);

	g_free(title);
	g_free(format);
	g_free(author);
	g_free(subject);
	g_free(keywords);
	g_free(creator);
	g_free(producer);
	g_free(linearized);

	return (GtkTable *) viewer->table_doc_info;
}

static void poppler_button_document_info_cb(GtkButton *button, PopplerViewer *viewer)
{
  gchar *buf;
  buf = g_strdup_printf(_("PDF Viewer Plugin"));

  alertpanel_full(buf, NULL, GTK_STOCK_CLOSE, NULL, NULL,
		FALSE, (GtkWidget *) poppler_fill_info_table(viewer), ALERT_NOTICE, G_ALERTDEFAULT);
  g_free(buf);
}

/*static gboolean scrollwin_key_up_cb(GtkWidget *widg, GdkEventKey *event, PopplerViewer *viewer)
{

    if(event != NULL && event->keyval == GDK_Up) 

    return FALSE;
}

static gboolean scrollwin_key_down_cb(GtkWidget *widg, GdkEventKey *event, PopplerViewer *viewer)
{

    if(event != NULL && event->keyval == GDK_Down) 

    return FALSE;
}

static gboolean scrollwin_page_up_cb(GtkWidget *widg, GdkEventKey *event, PopplerViewer *viewer)
{
    debug_print("key_event_press %s", event->string);
    if(event != NULL && event->keyval == GDK_Page_Up) {
	if (viewer->cur_page_int > 1) 
	    poppler_pdf_view_update((MimeViewer *)viewer, FALSE, viewer->cur_page_int - 1);

    }

    return FALSE;
}

static gboolean scrollwin_page_down_cb(GtkWidget *widg, GdkEventKey *event, PopplerViewer *viewer)
{
    if(event != NULL && event->keyval == GDK_Page_Down)
	if (viewer->cur_page_int < viewer->num_pages_int) 
	    poppler_pdf_view_update((MimeViewer *)viewer, FALSE, viewer->cur_page_int + 1);

    return FALSE;
}
*/
static FileType poppler_mimepart_get_type(MimeInfo *partinfo)
{
	gchar *content_type = NULL;
	FileType type = TYPE_UNKNOWN;
	if ((partinfo->type == MIMETYPE_APPLICATION) &&
            (!g_ascii_strcasecmp(partinfo->subtype, "octet-stream"))) {
		const gchar *filename;

		filename = procmime_mimeinfo_get_parameter(partinfo, "filename");
		if (filename == NULL)
			filename = procmime_mimeinfo_get_parameter(partinfo, "name");
		if (filename != NULL)
			content_type = procmime_get_mime_type(filename);
	} else {
		content_type = procmime_get_content_type_str(partinfo->type, partinfo->subtype);
	}

	if (content_type == NULL)
		type = TYPE_UNKNOWN;
	else if (!strcmp(content_type, "application/pdf"))
		type = TYPE_PDF;
	else if (!strcmp(content_type, "application/postscript"))
		type = TYPE_PS;
	else
		type = TYPE_UNKNOWN;
	
	g_free(content_type);
	return type;
}

static void poppler_show_controls(PopplerViewer *viewer, gboolean show)
{
	if (show) {
		gtk_widget_show(viewer->cur_page);
		gtk_widget_show(viewer->num_pages);
		gtk_widget_show(viewer->prev);
		gtk_widget_show(viewer->next);
		gtk_widget_show(viewer->zoom_in);
		gtk_widget_show(viewer->zoom_out);
		gtk_widget_show(viewer->zoom_fit);
		gtk_widget_show(viewer->zoom_width);
		gtk_widget_show(viewer->vsep);
		gtk_widget_show(viewer->vsep1);	
		gtk_widget_show(viewer->rotate_right);
		gtk_widget_show(viewer->rotate_left);
		gtk_widget_show(viewer->doc_info);
	} else {
		gtk_widget_hide(viewer->cur_page);
		gtk_widget_hide(viewer->num_pages);
		gtk_widget_hide(viewer->prev);
		gtk_widget_hide(viewer->next);
		gtk_widget_hide(viewer->zoom_in);
		gtk_widget_hide(viewer->zoom_out);
		gtk_widget_hide(viewer->zoom_fit);
		gtk_widget_hide(viewer->zoom_width);
		gtk_widget_hide(viewer->vsep);
		gtk_widget_hide(viewer->vsep1);	
		gtk_widget_hide(viewer->rotate_right);
		gtk_widget_hide(viewer->rotate_left);
		gtk_widget_hide(viewer->doc_info);
	}
}

static void poppler_pdf_view_update(MimeViewer *_viewer, gboolean reload_file, int page_num) {

	PopplerViewer *viewer = (PopplerViewer *) _viewer;
	GError *error;
	GdkPixbuf *pb;
	
	gchar *tmpfile = NULL;
	error = NULL;
	if (reload_file) {
		pb = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 1, 1);	
		gdk_pixbuf_fill (pb, 0x00000000);
		gtk_image_set_from_pixbuf(GTK_IMAGE(viewer->pdf_view), pb);
		g_object_unref(G_OBJECT(pb));
		pb = NULL;
		if (viewer->pdf_doc) {
			g_object_unref(G_OBJECT(viewer->pdf_doc));
			viewer->pdf_doc = NULL;
		}
		
		if (poppler_mimepart_get_type(viewer->to_load) == TYPE_PS) {
			stock_pixmap_gdk(viewer->hbox, 
					STOCK_PIXMAP_MIME_PS, 
					&viewer->icon_pixmap, 
					&viewer->icon_bitmap);
			gtk_image_set_from_pixmap(GTK_IMAGE(viewer->icon_type), viewer->icon_pixmap, viewer->icon_bitmap);
		} else if (poppler_mimepart_get_type(viewer->to_load) == TYPE_PDF) {
			stock_pixmap_gdk(viewer->hbox, 
					STOCK_PIXMAP_MIME_PDF, 
					&viewer->icon_pixmap, 
					&viewer->icon_bitmap);
			gtk_image_set_from_pixmap(GTK_IMAGE(viewer->icon_type), viewer->icon_pixmap, viewer->icon_bitmap);
		} else {
			stock_pixmap_gdk(viewer->hbox, 
					STOCK_PIXMAP_MIME_APPLICATION, 
					&viewer->icon_pixmap, 
					&viewer->icon_bitmap);
			gtk_image_set_from_pixmap(GTK_IMAGE(viewer->icon_type), viewer->icon_pixmap, viewer->icon_bitmap);
		}

		gtk_label_set_text(GTK_LABEL(viewer->doc_label), _("Loading..."));	
		poppler_show_controls(viewer, FALSE);
		main_window_cursor_wait(mainwindow_get_mainwindow());

		GTK_EVENTS_FLUSH();

		if (poppler_mimepart_get_type(viewer->to_load) == TYPE_PS) {
			gchar *cmdline = NULL, *tmp = NULL;
			gint result = 0;
			/* convert postscript to pdf */
			tmpfile = get_tmp_file();
			cmdline = g_strdup_printf(
				"gs -dNOPAUSE -dBATCH -sDEVICE=pdfwrite -sOutputFile=%s -c .setpdfwrite -f \"%s\"",
				tmpfile, viewer->filename);
			result = execute_command_line(cmdline, FALSE);
			if (result == 0) {
				tmp = g_strdup_printf("file://%s", tmpfile);
				viewer->pdf_doc = poppler_document_new_from_file( tmp, NULL, &error);
				g_free(tmp);
			} else {
				g_warning("gs conversion failed: %s returned %d\n", cmdline, result);
				tmp = g_strdup_printf("gs: err %d", result);
				gtk_label_set_text(GTK_LABEL(viewer->doc_label), tmp);
				g_free(tmp);
			}
			g_free(cmdline);
			g_unlink(tmpfile);
			g_free(tmpfile);
			if (result != 0) {
				main_window_cursor_normal(mainwindow_get_mainwindow());
				return;
			}
		} else {
			viewer->pdf_doc = poppler_document_new_from_file( viewer->fsname, NULL, &error);
		}

		main_window_cursor_normal(mainwindow_get_mainwindow());
	} 
	if (viewer->pdf_doc == NULL) {
		strretchomp(error->message);

		stock_pixmap_gdk(viewer->hbox, 
				STOCK_PIXMAP_MIME_APPLICATION, 
				&viewer->icon_pixmap, 
				&viewer->icon_bitmap);

		gtk_image_set_from_pixmap(GTK_IMAGE(viewer->icon_type), viewer->icon_pixmap, viewer->icon_bitmap);
		gtk_label_set_text(GTK_LABEL(viewer->doc_label), error->message);
		g_error_free(error);
		return;
	}

	if(page_num > 0 && page_num <= poppler_document_get_n_pages(viewer->pdf_doc)) {
	    gchar *page_str = g_strdup_printf("%d", page_num);
	    gtk_spin_button_set_value(GTK_SPIN_BUTTON(viewer->cur_page), (gdouble) page_num);

	    if (reload_file) {
		gint n;
		n = poppler_document_get_n_pages(viewer->pdf_doc);
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(viewer->cur_page), 1, n);
		gtk_label_set_text(GTK_LABEL(viewer->num_pages), g_strdup_printf("/ %d", n));
	    }

	    viewer->pdf_page = poppler_document_get_page_by_label (viewer->pdf_doc, page_str);
	    g_free(page_str);

	    if (viewer->pdf_page == NULL) {
		g_warning ("Page not found\n");
		return;
	    }   
	
	    //gtk_entry_set_text(GTK_ENTRY(viewer->cur_page), g_strdup_printf("%d",page_num));
	
	    if(viewer->rotate == 90 || viewer->rotate == 270) {
		poppler_page_get_size(viewer->pdf_page, &viewer->height, &viewer->width);
	    }
	    else {
		poppler_page_get_size(viewer->pdf_page, &viewer->width, &viewer->height);
	    }
	
	    pb = gdk_pixbuf_new (GDK_COLORSPACE_RGB, 
				FALSE, 8, 
				(int)(viewer->width * viewer->zoom), 
				(int)(viewer->height * viewer->zoom));	
	
	    poppler_page_render_to_pixbuf (viewer->pdf_page, 
					    0, 
					    0, 
					    (int)(viewer->width * viewer->zoom), 
					    (int)(viewer->height * viewer->zoom), 
					    viewer->zoom, 
					    viewer->rotate, 
					    pb);
	
	    gtk_image_set_from_pixbuf(GTK_IMAGE(viewer->pdf_view), pb);
	    g_object_unref(G_OBJECT(pb));
	    g_object_unref(G_OBJECT(viewer->pdf_page));

	    gtk_label_set_text(GTK_LABEL(viewer->doc_label), 
	    g_strdup_printf(_("%s document"),(poppler_mimepart_get_type(viewer->to_load) == TYPE_PDF) ? "PDF":"Postscript"));
	    
	    poppler_show_controls(viewer, TRUE);
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
	viewer->zoom = 1.0;
	viewer->rotate = 0;
	viewer->to_load = partinfo;
	gtk_timeout_add(5, (GtkFunction)poppler_show_mimepart_real, viewer);
	
}

static void poppler_clear_viewer(MimeViewer *_viewer)
{
	PopplerViewer *viewer = (PopplerViewer *) _viewer;
	GtkAdjustment *vadj;
	GdkPixbuf *pb;

	debug_print("poppler_clear_viewer\n");
	viewer->to_load = NULL;

	if (viewer->pdf_doc) {
		g_object_unref(G_OBJECT(viewer->pdf_doc));
		viewer->pdf_doc = NULL;
	}
	pb = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 1, 1);	
	gdk_pixbuf_fill (pb, 0x00000000);
	gtk_image_set_from_pixbuf(GTK_IMAGE(viewer->pdf_view), pb);
	g_object_unref(G_OBJECT(pb));

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
		if (!up) {
			poppler_pdf_view_update(_viewer, FALSE, gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(viewer->cur_page)) + 1);
			vadj = gtk_scrolled_window_get_vadjustment(
				GTK_SCROLLED_WINDOW(viewer->scrollwin));
			vadj->value = 0.0;
			g_signal_emit_by_name(G_OBJECT(vadj), "value-changed", 0);
			return TRUE;
		} else if (up) {
			poppler_pdf_view_update(_viewer, FALSE, gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(viewer->cur_page)) - 1);
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

static void button_set_pixmap(GtkWidget *widg, char **button_image) 
{
    GdkPixmap *pixmap;
    GdkBitmap *mask;

    pixmap = mask = NULL;
    PIXMAP_CREATE(mainwindow_get_mainwindow()->window, pixmap, mask, button_image);
    gtk_button_set_image(GTK_BUTTON(widg), 
		    gtk_image_new_from_pixmap(pixmap, mask));
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
	viewer->pdf_view = gtk_image_new();
	viewer->icon_type = gtk_image_new();
	viewer->doc_label = gtk_label_new("");
	viewer->vsep = gtk_vseparator_new();
	viewer->vsep1 = gtk_vseparator_new();
	viewer->cur_page = gtk_spin_button_new_with_range(0,0,1);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(viewer->cur_page), TRUE);
	viewer->button_bar_tips = gtk_tooltips_new();

	viewer->prev = gtk_button_new();
	gtk_widget_set_size_request(viewer->prev, -1, -1);
	button_set_pixmap(viewer->prev, left_arrow_xpm);

	viewer->next = gtk_button_new();
	gtk_widget_set_size_request(viewer->next, -1, -1);
	button_set_pixmap(viewer->next, right_arrow_xpm);

	viewer->zoom_in = gtk_button_new();
	gtk_widget_set_size_request(viewer->zoom_in, -1, -1);
	button_set_pixmap(viewer->zoom_in, zoom_in_xpm);

	viewer->zoom_out = gtk_button_new();
	gtk_widget_set_size_request(viewer->zoom_out, -1, -1);
	button_set_pixmap(viewer->zoom_out, zoom_out_xpm);

	viewer->zoom_fit = gtk_button_new();
	gtk_widget_set_size_request(viewer->zoom_fit, -1, -1);
	button_set_pixmap(viewer->zoom_fit, zoom_fit_xpm);

	viewer->zoom_width = gtk_button_new();
	gtk_widget_set_size_request(viewer->zoom_width, -1, -1);
	button_set_pixmap(viewer->zoom_width, zoom_width_xpm);

	viewer->rotate_left = gtk_button_new();
	gtk_widget_set_size_request(viewer->rotate_left, -1, -1);
	button_set_pixmap(viewer->rotate_left, rotate_left_xpm);
	
	viewer->rotate_right = gtk_button_new();
	gtk_widget_set_size_request(viewer->rotate_right, -1, -1);
	button_set_pixmap(viewer->rotate_right, rotate_right_xpm);
	
	viewer->doc_info = gtk_button_new();
	gtk_widget_set_size_request(viewer->doc_info, -1, -1);
	button_set_pixmap(viewer->doc_info, doc_info_xpm);

	viewer->num_pages = gtk_label_new("");
	

	gtk_scrolled_window_set_policy(
			GTK_SCROLLED_WINDOW(viewer->scrollwin), 
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_scrolled_window_set_shadow_type(
			GTK_SCROLLED_WINDOW(viewer->scrollwin),
			GTK_SHADOW_IN);

	gtk_scrolled_window_add_with_viewport(
		GTK_SCROLLED_WINDOW(viewer->scrollwin),
		viewer->pdf_view);

	viewer->vbox = gtk_vbox_new(FALSE, 4);
	gtk_widget_show(viewer->vbox);

	
	viewer->hbox = gtk_hbox_new(FALSE, 4);
	gtk_widget_show(viewer->hbox);

	
	stock_pixmap_gdk(viewer->hbox, 
			STOCK_PIXMAP_MIME_TEXT_PLAIN, 
			&viewer->icon_pixmap, 
			&viewer->icon_bitmap);
	gtk_image_set_from_pixmap(GTK_IMAGE(viewer->icon_type), viewer->icon_pixmap, viewer->icon_bitmap);
	
	gtk_box_pack_start(GTK_BOX(viewer->hbox), viewer->icon_type, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(viewer->hbox), viewer->doc_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(viewer->hbox), viewer->vsep, FALSE, TRUE, 1);
	
	gtk_box_pack_start(GTK_BOX(viewer->hbox), viewer->cur_page, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(viewer->hbox), viewer->num_pages, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(viewer->hbox), viewer->vsep1, FALSE, FALSE, 1);

	gtk_box_pack_start(GTK_BOX(viewer->hbox), viewer->prev, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(viewer->hbox), viewer->next, FALSE, FALSE, 1);
	
	gtk_box_pack_start(GTK_BOX(viewer->hbox), viewer->zoom_in, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(viewer->hbox), viewer->zoom_out, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(viewer->hbox), viewer->zoom_fit, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(viewer->hbox), viewer->zoom_width, FALSE, FALSE, 1);

	gtk_box_pack_start(GTK_BOX(viewer->hbox), viewer->rotate_left, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(viewer->hbox), viewer->rotate_right, FALSE, FALSE, 1);
	
	gtk_box_pack_start(GTK_BOX(viewer->hbox), viewer->doc_info, FALSE, FALSE, 1);
	
	gtk_box_pack_start(GTK_BOX(viewer->vbox), viewer->hbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(viewer->vbox), viewer->scrollwin, TRUE, TRUE, 0);

	gtk_widget_show(GTK_WIDGET(viewer->scrollwin));
	gtk_widget_ref(GTK_WIDGET(viewer->scrollwin));
	gtk_widget_show(GTK_WIDGET(viewer->hbox));
	gtk_widget_ref(GTK_WIDGET(viewer->hbox));	
	gtk_widget_show(GTK_WIDGET(viewer->vbox));
	gtk_widget_ref(GTK_WIDGET(viewer->vbox));
	gtk_widget_show(GTK_WIDGET(viewer->num_pages));
	gtk_widget_ref(GTK_WIDGET(viewer->num_pages));
	gtk_widget_show(GTK_WIDGET(viewer->cur_page));
	gtk_widget_ref(GTK_WIDGET(viewer->cur_page));
	gtk_widget_show(GTK_WIDGET(viewer->vsep));
	gtk_widget_ref(GTK_WIDGET(viewer->vsep));
	gtk_widget_show(GTK_WIDGET(viewer->vsep1));
	gtk_widget_ref(GTK_WIDGET(viewer->vsep1));
	
	gtk_widget_show(GTK_WIDGET(viewer->prev));
	gtk_widget_ref(GTK_WIDGET(viewer->prev));

	gtk_widget_show(GTK_WIDGET(viewer->next));
	gtk_widget_ref(GTK_WIDGET(viewer->next));

	gtk_widget_show(GTK_WIDGET(viewer->zoom_in));
	gtk_widget_ref(GTK_WIDGET(viewer->zoom_in));
	gtk_widget_show(GTK_WIDGET(viewer->zoom_out));
	gtk_widget_ref(GTK_WIDGET(viewer->zoom_out));
	gtk_widget_show(GTK_WIDGET(viewer->zoom_fit));
	gtk_widget_ref(GTK_WIDGET(viewer->zoom_fit));
	gtk_widget_show(GTK_WIDGET(viewer->zoom_width));
	gtk_widget_ref(GTK_WIDGET(viewer->zoom_width));

	gtk_widget_show(GTK_WIDGET(viewer->rotate_right));
	gtk_widget_ref(GTK_WIDGET(viewer->rotate_right));
	gtk_widget_show(GTK_WIDGET(viewer->rotate_left));
	gtk_widget_ref(GTK_WIDGET(viewer->rotate_left));
	gtk_widget_show(GTK_WIDGET(viewer->doc_info));
	gtk_widget_ref(GTK_WIDGET(viewer->doc_info));
	gtk_widget_show(GTK_WIDGET(viewer->doc_label));
	gtk_widget_ref(GTK_WIDGET(viewer->doc_label));
	gtk_widget_show(GTK_WIDGET(viewer->icon_type));
	gtk_widget_ref(GTK_WIDGET(viewer->icon_type));	
	gtk_widget_show(GTK_WIDGET(viewer->pdf_view));
	gtk_widget_ref(GTK_WIDGET(viewer->pdf_view));
	
	/* setting tooltips */
	gtk_tooltips_set_tip (GTK_TOOLTIPS (viewer->button_bar_tips), 
			    viewer->prev,
			    _("Previous Page"),
			    "");

	gtk_tooltips_set_tip (GTK_TOOLTIPS (viewer->button_bar_tips), 
			    viewer->next,
			    _("Next Page"),
			    "");

	gtk_tooltips_set_tip (GTK_TOOLTIPS (viewer->button_bar_tips), 
			    viewer->zoom_in,
			    _("Zoom In"),
			    "");

	gtk_tooltips_set_tip (GTK_TOOLTIPS (viewer->button_bar_tips), 
			    viewer->zoom_out,
			    _("Zoom Out"),
			    "");
	gtk_tooltips_set_tip (GTK_TOOLTIPS (viewer->button_bar_tips), 
			    viewer->zoom_fit,
			    _("Fit Page"),
			    "");
	gtk_tooltips_set_tip (GTK_TOOLTIPS (viewer->button_bar_tips), 
			    viewer->zoom_width,
			    _("Fit Page Width"),
			    "");
	gtk_tooltips_set_tip (GTK_TOOLTIPS (viewer->button_bar_tips), 
			    viewer->rotate_left,
			    _("Rotate Left"),
			    "");
	gtk_tooltips_set_tip (GTK_TOOLTIPS (viewer->button_bar_tips), 
			    viewer->rotate_right,
			    _("Rotate Right"),
			    "");
	gtk_tooltips_set_tip (GTK_TOOLTIPS (viewer->button_bar_tips), 
			    viewer->doc_info,
			    _("Document Info"),
			    "");

	/* Signals */
	
	/*
	g_signal_connect (G_OBJECT(viewer->scrollwin), 
			    "key_press_event", 
			    G_CALLBACK(scrollwin_key_down_cb), 
			    (gpointer) viewer);
	g_signal_connect (G_OBJECT(viewer->scrollwin), 
			    "key_press_event", 
			    G_CALLBACK(scrollwin_key_up_cb), 
			    (gpointer) viewer);
	g_signal_connect (G_OBJECT(viewer->scrollwin), 
			    "key_press_event", 
			    G_CALLBACK(scrollwin_page_down_cb), 
			    (gpointer) viewer);
	g_signal_connect (G_OBJECT(viewer->pdf_view), 
			    "key_press_event", 
			    G_CALLBACK(scrollwin_page_up_cb), 
			    (gpointer) viewer);
	*/

	g_signal_connect (G_OBJECT(viewer->cur_page), 
			"value-changed", 
			G_CALLBACK(poppler_spin_change_page_cb), 
			(gpointer) viewer);
	g_signal_connect (G_OBJECT(viewer->prev), 
			"clicked", 
			G_CALLBACK(poppler_button_prev_page_cb), 
			(gpointer) viewer);
	g_signal_connect (G_OBJECT(viewer->next), 
			"clicked", 
			G_CALLBACK(poppler_button_next_page_cb), 
			(gpointer) viewer);
	g_signal_connect (G_OBJECT(viewer->zoom_in), 
			"clicked", 
			G_CALLBACK(poppler_button_zoom_in_cb), 
			(gpointer) viewer);
	g_signal_connect (G_OBJECT(viewer->zoom_out), 
			"clicked", 
			G_CALLBACK(poppler_button_zoom_out_cb), 
			(gpointer) viewer);

	g_signal_connect (G_OBJECT(viewer->zoom_fit), 
			"clicked", 
			G_CALLBACK(poppler_button_zoom_fit_cb), 
			(gpointer) viewer);
	g_signal_connect (G_OBJECT(viewer->zoom_width), 
			"clicked", 
			G_CALLBACK(poppler_button_zoom_width_cb), 
			(gpointer) viewer);

	g_signal_connect (G_OBJECT(viewer->rotate_right), 
			"clicked", 
			G_CALLBACK(poppler_button_rotate_right_cb), 
			(gpointer) viewer);
	g_signal_connect (G_OBJECT(viewer->rotate_left), 
			"clicked", 
			G_CALLBACK(poppler_button_rotate_left_cb), 
			(gpointer) viewer);
	g_signal_connect (G_OBJECT(viewer->doc_info), 
			"clicked", 
			G_CALLBACK(poppler_button_document_info_cb), 
			(gpointer) viewer);	

	viewer->filename = NULL;
	return (MimeViewer *) viewer;
}

static gchar *content_types[] =
	{"application/pdf", 
	 "application/postscript", 
	 NULL};

static MimeViewerFactory poppler_viewer_factory =
{
	content_types,
	0,
	poppler_viewer_create,
};

gint plugin_init(gchar **error)
{
    msg = g_strdup_printf(_("This plugin enables the viewing of PDF and PostScript "
				"attachments using the Poppler %s Lib.\n\n"
				"PDF Viewer is Copyright (C) 2007\n"
				"by Salvatore De Paolis <iwkse@claws-mail.org>"
				),poppler_get_version());

	bindtextdomain(TEXTDOMAIN, LOCALEDIR);
	bind_textdomain_codeset(TEXTDOMAIN, "UTF-8");

	if (!check_plugin_version(MAKE_NUMERIC_VERSION(2, 7, 2, 57),
		    VERSION_NUMERIC, _("PDF Viewer"), error))
		return -1;
	mimeview_register_viewer_factory(&poppler_viewer_factory);
	return 0;
}

void plugin_done(void)
{
    g_free(msg);	
    mimeview_unregister_viewer_factory(&poppler_viewer_factory);
}

const gchar *plugin_name(void)
{
	return _("PDF Viewer");
}

const gchar *plugin_desc(void)
{
    return msg;
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
		  {PLUGIN_MIMEVIEWER, "application/postscript"},
		  {PLUGIN_NOTHING, NULL} };
	return features;
}

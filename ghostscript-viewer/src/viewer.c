/*
 * Sylpheed-Claws Ghostscript Viewer Plugin
 * Copyright (C) 2003-2004 Christoph Hohmann
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "pluginconfig.h"
#endif
#include "config.h"

#include <glib.h>
#include <gtk/gtk.h>

#include "common/intl.h"
#include "common/utils.h"
#include "procmime.h"
#include "mimeview.h"
#include "gtkgs.h"

#include "viewerprefs.h"

typedef struct _GhostscriptViewer GhostscriptViewer;

MimeViewerFactory ghostscript_viewer_factory;
static GdkCursor *pan_cursor;

struct _GhostscriptViewer
{
	MimeViewer mimeviewer;

	gchar *filename;

	gboolean pan;
	gdouble prev_x, prev_y;
	gint page;
	gint pages;
	gfloat zoom;
	ZoomMode zoommode;

	GtkWidget *viewer;
	GtkWidget *popupmenu;
	GtkWidget *gs;
	GtkWidget *nextpage_btn;
	GtkWidget *prevpage_btn;
	GtkWidget *nextpage_menu;
	GtkWidget *prevpage_menu;
	GtkWidget *fit_page_menu;
	GtkWidget *fit_width_menu;
};

static void update_sensitivity(GhostscriptViewer *ghostscriptviewer)
{
	gboolean next, prev;

	if (ghostscriptviewer->pages < 2) {
		next = FALSE;
		prev = FALSE;
	} else {
		if (ghostscriptviewer->page < (ghostscriptviewer->pages - 1))
			next = TRUE;
		else
			next = FALSE;
		if (ghostscriptviewer->page > 0)
			prev = TRUE;
		else
			prev = FALSE;
	}

	gtk_widget_set_sensitive(ghostscriptviewer->nextpage_btn, next);
	gtk_widget_set_sensitive(ghostscriptviewer->prevpage_btn, prev);
	gtk_widget_set_sensitive(ghostscriptviewer->nextpage_menu, next);
	gtk_widget_set_sensitive(ghostscriptviewer->prevpage_menu, prev);
	gtk_widget_set_sensitive(ghostscriptviewer->fit_page_menu,
				 ghostscriptviewer->zoommode != ZOOM_PAGE);
	gtk_widget_set_sensitive(ghostscriptviewer->fit_width_menu,
				 ghostscriptviewer->zoommode != ZOOM_WIDTH);
}

static GtkWidget *ghostscript_viewer_get_widget(MimeViewer *_mimeviewer)
{
	GhostscriptViewer *ghostscriptviewer = (GhostscriptViewer *) _mimeviewer;

	return ghostscriptviewer->viewer;
}

static void ghostscript_viewer_show_mimepart(MimeViewer *_mimeviewer, const gchar *file, MimeInfo *mimeinfo)
{
	GhostscriptViewer *ghostscriptviewer = (GhostscriptViewer *) _mimeviewer;

	if (ghostscriptviewer->filename != NULL) {
		unlink(ghostscriptviewer->filename);
		g_free(ghostscriptviewer->filename);
	}

	ghostscriptviewer->filename = procmime_get_tmp_file_name(mimeinfo);

	if (!(procmime_get_part(ghostscriptviewer->filename, mimeinfo) < 0)) {
		GtkGS *gs = GTK_GS(ghostscriptviewer->gs);

		gtk_gs_set_antialiasing(gs, ghostscriptviewerprefs.antialiasing);
		gtk_gs_set_respect_eof(gs, ghostscriptviewerprefs.respect_eof);
		gtk_gs_load(gs, ghostscriptviewer->filename);
		gtk_gs_set_page_size(gs, -1, 0);
		ghostscriptviewer->zoommode = ghostscriptviewerprefs.autozoom;
		switch (ghostscriptviewer->zoommode) {
		case ZOOM_PAGE:
			ghostscriptviewer->zoom = gtk_gs_zoom_to_fit(gs, FALSE);
			break;
		case ZOOM_WIDTH:
			ghostscriptviewer->zoom = gtk_gs_zoom_to_fit(gs, TRUE);
			break;
		default:
			break;
		}
		gtk_gs_set_zoom(gs, ghostscriptviewer->zoom);

		ghostscriptviewer->page = gtk_gs_get_current_page(gs);
		ghostscriptviewer->pages = gtk_gs_get_page_count(gs);

		update_sensitivity(ghostscriptviewer);
	}
}

static void ghostscript_viewer_clear_viewer(MimeViewer *_mimeviewer)
{
	GhostscriptViewer *ghostscriptviewer = (GhostscriptViewer *) _mimeviewer;

	gtk_gs_cleanup(GTK_GS(ghostscriptviewer->gs));
	unlink(ghostscriptviewer->filename);
	g_free(ghostscriptviewer->filename);
	ghostscriptviewer->filename = NULL;
	ghostscriptviewer->zoom = 1.0;
	ghostscriptviewer->zoommode = ZOOM_FREE;
}

static void ghostscript_viewer_destroy_viewer(MimeViewer *_mimeviewer)
{
	GhostscriptViewer *ghostscriptviewer = (GhostscriptViewer *) _mimeviewer;

	ghostscript_viewer_clear_viewer(_mimeviewer);
	gtk_widget_unref(ghostscriptviewer->viewer);
	g_free(ghostscriptviewer);
}

gboolean button_press_callback(GtkWidget * widget, GdkEventButton * event,
	   		       gpointer data)
{
	GhostscriptViewer *ghostscriptviewer = (GhostscriptViewer *) data;
	GtkGS *gs = GTK_GS(ghostscriptviewer->gs);
	gint button = event->button;
	gint wx = 0, wy = 0;

	switch (button) {
	case 1:
		if (!ghostscriptviewer->pan) {
			gdk_window_get_pointer(widget->window, &wx, &wy, NULL);

			ghostscriptviewer->pan = TRUE;
			if (pan_cursor == NULL)
				pan_cursor = gdk_cursor_new(GDK_FLEUR);

			gtk_grab_add(widget);
			gdk_pointer_grab(widget->window, FALSE,
					 GDK_POINTER_MOTION_MASK |
					 GDK_BUTTON_RELEASE_MASK, NULL,
					 pan_cursor, GDK_CURRENT_TIME);
			ghostscriptviewer->prev_x = wx;
			ghostscriptviewer->prev_y = wy;
			gtk_gs_start_scroll(gs);
		}
		return TRUE;
	default:
		break;
	}

	return FALSE;
}

gboolean button_release_callback(GtkWidget * widget, GdkEventButton * event,
			         gpointer data)
{
	GhostscriptViewer *ghostscriptviewer = (GhostscriptViewer *) data;
	GtkGS *gs = GTK_GS(ghostscriptviewer->gs);
	gint button = event->button;

	switch (button) {
	case 1:
		if (ghostscriptviewer->pan) {
		        ghostscriptviewer->pan = FALSE;
			gdk_pointer_ungrab(GDK_CURRENT_TIME);
			gtk_grab_remove(widget);
			gtk_gs_end_scroll(gs);
		}
		return TRUE;
	case 3:
		gtk_menu_popup(GTK_MENU(ghostscriptviewer->popupmenu),
			       NULL, NULL, NULL, NULL,
			       button, event->time);
		return TRUE;
	default:
		break;
	}

	return FALSE;
}

void motion_callback(GtkWidget * widget, GdkEventMotion * event,
		     gpointer data)
{
	GhostscriptViewer *ghostscriptviewer = (GhostscriptViewer *) data;
	GtkGS *gs = GTK_GS(ghostscriptviewer->gs);

	if (ghostscriptviewer->pan) {
		gtk_gs_scroll(gs, -event->x + ghostscriptviewer->prev_x, 
				  -event->y + ghostscriptviewer->prev_y);;
		ghostscriptviewer->prev_x = event->x;
		ghostscriptviewer->prev_y = event->y;
	}
}

void interpreter_message(GtkGS *gs, gchar *msg, gpointer user_data)
{
	debug_print("%s\n", msg);
}

void goto_page(GhostscriptViewer *ghostscriptviewer, gint page)
{
	gtk_gs_goto_page(GTK_GS(ghostscriptviewer->gs), page);
	ghostscriptviewer->page = gtk_gs_get_current_page(GTK_GS(ghostscriptviewer->gs));

	update_sensitivity(ghostscriptviewer);
}

void nextpage(GhostscriptViewer *ghostscriptviewer)
{
	goto_page(ghostscriptviewer, ghostscriptviewer->page + 1);
}

void prevpage(GhostscriptViewer *ghostscriptviewer)
{
	goto_page(ghostscriptviewer, ghostscriptviewer->page - 1);
}

void zoomin(GhostscriptViewer *ghostscriptviewer)
{
	ghostscriptviewer->zoom = ghostscriptviewer->zoom * 1.2;
	ghostscriptviewer->zoommode = ZOOM_FREE;
	update_sensitivity(ghostscriptviewer);

	gtk_gs_set_zoom(GTK_GS(ghostscriptviewer->gs), ghostscriptviewer->zoom);
}

void zoomout(GhostscriptViewer *ghostscriptviewer)
{
	ghostscriptviewer->zoom = ghostscriptviewer->zoom / 1.2;
	ghostscriptviewer->zoommode = ZOOM_FREE;
	update_sensitivity(ghostscriptviewer);

	gtk_gs_set_zoom(GTK_GS(ghostscriptviewer->gs), ghostscriptviewer->zoom);
}

void nextpage_cb(GtkWidget *widget, GhostscriptViewer *ghostscriptviewer)
{
	nextpage(ghostscriptviewer);
}

void prevpage_cb(GtkWidget *widget, GhostscriptViewer *ghostscriptviewer)
{
	prevpage(ghostscriptviewer);
}

void zoomin_cb(GtkWidget *widget, GhostscriptViewer *ghostscriptviewer)
{
	zoomin(ghostscriptviewer);
}

void zoomout_cb(GtkWidget *widget, GhostscriptViewer *ghostscriptviewer)
{
	zoomout(ghostscriptviewer);
}

void on_next_page_activate(GtkMenuItem *menuitem, GhostscriptViewer *ghostscriptviewer)
{
	nextpage(ghostscriptviewer);
}

void on_prev_page_activate(GtkMenuItem *menuitem, GhostscriptViewer *ghostscriptviewer)
{
	prevpage(ghostscriptviewer);
}

void on_first_page_activate(GtkMenuItem *menuitem, GhostscriptViewer *ghostscriptviewer)
{
	goto_page(ghostscriptviewer, 0);
}

void on_last_page_activate(GtkMenuItem *menuitem, GhostscriptViewer *ghostscriptviewer)
{
	goto_page(ghostscriptviewer, ghostscriptviewer->pages);
}

void on_zoom_in_activate(GtkMenuItem *menuitem, GhostscriptViewer *ghostscriptviewer)
{
	zoomin(ghostscriptviewer);
}

void on_zoom_out_activate(GtkMenuItem *menuitem, GhostscriptViewer *ghostscriptviewer)
{
	zoomout(ghostscriptviewer);
}

void on_fit_page_activate(GtkMenuItem *menuitem, GhostscriptViewer *ghostscriptviewer)
{
	ghostscriptviewer->zoom = gtk_gs_zoom_to_fit(GTK_GS(ghostscriptviewer->gs), FALSE);
	ghostscriptviewer->zoommode = ZOOM_PAGE;
	update_sensitivity(ghostscriptviewer);

	gtk_gs_set_zoom(GTK_GS(ghostscriptviewer->gs), ghostscriptviewer->zoom);
}

void on_fit_width_activate(GtkMenuItem *menuitem, GhostscriptViewer *ghostscriptviewer)
{
	ghostscriptviewer->zoom = gtk_gs_zoom_to_fit(GTK_GS(ghostscriptviewer->gs), TRUE);
	ghostscriptviewer->zoommode = ZOOM_WIDTH;
	update_sensitivity(ghostscriptviewer);

	gtk_gs_set_zoom(GTK_GS(ghostscriptviewer->gs), ghostscriptviewer->zoom);
}

MimeViewer *ghostscript_viewer_create(void)
{
	GhostscriptViewer *ghostscriptviewer;
	GtkObject *hadj, *vadj;
	GtkWidget *gs;
	/*
	 *  glade generated code (do not touch)
	 */
	GtkWidget *viewer;
	GtkWidget *hbuttonbox1;
	GtkWidget *prevpage_btn;
	GtkWidget *nextpage_btn;
	GtkWidget *zoomin_btn;
	GtkWidget *zoomout_btn;
	GtkWidget *gsframe;

	GtkWidget *popupmenu;
	GtkAccelGroup *popupmenu_accels;
	GtkWidget *next_page;
	GtkWidget *prev_page;
	GtkWidget *first_page;
	GtkWidget *last_page;
	GtkWidget *item1;
	GtkWidget *zoom_in;
	GtkWidget *zoom_out;
	GtkWidget *fit_page;
	GtkWidget *fit_width;

	viewer = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(viewer);

	hbuttonbox1 = gtk_hbutton_box_new();
	gtk_widget_show(hbuttonbox1);
	gtk_box_pack_start(GTK_BOX(viewer), hbuttonbox1, FALSE, FALSE, 0);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbuttonbox1),
				  GTK_BUTTONBOX_START);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbuttonbox1), 0);
	gtk_button_box_set_child_size(GTK_BUTTON_BOX(hbuttonbox1), 0, 0);
	gtk_button_box_set_child_ipadding(GTK_BUTTON_BOX(hbuttonbox1), 4,
					  0);

	prevpage_btn = gtk_button_new_with_label(_("Prev Page"));
	gtk_widget_show(prevpage_btn);
	gtk_container_add(GTK_CONTAINER(hbuttonbox1), prevpage_btn);
	GTK_WIDGET_SET_FLAGS(prevpage_btn, GTK_CAN_DEFAULT);

	nextpage_btn = gtk_button_new_with_label(_("Next Page"));
	gtk_widget_show(nextpage_btn);
	gtk_container_add(GTK_CONTAINER(hbuttonbox1), nextpage_btn);
	GTK_WIDGET_SET_FLAGS(nextpage_btn, GTK_CAN_DEFAULT);

	zoomin_btn = gtk_button_new_with_label(_("Zoom in"));
	gtk_widget_show(zoomin_btn);
	gtk_container_add(GTK_CONTAINER(hbuttonbox1), zoomin_btn);
	GTK_WIDGET_SET_FLAGS(zoomin_btn, GTK_CAN_DEFAULT);

	zoomout_btn = gtk_button_new_with_label(_("Zoom out"));
	gtk_widget_show(zoomout_btn);
	gtk_container_add(GTK_CONTAINER(hbuttonbox1), zoomout_btn);
	GTK_WIDGET_SET_FLAGS(zoomout_btn, GTK_CAN_DEFAULT);

	gsframe = gtk_frame_new(NULL);
	gtk_widget_show(gsframe);
	gtk_box_pack_start(GTK_BOX(viewer), gsframe, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(gsframe), GTK_SHADOW_IN);

	popupmenu = gtk_menu_new();
	gtk_object_set_data(GTK_OBJECT(popupmenu), "popupmenu", popupmenu);
	popupmenu_accels =
	    gtk_menu_ensure_uline_accel_group(GTK_MENU(popupmenu));

	next_page = gtk_menu_item_new_with_label(_("Next Page"));
	gtk_widget_show(next_page);
	gtk_container_add(GTK_CONTAINER(popupmenu), next_page);

	prev_page = gtk_menu_item_new_with_label(_("Prev Page"));
	gtk_widget_show(prev_page);
	gtk_container_add(GTK_CONTAINER(popupmenu), prev_page);

	first_page = gtk_menu_item_new_with_label(_("First Page"));
	gtk_widget_show(first_page);
	gtk_container_add(GTK_CONTAINER(popupmenu), first_page);

	last_page = gtk_menu_item_new_with_label(_("Last Page"));
	gtk_widget_show(last_page);
	gtk_container_add(GTK_CONTAINER(popupmenu), last_page);

	item1 = gtk_menu_item_new();
	gtk_widget_show(item1);
	gtk_container_add(GTK_CONTAINER(popupmenu), item1);
	gtk_widget_set_sensitive(item1, FALSE);

	zoom_in = gtk_menu_item_new_with_label(_("Zoom in"));
	gtk_widget_show(zoom_in);
	gtk_container_add(GTK_CONTAINER(popupmenu), zoom_in);

	zoom_out = gtk_menu_item_new_with_label(_("Zoom out"));
	gtk_widget_show(zoom_out);
	gtk_container_add(GTK_CONTAINER(popupmenu), zoom_out);

	fit_page = gtk_menu_item_new_with_label(_("Fit page"));
	gtk_widget_show(fit_page);
	gtk_container_add(GTK_CONTAINER(popupmenu), fit_page);

	fit_width = gtk_menu_item_new_with_label(_("Fit width"));
	gtk_widget_show(fit_width);
	gtk_container_add(GTK_CONTAINER(popupmenu), fit_width);
	/*
	 *  end of glade code
	 */

	gtk_widget_ref(viewer);

	ghostscriptviewer = g_new0(GhostscriptViewer, 1);
	ghostscriptviewer->mimeviewer.factory = &ghostscript_viewer_factory;

	ghostscriptviewer->mimeviewer.get_widget = ghostscript_viewer_get_widget;
	ghostscriptviewer->mimeviewer.show_mimepart = ghostscript_viewer_show_mimepart;
	ghostscriptviewer->mimeviewer.clear_viewer = ghostscript_viewer_clear_viewer;
	ghostscriptviewer->mimeviewer.destroy_viewer = ghostscript_viewer_destroy_viewer;

	hadj = gtk_adjustment_new(0.0, 0.0, 1.0, 1.0, 1.0, 0.5);
        vadj = gtk_adjustment_new(0.0, 0.0, 1.0, 1.0, 1.0, 0.5);
	gs = gtk_gs_new(GTK_ADJUSTMENT(hadj), GTK_ADJUSTMENT(vadj));
	gtk_gs_set_show_scroll_rect(GTK_GS(gs), FALSE);
	gtk_gs_set_watch_file(GTK_GS(gs), FALSE);
	gtk_widget_set_events(gs,
                    	      GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);
	gtk_widget_show(gs);
	gtk_container_add(GTK_CONTAINER(gsframe), gs);

//	gtk_range_set_adjustment(GTK_RANGE(hscrollbar), GTK_ADJUSTMENT(hadj));
//	gtk_range_set_adjustment(GTK_RANGE(vscrollbar), GTK_ADJUSTMENT(vadj));

	ghostscriptviewer->viewer = viewer;
	ghostscriptviewer->popupmenu = popupmenu;
	ghostscriptviewer->gs = gs;
	ghostscriptviewer->nextpage_btn = nextpage_btn;
	ghostscriptviewer->prevpage_btn = prevpage_btn;
	ghostscriptviewer->nextpage_menu = next_page;
	ghostscriptviewer->prevpage_menu = prev_page;
	ghostscriptviewer->fit_page_menu = fit_page;
	ghostscriptviewer->fit_width_menu = fit_width;
	ghostscriptviewer->zoom = 1.0;
	ghostscriptviewer->zoommode = ZOOM_FREE;

	/* viewer callbacks */
	gtk_signal_connect(GTK_OBJECT(gsframe), "button_press_event",
                	   GTK_SIGNAL_FUNC(button_press_callback), ghostscriptviewer);
	gtk_signal_connect(GTK_OBJECT(gsframe), "button_release_event",
                	   GTK_SIGNAL_FUNC(button_release_callback), ghostscriptviewer);
	gtk_signal_connect(GTK_OBJECT(gsframe), "motion_notify_event",
                	   GTK_SIGNAL_FUNC(motion_callback), ghostscriptviewer);
	gtk_signal_connect(GTK_OBJECT(gs), "interpreter_message",
                	   GTK_SIGNAL_FUNC(interpreter_message), ghostscriptviewer);
        gtk_signal_connect(GTK_OBJECT(nextpage_btn), "released",
                           GTK_SIGNAL_FUNC(nextpage_cb), ghostscriptviewer);
        gtk_signal_connect(GTK_OBJECT(prevpage_btn), "released",
                           GTK_SIGNAL_FUNC(prevpage_cb), ghostscriptviewer);
        gtk_signal_connect(GTK_OBJECT(zoomin_btn), "released",
                           GTK_SIGNAL_FUNC(zoomin_cb), ghostscriptviewer);
        gtk_signal_connect(GTK_OBJECT(zoomout_btn), "released",
                           GTK_SIGNAL_FUNC(zoomout_cb), ghostscriptviewer);

	/* menu callbacks */
	gtk_signal_connect(GTK_OBJECT(next_page), "activate",
			   GTK_SIGNAL_FUNC(on_next_page_activate), ghostscriptviewer);
	gtk_signal_connect(GTK_OBJECT(prev_page), "activate",
			   GTK_SIGNAL_FUNC(on_prev_page_activate), ghostscriptviewer);
	gtk_signal_connect(GTK_OBJECT(first_page), "activate",
			   GTK_SIGNAL_FUNC(on_first_page_activate), ghostscriptviewer);
	gtk_signal_connect(GTK_OBJECT(last_page), "activate",
			   GTK_SIGNAL_FUNC(on_last_page_activate), ghostscriptviewer);

	gtk_signal_connect(GTK_OBJECT(zoom_in), "activate",
			   GTK_SIGNAL_FUNC(on_zoom_in_activate), ghostscriptviewer);
	gtk_signal_connect(GTK_OBJECT(zoom_out), "activate",
			   GTK_SIGNAL_FUNC(on_zoom_out_activate), ghostscriptviewer);
	gtk_signal_connect(GTK_OBJECT(fit_page), "activate",
			   GTK_SIGNAL_FUNC(on_fit_page_activate), ghostscriptviewer);
	gtk_signal_connect(GTK_OBJECT(fit_width), "activate",
			   GTK_SIGNAL_FUNC(on_fit_width_activate), ghostscriptviewer);

	return (MimeViewer *) ghostscriptviewer;
}

static gchar *content_types[] =
	{"application/postscript", "application/pdf", NULL};

MimeViewerFactory ghostscript_viewer_factory =
{
	content_types,
	0,
	
	ghostscript_viewer_create,
};

void ghostscript_viewer_init(void)
{
	mimeview_register_viewer_factory(&ghostscript_viewer_factory);
}

void ghostscript_viewer_done(void)
{
	mimeview_unregister_viewer_factory(&ghostscript_viewer_factory);
}

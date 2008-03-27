/*
 * Claws Mail -- a GTK+ based, lightweight, and fast e-mail client
 *
 * Copyright (c) 2007-2008 Juha Kautto (juha at xfce.org)
 * Copyright (c) 2008 Colin Leroy (colin@colino.net)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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
#  include <config.h>
#endif
#include "gettext.h"
#include "defs.h"

#include <string.h>
#include <time.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "summaryview.h"
#include "vcalendar.h"
#include "vcal_folder.h"
#include "vcal_prefs.h"
#include "vcal_manager.h"
#include "vcal_meeting_gtk.h"
#include "menu.h"


static void view_new_meeting_cb			(GtkWidget	*widget,
						 guint		 action,
						 void		*user_data);
static void view_edit_meeting_cb		(GtkWidget	*widget,
						 guint		 action,
						 void		*user_data);
static void view_cancel_meeting_cb		(GtkWidget	*widget,
						 guint		 action,
						 void		*user_data);
static void view_go_today_cb			(GtkWidget	*widget,
						 guint		 action,
						 void		*user_data);

static GtkItemFactoryEntry view_popup_entries[] = 
{
	{N_("/_Create new meeting..."),	NULL, view_new_meeting_cb, 0, NULL},
	{"/---",				NULL, NULL, 0, "<Separator>"},
	{N_("/_Go to today"),		NULL, view_go_today_cb, 0, NULL},
};

static GtkItemFactoryEntry view_event_popup_entries[] = 
{
	{N_("/_Edit this meeting..."),	NULL, view_edit_meeting_cb, 0, NULL},
	{N_("/_Cancel this meeting..."),NULL, view_cancel_meeting_cb, 0, NULL},
	{"/---",				NULL, NULL, 0, "<Separator>"},
	{N_("/C_reate new meeting..."),	NULL, view_new_meeting_cb, 0, NULL},
	{"/---",				NULL, NULL, 0, "<Separator>"},
	{N_("/_Go to today"),		NULL, view_go_today_cb, 0, NULL},
};

GtkWidget *build_line(gint start_x, gint start_y
        , gint width, gint height, GtkWidget *hour_line, GdkColor *line_color)
{
    GdkColormap *pic1_cmap;
    GdkVisual *pic1_vis;
    GdkPixmap *pic1;
    GdkGC *pic1_gc;
    GtkWidget *new_hour_line;
    gint depth = 16;
    gboolean first = FALSE;

    /*
     * GdkPixbuf *scaled;
    scaled = gdk_pixbuf_scale_simple (pix, w, h, GDK_INTERP_BILINEAR);
    */
     
    pic1_cmap = gdk_colormap_get_system();
    pic1_vis = gdk_colormap_get_visual(pic1_cmap);
    depth = pic1_vis->depth;
    if (hour_line == NULL) {
        pic1 = gdk_pixmap_new(NULL, width, height, depth);
        gdk_drawable_set_colormap(pic1, pic1_cmap);
        first = TRUE;
    }
    else
        gtk_image_get_pixmap(GTK_IMAGE(hour_line), &pic1, NULL);
    pic1_gc = gdk_gc_new(pic1);
    if (first) {
        gdk_gc_set_foreground(pic1_gc, line_color);
        gdk_draw_rectangle(pic1, pic1_gc, TRUE, start_x, start_y, width, height);
    }
    else {
        gdk_draw_rectangle(pic1, pic1_gc, TRUE, start_x, start_y, width, height);
    }
    
    new_hour_line = gtk_image_new_from_pixmap(pic1, NULL);
    g_object_unref(pic1_gc);
    g_object_unref(pic1);
    return(new_hour_line);
}

/* move one day forward or backward */
void orage_move_day(struct tm *t, int day)
{
    guint monthdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    t->tm_year += 1900;
    if (((t->tm_year%4) == 0) 
    && (((t->tm_year%100) != 0) || ((t->tm_year%400) == 0)))
        ++monthdays[1]; /* leap year, february has 29 days */

    t->tm_mday += day; /* may be negative */
    if (t->tm_mday == 0) { /*  we went to previous month */
        if (--t->tm_mon == -1) { /* previous year */
            --t->tm_year;
            t->tm_mon = 11;
        }
        t->tm_mday = monthdays[t->tm_mon];
    }
    else if (t->tm_mday > (monthdays[t->tm_mon])) { /* next month */
        if (++t->tm_mon == 12) {
            ++t->tm_year;
            t->tm_mon = 0;
        }
        t->tm_mday = 1;
    }
    t->tm_year -= 1900;
    t->tm_wday += day; 
    if (t->tm_wday < 0)
        t->tm_wday = 6;
    t->tm_wday %=7;
}

gint orage_days_between(struct tm *t1, struct tm *t2)
{
    GDate *g_t1, *g_t2;
    gint dd;
    g_t1 = g_date_new_dmy(t1->tm_mday, t1->tm_mon, t1->tm_year);
    g_t2 = g_date_new_dmy(t2->tm_mday, t2->tm_mon, t2->tm_year);
    dd = g_date_days_between(g_t1, g_t2);
    g_date_free(g_t1);
    g_date_free(g_t2);
    return(dd);
}

gint vcal_view_set_calendar_page(GtkWidget *to_show, GCallback cb, gpointer data)
{
    SummaryView *summaryview = NULL;
    gint selsig = -1;
    if (mainwindow_get_mainwindow()) {
	summaryview = mainwindow_get_mainwindow()->summaryview;
	gtk_container_add(GTK_CONTAINER(summaryview->mainwidget_book),
		to_show);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(summaryview->mainwidget_book),
		gtk_notebook_page_num(GTK_NOTEBOOK(summaryview->mainwidget_book), 
		to_show));
	main_window_set_menu_sensitive(mainwindow_get_mainwindow());
	toolbar_main_set_sensitive(mainwindow_get_mainwindow());
	selsig = g_signal_connect(G_OBJECT(summaryview->ctree), "tree_select_row",
			 G_CALLBACK(cb), data);
    }
    return selsig;
}

void vcal_view_set_summary_page(GtkWidget *to_remove, guint selsig)
{
    SummaryView *summaryview = NULL;
    if (mainwindow_get_mainwindow()) {
	summaryview = mainwindow_get_mainwindow()->summaryview;
        if (selsig)
        	g_signal_handler_disconnect(G_OBJECT(summaryview->ctree), selsig);
	gtk_container_remove(GTK_CONTAINER(summaryview->mainwidget_book),
		to_remove);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(summaryview->mainwidget_book),
		gtk_notebook_page_num(GTK_NOTEBOOK(summaryview->mainwidget_book), 
		summaryview->scrolledwin));
	main_window_set_menu_sensitive(mainwindow_get_mainwindow());
	toolbar_main_set_sensitive(mainwindow_get_mainwindow());
    }
	
}

void vcal_view_select_event (const gchar *uid, FolderItem *item, gboolean edit,
		    	    GCallback block_cb, gpointer block_data)
{
	if (edit) {
    		VCalEvent *event = NULL;
        	event = vcal_manager_load_event(uid);
		if (event) {
			vcal_meeting_create(event);
			vcal_manager_free_event(event);
        	}
	} else {
        	SummaryView *summaryview = NULL;
		if (mainwindow_get_mainwindow()) {
		   MsgInfo *info = folder_item_get_msginfo_by_msgid(item, uid);
		   if (info) {
			   summaryview = mainwindow_get_mainwindow()->summaryview;
			   g_signal_handlers_block_by_func(G_OBJECT(summaryview->ctree),
					       G_CALLBACK(block_cb), block_data);
			   summary_select_by_msgnum(summaryview, info->msgnum);
			   summary_display_msg_selected(summaryview, FALSE);
			   procmsg_msginfo_free(info);
			   g_signal_handlers_unblock_by_func(G_OBJECT(summaryview->ctree),
					       G_CALLBACK(block_cb), block_data);
		   }
		}
	}
}

void vcal_view_create_popup_menus(gpointer data, 
				GtkWidget **view_menu, GtkItemFactory **view_fact,
				GtkWidget **event_menu, GtkItemFactory **event_fact)
{
	gint n_entries;
	GtkWidget *view_popupmenu;
	GtkItemFactory *view_popupfactory;
	GtkWidget *view_event_popupmenu;
	GtkItemFactory *view_event_popupfactory;

	n_entries = sizeof(view_popup_entries) /
		sizeof(view_popup_entries[0]);
	view_popupmenu = menu_create_items(view_popup_entries, n_entries,
				      "<UriPopupMenu>", &view_popupfactory,
				      data);

	n_entries = sizeof(view_event_popup_entries) /
		sizeof(view_event_popup_entries[0]);
	view_event_popupmenu = menu_create_items(view_event_popup_entries, n_entries,
				      "<UriPopupMenu>", &view_event_popupfactory,
				      data);
	*view_menu = view_popupmenu;
	*view_fact = view_popupfactory;
	*event_menu= view_event_popupmenu;
	*event_fact= view_event_popupfactory;
}

static void view_new_meeting_cb			(GtkWidget	*widget,
						 guint		 action,
						 void		*user_data)
{
	gpointer data_i = g_object_get_data(G_OBJECT(widget), "menu_data_i");
	gpointer data_s = g_object_get_data(G_OBJECT(widget), "menu_data_s");
	gpointer win = g_object_get_data(G_OBJECT(widget), "menu_win");
	void (*cb)(gpointer win, gpointer data_i, gpointer data_s) = 
		g_object_get_data(G_OBJECT(widget), "new_meeting_cb");
	if (cb)
		cb(win, data_i, data_s);
}
static void view_edit_meeting_cb		(GtkWidget	*widget,
						 guint		 action,
						 void		*user_data)
{
	gpointer data_i = g_object_get_data(G_OBJECT(widget), "menu_data_i");
	gpointer data_s = g_object_get_data(G_OBJECT(widget), "menu_data_s");
	gpointer win = g_object_get_data(G_OBJECT(widget), "menu_win");
	void (*cb)(gpointer win, gpointer data_i, gpointer data_s) = 
		g_object_get_data(G_OBJECT(widget), "edit_meeting_cb");
	if (cb)
		cb(win, data_i, data_s);
}
static void view_cancel_meeting_cb		(GtkWidget	*widget,
						 guint		 action,
						 void		*user_data)
{
	gpointer data_i = g_object_get_data(G_OBJECT(widget), "menu_data_i");
	gpointer data_s = g_object_get_data(G_OBJECT(widget), "menu_data_s");
	gpointer win = g_object_get_data(G_OBJECT(widget), "menu_win");
	void (*cb)(gpointer win, gpointer data_i, gpointer data_s) = 
		g_object_get_data(G_OBJECT(widget), "cancel_meeting_cb");
	if (cb)
		cb(win, data_i, data_s);
}
static void view_go_today_cb			(GtkWidget	*widget,
						 guint		 action,
						 void		*user_data)
{
	gpointer data_i = g_object_get_data(G_OBJECT(widget), "menu_data_i");
	gpointer data_s = g_object_get_data(G_OBJECT(widget), "menu_data_s");
	gpointer win = g_object_get_data(G_OBJECT(widget), "menu_win");
	void (*cb)(gpointer win, gpointer data_i, gpointer data_s) = 
		g_object_get_data(G_OBJECT(widget), "go_today_cb");
	if (cb)
		cb(win, data_i, data_s);
}

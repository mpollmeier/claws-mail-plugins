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

#define MAX_DAYS 40
struct _day_win
{
    GtkAccelGroup *accel_group;
    GtkTooltips   *Tooltips;

    GtkWidget *Window;
    GtkWidget *Vbox;

    GtkWidget *Menubar;
    GtkWidget *File_menu;
    GtkWidget *File_menu_new;
    GtkWidget *File_menu_close;
    GtkWidget *View_menu;
    GtkWidget *View_menu_refresh;
    GtkWidget *Go_menu;
    GtkWidget *Go_menu_today;
    GtkWidget *Go_menu_prev;
    GtkWidget *Go_menu_next;

    GtkWidget *Toolbar;
    GtkWidget *Create_toolbutton;
    GtkWidget *Previous_toolbutton;
    GtkWidget *Today_toolbutton;
    GtkWidget *Next_toolbutton;
    GtkWidget *Refresh_toolbutton;
    GtkWidget *Close_toolbutton;

    GtkWidget *StartDate_button;
    GtkRequisition StartDate_button_req;
    GtkWidget *day_spin;

    GtkWidget *day_view_vbox;
    GtkWidget *scroll_win_h;
    GtkWidget *dtable_h; /* header of day table */
    GtkWidget *scroll_win;
    GtkWidget *dtable;   /* day table */
    GtkRequisition hour_req;

    GtkWidget *header[MAX_DAYS];
    GtkWidget *element[24][MAX_DAYS];
    GtkWidget *line[24][MAX_DAYS];

    guint upd_timer;
    gdouble scroll_pos; /* remember the scroll position */

    GdkColor bg1, bg2, line_color, bg_today, fg_sunday;
    GList    *apptw_list; /* keep track of appointments being updated */
    struct tm startdate;
    FolderItem *item;
};

gchar *get_locale_date(struct tm *tmdate)
{
	gchar *d = g_malloc(100);
	strftime(d, 99, "%x", tmdate);
	return d;
}

static void do_appt_win(char *mode, char *uid, day_win *dw)
{
#if 0
    appt_win *apptw;

    apptw = create_appt_win(mode, uid);
    if (apptw) {
        /* we started this, so keep track of it */
        dw->apptw_list = g_list_prepend(dw->apptw_list, apptw);
        /* inform the appointment that we are interested in it */
        apptw->dw = dw;
    }
#endif
};

static void set_scroll_position(day_win *dw)
{
    GtkAdjustment *v_adj;

    v_adj = gtk_scrolled_window_get_vadjustment(
            GTK_SCROLLED_WINDOW(dw->scroll_win));
    if (dw->scroll_pos > 0) /* we have old value */
        gtk_adjustment_set_value(v_adj, dw->scroll_pos);
    else if (dw->scroll_pos < 0)
        /* default: let's try to start roughly from line 8 = 8 o'clock */
        gtk_adjustment_set_value(v_adj, v_adj->upper/3);
    gtk_adjustment_changed(v_adj);
}

static gboolean scroll_position_timer(gpointer user_data)
{
    set_scroll_position((day_win *)user_data);
    return(FALSE); /* only once */
}

static void get_scroll_position(day_win *dw)
{
    GtkAdjustment *v_adj;

    v_adj = gtk_scrolled_window_get_vadjustment(
            GTK_SCROLLED_WINDOW(dw->scroll_win));
    dw->scroll_pos = gtk_adjustment_get_value(v_adj);
}

static GtkWidget *build_line(day_win *dw, gint left_x, gint top_y
        , gint width, gint height, GtkWidget *hour_line)
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
        gdk_gc_set_foreground(pic1_gc, &dw->line_color);
        gdk_draw_rectangle(pic1, pic1_gc, TRUE, left_x, top_y, width, height);
    }
    else {
        gdk_draw_rectangle(pic1, pic1_gc, TRUE, left_x, top_y, width, height);
    }
    
    new_hour_line = gtk_image_new_from_pixmap(pic1, NULL);
    g_object_unref(pic1_gc);
    g_object_unref(pic1);
    return(new_hour_line);
}

void dw_close_window(day_win *dw)
{
    SummaryView *summaryview = NULL;
    if (mainwindow_get_mainwindow()) {
	summaryview = mainwindow_get_mainwindow()->summaryview;
	gtk_container_remove(GTK_CONTAINER(summaryview->mainwidget_book),
		dw->Vbox);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(summaryview->mainwidget_book),
		gtk_notebook_page_num(GTK_NOTEBOOK(summaryview->mainwidget_book), 
		summaryview->scrolledwin));
	main_window_set_menu_sensitive(mainwindow_get_mainwindow());
	toolbar_main_set_sensitive(mainwindow_get_mainwindow());
    }
    gtk_object_destroy(GTK_OBJECT(dw->Tooltips));
    g_free(dw);
    dw = NULL;
}

/* move one day forward or backward */
static void orage_move_day(struct tm *t, int day)
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

char *orage_tm_date_to_i18_date(struct tm *tm_date)
{
    static char i18_date[32];

    if (strftime(i18_date, 32, "%x", tm_date) == 0)
        g_error("Orage: orage_tm_date_to_i18_date too long string in strftime");
    return(i18_date);
}

static void on_Today_clicked(GtkButton *b, day_win *dw)
{
    time_t t = time(NULL);
    localtime_r(&t, &(dw->startdate));
    refresh_day_win(dw);
}


static void changeSelectedDate(day_win *dw, gint day)
{
    if (day > 0) {
     do {
      orage_move_day(&(dw->startdate), 1);
     } while (--day > 0);
    } else {
     do {
      orage_move_day(&(dw->startdate), -1);
     } while (++day < 0);
    }
    refresh_day_win(dw);
}

static gint on_Previous_clicked(GtkWidget *button, GdkEventButton *event,
				    day_win *dw)
{
    int days = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dw->day_spin));
    changeSelectedDate(dw, -days);
    return TRUE;
}

static gint on_Next_clicked(GtkWidget *button, GdkEventButton *event,
				    day_win *dw)
{
    int days = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dw->day_spin));
    changeSelectedDate(dw, +days);
    return TRUE;
}

static gboolean upd_day_view(day_win *dw)
{
    static guint day_cnt=-1;
    guint day_cnt_n;

    /* we only need to do this if it is really a new day count. We may get
     * many of these while spin button is changing day count and it is enough
     * to show only the last one, which is visible */
    day_cnt_n = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dw->day_spin));
    if (day_cnt != day_cnt_n) { /* need really do it */
        refresh_day_win(dw);
        day_cnt = day_cnt_n;
    }
    dw->upd_timer = 0;
    return(FALSE); /* we do this only once */
}

static void on_spin_changed(GtkSpinButton *b, gpointer *user_data)
{
    day_win *dw = (day_win *)user_data;

    /* refresh_day_win is rather heavy (=slow), so doing it here 
     * is not a good idea. We can't keep up with repeated quick presses 
     * if we do the whole thing here. So let's throw it to background 
     * and do it later. */
    if (dw->upd_timer) {
        g_source_remove(dw->upd_timer);       
    }
    dw->upd_timer = g_timeout_add(500, (GtkFunction)upd_day_view, dw);
}

static void header_button_clicked_cb(GtkWidget *button, day_win *dw)
{
    int offset = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "offset"));
    struct tm tm_date = dw->startdate;
    while (offset > 0) {
	    orage_move_day(&tm_date, 1);
	    offset--;
    }
    tm_date.tm_hour = 0;
    vcal_meeting_create_with_start(NULL, &tm_date);
}

static void on_button_press_event_cb(GtkWidget *widget
        , GdkEventButton *event, gpointer *user_data)
{
    day_win *dw = (day_win *)user_data;
    gchar *uid;
    uid = g_object_get_data(G_OBJECT(widget), "UID");

    if (event->type==GDK_2BUTTON_PRESS) {
    	VCalEvent *event = NULL;
        uid = g_object_get_data(G_OBJECT(widget), "UID");
        event = vcal_manager_load_event(uid);
	if (event) {
		vcal_meeting_create(event);
		vcal_manager_free_event(event);
        }
    } else {
        SummaryView *summaryview = NULL;
	if (mainwindow_get_mainwindow()) {
	   MsgInfo *info = folder_item_get_msginfo_by_msgid(dw->item, uid);
	   if (info) {
		   summaryview = mainwindow_get_mainwindow()->summaryview;
		   summary_select_by_msgnum(summaryview, info->msgnum);
		   summary_display_msg_selected(summaryview, FALSE);
		   procmsg_msginfo_free(info);
	   }
	}
    }
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

static void add_row(day_win *dw, VCalEvent *event, gint days)
{
    gint row, start_row, end_row;
    gint col, start_col, end_col, first_col, last_col;
    gint height, start_height, end_height;
    gchar *text, *tip, *start_date, *end_date;
    GtkWidget *ev, *lab, *hb;
    time_t t_start, t_end;
    struct tm tm_first, tm_start, tm_end;
    GdkColor *color;

    /* First clarify timings */
    t_start = icaltime_as_timet(icaltime_from_string(event->dtstart));
    t_end = icaltime_as_timet(icaltime_from_string(event->dtend));

    localtime_r(&t_start, &tm_start);
    localtime_r(&t_end, &tm_end);
    tm_first = dw->startdate;
   
    tm_first.tm_year += 1900;
    tm_first.tm_mon += 1;
    tm_start.tm_year += 1900;
    tm_start.tm_mon += 1;
    tm_end.tm_year += 1900;
    tm_end.tm_mon += 1;

    start_col = orage_days_between(&tm_first, &tm_start)+1;
    end_col   = orage_days_between(&tm_first, &tm_end)+1;

    if (end_col < 1)
    	return;
    if (start_col > days)
        return;

    else {
        col = start_col;
        row = tm_start.tm_hour;
    }

    /* then add the appointment */
    text = g_strdup(event->summary?event->summary : _("Unknown"));
    ev = gtk_event_box_new();
    lab = gtk_label_new(text);
    gtk_container_add(GTK_CONTAINER(ev), lab);

    if ((row % 2) == 1)
        gtk_widget_modify_bg(ev, GTK_STATE_NORMAL, &dw->bg1);
    if (dw->element[row][col] == NULL) {
        hb = gtk_hbox_new(TRUE, 3);
        dw->element[row][col] = hb;
    }
    else {
        hb = dw->element[row][col];
        /* FIXME: set some real bar here to make it visible that we
         * have more than 1 appointment here
         */
    }
    if (orage_days_between(&tm_start, &tm_end) == 0)
        tip = g_strdup_printf("%s\n%02d:%02d-%02d:%02d\n%s"
                , text, tm_start.tm_hour, tm_start.tm_min
                , tm_end.tm_hour, tm_end.tm_min, event->description);
    else {
/* we took the date in unnormalized format, so we need to do that now */
        start_date = g_strdup(orage_tm_date_to_i18_date(&tm_start));
        end_date = g_strdup(orage_tm_date_to_i18_date(&tm_end));
        tip = g_strdup_printf("%s\n%s %02d:%02d - %s %02d:%02d\n%s"
                , text
                , start_date, tm_start.tm_hour, tm_start.tm_min
                , end_date, tm_end.tm_hour, tm_end.tm_min
                , event->description);
        g_free(start_date);
        g_free(end_date);
    }
    gtk_tooltips_set_tip(dw->Tooltips, ev, tip, NULL);
    /*
    gtk_box_pack_start(GTK_BOX(hb2), ev, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hb), hb2, TRUE, TRUE, 0);
    */
    gtk_box_pack_start(GTK_BOX(hb), ev, TRUE, TRUE, 0);
    g_object_set_data_full(G_OBJECT(ev), "UID", g_strdup(event->uid), g_free);
    g_signal_connect((gpointer)ev, "button-press-event"
            , G_CALLBACK(on_button_press_event_cb), dw);
    g_free(tip);
    g_free(text);

    /* and finally draw the line to show how long the appointment is,
     * but only if it is Busy type event (=availability != 0) 
     * and it is not whole day event */
    if (TRUE) {
        height = dw->StartDate_button_req.height;
        /*
         * same_date = !strncmp(start_ical_time, end_ical_time, 8);
         * */
        if (start_col < 1)
            first_col = 1;
        else
            first_col = start_col;
        if (end_col > days)
            last_col = days;
        else
            last_col = end_col;
        for (col = first_col; col <= last_col; col++) {
            if (col == start_col)
                start_row = tm_start.tm_hour;
            else
                start_row = 0;
            if (col == end_col)
                end_row   = tm_end.tm_hour;
            else
                end_row   = 23;
            for (row = start_row; row <= end_row; row++) {
                if (row == tm_start.tm_hour && col == start_col)
                    start_height = tm_start.tm_min*height/60;
                else
                    start_height = 0;
                if (row == tm_end.tm_hour && col == end_col)
                    end_height = tm_end.tm_min*height/60;
                else
                    end_height = height;
                dw->line[row][col] = build_line(dw, 1, start_height
                        , 2, end_height-start_height, dw->line[row][col]);
            }
        }
    }
}

static void app_rows(day_win *dw, FolderItem *item)
{
   GSList *events = vcal_get_events_list(item);
   GSList *cur = NULL;
   int days = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dw->day_spin));
   for (cur = events; cur ; cur = cur->next) {
   	VCalEvent *event = (VCalEvent *) (cur->data);
	add_row(dw, event, days);
	vcal_manager_free_event(event);
   }
}

static void app_data(day_win *dw, FolderItem *item)
{
    app_rows(dw, item);
}


static void fill_days(day_win *dw, gint days, FolderItem *item)
{
    gint row, col, height, width;
    GtkWidget *ev, *hb;

    height = dw->StartDate_button_req.height;
    width = dw->StartDate_button_req.width;

    /* first clear the structure */
    for (col = 1; col <  days+1; col++) {
        dw->header[col] = NULL;
        for (row = 0; row < 24; row++) {
            dw->element[row][col] = NULL;
    /* gdk_draw_rectangle(, , , left_x, top_y, width, height); */
            dw->line[row][col] = build_line(dw, 0, 0, 3, height, NULL);
        }
    }

    app_data(dw, item);

    for (col = 1; col < days+1; col++) {
        hb = gtk_hbox_new(FALSE, 0);
        /* check if we have full day events and put them to header */
        if (dw->header[col]) {
            gtk_box_pack_start(GTK_BOX(hb), dw->header[col], TRUE, TRUE, 0);
            gtk_widget_set_size_request(hb, width, -1);
        }
        else {
            ev = gtk_event_box_new();
            gtk_widget_modify_bg(ev, GTK_STATE_NORMAL, &dw->bg2);
            gtk_box_pack_start(GTK_BOX(hb), ev, TRUE, TRUE, 0);
        }
        gtk_table_attach(GTK_TABLE(dw->dtable_h), hb, col, col+1, 1, 2
                 , (GTK_FILL), (0), 0, 0);

        /* check rows */
        for (row = 0; row < 24; row++) {
            hb = gtk_hbox_new(FALSE, 0);
            if (row == 0)
                gtk_widget_set_size_request(hb, width, -1);
            if (dw->element[row][col]) {
                gtk_box_pack_start(GTK_BOX(hb), dw->line[row][col]
                        , FALSE, FALSE, 0);
                gtk_box_pack_start(GTK_BOX(hb), dw->element[row][col]
                        , TRUE, TRUE, 0);
                gtk_widget_set_size_request(hb, width, -1);
            }
            else {
                ev = gtk_event_box_new();
                /*
                name = gtk_label_new(" ");
                gtk_container_add(GTK_CONTAINER(ev), name);
                */
                if ((row % 2) == 1)
                    gtk_widget_modify_bg(ev, GTK_STATE_NORMAL, &dw->bg1);
                gtk_box_pack_start(GTK_BOX(hb), dw->line[row][col]
                        , FALSE, FALSE, 0);
                gtk_box_pack_start(GTK_BOX(hb), ev, TRUE, TRUE, 0);
            }
            gtk_table_attach(GTK_TABLE(dw->dtable), hb, col, col+1, row, row+1
                     , (GTK_FILL), (0), 0, 0);
        }
    }
}

static void build_day_view_header(day_win *dw, char *start_date)
{
    GtkWidget *hbox, *label, *space_label;
    SummaryView *summaryview = NULL;
    int avail_w = 0, avail_d = 0;

    hbox = gtk_hbox_new(FALSE, 0);

    label = gtk_label_new(_("Start"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);

    /* start date button */
    dw->StartDate_button = gtk_button_new();
    gtk_box_pack_start(GTK_BOX(hbox), dw->StartDate_button, FALSE, FALSE, 0);

    space_label = gtk_label_new("  ");
    gtk_box_pack_start(GTK_BOX(hbox), space_label, FALSE, FALSE, 0);

    space_label = gtk_label_new("     ");
    gtk_box_pack_start(GTK_BOX(hbox), space_label, FALSE, FALSE, 0);

    label = gtk_label_new(_("Show"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);

    /* show days spin = how many days to show */
    dw->day_spin = gtk_spin_button_new_with_range(1, MAX_DAYS, 1);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(dw->day_spin), TRUE);
    gtk_widget_set_size_request(dw->day_spin, 40, -1);
    gtk_box_pack_start(GTK_BOX(hbox), dw->day_spin, FALSE, FALSE, 0);
    label = gtk_label_new(_("days"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

    space_label = gtk_label_new("   ");
    gtk_box_pack_start(GTK_BOX(hbox), space_label, FALSE, FALSE, 0);

    /* sizes */
    gtk_button_set_label(GTK_BUTTON(dw->StartDate_button)
            , (const gchar *)start_date);
    gtk_widget_size_request(dw->StartDate_button, &dw->StartDate_button_req);
    dw->StartDate_button_req.width += dw->StartDate_button_req.width/10;
    label = gtk_label_new("00");
    gtk_widget_size_request(label, &dw->hour_req);

    if (mainwindow_get_mainwindow()) {
        GtkAllocation allocation;
	summaryview = mainwindow_get_mainwindow()->summaryview;
	allocation = summaryview->mainwidget_book->allocation;
	
	avail_w = allocation.width - 20 - 2*(dw->hour_req.width);
	avail_d = avail_w / dw->StartDate_button_req.width;
    }
   
    /* initial values */
    if (avail_d)
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(dw->day_spin), avail_d);

    g_signal_connect((gpointer)dw->day_spin, "value-changed"
            , G_CALLBACK(on_spin_changed), dw);
}

static void build_day_view_colours(day_win *dw)
{
    GtkStyle *def_style;
    GdkColormap *pic1_cmap;

    def_style = gtk_widget_get_default_style();
    pic1_cmap = gdk_colormap_get_system();
    dw->bg1 = def_style->bg[GTK_STATE_NORMAL];
    dw->bg1.red +=  (dw->bg1.red < 64000 ? 1000 : -1000);
    dw->bg1.green += (dw->bg1.green < 64000 ? 1000 : -1000);
    dw->bg1.blue += (dw->bg1.blue < 64000 ? 1000 : -1000);
    gdk_colormap_alloc_color(pic1_cmap, &dw->bg1, FALSE, TRUE);

    dw->bg2 = def_style->bg[GTK_STATE_NORMAL];
    dw->bg2.red +=  (dw->bg2.red > 1000 ? -1000 : 1000);
    dw->bg2.green += (dw->bg2.green > 1000 ? -1000 : 1000);
    dw->bg2.blue += (dw->bg2.blue > 2000 ? -2000 : 2000);
    gdk_colormap_alloc_color(pic1_cmap, &dw->bg2, FALSE, TRUE);

    if (!gdk_color_parse("white", &dw->line_color)) {
        dw->line_color.red =  239 * (65535/255);
        dw->line_color.green = 235 * (65535/255);
        dw->line_color.blue = 230 * (65535/255);
    }
    gdk_colormap_alloc_color(pic1_cmap, &dw->line_color, FALSE, TRUE);

    if (!gdk_color_parse("blue", &dw->fg_sunday)) {
        g_warning("color parse failed: red\n");
        dw->fg_sunday.red = 10 * (65535/255);
        dw->fg_sunday.green = 10 * (65535/255);
        dw->fg_sunday.blue = 255 * (65535/255);
    }
    gdk_colormap_alloc_color(pic1_cmap, &dw->fg_sunday, FALSE, TRUE);

    if (!gdk_color_parse("gold", &dw->bg_today)) {
        g_warning("color parse failed: gold\n");
        dw->bg_today.red = 255 * (65535/255);
        dw->bg_today.green = 215 * (65535/255);
        dw->bg_today.blue = 115 * (65535/255);
    }
    gdk_colormap_alloc_color(pic1_cmap, &dw->bg_today, FALSE, TRUE);
}

static void fill_hour(day_win *dw, gint col, gint row, char *text)
{
    GtkWidget *name, *ev;

    ev = gtk_event_box_new();
    name = gtk_label_new(text);
    gtk_container_add(GTK_CONTAINER(ev), name);
    if ((row % 2) == 1)
        gtk_widget_modify_bg(ev, GTK_STATE_NORMAL, &dw->bg1);
    gtk_widget_set_size_request(ev, dw->hour_req.width
            , dw->StartDate_button_req.height);
    if (text)
        gtk_table_attach(GTK_TABLE(dw->dtable), ev, col, col+1, row, row+1
             , (GTK_FILL), (0), 0, 0);
    else  /* special, needed for header table full day events */
        gtk_table_attach(GTK_TABLE(dw->dtable_h), ev, col, col+1, row, row+1
             , (GTK_FILL), (0), 0, 0);
}

static void build_day_view_table(day_win *dw)
{
    gint days;   /* number of days to show */
    int year, month, day;
    gint i = 0;
    GtkWidget *label, *button;
    char text[5+1], *date, *today;
    struct tm tm_date, tm_today;
    guint monthdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    GtkWidget *vp;
    time_t t = time(NULL);
    GtkWidget *arrow;
    gchar *tip;

    localtime_r(&t, &tm_today);
    days = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dw->day_spin));
    today = get_locale_date(&tm_today);
    /****** header of day table = days columns ******/
    dw->scroll_win_h = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(dw->scroll_win_h)
            , GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
    gtk_box_pack_start(GTK_BOX(dw->Vbox), dw->scroll_win_h
            , TRUE, TRUE, 0);
    dw->day_view_vbox = gtk_vbox_new(FALSE, 0);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(dw->scroll_win_h)
            , dw->day_view_vbox);
    /*
    gtk_container_add(GTK_CONTAINER(dw->scroll_win_h), dw->day_view_vbox);
    */
    /* row 1= day header buttons 
     * row 2= full day events after the buttons */
    dw->dtable_h = gtk_table_new(2 , days+2, FALSE);
    gtk_box_pack_start(GTK_BOX(dw->day_view_vbox), dw->dtable_h
            , FALSE, FALSE, 0);

    tm_date = dw->startdate;

    year = tm_date.tm_year + 1900;
    month = tm_date.tm_mon;
    day = tm_date.tm_mday;
    if (((tm_date.tm_year%4) == 0) && (((tm_date.tm_year%100) != 0) 
            || ((tm_date.tm_year%400) == 0)))
        ++monthdays[1];


    i=0;
    dw->Previous_toolbutton = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(dw->Previous_toolbutton), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(dw->Previous_toolbutton), 0);
    arrow = gtk_arrow_new(GTK_ARROW_LEFT, GTK_SHADOW_NONE);
    gtk_container_add(GTK_CONTAINER(dw->Previous_toolbutton), arrow);
    gtk_table_attach(GTK_TABLE(dw->dtable_h), dw->Previous_toolbutton, i, i+1, 0, 1
                , (GTK_FILL), (0), 0, 0);
    gtk_widget_show_all(dw->Previous_toolbutton);
    g_signal_connect((gpointer)dw->Previous_toolbutton, "button_release_event"
            , G_CALLBACK(on_Previous_clicked), dw);
    tip = g_strdup_printf("Back %d days", days);
    gtk_tooltips_set_tip(dw->Tooltips, dw->Previous_toolbutton, tip, NULL);
    g_free(tip);
    for (i = 1; i < days+1; i++) {
        tip = g_malloc(100);
	strftime(tip, 99, "%A %d %B %Y", &tm_date);
        date = get_locale_date(&tm_date);
        button = gtk_button_new();
        gtk_button_set_label(GTK_BUTTON(button), date);
        if (strcmp(today, date) == 0) {
            gtk_widget_modify_bg(button, GTK_STATE_NORMAL, &dw->bg_today);
        }

        if (tm_date.tm_wday % 7 == 0) {
            label = gtk_bin_get_child(GTK_BIN(button));
            gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &dw->fg_sunday);
        }
        gtk_tooltips_set_tip(dw->Tooltips, button, tip, NULL);
	g_free(tip);
        gtk_widget_set_size_request(button, dw->StartDate_button_req.width, -1);
        g_signal_connect((gpointer)button, "clicked"
                , G_CALLBACK(header_button_clicked_cb), dw);
        g_object_set_data(G_OBJECT(button), "offset", GINT_TO_POINTER(i-1));
        gtk_table_attach(GTK_TABLE(dw->dtable_h), button, i, i+1, 0, 1
                , (GTK_FILL), (0), 0, 0);

        if (++tm_date.tm_mday == (monthdays[tm_date.tm_mon]+1)) {
            if (++tm_date.tm_mon == 12) {
                ++tm_date.tm_year;
                tm_date.tm_mon = 0;
            }
            tm_date.tm_mday = 1;
        }
	++tm_date.tm_wday; tm_date.tm_wday %=7;
	g_free(date);
    }

    dw->Next_toolbutton = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(dw->Next_toolbutton), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(dw->Next_toolbutton), 0);
    arrow = gtk_arrow_new(GTK_ARROW_RIGHT, GTK_SHADOW_NONE);
    gtk_container_add(GTK_CONTAINER(dw->Next_toolbutton), arrow);
    gtk_table_attach(GTK_TABLE(dw->dtable_h), dw->Next_toolbutton, i, i+1, 0, 1
                , (GTK_FILL), (0), 0, 0);
    gtk_widget_show_all(dw->Next_toolbutton);
    g_signal_connect((gpointer)dw->Next_toolbutton, "button_release_event"
            , G_CALLBACK(on_Next_clicked), dw);
    tip = g_strdup_printf("Forward %d days", days);
    gtk_tooltips_set_tip(dw->Tooltips, dw->Next_toolbutton, tip, NULL);

    g_free(today);

    /****** body of day table ******/
    dw->scroll_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(dw->scroll_win)
            , GTK_SHADOW_NONE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(dw->scroll_win)
            , GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_scrolled_window_set_placement(GTK_SCROLLED_WINDOW(dw->scroll_win)
            , GTK_CORNER_TOP_LEFT);
    gtk_box_pack_start(GTK_BOX(dw->day_view_vbox), dw->scroll_win
            , TRUE, TRUE, 0);
    vp = gtk_viewport_new(NULL, NULL);
    gtk_viewport_set_shadow_type(GTK_VIEWPORT(vp), GTK_SHADOW_NONE);
    gtk_container_add(GTK_CONTAINER(dw->scroll_win), vp);
    dw->dtable = gtk_table_new(24, days+2, FALSE);
    gtk_container_add(GTK_CONTAINER(vp), dw->dtable);

    gtk_widget_show_all(dw->dtable_h);
    /* hours column = hour rows */
    for (i = 0; i < 24; i++) {
        g_sprintf(text, "%02d", i);
        /* ev is needed for background colour */
        fill_hour(dw, 0, i, text);
        fill_hour(dw, days+1, i, text);
    }
    fill_days(dw, days, dw->item);
}

void refresh_day_win(day_win *dw)
{
printf("refresh\n");
    get_scroll_position(dw);
    gtk_widget_destroy(dw->scroll_win_h);
    build_day_view_table(dw);
    gtk_widget_show_all(dw->scroll_win_h);
    /* I was not able to get this work without the timer. Ugly yes, but
     * it works and does not hurt - much */
    g_timeout_add(100, (GtkFunction)scroll_position_timer, (gpointer)dw);
}

day_win *create_day_win(FolderItem *item, struct tm tmdate)
{
    day_win *dw;
    char *start_date = get_locale_date(&tmdate);
    SummaryView *summaryview = NULL;
    
    /* initialisation + main window + base vbox */
    dw = g_new0(day_win, 1);
    dw->scroll_pos = -1; /* not set */
    dw->Tooltips = gtk_tooltips_new();
    dw->accel_group = gtk_accel_group_new();
    dw->startdate = tmdate;

    dw->Vbox = gtk_vbox_new(FALSE, 0);

    dw->item = item;
    build_day_view_colours(dw);
    build_day_view_header(dw, start_date);
    build_day_view_table(dw);
    gtk_widget_show_all(dw->Vbox);
    if (mainwindow_get_mainwindow()) {
	summaryview = mainwindow_get_mainwindow()->summaryview;
	gtk_container_add(GTK_CONTAINER(summaryview->mainwidget_book),
		dw->Vbox);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(summaryview->mainwidget_book),
		gtk_notebook_page_num(GTK_NOTEBOOK(summaryview->mainwidget_book), 
		dw->Vbox));
	main_window_set_menu_sensitive(mainwindow_get_mainwindow());
	toolbar_main_set_sensitive(mainwindow_get_mainwindow());
    }

    g_timeout_add(100, (GtkFunction)scroll_position_timer, (gpointer)dw);

    return(dw);
}

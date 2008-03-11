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
struct _month_win
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

    GtkWidget *month_view_vbox;
    GtkWidget *scroll_win_h;
    GtkWidget *dtable_h; /* header of day table */
    GtkWidget *scroll_win;
    GtkWidget *dtable;   /* day table */
    GtkRequisition hour_req;

    GtkWidget *header[MAX_DAYS];
    GtkWidget *element[6][MAX_DAYS];
    GtkWidget *line[6][MAX_DAYS];

    guint upd_timer;
    gdouble scroll_pos; /* remember the scroll position */

    GdkColor bg1, bg2, line_color, bg_today, fg_sunday;
    GList    *apptw_list; /* keep track of appointments being updated */
    struct tm startdate;
    FolderItem *item;
};

gchar *dayname[7] = {
    	N_("Monday"),
    	N_("Tuesday"),
    	N_("Wednesday"),
    	N_("Thursday"),
    	N_("Friday"),
    	N_("Saturday"),
    	N_("Sunday")
	};
gchar *monthname[12] = {
    	N_("January"),
    	N_("February"),
    	N_("March"),
    	N_("April"),
    	N_("May"),
    	N_("June"),
    	N_("July"),
    	N_("August"),
    	N_("September"),
    	N_("October"),
    	N_("November"),
    	N_("December")
	};

static gchar *get_locale_date(struct tm *tmdate)
{
	gchar *d = g_malloc(100);
	strftime(d, 99, "%x", tmdate);
	return d;
}

static void do_appt_win(char *mode, char *uid, month_win *mw)
{
#if 0
    appt_win *apptw;

    apptw = create_appt_win(mode, uid);
    if (apptw) {
        /* we started this, so keep track of it */
        mw->apptw_list = g_list_prepend(mw->apptw_list, apptw);
        /* inform the appointment that we are interested in it */
        apptw->mw = mw;
    }
#endif
};

static void set_scroll_position(month_win *mw)
{
    GtkAdjustment *v_adj;
    return;
    v_adj = gtk_scrolled_window_get_vadjustment(
            GTK_SCROLLED_WINDOW(mw->scroll_win));
    if (mw->scroll_pos > 0) /* we have old value */
        gtk_adjustment_set_value(v_adj, mw->scroll_pos);
    else if (mw->scroll_pos < 0)
        /* default: let's try to start roughly from line 8 = 8 o'clock */
        gtk_adjustment_set_value(v_adj, v_adj->upper/3);
    gtk_adjustment_changed(v_adj);
}

static gboolean scroll_position_timer(gpointer user_data)
{
    set_scroll_position((month_win *)user_data);
    return(FALSE); /* only once */
}

static void get_scroll_position(month_win *mw)
{
    GtkAdjustment *v_adj;

    v_adj = gtk_scrolled_window_get_vadjustment(
            GTK_SCROLLED_WINDOW(mw->scroll_win));
    mw->scroll_pos = gtk_adjustment_get_value(v_adj);
}

static GtkWidget *build_line(month_win *mw, gint left_y, gint top_x
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
        gdk_gc_set_foreground(pic1_gc, &mw->line_color);
        gdk_draw_rectangle(pic1, pic1_gc, TRUE, top_x, left_y, width, height);
    }
    else {
        gdk_draw_rectangle(pic1, pic1_gc, TRUE, top_x, left_y, width, height);
    }
    
    new_hour_line = gtk_image_new_from_pixmap(pic1, NULL);
    g_object_unref(pic1_gc);
    g_object_unref(pic1);
    return(new_hour_line);
}

void mw_close_window(month_win *mw)
{
    SummaryView *summaryview = NULL;
    if (mainwindow_get_mainwindow()) {
	summaryview = mainwindow_get_mainwindow()->summaryview;
	gtk_container_remove(GTK_CONTAINER(summaryview->mainwidget_book),
		mw->Vbox);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(summaryview->mainwidget_book),
		gtk_notebook_page_num(GTK_NOTEBOOK(summaryview->mainwidget_book), 
		summaryview->scrolledwin));
	main_window_set_menu_sensitive(mainwindow_get_mainwindow());
	toolbar_main_set_sensitive(mainwindow_get_mainwindow());
    }
    gtk_object_destroy(GTK_OBJECT(mw->Tooltips));
    g_free(mw);
    mw = NULL;
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

static char *orage_tm_date_to_i18_date(struct tm *tm_date)
{
    static char i18_date[32];

    if (strftime(i18_date, 32, "%x", tm_date) == 0)
        g_error("Orage: orage_tm_date_to_i18_date too long string in strftime");
    return(i18_date);
}

static void on_Today_clicked(GtkButton *b, month_win *mw)
{
    time_t t = time(NULL);
    localtime_r(&t, &(mw->startdate));
    refresh_month_win(mw);
}


static void changeSelectedDate(month_win *mw, gint month)
{
    gint curmon = mw->startdate.tm_mon;
    if (month > 0) {
     do { /* go to first of next month */
      orage_move_day(&(mw->startdate), 1);
     } while (curmon == mw->startdate.tm_mon);
    } else {
     do { /* go to last day of last month */
      orage_move_day(&(mw->startdate), -1);
     } while (curmon == mw->startdate.tm_mon);
     do { /* go to first of last month */
      orage_move_day(&(mw->startdate), -1);
     } while (mw->startdate.tm_mday > 1);
    }
    refresh_month_win(mw);
}

static gint on_Previous_clicked(GtkWidget *button, GdkEventButton *event,
				    month_win *mw)
{
    int days = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(mw->day_spin));
    changeSelectedDate(mw, -1);
    return TRUE;
}

static gint on_Next_clicked(GtkWidget *button, GdkEventButton *event,
				    month_win *mw)
{
    int days = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(mw->day_spin));
    changeSelectedDate(mw, +1);
    return TRUE;
}

static gboolean upd_month_view(month_win *mw)
{
    static guint day_cnt=-1;
    guint day_cnt_n;

    /* we only need to do this if it is really a new day count. We may get
     * many of these while spin button is changing day count and it is enough
     * to show only the last one, which is visible */
    day_cnt_n = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(mw->day_spin));
    if (day_cnt != day_cnt_n) { /* need really do it */
        refresh_month_win(mw);
        day_cnt = day_cnt_n;
    }
    mw->upd_timer = 0;
    return(FALSE); /* we do this only once */
}

static void on_spin_changed(GtkSpinButton *b, gpointer *user_data)
{
    month_win *mw = (month_win *)user_data;

    /* refresh_month_win is rather heavy (=slow), so doing it here 
     * is not a good idea. We can't keep up with repeated quick presses 
     * if we do the whole thing here. So let's throw it to background 
     * and do it later. */
    if (mw->upd_timer) {
        g_source_remove(mw->upd_timer);       
    }
    mw->upd_timer = g_timeout_add(500, (GtkFunction)upd_month_view, mw);
}

static void header_button_clicked_cb(GtkWidget *button
        , GdkEventButton *event, gpointer *user_data)
{
    month_win *mw = (month_win *)user_data;
    int offset = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "day"));
    struct tm tm_date = mw->startdate;

    while (offset > tm_date.tm_mday) {
	    orage_move_day(&tm_date, 1);
    }
    while (offset < tm_date.tm_mday) {
	    orage_move_day(&tm_date, -1);
    }
    tm_date.tm_hour = 0;
    vcal_meeting_create_with_start(NULL, &tm_date);
}

static void on_button_press_event_cb(GtkWidget *widget
        , GdkEventButton *event, gpointer *user_data)
{
    month_win *mw = (month_win *)user_data;
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
	   MsgInfo *info = folder_item_get_msginfo_by_msgid(mw->item, uid);
	   if (info) {
		   summaryview = mainwindow_get_mainwindow()->summaryview;
		   summary_select_by_msgnum(summaryview, info->msgnum);
		   summary_display_msg_selected(summaryview, FALSE);
		   procmsg_msginfo_free(info);
	   }
	}
    }
}

static gint orage_days_between(struct tm *t1, struct tm *t2)
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

static void add_row(month_win *mw, VCalEvent *event, gint days)
{
    gint row, start_row, end_row, first_row, last_row;
    gint col, start_col, end_col, first_col, last_col;
    gint width, start_width, end_width;
    gchar *text, *tip, *start_date, *end_date;
    GtkWidget *ev, *lab, *hb;
    time_t t_start, t_end;
    struct tm tm_first, tm_start, tm_end;
    GdkColor *color;
    guint monthdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int weekoffset = -1;
    gboolean pack = TRUE, update_tip = FALSE;

    /* First clarify timings */
    t_start = icaltime_as_timet(icaltime_from_string(event->dtstart));
    t_end = icaltime_as_timet(icaltime_from_string(event->dtend));

    localtime_r(&t_start, &tm_start);
    localtime_r(&t_end, &tm_end);
    tm_first = mw->startdate;
   
    tm_first.tm_year += 1900;

    if (((tm_first.tm_year%4) == 0) 
    && (((tm_first.tm_year%100) != 0) || ((tm_first.tm_year%400) == 0)))
        ++monthdays[1]; /* leap year, february has 29 days */


    tm_first.tm_mon += 1;
    tm_start.tm_year += 1900;
    tm_start.tm_mon += 1;
    tm_end.tm_year += 1900;
    tm_end.tm_mon += 1;

    start_col = orage_days_between(&tm_first, &tm_start)+1;
    end_col   = orage_days_between(&tm_first, &tm_end)+1;

    if (end_col < 1)
    	return;
    if (start_col > monthdays[tm_first.tm_mon])
        return;

    else {
        GDate *fdate = g_date_new_dmy(1, tm_start.tm_mon, tm_start.tm_year);
        GDate *sdate = g_date_new_dmy(tm_start.tm_mday, tm_start.tm_mon, tm_start.tm_year);
        GDate *edate = g_date_new_dmy(tm_end.tm_mday, tm_end.tm_mon, tm_end.tm_year);

	col = start_col = (int)g_date_get_weekday(sdate);
	end_col = (int)g_date_get_weekday(edate);
	weekoffset = (int)g_date_get_monday_week_of_year(fdate);
	row = start_row = (int)g_date_get_monday_week_of_year(sdate) - weekoffset;
	end_row = (int)g_date_get_monday_week_of_year(edate) - weekoffset;

	g_date_free(fdate);
	g_date_free(sdate);
	g_date_free(edate);
    }

    if (end_col > 5)
       end_col = 5;

    /* then add the appointment */
    text = g_strdup(event->summary?event->summary : _("Unknown"));

    if (mw->element[row][col] == NULL) {
        hb = gtk_vbox_new(TRUE, 1);
        mw->element[row][col] = hb;
    }
    else {
        GList *children = NULL;
        hb = mw->element[row][col];
        /* FIXME: set some real bar here to make it visible that we
         * have more than 1 appointment here
         */
	children = gtk_container_get_children(GTK_CONTAINER(hb));
	if (g_list_length(children) > 2) {
		pack = FALSE;
		update_tip = TRUE;
	} else if (g_list_length(children) > 1) {
		pack = FALSE;
		update_tip = FALSE;
	}
	g_list_free(children);
    }
    if (pack || !update_tip) {
       ev = gtk_event_box_new();
       lab = gtk_label_new(text);
       gtk_misc_set_alignment(GTK_MISC(lab), 0.0, 0.0);
       if ((row % 2) == 1)
           gtk_widget_modify_bg(ev, GTK_STATE_NORMAL, &mw->bg1);
    } else if (!pack && update_tip) {
       GList *children = gtk_container_get_children(GTK_CONTAINER(hb));
       ev = GTK_WIDGET(g_list_last(children)->data);
       g_list_free(children);
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
    if (pack) {
        gtk_container_add(GTK_CONTAINER(ev), lab);
	gtk_tooltips_set_tip(mw->Tooltips, ev, tip, NULL);
        gtk_box_pack_start(GTK_BOX(hb), ev, TRUE, TRUE, 0);
    } else if (!update_tip) {
        gtk_label_set_label(GTK_LABEL(lab), "...");
        gtk_container_add(GTK_CONTAINER(ev), lab);
	gtk_tooltips_set_tip(mw->Tooltips, ev, tip, NULL);
        gtk_box_pack_start(GTK_BOX(hb), ev, TRUE, TRUE, 0);
    } else {
        GtkTooltipsData *tdata = gtk_tooltips_data_get(ev);
	gchar *new = g_strdup_printf("%s\n\n%s", tdata?tdata->tip_text:"", tip);
	gtk_tooltips_set_tip(mw->Tooltips, ev, new, NULL);
	g_free(new);
    }
    g_object_set_data_full(G_OBJECT(ev), "UID", g_strdup(event->uid), g_free);
    g_signal_connect((gpointer)ev, "button-press-event"
            , G_CALLBACK(on_button_press_event_cb), mw);
    g_free(tip);
    g_free(text);

    /* and finally draw the line to show how long the appointment is,
     * but only if it is Busy type event (=availability != 0) 
     * and it is not whole day event */
    if (TRUE) {
        width = mw->StartDate_button_req.width;
        /*
         * same_date = !strncmp(start_ical_time, end_ical_time, 8);
         * */
        if (start_row < 1)
            first_row = 1;
        else
            first_row = start_row;
        if (end_row > 5)
            last_row = 5;
        else
            last_row = end_row;
        for (row = first_row; row <= last_row; row++) {
            if (row == start_row)
                first_col = start_col;
            else
                first_col = 0;
            if (row == end_row)
                last_col   = end_col;
            else
                last_col   = 7;
            for (col = first_col; col <= last_col; col++) {
                if (row == start_row && col == start_col)
                    start_width = ((tm_start.tm_hour*60)+(tm_start.tm_min))*width/(24*60);
                else
                    start_width = 0;
                if (row == last_row && col == last_col)
                    end_width = ((tm_end.tm_hour*60)+(tm_end.tm_min))*width/(24*60);
                else
                    end_width = width;

                mw->line[row][col] = build_line(mw, 0, start_width
                        , end_width-start_width, 2, mw->line[row][col]);
            }
        }
    }
}

static void app_rows(month_win *mw, FolderItem *item)
{
   GSList *events = vcal_get_events_list(item);
   GSList *cur = NULL;
   int days = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(mw->day_spin));
   for (cur = events; cur ; cur = cur->next) {
   	VCalEvent *event = (VCalEvent *) (cur->data);
	add_row(mw, event, days);
	vcal_manager_free_event(event);
   }
   g_slist_free(events);
}

static void app_data(month_win *mw, FolderItem *item)
{
    app_rows(mw, item);
}


static void fill_days(month_win *mw, gint days, FolderItem *item)
{
    gint day, col, height, width;
    GtkWidget *ev, *vb, *hb;
    guint monthdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    struct tm t = mw->startdate;
    int weekoffset = -1;
    time_t now = time(NULL);
    struct tm tm_today;
    
    
    localtime_r(&now, &tm_today);

    t.tm_year += 1900;
    t.tm_mon++;
    tm_today.tm_year += 1900;
    tm_today.tm_mon++;
    
    if (((t.tm_year%4) == 0) 
    && (((t.tm_year%100) != 0) || ((t.tm_year%400) == 0)))
        ++monthdays[1]; /* leap year, february has 29 days */

    height = mw->StartDate_button_req.height;
    width = mw->StartDate_button_req.width;

    /* first clear the structure */
    for (col = 1; col <  days+1; col++) {
        mw->header[col] = NULL;
    }
    for (day = 0; day < MAX_DAYS; day++)
    	for (col = 0; col < 6; col++)
		mw->line[col][day] = NULL;
    for (day = 1; day <= monthdays[t.tm_mon-1]; day++) {
	GDate *date = g_date_new_dmy(day, t.tm_mon, t.tm_year);
	int dcol = (int)g_date_get_weekday(date);
	int row = (int)g_date_get_monday_week_of_year(date);
	if (weekoffset == -1) {
		weekoffset = row;
		row = 0;
	} else {
		row = row - weekoffset;
	}
        mw->element[row][dcol] = NULL;
        mw->line[row][dcol] = build_line(mw, 0, 0, width, 3, NULL);
	g_date_free(date);
    }

    app_data(mw, item);

    /* check rows */
    for (day = 1; day <= monthdays[t.tm_mon-1]; day++) {
	GDate *date = g_date_new_dmy(day, t.tm_mon, t.tm_year);
	int col = (int)g_date_get_weekday(date);
	int row = (int)g_date_get_monday_week_of_year(date);
	gchar *label;
	gchar *tmp;
	GtkWidget *name;

	if (weekoffset == -1) {
		weekoffset = row;
		row = 0;
	} else {
		row = row - weekoffset;
	}
        vb = gtk_vbox_new(FALSE, 0);
        gtk_widget_set_size_request(vb, width, height);
	    label = g_strdup_printf("%d", g_date_get_day(date));
	    tmp = g_strdup_printf("%s %d %s %d",
	    			_(dayname[col-1]),
				g_date_get_day(date),
				_(monthname[g_date_get_month(date)-1]),
				g_date_get_year(date));

            ev = gtk_event_box_new();
	    g_object_set_data(G_OBJECT(ev), "day", GINT_TO_POINTER((int)g_date_get_day(date)));
	    g_signal_connect((gpointer)ev, "button-press-event"
        	    , G_CALLBACK(header_button_clicked_cb), mw);
            name = gtk_label_new(label);
	    gtk_misc_set_alignment(GTK_MISC(name), 0.0, 0.0);
	    gtk_tooltips_set_tip(mw->Tooltips, ev, tmp, NULL);
            gtk_container_add(GTK_CONTAINER(ev), name);
	    g_free(tmp);
            g_free(label);

            if ((row % 2) == 1)
                gtk_widget_modify_bg(ev, GTK_STATE_NORMAL, &mw->bg1);
	    if (col == 7)
	    	gtk_widget_modify_fg(name, GTK_STATE_NORMAL, &mw->fg_sunday);
	    if (day == tm_today.tm_mday && t.tm_mon == tm_today.tm_mon && t.tm_year == tm_today.tm_year)
	    	gtk_widget_modify_bg(ev, GTK_STATE_NORMAL, &mw->bg_today);

            hb = gtk_hbox_new(FALSE, 0);
            gtk_box_pack_start(GTK_BOX(hb), ev, TRUE, TRUE, 1);
            gtk_box_pack_start(GTK_BOX(vb), hb, TRUE, TRUE, 0);
            if (mw->element[row][col]) {
		gtk_box_pack_start(GTK_BOX(vb), mw->element[row][col], TRUE, TRUE, 0);
            }
            gtk_box_pack_start(GTK_BOX(vb), mw->line[row][col]
                    , FALSE, FALSE, 0);

        gtk_table_attach(GTK_TABLE(mw->dtable), vb, col, col+1, row, row+1
                 , (GTK_FILL), (0), 0, 0);
	g_date_free(date);
    }
}

static void build_month_view_header(month_win *mw, char *start_date)
{
    GtkWidget *hbox, *label, *space_label;
    SummaryView *summaryview = NULL;
    int avail_w = 0, avail_d = 0;
    int avail_h = 0;

    hbox = gtk_hbox_new(FALSE, 0);

    label = gtk_label_new(_("Start"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);

    /* start date button */
    mw->StartDate_button = gtk_button_new();
    gtk_box_pack_start(GTK_BOX(hbox), mw->StartDate_button, FALSE, FALSE, 0);

    space_label = gtk_label_new("  ");
    gtk_box_pack_start(GTK_BOX(hbox), space_label, FALSE, FALSE, 0);

    space_label = gtk_label_new("     ");
    gtk_box_pack_start(GTK_BOX(hbox), space_label, FALSE, FALSE, 0);

    label = gtk_label_new(_("Show"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);

    /* show days spin = how many days to show */
    mw->day_spin = gtk_spin_button_new_with_range(1, MAX_DAYS, 1);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(mw->day_spin), TRUE);
    gtk_widget_set_size_request(mw->day_spin, 40, -1);
    gtk_box_pack_start(GTK_BOX(hbox), mw->day_spin, FALSE, FALSE, 0);
    label = gtk_label_new(_("days"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

    space_label = gtk_label_new("   ");
    gtk_box_pack_start(GTK_BOX(hbox), space_label, FALSE, FALSE, 0);

    /* sizes */
    gtk_button_set_label(GTK_BUTTON(mw->StartDate_button)
            , (const gchar *)start_date);
    gtk_widget_size_request(mw->StartDate_button, &mw->StartDate_button_req);
    mw->StartDate_button_req.width += mw->StartDate_button_req.width/10;
    label = gtk_label_new("00");
    gtk_widget_size_request(label, &mw->hour_req);

    if (mainwindow_get_mainwindow()) {
        GtkAllocation allocation;
	summaryview = mainwindow_get_mainwindow()->summaryview;
	allocation = summaryview->mainwidget_book->allocation;
	
	avail_w = allocation.width - 20 - 2*(mw->hour_req.width);
	avail_h = allocation.height - 20;
	avail_d = avail_w / mw->StartDate_button_req.width;
    }
    avail_d = 7;
    gtk_widget_set_size_request(mw->StartDate_button, avail_w / avail_d, 
    			(avail_h - mw->StartDate_button_req.height)/6);
    gtk_widget_size_request(mw->StartDate_button, &mw->StartDate_button_req);
   
    /* initial values */
    if (avail_d)
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(mw->day_spin), avail_d);

    g_signal_connect((gpointer)mw->day_spin, "value-changed"
            , G_CALLBACK(on_spin_changed), mw);
}

static void build_month_view_colours(month_win *mw)
{
    GtkStyle *def_style;
    GdkColormap *pic1_cmap;

    def_style = gtk_widget_get_default_style();
    pic1_cmap = gdk_colormap_get_system();
    mw->bg1 = def_style->bg[GTK_STATE_NORMAL];
    mw->bg1.red +=  (mw->bg1.red < 64000 ? 1000 : -1000);
    mw->bg1.green += (mw->bg1.green < 64000 ? 1000 : -1000);
    mw->bg1.blue += (mw->bg1.blue < 64000 ? 1000 : -1000);
    gdk_colormap_alloc_color(pic1_cmap, &mw->bg1, FALSE, TRUE);

    mw->bg2 = def_style->bg[GTK_STATE_NORMAL];
    mw->bg2.red +=  (mw->bg2.red > 1000 ? -1000 : 1000);
    mw->bg2.green += (mw->bg2.green > 1000 ? -1000 : 1000);
    mw->bg2.blue += (mw->bg2.blue > 2000 ? -2000 : 2000);
    gdk_colormap_alloc_color(pic1_cmap, &mw->bg2, FALSE, TRUE);

    if (!gdk_color_parse("white", &mw->line_color)) {
        mw->line_color.red =  239 * (65535/255);
        mw->line_color.green = 235 * (65535/255);
        mw->line_color.blue = 230 * (65535/255);
    }
    gdk_colormap_alloc_color(pic1_cmap, &mw->line_color, FALSE, TRUE);

    if (!gdk_color_parse("blue", &mw->fg_sunday)) {
        g_warning("color parse failed: red\n");
        mw->fg_sunday.red = 10 * (65535/255);
        mw->fg_sunday.green = 10 * (65535/255);
        mw->fg_sunday.blue = 255 * (65535/255);
    }
    gdk_colormap_alloc_color(pic1_cmap, &mw->fg_sunday, FALSE, TRUE);

    if (!gdk_color_parse("gold", &mw->bg_today)) {
        g_warning("color parse failed: gold\n");
        mw->bg_today.red = 255 * (65535/255);
        mw->bg_today.green = 215 * (65535/255);
        mw->bg_today.blue = 115 * (65535/255);
    }
    gdk_colormap_alloc_color(pic1_cmap, &mw->bg_today, FALSE, TRUE);
}

static void fill_hour(month_win *mw, gint col, gint row, char *text)
{
    GtkWidget *name, *ev;

    ev = gtk_event_box_new();
    name = gtk_label_new(text);
    gtk_container_add(GTK_CONTAINER(ev), name);
    gtk_widget_set_size_request(ev, mw->hour_req.width
            , mw->StartDate_button_req.height);
    if (text)
        gtk_table_attach(GTK_TABLE(mw->dtable), ev, col, col+1, row, row+1
             , (GTK_FILL), (0), 0, 0);
    else  /* special, needed for header table full day events */
        gtk_table_attach(GTK_TABLE(mw->dtable_h), ev, col, col+1, row, row+1
             , (GTK_FILL), (0), 0, 0);
}

static void build_month_view_table(month_win *mw)
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
    days = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(mw->day_spin));
    /****** header of day table = days columns ******/
    mw->scroll_win_h = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(mw->scroll_win_h)
            , GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
    gtk_box_pack_start(GTK_BOX(mw->Vbox), mw->scroll_win_h
            , TRUE, TRUE, 0);
    mw->month_view_vbox = gtk_vbox_new(FALSE, 0);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(mw->scroll_win_h)
            , mw->month_view_vbox);
    /*
    gtk_container_add(GTK_CONTAINER(mw->scroll_win_h), mw->month_view_vbox);
    */
    /* row 1= day header buttons 
     * row 2= full day events after the buttons */
    mw->dtable_h = gtk_table_new(2 , days+2, FALSE);
    gtk_box_pack_start(GTK_BOX(mw->month_view_vbox), mw->dtable_h
            , FALSE, FALSE, 0);

    tm_date = mw->startdate;

    year = tm_date.tm_year + 1900;
    month = tm_date.tm_mon;
    day = tm_date.tm_mday;
    if (((tm_date.tm_year%4) == 0) && (((tm_date.tm_year%100) != 0) 
            || ((tm_date.tm_year%400) == 0)))
        ++monthdays[1];


    i=0;
    mw->Previous_toolbutton = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(mw->Previous_toolbutton), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(mw->Previous_toolbutton), 0);
    arrow = gtk_arrow_new(GTK_ARROW_LEFT, GTK_SHADOW_NONE);
    gtk_container_add(GTK_CONTAINER(mw->Previous_toolbutton), arrow);
    gtk_table_attach(GTK_TABLE(mw->dtable_h), mw->Previous_toolbutton, i, i+1, 0, 1
                , (GTK_FILL), (0), 0, 0);
    gtk_widget_show_all(mw->Previous_toolbutton);
    g_signal_connect((gpointer)mw->Previous_toolbutton, "button_release_event"
            , G_CALLBACK(on_Previous_clicked), mw);
    gtk_tooltips_set_tip(mw->Tooltips, mw->Previous_toolbutton, _("Previous month"), NULL);
    for (i = 1; i < days+1; i++) {
        button = gtk_label_new(_(dayname[i-1]));

        if (i == 8) {
            gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &mw->fg_sunday);
        }
        gtk_widget_set_size_request(button, mw->StartDate_button_req.width, -1);
        g_object_set_data(G_OBJECT(button), "offset", GINT_TO_POINTER(i-1));
        gtk_table_attach(GTK_TABLE(mw->dtable_h), button, i, i+1, 0, 1
                , (GTK_FILL), (0), 0, 0);
    }

    mw->Next_toolbutton = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(mw->Next_toolbutton), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(mw->Next_toolbutton), 0);
    arrow = gtk_arrow_new(GTK_ARROW_RIGHT, GTK_SHADOW_NONE);
    gtk_container_add(GTK_CONTAINER(mw->Next_toolbutton), arrow);
    gtk_table_attach(GTK_TABLE(mw->dtable_h), mw->Next_toolbutton, i, i+1, 0, 1
                , (GTK_FILL), (0), 0, 0);
    gtk_widget_show_all(mw->Next_toolbutton);
    g_signal_connect((gpointer)mw->Next_toolbutton, "button_release_event"
            , G_CALLBACK(on_Next_clicked), mw);
    gtk_tooltips_set_tip(mw->Tooltips, mw->Next_toolbutton, _("Next month"), NULL);

    /****** body of day table ******/
    mw->scroll_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(mw->scroll_win)
            , GTK_SHADOW_NONE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(mw->scroll_win)
            , GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_placement(GTK_SCROLLED_WINDOW(mw->scroll_win)
            , GTK_CORNER_TOP_LEFT);
    gtk_box_pack_start(GTK_BOX(mw->month_view_vbox), mw->scroll_win
            , TRUE, TRUE, 0);
    vp = gtk_viewport_new(NULL, NULL);
    gtk_viewport_set_shadow_type(GTK_VIEWPORT(vp), GTK_SHADOW_NONE);
    gtk_container_add(GTK_CONTAINER(mw->scroll_win), vp);
    mw->dtable = gtk_table_new(6, days+2, FALSE);
    gtk_container_add(GTK_CONTAINER(vp), mw->dtable);

    gtk_widget_show_all(mw->dtable_h);

    /* hours column = hour rows */
    for (i = 0; i < 6; i++) {
        /* ev is needed for background colour */
        fill_hour(mw, 0, i, "");
        fill_hour(mw, days+1, i, "");
    }
    fill_days(mw, days, mw->item);
}

void refresh_month_win(month_win *mw)
{
    get_scroll_position(mw);
    gtk_widget_destroy(mw->scroll_win_h);
    build_month_view_table(mw);
    gtk_widget_show_all(mw->scroll_win_h);
    /* I was not able to get this work without the timer. Ugly yes, but
     * it works and does not hurt - much */
    g_timeout_add(100, (GtkFunction)scroll_position_timer, (gpointer)mw);
}

month_win *create_month_win(FolderItem *item, struct tm tmdate)
{
    month_win *mw;
    char *start_date = get_locale_date(&tmdate);
    SummaryView *summaryview = NULL;
    
    /* initialisation + main window + base vbox */
    mw = g_new0(month_win, 1);
    mw->scroll_pos = -1; /* not set */
    mw->Tooltips = gtk_tooltips_new();
    mw->accel_group = gtk_accel_group_new();

    while (tmdate.tm_mday != 1)
    	orage_move_day(&tmdate, -1);

    mw->startdate = tmdate;

    mw->Vbox = gtk_vbox_new(FALSE, 0);

    mw->item = item;
    build_month_view_colours(mw);
    build_month_view_header(mw, start_date);
    build_month_view_table(mw);
    gtk_widget_show_all(mw->Vbox);
    if (mainwindow_get_mainwindow()) {
	summaryview = mainwindow_get_mainwindow()->summaryview;
	gtk_container_add(GTK_CONTAINER(summaryview->mainwidget_book),
		mw->Vbox);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(summaryview->mainwidget_book),
		gtk_notebook_page_num(GTK_NOTEBOOK(summaryview->mainwidget_book), 
		mw->Vbox));
	main_window_set_menu_sensitive(mainwindow_get_mainwindow());
	toolbar_main_set_sensitive(mainwindow_get_mainwindow());
    }

    g_timeout_add(100, (GtkFunction)scroll_position_timer, (gpointer)mw);

    return(mw);
}

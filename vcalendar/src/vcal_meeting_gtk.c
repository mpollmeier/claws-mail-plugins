/*
 * Claws Mail -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2007 Colin Leroy <colin@colino.net> and 
 * the Claws Mail team
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
#  include "config.h"
#endif

#include "defs.h"

#include <ical.h>
#include "gettext.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <curl/curl.h>
#include <curl/curlver.h>

#include "utils.h"
#include "vcalendar.h"
#include "vcal_folder.h"
#include "vcal_manager.h"
#include "vcal_meeting_gtk.h"
#include "vcal_prefs.h"
#include "mainwindow.h"
#include "prefs_account.h"
#include "account.h"
#include "filesel.h"
#include "alertpanel.h"
#include "addr_compl.h"
#include "gtkutils.h"
#include "log.h"

struct _VCalMeeting
{
	gchar     *uid;
	gint       sequence;
	gint 	   method;
	GtkWidget *window;
#ifndef MAEMO
	GtkWidget *table;
#else
	GtkWidget *table1;
	GtkWidget *table2;
#endif
	GtkWidget *type;
	GtkWidget *who;
	GtkWidget *avail_evtbox;
	GtkWidget *avail_img;
	GtkWidget *start_c;
	GtkWidget *start_time;
	GtkWidget *end_c;
	GtkWidget *end_time;
	GtkWidget *summary;
	GtkWidget *description;
	GSList 	  *attendees;
	GtkWidget *attendees_vbox;
	GtkWidget *save_btn;
	GtkWidget *avail_btn;
	GSList 	  *avail_accounts;
	GtkWidget *total_avail_evtbox;
	GtkWidget *total_avail_img;
	GtkWidget *total_avail_msg;
	PrefsAccount *account;
	gboolean visible;
	GtkTooltips *tips;
};

struct _VCalAttendee {
	GtkWidget *address;
	GtkWidget *remove_btn;
	GtkWidget *add_btn;
	GtkWidget *cutype;
	GtkWidget *hbox;
	VCalMeeting *meet;
	gchar *status;
	GtkWidget *avail_evtbox;
	GtkWidget *avail_img;
	gchar *cached_contents;
	gboolean org;
};

static GdkCursor *watch_cursor = NULL;

VCalAttendee *attendee_add(VCalMeeting *meet, gchar *address, gchar *name, gchar *partstat, gchar *cutype, gboolean first);

#ifndef MAEMO
#define TABLE_ADD_LINE(label_text, widget, do_space) {				\
	gchar *tmpstr = g_strdup_printf("<span weight=\"bold\">%s</span>",	\
				label_text?label_text:"");			\
	GtkWidget *label = NULL;				 		\
	GtkWidget *spacer = NULL;						\
	GtkWidget *s_hbox = NULL;						\
	if (do_space) {								\
		spacer = gtk_label_new("");					\
		gtk_widget_set_usize(spacer, 18, 16);				\
		s_hbox = gtk_hbox_new(FALSE, 6);				\
		gtk_box_pack_start(GTK_BOX(s_hbox), spacer, FALSE, FALSE, 0);	\
		gtk_box_pack_start(GTK_BOX(s_hbox), widget, TRUE, TRUE, 0);	\
	}									\
	if (label_text) {							\
		label = gtk_label_new(tmpstr);					\
		g_free(tmpstr);							\
		gtk_label_set_use_markup (GTK_LABEL (label), TRUE);		\
		gtk_misc_set_alignment (GTK_MISC(label), 1, 0.5);		\
		gtk_table_attach (GTK_TABLE (meet->table), 			\
				  label, 0, 1, i, i+1,				\
				  GTK_FILL, GTK_FILL, 6, 6);			\
		gtk_table_attach (GTK_TABLE (meet->table), 			\
				  do_space?s_hbox:widget, 1, 2, i, i+1,		\
				  GTK_FILL|GTK_EXPAND, GTK_FILL, 6, 6);		\
		if (GTK_IS_LABEL(widget)) {					\
			gtk_label_set_use_markup(GTK_LABEL (widget), TRUE);	\
			gtk_misc_set_alignment (GTK_MISC(widget),0, 0);		\
			gtk_label_set_line_wrap(GTK_LABEL(widget), TRUE);	\
		}								\
	} else {								\
		g_free(tmpstr);							\
		gtk_table_attach (GTK_TABLE (meet->table), 			\
				  do_space?s_hbox:widget, 0, 2, i, i+1,		\
				  GTK_FILL|GTK_EXPAND, GTK_FILL, 6, 6);		\
	}									\
	i++;									\
}
#else
#define TABLE_ADD_LINE(label_text, widget, do_space, intable1) {			\
	gchar *tmpstr = g_strdup_printf("<span weight=\"bold\">%s</span>",	\
				label_text?label_text:"");			\
	GtkWidget *label = NULL;				 		\
	GtkWidget *spacer = NULL;						\
	GtkWidget *s_hbox = NULL;						\
	if (do_space) {								\
		spacer = gtk_label_new("");					\
		gtk_widget_set_usize(spacer, 18, 16);				\
		s_hbox = gtk_hbox_new(FALSE, 6);				\
		gtk_box_pack_start(GTK_BOX(s_hbox), spacer, FALSE, FALSE, 0);	\
		gtk_box_pack_start(GTK_BOX(s_hbox), widget, TRUE, TRUE, 0);	\
	}									\
	if (label_text) {							\
		label = gtk_label_new(tmpstr);					\
		g_free(tmpstr);							\
		gtk_label_set_use_markup (GTK_LABEL (label), TRUE);		\
		gtk_misc_set_alignment (GTK_MISC(label), 1, 0.5);		\
		if(intable1)	{						\
			gtk_table_attach (GTK_TABLE (meet->table1), 		\
					  label, 0, 1, i, i+1,			\
					  GTK_FILL, GTK_FILL, 1, 1);		\
		}								\
		else	{							\
			gtk_table_attach (GTK_TABLE (meet->table2), 		\
					  label, 0, 1, i, i+1,			\
					  GTK_FILL, GTK_FILL, 1, 1);		\
		}								\
		if(intable1)	{						\
			gtk_table_attach (GTK_TABLE (meet->table1), 		\
					  do_space?s_hbox:widget, 1, 2, i, i+1,	\
					  GTK_FILL|GTK_EXPAND, GTK_FILL, 1, 1);	\
		}								\
		else	{							\
			gtk_table_attach (GTK_TABLE (meet->table2), 		\
					  do_space?s_hbox:widget, 1, 2, i, i+1,	\
					  GTK_FILL|GTK_EXPAND, GTK_FILL, 1, 1);	\
		}								\
		if (GTK_IS_LABEL(widget)) {					\
			gtk_label_set_use_markup(GTK_LABEL (widget), TRUE);	\
			gtk_misc_set_alignment (GTK_MISC(widget),0, 0);		\
			gtk_label_set_line_wrap(GTK_LABEL(widget), TRUE);	\
		}								\
	} else {								\
		g_free(tmpstr);							\
		if(intable1)	{						\
			gtk_table_attach (GTK_TABLE (meet->table1), 		\
					  do_space?s_hbox:widget, 0, 2, i, i+1,	\
					  GTK_FILL|GTK_EXPAND, GTK_FILL, 1, 1);	\
		}								\
		else	{							\
			gtk_table_attach (GTK_TABLE (meet->table2), 		\
					  do_space?s_hbox:widget, 0, 2, i, i+1,	\
					  GTK_FILL|GTK_EXPAND, GTK_FILL, 1, 1);	\
		}								\
	}									\
	i++;									\
}
#endif
enum {
	DAY,
	MONTH,
	YEAR,
	HOUR,
	MINUTE
};

static gboolean avail_btn_can_be_sensitive(void)
{
	if (vcalprefs.freebusy_get_url == NULL
	||  *vcalprefs.freebusy_get_url == '\0')
		return FALSE;
	else 
		return TRUE;
}

static gchar *get_month_name(gint i)
{
	struct tm *lt;
	time_t t;
	gchar day[4], mon[4];
	gint dd, hh, mm, ss, yyyy;
	struct tm buft;
	
	tzset();
	
	t = time(NULL);
	lt = localtime_r(&t, &buft);
	lt->tm_mon = i;
	
	sscanf(asctime(lt), "%3s %3s %d %d:%d:%d %d\n",
	       day, mon, &dd, &hh, &mm, &ss, &yyyy);
	
	return g_strdup(mon);
}

static gint get_dtdate(const gchar *str, gint field)
{
	time_t t = icaltime_as_timet((icaltime_from_string(str)));
	struct tm buft;
	struct tm *lt;
	
	tzset();

	lt = localtime_r(&t, &buft);

	switch(field){
	case DAY:
		return lt->tm_mday;
	case MONTH:
		return lt->tm_mon + 1;
	case YEAR:
		return lt->tm_year + 1900;
	case HOUR:
		return lt->tm_hour;
	case MINUTE:
		return lt->tm_min;
	}
	return -1;

}

static gboolean add_btn_cb(GtkButton *widget, gpointer data)
{
	VCalAttendee *attendee = (VCalAttendee *)data;
	attendee_add(attendee->meet, NULL, NULL, NULL, NULL, FALSE);
	return TRUE;
}

static gboolean remove_btn_cb(GtkButton *widget, gpointer data)
{
	VCalAttendee *attendee = (VCalAttendee *)data;
	gtk_container_remove(GTK_CONTAINER(attendee->meet->attendees_vbox), attendee->hbox);
	attendee->meet->attendees = g_slist_remove(attendee->meet->attendees, attendee);
	
	g_free(attendee->status);

	return TRUE;
}

VCalAttendee *attendee_add(VCalMeeting *meet, gchar *address, gchar *name, gchar *partstat, gchar *cutype, gboolean first)
{
	GtkWidget *att_hbox = gtk_hbox_new(FALSE, 6);
	VCalAttendee *attendee 	= g_new0(VCalAttendee, 1);
	attendee->address	= gtk_entry_new();
	attendee->cutype	= gtk_combo_box_new_text();
	attendee->avail_evtbox  = gtk_event_box_new();
	attendee->avail_img	= gtk_image_new_from_stock
                        (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_SMALL_TOOLBAR);

	gtk_widget_show(attendee->address);
	gtk_widget_show(attendee->cutype);
	gtk_widget_show(attendee->avail_evtbox);

	gtk_tooltips_set_tip(meet->tips, attendee->address, 
				_("Use <tab> to autocomplete from addressbook"), NULL);
	gtk_widget_set_usize(attendee->avail_evtbox, 18, 16);
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(attendee->avail_evtbox), FALSE);
	gtk_container_add (GTK_CONTAINER(attendee->avail_evtbox), attendee->avail_img);

	if (address) {
		gchar *str = g_strdup_printf("%s%s%s%s",
				(name && strlen(name))?name:"",
				(name && strlen(name))?" <":"",
				address,
				(name && strlen(name))?">":"");
		gtk_entry_set_text(GTK_ENTRY(attendee->address), str);
		g_free(str);
	}

	if (partstat)
		attendee->status = g_strdup(partstat);

	gtk_combo_box_append_text(GTK_COMBO_BOX(attendee->cutype), _("Individual"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(attendee->cutype), _("Group"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(attendee->cutype), _("Resource"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(attendee->cutype), _("Room"));
	
	gtk_combo_box_set_active(GTK_COMBO_BOX(attendee->cutype), 0);
	
	if (cutype) {
		if (!strcmp(cutype, "group"))
			gtk_combo_box_set_active(GTK_COMBO_BOX(attendee->cutype), 1);
		if (!strcmp(cutype, "resource"))
			gtk_combo_box_set_active(GTK_COMBO_BOX(attendee->cutype), 2);
		if (!strcmp(cutype, "room"))
			gtk_combo_box_set_active(GTK_COMBO_BOX(attendee->cutype), 3);
	}
	
	attendee->add_btn	= gtk_button_new_with_label(_("Add..."));
	attendee->remove_btn	= gtk_button_new_with_label(_("Remove"));
	attendee->meet		= meet;
	attendee->hbox		= att_hbox;

	gtk_widget_show(attendee->add_btn);
	gtk_widget_show(attendee->remove_btn);
	gtk_widget_show(attendee->hbox);

	gtk_box_pack_start(GTK_BOX(attendee->hbox), attendee->avail_evtbox, FALSE, FALSE, 0);
	gtk_widget_set_sensitive(attendee->remove_btn, !first);
	meet->attendees 	= g_slist_append(meet->attendees, attendee);
	
	g_signal_connect(G_OBJECT(attendee->remove_btn), "clicked",
			 G_CALLBACK(remove_btn_cb), attendee);
	g_signal_connect(G_OBJECT(attendee->add_btn), "clicked",
			 G_CALLBACK(add_btn_cb), attendee);
	
	gtk_box_pack_start(GTK_BOX(att_hbox), attendee->address, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(att_hbox), attendee->cutype, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(att_hbox), attendee->add_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(att_hbox), attendee->remove_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(meet->attendees_vbox), att_hbox, FALSE, FALSE, 0);
	address_completion_register_entry(GTK_ENTRY(attendee->address), FALSE);
#ifndef MAEMO
	gtk_widget_set_size_request(attendee->address, 320, -1);
#else
	gtk_widget_set_size_request(attendee->address, 220, -1);
#endif
	return attendee;
}

static gchar *get_organizer(VCalMeeting *meet)
{
	int index = gtk_combo_box_get_active(GTK_COMBO_BOX(meet->who));
	int i = 0;
	GSList *cur = meet->avail_accounts;
	while (i < index && cur && cur->data) {
		debug_print("%d:skipping %s\n",i,((PrefsAccount *)(cur->data))->address);
		cur = cur->next;
		i++;
	}
	if (cur)
		return g_strdup(((PrefsAccount *)(cur->data))->address);
	else
		return g_strdup("");
}

static gchar *get_organizer_name(VCalMeeting *meet)
{
	int index = gtk_combo_box_get_active(GTK_COMBO_BOX(meet->who));
	int i = 0;
	GSList *cur = meet->avail_accounts;
	while (i < index && cur && cur->data) {
		debug_print("%d:skipping %s\n",i,((PrefsAccount *)(cur->data))->address);
		cur = cur->next;
		i++;
	}
	if (cur)
		return g_strdup(((PrefsAccount *)(cur->data))->name);
	else
		return g_strdup("");
}

static void get_time_from_combo(GtkCombo *combo, int *h, int *m)
{
	gchar *tmp;
	gchar **parts;
	
	if (!h || !m) 
		return;

	tmp = gtk_editable_get_chars(GTK_EDITABLE(combo->entry), 0, -1);
	parts = g_strsplit(tmp, ":", 2);
	if (parts[0] && parts[1] && *parts[0] && *parts[1]) {
		*h = atoi(parts[0]);
		*m = atoi(parts[1]);
	}
	g_strfreev(parts);
	g_free(tmp);
}

static int get_current_gmt_offset(void)
{
	time_t now = time(NULL);
	struct tm gmt;
	struct tm local;
	
	tzset();

	gmtime_r(& now, & gmt);
	localtime_r(& now, & local);
	
	local.tm_isdst = 0;
	return mktime(&local)-mktime(&gmt);
}

static int get_gmt_offset_at_time(time_t then)
{
	struct tm gmt;
	struct tm local;
	
	tzset();

	gmtime_r(& then, & gmt);
	localtime_r(& then, & local);
	
	local.tm_isdst = 0;
	return mktime(&local)-mktime(&gmt);
}

static gchar *get_date(VCalMeeting *meet, int start) 
{
	struct tm *lt;
	time_t t;
	guint d, m, y;
	int dst_offset = 0;
	struct tm buft;

	tzset();

	t = time(NULL);
	lt = localtime_r(&t, &buft);
	
	gtk_calendar_get_date(GTK_CALENDAR(start ? meet->start_c : meet->end_c), &y, &m, &d);
	lt->tm_mday = d;
	lt->tm_mon  = m;
	lt->tm_year = y - 1900;
	lt->tm_hour = 0;
	lt->tm_min  = 0;
	lt->tm_sec  = 0;

	if (start) {
		get_time_from_combo(GTK_COMBO(meet->start_time), &lt->tm_hour, &lt->tm_min);
	} else {
		get_time_from_combo(GTK_COMBO(meet->end_time), &lt->tm_hour, &lt->tm_min);
	}
	
	debug_print("%d %d %d, %d:%d\n", lt->tm_mday, lt->tm_mon, lt->tm_year, lt->tm_hour, lt->tm_min);
	t = mktime(lt);

	dst_offset = get_current_gmt_offset() - get_gmt_offset_at_time(t);
	debug_print("DST change offset to apply to time %d\n", dst_offset);
	t += dst_offset;
	debug_print("%s\n", ctime(&t));
	return g_strdup(icaltime_as_ical_string(icaltime_from_timet(t, FALSE)));
}

static gchar *get_summary(VCalMeeting *meet) 
{
	return gtk_editable_get_chars(GTK_EDITABLE(meet->summary),0, -1);
}

static gchar *get_description(VCalMeeting *meet) 
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(meet->description));
	GtkTextIter start, end;
	
	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);
	return gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
}

void vcal_meeting_free(VCalMeeting *meet)
{
	debug_print("freeing meeting\n");
	g_free(meet->uid);
	address_completion_end(meet->window);
	g_slist_free(meet->avail_accounts);
	g_slist_free(meet->attendees);
	g_free(meet);
}

static void destroy_meeting_cb(GtkWidget *widget, gpointer data)
{
	VCalMeeting *meet = (VCalMeeting *)data;
	vcal_meeting_free(meet);
}

static void vcal_destroy(VCalMeeting *meet)
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(meet->description));
	gtk_text_buffer_remove_selection_clipboard(buffer, gtk_clipboard_get(GDK_SELECTION_PRIMARY));
	gtk_widget_destroy(meet->window);
}

static gboolean meeting_key_pressed(GtkWidget *widget,
				    GdkEventKey *event,
				    gpointer data)
{
	VCalMeeting *meet = (VCalMeeting *)data;
	
	if (event && event->keyval == GDK_Escape) {
		vcal_destroy(meet);
	}
	return FALSE;
}

static int get_list_item_num(int h, int m)
{
	if (m % 15 != 0)
		return -1;

	return (h*4 + m/15);
}

static void meeting_end_changed(GtkWidget *widget, gpointer data);

static void meeting_start_changed(GtkWidget *widget, gpointer data)
{
	VCalMeeting *meet = (VCalMeeting *)data;
	struct tm start_lt;
	struct tm end_lt;
	time_t start_t, end_t;
	guint d, m, y;
	int num = -1;

	if (strlen(gtk_entry_get_text(
		   GTK_ENTRY(GTK_COMBO(meet->start_time)->entry))) < 5)
		return;

	tzset();

	start_t = time(NULL);
	end_t = time(NULL);
	localtime_r(&start_t, &start_lt);
	localtime_r(&end_t, &end_lt);

	gtk_calendar_get_date(GTK_CALENDAR(meet->start_c), &y, &m, &d);
	start_lt.tm_mday = d; start_lt.tm_mon  = m; start_lt.tm_year = y - 1900;
	get_time_from_combo(GTK_COMBO(meet->start_time), &start_lt.tm_hour, &start_lt.tm_min);

	start_t = mktime(&start_lt);
	debug_print("start %s\n", ctime(&start_t));

	gtk_calendar_get_date(GTK_CALENDAR(meet->end_c), &y, &m, &d);
	end_lt.tm_mday = d; end_lt.tm_mon  = m; end_lt.tm_year = y - 1900;
	get_time_from_combo(GTK_COMBO(meet->end_time), &end_lt.tm_hour, &end_lt.tm_min);

	end_t = mktime(&end_lt);

	debug_print("end   %s\n", ctime(&end_t));
	
	if (end_t > start_t) {
		debug_print("ok\n");
		return;
	}
	end_t = start_t + 3600;

	localtime_r(&end_t, &end_lt);
	debug_print("n %d %d %d, %d:%d\n", end_lt.tm_mday, end_lt.tm_mon, end_lt.tm_year, end_lt.tm_hour, end_lt.tm_min);

	g_signal_handlers_block_by_func(GTK_COMBO(meet->end_time)->entry, meeting_end_changed, meet);
	g_signal_handlers_block_by_func(meet->end_c, meeting_end_changed, meet);

	gtk_calendar_select_day(GTK_CALENDAR(meet->end_c), end_lt.tm_mday);

	gtk_calendar_select_month(GTK_CALENDAR(meet->end_c),
				end_lt.tm_mon,
				end_lt.tm_year + 1900);

	num = get_list_item_num(end_lt.tm_hour, end_lt.tm_min);
	if (num > -1)
		gtk_list_select_item(GTK_LIST(GTK_COMBO(meet->end_time)->list), num);
	else {
		gchar *tmp = g_strdup_printf("%02d:%02d",
				end_lt.tm_hour, 
				end_lt.tm_min);
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(meet->end_time)->entry),
				   tmp);
		g_free(tmp);
	}
	g_signal_handlers_unblock_by_func(GTK_COMBO(meet->end_time)->entry, meeting_end_changed, meet);
	g_signal_handlers_unblock_by_func(meet->end_c, meeting_end_changed, meet);
}

static void meeting_end_changed(GtkWidget *widget, gpointer data)
{
	VCalMeeting *meet = (VCalMeeting *)data;
	struct tm start_lt;
	struct tm end_lt;
	time_t start_t, end_t;
	guint d, m, y;
	int num = -1;

	if (strlen(gtk_entry_get_text(
		   GTK_ENTRY(GTK_COMBO(meet->end_time)->entry))) < 5)
		return;
	start_t = time(NULL);
	end_t = time(NULL);

	tzset();

	localtime_r(&start_t, &start_lt);
	localtime_r(&end_t, &end_lt);
	
	gtk_calendar_get_date(GTK_CALENDAR(meet->start_c), &y, &m, &d);
	start_lt.tm_mday = d; start_lt.tm_mon  = m; start_lt.tm_year = y - 1900;
	get_time_from_combo(GTK_COMBO(meet->start_time), &start_lt.tm_hour, &start_lt.tm_min);

	start_t = mktime(&start_lt);
	debug_print("start %s\n", ctime(&start_t));

	gtk_calendar_get_date(GTK_CALENDAR(meet->end_c), &y, &m, &d);
	end_lt.tm_mday = d; end_lt.tm_mon  = m; end_lt.tm_year = y - 1900;
	get_time_from_combo(GTK_COMBO(meet->end_time), &end_lt.tm_hour, &end_lt.tm_min);

	end_t = mktime(&end_lt);

	debug_print("end   %s\n", ctime(&end_t));
	
	if (end_t > start_t) {
		debug_print("ok\n");
		return;
	}
	start_t = end_t - 3600;
	
	tzset();

	localtime_r(&start_t, &start_lt);
	debug_print("n %d %d %d, %d:%d\n", start_lt.tm_mday, start_lt.tm_mon, start_lt.tm_year, start_lt.tm_hour, start_lt.tm_min);

	g_signal_handlers_block_by_func(GTK_COMBO(meet->start_time)->entry, meeting_start_changed, meet);
	g_signal_handlers_block_by_func(meet->start_c, meeting_start_changed, meet);

	gtk_calendar_select_day(GTK_CALENDAR(meet->start_c), start_lt.tm_mday);

	gtk_calendar_select_month(GTK_CALENDAR(meet->start_c),
				start_lt.tm_mon,
				start_lt.tm_year + 1900);

	num = get_list_item_num(start_lt.tm_hour, start_lt.tm_min);
	if (num > -1)
		gtk_list_select_item(GTK_LIST(GTK_COMBO(meet->start_time)->list), num);
	else {
		gchar *tmp = g_strdup_printf("%02d:%02d",
				start_lt.tm_hour, 
				start_lt.tm_min);
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(meet->start_time)->entry),
				   tmp);
		g_free(tmp);
	}

	g_signal_handlers_unblock_by_func(GTK_COMBO(meet->start_time)->entry, meeting_start_changed, meet);
	g_signal_handlers_unblock_by_func(meet->start_c, meeting_start_changed, meet);
}

static void att_update_icon(VCalMeeting *meet, VCalAttendee *attendee, gint avail, gchar *text)
{
	const gchar *icon = GTK_STOCK_DIALOG_INFO;

	switch (avail) {
		case 0:  icon = GTK_STOCK_DIALOG_WARNING;	break;
		case 1:  icon = GTK_STOCK_DIALOG_INFO;		break;
		default: icon = GTK_STOCK_DIALOG_QUESTION;	break;
	}
	if (!gtk_entry_get_text(GTK_ENTRY(attendee->address)) 
	 || strlen(gtk_entry_get_text(GTK_ENTRY(attendee->address)))==0) {
		if (attendee->avail_img) {
			gtk_widget_hide(attendee->avail_img);
		}
		gtk_tooltips_set_tip(meet->tips, attendee->avail_evtbox, NULL, NULL);
	} else if (attendee->avail_img) {
		gtk_image_set_from_stock
		        (GTK_IMAGE(attendee->avail_img), 
			icon, 
			GTK_ICON_SIZE_SMALL_TOOLBAR);
		gtk_widget_show(attendee->avail_img);
		gtk_tooltips_set_tip(meet->tips, attendee->avail_evtbox, text, NULL);
	}
}

gboolean attendee_available(VCalAttendee *attendee, const gchar *dtstart, const gchar *dtend, const gchar *contents)
{
	icalcomponent *toplvl, *vfreebusy;
	icalproperty *busyprop;
	struct icaltimetype start = icaltime_from_string(dtstart);
	struct icaltimetype end = icaltime_from_string(dtend);
	gboolean result = TRUE;
	

	if (contents == NULL)
		return TRUE;

	toplvl = icalcomponent_new_from_string((gchar *)contents);
	
	if (toplvl == NULL)
		return TRUE;

	vfreebusy = icalcomponent_get_first_component(toplvl, ICAL_VFREEBUSY_COMPONENT);
	while (vfreebusy && icalcomponent_isa(vfreebusy) != ICAL_VFREEBUSY_COMPONENT)
		vfreebusy = icalcomponent_get_next_component(toplvl, ICAL_VFREEBUSY_COMPONENT);
	
	if (vfreebusy) {
		busyprop = icalcomponent_get_first_property(vfreebusy, ICAL_FREEBUSY_PROPERTY);
		while (busyprop) {
			struct icalperiodtype ipt = icalproperty_get_freebusy(busyprop);
			
			if ( icaltime_compare(start, ipt.end) >= 0 || icaltime_compare(end, ipt.start) <= 0 ) {
				result = TRUE;
			} else {
				result = FALSE;
				break;
			}
			busyprop = icalcomponent_get_next_property(vfreebusy, ICAL_FREEBUSY_PROPERTY);
		}
	}

	icalcomponent_free(toplvl);
	return result;
}

static gchar *get_avail_msg(const gchar *unavailable_persons, gboolean multiple, 
	gboolean short_version, gint offset_before, gint offset_after)
{
	gchar *msg, *intro = NULL, *outro = NULL, *before = NULL, *after = NULL;

	if (multiple)
		intro = g_strdup(_("The following person(s) are busy at the time of your planned meeting:\n- "));
	else if (!strcmp(unavailable_persons, _("You")))
		intro = g_strdup(_("You are busy at the time of your planned meeting"));
	else
		intro = g_strdup_printf(_("%s is busy at the time of your planned meeting"), unavailable_persons);
	if (offset_before == 3600)
		before = g_strdup_printf(_("%d hour sooner"), offset_before/3600);
	else if (offset_before > 3600 && offset_before%3600 == 0)
		before = g_strdup_printf(_("%d hours sooner"), offset_before/3600);
	else if (offset_before > 3600)
		before = g_strdup_printf(_("%d hours and %d minutes sooner"), offset_before/3600, (offset_before%3600)/60);
	else if (offset_before == 1800)
		before = g_strdup_printf(_("%d minutes sooner"), offset_before/60);
	else
		before = NULL;
	
	if (offset_after == 3600)
		after = g_strdup_printf(_("%d hour later"), offset_after/3600);
	else if (offset_after > 3600 && offset_after%3600 == 0)
		after = g_strdup_printf(_("%d hours later"), offset_after/3600);
	else if (offset_after > 3600)
		after = g_strdup_printf(_("%d hours and %d minutes later"), offset_after/3600, (offset_after%3600)/60);
	else if (offset_after == 1800)
		after = g_strdup_printf(_("%d minutes later"), offset_after/60);
	else
		after = NULL;
	
	if (multiple) {
		if (before && after)
			outro = g_strdup_printf(_("\n\nEveryone would be available %s or %s."), before, after);
		else if (before || after)
			outro = g_strdup_printf(_("\n\nEveryone would be available %s."), before?before:after);
		else
			outro = g_strdup_printf(_("\n\nIt isn't possible to have this meeting with everyone "
						"in the previous or next 6 hours."));
	} else {
		if (short_version) {
			if (before && after)
				outro = g_markup_printf_escaped(_("would be available %s or %s"), before, after);
			else if (before || after)
				outro = g_markup_printf_escaped(_("would be available %s"), before?before:after);
			else
				outro = g_strdup_printf(_("not available"));
		} else {
			if (before && after)
				outro = g_markup_printf_escaped(_(", but would be available %s or %s."), before, after);
			else if (before || after)
				outro = g_markup_printf_escaped(_(", but would be available %s."), before?before:after);
			else
				outro = g_strdup_printf(_(", and isn't available "
							"in the previous or next 6 hours."));
		}
	}
	if (multiple && short_version)
		msg = g_strconcat(outro+2, NULL);
	else if (multiple)
		msg = g_strconcat(intro, unavailable_persons, outro, NULL);
	else if (short_version)
		msg = g_strdup(outro);
	else
		msg = g_strconcat(intro, outro, NULL);
	g_free(intro);
	g_free(outro);
	g_free(before);
	g_free(after);
	return msg;
}

static gboolean find_availability(const gchar *dtstart, const gchar *dtend, GSList *attendees, gboolean for_send, VCalMeeting *meet)
{
	GSList *cur;
	gint offset = -1800, offset_before = 0, offset_after = 0;
	gboolean found = FALSE;
	gchar *unavailable_persons = NULL;
	gchar *msg = NULL;
	struct icaltimetype start = icaltime_from_string(dtstart);
	struct icaltimetype end = icaltime_from_string(dtend);
	AlertValue val;
	gint total = 0;
	GHashTable *avail_table_avail = g_hash_table_new(NULL, g_direct_equal);
	GHashTable *avail_table_before = g_hash_table_new(NULL, g_direct_equal);
	GHashTable *avail_table_after = g_hash_table_new(NULL, g_direct_equal);
	
	for (cur = attendees; cur; cur = cur->next) {
		VCalAttendee *attendee = (VCalAttendee *)cur->data;
		if (!attendee_available(attendee, icaltime_as_ical_string(start), icaltime_as_ical_string(end),
				attendee->cached_contents)) {
			gchar *mail = NULL;
			
			if (attendee->org)
				mail = g_strdup(_("You"));
			else
				mail = gtk_editable_get_chars(GTK_EDITABLE(attendee->address), 0, -1);

			if (unavailable_persons == NULL) {
				unavailable_persons = g_markup_printf_escaped("%s", mail);
			} else {
				gchar *tmp = g_markup_printf_escaped("%s,\n- %s", unavailable_persons, mail);
				g_free(unavailable_persons);
				unavailable_persons = tmp;
			}
			total++;
			g_free(mail);
			att_update_icon(meet, attendee, 0, _("not available"));
		} else {
			if (attendee->cached_contents != NULL)
				att_update_icon(meet, attendee, 1, _("available"));
			else
				att_update_icon(meet, attendee, 2, _("Free/busy retrieval failed"));

			g_hash_table_insert(avail_table_avail, attendee, GINT_TO_POINTER(1));
		}
	}
	offset = -1800;
	found = FALSE;
	while (!found && offset >= -3600*6) {
		gboolean ok = TRUE;
		struct icaltimetype new_start = icaltime_from_timet(icaltime_as_timet(start)+offset, FALSE);
		struct icaltimetype new_end   = icaltime_from_timet(icaltime_as_timet(end)+offset, FALSE);
		for (cur = attendees; cur; cur = cur->next) {
			VCalAttendee *attendee = (VCalAttendee *)cur->data;
			debug_print("trying %s - %s (offset %d)\n", 
				icaltime_as_ical_string(new_start), icaltime_as_ical_string(new_end), offset);
			if (!attendee_available(attendee, icaltime_as_ical_string(new_start), icaltime_as_ical_string(new_end),
					attendee->cached_contents)) {
				ok = FALSE;
				break;
			} else {
				if (!g_hash_table_lookup(avail_table_before, attendee)
				&&  !g_hash_table_lookup(avail_table_avail, attendee))
					g_hash_table_insert(avail_table_before, attendee, GINT_TO_POINTER(-offset));
			}
		}
		if (ok) {
			found = TRUE;
			offset_before = -offset;
		}
		offset -= 1800;
	}
	found = FALSE;
	offset = 1800;
	while (!found && offset <= 3600*6) {
		gboolean ok = TRUE;
		struct icaltimetype new_start = icaltime_from_timet(icaltime_as_timet(start)+offset, FALSE);
		struct icaltimetype new_end   = icaltime_from_timet(icaltime_as_timet(end)+offset, FALSE);
		for (cur = attendees; cur; cur = cur->next) {
			VCalAttendee *attendee = (VCalAttendee *)cur->data;
			debug_print("trying %s - %s (offset %d)\n", 
				icaltime_as_ical_string(new_start), icaltime_as_ical_string(new_end), offset);
			if (!attendee_available(attendee, icaltime_as_ical_string(new_start), icaltime_as_ical_string(new_end),
					attendee->cached_contents)) {
				ok = FALSE;
				break;
			} else {
				if (!g_hash_table_lookup(avail_table_after, attendee)
				&&  !g_hash_table_lookup(avail_table_avail, attendee))
					g_hash_table_insert(avail_table_after, attendee, GINT_TO_POINTER(offset));
			}
		}
		if (ok) {
			found = TRUE;
			offset_after = offset;
		}

		offset += 1800;
	}
	
	for (cur = attendees; cur; cur = cur->next) {
		VCalAttendee *attendee = (VCalAttendee *)cur->data;
		gint ok = GPOINTER_TO_INT(g_hash_table_lookup(avail_table_avail, attendee));
		gint o_before = GPOINTER_TO_INT(g_hash_table_lookup(avail_table_before, attendee));
		gint o_after = GPOINTER_TO_INT(g_hash_table_lookup(avail_table_after, attendee));
		if (!o_before && !o_after && !ok) {
			att_update_icon(meet, attendee, 0, _("not available"));
		} else if ((o_before != 0 || o_after != 0) && !ok) {
			if (attendee->org)
				msg = get_avail_msg(_("You"), FALSE, TRUE, o_before, o_after);
			else
				msg = get_avail_msg(gtk_entry_get_text(GTK_ENTRY(attendee->address)), FALSE, TRUE, o_before, o_after);
			att_update_icon(meet, attendee, 0, msg);
			g_free(msg);
		}
		
	}
	g_hash_table_destroy(avail_table_before);
	g_hash_table_destroy(avail_table_after);

	if (for_send) {
		msg = get_avail_msg(unavailable_persons, (total > 1), FALSE, offset_before, offset_after);

		val = alertpanel_full(_("Not everyone is available"), msg,
				   	GTK_STOCK_CANCEL, _("Send anyway"), NULL, FALSE,
				   	NULL, ALERT_QUESTION, G_ALERTDEFAULT);
		g_free(msg);
	}
	msg = get_avail_msg(unavailable_persons, TRUE, TRUE, offset_before, offset_after);
	g_free(unavailable_persons);
	gtk_image_set_from_stock
		(GTK_IMAGE(meet->total_avail_img), 
		GTK_STOCK_DIALOG_WARNING, 
		GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_widget_show(meet->total_avail_img);
	gtk_label_set_text(GTK_LABEL(meet->total_avail_msg), _("Not everyone is available. "
				"See tooltips for more info..."));
	gtk_tooltips_set_tip(meet->tips, meet->total_avail_evtbox, msg, NULL);
	g_free(msg);
	return (val == G_ALERTALTERNATE);
}

static gboolean check_attendees_availability(VCalMeeting *meet, gboolean tell_if_ok, gboolean for_send)
{
	GSList *cur;
	gchar *tmp = NULL;
	gchar *real_url = NULL;
	gint num_format = 0;
	gint change_user = -1, change_dom = -1;
	gchar *problems = NULL;
	gchar *dtstart = NULL;
	gchar *dtend = NULL;
	gboolean find_avail = FALSE;
	gboolean res = TRUE, uncertain = FALSE;
	gchar *organizer = NULL;
	VCalAttendee *dummy_org = NULL;
	gchar *internal_ifb = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S,
				"vcalendar", G_DIR_SEPARATOR_S, 
				"internal.ifb", NULL);
	gboolean local_only = FALSE;
	GSList *attlist;

	if (vcalprefs.freebusy_get_url == NULL
	||  *vcalprefs.freebusy_get_url == '\0') {
		local_only = TRUE;
	} else {
		real_url = g_strdup(vcalprefs.freebusy_get_url);
		tmp = real_url;

		while (strchr(tmp, '%')) {
			tmp = strchr(tmp, '%')+1;
			num_format++;
		}
		if (num_format > 2) {
			g_warning("wrong format in %s!\n", real_url);
			g_free(real_url);
			return FALSE;
		}

		tmp = NULL;
		if (strstr(real_url, "%u") != NULL) {
			change_user = (gint)strstr(real_url, "%u");
			*(strstr(real_url, "%u")+1) = 's';
		} 
		if (strstr(real_url, "%d") != NULL) {
			change_dom = (gint)strstr(real_url, "%d");
			*(strstr(real_url, "%d")+1) = 's';
		} 
		debug_print("url format %s\n", real_url);
	}
	dtstart = get_date(meet, TRUE);
	dtend = get_date(meet, FALSE);

	/* hack to check our own avail. */
	organizer = get_organizer(meet);
	dummy_org = g_new0(VCalAttendee, 1);
	dummy_org->address	= gtk_entry_new();
	dummy_org->avail_img	= meet->avail_img;
	dummy_org->avail_evtbox	= meet->avail_evtbox;
	dummy_org->org = TRUE;
	gtk_entry_set_text(GTK_ENTRY(dummy_org->address), organizer);
	g_free(organizer);
	dummy_org->cached_contents = file_read_to_str(internal_ifb);
	g_free(internal_ifb);
	
	if (!local_only) {
		meet->attendees = g_slist_prepend(meet->attendees, dummy_org);
		attlist = meet->attendees;
	} else {
		attlist = g_slist_prepend(NULL, dummy_org);
	}
	
	gtk_widget_set_sensitive(meet->save_btn, FALSE);
	gtk_widget_set_sensitive(meet->avail_btn, FALSE);

	if (meet->window->window)
		gdk_window_set_cursor(meet->window->window, watch_cursor);

	for (cur = attlist; cur && cur->data; cur = cur->next) {
		VCalAttendee *attendee = (VCalAttendee *)cur->data;
		gchar *email = gtk_editable_get_chars(GTK_EDITABLE(attendee->address), 0, -1);
		gchar *remail, *user, *domain;
		gchar *contents = NULL;

		if (*email == '\0') {
			g_free(email);
			att_update_icon(meet, attendee, 0, NULL);
			continue;
		}

		if (!local_only) {
			remail = g_strdup(email);
			g_free(email);
			extract_address(remail);
			if (strrchr(remail, ' '))
				user = g_strdup(strrchr(remail, ' ')+1);
			else
				user = g_strdup(remail);
			if (strchr(user, '@')) {
				domain = g_strdup(strchr(user, '@')+1);
				*(strchr(user, '@')) = '\0';
			} else {
				domain = g_strdup("");
			}
			g_free(remail);
			if (change_user > 0 && change_dom > 0) {
				if (change_user < change_dom)
					tmp = g_strdup_printf(real_url, user, domain);
				else
					tmp = g_strdup_printf(real_url, domain, user);
			} else if (change_user > 0) {
				tmp = g_strdup_printf(real_url, user);
			} else if (change_dom > 0) {
				tmp = g_strdup_printf(real_url, domain);
			} else {
				tmp = g_strdup(real_url);
			}
			g_free(user);
			g_free(domain);
			debug_print("url to get %s\n", tmp);
		}

		if (attendee->cached_contents != NULL) {
			contents = attendee->cached_contents;
			attendee->cached_contents = NULL;
		} else if (!local_only) {
			if (strncmp(tmp, "http://", 7) 
			&& strncmp(tmp, "https://", 8)
			&& strncmp(tmp, "webcal://", 9)
			&& strncmp(tmp, "ftp://", 6))
				contents = file_read_to_str(tmp);
			else {
				if (!strncmp(tmp, "webcal://", 9)) {
					gchar *tmp2 = g_strdup_printf("http://%s", tmp+9);
					g_free(tmp);
					tmp = tmp2;
				}
				contents = vcal_curl_read(tmp, FALSE, NULL);
			}
		} else {
			contents = NULL;
		}
		g_free(tmp);
		if (contents == NULL) {
			uncertain = TRUE;
			att_update_icon(meet, attendee, 2, _("Free/busy retrieval failed"));
			continue;
		}
		else {
			if (!attendee_available(attendee, dtstart, dtend, contents)) {
				find_avail = TRUE;
				debug_print("not available!\n");
			} else {
				debug_print("available!\n");
				att_update_icon(meet, attendee, 1, _("Available"));
			}
			attendee->cached_contents = contents;
			
		}
	}
	
	if (find_avail) {
		res = find_availability((dtstart), (dtend), attlist, for_send, meet);
	} else {
		res = TRUE;
		if (tell_if_ok) {
			if (for_send)
				alertpanel_notice(_("Everyone is available."));
			else if (!uncertain) {
				gtk_image_set_from_stock
		        		(GTK_IMAGE(meet->total_avail_img), 
					GTK_STOCK_DIALOG_INFO, 
					GTK_ICON_SIZE_SMALL_TOOLBAR);
				gtk_widget_show(meet->total_avail_img);
				gtk_label_set_text(GTK_LABEL(meet->total_avail_msg), _("Everyone is available."));
				gtk_tooltips_set_tip(meet->tips, meet->total_avail_evtbox, NULL, NULL);
			} else {
				gtk_image_set_from_stock
		        		(GTK_IMAGE(meet->total_avail_img), 
					GTK_STOCK_DIALOG_QUESTION, 
					GTK_ICON_SIZE_SMALL_TOOLBAR);
				gtk_widget_show(meet->total_avail_img);
				gtk_label_set_text(GTK_LABEL(meet->total_avail_msg), _("Everyone is available."));
				gtk_tooltips_set_tip(meet->tips, meet->total_avail_evtbox, 
					_("Everyone seems available, but some free/busy information failed to be retrieved."), NULL);
			}
		}
	}

	for (cur = attlist; cur && cur->data; cur = cur->next) {
		VCalAttendee *attendee = (VCalAttendee *)cur->data;
		g_free(attendee->cached_contents);
		attendee->cached_contents = NULL;
	}
	gtk_widget_set_sensitive(meet->save_btn, TRUE);
	gtk_widget_set_sensitive(meet->avail_btn, avail_btn_can_be_sensitive());
	if (meet->window->window)
		gdk_window_set_cursor(meet->window->window, NULL);

	if (!local_only)
		meet->attendees = g_slist_remove(meet->attendees, dummy_org);
	else
		g_slist_free(attlist);
	gtk_widget_destroy(dummy_org->address);
	g_free(dummy_org);

	if (!local_only)
		g_free(real_url);

	g_free(dtstart);
	g_free(dtend);
	return res;
}

static gboolean check_avail_cb(GtkButton *widget, gpointer data)
{
	VCalMeeting *meet = (VCalMeeting *)data;
	check_attendees_availability(meet, TRUE, FALSE);
	return TRUE;
}

static gboolean send_meeting_cb(GtkButton *widget, gpointer data)
{
	VCalMeeting *meet = (VCalMeeting *)data;
	gchar *uid = NULL;
	gchar *organizer = NULL;
	gchar *organizer_name = NULL;
	gchar *dtstart = NULL;
	gchar *dtend = NULL;
	gchar *tzid = NULL;
	gchar *summary = NULL;
	gchar *description = NULL;
	VCalEvent *event = NULL;
	gchar buf[256];
	GSList *cur;
	PrefsAccount *account = NULL;
	gboolean res = FALSE;
	gboolean found_att = FALSE;
	Folder *folder = folder_find_from_name ("vCalendar", vcal_folder_get_class());
	gboolean redisp = FALSE;

	if (meet->uid == NULL && meet->visible && 
	    !check_attendees_availability(meet, FALSE, TRUE)) {
		return FALSE;
	}

	if (folder) {
		MainWindow *mainwin = mainwindow_get_mainwindow();
		if (mainwin->summaryview->folder_item == folder->inbox) {
			redisp = TRUE;
			summary_show(mainwin->summaryview, NULL);
		}
	}
	gtk_widget_set_sensitive(meet->save_btn, FALSE);
	gtk_widget_set_sensitive(meet->avail_btn, FALSE);
	if (meet->window->window)
		gdk_window_set_cursor(meet->window->window, watch_cursor);

	organizer	= get_organizer(meet);
	organizer_name	= get_organizer_name(meet);
	account		= account_find_from_address(organizer);

	if (account && account->set_domain && account->domain) {
		g_snprintf(buf, sizeof(buf), "%s", account->domain); 
	} else if (!strncmp(get_domain_name(), "localhost", strlen("localhost"))) {
		g_snprintf(buf, sizeof(buf), "%s", 
			strchr(account->address, '@') ?
				strchr(account->address, '@')+1 :
				account->address);
	} else {
		g_snprintf(buf, sizeof(buf), "%s", "");
	}
	generate_msgid(buf, 255);

	if (meet->uid) {
		uid 	= g_strdup(meet->uid);
	} else {
		uid 	= g_strdup(buf);
	}

	dtstart		= get_date(meet, TRUE);
	dtend		= get_date(meet, FALSE);
	summary		= get_summary(meet);
	description	= get_description(meet);
	
	event = vcal_manager_new_event(uid, organizer, organizer_name, summary, description,
					dtstart, dtend, NULL, tzid, NULL, meet->method, 
					meet->sequence,
					ICAL_VEVENT_COMPONENT);
	
	vcal_manager_update_answer(event, organizer, organizer_name, 
				   ICAL_PARTSTAT_ACCEPTED,
				   ICAL_CUTYPE_INDIVIDUAL);
	
	for (cur = meet->attendees; cur && cur->data; cur = cur->next) {
		VCalAttendee *attendee = (VCalAttendee *)cur->data;
		gchar *email = gtk_editable_get_chars(GTK_EDITABLE(attendee->address), 0, -1);
		gint index = 0;
		gchar *orig_email = email;
		gchar *name = NULL;
		enum icalparameter_cutype cutype = ICAL_CUTYPE_INDIVIDUAL;
		enum icalparameter_partstat status = ICAL_PARTSTAT_NEEDSACTION;
		
		index = gtk_combo_box_get_active(GTK_COMBO_BOX(attendee->cutype));
		
		cutype = ICAL_CUTYPE_INDIVIDUAL + index;
		if (attendee->status) {
			if(!strcmp(attendee->status, "accepted"))
				status = ICAL_PARTSTAT_ACCEPTED;
			if(!strcmp(attendee->status, "tentatively accepted"))
				status = ICAL_PARTSTAT_TENTATIVE;
			if(!strcmp(attendee->status, "declined"))
				status = ICAL_PARTSTAT_DECLINED;
			g_free(attendee->status);			
		}
		if (strlen(email)) {
			if (strstr(email, " <")) {
				name = email;
				email = strstr(email," <") + 2;
				*(strstr(name," <")) = '\0';
				if (strstr(email, ">"))
					*(strstr(email, ">")) = '\0';
			} 
			
			vcal_manager_update_answer(event, email, name, 
					status,	cutype);
					
			found_att = strcmp(email, organizer);
		}
		g_free(orig_email);
	}
	
	if (found_att)
		res = vcal_manager_request(account, event);
	else
		res = TRUE;
	g_free(uid);
	g_free(organizer);
	g_free(organizer_name);
	g_free(dtstart);
	g_free(dtend);
	g_free(description);
	g_free(summary);
	vcal_manager_free_event(event);

	gtk_widget_set_sensitive(meet->save_btn, TRUE);
	gtk_widget_set_sensitive(meet->avail_btn, avail_btn_can_be_sensitive());
	if (meet->window->window)
		gdk_window_set_cursor(meet->window->window, NULL);

	if (res) {
		vcal_destroy(meet);
	} else {
		alertpanel_error(_("Could not send the meeting invitation.\n"
				   "Check the recipients."));
	}

	if (folder)
		folder_item_scan(folder->inbox);

	if (folder && redisp) {
		MainWindow *mainwin = mainwindow_get_mainwindow();
		summary_show(mainwin->summaryview, folder->inbox);
	}

	return res;
}

static GList *get_predefined_times(void)
{
	int h,m;
	GList *times = NULL;
	for (h = 0; h < 24; h++) {
		for (m = 0; m < 60; m += 15) {
			gchar *tmp = g_strdup_printf("%02d:%02d", h, m);
			times = g_list_append(times, tmp);
		}
	}
	return times;
}

static VCalMeeting *vcal_meeting_create_real(VCalEvent *event, gboolean visible)
{
	VCalMeeting *meet = g_new0(VCalMeeting, 1);
	GtkTextBuffer *buffer = NULL;
	GtkWidget *date_hbox, *date_vbox, *save_hbox, *label, *vbox, *hbox;
	gchar *s = NULL;
	GtkObject *start_h_adj, *start_m_adj, *end_h_adj, *end_m_adj;
	int i = 0, num = 0;
	GtkWidget *scrolledwin;
	GList *times = NULL;
	GList *accounts;
	
	if (!watch_cursor)
		watch_cursor = gdk_cursor_new(GDK_WATCH);

	meet->tips = gtk_tooltips_new();
	meet->visible = visible;
	start_h_adj = gtk_adjustment_new (0, 0, 23, 1, 10, 10);
	start_m_adj = gtk_adjustment_new (0, 0, 59, 1, 10, 10);
	end_h_adj   = gtk_adjustment_new (0, 0, 23, 1, 10, 10);
	end_m_adj   = gtk_adjustment_new (0, 0, 59, 1, 10, 10);

	meet->window 		= gtkut_window_new(GTK_WINDOW_TOPLEVEL, "vcal_meeting_gtk");
#ifndef MAEMO
	meet->table  		= gtk_table_new(7, 2, FALSE);
#else
	meet->table1  		= gtk_table_new(4, 2, FALSE);
	meet->table2  		= gtk_table_new(2, 2, FALSE);
#endif
	meet->who    		= gtk_combo_box_new_text();
	
	meet->start_c		= gtk_calendar_new();
	meet->end_c		= gtk_calendar_new();

	meet->avail_evtbox  = gtk_event_box_new();
	meet->avail_img	= gtk_image_new_from_stock
                        (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_SMALL_TOOLBAR);

	times = get_predefined_times();

	meet->start_time 	= gtk_combo_new();
	gtk_combo_set_popdown_strings(GTK_COMBO(meet->start_time), times);
	gtk_combo_set_value_in_list(GTK_COMBO(meet->start_time), FALSE, FALSE);
	meet->end_time 		= gtk_combo_new();
	gtk_combo_set_popdown_strings(GTK_COMBO(meet->end_time), times);
	gtk_combo_set_value_in_list(GTK_COMBO(meet->end_time), FALSE, FALSE);

	list_free_strings(times);
	g_list_free(times);

	meet->summary		= gtk_entry_new();
	meet->description	= gtk_text_view_new();
        buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(meet->description));
        gtk_text_view_set_editable(GTK_TEXT_VIEW(meet->description), TRUE);
        gtk_text_buffer_add_selection_clipboard(buffer, gtk_clipboard_get(GDK_SELECTION_PRIMARY));

	scrolledwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwin),
					    GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(scrolledwin), meet->description);

	if (event) {
		meet->uid = g_strdup(event->uid);
		meet->sequence = event->sequence + 1;
		meet->method = (event->method == ICAL_METHOD_CANCEL ?
				ICAL_METHOD_CANCEL:ICAL_METHOD_REQUEST);

		gtk_entry_set_text(GTK_ENTRY(meet->summary), event->summary);	
		gtk_text_buffer_set_text(buffer, event->description, -1);	
	} else 
		meet->method = ICAL_METHOD_REQUEST;
	
	meet->save_btn		= gtk_button_new_with_label(_("Save & Send"));
	meet->avail_btn		= gtk_button_new_with_label(_("Check availability"));

	meet->total_avail_evtbox  = gtk_event_box_new();
	meet->total_avail_img	= gtk_image_new_from_stock
                        (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_SMALL_TOOLBAR);
	meet->total_avail_msg = gtk_label_new("");
	
	gtk_widget_set_usize(meet->total_avail_evtbox, 18, 16);
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(meet->total_avail_evtbox), FALSE);
	gtk_container_add (GTK_CONTAINER(meet->total_avail_evtbox), meet->total_avail_img);

	g_signal_connect(G_OBJECT(meet->save_btn), "clicked",
			 G_CALLBACK(send_meeting_cb), meet);

	g_signal_connect(G_OBJECT(meet->avail_btn), "clicked",
			 G_CALLBACK(check_avail_cb), meet);

	g_signal_connect(G_OBJECT(meet->window), "destroy",
			 G_CALLBACK(destroy_meeting_cb), meet);
	g_signal_connect(G_OBJECT(meet->window), "key_press_event",
			 G_CALLBACK(meeting_key_pressed), meet);
	

	gtk_widget_set_size_request(meet->description, -1, 100);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(meet->description), GTK_WRAP_WORD);
	
	if (!event || (event && !event->dtstart && !event->dtend)) {
		time_t t = time (NULL)+ 3600;
		struct tm buft1, buft2;
 		struct tm *lt = localtime_r (&t, &buft1);
		int num = -1;
		mktime(lt);
		gtk_calendar_select_day(GTK_CALENDAR(meet->start_c),
					lt->tm_mday);
		gtk_calendar_select_month(GTK_CALENDAR(meet->start_c),
					lt->tm_mon, lt->tm_year + 1900);
	
		num = get_list_item_num(lt->tm_hour, 0);
		if (num > -1)
			gtk_list_select_item(GTK_LIST(GTK_COMBO(meet->start_time)->list), num);

		t += 3600;
		lt = localtime_r(&t, &buft2);
		
		gtk_calendar_select_day(GTK_CALENDAR(meet->end_c),
					lt->tm_mday);
		gtk_calendar_select_month(GTK_CALENDAR(meet->end_c),
					lt->tm_mon, lt->tm_year + 1900);

		num = get_list_item_num(lt->tm_hour, 0);
		if (num > -1)
			gtk_list_select_item(GTK_LIST(GTK_COMBO(meet->end_time)->list), num);

	} else {
		gtk_calendar_select_day(GTK_CALENDAR(meet->start_c),
					get_dtdate(event->dtstart, DAY));
		gtk_calendar_select_day(GTK_CALENDAR(meet->end_c),
					get_dtdate(event->dtend, DAY));

		gtk_calendar_select_month(GTK_CALENDAR(meet->start_c),
					get_dtdate(event->dtstart, MONTH)-1,
					get_dtdate(event->dtstart, YEAR));
		gtk_calendar_select_month(GTK_CALENDAR(meet->end_c),
					get_dtdate(event->dtend, MONTH)-1,
					get_dtdate(event->dtend, YEAR));

		num = get_list_item_num(get_dtdate(event->dtstart, HOUR), 
					get_dtdate(event->dtstart, MINUTE));
		if (num > -1)
			gtk_list_select_item(GTK_LIST(GTK_COMBO(meet->start_time)->list), num);
		else {
			gchar *tmp = g_strdup_printf("%02d:%02d",
					get_dtdate(event->dtstart, HOUR), 
					get_dtdate(event->dtstart, MINUTE));
			gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(meet->start_time)->entry),
					   tmp);
			g_free(tmp);
		}

		num = get_list_item_num(get_dtdate(event->dtend, HOUR), 
					get_dtdate(event->dtend, MINUTE));
		if (num > -1)
			gtk_list_select_item(GTK_LIST(GTK_COMBO(meet->end_time)->list), num);
		else {
			gchar *tmp = g_strdup_printf("%02d:%02d",
					get_dtdate(event->dtend, HOUR), 
					get_dtdate(event->dtend, MINUTE));
			gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(meet->end_time)->entry),
					   tmp);
			g_free(tmp);
		}
	}

	g_signal_connect(G_OBJECT(meet->start_c), "day-selected",
			 G_CALLBACK(meeting_start_changed), meet);
	g_signal_connect(G_OBJECT(GTK_COMBO(meet->start_time)->entry),
			 "changed",
			 G_CALLBACK(meeting_start_changed),
			 meet);

	g_signal_connect(G_OBJECT(meet->end_c), "day-selected",
			 G_CALLBACK(meeting_end_changed), meet);
	g_signal_connect(G_OBJECT(GTK_COMBO(meet->end_time)->entry),
			 "changed",
			 G_CALLBACK(meeting_end_changed),
			 meet);

#ifndef MAEMO
	gtk_widget_set_size_request(meet->start_time, 80, -1);
	gtk_widget_set_size_request(meet->end_time, 80, -1);
#else
	gtk_widget_set_size_request(meet->start_time, 120, -1);
	gtk_widget_set_size_request(meet->end_time, 120, -1);
#endif
	
	date_hbox = gtk_hbox_new(FALSE, 6);
	date_vbox = gtk_vbox_new(FALSE, 6);
	hbox = gtk_hbox_new(FALSE, 6);
	label = gtk_label_new(_("<b>Starts at:</b> ")); gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), meet->start_time, FALSE, FALSE, 0);
	label = gtk_label_new(_("<b> on:</b>")); gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(date_vbox), hbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(date_vbox), meet->start_c, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(date_hbox), date_vbox, FALSE, FALSE, 0);

#ifndef MAEMO
	label = gtk_label_new(" "); 
#else
	label = gtk_label_new(""); 
#endif
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(date_hbox), label, TRUE, TRUE, 0);

	date_vbox = gtk_vbox_new(FALSE, 6);
	hbox = gtk_hbox_new(FALSE, 6);
	label = gtk_label_new(_("<b>Ends at:</b> ")); gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), meet->end_time, FALSE, FALSE, 0);
	label = gtk_label_new(_("<b> on:</b>")); gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(date_vbox), hbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(date_vbox), meet->end_c, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(date_hbox), date_vbox, FALSE, FALSE, 0);

	meet->attendees_vbox = gtk_vbox_new(FALSE, 6);
	gtk_widget_show_all(meet->attendees_vbox);
	if (!event) {
		attendee_add(meet, NULL, NULL, NULL, NULL, TRUE);
	} else {
		gboolean firstadd = TRUE;
		GSList *list = vcal_manager_get_answers_emails(event);
		while (list && list->data) {
			gchar *address = (gchar *)list->data;
			gchar *name = vcal_manager_get_attendee_name(event, address);
			gchar *answer = vcal_manager_get_reply_text_for_attendee(event, address);
			gchar *type = vcal_manager_get_cutype_text_for_attendee(event, address);
			if (strcmp(event->organizer, address)) {
				attendee_add(meet, address, name, answer, type, firstadd);
				firstadd = FALSE;
			}
			g_free(name);
			g_free(answer);
			g_free(type);
			list = list->next;
		} 
		
		if (firstadd == TRUE)
			attendee_add(meet, NULL, NULL, NULL, NULL, TRUE);
	}

	if (!event) {
		gtk_window_set_title(GTK_WINDOW(meet->window), _("New meeting"));
	} else {
		gchar *title = g_strdup_printf(_("%s - Edit meeting"),
			event->summary);
		gtk_window_set_title(GTK_WINDOW(meet->window), title);
		g_free(title);
	}
	address_completion_start(meet->window);
	
	accounts = account_get_list();
	g_return_val_if_fail(accounts != NULL, NULL);

	for (i = 0; accounts != NULL; accounts = accounts->next) {
		PrefsAccount *ac = (PrefsAccount *)accounts->data;
		
		if (ac->protocol == A_NNTP) {
			continue;
		}
		if (!event && ac == cur_account) {
			num = i;
		}
		else if (event && !strcmp(ac->address, event->organizer))
			num = i;

		meet->avail_accounts = g_slist_append(meet->avail_accounts, ac);
		
		if (ac->name)
			s = g_strdup_printf("%s: %s <%s>",
					       ac->account_name,
					       ac->name, ac->address);
		else
			s = g_strdup_printf("%s: %s",
					       ac->account_name, ac->address);

		gtk_combo_box_append_text(GTK_COMBO_BOX(meet->who), s);
		g_free(s);
		i++;
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(meet->who), num);
	
	save_hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(save_hbox), meet->save_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(save_hbox), meet->avail_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(save_hbox), meet->total_avail_evtbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(save_hbox), meet->total_avail_msg, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbox), meet->avail_evtbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), meet->who, TRUE, TRUE, 0);

	gtk_widget_set_usize(meet->avail_evtbox, 18, 16);
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(meet->avail_evtbox), FALSE);
	gtk_container_add (GTK_CONTAINER(meet->avail_evtbox), meet->avail_img);

#ifndef MAEMO
	TABLE_ADD_LINE(_("Organizer:"), hbox, FALSE);
	TABLE_ADD_LINE(_("Summary:"), meet->summary, TRUE);
	TABLE_ADD_LINE(_("Time:"), date_hbox, TRUE);
	TABLE_ADD_LINE(_("Description:"), scrolledwin, TRUE);
	TABLE_ADD_LINE(_("Attendees:"), meet->attendees_vbox, FALSE);
	TABLE_ADD_LINE("", save_hbox, TRUE);
	
	gtk_widget_set_size_request(meet->window, -1, -1);
	gtk_container_add(GTK_CONTAINER(meet->window), meet->table);
#else
	TABLE_ADD_LINE(_("Organizer:"), hbox, FALSE, TRUE);
	TABLE_ADD_LINE(_("Summary:"), meet->summary, TRUE, TRUE);
	TABLE_ADD_LINE(_("Description:"), scrolledwin, TRUE, TRUE);
	TABLE_ADD_LINE(_("Attendees:"), meet->attendees_vbox, FALSE, TRUE);
	TABLE_ADD_LINE("", date_hbox, TRUE, FALSE);
	
	GtkWidget *notebook;
	notebook = gtk_notebook_new ();
	gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
			meet->table1,
			gtk_label_new_with_mnemonic(_("Event:")));
			
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
			meet->table2,
			gtk_label_new_with_mnemonic(_("Time:")));
	gtk_widget_show (notebook);
	
	GtkWidget *maemo_vbox0 = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(maemo_vbox0), notebook, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(maemo_vbox0), save_hbox, FALSE, FALSE, 0);
	
	gtk_widget_set_size_request(meet->window, -1, -1);
	gtk_container_add (GTK_CONTAINER (meet->window), maemo_vbox0);
	
	maemo_connect_key_press_to_mainwindow(GTK_WINDOW(meet->window));
#endif
	if (visible) {
		GSList *cur;
		gtk_widget_show_all(meet->window);
#ifdef MAEMO
		maemo_window_full_screen_if_needed(GTK_WINDOW(meet->window));
#endif
		for (cur = meet->attendees; cur; cur = cur->next) {
			gtk_widget_hide(((VCalAttendee *)cur->data)->avail_img);
		}
		gtk_widget_hide(meet->avail_img);
		gtk_widget_hide(meet->total_avail_img);
		gtk_widget_set_sensitive(meet->avail_btn, avail_btn_can_be_sensitive());
	}
	return meet;
}

VCalMeeting *vcal_meeting_create(VCalEvent *event)
{
	return vcal_meeting_create_real(event, TRUE);
}

VCalMeeting *vcal_meeting_create_hidden(VCalEvent *event)
{
	return vcal_meeting_create_real(event, FALSE);
}

gboolean vcal_meeting_send(VCalMeeting *meet)
{
	return send_meeting_cb(NULL, meet);
}

static gboolean g_slist_find_string(GSList *list, const gchar * str)
{
	GSList *cur = list;
	while (cur) {
		if (!strcmp((gchar *)cur->data, str))
			return TRUE;
		cur = cur->next;
	}
	return FALSE;
}

gint vcal_meeting_alert_check(gpointer data)
{
	GSList *events = NULL, *cur = NULL;
	static GSList *alert_done = NULL;

	if (!vcalprefs.alert_enable)
		return TRUE;

	events = vcal_folder_get_waiting_events();

	for (cur = events; cur; cur = cur->next) {
		VCalEvent *event = (VCalEvent *)cur->data;
		time_t start, end, current;
		gboolean warn = FALSE;

		tzset();

		start = icaltime_as_timet(icaltime_from_string(event->dtstart));
		end = icaltime_as_timet(icaltime_from_string(event->dtend));
		current = time(NULL);
		
		if (start - current <= (vcalprefs.alert_delay*60) 
		&&  start - current + 60 > (vcalprefs.alert_delay*60)) {
			warn = TRUE;
		} else if (event->postponed - current <= (vcalprefs.alert_delay*60)
		&&         event->postponed - current + 60 > (vcalprefs.alert_delay*60)) {
			warn = TRUE;
		}
		if (warn) {
			time_t tmpt = icaltime_as_timet((icaltime_from_string(event->dtstart)));
			gchar *estart = NULL;
			AlertValue aval;
			int length = (end - start) / 60;
			gchar *duration = NULL, *hours = NULL, *minutes = NULL;
			gchar *message = NULL;
			gchar *title = NULL;
			gchar *label = NULL;
			int postpone_min = 0;

			tzset();

			estart = g_strdup(ctime(&tmpt));

			if (length >= 60)
				hours = g_strdup_printf(ngettext("%d hour", "%d hours", 
						(length/60) > 1 ? 2 : 1), length/60);
			if (length%60)
				minutes = g_strdup_printf(ngettext("%d minute", "%d minutes",
						length%60), length%60);

			duration = g_strdup_printf("%s%s%s",
					hours?hours:"",
					hours && minutes ? " ":"",
					minutes?minutes:"");

			g_free(hours);
			g_free(minutes);

			title = g_strdup_printf(_("Upcoming event: %s"), event->summary);
			message = g_strdup_printf(_("You have a meeting or event soon.\n"
					 "It starts at %s and ends %s later.\n"
					 "More information:\n\n"
					 "%s"),
						estart,
						duration,
						event->description);

			g_free(duration);
			g_free(estart);

			postpone_min = (vcalprefs.alert_delay/2 > 15) ? 15: (vcalprefs.alert_delay/2);
			if (postpone_min == 0)
				postpone_min = 1;

			label = g_strdup_printf(ngettext("Remind me in %d minute", "Remind me in %d minutes",
						 postpone_min > 1 ? 2:1), 
						 postpone_min);
			aval = alertpanel_full(title, message,
				   	label, GTK_STOCK_OK, NULL, FALSE,
				   	NULL, ALERT_NOTICE, G_ALERTDEFAULT);
			g_free(label);

			g_free(title);
			g_free(message);

			if (aval == G_ALERTDEFAULT) {
				if (event->postponed == 0)
					event->postponed = start + (postpone_min*60);
				else
					event->postponed += (postpone_min*60);
			} else {
				event->postponed = (time_t)0;
			}
			vcal_manager_save_event(event, FALSE);
		}
		
		vcal_manager_free_event((VCalEvent *)cur->data);
	}
	
	g_slist_free(events);

	return TRUE;
}

void multisync_export(void)
{
	GSList *list = NULL;
	gchar *path = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S,
				"vcalendar", G_DIR_SEPARATOR_S, 
				"multisync", NULL);
	GSList *files = NULL;
	GSList *cur = NULL;
	gchar *file = NULL;
	gchar *tmp = NULL;
	gint i = 0;
	icalcomponent *calendar = NULL;
	FILE *fp;

	if (is_dir_exist(path))
		remove_dir_recursive(path);
	if (!is_dir_exist(path))
		make_dir(path);
	if (!is_dir_exist(path)) {
		perror(path);
		g_free(path);
		return;
	}
	
	list = vcal_folder_get_waiting_events();
	for (cur = list; cur; cur = cur->next) {
		VCalEvent *event = (VCalEvent *)cur->data;
		file = g_strdup_printf("multisync%lu-%d", time(NULL), i);

		i++;

		calendar = 
        		icalcomponent_vanew(
        		    ICAL_VCALENDAR_COMPONENT,
	        	    icalproperty_new_version("2.0"),
        		    icalproperty_new_prodid(
                		 "-//Claws Mail//NONSGML Claws Mail Calendar//EN"),
			    icalproperty_new_calscale("GREGORIAN"),
        		    0
        	    ); 	
		vcal_manager_event_dump(event, FALSE, FALSE, calendar, FALSE);
		tmp = g_strconcat(path, G_DIR_SEPARATOR_S, file, NULL);
		str_write_to_file(icalcomponent_as_ical_string(calendar), tmp);
		g_free(tmp);
		files = g_slist_append(files, file);
		vcal_manager_free_event(event);
		icalcomponent_free(calendar);
	}

	g_slist_free(list);
	
	file = g_strconcat(path, G_DIR_SEPARATOR_S, "backup_entries", NULL);
	fp = fopen(file, "wb");
	g_free(file);
	if (fp) {
		for (cur = files; cur; cur = cur->next) {
			file = (char *)cur->data;
			fprintf(fp, "1 1 %s\n", file);
			g_free(file);
		}
		fclose(fp);
	} else {
		perror(file);
	}
	g_free(path);
	g_slist_free(files);
}

gboolean vcal_meeting_export_calendar(const gchar *path, 
				const gchar *user, const gchar *pass,
				gboolean automatic)
{
	GSList *list = vcal_folder_get_waiting_events();
	GSList *subs = NULL;
	GSList *cur;
	icalcomponent *calendar = NULL;
	gchar *file = NULL;
	gchar *tmpfile = get_tmp_file();
	gchar *internal_file = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S,
				"vcalendar", G_DIR_SEPARATOR_S, 
				"internal.ics", NULL);

	gboolean res = TRUE;
	long filesize = 0;
	
	multisync_export();

	if (vcalprefs.export_subs && vcalprefs.export_enable)
		subs = vcal_folder_get_webcal_events();

	if (g_slist_length(list) == 0 && g_slist_length(subs) == 0) {
		g_slist_free(list);
		g_slist_free(subs);
		if (!automatic) {
			alertpanel_full(_("Empty calendar"),
					_("There is nothing to export."),
				   	GTK_STOCK_OK, NULL, NULL, FALSE,
				   	NULL, ALERT_NOTICE, G_ALERTDEFAULT);
			return FALSE;
		} else {
			str_write_to_file("", tmpfile);
			goto putfile;
		}
	}
	
	calendar = 
        	icalcomponent_vanew(
        	    ICAL_VCALENDAR_COMPONENT,
	            icalproperty_new_version("2.0"),
        	    icalproperty_new_prodid(
                	 "-//Claws Mail//NONSGML Claws Mail Calendar//EN"),
		    icalproperty_new_calscale("GREGORIAN"),
        	    0
            ); 	

	for (cur = list; cur; cur = cur->next) {
		VCalEvent *event = (VCalEvent *)cur->data;
		vcal_manager_event_dump(event, FALSE, FALSE, calendar, FALSE);
		vcal_manager_free_event(event);
	}

	if (str_write_to_file(icalcomponent_as_ical_string(calendar), internal_file) < 0) {
		g_warning("can't export internal cal\n");
	}
	
	g_free(internal_file);

	for (cur = subs; cur; cur = cur->next) {
		icalcomponent *event = (icalcomponent *)cur->data;
		vcal_manager_icalevent_dump(event, NULL, calendar);
		icalcomponent_free(event);
	}

	if (vcalprefs.export_enable || path == NULL) {
		if (str_write_to_file(icalcomponent_as_ical_string(calendar), tmpfile) < 0) {
			alertpanel_error(_("Could not export the calendar."));
			g_free(tmpfile);
			icalcomponent_free(calendar);
			g_slist_free(list);
			g_slist_free(subs);
			return FALSE;
		}
		filesize = strlen(icalcomponent_as_ical_string(calendar));
	}

	icalcomponent_free(calendar);
	
putfile:
	g_slist_free(list);
	g_slist_free(subs);

	if (!path && !automatic)
		file = filesel_select_file_save(_("Export calendar to ICS"), NULL);
	else
		file = g_strdup(path);

	if (automatic && (!path || strlen(path) == 0 || !vcalprefs.export_enable)) {
		g_free(tmpfile);
		g_free(file);
		return TRUE;
	}

	if (file 
	&& strncmp(file, "http://", 7) 
	&& strncmp(file, "https://", 8)
	&& strncmp(file, "webcal://", 9)
	&& strncmp(file, "ftp://", 6)) {
		gchar *afile = NULL;
		if (file[0] != G_DIR_SEPARATOR)
			afile=g_strdup_printf("%s%s%s", get_home_dir(), 
					G_DIR_SEPARATOR_S, file);
		else
			afile=g_strdup(file);
		if (move_file(tmpfile, afile, TRUE) != 0) {
			log_error(LOG_PROTOCOL, _("Couldn't export calendar to '%s'\n"),
				afile);
			res = FALSE;
		}
		g_free(afile);
		g_free(file);
	} else if (file) {
		FILE *fp = fopen(tmpfile, "rb");
		if (!strncmp(file, "webcal://", 9)) {
			gchar *tmp = g_strdup_printf("http://%s", file+9);
			g_free(file);
			file = tmp;
		}
		if (fp) {
			res = vcal_curl_put(file, fp, filesize, user, pass);
			fclose(fp);
		}
		g_free(file);
	}
	g_free(tmpfile);
	return res;
}

#if 0
typedef struct _BusySpot BusySpot;
struct _BusySpot {
	time_t start;
	time_t end;
};

static GSList *vcal_g_slist_merge(GSList *list, BusySpot *spot, gboolean append_if_not_merged)
{
	BusySpot *newspot = g_new0(BusySpot, 1);
	newspot->start = spot->start;
	newspot->end = spot->end;
	gchar a[100], b[100], c[100], d[100];

	if (list == NULL) {
		debug_print("%p (%s-%s) fits nowhere because list empty, %smerging\n",
			spot, ctime_r(&spot->start, a), ctime_r(&spot->end, b),
			append_if_not_merged?"":"not ");
		if (append_if_not_merged)
			return g_slist_append(NULL, newspot);
		else 
			return NULL;
	} else {
		GSList *cur = list;
		for (; cur; cur = cur->next) {
			BusySpot *curspot = (BusySpot *)cur->data;
			if (curspot == spot) {
				/* same one ! don't merge. */
				debug_print("not merging %p (%s-%s) with itself\n", spot, ctime_r(&spot->start, a),
					ctime_r(&spot->end, b));
				continue;
			} else if (curspot->start <= newspot->start && curspot->end >= newspot->end) {
				/* fits in, remove the new one */
				debug_print("%p:\n%s%s fits in %p:\n%s%s\n\n",
					spot, ctime_r(&spot->start, a), ctime_r(&spot->end, b),
					curspot, ctime_r(&curspot->start, c), ctime_r(&curspot->end, d));
				g_free(newspot);
				return list;
			} else if (newspot->start < curspot->start && newspot->end > curspot->end) {
				/* the new one extends the existing one */
				debug_print("%p:\n%s%s extends %p:\n%s%s\n\n",
					spot, ctime_r(&spot->start, a), ctime_r(&spot->end, b),
					curspot, ctime_r(&curspot->start, c), ctime_r(&curspot->end, d));
				curspot->start = newspot->start;
				curspot->end = newspot->end;
				g_free(newspot);
				return list;
			} else if (newspot->start < curspot->start && newspot->end >= curspot->start) {
				/* the new one starts before, update the existing */
				debug_print("%p:\n%s%s starts before %p:\n%s%s\n\n",
					spot, ctime_r(&spot->start, a), ctime_r(&spot->end, b),
					curspot, ctime_r(&curspot->start, c), ctime_r(&curspot->end, d));
				curspot->start = newspot->start;
				g_free(newspot);
				return list;
			} else if (newspot->end > curspot->end && newspot->start <= curspot->end) {
				/* the new one ends after, update the existing */
				debug_print("%p:\n%s%s ends after %p:\n%s%s\n\n",
					spot, ctime_r(&spot->start, a), ctime_r(&spot->end, b),
					curspot, ctime_r(&curspot->start, c), ctime_r(&curspot->end, d));
				curspot->end = newspot->end;
				g_free(newspot);
				return list;
			} else {
				/* they don't overlap. continue searching. */
			}
		}
		debug_print("%p (%s-%s) fits nowhere, %sadding\n",
			spot, ctime_r(&spot->start, a), ctime_r(&spot->end, b),
			append_if_not_merged?"":"not ");

		/* if we reach that, we found nowhere to overlap, so insert */
		if (append_if_not_merged)
			return g_slist_append(list, newspot);
		else
			return list;
	}
}

static GSList *vcal_g_slist_merge_all(GSList *list)
{
	int length = 0;
	
merge_again:
	length = g_slist_length(list);
	
	if (length <= 1) {
		/* unmergeable */
		return list;
	} else {
		GSList *cur = list;
		GSList *new_list = NULL;
		for (cur = list; cur; cur = cur->next) {
			new_list = vcal_g_slist_merge(new_list, (BusySpot *)cur->data, TRUE);
			g_free(cur->data);
		}
		g_slist_free(list);
		list = new_list;
		if (g_slist_length(new_list) < length) {
			/* we did merge stuff */
			goto merge_again;
		}
		return list;
	}
}
#endif

gboolean vcal_meeting_export_freebusy(const gchar *path, const gchar *user,
				const gchar *pass)
{
	GSList *list = vcal_folder_get_waiting_events();
	GSList *cur;
	icalcomponent *calendar = NULL, *timezone = NULL, *tzc = NULL, *vfreebusy = NULL;
	gchar *file = NULL;
	gchar *tmpfile = get_tmp_file();
	gchar *internal_file = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S,
				"vcalendar", G_DIR_SEPARATOR_S, 
				"internal.ifb", NULL);
	time_t whole_start = time(NULL);
	time_t whole_end = whole_start + (60*60*24*365);
	gboolean res = TRUE;
	GSList *busy_spots = NULL;
	struct icaltimetype itt_start, itt_end;
	long filesize = 0;
	
	multisync_export();

	calendar = 
        	icalcomponent_vanew(
        	    ICAL_VCALENDAR_COMPONENT,
	            icalproperty_new_version("2.0"),
        	    icalproperty_new_prodid(
                	 "-//Claws Mail//NONSGML Claws Mail Calendar//EN"),
		    icalproperty_new_calscale("GREGORIAN"),
        	    0
            ); 	

	timezone = icalcomponent_new(ICAL_VTIMEZONE_COMPONENT);
	
	icalcomponent_add_property(timezone,
		icalproperty_new_tzid("UTC"));
	
	tzc = icalcomponent_new(ICAL_XSTANDARD_COMPONENT);
	icalcomponent_add_property(tzc,
		icalproperty_new_dtstart(
			icaltime_from_string("19700101T000000")));
	icalcomponent_add_property(tzc,
		icalproperty_new_tzoffsetfrom(0.0));
	icalcomponent_add_property(tzc,
		icalproperty_new_tzoffsetto(0.0));
	icalcomponent_add_property(tzc,
		icalproperty_new_tzname("Greenwich meridian time"));

        icalcomponent_add_component(timezone, tzc);

	icalcomponent_add_component(calendar, timezone);

#if 0
	for (cur = list; cur; cur = cur->next) {
		VCalEvent *event = (VCalEvent *)cur->data;
		BusySpot *busy = g_new0(BusySpot, 1);
		BusySpot *merged;
		busy->start = icaltime_as_timet((icaltime_from_string(event->dtstart)));
		busy->end = icaltime_as_timet((icaltime_from_string(event->dtend)));
		busy_spots = vcal_g_slist_merge(busy_spots, busy, TRUE);
		g_free(busy);
		vcal_manager_free_event(event);
	}

	busy_spots = vcal_g_slist_merge_all(busy_spots);
	debug_print("merged to %d events\n", g_slist_length(busy_spots));
#endif

	itt_start = icaltime_from_timet(whole_start, FALSE);
	itt_end = icaltime_from_timet(whole_end, FALSE);
	itt_start.second = itt_start.minute = itt_start.hour = 0;
	itt_end.second = 59; itt_end.minute = 59; itt_end.hour = 23;


	vfreebusy = 
	    icalcomponent_vanew(
                ICAL_VFREEBUSY_COMPONENT,
		icalproperty_vanew_dtstart(itt_start, 0),
		icalproperty_vanew_dtend(itt_end, 0),
                0
                );

	debug_print("DTSTART:%s\nDTEND:%s\n",
		icaltime_as_ical_string(itt_start),
		icaltime_as_ical_string(itt_end));

#if 0
	for (cur = busy_spots; cur; cur = cur->next) {
		icalproperty *prop;
		struct icalperiodtype ipt;

		BusySpot *spot = (BusySpot *)cur->data;
		debug_print("FREEBUSY:%s/%s\n",
			icaltime_as_ical_string(icaltime_from_timet(spot->start, FALSE)),
			icaltime_as_ical_string(icaltime_from_timet(spot->end, FALSE)));
		
		ipt.start = icaltime_from_timet(spot->start, FALSE);
		ipt.end = icaltime_from_timet(spot->end, FALSE);
		ipt.duration = icaltime_subtract(ipt.end, ipt.start);
		prop = icalproperty_new_freebusy(ipt);
		icalcomponent_add_property(vfreebusy, prop);

		g_free(spot);
	}
	g_slist_free(busy_spots);
#endif

	for (cur = list; cur; cur = cur->next) {
		VCalEvent *event = (VCalEvent *)cur->data;
		icalproperty *prop;
		struct icalperiodtype ipt;
	
		ipt.start = icaltime_from_string(event->dtstart);
		ipt.end = icaltime_from_string(event->dtend);
		ipt.duration = icaltime_subtract(ipt.end, ipt.start);
		if (icaltime_as_timet(ipt.start) <= icaltime_as_timet(itt_end) 
		 && icaltime_as_timet(ipt.end) >= icaltime_as_timet(itt_start)) {
			prop = icalproperty_new_freebusy(ipt);
			icalcomponent_add_property(vfreebusy, prop);
		}
		vcal_manager_free_event(event);
	}

	icalcomponent_add_component(calendar, vfreebusy);
	
	if (str_write_to_file(icalcomponent_as_ical_string(calendar), internal_file) < 0) {
		g_warning("can't export freebusy\n");
	}
	
	g_free(internal_file);

	if (vcalprefs.export_freebusy_enable) {
		if (str_write_to_file(icalcomponent_as_ical_string(calendar), tmpfile) < 0) {
			alertpanel_error(_("Could not export the freebusy info."));
			g_free(tmpfile);
			icalcomponent_free(calendar);
			g_slist_free(list);
			return FALSE;
		}
		filesize = strlen(icalcomponent_as_ical_string(calendar));
	}

	icalcomponent_free(calendar);
	g_slist_free(list);
	
putfile:
	if ((!path || strlen(path) == 0 || !vcalprefs.export_freebusy_enable)) {
		g_free(tmpfile);
		return TRUE;
	}
	file = g_strdup(path);


	if (file 
	&& strncmp(file, "http://", 7) 
	&& strncmp(file, "https://", 8)
	&& strncmp(file, "webcal://", 9)
	&& strncmp(file, "ftp://", 6)) {
		gchar *afile = NULL;
		if (file[0] != G_DIR_SEPARATOR)
			afile=g_strdup_printf("%s%s%s", get_home_dir(), 
					G_DIR_SEPARATOR_S, file);
		else
			afile=g_strdup(file);
		if (move_file(tmpfile, file, TRUE) != 0) {
			log_error(LOG_PROTOCOL, _("Couldn't export free/busy to '%s'\n"),
				afile);
			res = FALSE;
		}
		g_free(afile);
		g_free(file);
	} else if (file) {
		FILE *fp = fopen(tmpfile, "rb");
		if (!strncmp(file, "webcal://", 9)) {
			gchar *tmp = g_strdup_printf("http://%s", file+9);
			g_free(file);
			file = tmp;
		}
		if (fp) {
			res = vcal_curl_put(file, fp, filesize, user, pass);
			fclose(fp);
		}
		g_free(file);
	}
	g_free(tmpfile);
	return res;
}

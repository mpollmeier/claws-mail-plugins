/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2005 Hiroyuki Yamamoto & the Sylpheed-Claws team
 * This file (C) 2004-2005 Colin Leroy <colin@colino.net>
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
#include "prefs_account.h"
#include "account.h"
#include "filesel.h"
#include "alertpanel.h"
#include "addr_compl.h"

struct _VCalMeeting
{
	gchar     *uid;
	gint       sequence;
	gint 	   method;
	GtkWidget *window;
	GtkWidget *table;
	GtkWidget *type;
	GtkWidget *who;
	GtkWidget *start_c;
	GtkWidget *start_hh;
	GtkWidget *start_mm;
	GtkWidget *end_c;
	GtkWidget *end_hh;
	GtkWidget *end_mm;
	GtkWidget *summary;
	GtkWidget *description;
	GSList 	  *attendees;
	GtkWidget *attendees_vbox;
	GtkWidget *save_btn;
	GSList 	  *avail_accounts;
	PrefsAccount *account;
};

typedef struct _VCalAttendee VCalAttendee;

struct _VCalAttendee {
	GtkWidget *address;
	GtkWidget *remove_btn;
	GtkWidget *add_btn;
	GtkWidget *cutype;
	GtkWidget *hbox;
	VCalMeeting *meet;
	gchar *status;
};

VCalAttendee *attendee_add(VCalMeeting *meet, gchar *address, gchar *name, gchar *partstat, gchar *cutype, gboolean first);

#define TABLE_ADD_LINE(label_text, widget) { 					\
	gchar *tmpstr = g_strdup_printf("<span weight=\"bold\">%s</span>",	\
				label_text?label_text:"");			\
	GtkWidget *label = NULL;				 		\
	if (label_text) {							\
		label = gtk_label_new(tmpstr);					\
		g_free(tmpstr);							\
		gtk_label_set_use_markup (GTK_LABEL (label), TRUE);		\
		gtk_misc_set_alignment (GTK_MISC(label), 1, 0.5);		\
		gtk_table_attach (GTK_TABLE (meet->table), 			\
				  label, 0, 1, i, i+1,				\
				  GTK_FILL, GTK_FILL, 6, 6);			\
		gtk_table_attach (GTK_TABLE (meet->table), 			\
				  widget, 1, 2, i, i+1,				\
				  GTK_FILL|GTK_EXPAND, GTK_FILL, 6, 6);		\
		if (GTK_IS_LABEL(widget)) {					\
			gtk_label_set_use_markup(GTK_LABEL (widget), TRUE);	\
			gtk_misc_set_alignment (GTK_MISC(widget),0, 0);		\
			gtk_label_set_line_wrap(GTK_LABEL(widget), TRUE);	\
		}								\
	} else {								\
		g_free(tmpstr);							\
		gtk_table_attach (GTK_TABLE (meet->table), 			\
				  widget, 0, 2, i, i+1,				\
				  GTK_FILL|GTK_EXPAND, GTK_FILL, 6, 6);		\
	}									\
	i++;									\
}

enum {
	DAY,
	MONTH,
	YEAR,
	HOUR,
	MINUTE
};

static gchar *get_month_name(gint i)
{
	struct tm *lt;
	time_t t;
	gchar day[4], mon[4];
	gint dd, hh, mm, ss, yyyy;

	t = time(NULL);
	lt = localtime(&t);
	lt->tm_mon = i;
	
	sscanf(asctime(lt), "%3s %3s %d %d:%d:%d %d\n",
	       day, mon, &dd, &hh, &mm, &ss, &yyyy);
	
	return g_strdup(mon);
}

static gint get_curdate(gint field)
{
	struct tm *lt;
	time_t t;

	t = time(NULL);
	lt = localtime(&t);

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

static gint get_dtdate(const gchar *str, gint field)
{
	time_t t = icaltime_as_timet((icaltime_from_string(str)));
	struct tm *lt = localtime(&t);

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

	/* resizing doesn't work :-( */
	gtk_widget_set_size_request(attendee->meet->window, -1, -1);
	gtk_widget_show_all(attendee->meet->window);
	return TRUE;
}

VCalAttendee *attendee_add(VCalMeeting *meet, gchar *address, gchar *name, gchar *partstat, gchar *cutype, gboolean first)
{
	GtkWidget *att_hbox = gtk_hbox_new(FALSE, 6);
	VCalAttendee *attendee 	= g_new0(VCalAttendee, 1);
	attendee->address	= gtk_entry_new();
	attendee->cutype	= gtk_combo_box_new_text();

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
	address_completion_register_entry(GTK_ENTRY(attendee->address));
	gtk_widget_set_size_request(attendee->address, 320, -1);
	gtk_widget_show_all(meet->attendees_vbox);
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

static gchar *get_date(VCalMeeting *meet, int start) 
{
	struct tm *lt;
	time_t t;
	guint d, m, y;

	t = time(NULL);
	lt = localtime(&t);
	
	gtk_calendar_get_date(GTK_CALENDAR(start ? meet->start_c : meet->end_c), &y, &m, &d);
	lt->tm_mday = d;
	lt->tm_mon  = m;
	lt->tm_year = y - 1900;
	if (start) {
		lt->tm_hour = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(meet->start_hh));
		lt->tm_min = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(meet->start_mm));
		lt->tm_sec = 0;
	} else {
		lt->tm_hour = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(meet->end_hh));
		lt->tm_min = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(meet->end_mm));
		lt->tm_sec = 0;
	}
	
	debug_print("%d %d %d, %d:%d\n", lt->tm_mday, lt->tm_mon, lt->tm_year, lt->tm_hour, lt->tm_min);
	t = mktime(lt);
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

static gboolean meeting_key_pressed(GtkWidget *widget,
				    GdkEventKey *event,
				    gpointer data)
{
	VCalMeeting *meet = (VCalMeeting *)data;
	
	if (event && event->keyval == GDK_Escape) {
		gtk_widget_destroy(meet->window);
	}
	return FALSE;
}

static void meeting_start_changed(GtkWidget *widget, gpointer data)
{
	VCalMeeting *meet = (VCalMeeting *)data;
	struct tm *start_lt;
	struct tm *end_lt;
	time_t start_t, end_t;
	guint d, m, y;

	start_t = time(NULL);
	end_t = time(NULL);
	start_lt = localtime(&start_t);
	end_lt = localtime(&end_t);
	
	gtk_calendar_get_date(GTK_CALENDAR(meet->start_c), &y, &m, &d);
	start_lt->tm_mday = d; start_lt->tm_mon  = m; start_lt->tm_year = y - 1900;
	start_lt->tm_hour = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(meet->start_hh));
	start_lt->tm_min = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(meet->start_mm));
	start_lt->tm_sec = 0;

	start_t = mktime(start_lt);
	debug_print("start %s\n", ctime(&start_t));

	gtk_calendar_get_date(GTK_CALENDAR(meet->end_c), &y, &m, &d);
	end_lt->tm_mday = d; end_lt->tm_mon  = m; end_lt->tm_year = y - 1900;
	end_lt->tm_hour = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(meet->end_hh));
	end_lt->tm_min = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(meet->end_mm));
	end_lt->tm_sec = 0;

	end_t = mktime(end_lt);

	debug_print("end   %s\n", ctime(&end_t));
	
	if (end_t > start_t) {
		debug_print("ok\n");
		return;
	}
	end_t = start_t + 3600;
	end_lt = localtime(&end_t);
	debug_print("n %d %d %d, %d:%d\n", end_lt->tm_mday, end_lt->tm_mon, end_lt->tm_year, end_lt->tm_hour, end_lt->tm_min);

	gtk_calendar_select_day(GTK_CALENDAR(meet->end_c), end_lt->tm_mday);

	gtk_calendar_select_month(GTK_CALENDAR(meet->end_c),
				end_lt->tm_mon,
				end_lt->tm_year + 1900);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(meet->end_hh),
				end_lt->tm_hour);	
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(meet->end_mm),
				end_lt->tm_min);	
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

	generate_msgid(buf, 255);

	if (meet->uid) {
		uid 	= g_strdup(meet->uid);
	} else {
		uid 	= g_strdup(buf);
	}
	organizer	= get_organizer(meet);
	organizer_name	= get_organizer_name(meet);
	account		= account_find_from_address(organizer);
	dtstart		= get_date(meet, TRUE);
	dtend		= get_date(meet, FALSE);
	summary		= get_summary(meet);
	description	= get_description(meet);
	
	event = vcal_manager_new_event(uid, organizer, organizer_name, summary, description,
					dtstart, dtend, tzid, NULL, meet->method, 
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

	if (res) {
		gtk_widget_destroy(meet->window);
	} else {
		alertpanel_error(_("Could not send the meeting invitation.\n"
				   "Check the recipients."));
		return res;
	}
	if (!found_att) {
		Folder *folder = folder_find_from_name ("vCalendar", vcal_folder_get_class());
		if (folder)
			folder_item_scan(folder->inbox);
		vcalviewer_reload();
	}

	return res;
}


static VCalMeeting *vcal_meeting_create_real(VCalEvent *event, gboolean visible)
{
	VCalMeeting *meet = g_new0(VCalMeeting, 1);
	GtkTextBuffer *buffer = NULL;
	GtkWidget *date_hbox, *date_vbox, *save_hbox, *label, *vbox, *hbox;
	gchar *s = NULL;
	GtkObject *start_h_adj, *start_m_adj, *end_h_adj, *end_m_adj;
	int i = 0, decs = -1, dece = -1, num = 0;
	
	GList *accounts;
	
	start_h_adj = gtk_adjustment_new (0, 0, 23, 1, 10, 10);
	start_m_adj = gtk_adjustment_new (0, 0, 59, 1, 10, 10);
	end_h_adj   = gtk_adjustment_new (0, 0, 23, 1, 10, 10);
	end_m_adj   = gtk_adjustment_new (0, 0, 59, 1, 10, 10);

	meet->window 		= gtk_window_new(GTK_WINDOW_TOPLEVEL);
	meet->table  		= gtk_table_new(7, 2, FALSE);
	meet->who    		= gtk_combo_box_new_text();
	
	meet->start_c		= gtk_calendar_new();
	meet->end_c		= gtk_calendar_new();

	meet->start_hh		= gtk_spin_button_new
		(GTK_ADJUSTMENT (start_h_adj), 1, 0);
	meet->start_mm		= gtk_spin_button_new
		(GTK_ADJUSTMENT (start_m_adj), 1, 0);
	meet->end_hh		= gtk_spin_button_new
		(GTK_ADJUSTMENT (end_h_adj), 1, 0);
	meet->end_mm		= gtk_spin_button_new
		(GTK_ADJUSTMENT (end_m_adj), 1, 0);
	
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (meet->start_hh), TRUE);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (meet->start_mm), TRUE);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (meet->end_hh), TRUE);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (meet->end_mm), TRUE);

	meet->summary		= gtk_entry_new();
	meet->description	= gtk_text_view_new();
        buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(meet->description));
        gtk_text_view_set_editable(GTK_TEXT_VIEW(meet->description), TRUE);
        gtk_text_buffer_add_selection_clipboard(buffer, gtk_clipboard_get(GDK_SELECTION_PRIMARY));
	
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

	g_signal_connect(G_OBJECT(meet->save_btn), "clicked",
			 G_CALLBACK(send_meeting_cb), meet);

	g_signal_connect(G_OBJECT(meet->window), "destroy",
			 G_CALLBACK(destroy_meeting_cb), meet);
	g_signal_connect(G_OBJECT(meet->window), "key_press_event",
			 G_CALLBACK(meeting_key_pressed), meet);
	
	g_signal_connect(G_OBJECT(meet->start_hh), "value-changed",
			 G_CALLBACK(meeting_start_changed), meet);

	g_signal_connect(G_OBJECT(meet->start_mm), "value-changed",
			 G_CALLBACK(meeting_start_changed), meet);

	g_signal_connect(G_OBJECT(meet->start_c), "day-selected",
			 G_CALLBACK(meeting_start_changed), meet);


	gtk_widget_set_size_request(meet->description, -1, 100);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(meet->description), GTK_WRAP_WORD);

	if (get_curdate(HOUR) + 2 > 23)
		dece = 0;
	else if (get_curdate(HOUR) + 1 > 23)
		decs = dece = 0;
	
	if (!event) {
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(meet->start_hh),
			(get_curdate(HOUR)+1)%24);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(meet->end_hh),
			(get_curdate(HOUR)+2)%24);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(meet->start_mm),
			0);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(meet->end_mm),
			0);
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

		gtk_spin_button_set_value(GTK_SPIN_BUTTON(meet->start_hh),
			get_dtdate(event->dtstart, HOUR));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(meet->end_hh),
			get_dtdate(event->dtend, HOUR));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(meet->start_mm),
			get_dtdate(event->dtstart, MINUTE));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(meet->end_mm),
			get_dtdate(event->dtend, MINUTE));
	}
	gtk_widget_set_size_request(meet->start_mm, 40, -1);
	gtk_widget_set_size_request(meet->start_hh, 40, -1);
	gtk_widget_set_size_request(meet->end_mm, 40, -1);
	gtk_widget_set_size_request(meet->end_hh, 40, -1);
	
	date_hbox = gtk_hbox_new(FALSE, 6);
	date_vbox = gtk_vbox_new(FALSE, 6);
	hbox = gtk_hbox_new(FALSE, 6);
	label = gtk_label_new(_("<b>Starts at:</b> ")); gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), meet->start_hh, FALSE, FALSE, 0);
	label = gtk_label_new(_("h")); gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), meet->start_mm, FALSE, FALSE, 0);
	label = gtk_label_new(_("<b> on:</b>")); gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(date_vbox), hbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(date_vbox), meet->start_c, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(date_hbox), date_vbox, FALSE, FALSE, 0);

	label = gtk_label_new(" "); gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(date_hbox), label, TRUE, TRUE, 0);

	date_vbox = gtk_vbox_new(FALSE, 6);
	hbox = gtk_hbox_new(FALSE, 6);
	label = gtk_label_new(_("<b>Ends at:</b> ")); gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), meet->end_hh, FALSE, FALSE, 0);
	label = gtk_label_new(_("h")); gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), meet->end_mm, FALSE, FALSE, 0);
	label = gtk_label_new(_("<b> on:</b>")); gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(date_vbox), hbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(date_vbox), meet->end_c, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(date_hbox), date_vbox, FALSE, FALSE, 0);

	meet->attendees_vbox = gtk_vbox_new(FALSE, 6);
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
	TABLE_ADD_LINE(_("Organizer:"), meet->who);
	TABLE_ADD_LINE(_("Summary:"), meet->summary);
	TABLE_ADD_LINE(_("Time:"), date_hbox);
	TABLE_ADD_LINE(_("Description:"), meet->description);
	TABLE_ADD_LINE(_("Attendees:"), meet->attendees_vbox);
	TABLE_ADD_LINE("", save_hbox);
	
	gtk_widget_set_size_request(meet->window, -1, -1);
	gtk_container_add(GTK_CONTAINER(meet->window), meet->table);
	if (visible)
		gtk_widget_show_all(meet->window);
	
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
			vcal_manager_save_event(event);
		}
		
		vcal_manager_free_event((VCalEvent *)cur->data);
	}
	
	g_slist_free(events);

	return TRUE;
}

void multisync_export(void)
{
	GSList *list = vcal_folder_get_waiting_events();
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
		return;
	}
		
	for (cur = list; cur; cur = cur->next) {
		VCalEvent *event = (VCalEvent *)cur->data;
		file = g_strdup_printf("multisync%lu-%d", time(NULL), i);

		i++;

		calendar = 
        		icalcomponent_vanew(
        		    ICAL_VCALENDAR_COMPONENT,
	        	    icalproperty_new_version("2.0"),
        		    icalproperty_new_prodid(
                		 "-//Sylpheed-Claws//NONSGML Sylpheed-Claws Calendar//EN"),
			    icalproperty_new_calscale("GREGORIAN"),
        		    0
        	    ); 	
		vcal_manager_event_dump(event, FALSE, FALSE, calendar);
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
	g_slist_free(files);
}

gboolean vcal_meeting_export_calendar(const gchar *path, gboolean automatic)
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
		if (path == NULL) {
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
                	 "-//Sylpheed-Claws//NONSGML Sylpheed-Claws Calendar//EN"),
		    icalproperty_new_calscale("GREGORIAN"),
        	    0
            ); 	

	for (cur = list; cur; cur = cur->next) {
		VCalEvent *event = (VCalEvent *)cur->data;
		vcal_manager_event_dump(event, FALSE, FALSE, calendar);
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
	g_slist_free(list);
	g_slist_free(subs);
	
putfile:
	if (!path && !automatic)
		file = filesel_select_file_save(_("Export calendar to ICS"), NULL);
	else
		file = g_strdup(path);

	if (automatic && (!path || strlen(path) == 0))
		return TRUE;

	if (file 
	&& strncmp(file, "http://", 7) 
	&& strncmp(file, "https://", 8)
	&& strncmp(file, "webdav://", 9)
	&& strncmp(file, "ftp://", 6)) {
		if (move_file(tmpfile, file, TRUE) != 0)
			res = FALSE;
		g_free(file);
	} else if (file) {
		FILE *fp = fopen(tmpfile, "rb");
		if (!strncmp(file, "webdav://", 9)) {
			gchar *tmp = g_strdup_printf("http://%s", file+9);
			g_free(file);
			file = tmp;
		}
		if (fp) {
			CURL *curl_ctx = curl_easy_init();
			int response_code = 0;
			struct curl_slist * headers = curl_slist_append(NULL, 
				"Content-Type: text/calendar; charset=\"utf-8\"" );


			curl_easy_setopt(curl_ctx, CURLOPT_URL, file);
			curl_easy_setopt(curl_ctx, CURLOPT_UPLOAD, 1);
			curl_easy_setopt(curl_ctx, CURLOPT_READDATA, fp);
			curl_easy_setopt(curl_ctx, CURLOPT_HTTPHEADER, headers);
#if LIBCURL_VERSION_NUM >= 0x070a00
			curl_easy_setopt(curl_ctx, CURLOPT_SSL_VERIFYPEER, 0);
			curl_easy_setopt(curl_ctx, CURLOPT_SSL_VERIFYHOST, 0);
#endif
			curl_easy_setopt(curl_ctx, CURLOPT_USERAGENT, 
				"Sylpheed-Claws vCalendar plugin "
				"(http://claws.sylpheed.org/plugins.php)");
			curl_easy_setopt(curl_ctx, CURLOPT_INFILESIZE, filesize);
			curl_easy_perform(curl_ctx);
			curl_easy_getinfo(curl_ctx, CURLINFO_RESPONSE_CODE, &response_code);
			if (response_code < 200 || response_code >= 300) {
				g_warning("Can't export calendar, got code %d\n", response_code);
				res = FALSE;
			}
			curl_easy_cleanup(curl_ctx);
			curl_slist_free_all(headers);
			fclose(fp);
		}
	}
	g_free(tmpfile);
	return res;
}

/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2004 Hiroyuki Yamamoto & the Sylpheed-Claws team
 * This file (C) 2004 Colin Leroy <colin@colino.net>
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

#include <ical.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "intl.h"
#include "mimeview.h"
#include "utils.h"
#include "icaltime_as_local.h"
#include "vcalendar.h"
#include "vcal_manager.h"
#include "vcal_folder.h"
#include "vcal_meeting_gtk.h"
#include "prefs_account.h"
#include "account.h"
#include "codeconv.h"
#include "xml.h"
#include "xmlprops.h"
#include "alertpanel.h"
#include "menu.h"

MimeViewerFactory vcal_viewer_factory;

struct _VCalViewer
{
	MimeViewer mimeviewer;

	gchar	  *file;
	MimeInfo  *mimeinfo;
	gchar 	  *tmpfile;
	icalcomponent *comp;
	icalcomponent *icomp;
	
	GtkWidget *scrolledwin;
	GtkWidget *table;
	GtkWidget *type;
	GtkWidget *who;
	GtkWidget *start;
	GtkWidget *end;
	GtkWidget *summary;
	GtkWidget *description;
	GtkWidget *answer;
	GtkWidget *button;
	GtkWidget *reedit;
	GtkWidget *cancel;
	GtkWidget *attendees;
};

static gchar *get_tmpfile(VCalViewer *vcalviewer)
{
	gchar *tmpfile = NULL;
	
	if (!vcalviewer->tmpfile) {
		tmpfile = procmime_get_tmp_file_name(vcalviewer->mimeinfo);
		debug_print("creating %s\n", tmpfile);

		if (procmime_get_part(tmpfile, vcalviewer->mimeinfo) < 0) {
			g_warning("Can't get mimepart file");	
			g_free(tmpfile);
			return NULL;
		}
		vcalviewer->tmpfile = tmpfile;
	}
	
	return vcalviewer->tmpfile;
}

static GtkWidget *vcal_viewer_get_widget(MimeViewer *_mimeviewer)
{
	VCalViewer *vcalviewer = (VCalViewer *) _mimeviewer;

	debug_print("vcal_viewer_get_widget\n");
	
	return vcalviewer->scrolledwin;
}

static void vcal_viewer_clear_viewer(MimeViewer *_mimeviewer)
{
	VCalViewer *vcalviewer = (VCalViewer *) _mimeviewer;

	debug_print("vcal_viewer_clear_viewer\n");

	g_free(vcalviewer->file);
	vcalviewer->file = NULL;
	if (vcalviewer->tmpfile) {
		debug_print("unlinking %s\n", vcalviewer->tmpfile);
		unlink(vcalviewer->tmpfile);
		g_free(vcalviewer->tmpfile);
		vcalviewer->tmpfile = NULL;
	}
	vcalviewer->mimeinfo = NULL;
}

static gchar *get_email_from_attendee_property(icalproperty *p)
{
	gchar *tmp = NULL;
	gchar *email = NULL;
	
	if (p)
		tmp = g_strdup(icalproperty_get_attendee(p));

	if (!tmp) 
		return NULL;
		
	if (!strncasecmp(tmp, "MAILTO:", strlen("MAILTO:")))
		email = g_strdup(tmp+strlen("MAILTO:"));
	else
		email = g_strdup(tmp);
	g_free(tmp);
	
	return email;
}

static gchar *get_email_from_organizer_property(icalproperty *p)
{
	gchar *tmp = NULL;
	gchar *email = NULL;
	
	if (p)
		tmp = g_strdup(icalproperty_get_organizer(p));

	if (!tmp) 
		return NULL;
		
	if (!strncasecmp(tmp, "MAILTO:", strlen("MAILTO:")))
		email = g_strdup(tmp+strlen("MAILTO:"));
	else
		email = g_strdup(tmp);
	g_free(tmp);
	
	return email;
}

static icalproperty *vcalviewer_get_property(VCalViewer *vcalviewer, icalproperty_kind kind)
{
	icalproperty *iprop = NULL;
	
	if (!vcalviewer->icomp || !vcalviewer->comp)
		return NULL;

	iprop = icalcomponent_get_first_property(vcalviewer->icomp, kind);
	if (!iprop)
		iprop = icalcomponent_get_first_property(vcalviewer->comp, kind);
	
	return iprop;
}

static GSList *vcalviewer_get_properties(VCalViewer *vcalviewer, icalproperty_kind kind)
{
	GSList *list = NULL;
	icalproperty *iprop = NULL;
	
	if (!vcalviewer->icomp || !vcalviewer->comp)
		return NULL;

	iprop = icalcomponent_get_first_property(vcalviewer->icomp, kind);
	if (!iprop) {
		return NULL;
	} else do {
		list = g_slist_append(list, iprop);
	} while ((iprop = icalcomponent_get_next_property(vcalviewer->icomp, kind)) != NULL);
	
	return list;
}

static icalcomponent *vcalviewer_get_component(const gchar *file)
{
	gchar *compstr = NULL;
	icalcomponent *comp = NULL;
	
	compstr = file_read_to_str(file);
	if (compstr) {
		comp = icalcomponent_new_from_string(compstr);
		g_free(compstr);
	}
	
	return comp;
}

static void vcalviewer_reset(VCalViewer *vcalviewer) 
{
	gtk_label_set_text(GTK_LABEL(vcalviewer->type), "-");
	gtk_label_set_text(GTK_LABEL(vcalviewer->who), "-");
	gtk_label_set_text(GTK_LABEL(vcalviewer->summary), "-");
	gtk_label_set_text(GTK_LABEL(vcalviewer->description), "-");
	gtk_label_set_text(GTK_LABEL(vcalviewer->start), "-");
	gtk_label_set_text(GTK_LABEL(vcalviewer->end), "-");
	gtk_label_set_text(GTK_LABEL(vcalviewer->attendees), "-");
}

static gchar *get_attendee_replying(VCalViewer *vcalviewer)
{
	gchar *attendee = NULL;
	icalproperty *p = NULL;
	
	p = vcalviewer_get_property(vcalviewer, ICAL_ATTENDEE_PROPERTY);
	if (!p)
		return NULL;

	attendee = get_email_from_attendee_property(p);

	icalproperty_free(p);
	
	return attendee;
	
}

static gchar *get_attendee_replying_name(VCalViewer *vcalviewer)
{
	gchar *name = NULL;
	icalproperty *p = NULL;
	icalparameter *param = NULL;
	
	p = vcalviewer_get_property(vcalviewer, ICAL_ATTENDEE_PROPERTY);
	if (!p)
		return NULL;

	param = icalproperty_get_first_parameter(p, ICAL_CN_PARAMETER);
	if (param)
		name = g_strdup(icalparameter_get_cn(param));
		
	icalproperty_free(p);
	
	return name;
	
}

enum icalparameter_partstat get_attendee_reply(VCalViewer *vcalviewer)
{
	icalproperty *p = NULL;
	icalparameter *param = NULL;
	enum icalparameter_partstat status = 0;
	
	p = vcalviewer_get_property(vcalviewer, ICAL_ATTENDEE_PROPERTY);
	if (!p)
		return 0;
	
	param = icalproperty_get_first_parameter(p, ICAL_PARTSTAT_PARAMETER);
	
	if (param) {
		status = icalparameter_get_partstat(param);
		icalproperty_free(p);
	}
	
	return status;
}

enum icalparameter_partstat get_attendee_replying_cutype(VCalViewer *vcalviewer)
{
	icalproperty *p = NULL;
	icalparameter *param = NULL;
	enum icalparameter_cutype status = 0;
	
	p = vcalviewer_get_property(vcalviewer, ICAL_ATTENDEE_PROPERTY);
	if (!p)
		return 0;
	
	param = icalproperty_get_first_parameter(p, ICAL_CUTYPE_PARAMETER);
	
	if (param) {
		status = icalparameter_get_cutype(param);
		icalproperty_free(p);
	}
	
	return status;
}

/*FIXME*/
static void vcalviewer_get_attendees(VCalViewer *vcalviewer, VCalEvent *event)
{
	GSList *attendees = vcalviewer_get_properties(vcalviewer, ICAL_ATTENDEE_PROPERTY);
	GSList *cur = attendees;
	while (cur && cur->data) {
		icalproperty *iprop = (icalproperty *)cur->data;
		gchar *email = get_email_from_attendee_property(iprop);
		const gchar *name  = NULL;
		icalparameter *param = NULL;
		icalparameter *cutype = NULL;
		param = icalproperty_get_first_parameter(iprop, ICAL_CN_PARAMETER);
		if (param)
			name = icalparameter_get_cn(param);
		
		param = icalproperty_get_first_parameter(iprop, ICAL_PARTSTAT_PARAMETER);
		cutype= icalproperty_get_first_parameter(iprop, ICAL_CUTYPE_PARAMETER);
		
		if (email && param) {
			debug_print("updating %s: %d\n", email, icalparameter_get_partstat(param));
			vcal_manager_update_answer(event, email, name,
				icalparameter_get_partstat(param),
				cutype?icalparameter_get_cutype(cutype):ICAL_CUTYPE_INDIVIDUAL);
		} else if (email) {
			debug_print("updating %s: %d\n", email, ICAL_PARTSTAT_NEEDSACTION);
			vcal_manager_update_answer(event, email, name,
				ICAL_PARTSTAT_NEEDSACTION,
				cutype?icalparameter_get_cutype(cutype):ICAL_CUTYPE_INDIVIDUAL);
		}
		g_free(email);
		icalproperty_free(iprop);
		
		cur = cur->next;
	}
	g_slist_free(cur);
}


static PrefsAccount *get_account_from_attendee(icalproperty *attendee)
{
	gchar *email = get_email_from_attendee_property(attendee);
	if (email != NULL && account_find_from_address(email) != NULL)
		return account_find_from_address(email);

	return NULL;
}

static PrefsAccount *get_account_from_attendees(VCalViewer *vcalviewer)
{
	GSList *attendees = vcalviewer_get_properties(vcalviewer, ICAL_ATTENDEE_PROPERTY);
	GSList *cur = attendees;
	PrefsAccount *account = NULL;
	while (cur && cur->data) {
		icalproperty *iprop = (icalproperty *)cur->data;
		
		if (get_account_from_attendee(iprop)) {
			account = get_account_from_attendee(iprop);
			icalproperty_free(iprop);
			break;
		}

		icalproperty_free(iprop);
		cur = cur->next;
	}
	g_slist_free(cur);
	return account;
}

static void vcalviewer_answer_set_choices(VCalViewer *vcalviewer, VCalEvent *event, enum icalproperty_method method)
{
	GtkWidget *menu, *menu_item;

	gtk_widget_hide(vcalviewer->reedit);
	gtk_widget_hide(vcalviewer->cancel);
	gtk_widget_hide(vcalviewer->answer);
	gtk_widget_hide(vcalviewer->button);

	menu = gtk_menu_new();
		
	if (method == ICAL_METHOD_REQUEST) {
		MENUITEM_ADD(menu, menu_item, _("Accept"), 1);
		MENUITEM_ADD(menu, menu_item, _("Tentatively accept"), 2);
		MENUITEM_ADD(menu, menu_item, _("Decline"), 3);
		
		gtk_option_menu_set_menu (GTK_OPTION_MENU (vcalviewer->answer), menu);
		
		gtk_widget_set_sensitive(vcalviewer->answer, TRUE);
		gtk_widget_set_sensitive(vcalviewer->button, TRUE);
		gtk_widget_show_all(vcalviewer->answer);
		gtk_widget_show(vcalviewer->button);
	} else {
		MENUITEM_ADD(menu, menu_item, _("-"), 0);
		gtk_option_menu_set_menu (GTK_OPTION_MENU (vcalviewer->answer), menu);
		gtk_widget_set_sensitive(vcalviewer->answer, FALSE);
		gtk_widget_set_sensitive(vcalviewer->button, FALSE);
	}
	
	if (event && event->method == ICAL_METHOD_REQUEST) {
		PrefsAccount *account = vcal_manager_get_account_from_event(event);
		if (account) {
			enum icalparameter_partstat answer = 
				vcal_manager_get_reply_for_attendee(event, account->address);
				
			if (answer == ICAL_PARTSTAT_ACCEPTED)
				gtk_option_menu_set_history(GTK_OPTION_MENU(vcalviewer->answer), 0);
			if (answer == ICAL_PARTSTAT_TENTATIVE)
				gtk_option_menu_set_history(GTK_OPTION_MENU(vcalviewer->answer), 1);
			if (answer == ICAL_PARTSTAT_DECLINED)
				gtk_option_menu_set_history(GTK_OPTION_MENU(vcalviewer->answer), 2);
		}
	}
}

void vcalviewer_display_event (VCalViewer *vcalviewer, VCalEvent *event)
{
	GSList *list = NULL;
	gchar *attendees = NULL;
	gboolean firstatt = TRUE;
	gboolean mine = FALSE;
	
	if (!event)
		return;
	if (!vcalviewer)
		return;

	if (event->method == ICAL_METHOD_REQUEST) {
		if (account_find_from_address(event->organizer)) {
			gtk_label_set_text(GTK_LABEL(vcalviewer->type), 
				_("You have created a meeting. Details follow:"));
			mine = TRUE;
		} else 
			gtk_label_set_text(GTK_LABEL(vcalviewer->type), 
				_("You have been invited to a meeting. Details follow:"));
	}
	else if (event->method == ICAL_METHOD_REPLY) {
		gchar *label = NULL;
		enum icalparameter_partstat answer = get_attendee_reply(vcalviewer);
		enum icalparameter_cutype cutype = get_attendee_replying_cutype(vcalviewer);
		gchar *attendee = get_attendee_replying(vcalviewer);
		gchar *name = get_attendee_replying_name(vcalviewer);
		
		vcal_manager_update_answer(event, attendee, name, answer, cutype);

		if (!attendee) {
			label = g_strdup_printf(
				_("You have received an answer to an unknown meeting proposal. Details follow:"));
		} else {
			label = g_strdup_printf(_("You have received an answer to a meeting proposal.\n"
				"%s has %s the invitation whose details follow:"),
				attendee, vcal_manager_get_reply_text_for_attendee(event, attendee));
			gtk_label_set_text(GTK_LABEL(vcalviewer->type), label);
			g_free(attendee);
		}
			
		g_free(label);
	} else if (event->method == ICAL_METHOD_CANCEL) {
		Folder *folder = NULL;
		gtk_label_set_text(GTK_LABEL(vcalviewer->type), 
			_("A meeting to which you had been invited has been cancelled. Details follow:"));
		vcal_manager_save_event(event);
		folder = folder_find_from_name ("vCalendar", vcal_folder_get_class());
		if (folder)
			folder_item_scan(folder->inbox);
	} else 
		gtk_label_set_text(GTK_LABEL(vcalviewer->type), 
			_("You have been forwarded an appointment. Details follow:"));

	
	if (event->organizer && strlen(event->organizer)) {
		gtk_label_set_text(GTK_LABEL(vcalviewer->who), event->organizer);
	} else
		gtk_label_set_text(GTK_LABEL(vcalviewer->who), "-");
	
	if (event->summary && strlen(event->summary)) {
		gtk_label_set_text(GTK_LABEL(vcalviewer->summary), event->summary);
	} else
		gtk_label_set_text(GTK_LABEL(vcalviewer->summary), "-");
	
	if (event->description && strlen(event->description)) {
		gtk_label_set_text(GTK_LABEL(vcalviewer->description), event->description);
	} else
		gtk_label_set_text(GTK_LABEL(vcalviewer->description), "-");
	
	if (event->start && strlen(event->start)) {
		gtk_label_set_text(GTK_LABEL(vcalviewer->start), event->start);
	} else
		gtk_label_set_text(GTK_LABEL(vcalviewer->start), "-");
	
	if (event->end && strlen(event->end)) {
		gtk_label_set_text(GTK_LABEL(vcalviewer->end), event->end);
	} else
		gtk_label_set_text(GTK_LABEL(vcalviewer->end), "-");
	
	for (list = vcal_manager_get_answers_emails(event); 
	     list && list->data; list = list->next) {
	     	gchar *attendee = (gchar *)list->data;
		gchar *name = vcal_manager_get_attendee_name(event, attendee);
		gchar *answer = vcal_manager_get_reply_text_for_attendee(event, attendee);
		gchar *type = vcal_manager_get_cutype_text_for_attendee(event, attendee);
		if (firstatt) {
			attendees = g_strdup_printf("%s%s<%s> (%s, %s)", 
					name?name:"",
					name?" ":"",
					attendee, type, answer);
			firstatt = FALSE;
		} else {
			gchar *tmp = g_strdup(attendees);
			g_free(attendees);
			attendees = g_strdup_printf("%s\n%s%s<%s> (%s, %s)", 
					tmp, name?name:"",
					name?" ":"", attendee, type, answer);
			g_free(tmp);
		}
		g_free(answer);
		g_free(type);
		g_free(name);
	}

	if (attendees && strlen(attendees))
		gtk_label_set_text(GTK_LABEL(vcalviewer->attendees), attendees);
	else
		gtk_label_set_text(GTK_LABEL(vcalviewer->attendees), "-");
	
	if (!mine)
		vcalviewer_answer_set_choices(vcalviewer, event, event->method);
	else {
		vcalviewer_answer_set_choices(vcalviewer, event, ICAL_METHOD_REPLY);
		gtk_widget_show(vcalviewer->reedit);
		gtk_widget_show(vcalviewer->cancel);
	}
}

gchar *vcalviewer_get_uid_from_mimeinfo(MimeInfo *mimeinfo)
{
	gchar *tmpfile = procmime_get_tmp_file_name(mimeinfo);
	gchar *compstr = NULL;
	icalcomponent *comp = NULL, *icomp = NULL;
	icalproperty *iprop = NULL;
	gchar *res = NULL;

	if (procmime_get_part(tmpfile, mimeinfo) < 0) {
		g_warning("Can't get mimepart file");	
		g_free(tmpfile);
		return NULL;
	}
	
	compstr = file_read_to_str(tmpfile);
	unlink(tmpfile);
	g_free(tmpfile);
	comp = icalcomponent_new_from_string(compstr);
	g_free(compstr);
	if (!comp) {
		g_warning("can't get component");
		return NULL;
	}
	icomp = icalcomponent_get_inner(comp);
	if (!icomp) {
		g_warning("can't get inner component");
		icalcomponent_free(comp);
		return NULL;
	}
	iprop = icalcomponent_get_first_property(icomp, ICAL_UID_PROPERTY);

	if (iprop) {
		res = g_strdup(icalproperty_get_uid(iprop));
		icalproperty_free(iprop);
	} else {
		g_warning("can't get property");
	}

	icalcomponent_free(comp);
	
	debug_print("got uid: %s\n", res);
	return res;

}

static void vcalviewer_get_request_values(VCalViewer *vcalviewer, MimeInfo *mimeinfo) 
{
	icalproperty *iprop = NULL;
	gchar *org = NULL, *start = NULL, *end = NULL, *summary = NULL, *description = NULL;
	gchar *dtstart = NULL, *dtend = NULL, *tzid = NULL;
	enum icalproperty_method method = ICAL_METHOD_REQUEST;
	VCalEvent *event = NULL;
	gchar *tmp;
	const gchar *charset = procmime_mimeinfo_get_parameter(mimeinfo, "charset");
	gchar * uid = vcalviewer_get_uid_from_mimeinfo(mimeinfo);
	gint sequence = 0;
	
	if (!charset)
		charset=CS_ISO_8859_1;
		
	iprop = vcalviewer_get_property(vcalviewer, ICAL_SEQUENCE_PROPERTY);
	if (iprop) {
		sequence = icalproperty_get_sequence(iprop);
		icalproperty_free(iprop);
	}  

	iprop = vcalviewer_get_property(vcalviewer, ICAL_METHOD_PROPERTY);
	if (iprop) {
		method = icalproperty_get_method(iprop);
		icalproperty_free(iprop);
	}
	
	/* see if we have it registered and more recent */
	event = vcal_manager_load_event(uid);
	if (event && event->sequence >= sequence) {
		event->method = method;
		vcalviewer_display_event(vcalviewer, event);
		vcal_manager_free_event(event);
		g_free(uid);
		return;
	}
	
	/* load it and register it */

	iprop = vcalviewer_get_property(vcalviewer, ICAL_ORGANIZER_PROPERTY);
	if (iprop) {
		tmp = get_email_from_organizer_property(iprop);
		org = conv_codeset_strdup(tmp, charset, CS_UTF_8);
		g_free(tmp);
		icalproperty_free(iprop);
	} 
	
	iprop = vcalviewer_get_property(vcalviewer, ICAL_SUMMARY_PROPERTY);
	if (iprop) {
		summary = conv_codeset_strdup(icalproperty_get_summary(iprop), charset, CS_UTF_8);
		icalproperty_free(iprop);
	} 

	iprop = vcalviewer_get_property(vcalviewer, ICAL_DESCRIPTION_PROPERTY);
	if (iprop) {
		description = conv_codeset_strdup(icalproperty_get_description(iprop), charset, CS_UTF_8);
		icalproperty_free(iprop);
	} 
	
	iprop = vcalviewer_get_property(vcalviewer, ICAL_DTSTART_PROPERTY);
	if (iprop) {
		struct icaltimetype itt = icaltime_as_local(icalproperty_get_dtstart(iprop));
		start = g_strdup(icaltime_as_ctime(itt));
		dtstart = g_strdup(icaltime_as_ical_string(itt));
		icalproperty_free(iprop);
	} 

	iprop = vcalviewer_get_property(vcalviewer, ICAL_DTEND_PROPERTY);
	if (iprop) {
		struct icaltimetype itt = icaltime_as_local(icalproperty_get_dtend(iprop));
		end = g_strdup(icaltime_as_ctime(itt));
		dtend = g_strdup(icaltime_as_ical_string(itt));
		icalproperty_free(iprop);
	} 
	
	iprop = vcalviewer_get_property(vcalviewer, ICAL_TZID_PROPERTY);
	if (iprop) {
		tzid = g_strdup(icalproperty_get_tzid(iprop));
		icalproperty_free(iprop);
	} 
	
	iprop = vcalviewer_get_property(vcalviewer, ICAL_METHOD_PROPERTY);
	if (iprop) {
		method = icalproperty_get_method(iprop);
		icalproperty_free(iprop);
	}
	
	event = vcal_manager_new_event( uid,
					org, start, end, summary, description,
					dtstart, dtend, tzid, method, sequence);
	vcalviewer_get_attendees(vcalviewer, event);
	vcal_manager_save_event(event);

	g_free(org); 
	g_free(start);
	g_free(end);
	g_free(summary);
	g_free(description);
	g_free(uid);
	vcalviewer_display_event(vcalviewer, event);
	vcal_manager_free_event(event);
}

static void vcalviewer_get_reply_values(VCalViewer *vcalviewer, MimeInfo *mimeinfo) 
{
	icalproperty *iprop = NULL;
	gchar *tmp = NULL;
	const gchar *charset = procmime_mimeinfo_get_parameter(mimeinfo, "charset");
	if (!charset)
		charset=CS_ISO_8859_1;

	iprop = vcalviewer_get_property(vcalviewer, ICAL_UID_PROPERTY);
	if (iprop) {
		VCalEvent *event = vcal_manager_load_event(icalproperty_get_uid(iprop));
		if (event) {
			event->method = ICAL_METHOD_REPLY;
			vcalviewer_display_event(vcalviewer, event);
			vcal_manager_free_event(event);
			return;
		} 
	} 
	
	/* unknown answer */
	gtk_label_set_text(GTK_LABEL(vcalviewer->type), 
		_("You have received an answer to an unknown meeting proposal. Details follow:"));
	iprop = vcalviewer_get_property(vcalviewer, ICAL_ORGANIZER_PROPERTY);
	if (iprop) {
		gchar *org;
		tmp = get_email_from_organizer_property(iprop);
		org = conv_codeset_strdup(tmp, charset, CS_UTF_8);
		g_free(tmp);
		gtk_label_set_text(GTK_LABEL(vcalviewer->who), org);
		icalproperty_free(iprop);
		g_free(org);
	} else
		gtk_label_set_text(GTK_LABEL(vcalviewer->who), "-");

	iprop = vcalviewer_get_property(vcalviewer, ICAL_SUMMARY_PROPERTY);
	if (iprop) {
		tmp = conv_codeset_strdup(icalproperty_get_summary(iprop), charset, CS_UTF_8);
		gtk_label_set_text(GTK_LABEL(vcalviewer->summary), tmp);
		g_free(tmp);
		icalproperty_free(iprop);
	} else
		gtk_label_set_text(GTK_LABEL(vcalviewer->summary), "-");

	iprop = vcalviewer_get_property(vcalviewer, ICAL_DESCRIPTION_PROPERTY);
	if (iprop) {
		tmp = conv_codeset_strdup(icalproperty_get_description(iprop), charset, CS_UTF_8);
		gtk_label_set_text(GTK_LABEL(vcalviewer->description), tmp);
		g_free(tmp);
		icalproperty_free(iprop);
	} else
		gtk_label_set_text(GTK_LABEL(vcalviewer->description), "-");

	iprop = vcalviewer_get_property(vcalviewer, ICAL_DTSTART_PROPERTY);
	if (iprop) {
		struct icaltimetype itt = icaltime_as_local(icalproperty_get_dtstart(iprop));
		gtk_label_set_text(GTK_LABEL(vcalviewer->start), 
			icaltime_as_ctime(itt));
		icalproperty_free(iprop);
	} else
		gtk_label_set_text(GTK_LABEL(vcalviewer->start), "-");

	iprop = vcalviewer_get_property(vcalviewer, ICAL_DTEND_PROPERTY);
	if (iprop) {
		struct icaltimetype itt = icaltime_as_local(icalproperty_get_dtstart(iprop));
		gtk_label_set_text(GTK_LABEL(vcalviewer->end), 
			icaltime_as_ctime(itt));
		icalproperty_free(iprop);
	} else
		gtk_label_set_text(GTK_LABEL(vcalviewer->end), "-");
	
	gtk_label_set_text(GTK_LABEL(vcalviewer->attendees), "-"); 

	vcalviewer_answer_set_choices(vcalviewer, NULL, ICAL_METHOD_REPLY);
}

static void vcalviewer_get_info(VCalViewer *vcalviewer, MimeInfo *mimeinfo) 
{
	icalproperty *method = NULL;
	
	gchar *tmpfile = get_tmpfile(vcalviewer);
	
	if (vcalviewer->comp) {
		icalcomponent_free(vcalviewer->comp);
		vcalviewer->comp = NULL;
		vcalviewer->icomp = NULL;
	}
	
	if (!tmpfile) {
		vcalviewer_reset(vcalviewer);
		return;
	}

	vcalviewer->comp = vcalviewer_get_component(tmpfile);
	if (vcalviewer->comp)
		vcalviewer->icomp= icalcomponent_get_inner(vcalviewer->comp);
	else {
		vcalviewer_reset(vcalviewer);
		return;
	}
	
	method = vcalviewer_get_property(vcalviewer, ICAL_METHOD_PROPERTY);
	
	if (icalcomponent_isa(vcalviewer->comp) == ICAL_VCALENDAR_COMPONENT) {
		if (icalproperty_get_method(method) == ICAL_METHOD_REQUEST
		||  icalproperty_get_method(method) == ICAL_METHOD_PUBLISH
		||  icalproperty_get_method(method) == ICAL_METHOD_CANCEL)
			vcalviewer_get_request_values(vcalviewer, mimeinfo);
		else if (icalproperty_get_method(method) == ICAL_METHOD_REPLY)
			vcalviewer_get_reply_values(vcalviewer, mimeinfo);
		else
			vcalviewer_reset(vcalviewer);
	} else
		vcalviewer_reset(vcalviewer);
}

static VCalViewer *s_vcalviewer = NULL;

static void vcal_viewer_show_mimepart(MimeViewer *_mimeviewer, const gchar *file, MimeInfo *mimeinfo)
{
	VCalViewer *vcalviewer = (VCalViewer *) _mimeviewer;
	s_vcalviewer = vcalviewer;
	
	debug_print("vcal_viewer_show_mimepart : %s\n", file);

	vcal_viewer_clear_viewer(_mimeviewer);
	g_free(vcalviewer->file);
	vcalviewer->file = g_strdup(file);
	vcalviewer->mimeinfo = mimeinfo;
	vcalviewer_get_info(vcalviewer, mimeinfo);
}

void vcalviewer_reload(void)
{
	if (s_vcalviewer)
		vcal_viewer_show_mimepart((MimeViewer *)s_vcalviewer, NULL, s_vcalviewer->mimeinfo);
}

static void vcal_viewer_destroy_viewer(MimeViewer *_mimeviewer)
{
	VCalViewer *vcalviewer = (VCalViewer *) _mimeviewer;

	debug_print("vcal_viewer_destroy_viewer\n");

	vcal_viewer_clear_viewer(_mimeviewer);
	g_free(vcalviewer);
}

static gboolean vcalviewer_reedit_cb(GtkButton *widget, gpointer data)
{
	VCalViewer *vcalviewer = (VCalViewer *)data;
	gchar * uid = vcalviewer_get_uid_from_mimeinfo(vcalviewer->mimeinfo);
	VCalEvent *event = NULL;

	/* see if we have it registered and more recent */
	event = vcal_manager_load_event(uid);
	vcal_meeting_create(event);
	vcal_manager_free_event(event);
	return TRUE;
}

static gboolean vcalviewer_cancel_cb(GtkButton *widget, gpointer data)
{
	VCalViewer *vcalviewer = (VCalViewer *)data;
	gchar * uid = vcalviewer_get_uid_from_mimeinfo(vcalviewer->mimeinfo);
	VCalEvent *event = NULL;
	VCalMeeting *meet = NULL;
	
	gint val = alertpanel(_("Cancel meeting"), 
			      _("Are you sure you want to cancel this meeting?\n"
			        "A notification will be sent to attendees."),
			      _("Yes"), _("+No"), NULL);
	if (val != G_ALERTDEFAULT)
		return TRUE;

	event = vcal_manager_load_event(uid);
	event->method = ICAL_METHOD_CANCEL;
	
	meet = vcal_meeting_create_hidden(event);
	vcal_meeting_send(meet);
	vcal_manager_save_event(event);
	vcal_manager_free_event(event);
	return TRUE;
}

static gboolean vcalviewer_action_cb(GtkButton *widget, gpointer data)
{
        VCalViewer *vcalviewer = (VCalViewer *)data;
	VCalEvent *event = NULL;
	icalproperty *iprop = NULL;
	gint index = 0;
	enum icalparameter_partstat reply[3] = {ICAL_PARTSTAT_ACCEPTED, ICAL_PARTSTAT_TENTATIVE, ICAL_PARTSTAT_DECLINED};
	PrefsAccount *account = NULL;
	GtkWidget *menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(vcalviewer->answer));
	index = GPOINTER_TO_INT(gtk_object_get_user_data(
                                   GTK_OBJECT(GTK_MENU_ITEM(gtk_menu_get_active(
				   GTK_MENU(menu)))))) - 1;

	debug_print("index chosen %d\n", index);
	
	if (index < 0 || index > 2) {
		return TRUE;
	}
	
	iprop = vcalviewer_get_property(vcalviewer, ICAL_UID_PROPERTY);
	if (iprop) {
		event = vcal_manager_load_event(icalproperty_get_uid(iprop));
		icalproperty_free(iprop);
	}
	
	if (!event) {
		g_warning("can't get event\n");
		return TRUE;
	}
	account = get_account_from_attendees(vcalviewer);
	
	if (!account) {
		alertpanel_error (_("You have no account matching any attendee.\n"
				    "Have you really been invited?"));
		return TRUE;
	}
	
	vcal_manager_update_answer(event, account->address, account->name, reply[index], 0);
	
	if (!vcal_manager_reply(account, event)) {
		g_warning("couldn't send reply\n");
	}
	
	vcal_manager_free_event(event);
	
        return TRUE;
};

#define TABLE_ADD_LINE(label_text, widget) { 				\
	GtkWidget *label = gtk_label_new(_(label_text)); 		\
	gtk_misc_set_alignment (GTK_MISC(label), 1, 0);			\
	gtk_table_attach (GTK_TABLE (vcalviewer->table), 		\
			  label, 0, 1, i, i+1,				\
			  GTK_FILL, GTK_FILL, 6, 6);			\
	gtk_table_attach (GTK_TABLE (vcalviewer->table), 		\
			  widget, 1, 2, i, i+1,				\
			  GTK_FILL, GTK_FILL, 6, 6);			\
	if (GTK_IS_LABEL(widget)) {					\
		gtk_misc_set_alignment (GTK_MISC(widget),0, 0);		\
		gtk_label_set_line_wrap(GTK_LABEL(widget), TRUE);	\
	}								\
	i++;								\
}

MimeViewer *vcal_viewer_create(void)
{
	VCalViewer *vcalviewer;
	int i = 0;
	GtkWidget *hbox = NULL;
	
	debug_print("Creating vcal view...\n");
	vcalviewer = g_new0(VCalViewer, 1);
	vcalviewer->mimeviewer.factory = &vcal_viewer_factory;

	vcalviewer->mimeviewer.get_widget = vcal_viewer_get_widget;
	vcalviewer->mimeviewer.show_mimepart = vcal_viewer_show_mimepart;
	vcalviewer->mimeviewer.clear_viewer = vcal_viewer_clear_viewer;
	vcalviewer->mimeviewer.destroy_viewer = vcal_viewer_destroy_viewer;
	
	vcalviewer->table = gtk_table_new(8, 2, FALSE);
	vcalviewer->type = gtk_label_new("meeting");
	vcalviewer->who = gtk_label_new("who");
	vcalviewer->start = gtk_label_new("start");
	vcalviewer->end = gtk_label_new("end");
	vcalviewer->summary = gtk_label_new("summary");
	vcalviewer->description = gtk_label_new("description");
	vcalviewer->attendees = gtk_label_new("attendees");
	vcalviewer->answer = gtk_option_menu_new();
	vcalviewer->button = gtk_button_new_with_label(_("Answer"));
	vcalviewer->reedit = gtk_button_new_with_label(_("Edit meeting..."));
	vcalviewer->cancel = gtk_button_new_with_label(_("Cancel meeting..."));
	vcalviewer_answer_set_choices(vcalviewer, NULL, ICAL_METHOD_REQUEST);
	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbox), vcalviewer->answer, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vcalviewer->button, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vcalviewer->reedit, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vcalviewer->cancel, FALSE, FALSE, 0);
	
	gtk_signal_connect(GTK_OBJECT(vcalviewer->button), "clicked",
			 GTK_SIGNAL_FUNC(vcalviewer_action_cb), vcalviewer);

	gtk_signal_connect(GTK_OBJECT(vcalviewer->reedit), "clicked",
			 GTK_SIGNAL_FUNC(vcalviewer_reedit_cb), vcalviewer);

	gtk_signal_connect(GTK_OBJECT(vcalviewer->cancel), "clicked",
			 GTK_SIGNAL_FUNC(vcalviewer_cancel_cb), vcalviewer);

	TABLE_ADD_LINE("Event:", vcalviewer->type);
	TABLE_ADD_LINE("Organizer:", vcalviewer->who);
	TABLE_ADD_LINE("Summary:", vcalviewer->summary);
	TABLE_ADD_LINE("Starting:", vcalviewer->start);
	TABLE_ADD_LINE("Ending:", vcalviewer->end);
	TABLE_ADD_LINE("Description:", vcalviewer->description);
	TABLE_ADD_LINE("Attendees:", vcalviewer->attendees);
	TABLE_ADD_LINE("Action:", hbox);
	
	vcalviewer->scrolledwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_add_with_viewport(
		GTK_SCROLLED_WINDOW(vcalviewer->scrolledwin), 
		vcalviewer->table);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(vcalviewer->scrolledwin),
				       GTK_POLICY_NEVER,
				       GTK_POLICY_AUTOMATIC);
	
	gtk_widget_show_all(vcalviewer->scrolledwin);
	
	return (MimeViewer *) vcalviewer;
}

static gchar *content_types[] =
	{"text/calendar", NULL};
	
MimeViewerFactory vcal_viewer_factory =
{
	content_types,
	0,
	vcal_viewer_create,
};

void vcalendar_init(void)
{
	Folder *folder = NULL;
	
	mimeview_register_viewer_factory(&vcal_viewer_factory);
	folder_register_class(vcal_folder_get_class());

	folder = folder_find_from_name ("vCalendar", vcal_folder_get_class());
	if (!folder) {
		folder = folder_new(vcal_folder_get_class(), "vCalendar", NULL);
		folder->klass->create_tree(folder);
		folder_add(folder);
		folder_scan_tree(folder);
	}
	if (folder)
		folder_item_scan(folder->inbox);
	
	vcal_folder_gtk_init();
}

void vcalendar_done(void)
{
	mimeview_unregister_viewer_factory(&vcal_viewer_factory);
	folder_unregister_class(vcal_folder_get_class());
	vcal_folder_gtk_done();
}

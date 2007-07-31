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

#include <ical.h>
#include "gettext.h"
#include <gtk/gtk.h>

#include "mimeview.h"
#include "utils.h"
#include "vcalendar.h"
#include "vcal_manager.h"
#include "vcal_folder.h"
#include "vcal_meeting_gtk.h"
#include "prefs_account.h"
#include "prefs_common.h"
#include "account.h"
#include "codeconv.h"
#include "xml.h"
#include "xmlprops.h"
#include "alertpanel.h"
#include "vcal_prefs.h"
#include "statusbar.h"

MimeViewerFactory vcal_viewer_factory;

static void refresh_folder_contents(VCalViewer *vcalviewer);

static void create_meeting_from_message_cb(gpointer callback_data, guint callback_action, GtkWidget *widget);

static GdkColor uri_color = {
	(gulong)0,
	(gushort)0,
	(gushort)0,
	(gushort)0
};

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
	gchar	  *url;
	GtkWidget *answer;
	GtkWidget *button;
	GtkWidget *reedit;
	GtkWidget *cancel;
	GtkWidget *uribtn;
	GtkWidget *attendees;
	GtkWidget *unavail_box;
};

static GtkItemFactoryEntry vcalendar_main_menu = {
	N_("/Message/Create meeting from message..."),
	NULL,
	create_meeting_from_message_cb,
	0,
	NULL
};

static GtkItemFactoryEntry vcalendar_context_menu = {
	N_("/Create meeting from message..."),
	NULL,
	create_meeting_from_message_cb,
	0,
	NULL
};

static void create_meeting_from_message_cb(gpointer callback_data, guint callback_action, GtkWidget *widget)
{
	MainWindow *mainwin = mainwindow_get_mainwindow();
	SummaryView *summaryview = mainwin->summaryview;
	GSList *msglist = summary_get_selected_msg_list(summaryview);
	FolderItem *item = NULL;
	GSList *cur;
	gchar *msg;
	gint curnum=0, total=0;

	if (summary_is_locked(summaryview) || !msglist) {
		if (msglist)
			g_slist_free(msglist);
		return;
	}
	total = g_slist_length(msglist);
	
	msg = g_strdup_printf(_("You are about to create %d "
				       "meetings, one by one. Do you "
				       "want to continue?"), 
				       total);
	if (total > 9
	&&  alertpanel(_("Warning"), msg, GTK_STOCK_CANCEL, "+" GTK_STOCK_YES, NULL)
	    != G_ALERTALTERNATE) {
		g_free(msg);
		return;
	}
	g_free(msg);

	main_window_cursor_wait(summaryview->mainwin);
	gtk_clist_freeze(GTK_CLIST(summaryview->ctree));
	folder_item_update_freeze();
	inc_lock();

	item = summaryview->folder_item;

	STATUSBAR_PUSH(mainwin, _("Creating meeting..."));

	for (cur = msglist; cur; cur = cur->next) {
		MsgInfo *msginfo = procmsg_msginfo_get_full_info((MsgInfo *)cur->data);
		VCalEvent *event = NULL;
		FILE *fp = NULL;

		if (MSG_IS_ENCRYPTED(msginfo->flags)) {
			fp = procmime_get_first_encrypted_text_content(msginfo);
		} else {
			fp = procmime_get_first_text_content(msginfo);
		}
		
		if (fp) {
			gchar uid[256];
			time_t t = time(NULL);
			time_t t2 = t+3600;
			gchar *org = NULL;
			gchar *orgname = NULL;
			gchar *summary = g_strdup(msginfo->subject ? msginfo->subject:_("no subject"));
			gchar *description = file_read_stream_to_str(fp);
			gchar *dtstart = g_strdup(icaltime_as_ical_string(icaltime_from_timet(t, FALSE)));
			gchar *dtend = g_strdup(icaltime_as_ical_string(icaltime_from_timet(t2, FALSE)));
			gchar *recur = NULL;
			gchar *tzid = g_strdup("UTC");
			gchar *url = NULL;
			gint method = ICAL_METHOD_REQUEST;
			gint sequence = 1;
			PrefsAccount *account = NULL;
			
			fclose(fp);

			if (item && item->prefs && item->prefs->enable_default_account)
				account = account_find_from_id(item->prefs->default_account);

		 	if (!account) 
				account = cur_account;
			
			if (!account)
				goto bail;

			org = g_strdup(account->address);

			generate_msgid(uid, 255);
			
			event = vcal_manager_new_event(uid,
					org, NULL, summary, description, 
					dtstart, dtend, recur, tzid, url, method, sequence, 
					ICAL_VTODO_COMPONENT);
			
			/* hack to get default hours */
			g_free(event->dtstart);
			g_free(event->dtend);
			event->dtstart = NULL;
			event->dtend = NULL;

			vcal_meeting_create(event);
			vcal_manager_free_event(event);
			
bail:
			g_free(org);
			g_free(orgname);
			g_free(summary);
			g_free(description);
			g_free(dtstart);
			g_free(dtend);
			g_free(recur);
			g_free(tzid);
			g_free(url);
		}

		procmsg_msginfo_free(msginfo);
	}

	statusbar_progress_all(0,0,0);
	STATUSBAR_POP(mainwin);
	inc_unlock();
	folder_item_update_thaw();
	gtk_clist_thaw(GTK_CLIST(summaryview->ctree));
	main_window_cursor_normal(summaryview->mainwin);
	g_slist_free(msglist);
}


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
	gtk_widget_show_all(vcalviewer->scrolledwin);
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

static gchar *get_name_from_organizer_property(icalproperty *p)
{
	gchar *tmp = NULL;
	
	if (p && icalproperty_get_parameter_as_string(p, "CN") != NULL)
		tmp = g_strdup(icalproperty_get_parameter_as_string(p, "CN"));

	return tmp;
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
	FILE *fp;
	GByteArray *array;
	gchar buf[BUFSIZ];
	gint n_read;
	gchar *str;

	g_return_val_if_fail(file != NULL, NULL);

	if ((fp = fopen(file, "rb")) == NULL) {
		FILE_OP_ERROR(file, "fopen");
		return NULL;
	}

	array = g_byte_array_new();

	while ((n_read = fread(buf, sizeof(gchar), sizeof(buf), fp)) > 0) {
		if (n_read < sizeof(buf) && ferror(fp))
			break;
		g_byte_array_append(array, (guchar *)buf, n_read);
	}

	if (ferror(fp)) {
		FILE_OP_ERROR("file stream", "fread");
		g_byte_array_free(array, TRUE);
		return NULL;
	}

	buf[0] = '\0';
	g_byte_array_append(array, (guchar *)buf, 1);
	compstr = (gchar *)array->data;
	g_byte_array_free(array, FALSE);

	fclose(fp);	

	if (compstr) {
		comp = icalcomponent_new_from_string(compstr);
		g_free(compstr);
	}
	
	return comp;
}

#define GTK_LABEL_SET_TEXT_TRIMMED(label, text) {		\
	gchar *tmplbl = strretchomp(g_strdup(text));		\
	gtk_label_set_text(label, tmplbl);			\
	g_free(tmplbl);						\
}

static void vcalviewer_answer_set_choices(VCalViewer *vcalviewer, VCalEvent *event, enum icalproperty_method method);

static void vcalviewer_reset(VCalViewer *vcalviewer) 
{
	GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->type), "-");
	GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->who), "-");
	GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->summary), "-");
	GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->description), "-");
	GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->start), "-");
	GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->end), "-");
	GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->attendees), "-");
	g_free(vcalviewer->url);
	vcalviewer->url = NULL;
	gtk_widget_hide(vcalviewer->uribtn);
	vcalviewer_answer_set_choices(vcalviewer, NULL, ICAL_METHOD_CANCEL);
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

static gchar *vcalviewer_get_recurrence(VCalViewer *vcalviewer)
{
	gchar *rrule = NULL;
	icalproperty *iprop = vcalviewer_get_property(vcalviewer, ICAL_RRULE_PROPERTY);
	if (iprop) {
		struct icalrecurrencetype recur = icalproperty_get_rrule(iprop);
		rrule = g_strdup(icalrecurrencetype_as_string(&recur));
		icalproperty_free(iprop);
	} 
	return rrule;
}

static gchar *vcalviewer_get_dtstart(VCalViewer *vcalviewer)
{
	gchar *dtstart = NULL;
	icalproperty *iprop = vcalviewer_get_property(vcalviewer, ICAL_DTSTART_PROPERTY);
	if (iprop) {
		struct icaltimetype itt = icalproperty_get_dtstart(iprop);
		dtstart = g_strdup(icaltime_as_ical_string(itt));
		icalproperty_free(iprop);
	} 
	return dtstart;
}

static gchar *vcalviewer_get_dtend(VCalViewer *vcalviewer)
{
	gchar *dtend = NULL;
	icalproperty *iprop = vcalviewer_get_property(vcalviewer, ICAL_DTEND_PROPERTY);
	if (iprop) {
		struct icaltimetype itt = icalproperty_get_dtend(iprop);
		dtend = g_strdup(icaltime_as_ical_string(itt));
		icalproperty_free(iprop);
	} else {
		iprop = vcalviewer_get_property(vcalviewer, ICAL_DURATION_PROPERTY);
		if (iprop) {
			struct icaldurationtype duration = icalproperty_get_duration(iprop);
			struct icaltimetype itt;
			icalproperty_free(iprop);
			iprop = vcalviewer_get_property(vcalviewer, ICAL_DTSTART_PROPERTY);
			itt = icalproperty_get_dtstart(iprop);
			if (iprop) {
				icalproperty_free(iprop);
				dtend = g_strdup(icaltime_as_ical_string(icaltime_add(itt,duration)));
			}
		}
	}
	return dtend;
}

static void vcalviewer_show_unavailable(VCalViewer *vcalviewer, gboolean visi)
{
	if (visi)
		gtk_widget_show_all(vcalviewer->unavail_box);
	else
		gtk_widget_hide(vcalviewer->unavail_box);
}

static void vcalviewer_answer_set_choices(VCalViewer *vcalviewer, VCalEvent *event, enum icalproperty_method method)
{
	int i = 0;
	
	gtk_widget_hide(vcalviewer->reedit);
	gtk_widget_hide(vcalviewer->cancel);
	gtk_widget_hide(vcalviewer->answer);
	gtk_widget_hide(vcalviewer->button);

	for (i = 0; i < 3; i++) 
		gtk_combo_box_remove_text(GTK_COMBO_BOX(vcalviewer->answer), 0);
	
	vcalviewer_show_unavailable(vcalviewer, FALSE);

	if (method == ICAL_METHOD_REQUEST && event && !event->rec_occurence) {
		PrefsAccount *account = vcal_manager_get_account_from_event(event);
		
		if (!account)
			account = get_account_from_attendees(vcalviewer);

		if (!account && event) {
			account = account_get_default();
			vcal_manager_update_answer(event, account->address, 
					account->name,
					ICAL_PARTSTAT_NEEDSACTION, 
					ICAL_CUTYPE_INDIVIDUAL);
		}
		if (account) {
			gtk_combo_box_append_text(GTK_COMBO_BOX(vcalviewer->answer), _("Accept"));
			gtk_combo_box_append_text(GTK_COMBO_BOX(vcalviewer->answer), _("Tentatively accept"));
			gtk_combo_box_append_text(GTK_COMBO_BOX(vcalviewer->answer), _("Decline"));
			gtk_widget_set_sensitive(vcalviewer->answer, TRUE);
			gtk_widget_set_sensitive(vcalviewer->button, TRUE);
			gtk_widget_show(vcalviewer->answer);
			gtk_widget_show(vcalviewer->button);
		} else {
			gtk_combo_box_append_text(GTK_COMBO_BOX(vcalviewer->answer), "-");
			gtk_widget_set_sensitive(vcalviewer->answer, FALSE);
			gtk_widget_set_sensitive(vcalviewer->button, FALSE);
		}
	} else {
		gtk_combo_box_append_text(GTK_COMBO_BOX(vcalviewer->answer), "-");
		gtk_widget_set_sensitive(vcalviewer->answer, FALSE);
		gtk_widget_set_sensitive(vcalviewer->button, FALSE);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(vcalviewer->answer), 0);
	
	if (event && event->method == ICAL_METHOD_REQUEST) {
		PrefsAccount *account = vcal_manager_get_account_from_event(event);
		gchar *internal_ifb = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S,
					"vcalendar", G_DIR_SEPARATOR_S, 
					"internal.ifb", NULL);
		gchar *myfb = file_read_to_str(internal_ifb);
		gchar *dtstart = vcalviewer_get_dtstart(vcalviewer);
		gchar *dtend = vcalviewer_get_dtend(vcalviewer);
		g_free(internal_ifb);
		if (account) {
			enum icalparameter_partstat answer = 
				vcal_manager_get_reply_for_attendee(event, account->address);
				
			if (answer == ICAL_PARTSTAT_ACCEPTED)
				gtk_combo_box_set_active(GTK_COMBO_BOX(vcalviewer->answer), 0);
			if (answer == ICAL_PARTSTAT_TENTATIVE)
				gtk_combo_box_set_active(GTK_COMBO_BOX(vcalviewer->answer), 1);
			if (answer == ICAL_PARTSTAT_DECLINED)
				gtk_combo_box_set_active(GTK_COMBO_BOX(vcalviewer->answer), 2);
			if (dtstart && dtend && myfb && *myfb 
			&& answer != ICAL_PARTSTAT_ACCEPTED 
			&& answer != ICAL_PARTSTAT_TENTATIVE) {
				if (!attendee_available(NULL, dtstart, dtend, myfb))
					vcalviewer_show_unavailable(vcalviewer, TRUE);
			}
		}
		g_free(dtstart);
		g_free(dtend);
		g_free(myfb);
	}
}

void vcalviewer_display_event (VCalViewer *vcalviewer, VCalEvent *event)
{
	GSList *list = NULL;
	gchar *attendees = NULL;
	gboolean firstatt = TRUE;
	gboolean mine = FALSE;
	gchar *label = NULL;
	gboolean save_evt = FALSE;
	if (!event)
		return;
	if (!vcalviewer)
		return;

	if (event->type == ICAL_VTODO_COMPONENT) {
		label = g_strjoin(" ", _("You have a Todo item."),
				_("Details follow:"), NULL);
		GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->type), label);
	} else if (event->method == ICAL_METHOD_REQUEST) {
		if (account_find_from_address(event->organizer)) {
			label = g_strjoin(" ", _("You have created a meeting."),
					_("Details follow:"), NULL);
			GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->type), label);
			mine = TRUE;
		} else {
			label = g_strjoin(" ", _("You have been invited to a meeting."),
						_("Details follow:"), NULL);
			GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->type), label);
		}
	}
	else if (event->method == ICAL_METHOD_REPLY) {
		enum icalparameter_partstat answer = get_attendee_reply(vcalviewer);
		enum icalparameter_cutype cutype = get_attendee_replying_cutype(vcalviewer);
		gchar *attendee = get_attendee_replying(vcalviewer);
		gchar *name = get_attendee_replying_name(vcalviewer);
		
		vcal_manager_update_answer(event, attendee, name, answer, cutype);
		save_evt = TRUE;
		if (!attendee) {
			label = g_strjoin(" ",
				_("You have received an answer to an unknown meeting proposal."),
				_("Details follow:"), NULL);
			GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->type), label);
		} else {
			label = g_strdup_printf(_("You have received an answer to a meeting proposal.\n"
				"%s has %s the invitation whose details follow:"),
				attendee, vcal_manager_get_reply_text_for_attendee(event, attendee));
			GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->type), label);
			g_free(attendee);
		}
	} else if (event->method == ICAL_METHOD_CANCEL) {
		label = g_strjoin(" ",
				_("A meeting to which you had been invited has been cancelled."),
				_("Details follow:"), NULL);
		GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->type), label);
		save_evt = TRUE;
		refresh_folder_contents(vcalviewer);
	} else {
		label = g_strjoin(" ", _("You have been forwarded an appointment."),
				_("Details follow:"), NULL);
		GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->type), label);
	}
	g_free(label);

	if (event->orgname && strlen(event->orgname)
	&&  event->organizer && strlen(event->organizer)) {
		gchar *addr = g_strconcat(event->orgname, " <", event->organizer, ">", NULL);
		GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->who), addr);
		g_free(addr);
	} else if (event->organizer && strlen(event->organizer)) {
		GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->who), event->organizer);
	} else {
		GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->who), "-");
	}
	if (event->summary && strlen(event->summary)) {
		GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->summary), event->summary);
	} else {
		GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->summary), "-");
	}
	if (event->description && strlen(event->description)) {
		GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->description), event->description);
	} else {
		GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->description), "-");
	}
	g_free(vcalviewer->url);
	if (event->url && strlen(event->url)) {
		vcalviewer->url = g_strdup(event->url);
		gtk_widget_show(vcalviewer->uribtn);
	} else {
		vcalviewer->url = NULL;
		gtk_widget_hide(vcalviewer->uribtn);
	}
	
	if (event->start && strlen(event->start)) {
		if (event->recur && strlen(event->recur)) {
			gchar *tmp = g_strdup_printf(_("%s <span weight=\"bold\">(this event recurs)</span>"),
					event->start);
			GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->start), tmp);
			gtk_label_set_use_markup(GTK_LABEL(vcalviewer->start), TRUE);
			g_free(tmp);
		} else if (event->rec_occurence) {
			gchar *tmp = g_strdup_printf(_("%s <span weight=\"bold\">(this event is part of a recurring event)</span>"),
					event->start);
			GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->start), tmp);
			gtk_label_set_use_markup(GTK_LABEL(vcalviewer->start), TRUE);
			g_free(tmp);
		} else {
			GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->start), event->start);
		}
	} else {
		GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->start), "-");
	}
	if (event->end && strlen(event->end)) {
		GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->end), event->end);
	} else {
		GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->end), "-");
	}
	attendees = g_strdup("");
	for (list = vcal_manager_get_answers_emails(event); 
	     list && list->data; list = list->next) {
	     	gchar *attendee = (gchar *)list->data;
		gchar *name = vcal_manager_get_attendee_name(event, attendee);
		gchar *ename = g_markup_printf_escaped("%s", name?name:"");
		gchar *eatt = g_markup_printf_escaped("%s", attendee);
		enum icalparameter_partstat acode = vcal_manager_get_reply_for_attendee(event, attendee);
		gchar *answer = vcal_manager_get_reply_text_for_attendee(event, attendee);
		gchar *type = vcal_manager_get_cutype_text_for_attendee(event, attendee);
		gchar *tmp = NULL;
		gint e_len, n_len;
		tmp = g_strdup_printf(
				"%s%s&lt;%s&gt; (%s, <span%s>%s</span>)", 
				(ename && strlen(ename))?ename:"",
				(ename && strlen(ename))?" ":"",
				(eatt && strlen(eatt))?eatt:"", 
				(type && strlen(type))?type:"", 
				(acode != ICAL_PARTSTAT_ACCEPTED ? " foreground=\"red\"":""),
				(answer && strlen(answer))?answer:"");
		e_len = strlen(attendees);
		n_len = strlen(tmp);
		if (e_len) {
			attendees = g_realloc(attendees, e_len+n_len+2);
			*(attendees+e_len) = '\n';
			strcpy(attendees+e_len+1, tmp);
		} else {
			attendees = g_realloc(attendees, e_len+n_len+1);
			strcpy(attendees, tmp);
		}
		g_free(tmp);
		g_free(answer);
		g_free(type);
		g_free(name);
		g_free(ename);
		g_free(eatt);
	}
	
	if (attendees && strlen(attendees)) {
		GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->attendees), attendees);
	} else {
		GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->attendees), "-");
	}
	g_free(attendees);
	gtk_label_set_use_markup(GTK_LABEL(vcalviewer->attendees), TRUE);
	if (!mine)
		if (event->type != ICAL_VTODO_COMPONENT)
			vcalviewer_answer_set_choices(vcalviewer, event, event->method);
		else
			vcalviewer_answer_set_choices(vcalviewer, event, ICAL_METHOD_PUBLISH);
	else {
		vcalviewer_answer_set_choices(vcalviewer, event, ICAL_METHOD_REPLY);
		gtk_widget_show(vcalviewer->reedit);
		gtk_widget_show(vcalviewer->cancel);
	}
	if (save_evt)
		vcal_manager_save_event(event, TRUE);
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

static void vcalviewer_get_request_values(VCalViewer *vcalviewer, MimeInfo *mimeinfo, gboolean is_todo) 
{
	icalproperty *iprop = NULL;
	gchar *org = NULL, *summary = NULL, *description = NULL, *url = NULL;
	gchar *dtstart = NULL, *dtend = NULL, *tzid = NULL, *orgname = NULL;
	gchar *recur = NULL;
	enum icalproperty_method method = ICAL_METHOD_REQUEST;
	VCalEvent *event = NULL;
	gchar *tmp;
	const gchar *charset = procmime_mimeinfo_get_parameter(mimeinfo, "charset");
	const gchar *saveme =  procmime_mimeinfo_get_parameter(mimeinfo, "vcalsave");
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
		charset = CS_INTERNAL;
		event->method = method;
		vcalviewer_display_event(vcalviewer, event);
		vcal_manager_free_event(event);
		g_free(uid);
		return;
	} else if (event) {
		vcal_manager_free_event(event);
	}

	/* load it and register it */

	iprop = vcalviewer_get_property(vcalviewer, ICAL_ORGANIZER_PROPERTY);
	if (iprop) {
		tmp = get_email_from_organizer_property(iprop);
		if (tmp && !g_utf8_validate(tmp, -1, NULL))
			org = conv_codeset_strdup(tmp, charset, CS_UTF_8);
		else if (tmp)
			org = g_strdup(tmp);
		else
			org = NULL;
		g_free(tmp);
		icalproperty_free(iprop);
		tmp = get_name_from_organizer_property(iprop);
		if (tmp && !g_utf8_validate(tmp, -1, NULL))
			orgname = conv_codeset_strdup(tmp, charset, CS_UTF_8);
		else if (tmp)
			orgname = g_strdup(tmp);
		else
			orgname = NULL;
		g_free(tmp);
	} 
	
	iprop = vcalviewer_get_property(vcalviewer, ICAL_SUMMARY_PROPERTY);
	if (iprop) {
		if (!g_utf8_validate(icalproperty_get_summary(iprop), -1, NULL))
			summary = conv_codeset_strdup(icalproperty_get_summary(iprop), charset, CS_UTF_8);
		else
			summary = g_strdup(icalproperty_get_summary(iprop));
		icalproperty_free(iprop);
	} 

	iprop = vcalviewer_get_property(vcalviewer, ICAL_DESCRIPTION_PROPERTY);
	if (iprop) {
		if (!g_utf8_validate(icalproperty_get_description(iprop), -1, NULL))
			description = conv_codeset_strdup(icalproperty_get_description(iprop), charset, CS_UTF_8);
		else
			description = g_strdup(icalproperty_get_description(iprop));
		icalproperty_free(iprop);
	} 
	
	iprop = vcalviewer_get_property(vcalviewer, ICAL_URL_PROPERTY);
	if (iprop) {
		if (!g_utf8_validate(icalproperty_get_url(iprop), -1, NULL))
			url = conv_codeset_strdup(icalproperty_get_url(iprop), charset, CS_UTF_8);
		else
			url = g_strdup(icalproperty_get_url(iprop));
		icalproperty_free(iprop);
	} 
	dtstart = vcalviewer_get_dtstart(vcalviewer);
	dtend = vcalviewer_get_dtend(vcalviewer);
	recur = vcalviewer_get_recurrence(vcalviewer);
	
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
					org, orgname, summary, description, 
					dtstart, dtend, recur, tzid, url, method, sequence, 
					is_todo?ICAL_VTODO_COMPONENT:ICAL_VEVENT_COMPONENT);
	vcalviewer_get_attendees(vcalviewer, event);
	if (!saveme || strcmp(saveme,"no"))
		vcal_manager_save_event(event, FALSE);

	g_free(org); 
	g_free(orgname); 
	g_free(summary);
	g_free(description);
	g_free(url);
	g_free(uid);
	g_free(dtstart);
	g_free(dtend);
	g_free(recur);
	g_free(tzid);
	vcalviewer_display_event(vcalviewer, event);
	vcal_manager_free_event(event);
}

static void vcalviewer_get_reply_values(VCalViewer *vcalviewer, MimeInfo *mimeinfo) 
{
	icalproperty *iprop = NULL;
	gchar *tmp = NULL;
	const gchar *charset = procmime_mimeinfo_get_parameter(mimeinfo, "charset");
	gchar *label = NULL;
	enum icalparameter_partstat answer = get_attendee_reply(vcalviewer);
	enum icalparameter_cutype cutype = get_attendee_replying_cutype(vcalviewer);
	gchar *attendee = get_attendee_replying(vcalviewer);
	gchar *name = get_attendee_replying_name(vcalviewer);

	if (!charset)
		charset=CS_ISO_8859_1;

	iprop = vcalviewer_get_property(vcalviewer, ICAL_UID_PROPERTY);
	if (iprop) {
		VCalEvent *event = vcal_manager_load_event(icalproperty_get_uid(iprop));
		if (event) {
			charset = CS_INTERNAL;
			event->method = ICAL_METHOD_REPLY;
			vcalviewer_display_event(vcalviewer, event);
			vcal_manager_free_event(event);
			return;
		} 
	} 
	
	/* unknown answer but get the replier anyway */
		
	if (!attendee) {
		label = g_strjoin(" ",
			_("You have received an answer to an unknown meeting proposal."),
			_("Details follow:"), NULL);
		GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->type), label);
	} else {
		label = g_strdup_printf(_("You have received an answer to a meeting proposal.\n"
			"%s has %s the invitation whose details follow:"),
			attendee, vcal_manager_answer_get_text(answer));
		GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->type), label);
		g_free(attendee);
	}

	g_free(label);

	iprop = vcalviewer_get_property(vcalviewer, ICAL_ORGANIZER_PROPERTY);
	if (iprop) {
		gchar *org, *orgname;
		tmp = get_email_from_organizer_property(iprop);
		if (tmp && !g_utf8_validate(tmp, -1, NULL))
			org = conv_codeset_strdup(tmp, charset, CS_UTF_8);
		else if (tmp)
			org = g_strdup(tmp);
		else
			org = NULL;
		g_free(tmp);
		tmp = get_name_from_organizer_property(iprop);
		if (tmp && !g_utf8_validate(tmp, -1, NULL))
			orgname = conv_codeset_strdup(tmp, charset, CS_UTF_8);
		else if (tmp)
			orgname = g_strdup(tmp);
		else
			orgname = NULL;
		g_free(tmp);
		if (orgname && org) {
			gchar *addr = g_strconcat(orgname, " <", org, ">", NULL);
			GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->who), addr);
			g_free(addr);
		} else if (org) {
			GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->who), org);
		} else {
			GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->who), "-");
		}
		icalproperty_free(iprop);
		g_free(org);
		g_free(orgname);
	} else {
		GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->who), "-");
	}
	iprop = vcalviewer_get_property(vcalviewer, ICAL_SUMMARY_PROPERTY);
	if (iprop) {
		if (!g_utf8_validate(icalproperty_get_summary(iprop), -1, NULL))
			tmp = conv_codeset_strdup(icalproperty_get_summary(iprop), charset, CS_UTF_8);
		else
			tmp = g_strdup(icalproperty_get_summary(iprop));
		GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->summary), tmp);
		g_free(tmp);
		icalproperty_free(iprop);
	} else {
		GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->summary), "-");
	}
	iprop = vcalviewer_get_property(vcalviewer, ICAL_DESCRIPTION_PROPERTY);
	if (iprop) {
		if (!g_utf8_validate(icalproperty_get_description(iprop), -1, NULL))
			tmp = conv_codeset_strdup(icalproperty_get_description(iprop), charset, CS_UTF_8);
		else
			tmp = g_strdup(icalproperty_get_description(iprop));
		GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->description), tmp);
		g_free(tmp);
		icalproperty_free(iprop);
	} else {
		GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->description), "-");
	}
	iprop = vcalviewer_get_property(vcalviewer, ICAL_URL_PROPERTY);
	g_free(vcalviewer->url);
	if (iprop) {
		if (!g_utf8_validate(icalproperty_get_url(iprop), -1, NULL))
			tmp = conv_codeset_strdup(icalproperty_get_url(iprop), charset, CS_UTF_8);
		else
			tmp = g_strdup(icalproperty_get_url(iprop));
		vcalviewer->url = g_strdup(tmp);
		g_free(tmp);
		icalproperty_free(iprop);
		gtk_widget_show(vcalviewer->uribtn);
	} else {
		vcalviewer->url = NULL;
		gtk_widget_hide(vcalviewer->uribtn);
	}

	iprop = vcalviewer_get_property(vcalviewer, ICAL_DTSTART_PROPERTY);
	if (iprop) {
		struct icaltimetype itt = (icalproperty_get_dtstart(iprop));
		gchar buft[512];
		time_t tmp = icaltime_as_timet(itt);
		GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->start), ctime_r(&tmp, buft));
		icalproperty_free(iprop);
	} else {
		GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->start), "-");
	}

	iprop = vcalviewer_get_property(vcalviewer, ICAL_DTEND_PROPERTY);
	if (iprop) {
		struct icaltimetype itt = (icalproperty_get_dtstart(iprop));
		gchar buft[512];
		time_t tmp = icaltime_as_timet(itt);
		GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->end), ctime_r(&tmp, buft));
		icalproperty_free(iprop);
	} else {
		GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->end), "-");
	}

	GTK_LABEL_SET_TEXT_TRIMMED(GTK_LABEL(vcalviewer->attendees), "-"); 

	vcalviewer_answer_set_choices(vcalviewer, NULL, ICAL_METHOD_REPLY);
}

static void vcalviewer_get_info(VCalViewer *vcalviewer, MimeInfo *mimeinfo) 
{
	icalproperty *method = NULL;
	gboolean is_todo = FALSE;
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
	
	if (vcalviewer->icomp && icalcomponent_isa(vcalviewer->icomp) == ICAL_VTODO_COMPONENT)
		is_todo = TRUE;
debug_print("is_todo %d\n", is_todo);
	if (icalcomponent_isa(vcalviewer->comp) == ICAL_VCALENDAR_COMPONENT && method) {
		if (icalproperty_get_method(method) == ICAL_METHOD_REQUEST
		||  icalproperty_get_method(method) == ICAL_METHOD_PUBLISH
		||  icalproperty_get_method(method) == ICAL_METHOD_CANCEL)
			vcalviewer_get_request_values(vcalviewer, mimeinfo, is_todo);
		else if (icalproperty_get_method(method) == ICAL_METHOD_REPLY)
			vcalviewer_get_reply_values(vcalviewer, mimeinfo);
		else
			vcalviewer_reset(vcalviewer);
	} else if (is_todo && !method) {
		vcalviewer_get_request_values(vcalviewer, mimeinfo, is_todo);
	} else
		vcalviewer_reset(vcalviewer);
}

static VCalViewer *s_vcalviewer = NULL;

static void vcal_viewer_show_mimepart(MimeViewer *_mimeviewer, const gchar *file, MimeInfo *mimeinfo)
{
	VCalViewer *vcalviewer = (VCalViewer *) _mimeviewer;
	s_vcalviewer = vcalviewer;

	if (mimeinfo == NULL) {
		vcal_viewer_clear_viewer(_mimeviewer);
		return;
	}
	debug_print("vcal_viewer_show_mimepart : %s\n", file);

	vcal_viewer_clear_viewer(_mimeviewer);
	gtk_widget_show_all(vcalviewer->scrolledwin);
	g_free(vcalviewer->file);
	vcalviewer->file = g_strdup(file);
	vcalviewer->mimeinfo = mimeinfo;
	vcalviewer_get_info(vcalviewer, mimeinfo);
	GTK_EVENTS_FLUSH();
	gtk_widget_set_size_request(vcalviewer->description, 
		vcalviewer->scrolledwin->allocation.width - 200, -1);
	gtk_label_set_line_wrap(GTK_LABEL(vcalviewer->summary), TRUE);
	gtk_label_set_line_wrap(GTK_LABEL(vcalviewer->description), TRUE);
	gtk_label_set_line_wrap(GTK_LABEL(vcalviewer->attendees), FALSE);
	
	if (prefs_common.textfont) {
		PangoFontDescription *font_desc;

		font_desc = pango_font_description_from_string
						(prefs_common.textfont);
		if (font_desc) {
			gtk_widget_modify_font(
				vcalviewer->description, font_desc);
			pango_font_description_free(font_desc);
		}
	}
	
}

void vcalviewer_reload(void)
{
	if (s_vcalviewer) {
		MainWindow *mainwin = mainwindow_get_mainwindow();
		Folder *folder = folder_find_from_name ("vCalendar", vcal_folder_get_class());

		folder_item_scan(folder->inbox);
		if (mainwin && mainwin->summaryview->folder_item) {
			FolderItem *cur = mainwin->summaryview->folder_item;
			if (cur->folder == folder)
				folder_item_scan(cur);
		}
		if (mainwin && mainwin->summaryview->folder_item == folder->inbox) {
			debug_print("reload: %p, %p\n", (MimeViewer *)s_vcalviewer, s_vcalviewer->mimeinfo);
			summary_redisplay_msg(mainwin->summaryview);
		}
	}
}

static void vcal_viewer_destroy_viewer(MimeViewer *_mimeviewer)
{
	VCalViewer *vcalviewer = (VCalViewer *) _mimeviewer;

	debug_print("vcal_viewer_destroy_viewer\n");

	if (s_vcalviewer == vcalviewer)
		s_vcalviewer = NULL;
	vcal_viewer_clear_viewer(_mimeviewer);
	g_free(vcalviewer);
}

static gboolean vcalviewer_reedit_cb(GtkButton *widget, gpointer data)
{
	VCalViewer *vcalviewer = (VCalViewer *)data;
	gchar * uid = vcalviewer_get_uid_from_mimeinfo(vcalviewer->mimeinfo);
	VCalEvent *event = NULL;
	
	s_vcalviewer = vcalviewer;
	/* see if we have it registered and more recent */
	event = vcal_manager_load_event(uid);
	vcal_meeting_create(event);
	vcal_manager_free_event(event);
	return TRUE;
}

static gboolean vcalviewer_uribtn_cb(GtkButton *widget, gpointer data)
{
	VCalViewer *vcalviewer = (VCalViewer *)data;

	if (vcalviewer->url)
		open_uri(vcalviewer->url, prefs_common.uri_cmd);

	return TRUE;
}

static void refresh_folder_contents(VCalViewer *vcalviewer)
{
	Folder *folder = folder_find_from_name ("vCalendar", vcal_folder_get_class());
	if (folder) {
		MainWindow *mainwin = mainwindow_get_mainwindow();
		folder_item_scan(folder->inbox);
		if (mainwin->summaryview->folder_item == folder->inbox)
			summary_show(mainwin->summaryview, folder->inbox);
	}
}
static gboolean vcalviewer_cancel_cb(GtkButton *widget, gpointer data)
{
	VCalViewer *vcalviewer = (VCalViewer *)data;
	gchar * uid = vcalviewer_get_uid_from_mimeinfo(vcalviewer->mimeinfo);
	VCalEvent *event = NULL;
	VCalMeeting *meet = NULL;
	gchar *file = NULL;
	gint val = 0;
	val = alertpanel_full(_("Cancel meeting"),
				   _("Are you sure you want to cancel this meeting?\n"
			           "A notification will be sent to attendees."),
				   GTK_STOCK_NO, GTK_STOCK_YES, NULL, FALSE,
				   NULL, ALERT_WARNING, G_ALERTDEFAULT);

	if (val != G_ALERTALTERNATE)
		return TRUE;
	
	s_vcalviewer = vcalviewer;
	
	event = vcal_manager_load_event(uid);
	if (!event)
		return TRUE;
	event->method = ICAL_METHOD_CANCEL;
	
	meet = vcal_meeting_create_hidden(event);
	if (!vcal_meeting_send(meet)) {
		event->method = ICAL_METHOD_REQUEST;
		vcal_manager_save_event(event, TRUE);
		vcal_manager_free_event(event);
		refresh_folder_contents(vcalviewer);
		vcalviewer_reset(vcalviewer);
		return TRUE;
	}

	vcal_manager_save_event(event, TRUE);
	
	file = vcal_manager_get_event_file(event->uid);
	unlink(file);
	vcal_manager_free_event(event);
	g_free(file);
	refresh_folder_contents(vcalviewer);
	vcalviewer_reset(vcalviewer);

	return TRUE;
}

static gboolean vcalviewer_action_cb(GtkButton *widget, gpointer data)
{
        VCalViewer *vcalviewer = (VCalViewer *)data;
	VCalEvent *event = NULL;
	icalproperty *iprop = NULL;
	gint index = gtk_combo_box_get_active(GTK_COMBO_BOX(vcalviewer->answer));
	enum icalparameter_partstat reply[3] = {ICAL_PARTSTAT_ACCEPTED, ICAL_PARTSTAT_TENTATIVE, ICAL_PARTSTAT_DECLINED};
	PrefsAccount *account = NULL;
	
	debug_print("index chosen %d\n", index);
	
	if (index < 0 || index > 2) {
		return TRUE;
	}
	
	s_vcalviewer = vcalviewer;

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
		AlertValue val = alertpanel_full(_("No account found"), 
					_("You have no account matching any attendee.\n"
					    "Do you want to reply anyway ?"),
				   	GTK_STOCK_CANCEL, _("+Reply anyway"), NULL, FALSE,
				   	NULL, ALERT_QUESTION, G_ALERTDEFAULT);
		if (val == G_ALERTALTERNATE) {		
			account = account_get_default();
			vcal_manager_update_answer(event, account->address, 
					account->name,
					ICAL_PARTSTAT_NEEDSACTION, 
					ICAL_CUTYPE_INDIVIDUAL);
		} else
			return TRUE;
	}
	
	vcal_manager_update_answer(event, account->address, account->name, reply[index], 0);
	
	if (event->organizer && *(event->organizer) && !vcal_manager_reply(account, event)) {
		g_warning("couldn't send reply\n");
	} else {
		debug_print("no organizer, not sending answer\n");
	}
	
	vcal_manager_save_event(event, TRUE);

	vcalviewer_display_event(vcalviewer, event);

	vcal_manager_free_event(event);
	
        return TRUE;
};

#define TABLE_ADD_LINE(label_text, widget) { 				\
	gchar *tmpstr = g_strdup_printf("<span weight=\"bold\">%s</span>",\
				label_text);				\
	GtkWidget *label = gtk_label_new(tmpstr);		 	\
	g_free(tmpstr);							\
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);		\
	gtk_misc_set_alignment (GTK_MISC(label), 1, 0);			\
	gtk_table_attach (GTK_TABLE (vcalviewer->table), 		\
			  label, 0, 1, i, i+1,				\
			  GTK_FILL, GTK_FILL, 6, 6);			\
	gtk_table_attach (GTK_TABLE (vcalviewer->table), 		\
			  widget, 1, 2, i, i+1,				\
			  GTK_FILL, GTK_FILL, 6, 6);			\
	if (GTK_IS_LABEL(widget)) {					\
		gtk_label_set_use_markup(GTK_LABEL (widget), TRUE);	\
		gtk_misc_set_alignment (GTK_MISC(widget),0, 0);		\
		gtk_label_set_line_wrap(GTK_LABEL(widget), TRUE);	\
	}								\
	i++;								\
}

static gchar *vcal_viewer_get_selection(MimeViewer *_viewer)
{
	VCalViewer *viewer = (VCalViewer *)_viewer;
	if (viewer->summary == NULL)
		return NULL;
	if (gtk_label_get_text(GTK_LABEL(viewer->description))
	&&  strlen(gtk_label_get_text(GTK_LABEL(viewer->description))) > 2) {
		gint start, end;
		if (gtk_label_get_selection_bounds(GTK_LABEL(viewer->description), 
						   &start, &end)) {
			gchar *tmp = g_strdup(gtk_label_get_text(
						GTK_LABEL(viewer->description))+start);
			tmp[end-start] = '\0';
			return tmp;
		} else {
			return g_strdup(gtk_label_get_text(GTK_LABEL(viewer->description)));
		}
	}
	else if (gtk_label_get_text(GTK_LABEL(viewer->summary))
	&&  strlen(gtk_label_get_text(GTK_LABEL(viewer->summary))) > 2)
		return g_strdup(gtk_label_get_text(GTK_LABEL(viewer->summary)));
	else 
		return NULL;
}


static gboolean vcal_viewer_scroll_page(MimeViewer *_viewer, gboolean up)
{
	VCalViewer *viewer = (VCalViewer *)_viewer;
	GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(
		GTK_SCROLLED_WINDOW(viewer->scrolledwin));
	
	return gtkutils_scroll_page(viewer->table, vadj, up);
}

static void vcal_viewer_scroll_one_line(MimeViewer *_viewer, gboolean up)
{
	VCalViewer *viewer = (VCalViewer *)_viewer;
	GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(
		GTK_SCROLLED_WINDOW(viewer->scrolledwin));
	
	gtkutils_scroll_one_line(viewer->table, vadj, up);
}

MimeViewer *vcal_viewer_create(void)
{
	VCalViewer *vcalviewer;
	int i = 0;
	GtkWidget *hbox = NULL, *vbox = NULL;
	GtkStyle *style;
	GtkWidget *warning_img;
	GtkWidget *warning_label;
	
	debug_print("Creating vcal view...\n");
	vcalviewer = g_new0(VCalViewer, 1);
	vcalviewer->mimeviewer.factory = &vcal_viewer_factory;

	vcalviewer->mimeviewer.get_widget = vcal_viewer_get_widget;
	vcalviewer->mimeviewer.show_mimepart = vcal_viewer_show_mimepart;
	vcalviewer->mimeviewer.clear_viewer = vcal_viewer_clear_viewer;
	vcalviewer->mimeviewer.destroy_viewer = vcal_viewer_destroy_viewer;
	vcalviewer->mimeviewer.get_selection = vcal_viewer_get_selection;
	vcalviewer->mimeviewer.scroll_page = vcal_viewer_scroll_page;
	vcalviewer->mimeviewer.scroll_one_line = vcal_viewer_scroll_one_line;

	vcalviewer->table = gtk_table_new(8, 2, FALSE);
	vcalviewer->type = gtk_label_new("meeting");
	vcalviewer->who = gtk_label_new("who");
	vcalviewer->start = gtk_label_new("start");
	vcalviewer->end = gtk_label_new("end");
	vcalviewer->summary = gtk_label_new("summary");
	vcalviewer->description = gtk_label_new("description");
	vcalviewer->attendees = gtk_label_new("attendees");

	vcalviewer->answer = gtk_combo_box_new_text();
	vcalviewer->url = NULL;
	vcalviewer->button = gtk_button_new_with_label(_("Answer"));
	vcalviewer->reedit = gtk_button_new_with_label(_("Edit meeting..."));
	vcalviewer->cancel = gtk_button_new_with_label(_("Cancel meeting..."));
	vcalviewer->uribtn = gtk_button_new_with_label(_("Launch website"));
	vcalviewer->unavail_box = gtk_hbox_new(FALSE, 6);
	warning_img = gtk_image_new_from_stock
                        (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_SMALL_TOOLBAR);
	warning_label = gtk_label_new(_("You are already busy at this time."));

	gtk_box_pack_start(GTK_BOX(vcalviewer->unavail_box), warning_img, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vcalviewer->unavail_box), warning_label, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbox), vcalviewer->answer, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vcalviewer->button, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vcalviewer->reedit, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vcalviewer->cancel, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vcalviewer->uribtn, FALSE, FALSE, 0);
	
	vbox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), vcalviewer->unavail_box, FALSE, FALSE, 0);

	vcalviewer_answer_set_choices(vcalviewer, NULL, ICAL_METHOD_REQUEST);

	gtk_label_set_selectable(GTK_LABEL(vcalviewer->type), TRUE);
	gtk_label_set_selectable(GTK_LABEL(vcalviewer->who), TRUE);
	gtk_label_set_selectable(GTK_LABEL(vcalviewer->start), TRUE);
	gtk_label_set_selectable(GTK_LABEL(vcalviewer->end), TRUE);
	gtk_label_set_selectable(GTK_LABEL(vcalviewer->summary), TRUE);
	gtk_label_set_selectable(GTK_LABEL(vcalviewer->description), TRUE);
	gtk_label_set_selectable(GTK_LABEL(vcalviewer->attendees), TRUE);
	
/*	gtk_widget_ensure_style(vcalviewer->who);
	style = gtk_style_copy
		(gtk_widget_get_style(vcalviewer->who));
	style->fg[GTK_STATE_NORMAL]   = uri_color;
	style->fg[GTK_STATE_ACTIVE]   = uri_color;
	style->fg[GTK_STATE_PRELIGHT] = uri_color;
	gtk_widget_set_style(vcalviewer->who, style);

	gtk_widget_ensure_style(vcalviewer->attendees);
	style = gtk_style_copy
		(gtk_widget_get_style(vcalviewer->attendees));
	style->fg[GTK_STATE_NORMAL]   = uri_color;
	style->fg[GTK_STATE_ACTIVE]   = uri_color;
	style->fg[GTK_STATE_PRELIGHT] = uri_color;
	gtk_widget_set_style(vcalviewer->attendees, style);
*/
	g_signal_connect(G_OBJECT(vcalviewer->button), "clicked",
			 G_CALLBACK(vcalviewer_action_cb), vcalviewer);

	g_signal_connect(G_OBJECT(vcalviewer->reedit), "clicked",
			 G_CALLBACK(vcalviewer_reedit_cb), vcalviewer);

	g_signal_connect(G_OBJECT(vcalviewer->cancel), "clicked",
			 G_CALLBACK(vcalviewer_cancel_cb), vcalviewer);

	g_signal_connect(G_OBJECT(vcalviewer->uribtn), "clicked",
			 G_CALLBACK(vcalviewer_uribtn_cb), vcalviewer);

	TABLE_ADD_LINE(_("Event:"), vcalviewer->type);
	TABLE_ADD_LINE(_("Organizer:"), vcalviewer->who);
	TABLE_ADD_LINE(_("Summary:"), vcalviewer->summary);
	TABLE_ADD_LINE(_("Starting:"), vcalviewer->start);
	TABLE_ADD_LINE(_("Ending:"), vcalviewer->end);
	TABLE_ADD_LINE(_("Description:"), vcalviewer->description);
	TABLE_ADD_LINE(_("Attendees:"), vcalviewer->attendees);
	gtk_label_set_line_wrap(GTK_LABEL(vcalviewer->attendees), FALSE);
	TABLE_ADD_LINE(_("Action:"), vbox);
	
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

static gint alert_timeout_tag = 0;
static gint scan_timeout_tag = 0;

static gint vcal_webcal_check(gpointer data)
{
	Folder *root = folder_find_from_name ("vCalendar", vcal_folder_get_class());

	if (prefs_common.work_offline)
		return TRUE;
	
	manual_update = FALSE;
	folderview_check_new(root);
	manual_update = TRUE;
	return TRUE;
}

void vcalendar_init(void)
{
	GtkItemFactory *ifactory;
	MainWindow *mainwin = mainwindow_get_mainwindow();
	SummaryView *summaryview = mainwin->summaryview;
	Folder *folder = NULL;
	GSList *events = NULL;
	GSList *cur = NULL;
	gchar *tmp = NULL;
	gchar *directory = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S,
				"vcalendar", NULL);
	if (!is_dir_exist(directory))
		make_dir (directory);
	g_free(directory);

	vcal_prefs_init();

	mimeview_register_viewer_factory(&vcal_viewer_factory);
	folder_register_class(vcal_folder_get_class());

	folder = folder_find_from_name ("vCalendar", vcal_folder_get_class());
	if (!folder) {
		folder = folder_new(vcal_folder_get_class(), "vCalendar", NULL);
		folder->klass->create_tree(folder);
		folder_add(folder);
		folder_scan_tree(folder, TRUE);
	}
	if (folder)
		folder_item_scan(folder->inbox);
	
	vcal_folder_gtk_init();

	alert_timeout_tag = gtk_timeout_add(60*1000, 
				(GtkFunction)vcal_meeting_alert_check, 
				(gpointer)NULL);
	scan_timeout_tag = gtk_timeout_add(3600*1000, 
				(GtkFunction)vcal_webcal_check, 
				(gpointer)NULL);
	if (prefs_common.enable_color) {
		gtkut_convert_int_to_gdk_color(prefs_common.uri_col,
				       &uri_color);
	}

	vcalendar_main_menu.path = _(vcalendar_main_menu.path);
	vcalendar_context_menu.path = _(vcalendar_context_menu.path);
	ifactory = gtk_item_factory_from_widget(mainwin->menubar);
	gtk_item_factory_create_item(ifactory, &vcalendar_main_menu, mainwin, 1);
	gtk_item_factory_create_item(summaryview->popupfactory, &vcalendar_context_menu, summaryview, 1);
}

void vcalendar_done(void)
{
	MainWindow *mainwin = mainwindow_get_mainwindow();
	FolderView *folderview = NULL;
	FolderItem *fitem = NULL;
	GtkItemFactory *ifactory;
	SummaryView *summaryview = NULL;
	GtkWidget *widget;

	icalmemory_free_ring();

	if (mainwin == NULL)
		return;
	summaryview = mainwin->summaryview;
	folderview = mainwin->folderview;
	fitem = folderview->summaryview->folder_item;

	if (fitem && 
	    fitem->folder->klass == vcal_folder_get_class()) {
		folderview_unselect(folderview);
		summary_clear_all(folderview->summaryview);
	}

	mimeview_unregister_viewer_factory(&vcal_viewer_factory);
	folder_unregister_class(vcal_folder_get_class());
	vcal_folder_gtk_done();
	vcal_prefs_done();
	gtk_timeout_remove(alert_timeout_tag);
	alert_timeout_tag = 0;
	gtk_timeout_remove(scan_timeout_tag);
	scan_timeout_tag = 0;

	ifactory = gtk_item_factory_from_widget(mainwin->menubar);
	widget = gtk_item_factory_get_widget(ifactory, vcalendar_main_menu.path);
	gtk_widget_destroy(widget);
	gtk_item_factory_delete_item(ifactory, vcalendar_main_menu.path);

	widget = gtk_item_factory_get_widget(summaryview->popupfactory, vcalendar_context_menu.path);
	gtk_widget_destroy(widget);
	gtk_item_factory_delete_item(summaryview->popupfactory, vcalendar_context_menu.path);
}

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

#include <sys/types.h>
#include <sys/stat.h>
#include "gettext.h"
#include <ical.h>
#include "utils.h"
#include "vcalendar.h"
#include "vcal_folder.h"
#include "vcal_manager.h"
#include "vcal_meeting_gtk.h"
#include "vcal_prefs.h"
#include "xml.h"
#include "xmlprops.h"
#include "prefs.h"
#include "prefs_common.h"
#include "prefs_account.h"
#include "account.h"
#include "codeconv.h"
#include <time.h>
#include "folder.h"
#include "quoted-printable.h"

typedef struct _Answer Answer;

struct _Answer {
	gchar *attendee;
	gchar *name;
	enum icalparameter_partstat answer;
	enum icalparameter_cutype cutype;
};

static Answer *answer_new(const gchar *attendee, 
			  const gchar *name,
			  enum icalparameter_partstat ans,
			  enum icalparameter_cutype cutype)
{
	Answer *answer = g_new0(Answer, 1);
	answer->attendee = g_strdup(attendee);
	answer->name = g_strdup(name);
	answer->answer = ans;
	answer->cutype = cutype;
	return answer;
}

static void answer_free(Answer *answer)
{
	g_free(answer->attendee);
	g_free(answer->name);
	g_free(answer);
}

static GSList *answer_find(VCalEvent *event, Answer *answer)
{
	GSList *cur = event->answers;
	while (cur && cur->data) {
		Answer *b = (Answer *)cur->data;
		if (!strcmp(b->attendee, answer->attendee))
			return cur;
		cur = cur->next;
	}
	return NULL;
}

gchar *vcal_manager_answer_get_text(enum icalparameter_partstat ans) 
{
	static gchar *replies[5]={
		N_("accepted"),
		N_("tentatively accepted"),
		N_("declined"),
		N_("did not answer"),
		N_("unknown")
	};

	switch (ans) {
	case ICAL_PARTSTAT_ACCEPTED:
		return _(replies[0]);
		break;
	case ICAL_PARTSTAT_DECLINED:
		return _(replies[2]);
		break;
	case ICAL_PARTSTAT_TENTATIVE:
		return _(replies[1]);
		break;
	case ICAL_PARTSTAT_NEEDSACTION:
		return _(replies[3]);
	case ICAL_PARTSTAT_DELEGATED:
	case ICAL_PARTSTAT_COMPLETED:
	case ICAL_PARTSTAT_X:
	case ICAL_PARTSTAT_INPROCESS:
	case ICAL_PARTSTAT_NONE:
		return _(replies[4]);
		break;			
	}
	return NULL;
}

gchar *vcal_manager_cutype_get_text(enum icalparameter_cutype type) 
{
	static gchar *replies[5]={
		N_("individual"),
		N_("group"),
		N_("resource"),
		N_("room"),
		N_("unknown")
	};

	switch (type) {
	case ICAL_CUTYPE_INDIVIDUAL:
		return _(replies[0]);
		break;
	case ICAL_CUTYPE_GROUP:
		return _(replies[1]);
		break;
	case ICAL_CUTYPE_RESOURCE:
		return _(replies[2]);
		break;
	case ICAL_CUTYPE_ROOM:
		return _(replies[3]);
	default:
		return _(replies[4]);
		break;			
	}
	return NULL;
}

static GSList *answer_remove(VCalEvent *event, Answer *answer)
{
	GSList *cur = answer_find(event, answer);
	if (cur) {
		Answer *b = (Answer *)cur->data;
		event->answers = g_slist_remove(event->answers, b);
		answer_free(b);
	}
	return event->answers;
}

static GSList *answer_add(VCalEvent *event, Answer *answer)
{
	event->answers = g_slist_append(event->answers, answer);
	return event->answers;
}

GSList *vcal_manager_get_answers_emails(VCalEvent *event)
{
	GSList *new = NULL;
	GSList *cur = event->answers;
	while (cur && cur->data) {
		Answer *b = (Answer *)cur->data;
		new = g_slist_append(new, b->attendee);
		cur = cur->next;
	}
	return new;	
}

enum icalparameter_partstat vcal_manager_get_reply_for_attendee(VCalEvent *event, const gchar *att)
{
	Answer *a = answer_new(att, NULL, 0, 0);
	GSList *ans = answer_find(event, a);
	enum icalparameter_partstat res = 0;
	if (ans) {
		Answer *b = (Answer *)ans->data;
		res = b->answer;
	}
	answer_free(a);
	return res;
}

gchar *vcal_manager_get_cutype_text_for_attendee(VCalEvent *event, const gchar *att)
{
	enum icalparameter_cutype status = vcal_manager_get_cutype_for_attendee(event, att);
	gchar *res = NULL;
	if (status != 0) 
		res = g_strdup(vcal_manager_cutype_get_text(status));

	return res;
}

enum icalparameter_partstat vcal_manager_get_cutype_for_attendee(VCalEvent *event, const gchar *att)
{
	Answer *a = answer_new(att, NULL, 0, 0);
	GSList *ans = answer_find(event, a);
	enum icalparameter_cutype res = 0;
	if (ans) {
		Answer *b = (Answer *)ans->data;
		res = b->cutype;
	}
	answer_free(a);
	return res;
}

gchar *vcal_manager_get_reply_text_for_attendee(VCalEvent *event, const gchar *att)
{
	enum icalparameter_partstat status = vcal_manager_get_reply_for_attendee(event, att);
	gchar *res = NULL;
	if (status != 0) 
		res = g_strdup(vcal_manager_answer_get_text(status));

	return res;
}

gchar *vcal_manager_get_attendee_name(VCalEvent *event, const gchar *att)
{
	Answer *a = answer_new(att, NULL, 0, 0);
	GSList *ans = answer_find(event, a);
	gchar *res = NULL;
	if (ans) {
		Answer *b = (Answer *)ans->data;
		if (b->name)
			res = g_strdup(b->name);
	}
	answer_free(a);
	return res;
}

void vcal_manager_event_print(VCalEvent *event)
{
	GSList *list = event->answers;
	printf( "event->uid\t\t%s\n"
		"event->organizer\t\t%s\n"
		"event->start\t\t%s\n"
		"event->end\t\t%s\n"
		"event->summary\t\t%s\n"
		"event->description\t%s\n"
		"event->url\t%s\n"
		"event->dtstart\t\t%s\n"
		"event->dtend\t\t%s\n"
		"event->tzid\t\t%s\n"
		"event->method\t\t%d\n"
		"event->sequence\t\t%d\n",
		event->uid,
		event->organizer,
		event->start,
		event->end,
		event->summary,
		event->description,
		event->url,
		event->dtstart,
		event->dtend,
		event->tzid,
		event->method,
		event->sequence);
	while (list && list->data) {
		Answer *a = (Answer *)list->data;
		printf(" ans: %s %s, %s\n", a->name, a->attendee, vcal_manager_answer_get_text(a->answer));
		list = list->next;
	}
	
}

static gchar *write_headers(PrefsAccount 	*account, 
			    VCalEvent 		*event,
			    gboolean		 short_headers,
			    gboolean 		 is_reply, 
			    gboolean 		 is_pseudo_event);

gchar *vcal_manager_event_dump(VCalEvent *event, gboolean is_reply, gboolean is_pseudo_event,
				icalcomponent *use_calendar)
{
	gchar *organizer = g_strdup_printf("MAILTO:%s", event->organizer);
	PrefsAccount *account = vcal_manager_get_account_from_event(event);
	gchar *attendee  = NULL;
	gchar *body, *headers, *qpbody;
	gchar *tmpfile = NULL;
	gchar *tmpstr = NULL;
	icalcomponent *calendar, *ievent, *timezone, *tzc;
	struct icalperiodtype rtime;
	icalproperty *attprop;
	icalproperty *orgprop;
	icalparameter *param = NULL;
	enum icalparameter_partstat status = ICAL_PARTSTAT_NEEDSACTION;
	int i = 0;
	gchar *sanitized_uid = g_strdup(event->uid);
	
	subst_for_filename(sanitized_uid);

	tmpfile = g_strdup_printf("%s%cevt-%d-%s", g_get_tmp_dir(),
				      G_DIR_SEPARATOR, getuid(), sanitized_uid);
	g_free(sanitized_uid);

	if (!account) {
		g_free(organizer);
		g_free(tmpfile);
		debug_print("no account found\n");
		return NULL;
	}

	attendee = g_strdup_printf("MAILTO:%s", account->address);
	
	if (vcal_manager_get_reply_for_attendee(event, account->address) != 0)
		status = vcal_manager_get_reply_for_attendee(event, account->address);
	
	tzset();
	
	if (use_calendar != NULL) {
		calendar = use_calendar;
		g_free(tmpfile);
		tmpfile = NULL;
	} else
		calendar = 
        		icalcomponent_vanew(
        		    ICAL_VCALENDAR_COMPONENT,
	        	    icalproperty_new_version("2.0"),
        		    icalproperty_new_prodid(
                		 "-//Sylpheed-Claws//NONSGML Sylpheed-Claws Calendar//EN"),
			    icalproperty_new_calscale("GREGORIAN"),
			    icalproperty_new_method(is_reply ? ICAL_METHOD_REPLY:event->method),
        		    0
	        	    ); 	

	if (!calendar) {
		g_warning ("can't generate calendar");
		g_free(organizer);
		g_free(tmpfile);
		g_free(attendee);
		return NULL;
	}

	orgprop = icalproperty_new_organizer(organizer);
	tmpstr = vcal_manager_get_attendee_name(event, organizer);
	if (tmpstr) {
		param = icalparameter_new_cn(tmpstr);
		icalproperty_add_parameter(orgprop, param);
		g_free(tmpstr); tmpstr = NULL;
	} else if (event->orgname && strlen(event->orgname)) {
		param = icalparameter_new_cn(event->orgname);
		icalproperty_add_parameter(orgprop, param);
	}
	
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

	ievent = 
	    icalcomponent_vanew(
                ICAL_VEVENT_COMPONENT, 0);

	if (!ievent) {
		g_warning ("can't generate event");
		g_free(organizer);
		g_free(tmpfile);
		g_free(attendee);
		return NULL;
	}
	
	icalcomponent_add_property(ievent,
                icalproperty_new_uid(event->uid));
	icalcomponent_add_property(ievent,
		icalproperty_vanew_dtstamp(icaltime_from_timet(time(NULL), TRUE), 0));
	icalcomponent_add_property(ievent,
		icalproperty_vanew_dtstart((icaltime_from_string(event->dtstart)), 0));
	icalcomponent_add_property(ievent,
		icalproperty_vanew_dtend((icaltime_from_string(event->dtend)), 0));
	icalcomponent_add_property(ievent,
		icalproperty_new_description(event->description));
	icalcomponent_add_property(ievent,
		icalproperty_new_summary(event->summary));
	icalcomponent_add_property(ievent,
		icalproperty_new_sequence(event->sequence + 1));
	icalcomponent_add_property(ievent,
		icalproperty_new_class("PUBLIC"));
	icalcomponent_add_property(ievent,
		icalproperty_new_transp("OPAQUE"));
	icalcomponent_add_property(ievent,
		icalproperty_new_location(""));
	icalcomponent_add_property(ievent,
		icalproperty_new_status(ICAL_STATUS_CONFIRMED));
	icalcomponent_add_property(ievent,
		icalproperty_vanew_created(icaltime_from_timet(time(NULL), TRUE), 0));
	icalcomponent_add_property(ievent,
		icalproperty_vanew_lastmodified(icaltime_from_timet(time(NULL), TRUE), 0));
	icalcomponent_add_property(ievent,		
                orgprop);

	if (!icalcomponent_get_first_component(calendar, ICAL_VTIMEZONE_COMPONENT))
	        icalcomponent_add_component(calendar, timezone);

        icalcomponent_add_component(calendar, ievent);

	if (event->url && strlen(event->url)) {
		attprop = icalproperty_new_url(event->url);
        	icalcomponent_add_property(ievent, attprop);
	}

	if (is_reply) {
		/* dump only this attendee */
		attprop =
	                icalproperty_vanew_attendee(
                	    attendee,
                	    icalparameter_new_role(
                        	ICAL_ROLE_REQPARTICIPANT),
                	    icalparameter_new_rsvp(ICAL_RSVP_TRUE),
			    icalparameter_new_partstat(status),
                	    0
                	    );

		tmpstr = vcal_manager_get_attendee_name(event, attendee);
		if (tmpstr) {
			param = icalparameter_new_cn(tmpstr);
			icalproperty_add_parameter(attprop, param);
			g_free(tmpstr); tmpstr = NULL;
		}
			    
        	icalcomponent_add_property(ievent, attprop);
	} else {
		/* dump all attendees */
		GSList *cur = event->answers;
		while (cur && cur->data) {
			Answer *a = (Answer *)cur->data;

			if (a->cutype == 0)
				a->cutype = ICAL_CUTYPE_INDIVIDUAL;
			if (a->answer == 0)
				a->answer = ICAL_PARTSTAT_NEEDSACTION;

			attprop =
	                	icalproperty_vanew_attendee(
                		    a->attendee,
                		    icalparameter_new_role(
                        		ICAL_ROLE_REQPARTICIPANT),
                		    icalparameter_new_rsvp(ICAL_RSVP_TRUE),
                		    icalparameter_new_cutype(a->cutype),
				    icalparameter_new_partstat(a->answer),
                		    0
                		    );

			tmpstr = vcal_manager_get_attendee_name(event, a->attendee);
			if (tmpstr) {
				param = icalparameter_new_cn(tmpstr);
				icalproperty_add_parameter(attprop, param);
				g_free(tmpstr); tmpstr = NULL;
			}
			icalcomponent_add_property(ievent, attprop);
			cur = cur->next;
		}
	}

	if (use_calendar) {
		g_free(organizer);
		g_free(tmpfile);
		g_free(attendee);
		return NULL;
	}
	headers = write_headers(account, event, is_pseudo_event, is_reply, is_pseudo_event);

	if (!headers) {
		g_warning("can't get headers");
		g_free(organizer);
		g_free(tmpfile);
		g_free(attendee);
		return NULL;
	}

	body = g_strdup_printf("%s"
			       "\n"
			       "%s", headers, icalcomponent_as_ical_string(calendar));
	
	str_write_to_file(body, tmpfile); 
	chmod(tmpfile, S_IRUSR|S_IWUSR);

	g_free(body);
	g_free(headers);
	icalcomponent_free(calendar);
	g_free(attendee);
	g_free(organizer);
	
	return tmpfile;	
}

static gchar *write_headers_ical(PrefsAccount 	*account, 
			    icalcomponent	*ievent,
			    gchar		*orga);

gchar *vcal_manager_icalevent_dump(icalcomponent *event, gchar *orga, icalcomponent *use_calendar)
{
	PrefsAccount *account = cur_account;
	gchar *attendee  = NULL;
	gchar *body, *headers, *qpbody;
	gchar **lines = NULL;
	gchar *tmpfile = NULL;
	gchar *tmpstr = NULL;
	icalcomponent *calendar;
	icalproperty *attprop, *prop;
	icalproperty *orgprop;
	icalparameter *param = NULL;
	icalcomponent *ievent = NULL;
	
	enum icalparameter_partstat status = ICAL_PARTSTAT_NONE;
	int i = 0;

	ievent = icalcomponent_new_clone(event);

	prop = icalcomponent_get_first_property(ievent, ICAL_UID_PROPERTY);
	if (prop) {
		gchar *sanitized_uid = g_strdup(icalproperty_get_uid(prop));

		subst_for_filename(sanitized_uid);

		tmpfile = g_strdup_printf("%s%cevt-%d-%s", g_get_tmp_dir(),
					      G_DIR_SEPARATOR, getuid(), sanitized_uid);
		g_free(sanitized_uid);
		icalproperty_free(prop);
	} else {
		tmpfile = g_strdup_printf("%s%cevt-%d-%p", g_get_tmp_dir(),
				      G_DIR_SEPARATOR, getuid(), ievent);
	}

	if (!account) {
		g_free(tmpfile);
		icalcomponent_free(ievent);
		return NULL;
	}

	attendee = g_strdup_printf("MAILTO:%s", account->address);
	
	tzset();
	
	if (use_calendar != NULL) {
		calendar = use_calendar;
		g_free(tmpfile);
		tmpfile = NULL;
	} else
		calendar = 
        		icalcomponent_vanew(
        		    ICAL_VCALENDAR_COMPONENT,
	        	    icalproperty_new_version("2.0"),
        		    icalproperty_new_prodid(
                		 "-//Sylpheed-Claws//NONSGML Sylpheed-Claws Calendar//EN"),
			    icalproperty_new_calscale("GREGORIAN"),
			    icalproperty_new_method(ICAL_METHOD_PUBLISH),
        		    0
	        	    ); 	

	if (!calendar) {
		g_warning ("can't generate calendar");
		g_free(tmpfile);
		icalcomponent_free(ievent);
		return NULL;
	}
	
        icalcomponent_add_component(calendar, ievent);

	if (use_calendar)
		return NULL;

	headers = write_headers_ical(account, ievent, orga);

	if (!headers) {
		g_warning("can't get headers");
		g_free(tmpfile);
		icalcomponent_free(calendar);
		return NULL;
	}

	lines = g_strsplit(icalcomponent_as_ical_string(calendar), "\n", 0);
	qpbody = g_strdup("");
	
	/* encode to quoted-printable */
	while (lines[i]) {
		gchar buf[256];
		gint e_len = strlen(qpbody), n_len = 0;
		gchar *outline = conv_codeset_strdup(lines[i], CS_UTF_8, conv_get_outgoing_charset_str());
		
		qp_encode_line(buf, (guchar *)outline);
		n_len = strlen(buf);
		
		qpbody = g_realloc(qpbody, e_len + n_len + 1);
		strcpy(qpbody+e_len, buf);
		*(qpbody+n_len+e_len) = '\0';
		
		g_free(outline);
		i++;
	}
	
	body = g_strdup_printf("%s"
			       "\n"
			       "%s", headers, qpbody);
	
	str_write_to_file(body, tmpfile); 
	chmod(tmpfile, S_IRUSR|S_IWUSR);

	g_strfreev(lines);
	g_free(body);
	g_free(qpbody);
	g_free(headers);
	icalcomponent_free(calendar);
	g_free(attendee);
	
	return tmpfile;	
}

VCalEvent * vcal_manager_new_event	(const gchar 	*uid, 
					 const gchar	*organizer,
					 const gchar	*orgname,
					 const gchar	*summary,
					 const gchar	*description,
					 const gchar	*dtstart,
					 const gchar	*dtend,
					 const gchar	*tzid,
					 const gchar	*url,
					 enum icalproperty_method method,
					 gint 		 sequence,
					 enum icalproperty_kind type)
{
	VCalEvent *event = g_new0(VCalEvent, 1);
	
	event->uid 		= g_strdup(uid?uid:"");
	event->organizer 	= g_strdup(organizer?organizer:"");
	event->orgname	 	= g_strdup(orgname?orgname:"");

	if (dtend && strlen(dtend)) {
		time_t tmp = icaltime_as_timet((icaltime_from_string(dtend)));
		event->end	= g_strdup(ctime(&tmp));
	}
	
	if (dtstart && strlen(dtstart)) {
		time_t tmp = icaltime_as_timet((icaltime_from_string(dtstart)));
		time_t tmp_utc = icaltime_as_timet((icaltime_from_string(dtstart)));
		event->start	= g_strdup(ctime(&tmp));
	}
	event->dtstart		= g_strdup(dtstart?dtstart:"");
	event->dtend		= g_strdup(dtend?dtend:"");
	event->summary		= g_strdup(summary?summary:"");
	event->description	= g_strdup(description?description:"");
	event->url		= g_strdup(url?url:"");
	event->tzid		= g_strdup(tzid?tzid:"");
	event->method		= method;
	event->sequence		= sequence;
	event->type		= type;
	return event;
}
					 
void vcal_manager_free_event (VCalEvent *event)
{
	if (!event)
		return;
	g_free(event->uid);
	g_free(event->organizer);
	g_free(event->orgname);
	g_free(event->start);
	g_free(event->end);
	g_free(event->summary);
	g_free(event->dtstart);
	g_free(event->dtend);
	g_free(event->tzid);
	g_free(event->description);
	g_free(event->url);
	g_free(event);
}

gchar *vcal_manager_get_event_path(void)
{
	static gchar *event_path = NULL;
	if (!event_path)
		event_path = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S,
					"vcalendar", NULL);
	
	return event_path;
}

gchar *vcal_manager_get_event_file(const gchar *uid)
{
	gchar *tmp = g_strdup(uid);
	gchar *res = NULL;

	subst_for_filename(tmp);
	res = g_strconcat(vcal_manager_get_event_path(), G_DIR_SEPARATOR_S,
			   tmp, NULL);
	g_free(tmp);
	return res;
}

PrefsAccount *vcal_manager_get_account_from_event(VCalEvent *event)
{
	GSList *list = vcal_manager_get_answers_emails(event);
	GSList *cur = list;
	
	/* find an attendee corresponding to one of our accounts */
	while (cur && cur->data) {
		gchar *email = (gchar *)cur->data;
		if (account_find_from_address(email)) {
			g_slist_free(list);
			return account_find_from_address(email);
		}
		cur = cur->next;
	}
	g_slist_free(list);
	return NULL;
}

void vcal_manager_save_event (VCalEvent *event)
{
	XMLTag *tag = NULL;
	XMLNode *xmlnode = NULL;
	GNode *rootnode = NULL;
	PrefFile *pfile;
	gchar *path = NULL;
	GSList *answers = event->answers;
	gchar *tmp = NULL;
	gint tmp_method = event->method;
	
	tag = xml_tag_new("event");
	xml_tag_add_attr(tag, xml_attr_new("organizer", event->organizer));
	xml_tag_add_attr(tag, xml_attr_new("orgname", event->orgname));
	xml_tag_add_attr(tag, xml_attr_new("summary", event->summary));
	xml_tag_add_attr(tag, xml_attr_new("description", event->description));
	xml_tag_add_attr(tag, xml_attr_new("url", event->url));
	xml_tag_add_attr(tag, xml_attr_new("dtstart", event->dtstart));
	xml_tag_add_attr(tag, xml_attr_new("dtend", event->dtend));
	xml_tag_add_attr(tag, xml_attr_new("tzid", event->tzid));
	
	/* updating answers saves events, don't save them with reply type */
	if (tmp_method == ICAL_METHOD_REPLY)
		tmp_method = ICAL_METHOD_REQUEST;
	
	tmp = g_strdup_printf("%d", tmp_method);
	xml_tag_add_attr(tag, xml_attr_new("method", tmp));
	g_free(tmp);

	tmp = g_strdup_printf("%d", event->sequence);
	xml_tag_add_attr(tag, xml_attr_new("sequence", tmp));
	g_free(tmp);
	
	tmp = g_strdup_printf("%d", event->type);
	xml_tag_add_attr(tag, xml_attr_new("type", tmp));
	g_free(tmp);
	
	tmp = g_strdup_printf("%lu", event->postponed);
	xml_tag_add_attr(tag, xml_attr_new("postponed", tmp));
	g_free(tmp);
	
	xmlnode = xml_node_new(tag, NULL);
	rootnode = g_node_new(xmlnode);
	
	while (answers && answers->data) {
		XMLNode *ansxmlnode = NULL;
		GNode *ansnode = NULL;
		XMLTag *anstag = xml_tag_new("answer");
		Answer *a = (Answer *)answers->data;
		xml_tag_add_attr(anstag, xml_attr_new("attendee", a->attendee));
		xml_tag_add_attr(anstag, xml_attr_new("name", a->name?a->name:""));
		tmp = g_strdup_printf("%d", a->answer);
		xml_tag_add_attr(anstag, xml_attr_new("answer", tmp));
		g_free(tmp);
		tmp = g_strdup_printf("%d", a->cutype);
		xml_tag_add_attr(anstag, xml_attr_new("cutype", tmp));
		g_free(tmp);
		ansxmlnode = xml_node_new(anstag, NULL);
		ansnode = g_node_new(ansxmlnode);
		g_node_append(rootnode, ansnode);
		answers = answers->next;
	}
	
	path = vcal_manager_get_event_file(event->uid);
					
	if ((pfile = prefs_write_open(path)) == NULL) {
		make_dir(vcal_manager_get_event_path());
		if ((pfile = prefs_write_open(path)) == NULL) {
			free(path);
			return;
		}
	}
	
	free(path);
	xml_file_put_xml_decl(pfile->fp);
	xml_write_tree(rootnode, pfile->fp);
	xml_free_tree(rootnode);

	if (prefs_file_close(pfile) < 0) {
		g_warning("failed to write event.\n");
		return;
	}
 
	vcal_folder_export();
}

static VCalEvent *event_get_from_xml (const gchar *uid, GNode *node)
{
	XMLNode *xmlnode;
	GList *list;
	gchar *org = NULL, *summary = NULL, *orgname = NULL;
	gchar *dtstart = NULL, *dtend = NULL, *tzid = NULL, *description = NULL, *url = NULL;
	VCalEvent *event = NULL;
	enum icalproperty_method method = ICAL_METHOD_REQUEST;
	enum icalproperty_kind type = ICAL_VEVENT_COMPONENT;
	gint sequence = 0;
	time_t postponed = (time_t)0;
	
	g_return_val_if_fail(node->data != NULL, NULL);

	xmlnode = node->data;
	if (strcmp2(xmlnode->tag->tag, "event") != 0) {
		g_warning("tag name != \"event\"\n");
		return NULL;
	}
	
	list = xmlnode->tag->attr;
	for (; list != NULL; list = list->next) {
		XMLAttr *attr = list->data;

		if (!attr || !attr->name || !attr->value) continue;
		if (!strcmp(attr->name, "organizer"))
			org = g_strdup(attr->value);
		if (!strcmp(attr->name, "orgname"))
			orgname = g_strdup(attr->value);
		if (!strcmp(attr->name, "summary"))
			summary = g_strdup(attr->value);
		if (!strcmp(attr->name, "description"))
			description = g_strdup(attr->value);
		if (!strcmp(attr->name, "url"))
			url = g_strdup(attr->value);
		if (!strcmp(attr->name, "dtstart"))
			dtstart = g_strdup(attr->value);
		if (!strcmp(attr->name, "dtend"))
			dtend = g_strdup(attr->value);
		if (!strcmp(attr->name, "tzid"))
			tzid = g_strdup(attr->value);
		if (!strcmp(attr->name, "type"))
			type = atoi(attr->value);
		if (!strcmp(attr->name, "method"))
			method = atoi(attr->value);
		if (!strcmp(attr->name, "sequence"))
			sequence = atoi(attr->value);
		if (!strcmp(attr->name, "postponed"))
			postponed = atoi(attr->value);
	}

	event = vcal_manager_new_event(uid, org, orgname, summary, description, 
					dtstart, dtend, tzid, url, method, 
					sequence, type);

	event->postponed = postponed;

	g_free(org); 
	g_free(orgname); 
	g_free(summary); 
	g_free(description); 
	g_free(url); 
	g_free(dtstart); 
	g_free(dtend);
	g_free(tzid);

	node = node->children;
	while (node != NULL) {
		gchar *attendee = NULL;
		gchar *name = NULL;
		enum icalparameter_partstat answer = ICAL_PARTSTAT_NEEDSACTION;
		enum icalparameter_cutype cutype   = ICAL_CUTYPE_INDIVIDUAL;
		
		xmlnode = node->data;
		if (strcmp2(xmlnode->tag->tag, "answer") != 0) {
			g_warning("tag name != \"answer\"\n");
			return event;
		}
		list = xmlnode->tag->attr;
		for (; list != NULL; list = list->next) {
			XMLAttr *attr = list->data;

			if (!attr || !attr->name || !attr->value) continue;
			if (!strcmp(attr->name, "attendee"))
				attendee = g_strdup(attr->value);
			if (!strcmp(attr->name, "name"))
				name = g_strdup(attr->value);
			if (!strcmp(attr->name, "answer"))
				answer = atoi(attr->value);
			if (!strcmp(attr->name, "cutype"))
				cutype = atoi(attr->value);
		}

		event->answers = g_slist_append(event->answers, answer_new(attendee, name, answer, cutype));
		g_free(attendee);
		g_free(name);
		node = node->next;
	}
	
	return event;
}

VCalEvent *vcal_manager_load_event (const gchar *uid)
{
	GNode *node;
	gchar *path = NULL;
	VCalEvent *event = NULL;
	
	path = vcal_manager_get_event_file(uid);
	
	if (!is_file_exist(path)) {
		g_free(path);
		return NULL;
	}
	
	node = xml_parse_file(path);
	
	g_free(path);
	
	if (!node) {
		g_warning("no node\n");
		return NULL;
	}
	
	event = event_get_from_xml(uid, node);
	/* vcal_manager_event_print(event); */
	xml_free_tree(node);

	return event;
	
}

void vcal_manager_update_answer (VCalEvent 	*event, 
				 const gchar 	*attendee,
				 const gchar	*name,
				 enum icalparameter_partstat ans,
				 enum icalparameter_cutype cutype)
{
	Answer *answer = NULL;
	GSList *existing = NULL;
	Answer *existing_a = NULL;
	
	if (!ans)
		return;
		
	answer = answer_new(attendee, name, ans, cutype);
	existing = answer_find(event, answer);
		
	if (existing) {
		existing_a = (Answer *)existing->data;
	
		if (!answer->name && existing_a->name)
			answer->name = g_strdup(existing_a->name);
		if (!answer->cutype && existing_a->cutype)
			answer->cutype = existing_a->cutype;

		answer_remove(event, answer);
	}

	answer_add(event, answer);
	
	vcal_manager_save_event(event);
}

static void get_rfc822_date_from_time_t(gchar *buf, gint len, time_t t)
{
	struct tm *lt;
	gchar day[4], mon[4];
	gint dd, hh, mm, ss, yyyy;

	lt = localtime(&t);

	sscanf(asctime(lt), "%3s %3s %d %d:%d:%d %d\n",
	       day, mon, &dd, &hh, &mm, &ss, &yyyy);
	g_snprintf(buf, len, "%s, %d %s %d %02d:%02d:%02d %s",
		   day, dd, mon, yyyy, hh, mm, ss, tzoffset(&t));
}


static gchar *write_headers(PrefsAccount 	*account, 
			    VCalEvent 		*event,
			    gboolean		 short_headers,
			    gboolean 		 is_reply, 
			    gboolean 		 is_pseudo_display)
{
	gchar subject[512];
	gchar date[128];
	gchar *save_folder = NULL;
	gchar *result = NULL;
	gchar *queue_headers = NULL;
	gchar *method_str = NULL;
	gchar *attendees = NULL;
	enum icalparameter_partstat status;
	gchar *prefix = NULL;
	gchar enc_subject[512], enc_prefix[512], enc_from[512], *from = NULL;	
	memset(subject, 0, sizeof(subject));
	memset(date, 0, sizeof(date));

	conv_encode_header(subject, 511, event->summary, strlen("Subject: "), FALSE);
	
	if (is_pseudo_display) {
		struct icaltimetype itt = (icaltime_from_string(event->dtstart));
		struct icaltimetype itt_utc = (icaltime_from_string(event->dtstart));
		time_t t = icaltime_as_timet(itt);
		time_t t_utc = icaltime_as_timet(itt_utc);
		get_rfc822_date_from_time_t(date, sizeof(date), t);
	} else {
		get_rfc822_date(date, sizeof(date));
	}
	
	if (account_get_special_folder(account, F_OUTBOX)) {
		save_folder = folder_item_get_identifier(account_get_special_folder
				  (account, F_OUTBOX));
	}
	
	if (!is_reply) {
		GSList *cur = event->answers;
		while (cur && cur->data) {
			gchar *tmp = NULL;
			Answer *a = (Answer *)cur->data;
			
			if (strcmp(a->attendee, event->organizer)) {
				if (attendees) {
					tmp = g_strdup(attendees);
					g_free(attendees);
					attendees = g_strdup_printf("%s>,<%s", tmp, a->attendee);
				} else {
					attendees = g_strdup_printf("%s", a->attendee);
				}
			}
			cur = cur->next;
		}
	}
	
	if (!short_headers) {
		queue_headers = g_strdup_printf("S:%s\n"
				"SSV:%s\n"
				"R:<%s>\n"
				"MAID:%d\n"
				"%s%s%s"
				"X-Sylpheed-End-Special-Headers: 1\n",
				account->address,
				account->smtp_server,
				is_reply ? event->organizer:attendees,
				account->account_id,
				save_folder?"SCF:":"",
				save_folder?save_folder:"",
				save_folder?"\n":"");
	} else {
		queue_headers = g_strdup("");
	}
	
	prefix = "";
	if (is_reply) {
		method_str = "REPLY";
		status = vcal_manager_get_reply_for_attendee(event, account->address);
		if (status == ICAL_PARTSTAT_ACCEPTED)
			prefix = _("Accepted: ");
		else if (status == ICAL_PARTSTAT_DECLINED)
			prefix = _("Declined: ");
		else if (status == ICAL_PARTSTAT_TENTATIVE)
			prefix = _("Tentatively Accepted: ");
		else 
			prefix = "Re: ";
	} else if (event->method == ICAL_METHOD_PUBLISH)
		method_str = "PUBLISH";
	else
		method_str = "REQUEST";		
	
	conv_encode_header_full(enc_prefix, sizeof(enc_prefix), prefix, strlen("Subject: "), 
			FALSE, conv_get_outgoing_charset_str());
	conv_encode_header_full(enc_subject, sizeof(enc_subject), subject, strlen("Subject: "), 
			FALSE, conv_get_outgoing_charset_str());
	from = is_reply?account->name:(event->orgname?event->orgname:"");
	conv_encode_header_full(enc_from, sizeof(enc_from), from, strlen("From: "), 
			TRUE, conv_get_outgoing_charset_str());

	result = g_strdup_printf("%s"
				"From: %s <%s>\n"
				"To: <%s>\n"
				"Subject: %s%s\n"
				"Date: %s\n"
				"MIME-Version: 1.0\n"
				"Content-Type: text/calendar; method=%s; charset=\"%s\"\n"
				"Content-Transfer-Encoding: 8bit\n",
				queue_headers,
				enc_from,
				is_reply ? account->address:event->organizer,
				is_reply ? event->organizer:(attendees?attendees:event->organizer),
				enc_prefix,
				enc_subject,
				date,
				method_str,
				CS_UTF_8);
	
	g_free(save_folder);
	g_free(queue_headers);
	g_free(attendees);
	return result;			
                                                                               

}

static gchar *write_headers_ical(PrefsAccount 	*account, 
			    icalcomponent	*ievent,
			    gchar 		*orga)
{
	gchar subject[512];
	gchar date[128];
	gchar *result = NULL;
	gchar *method_str = NULL;
	gchar *attendees = NULL;
	gchar *summary = NULL;
	gchar *organizer = NULL;
	gchar *orgname = NULL;
	icalproperty *prop = NULL;
	time_t t;

	memset(subject, 0, sizeof(subject));
	memset(date, 0, sizeof(date));
	
	prop = icalcomponent_get_first_property(ievent, ICAL_SUMMARY_PROPERTY);
	if (prop) {
		summary = g_strdup(icalproperty_get_summary(prop));
		icalproperty_free(prop);
	} else {
		summary = g_strdup("");
	}

	prop = icalcomponent_get_first_property(ievent, ICAL_ORGANIZER_PROPERTY);
	if (prop) {
		organizer = g_strdup(icalproperty_get_organizer(prop));
		if (icalproperty_get_parameter_as_string(prop, "CN") != NULL)
			orgname = g_strdup(icalproperty_get_parameter_as_string(prop, "CN"));

		icalproperty_free(prop);
	} else {
		organizer = orga? g_strdup(orga):g_strdup("");
	}

	prop = icalcomponent_get_first_property(ievent, ICAL_DTSTART_PROPERTY);
	if (prop) {
		t = icaltime_as_timet(icalproperty_get_dtstart(prop));
		get_rfc822_date_from_time_t(date, sizeof(date), t);
	} else {
		get_rfc822_date(date, sizeof(date));
	}

	conv_encode_header(subject, 511, summary, strlen("Subject: "), FALSE);
			
	method_str = "PUBLISH";
					
	result = g_strdup_printf("From: %s <%s>\n"
				"To: <%s>\n"
				"Subject: %s%s\n"
				"Date: %s\n"
				"MIME-Version: 1.0\n"
				"Content-Type: text/calendar; method=%s; charset=\"%s\"; vcalsave=\"no\"\n"
				"Content-Transfer-Encoding: quoted-printable\n",
				orgname?orgname:"",
				!strncmp(organizer, "MAILTO:", 7) ? organizer+7 : organizer,
				account->address,
				"",
				subject,
				date,
				method_str,
				conv_get_outgoing_charset_str());
	
	g_free(orgname);
	g_free(organizer);
	g_free(summary);

	return result;			
                                                                               

}

static gboolean vcal_manager_send (PrefsAccount 	*account, 
				     VCalEvent 		*event,
				     gboolean		 is_reply)
{
	gchar *tmpfile = NULL;
	gint msgnum;
	FolderItem *folderitem;
	gchar *msgpath = NULL;
	Folder *folder = NULL;
	
	tmpfile = vcal_manager_event_dump(event, is_reply, FALSE, NULL);

	if (!tmpfile)
		return FALSE;

	folderitem = account_get_special_folder(account, F_QUEUE);
	if (!folderitem) {
		g_warning("can't find queue folder for %s\n", account->address);
		unlink(tmpfile);
		g_free(tmpfile);
		return FALSE;
	}
	folder_item_scan(folderitem);
	
	if ((msgnum = folder_item_add_msg(folderitem, tmpfile, NULL, TRUE)) < 0) {
		g_warning("can't queue the message\n");
		unlink(tmpfile);
		g_free(tmpfile);
		return FALSE;
	}

	msgpath = folder_item_fetch_msg(folderitem, msgnum);
	
	if (!prefs_common.work_offline) {
		gchar *err = NULL;
		gint val = procmsg_send_message_queue(msgpath, &err, folderitem, msgnum);
		if (val == 0) {
			folder_item_remove_msg(folderitem, msgnum);
			folder_item_scan(folderitem);
		} else if (err) {
			alertpanel_error_log(err);
			g_free(err);
		}
	}
	unlink(tmpfile);
	g_free(tmpfile);
	g_free(msgpath);

	folder = folder_find_from_name ("vCalendar", vcal_folder_get_class());
	if (folder)
		folder_item_scan(folder->inbox);

	vcalviewer_reload();
	return TRUE;
}

gboolean vcal_manager_reply (PrefsAccount 	*account, 
			     VCalEvent 		*event)
{
	return vcal_manager_send(account, event, TRUE);
}

gboolean vcal_manager_request (PrefsAccount 	*account, 
			       VCalEvent 	*event)
{
	return vcal_manager_send(account, event, FALSE);
}

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

#ifndef __VCAL_MANAGER_H__
#define __VCAL_MANAGER_H__

#include <glib.h>
#include <ical.h>
#include "prefs_account.h"

typedef struct _VCalEvent VCalEvent;

struct _VCalEvent 
{
	gchar *uid;
	gchar *organizer;
	gchar *start;
	gchar *end;
	gchar *dtstart;
	gchar *dtend;
	gchar *tzid;
	gchar *summary;
	gchar *description;
	GSList *answers;
	enum icalproperty_method method;
	gint sequence;	
};

VCalEvent *vcal_manager_new_event	(const gchar 	*uid, 
					 const gchar	*organizer,
					 const gchar	*summary,
					 const gchar	*description,
					 const gchar	*dtstart,
					 const gchar	*dtend,
					 const gchar	*tzid,
					 enum icalproperty_method method,
					 gint		 sequence);
					 
void vcal_manager_free_event (VCalEvent *event);
void vcal_manager_save_event (VCalEvent *event);
void vcal_manager_update_answer (VCalEvent 	*event, 
				 const gchar 	*attendee,
				 const gchar 	*name,
				 enum icalparameter_partstat ans,
				 enum icalparameter_cutype cutype);

VCalEvent *vcal_manager_load_event (const gchar *uid);
gboolean vcal_manager_reply (PrefsAccount 	*account, 
			     VCalEvent 		*event);
gboolean vcal_manager_request (PrefsAccount 	*account, 
			       VCalEvent 	*event);

GSList *vcal_manager_get_answers_emails(VCalEvent *event);
gchar *vcal_manager_get_attendee_name(VCalEvent *event, const gchar *attendee);
gchar *vcal_manager_get_reply_text_for_attendee(VCalEvent *event, const gchar *att);
gchar *vcal_manager_get_cutype_text_for_attendee(VCalEvent *event, const gchar *att);
enum icalparameter_partstat vcal_manager_get_reply_for_attendee(VCalEvent *event, const gchar *att);
enum icalparameter_partstat vcal_manager_get_cutype_for_attendee(VCalEvent *event, const gchar *att);
gchar *vcal_manager_get_event_path(void);
gchar *vcal_manager_get_event_file(const gchar *uid);
gchar *vcal_manager_event_dump(VCalEvent *event, gboolean change_date, gboolean change_from);

gchar *vcal_manager_answer_get_text(enum icalparameter_partstat ans);
gchar *vcal_manager_cutype_get_text(enum icalparameter_cutype type);

PrefsAccount *vcal_manager_get_account_from_event(VCalEvent *event);

void vcal_manager_event_print(VCalEvent *event);

#endif

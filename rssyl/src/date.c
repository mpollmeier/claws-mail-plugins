/**
 * @file common.c common routines for Liferea
 * 
 * Copyright (C) 2003-2005  Lars Lindner <lars.lindner@gmx.net>
 * Copyright (C) 2004,2005  Nathan J. Conrad <t98502@users.sourceforge.net>
 * Copyright (C) 2004       Karl Soderstrom <ks@xanadunet.net>
 *
 * parts of the RFC822 timezone decoding were taken from the gmime 
 * source written by 
 *
 * Authors: Michael Zucchi <notzed@helixcode.com>
 *          Jeffrey Stedfast <fejj@helixcode.com>
 *
 * Copyright 2000 Helix Code, Inc. (www.helixcode.com)
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/* this is needed for strptime() */
#define _XOPEN_SOURCE /* glibc2 needs this */

#include <time.h>
#include <glib.h>
#include <locale.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

/* converts a ISO 8601 time string to a time_t value */
time_t parseISO8601Date(gchar *date) {
	struct tm	tm;
	time_t		t, t2, offset = 0;
	gboolean	success = FALSE;
	gchar *pos;
	
	g_assert(date != NULL);
	
	memset(&tm, 0, sizeof(struct tm));
	
	/* we expect at least something like "2003-08-07T15:28:19" and
	   don't require the second fractions and the timezone info

	   the most specific format:   YYYY-MM-DDThh:mm:ss.sTZD
	 */
	 
	/* full specified variant */
	if(NULL != (pos = strptime((const char *)date, "%t%Y-%m-%dT%H:%M%t", &tm))) {
		/* Parse seconds */
		if (*pos == ':')
			pos++;
		if (isdigit(pos[0]) && !isdigit(pos[1])) {
			tm.tm_sec = pos[0] - '0';
			pos++;
		} else if (isdigit(pos[0]) && isdigit(pos[1])) {
			tm.tm_sec = 10*(pos[0]-'0') + pos[1] - '0';
			pos +=2;
		}
		/* Parse timezone */
		if (*pos == 'Z')
			offset = 0;
		else if ((*pos == '+' || *pos == '-') && isdigit(pos[1]) && isdigit(pos[2]) && strlen(pos) >= 3) {
			offset = (10*(pos[1] - '0') + (pos[2] - '0')) * 60 * 60;
			
			if (pos[3] == ':' && isdigit(pos[4]) && isdigit(pos[5]))
				offset +=  (10*(pos[4] - '0') + (pos[5] - '0')) * 60;
			else if (isdigit(pos[3]) && isdigit(pos[4]))
				offset +=  (10*(pos[3] - '0') + (pos[4] - '0')) * 60;
			
			offset *= (pos[0] == '+') ? 1 : -1;

		}
		success = TRUE;
	/* only date */
	} else if(NULL != strptime((const char *)date, "%t%Y-%m-%d", &tm))
		success = TRUE;
	/* there were others combinations too... */

	if(TRUE == success) {
		if((time_t)(-1) != (t = mktime(&tm))) {
			/* Correct for the local timezone*/
			t = t - offset;
			t2 = mktime(gmtime(&t));
			t = t - (t2 - t);
			
			return t;
		} else {
			g_warning("internal error! time conversion error! mktime failed!\n");
		}
	} else {
		g_warning("Invalid ISO8601 date format! Ignoring <dc:date> information!\n");
	}
	
	return 0;
}

gchar *dayofweek[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
gchar *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

gchar *createRFC822Date(const time_t *time) {
	struct tm *tm;

	tm = gmtime(time); /* No need to free because it is statically allocated */
	return g_strdup_printf("%s, %2d %s %4d %02d:%02d:%02d GMT", dayofweek[tm->tm_wday], tm->tm_mday,
					   months[tm->tm_mon], 1900 + tm->tm_year, tm->tm_hour, tm->tm_min, tm->tm_sec);
}

/* this table of RFC822 timezones is from gmime-utils.c of the gmime API */
static struct {
	char *name;
	int offset;
} tz_offsets [] = {
	{ "UT", 0 },
	{ "GMT", 0 },
	{ "EST", -500 },        /* these are all US timezones.  bloody yanks */
	{ "EDT", -400 },
	{ "CST", -600 },
	{ "CDT", -500 },
	{ "MST", -700 },
	{ "MDT", -600 },
	{ "PST", -800 },
	{ "PDT", -700 },
	{ "Z", 0 },
	{ "A", -100 },
	{ "M", -1200 },
	{ "N", 100 },
	{ "Y", 1200 }
};
/** @returns timezone offset in seconds */
static time_t common_parse_rfc822_tz(char *token) {
	int offset = 0;
	const char *inptr = token;
	
	if (*inptr == '+' || *inptr == '-') {
		offset = atoi (inptr);
	} else {
		int t;

		if (*inptr == '(')
			inptr++;

		for (t = 0; t < 15; t++)
			if (!strncmp (inptr, tz_offsets[t].name, strlen (tz_offsets[t].name)))
				offset = tz_offsets[t].offset;
	}
	
	return 60 * ((offset / 100) * 60 + (offset % 100));
}


/* converts a RFC822 time string to a time_t value */
time_t parseRFC822Date(gchar *date) {
	struct tm	tm;
	time_t		t, t2;
	char 		*oldlocale;
	char		*pos;
	gboolean	success = FALSE;

	memset(&tm, 0, sizeof(struct tm));

	/* we expect at least something like "03 Dec 12 01:38:34" 
	   and don't require a day of week or the timezone

	   the most specific format we expect:  "Fri, 03 Dec 12 01:38:34 CET"
	 */
	/* skip day of week */
	if(NULL != (pos = g_utf8_strchr(date, -1, ',')))
		date = ++pos;

	/* we expect English month names, so we set the locale */
	oldlocale = g_strdup(setlocale(LC_TIME, NULL));
	setlocale(LC_TIME, "C");
	
	/* standard format with 4 digit year */
	if(NULL != (pos = strptime((const char *)date, " %d %b %Y %T", &tm)))
		success = TRUE;
	/* non-standard format with 2 digit year */
	else if(NULL != (pos = strptime((const char *)date, " %d %b %y %T", &tm)))
		success = TRUE;
	
	while(pos != NULL && *pos != '\0' && isspace((int)*pos))       /* skip whitespaces before timezone */
		pos++;
	
	if(NULL != oldlocale) {
		setlocale(LC_TIME, oldlocale);	/* and reset it again */
		g_free(oldlocale);
	}
	
	if(TRUE == success) {
		if((time_t)(-1) != (t = mktime(&tm))) {
			/* GMT time, with no daylight savings time
			   correction. (Usually, there is no daylight savings
			   time since the input is GMT.) */
			t = t - common_parse_rfc822_tz(pos);
			t2 = mktime(gmtime(&t));
			t = t - (t2 - t);
			return t;
		} else
			g_warning("internal error! time conversion error! mktime failed!\n");
	}
	
	return 0;
}

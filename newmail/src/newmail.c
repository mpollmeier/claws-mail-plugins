/*
 * newmail - A plugin for Sylpheed-Claws
 *
 * Copyright (C) 2005-2005 H.Merijn Brand and the Sylpheed-Claws Team
 *
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2002 Hiroyuki Yamamoto and the Sylpheed-Claws Team
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

#include <errno.h>

#include <glib.h>
#include <glib/gi18n.h>

#include "version.h"
#include "sylpheed.h"
#include "plugin.h"
#include "utils.h"
#include "hooks.h"
#include "procmsg.h"

#include "plugin_version.h"

static guint hook_id;

static FILE *NewLog   = NULL;
static char *LogName  = NULL;
static int   truncLog = 1;

static gchar *defstr (gchar *s)
{
    return s ? s : "(null)";
    } /* defstr */

gboolean newmail_hook (gpointer source, gpointer data)
{
    auto MsgInfo    *msginfo = (MsgInfo *)source;
    auto FolderItem *tof;

    if (!msginfo) return (FALSE);

    tof = msginfo->to_folder;
    (void)fprintf (NewLog, "---\n"
	"Date:\t%s\n"
	"Subject:\t%s\n"
	"From:\t%s\n"
	"To:\t%s\n"
	"Cc:\t%s\n"
	"Size:\t%ld\n"
	"Path:\t%s\n"
	"Message:\t%d\n"
	"Folder:\t%s\n",
	    defstr (msginfo->date),
	    defstr (msginfo->subject),
	    defstr (msginfo->from),
	    defstr (msginfo->to),
	    defstr (msginfo->cc),
	    msginfo->size,
	    defstr (procmsg_get_message_file_path (msginfo)),
	    msginfo->msgnum,
	    tof ? defstr (tof->name) : "(null)");

    return (FALSE);
    } /* newmail_hook */

void plugin_done (void)
{
    if (NewLog) {
	(void)fclose (NewLog);
	NewLog  = NULL;
	LogName = NULL;
	}
    hooks_unregister_hook (MAIL_POSTFILTERING_HOOKLIST, hook_id);

    printf ("Newmail plugin unloaded\n");
    } /* plugin_done */

gint plugin_init (gchar **error)
{
    if ((sylpheed_get_version () > VERSION_NUMERIC)) {
	*error = g_strdup ("Your sylpheed version is newer than "
			   "the version the plugin was built with");
	return (-1);
	}

    if ((sylpheed_get_version () < MAKE_NUMERIC_VERSION (1, 9, 13, 25))) {
	*error = g_strdup ("Your sylpheed version is too old");
	return (-1);
	}

    hook_id = hooks_register_hook (MAIL_POSTFILTERING_HOOKLIST, newmail_hook, NULL);
    if (hook_id == -1) {
	*error = g_strdup ("Failed to register newmail hook");
	return (-1);
	}

    if (!NewLog) {
	auto char *mode = truncLog ? "w" : "a";
	if (!LogName) {
	    auto size_t l;
	    auto char   name[260];
	    (void)snprintf (name, 256, "%s/Mail/NewLog", getenv ("HOME"));
	    l = strlen (name);
	    if (l > 255 || !(LogName = (char *)malloc (l + 1))) {
		*error = g_strdup ("Cannot load plugin NewMail\n"
				   "$HOME is too long\n");
		plugin_done ();
		return (-1);
		}
	    (void)strcpy (LogName, name);
	    }
	if (!(NewLog = fopen (LogName, mode))) {
	    *error = g_strdup (sys_errlist[errno]);
	    plugin_done ();
	    return (-1);
	    }
	setbuf (NewLog, NULL);
	}

    printf ("Newmail plugin loaded\n"
            "Message header summaries written to %s\n", LogName);
    return (0);
    } /* plugin_init */

const gchar *plugin_name (void)
{
    return _("NewMail");
    } /* plugin_name */

const gchar *plugin_desc (void)
{
    return _ ("This Plugin writes a header summary to a log file for each "
	      "mail received after sorting.\n\n"
	      "Default is ~/Mail/NewLog");
    } /* plugin_desc */

const gchar *plugin_type (void)
{
    return ("Common");
    } /* plugin_type */

const gchar *plugin_licence (void)
{
    return ("GPL");
} /* plugin_licence */

const gchar *plugin_version(void)
{
	return PLUGINVERSION;
}


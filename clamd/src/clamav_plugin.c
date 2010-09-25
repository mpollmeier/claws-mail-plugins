/*
 * Claws Mail -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 2003-2008 Michael Rasmussen and the Claws Mail Team
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "defs.h"

#include <glib.h>
#include <glib/gi18n.h>

#include "common/claws.h"
#include "common/version.h"
#include "plugin.h"
#include "utils.h"
#include "hooks.h"
#include "inc.h"
#include "mimeview.h"
#include "folder.h"
#include "prefs.h"
#include "prefs_gtk.h"
#include "alertpanel.h"

#include "clamav_plugin.h"
#include "clamd-plugin.h"

#define PLUGIN_NAME (_("Clam AntiVirus"))

static guint hook_id;
static MessageCallback message_callback;

static ClamAvConfig config;

static PrefParam param[] = {
	{"clamav_enable", "FALSE", &config.clamav_enable, P_BOOL,
	 NULL, NULL, NULL},
/*	{"clamav_enable_arc", "FALSE", &config.clamav_enable_arc, P_BOOL,
	 NULL, NULL, NULL},*/
	{"clamav_max_size", "1", &config.clamav_max_size, P_USHORT,
	 NULL, NULL, NULL},
	{"clamav_recv_infected", "TRUE", &config.clamav_recv_infected, P_BOOL,
	 NULL, NULL, NULL},
	{"clamav_save_folder", NULL, &config.clamav_save_folder, P_STRING,
	 NULL, NULL, NULL},
	{"clamd_config_folder", NULL, &config.clamd_config_folder, P_STRING,
	 NULL, NULL, NULL},

	{NULL, NULL, NULL, P_OTHER, NULL, NULL, NULL}
};

struct clamd_result {
	Clamd_Stat status;
};

static gboolean scan_func(GNode *node, gpointer data)
{
	struct clamd_result *result = (struct clamd_result *) data;
	MimeInfo *mimeinfo = (MimeInfo *) node->data;
	gchar *outfile;
	response buf;
	int max;
	struct stat info;

	outfile = procmime_get_tmp_file_name(mimeinfo);
	if (procmime_get_part(outfile, mimeinfo) < 0)
		g_warning("Can't get the part of multipart message.");
	else {
    	max = config.clamav_max_size * 1048576; /* maximum file size */
		if (stat(outfile, &info) == -1)
			g_warning("Can't determine file size");
		else {
			if (info.st_size <= max) {
				debug_print("Scanning %s\n", outfile);
				result->status = clamd_verify_email(outfile, &buf);
				debug_print("status: %d\n", result->status);
				switch (result->status) {
					case NO_SOCKET: 
						g_warning("[scanning] No socket information");
						alertpanel_error(_("Scanning\nNo socket information.\nAntivirus disabled."));
						break;
					case NO_CONNECTION:
						g_warning("[scanning] Clamd does not respond to ping");
						alertpanel_warning(_("Scanning\nClamd does not respond to ping.\nIs clamd running?"));
						break;
					case VIRUS: 
						debug_print("Detected %s virus.\n", clamd_get_virus_name(buf.msg));
						break;
					case SCAN_ERROR:
						debug_print("Error: %s\n", buf.msg);
						alertpanel_error(_("Scanning error:\n%s"), buf.msg);
						break;
					case OK:
						debug_print("No virus detected.\n");
						break;
				}
			}
			else {
				debug_print("File: %s. Size (%d) greater than limit (%d)\n",
							outfile, (int) info.st_size, max);
			}
		}
		g_unlink(outfile);
	}

	return (result->status == OK) ? FALSE : TRUE;
}

static gboolean mail_filtering_hook(gpointer source, gpointer data)
{
	MailFilteringData *mail_filtering_data = (MailFilteringData *) source;
	MsgInfo *msginfo = mail_filtering_data->msginfo;
	MimeInfo *mimeinfo;

	struct clamd_result result;

	if (!config.clamav_enable)
		return FALSE;

	mimeinfo = procmime_scan_message(msginfo);
	if (!mimeinfo) return FALSE;

	debug_print("Scanning message %d for viruses\n", msginfo->msgnum);
	if (message_callback != NULL)
		message_callback(_("ClamAV: scanning message..."));

	debug_print("status: %d\n", result.status);
	g_node_traverse(mimeinfo->node, G_PRE_ORDER, G_TRAVERSE_ALL, -1, scan_func, &result);

	if (result.status == VIRUS) {
		if (config.clamav_recv_infected) {
			FolderItem *clamav_save_folder;

			if ((!config.clamav_save_folder) ||
			    (config.clamav_save_folder[0] == '\0') ||
			    ((clamav_save_folder = folder_find_item_from_identifier(config.clamav_save_folder)) == NULL))
				    clamav_save_folder = folder_get_default_trash();

			procmsg_msginfo_unset_flags(msginfo, ~0, 0);
			msginfo->filter_op = IS_MOVE;
			msginfo->to_filter_folder = clamav_save_folder;
		} else {
			folder_item_remove_msg(msginfo->folder, msginfo->msgnum);
		}
	}
	
	procmime_mimeinfo_free_all(mimeinfo);
	
	return (result.status == OK) ? FALSE : TRUE;
}

ClamAvConfig *clamav_get_config(void)
{
	return &config;
}

void clamav_save_config(void)
{
	PrefFile *pfile;
	gchar *rcpath;

	debug_print("Saving Clamd Page\n");

	rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, COMMON_RC, NULL);
	pfile = prefs_write_open(rcpath);
	g_free(rcpath);
	if (!pfile || (prefs_set_block_label(pfile, "ClamAV") < 0))
		return;

	if (prefs_write_param(param, pfile->fp) < 0) {
		g_warning("failed to write Clamd configuration to file\n");
		prefs_file_close_revert(pfile);
		return;
	}
        if (fprintf(pfile->fp, "\n") < 0) {
		FILE_OP_ERROR(rcpath, "fprintf");
		prefs_file_close_revert(pfile);
	} else
	        prefs_file_close(pfile);
}

void clamav_set_message_callback(MessageCallback callback)
{
	message_callback = callback;
}

gint plugin_init(gchar **error)
{
	gchar *rcpath;

	if (!check_plugin_version(MAKE_NUMERIC_VERSION(2,9,2,72),
				VERSION_NUMERIC, PLUGIN_NAME, error))
		return -1;

	hook_id = hooks_register_hook(MAIL_FILTERING_HOOKLIST, mail_filtering_hook, NULL);
	if (hook_id == -1) {
		*error = g_strdup(_("Failed to register mail filtering hook"));
		return -1;
	}

	prefs_set_default(param);
	rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, COMMON_RC, NULL);
	prefs_read_config(param, "ClamAV", rcpath, NULL);
	g_free(rcpath);

	clamav_gtk_init();

	if (config.clamav_enable) {
		debug_print("Creating socket\n");
		if (config.clamd_config_folder != NULL) {
			debug_print("clamd.conf: %s\n", config.clamd_config_folder);
			clamd_create_config(config.clamd_config_folder);
			Clamd_Stat status = clamd_init(NULL);
			switch (status) {
				case NO_SOCKET: 
					g_warning("[init] No socket information");
					alertpanel_error(_("Init\nNo socket information.\nAntivirus disabled."));
					break;
				case NO_CONNECTION:
					g_warning("[init] Clamd does not respond to ping");
					alertpanel_warning(_("Init\nClamd does not respond to ping.\nIs clamd running?"));
					break;
				default:
					break;
			}
		}
		else {
			if (clamd_find_socket()) {
				Clamd_Stat status = clamd_init(NULL);
				switch (status) {
					case NO_SOCKET: 
						g_warning("[init] No socket information");
						alertpanel_error(_("Init\nNo socket information.\nAntivirus disabled."));
						break;
					case NO_CONNECTION:
						g_warning("[init] Clamd does not respond to ping");
						alertpanel_warning(_("Init\nClamd does not respond to ping.\nIs clamd running?"));
						break;
					default:
						break;
				}
			}
		}
	}

	debug_print("Clamd plugin loaded\n");

	return 0;
	
}

gboolean plugin_done(void)
{
	hooks_unregister_hook(MAIL_FILTERING_HOOKLIST, hook_id);
	g_free(config.clamav_save_folder);
	clamav_gtk_done();
	clamd_free();

	debug_print("Clamd plugin unloaded\n");
	return TRUE;
}

const gchar *plugin_name(void)
{
	return PLUGIN_NAME;
}

const gchar *plugin_desc(void)
{
	return _("This plugin uses Clam AntiVirus to scan all messages that are "
	       "received from an IMAP, LOCAL or POP account.\n"
	       "\n"
	       "When a message attachment is found to contain a virus it can be "
	       "deleted or saved in a specially designated folder.\n"
	       "\n"
		   "Because this plugin communicates with clamd via a socket then "
		   "there are some minimum requirements to the permissions for your "
		   "home folder and the .claws-mail folder. All users at least need "
		   "to be given execute permissions on these folders.\n"
		   "\n"
	       "Options can be found in /Configuration/Preferences/Plugins/Clam AntiVirus");
}

const gchar *plugin_type(void)
{
	return "GTK2";
}

const gchar *plugin_licence(void)
{
	return "GPL3+";
}

const gchar *plugin_version(void)
{
	return PLUGINVERSION;
}

struct PluginFeature *plugin_provides(void)
{
	static struct PluginFeature features[] = 
		{ {PLUGIN_FILTERING, N_("Virus detection")},
		  {PLUGIN_NOTHING, NULL}};
	return features;
}

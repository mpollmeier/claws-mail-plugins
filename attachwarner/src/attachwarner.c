/*
 * Claws Mail -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2007 Hiroyuki Yamamoto and the Claws Mail Team
 * Copyright (C) 2006-2007 Ricardo Mones
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
 * along with this program; if not, write to the Free Software Foundation, 
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "attachwarner.h"
#include "attachwarner_prefs.h"
#include "codeconv.h"
#include "prefs_common.h"

/** Identifier for the hook. */
static guint hook_id;

/**
 * Builds a single regular expresion from an array of srings.
 *
 * @param strings The lines containing the different sub-regexp.
 *
 * @return The newly allocated regexp.
 */
static gchar *build_complete_regexp(gchar **strings)
{
	int i = 0;
	gchar *expr = NULL;
	while (strings && strings[i] && *strings[i]) {
		int old_len = expr ? strlen(expr):0;
		int new_len = 0;
		gchar *tmpstr = NULL;

		if (g_utf8_validate(strings[i], -1, NULL))
			tmpstr = g_strdup(strings[i]);
		else
			tmpstr = conv_codeset_strdup(strings[i], 
					conv_get_locale_charset_str_no_utf8(),
				 	CS_INTERNAL);

		if (strstr(tmpstr, "\n"))
			*(strstr(tmpstr, "\n")) = '\0';

		new_len = strlen(tmpstr);

		expr = g_realloc(expr, 
			expr ? (old_len + strlen("|()") + new_len + 1)
			     : (strlen("()") + new_len + 1));
		
		if (old_len) {
			strcpy(expr + old_len, "|(");
			strcpy(expr + old_len + 2, tmpstr);
			strcpy(expr + old_len + 2 + new_len, ")");
		} else {
			strcpy(expr+old_len, "(");
			strcpy(expr+old_len + 1, tmpstr);
			strcpy(expr+old_len + 1 + new_len, ")");
		}
		g_free(tmpstr);
		i++;
	}
	return expr;
}

/**
 * Creates the matcher.
 *
 * @return A newly allocated regexp matcher or null if no memory is available.
 */
MatcherProp * new_matcherprop(void)
{
	MatcherProp *m = NULL;
	gchar **strings = g_strsplit(attwarnerprefs.match_strings, "\n", -1);
	gchar *expr = NULL;
	
	expr = build_complete_regexp(strings);

	g_strfreev(strings);
	
	debug_print("building matcherprop for expr '%s'\n", expr);
	m = matcherprop_new(MATCHCRITERIA_SUBJECT, NULL, MATCHTYPE_REGEXP, 
			    expr, 0);
	if (m == NULL) {
		/* print error message */
		debug_print("failed to allocate memory for matcherprop\n");
	}

	g_free(expr);

	return m;
}

/**
 * Searches the str for matches.
 *
 * @param mp The matcher to use for searching.
 * @param str The string to search for matches.
 *
 * @return TRUE if the string given matches, FALSE otherwise.
 */
gboolean matcherprop_string_match(MatcherProp *mp, gchar *str)
{
	MsgInfo info;
	gboolean ret = FALSE;

	if (attwarnerprefs.skip_quotes
		&& *str != '\0'
		&& *prefs_common.quote_chars != '\0') {

		gchar **lines = g_strsplit(str, "\n", -1);
		int i;

		for (i = 0; lines[i] != NULL && ret == FALSE; i++) {
			if (line_has_quote_char(lines[i], 
				prefs_common.quote_chars) == NULL) {

				info.subject = lines[i];
				ret = matcherprop_match(mp, &info);
			}
		}
		g_strfreev(lines);
	} else {
		info.subject = str;
		ret = matcherprop_match(mp, &info);
	}

	return ret;
}

/**
 * Looks for attachment references in the composer text.
 *
 * @param compose The composer object to inspect.
 *
 * @return TRUE if attachment references are found, FALSE otherwise.
 */
gboolean are_attachments_mentioned(Compose *compose)
{
	GtkTextView *textview = NULL;
	GtkTextBuffer *textbuffer = NULL;
	GtkTextIter start, end;
	gchar *text = NULL;
	gboolean mentioned = FALSE;

	MatcherProp *matcher = new_matcherprop();

	if (matcher == NULL) {
		g_warning("couldn't allocate matcher");
		return FALSE;
	}

	textview = GTK_TEXT_VIEW(compose->text);
        textbuffer = gtk_text_view_get_buffer(textview);
	gtk_text_buffer_get_start_iter(textbuffer, &start);
	gtk_text_buffer_get_end_iter(textbuffer, &end);
	text = gtk_text_buffer_get_text(textbuffer, &start, &end, FALSE);

	debug_print("checking text for attachment mentions\n");
	if (text != NULL) {
		mentioned = matcherprop_string_match(matcher, text);
		
		g_free(text);
	}	

	if (matcher != NULL)
		matcherprop_free(matcher);

	return mentioned;
}

/**
 * Looks for files attached in the composer.
 *
 * @param compose The composer object to inspect.
 *
 * @return TRUE if there is one or more files attached, FALSE otherwise.
 */
gboolean does_not_have_attachments(Compose *compose)
{
	GtkTreeView *tree_view = GTK_TREE_VIEW(compose->attach_clist);
        GtkTreeModel *model;
        GtkTreeIter iter;

        model = gtk_tree_view_get_model(tree_view);

	debug_print("checking for attachments existence\n");
        if (!gtk_tree_model_get_iter_first(model, &iter))
                return TRUE;

	return FALSE;
}

/**
 * Check whether not check while redirecting or forwarding.
 *
 * @param mode The current compose->mode.
 *
 * @return TRUE for cancel further checking because it's being redirected or
 *         forwarded and user configured not to check, FALSE otherwise.
 */
gboolean do_not_check_redirect_forward(int mode)
{
	switch (mode) {
	case COMPOSE_FORWARD:
	case COMPOSE_FORWARD_AS_ATTACH:
	case COMPOSE_FORWARD_INLINE:
	case COMPOSE_REDIRECT:
		if (attwarnerprefs.skip_forwards_and_redirections)
			return TRUE;
	default:
		return FALSE;
	}
}

/**
 * Callback function to be called before sending the mail.
 * 
 * @param source The composer to be checked.
 * @param data Additional data.
 *
 * @return TRUE if no attachments are mentioned or files are attached,
 *         FALSE if attachments are mentioned and no files are attached.
 */
gboolean my_before_send_hook(gpointer source, gpointer data)
{
	Compose *compose = (Compose *)source;
	gboolean askuser = FALSE;

	debug_print("attachwarner invoked\n");
	if (compose->batch)
		return FALSE;	/* do not check while queuing */

	if (do_not_check_redirect_forward(compose->mode))
		return FALSE;

	askuser = (does_not_have_attachments(compose)
		   && are_attachments_mentioned(compose));
	if (askuser) { 
		AlertValue aval;

		aval = alertpanel(_("Attachment warning"),
				  _("An attachment is mentioned in the mail you're sending, but no file was attached. Send it anyway?"),
				  GTK_STOCK_CANCEL, _("+_Send"), NULL);
		if (aval != G_ALERTALTERNATE)
			return TRUE;
	}

	return FALSE;	/* continue sending */
}

/**
 * Initialize plugin.
 *
 * @param error  For storing the returned error message.
 *
 * @return 0 if initialization succeeds, -1 on failure.
 */
gint plugin_init(gchar **error)
{
        bindtextdomain(TEXTDOMAIN, LOCALEDIR);
	bind_textdomain_codeset(TEXTDOMAIN, "UTF-8");

	if (!check_plugin_version(MAKE_NUMERIC_VERSION(2,9,2,72),
				  VERSION_NUMERIC, _("Attach warner"), error))
		return -1;

	hook_id = hooks_register_hook(COMPOSE_CHECK_BEFORE_SEND_HOOKLIST, 
				      my_before_send_hook, NULL);
	
	if (hook_id == -1) {
		*error = g_strdup(_("Failed to register check before send hook"));
		return -1;
	}

	attachwarner_prefs_init();

	debug_print("Attachment warner plugin loaded\n");

	return 0;
}

/**
 * Destructor for the plugin.
 * Unregister the callback function and frees matcher.
 */
gboolean plugin_done(void)
{	
	hooks_unregister_hook(COMPOSE_CHECK_BEFORE_SEND_HOOKLIST, hook_id);
	attachwarner_prefs_done();
	debug_print("Attachment warner plugin unloaded\n");
	return TRUE;
}

/**
 * Get the name of the plugin.
 *
 * @return The plugin name (maybe translated).
 */
const gchar *plugin_name(void)
{
	return _("Attach warner");
}

/**
 * Get the description of the plugin.
 *
 * @return The plugin description (maybe translated).
 */
const gchar *plugin_desc(void)
{
	return _("Warns user if some reference to attachments is found in the "
	         "message text and no file is attached.");
}

/**
 * Get the kind of plugin.
 *
 * @return The "GTK2" constant.
 */
const gchar *plugin_type(void)
{
	return "GTK2";
}

/**
 * Get the license acronym the plugin is released under.
 *
 * @return The "GPL" constant.
 */
const gchar *plugin_licence(void)
{
	return "GPL2+";
}

/**
 * Get the version of the plugin.
 *
 * @return The current version string.
 */
const gchar *plugin_version(void)
{
	return PLUGINVERSION;
}

/**
 * Get the features implemented by the plugin.
 *
 * @return A constant PluginFeature structure with the features.
 */
struct PluginFeature *plugin_provides(void)
{
	static struct PluginFeature features[] = 
		{ {PLUGIN_OTHER, N_("Attach warner")},
		  {PLUGIN_NOTHING, NULL}};

	return features;
}


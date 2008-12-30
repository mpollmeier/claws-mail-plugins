/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2006 Hiroyuki Yamamoto and the Claws Mail Team
 * This file Copyright (C) 2006 iSteve <isteve@bofh.cz>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <unistd.h>
#include <stdio.h>

#include <glib.h>
#include "gettext.h"
#include <gtk/gtk.h>
#ifndef G_OS_WIN32
#if USE_PRINTUNIX
#if GTK_CHECK_VERSION(2,13,1)
#include <gtk/gtkunixprint.h>
#else
#include <gtk/gtkprintoperation.h>
#include <gtk/gtkprintjob.h>
#include <gtk/gtkprintunixdialog.h>
#endif
#endif
#include <gdk/gdkx.h>
#endif
#include "libgtkhtml/gtkhtml.h"
#include "libgtkhtml/view/htmlselection.h"
#include "libgtkhtml/layout/htmlbox.h"
#include "libgtkhtml/layout/htmlboxtext.h"
#include "common/claws.h"
#include "common/version.h"
#include "main.h"
#include "plugin.h"
#include "mimeview.h"
#include "messageview.h"
#include "prefs_common.h"
#include "gtkhtml2_prefs.h"
#include "log.h"
#include "codeconv.h"
#include "plugin.h"
#include "menu.h"
#include "defs.h"
#include "utils.h"
#include "alertpanel.h"
#include "addr_compl.h"
#include "printing.h"

#ifdef USE_PTHREAD
#include <pthread.h>
#endif

#ifdef HAVE_LIBCURL
#include <curl/curl.h>
#include <curl/curlver.h>
#endif

typedef struct _GtkHtml2Viewer GtkHtml2Viewer;

struct _GtkHtml2Viewer
{
	MimeViewer	 mimeviewer;
	GtkWidget	*html_view;
	GtkWidget	*scrollwin;
	HtmlDocument	*html_doc;
	gchar		*filename;
	gchar		*base;
	MimeInfo	*mimeinfo;
	MimeInfo	*to_load;
	gboolean	 force_image_loading;
	gint		 tag;
	gint		 loading;
	gint		 stop_previous;
	GMutex		*mutex;
	GtkWidget 	*link_popupmenu;
	GtkWidget	*context_popupmenu;
	gboolean	 is_on_url;
	gint		 last_search_match;
	gboolean	 preparing;
	gboolean	 printing;
};

static MimeViewerFactory gtkhtml2_viewer_factory;
static gchar *gtkhtml2_viewer_tmpdir = NULL;

static void gtkhtml_open_uri_cb			(GtkAction	*action,
						 GtkHtml2Viewer *viewer);
static void gtkhtml_copy_uri_cb			(GtkAction	*action,
						 GtkHtml2Viewer *viewer);
static void gtkhtml_zoomin_cb			(GtkAction	*action,
						 GtkHtml2Viewer *viewer);
static void gtkhtml_zoomout_cb			(GtkAction	*action,
						 GtkHtml2Viewer *viewer);

static GtkActionEntry gtkhtml_popup_entries[] = 
{
	{"GtkHtmlPopup",		NULL, "GtkHtmlPopup" },
	{"GtkHtmlPopup/OpenBrowser",	NULL, N_("_Open with Web browser"), NULL, NULL, G_CALLBACK(gtkhtml_open_uri_cb) },
	{"GtkHtmlPopup/CopyLink",	NULL, N_("Copy this _link"), NULL, NULL, G_CALLBACK(gtkhtml_copy_uri_cb) },
	{"GtkHtmlPopup/ZoomIn",		NULL, N_("Zoom _in"), NULL, NULL, G_CALLBACK(gtkhtml_zoomin_cb) },
	{"GtkHtmlPopup/ZoomOut",		NULL, N_("Zoom _out"), NULL, NULL, G_CALLBACK(gtkhtml_zoomout_cb) },
};

static void scrolled_cb (GtkAdjustment *adj, GtkHtml2Viewer *viewer)
{
}


static GtkWidget *gtkhtml2_get_widget(MimeViewer *_viewer)
{
	GtkHtml2Viewer *viewer = (GtkHtml2Viewer *) _viewer;

	debug_print("gtkhtml2_get_widget: %p\n", viewer->scrollwin);

	return GTK_WIDGET(viewer->scrollwin);
}

static gboolean found_in_addressbook(const gchar *address)
{
	gchar *addr = NULL;
	gboolean found = FALSE;
	gint num_addr = 0;
	
	if (!address)
		return FALSE;
	
	addr = g_strdup(address);
	extract_address(addr);
	num_addr = complete_address(addr);
	if (num_addr > 1) {
		/* skip first item (this is the search string itself) */
		int i = 1;
		for (; i < num_addr && !found; i++) {
			gchar *caddr = get_complete_address(i);
			extract_address(caddr);
			if (strcasecmp(caddr, addr) == 0)
				found = TRUE;
			g_free(caddr);
		}
	}
	g_free(addr);
	return found;
}

static gboolean load_images(GtkHtml2Viewer *viewer)
{
	MessageView *messageview = ((MimeViewer *)viewer)->mimeview 
					? ((MimeViewer *)viewer)->mimeview->messageview 
					: NULL;
	MsgInfo *msginfo = NULL;
	gchar *ab_folderpath = NULL;

	if (messageview == NULL)
		return FALSE;
	
	msginfo = messageview->msginfo;
	
	if (msginfo == NULL)
		return FALSE;

	/* don't load remote images, period. */
	if (gtkhtml_prefs.local)
		return FALSE;
	
	/* don't do whitelisting -> load images */
	if (!gtkhtml_prefs.whitelist_ab)
		return TRUE;

	if (*gtkhtml_prefs.whitelist_ab_folder != '\0' &&
	    strcasecmp(gtkhtml_prefs.whitelist_ab_folder, _("Any")) != 0)
		ab_folderpath = gtkhtml_prefs.whitelist_ab_folder;

	start_address_completion(ab_folderpath);

	/* do whitelisting -> check sender */
	if (found_in_addressbook(msginfo->from)) {
		end_address_completion();
		return TRUE;
	}
	
	end_address_completion();
	return FALSE;
}

static gboolean has_charset(FILE *fp)
{
	gchar buf[4096];
	gint loaded = 0;

	memset(buf, 0, sizeof(buf));
	while ((loaded = fread(buf, 1, 4096, fp)) > 0) {
		if (strcasestr(buf, "<meta") &&
		    strcasestr(buf, "http-equiv") &&
		    strcasestr(buf, "charset")) {
			rewind(fp);
			return TRUE;
		}
	}
	rewind(fp);
	return FALSE;
}

static gboolean has_header(FILE *fp)
{
	gchar buf[4096];
	gint loaded = 0;
	gboolean start = FALSE;
	memset(buf, 0, sizeof(buf));

	while ((loaded = fread(buf, 1, 4096, fp)) > 0) {
		if (strcasestr(buf, "<head>")) {
			start = TRUE;
		}
		if (strcasestr(buf, "</head>")) {
			rewind(fp);
			return (start == TRUE);
		}
		if (strcasestr(buf, "<body>")) {
			rewind(fp);
			return FALSE;
		}
	}
	rewind(fp);
	return FALSE;
}

static gint gtkhtml2_show_mimepart_real(MimeViewer *_viewer)
{
	GtkHtml2Viewer *viewer = (GtkHtml2Viewer *) _viewer;
	FILE *fp;
	gchar buf[4096];
	gint loaded = 0;
	const gchar *charset = NULL;
	MessageView *messageview = ((MimeViewer *)viewer)->mimeview 
					? ((MimeViewer *)viewer)->mimeview->messageview 
					: NULL;
	MimeInfo *partinfo = viewer->to_load;

	memset(buf, 0, sizeof(buf));

	viewer->loading = 1;
	messageview->updating = TRUE;
	debug_print("gtkhtml2_show_mimepart\n");

	if (viewer->filename != NULL) {
		claws_unlink(viewer->filename);
		g_free(viewer->filename);
		viewer->filename = NULL;
	}
	g_free(viewer->base);

	viewer->base = NULL;
	viewer->mimeinfo = NULL;
	if (messageview) {
		NoticeView *noticeview = messageview->noticeview;
		noticeview_hide(noticeview);
	}

	if (partinfo)
		viewer->filename = procmime_get_tmp_file_name(partinfo);
	html_document_clear(viewer->html_doc);

	html_view_zoom_reset(HTML_VIEW(viewer->html_view));
	if (partinfo && !(procmime_get_part(viewer->filename, partinfo) < 0)) {
		gboolean insert_in_header = FALSE, insert_all_header = FALSE;

		if (_viewer && _viewer->mimeview &&
		    _viewer->mimeview->messageview->forced_charset)
			charset = _viewer->mimeview->messageview->forced_charset;
		else
			charset = procmime_mimeinfo_get_parameter(partinfo, "charset");
		if (charset == NULL)
			charset = conv_get_locale_charset_str();

		debug_print("using charset %s by default\n", charset);
		
		if (html_document_open_stream(viewer->html_doc, "text/html")) {
			gboolean got_charset = FALSE;
			fp = g_fopen(viewer->filename, "rb");

			if (fp == NULL) {
				html_document_close_stream(viewer->html_doc);
				goto out;
			}

			got_charset = has_charset(fp);
			if (!got_charset) {
				if (has_header(fp))
					insert_in_header = TRUE;
				else
					insert_all_header = TRUE;
			}

			while ((loaded = fread(buf, 1, 4096, fp)) > 0) {
				g_mutex_lock(viewer->mutex);
				if (!viewer->stop_previous && !claws_is_exiting() && got_charset == FALSE) {
					if (insert_all_header) {
						gchar *meta_charset = g_strdup_printf(
							"<head><meta http-equiv=Content-Type content=\"text/html; charset=%s\"></head>",
							charset);
						html_document_write_stream(
							viewer->html_doc, meta_charset, strlen(meta_charset));
						debug_print("injected %s\n", meta_charset);
						g_free(meta_charset);
						got_charset = TRUE;
					} else if (insert_in_header && strcasestr(buf, "</head>")) {
						gchar *tmp_buf = g_malloc(loaded+1);
						gchar *start = NULL, *end = NULL;
						gchar *meta_charset = g_strdup_printf(
							"<meta http-equiv=Content-Type content=\"text/html; charset=%s\">",
							charset);

						strncpy(tmp_buf, buf, loaded);
						tmp_buf[loaded] = '\0';

						start = g_strdup(tmp_buf);
						*(strcasestr(start, "</head>")) = '\0';
						end = g_strdup(strcasestr(tmp_buf, "</head>"));
						
						html_document_write_stream(
							viewer->html_doc, start, strlen(start));
						html_document_write_stream(
							viewer->html_doc, meta_charset, strlen(meta_charset));
						strncpy(buf, end, strlen(end));
						loaded = strlen(end);
						debug_print("injected %s in head\n", meta_charset);
						g_free(tmp_buf);
						g_free(meta_charset);
						g_free(start);
						g_free(end);
						got_charset = TRUE;
					} 
				}
				if (!viewer->stop_previous && !claws_is_exiting())
					html_document_write_stream(viewer->html_doc, buf, loaded);
				else {
					g_mutex_unlock(viewer->mutex);
					break;
				}
				g_mutex_unlock(viewer->mutex);
				if (gtk_events_pending())
					gtk_main_iteration();
			}
			fclose(fp);
			html_document_close_stream(viewer->html_doc);
			viewer->mimeinfo = partinfo;
		}
	}
	g_mutex_lock(viewer->mutex);
out:
	viewer->tag = -1;
	viewer->loading = 0;
	viewer->stop_previous = FALSE;
	viewer->force_image_loading = FALSE;	
	g_mutex_unlock(viewer->mutex);
	messageview->updating = FALSE;
	viewer->preparing = FALSE;
	return FALSE;
}

static gint gtkhtml2_show_mimepart_prepare(MimeViewer *_viewer)
{
	GtkHtml2Viewer *viewer = (GtkHtml2Viewer *) _viewer;

	if (!g_mutex_trylock(viewer->mutex)) {
		if (viewer->loading) {
			viewer->stop_previous = TRUE;
			main_window_cursor_normal(mainwindow_get_mainwindow());
		}
		return TRUE;
	}

	if (viewer->tag > 0) {
		g_source_remove(viewer->tag);
		viewer->tag = -1;
		if (viewer->loading) {
			viewer->stop_previous = TRUE;
			main_window_cursor_normal(mainwindow_get_mainwindow());
		}
	}
	if (viewer->stop_previous) {
		g_mutex_unlock(viewer->mutex);
		return TRUE;
	}
	viewer->tag = g_timeout_add(5, (GtkFunction)gtkhtml2_show_mimepart_real, viewer);
	g_mutex_unlock(viewer->mutex);
	return FALSE;
}

static void gtkhtml2_show_mimepart(MimeViewer *_viewer,
				const gchar *infile,
				MimeInfo *partinfo)
{
	GtkHtml2Viewer *viewer = (GtkHtml2Viewer *) _viewer;
	viewer->to_load = partinfo;
	viewer->last_search_match = -1;
	viewer->preparing = TRUE;
	g_timeout_add(5, (GtkFunction)gtkhtml2_show_mimepart_prepare, viewer);
}

#ifdef HAVE_LIBCURL
static void reload_with_img(NoticeView *noticeview, GtkHtml2Viewer *viewer)
{
	viewer->force_image_loading = TRUE;
	gtkhtml2_show_mimepart((MimeViewer *)viewer, NULL, viewer->mimeinfo);
}
#endif

static void gtkhtml2_clear_viewer(MimeViewer *_viewer)
{
	GtkHtml2Viewer *viewer = (GtkHtml2Viewer *) _viewer;
	GtkAdjustment *vadj;
	
	MessageView *messageview = ((MimeViewer *)viewer)->mimeview 
					? ((MimeViewer *)viewer)->mimeview->messageview 
					: NULL;

	debug_print("gtkhtml2_clear_viewer\n");
	if (messageview) {
		NoticeView *noticeview = messageview->noticeview;
		noticeview_hide(noticeview);
	}

	viewer->to_load = NULL;
	if (!viewer->loading)
		html_document_clear(viewer->html_doc);
	viewer->last_search_match = -1;
	vadj = gtk_scrolled_window_get_vadjustment(
		GTK_SCROLLED_WINDOW(viewer->scrollwin));
	vadj->value = 0.0;
	g_signal_emit_by_name(G_OBJECT(vadj), "value-changed", 0);
}

static void gtkhtml2_destroy_viewer(MimeViewer *_viewer)
{
	GtkHtml2Viewer *viewer = (GtkHtml2Viewer *) _viewer;

	debug_print("gtkhtml2_destroy_viewer\n");

	g_object_unref(GTK_WIDGET(viewer->html_view));
	g_object_unref(GTK_WIDGET(viewer->scrollwin));
	claws_unlink(viewer->filename);
	g_free(viewer->filename);
    	g_free(viewer);
}

static gchar *make_url(const gchar *url, const gchar *base)
{
	if (url == NULL) {
		return NULL;
	} else if (strstr(url, "http://") ||
	    strstr(url, "https://") ||
	    strstr(url, "ftp://") ||
	    strstr(url, "sftp://") ||
	    strstr(url, "mailto:")) {
		return g_strdup(url);
	} else if (base == NULL || !strstr(base, "://")) {
		return g_strdup(url);
	} else {
		gchar *prefix = g_strdup(base);
		gchar *real_base = g_strdup(strstr(base, "://")+3);
		gchar *result = NULL;
		gboolean insert_sep = FALSE;
		*(strstr(prefix, "://")+3) = '\0';
		if (url[0] == '/') { /* absolute */
			if (strchr(real_base, '/'))
				*strchr(real_base, '/') = '\0';
		} else if (url[0] != '#') { /* relative and not anchor */
			if (strrchr(real_base, '/'))
				*(strrchr(real_base, '/')+1) = '\0';
			else
				insert_sep = TRUE;
		} else if (url[0] == '#') {
			insert_sep = FALSE;
		}
		result = g_strdup_printf("%s%s%s%s", prefix, real_base, insert_sep?"/":"", url);
		g_free(prefix);
		g_free(real_base);
		return result;
	}
}

static void gtkhtml_open_uri_cb (GtkAction *action, GtkHtml2Viewer *viewer)
{
	gchar *uri = g_object_get_data(G_OBJECT(viewer->link_popupmenu),
					   "uri");
	g_object_set_data(G_OBJECT(viewer->link_popupmenu), "uri", NULL);
	open_uri(uri, prefs_common_get_uri_cmd());
	g_free(uri);
}

static void gtkhtml_copy_uri_cb (GtkAction *action, GtkHtml2Viewer *viewer)
{
	gchar *uri = g_object_get_data(G_OBJECT(viewer->link_popupmenu),
					   "uri");
	g_object_set_data(G_OBJECT(viewer->link_popupmenu), "uri", NULL);
	gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY), uri, -1);
	gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), uri, -1);
	g_free(uri);
}

/* DESCRIPTION:
 * This callback gets executed if a user clicks a link, that is, if he
 * apparently attempts to move onto another document. Commonly handle
 * this by loading a document in same fashion as gtkhtml2_show_mimepart,
 * or just ignore it. */
void link_clicked(HtmlDocument *doc, const gchar *url, GtkHtml2Viewer *viewer) {
	gchar *real_url = make_url(url, viewer->base);
	GdkEvent *event = gtk_get_current_event();
	GdkEventButton *bevent = NULL;
	gint button = 1;
	gchar *tmp = g_object_get_data(G_OBJECT(viewer->link_popupmenu),
					   "uri");
	if (tmp)
		g_free(tmp);
	g_object_set_data(G_OBJECT(viewer->link_popupmenu), "uri", NULL);

	if (event && event->type == GDK_BUTTON_RELEASE) {
		bevent = (GdkEventButton *)event;
		button = bevent->button;
	}
	gdk_event_free(event);

	if (button == 1 || button == 2) {
		if (real_url)
			open_uri(real_url, prefs_common_get_uri_cmd());
		else
			open_uri(url, prefs_common_get_uri_cmd());
	} else if (button == 3) {
		g_object_set_data(G_OBJECT(viewer->link_popupmenu), "uri",
			  real_url ? g_strdup(real_url):g_strdup(url));
		gtk_menu_popup(GTK_MENU(viewer->link_popupmenu), 
			       NULL, NULL, NULL, NULL, 
			       bevent->button, bevent->time);
	}
	g_free(real_url);
}


/* DESCRIPTION:
 * This callback informs whenever user has a pointer over any URL. Probably
 * only useful for debugging and for statusbar update.
 */
static void on_url(GtkWidget *widget, const gchar *url, gpointer data)
{
	MimeViewer *viewer = (MimeViewer *)data;
	MessageView *messageview = viewer->mimeview ? viewer->mimeview->messageview : NULL;

	g_return_if_fail(messageview != NULL);

	if (url != NULL) {
		gchar *real_url = make_url(url, ((GtkHtml2Viewer *)data)->base);
		if (messageview->statusbar)
			gtk_statusbar_push(GTK_STATUSBAR(messageview->statusbar),
				   messageview->statusbar_cid, real_url);
		g_free(real_url);
		((GtkHtml2Viewer *)viewer)->is_on_url = TRUE;
	} else {
		if (messageview->statusbar)
			gtk_statusbar_pop(GTK_STATUSBAR(messageview->statusbar),
				   messageview->statusbar_cid);
		((GtkHtml2Viewer *)viewer)->is_on_url = FALSE;
	}
}

#ifdef HAVE_LIBCURL
struct _GtkHtmlThreadCtx {
	gchar *url;
	gboolean ready;
};

typedef struct _GtkHtmlThreadCtx GtkHtmlThreadCtx;

static void *gtkhtml_fetch_feed_threaded(void *arg)
{
	GtkHtmlThreadCtx *ctx = (GtkHtmlThreadCtx *)arg;
	CURL *eh = NULL;
	CURLcode res;
	gchar *template = get_tmp_file();
	FILE *f = NULL;

#ifdef USE_PTHREAD
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
#endif

	if (template != NULL)
		f = g_fopen(template, "wb");
	if (f == NULL) {
		perror("fdopen");
		ctx->ready = TRUE;
		claws_unlink(template);
		g_free(template);
		return NULL;
	}

	eh = curl_easy_init();

	if (eh == NULL) {
		g_warning("can't init curl");
		ctx->ready = TRUE;
		claws_unlink(template);
		g_free(template);
		return NULL;
	}

	curl_easy_setopt(eh, CURLOPT_URL, ctx->url);
	curl_easy_setopt(eh, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, NULL);
	curl_easy_setopt(eh, CURLOPT_WRITEDATA, f);
	curl_easy_setopt(eh, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(eh, CURLOPT_MAXREDIRS, 3);
#ifndef USE_PTHREAD
	curl_easy_setopt(eh, CURLOPT_TIMEOUT, prefs_common_get_prefs()->io_timeout_secs);
#endif
#if LIBCURL_VERSION_NUM >= 0x070a00
	curl_easy_setopt(eh, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(eh, CURLOPT_SSL_VERIFYHOST, 0);
#endif
	curl_easy_setopt(eh, CURLOPT_USERAGENT,
		"Claws Mail GtkHtml2 plugin "PLUGINVERSION
		" (" PLUGINS_URI ")");

	res = curl_easy_perform(eh);

	debug_print("res %d\n", res);
	curl_easy_cleanup(eh);

	fclose(f);

	ctx->ready = TRUE;

	return template;
}
#endif

static void set_base(HtmlDocument *doc, const gchar *url, gpointer data)
{
	GtkHtml2Viewer *viewer = (GtkHtml2Viewer *)data;
	g_free(viewer->base);
	viewer->base = g_strdup(url);
	
}

static void requested_url(HtmlDocument *doc, const gchar *url, HtmlStream *stream, gpointer data)
{
	void *tmpfile = NULL;
#ifdef HAVE_LIBCURL
#ifdef USE_PTHREAD
	pthread_t pt;
	pthread_attr_t pta;
#endif
	GtkHtmlThreadCtx *ctx = NULL;
	time_t start_time = time(NULL);
	gboolean killed = FALSE;
	gboolean remote_not_loaded = FALSE;
#endif
	gint loaded = 0;
	char buffer[4096];
	GtkHtml2Viewer *viewer = (GtkHtml2Viewer *)data;

	main_window_cursor_wait(mainwindow_get_mainwindow());
	if (!url)
		goto fail;
        if (strncmp(url, "cid:", 4) == 0) {
                MimeInfo *mimeinfo = ((MimeViewer *)data)->mimeview->mimeinfo;
                gchar *image = g_strconcat("<", url + 4, ">", NULL);

		debug_print("looking for %s in Content-ID\n", image);

                if (url + 4 == '\0') {
			g_free(image);
                        goto fail;
		}

                while ((mimeinfo = procmime_mimeinfo_next(mimeinfo)) != NULL) {
                        if (mimeinfo->id != NULL && strcmp(mimeinfo->id, image) == 0)
                                break;
                }
                g_free(image);

                if (mimeinfo == NULL) {
                        goto fail;
                }
		debug_print("found %s in mimeinfo's Content-ID\n", mimeinfo->id);

                tmpfile = procmime_get_tmp_file_name(mimeinfo);
                if (tmpfile == NULL)
                        goto fail;

                if (procmime_get_part(tmpfile, mimeinfo) < 0) {
                        g_free(tmpfile);
			tmpfile = NULL;
                        goto fail;
                }
        }
        else
        {
                MimeInfo *mimeinfo = ((MimeViewer *)data)->mimeview->mimeinfo;
#ifdef HAVE_LIBCURL
		gchar *real_url = NULL;
		gchar *cache_file = NULL;
#endif
		debug_print("looking for %s in Content-Location\n", url);
                if (url == '\0')
                        goto fail;

                while ((mimeinfo = procmime_mimeinfo_next(mimeinfo)) != NULL) {
                        if (mimeinfo->location != NULL && strcmp(mimeinfo->location, url) == 0)
                                break;
                }

                if (mimeinfo == NULL) {
                        goto not_found_local;
                }
		debug_print("found %s in mimeinfo's Content-Location\n", mimeinfo->location);
                tmpfile = procmime_get_tmp_file_name(mimeinfo);
                if (tmpfile == NULL)
                        goto not_found_local;

                if (procmime_get_part(tmpfile, mimeinfo) < 0) {
                        g_free(tmpfile);
			tmpfile = NULL;
                        goto not_found_local;
                }
		goto found_local;

not_found_local:
#ifdef HAVE_LIBCURL
		real_url = NULL;
		cache_file = NULL;
                if (!viewer->force_image_loading && !load_images(viewer)) {
			remote_not_loaded = TRUE;
                        goto fail;
		}

	        debug_print("looking for %s online\n", url);
		debug_print("using %s base\n", viewer->base);
		real_url = make_url(url, ((GtkHtml2Viewer *)data)->base);
		if (gtkhtml_prefs.cache_images) {
			cache_file = g_strconcat(gtkhtml2_viewer_tmpdir, G_DIR_SEPARATOR_S,
							itos(g_str_hash(real_url)), NULL);
			if (is_file_exist(cache_file)) {
				debug_print("cache file found (%s)\n", cache_file);
				tmpfile = get_tmp_file();
#ifdef G_OS_UNIX
				if (link(cache_file, tmpfile) == 0) {
#else
				if (copy_file(cache_file, tmpfile, TRUE) == 0) {
#endif
						g_free(cache_file);
						g_free(real_url);
						goto found_local;
				}
			} 
			debug_print("cache file not found (%s)\n", cache_file);
			if (!viewer->force_image_loading && prefs_common_get_prefs()->work_offline) {
				remote_not_loaded = TRUE;
				goto fail;
			}
		}
	        ctx = g_new0(GtkHtmlThreadCtx, 1);
	        ctx->url = real_url;
	        ctx->ready = FALSE;
		debug_print("final URL: %s\n", ctx->url);
#ifdef USE_PTHREAD
	        if (pthread_attr_init(&pta) != 0 ||
		    pthread_attr_setdetachstate(&pta, PTHREAD_CREATE_JOINABLE) != 0 ||
		    pthread_create(&pt, &pta, gtkhtml_fetch_feed_threaded,
				        (void *)ctx) != 0 ) {
		        /* Bummer, couldn't create thread. Continue non-threaded */
		        tmpfile = gtkhtml_fetch_feed_threaded(ctx);
	        } else {
		        /* Thread created, let's wait until it finishes */
		        debug_print("gtkhtml: waiting %d secs for thread to finish \n", prefs_common_get_prefs()->io_timeout_secs);
		        while( !ctx->ready ) {
			        claws_do_idle();
				if (time(NULL) - start_time > prefs_common_get_prefs()->io_timeout_secs) {
					log_error(LOG_PROTOCOL, _("Timeout connecting to %s\n"), url);
					pthread_cancel(pt);
					ctx->ready = TRUE;
					killed = TRUE;
				} 
				if (viewer->stop_previous || claws_is_exiting()) {
					pthread_cancel(pt);
					ctx->ready = TRUE;
					killed = TRUE;
				} 
			}
		        debug_print("gtkhtml: thread finished\n");

		        pthread_join(pt, &tmpfile);
			if (killed)
				tmpfile = NULL;
	        }
		g_free(ctx->url);
#else
	        debug_print("gtkhtml: no pthreads, run blocking fetch\n");
	        (gchar *)tmpfile = gtkhtml_fetch_feed_threaded(ctx);
#endif
		if (gtkhtml_prefs.cache_images && tmpfile) {
#ifdef G_OS_UNIX
			link(tmpfile, cache_file);
			debug_print("cache file created (%s)\n", cache_file);
#endif
			g_free(cache_file);
		}
#else
		debug_print("image not found locally, and no libcurl support.\n");
#endif
        }

found_local:
	debug_print("file %s\n", (char *)(tmpfile?tmpfile:""));
	if (tmpfile) {
		FILE *fp = g_fopen(tmpfile, "rb");
		if (fp == NULL) {
			html_stream_close(stream);
			claws_unlink(tmpfile);
			g_free(tmpfile);
#ifdef HAVE_LIBCURL
			g_free(ctx);
#endif
			main_window_cursor_normal(mainwindow_get_mainwindow());
			return;
		}

		while ((loaded = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
			if (viewer->stop_previous || claws_is_exiting()) {
				break;
			}
			html_stream_write(stream, buffer, loaded);
			while (gtk_events_pending())
				gtk_main_iteration();
		}
		fclose(fp);
		claws_unlink(tmpfile);
		g_free(tmpfile);
	}
#ifdef HAVE_LIBCURL
	g_free(ctx);
#endif
fail:
	main_window_cursor_normal(mainwindow_get_mainwindow());
#ifdef HAVE_LIBCURL
	if (remote_not_loaded) {
		MessageView *messageview = ((MimeViewer *)viewer)->mimeview 
						? ((MimeViewer *)viewer)->mimeview->messageview 
						: NULL;
		if (messageview) {
			gchar *text = NULL;
			NoticeView *noticeview = messageview->noticeview;
			if (!load_images(viewer)) {
				text = _("Remote images exist, but weren't loaded\naccording to your preferences.");
			} else if (prefs_common_get_prefs()->work_offline) {
				text = _("Remote images exist, but weren't loaded\nbecause you are offline.");
			} else {
				text = _("Remote images exist, but loading them failed.");
			}
			noticeview_set_icon(noticeview, STOCK_PIXMAP_NOTICE_WARN);
			noticeview_set_text(noticeview, text);
			noticeview_set_button_text(noticeview, _("Load images"));
			noticeview_set_button_press_callback(noticeview,
				     G_CALLBACK(reload_with_img), (gpointer)viewer);

			noticeview_set_2ndbutton_text(noticeview, NULL);
			noticeview_show(noticeview);
		}
	}
#endif
	html_stream_close(stream);
}

static gchar *gtkhtml2_get_selection(MimeViewer *_viewer)
{
	GtkHtml2Viewer *viewer = (GtkHtml2Viewer *)_viewer;
	
	if (viewer->html_view == NULL)
		return NULL;
	return html_selection_get_text((HtmlView *)viewer->html_view);
}

static HtmlBox *get_next_box(HtmlBox *box)
{
	HtmlBox *parent;
	
	if (!box)
		return NULL;
	if (box->children) {
		return box->children;
	}
	if (box->next) {
		return box->next;
	}

	for (parent = box->parent; parent != NULL;
	     parent = parent->parent) {
		if (parent->next)
			return parent->next;
	}

	return NULL;
}

static gboolean gtkhtml2_search_forward(GtkHtml2Viewer *viewer,
				      const gchar *str, gboolean case_sens,
				      gboolean select_result)
{
	HtmlBox *box = NULL;
	gchar *search_str = case_sens?g_strdup(str):g_utf8_strdown(g_strdup(str), -1);
	gint offset = 0, r_offset = 0;
	DomNode *last_node = NULL;
	if (HTML_VIEW(viewer->html_view) == NULL)
		return FALSE;
		
	box = HTML_VIEW(viewer->html_view)->root;
	for (; box; box = get_next_box(box)) {
		if (HTML_IS_BOX_TEXT(box)) {
			gchar *found = NULL;
			gchar *text = NULL;
			gchar *stext = NULL;
			
			if (HTML_BOX_TEXT(box)->canon_text == NULL)
				continue;
			text = case_sens?g_strndup(HTML_BOX_TEXT(box)->canon_text,
					HTML_BOX_TEXT(box)->length):
					g_utf8_strdown(g_strndup(HTML_BOX_TEXT(box)->canon_text,
					HTML_BOX_TEXT(box)->length), -1);
			stext = text;
			gint len;
			if (box->dom_node != last_node) {
				last_node = box->dom_node;
				r_offset = 0;
			}
search_substring:
			if (!stext)
				continue;
			found = strstr(stext, search_str);

			if (!found) {
				len = g_utf8_strlen(stext, -1);
				offset += len;
				r_offset += len;
			} else {
				debug_print("found: %s\n", found);
				*found = 0;
				debug_print("relative offset %d\n", r_offset);
				len = g_utf8_strlen(stext, -1);
				if (offset + len <= viewer->last_search_match) {
					offset += len;
					r_offset += len;
					*found=0x1;
					stext = found;
					goto search_substring;
				}
				offset += len;
				r_offset += len;
				*found=0x1;
				viewer->last_search_match = offset;
				if (select_result) {
					html_selection_set(HTML_VIEW(viewer->html_view),
						box->dom_node,
						r_offset, g_utf8_strlen(str, -1));
					html_view_scroll_to_node(HTML_VIEW(viewer->html_view),
						box->dom_node, HTML_VIEW_SCROLL_TO_BOTTOM);
				}
				g_free(text);
				return TRUE;
			}
			g_free(text);
		}
	}
	g_free(search_str);
	
	return FALSE;
}

static gboolean gtkhtml2_search_backward(GtkHtml2Viewer *viewer,
				      const gchar *str, gboolean case_sens)
{
	gint highest_index = viewer->last_search_match;
	gint prev_highest = -1;
	gint i = 0, j = 0;
	if (highest_index == -1) {
		/* find last match */
		while (gtkhtml2_search_forward(viewer, str, case_sens, FALSE))
			i++;
		highest_index = viewer->last_search_match;
		if (highest_index == -1)
			return FALSE; /* not found */
	} else {
		/* find previous match */
		prev_highest = viewer->last_search_match;
		viewer->last_search_match = -1;
		while (gtkhtml2_search_forward(viewer, str, case_sens, FALSE)
			 && viewer->last_search_match <= prev_highest)
			i++;
		i--;
		highest_index = viewer->last_search_match;
		if (highest_index == -1 || i < 0)
			return FALSE; /* not found */

	}
	
	if (i == 0)
		return FALSE;
	viewer->last_search_match = 0;
	for (j = 0; j < i-1; j++) {
		gtkhtml2_search_forward(viewer, str, case_sens, FALSE);
	}
	gtkhtml2_search_forward(viewer, str, case_sens, TRUE);
	
	return TRUE;
}

static gboolean	gtkhtml2_text_search (MimeViewer *_viewer, gboolean backward,
				      const gchar *str, gboolean case_sens)
{
	gboolean result;
	GtkHtml2Viewer *viewer = (GtkHtml2Viewer *)_viewer;
	
	if (backward)
		result = gtkhtml2_search_backward(viewer, str, case_sens);
	else
		result = gtkhtml2_search_forward(viewer, str, case_sens, TRUE);
	
	if (result == FALSE)
		viewer->last_search_match = -1;
	
	return result;
}

static gboolean gtkhtml2_scroll_page(MimeViewer *_viewer, gboolean up)
{
	GtkHtml2Viewer *viewer = (GtkHtml2Viewer *)_viewer;
	GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(
		GTK_SCROLLED_WINDOW(viewer->scrollwin));

	if (viewer->html_view == NULL)
		return FALSE;

	return gtkutils_scroll_page(viewer->html_view, vadj, up);
}

static void gtkhtml2_scroll_one_line(MimeViewer *_viewer, gboolean up)
{
	GtkHtml2Viewer *viewer = (GtkHtml2Viewer *)_viewer;
	GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(
		GTK_SCROLLED_WINDOW(viewer->scrollwin));

	if (viewer->html_view == NULL)
		return;

	gtkutils_scroll_one_line(viewer->html_view, vadj, up);
}

static gboolean htmlview_scrolled(GtkWidget *widget, GdkEventScroll *event,
				    GtkHtml2Viewer *viewer)
{
	if ((event->state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK) {
		if (event->direction == GDK_SCROLL_UP) {
			html_view_zoom_out(HTML_VIEW(viewer->html_view));
		} else {
			html_view_zoom_in(HTML_VIEW(viewer->html_view));
		}
		return TRUE;
	}
	return FALSE;
}

static void gtkhtml_zoomin_cb (GtkAction *action, GtkHtml2Viewer *viewer)
{
	html_view_zoom_in(HTML_VIEW(viewer->html_view));
}

static void gtkhtml_zoomout_cb (GtkAction *action, GtkHtml2Viewer *viewer)
{
	html_view_zoom_out(HTML_VIEW(viewer->html_view));
}

static gboolean htmlview_btn_released(GtkWidget *widget, GdkEventButton *event,
				    GtkHtml2Viewer *viewer)
{
	if (event->button == 3 && !viewer->is_on_url)
		gtk_menu_popup(GTK_MENU(viewer->context_popupmenu), 
		       NULL, NULL, NULL, NULL, 
		       event->button, event->time);
	return FALSE;
}

#if GTK_CHECK_VERSION(2,10,0) && USE_PRINTUNIX && !defined(G_OS_WIN32)
static void
job_complete_cb (GtkPrintJob *print_job,
		        GtkHtml2Viewer *viewer,
		        GError *error)
{
	if (error) {
		alertpanel_error(_("Printing failed:\n %s"), error->message);
	}
	viewer->printing = FALSE;
}

static void gtkhtml2_viewer_print(MimeViewer *mviewer)
{
	GtkHtml2Viewer *viewer = (GtkHtml2Viewer *)mviewer;
	MainWindow *mainwin = mainwindow_get_mainwindow();
	HtmlView *htmlview;
	gchar *program = NULL, *cmd = NULL;
	gchar *outfile = NULL;
	gint result;
	GError *error = NULL;
	GtkWidget *dialog;
	GtkPrintUnixDialog *print_dialog;
	GtkPrinter *printer;
	GtkPrintJob *job;

	htmlview = HTML_VIEW(viewer->html_view);

	gtk_widget_realize(viewer->html_view);
	
	while (viewer->preparing) {
		claws_do_idle();
	}
	debug_print("Preparing print job...\n");

	program = g_find_program_in_path("html2ps");

	if (program == NULL) {
		alertpanel_error(_("Printing HTML is only possible if the program 'html2ps' is installed."));
		return;
	}

	if (viewer->filename == NULL) {
		alertpanel_error(_("Filename is null."));
		return;
	}

	outfile = get_tmp_file();
	cmd = g_strdup_printf("%s%s -o %s %s", program, 
			load_images(viewer)?"":" -T", 
			outfile, viewer->filename);

	g_free(program);

	result = execute_command_line(cmd, FALSE);
	g_free(cmd);

	if (result != 0) {
		alertpanel_error(_("Conversion to postscript failed."));
		g_free(outfile);
		return;
	}

	debug_print("Starting print job...\n");
	
	dialog = gtk_print_unix_dialog_new (_("Print"),
		  	   	 mainwin? GTK_WINDOW (mainwin->window):NULL);
	print_dialog = GTK_PRINT_UNIX_DIALOG (dialog);
	gtk_print_unix_dialog_set_page_setup (print_dialog, printing_get_page_setup());
	gtk_print_unix_dialog_set_settings (print_dialog, printing_get_settings());

	gtk_print_unix_dialog_set_manual_capabilities(print_dialog,
		GTK_PRINT_CAPABILITY_GENERATE_PS);

	result = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_hide (dialog);

	printer = gtk_print_unix_dialog_get_selected_printer (print_dialog);

	if (result != GTK_RESPONSE_OK || !printer) {
		gtk_widget_destroy (dialog);
		g_free(outfile);
		return;
	}

	if (!gtk_printer_accepts_ps(printer)) {
		alertpanel_error(_("Printer %s doesn't accept PostScript files."), 
			gtk_printer_get_name(printer));
		g_free(outfile);
		return;
	}
	
	printing_store_settings(gtk_print_unix_dialog_get_settings(print_dialog));

	job = gtk_print_job_new(viewer->filename,
				printer,
				printing_get_settings(),
				printing_get_page_setup());

	gtk_print_job_set_source_file(job, outfile, &error);

	if (error) {
		alertpanel_error(_("Printing failed:\n%s"), error->message);
		g_error_free(error);
		g_free(outfile);
		return;
	}
	viewer->printing = TRUE;
	
	gtk_print_job_send (job,
		(GtkPrintJobCompleteFunc) job_complete_cb,
		viewer,
		NULL);	

	while (viewer->printing) {
		claws_do_idle();
	}

	g_free(outfile);
}
#endif

static MimeViewer *gtkhtml2_viewer_create(void)
{
	GtkHtml2Viewer *viewer;
	GtkAdjustment *adj;
	gfloat min_size = 0.0, min_size_new;
	PangoFontDescription *font_desc = NULL;
	GtkUIManager *ui_manager;
	GtkActionGroup *action_group;

	debug_print("gtkhtml2_viewer_create\n");

	viewer = g_new0(GtkHtml2Viewer, 1);
	viewer->mimeviewer.factory = &gtkhtml2_viewer_factory;
	viewer->mimeviewer.get_widget = gtkhtml2_get_widget;
	viewer->mimeviewer.show_mimepart = gtkhtml2_show_mimepart;
	viewer->mimeviewer.clear_viewer = gtkhtml2_clear_viewer;
	viewer->mimeviewer.destroy_viewer = gtkhtml2_destroy_viewer;
	viewer->mimeviewer.get_selection = gtkhtml2_get_selection;
	viewer->mimeviewer.text_search = gtkhtml2_text_search;
	viewer->mimeviewer.scroll_page = gtkhtml2_scroll_page;
	viewer->mimeviewer.scroll_one_line = gtkhtml2_scroll_one_line;
#if GTK_CHECK_VERSION(2,10,0) && USE_PRINTUNIX && !defined(G_OS_WIN32)
	viewer->mimeviewer.print = gtkhtml2_viewer_print;
#endif
	viewer->html_doc = html_document_new();
	viewer->html_view = html_view_new();
	viewer->scrollwin = gtk_scrolled_window_new(NULL, NULL);
	viewer->base      = NULL;
	viewer->mimeinfo  = NULL;
	viewer->force_image_loading = FALSE;
	viewer->tag       = -1;
	viewer->mutex     = g_mutex_new();

	font_desc = pango_font_description_from_string
			(prefs_common_get_prefs()->textfont);
	min_size_new = (gfloat)(pango_font_description_get_size(font_desc)/PANGO_SCALE);
	pango_font_description_free(font_desc);

	g_object_get (gtk_settings_get_default (),
			"gtkhtml-minimum-font-size",
			&min_size, NULL);

	if (min_size > 0.0 && min_size < min_size_new) {
		debug_print("setting minimum size to %.2f (overriding %.2f)\n",
				min_size_new, min_size);
		gtk_settings_set_double_property(gtk_settings_get_default(),
                                       "gtkhtml-minimum-font-size",
                                       (gdouble)min_size_new, "XProperty");
	} else if (min_size <= 0.0) {
		g_warning("Can't set minimum font size - you need libgtkhtml > 2.11.0\n");
	}

	gtk_scrolled_window_set_policy(
			GTK_SCROLLED_WINDOW(viewer->scrollwin), 
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_scrolled_window_set_shadow_type(
			GTK_SCROLLED_WINDOW(viewer->scrollwin),
			GTK_SHADOW_IN);

	gtk_container_add(
		GTK_CONTAINER(viewer->scrollwin),
		viewer->html_view);

	adj = gtk_scrolled_window_get_vadjustment(
		GTK_SCROLLED_WINDOW(viewer->scrollwin));
	g_signal_connect(G_OBJECT(adj), "value-changed",
			 G_CALLBACK(scrolled_cb), viewer);

	html_view_set_document(HTML_VIEW(viewer->html_view), viewer->html_doc);
	g_signal_connect(G_OBJECT(viewer->html_doc), "set_base", G_CALLBACK(set_base), viewer);
	g_signal_connect(G_OBJECT(viewer->html_doc), "request_url", G_CALLBACK(requested_url), viewer);
	g_signal_connect(G_OBJECT(viewer->html_doc), "link_clicked", G_CALLBACK(link_clicked), viewer);
	g_signal_connect(G_OBJECT(viewer->html_view),"on_url", G_CALLBACK(on_url), viewer);
	g_signal_connect(G_OBJECT(viewer->html_view), "scroll_event",
			 G_CALLBACK(htmlview_scrolled), viewer);
	g_signal_connect(G_OBJECT(viewer->html_view), "button_release_event",
			 G_CALLBACK(htmlview_btn_released), viewer);

	gtk_widget_show(GTK_WIDGET(viewer->scrollwin));
	g_object_ref(GTK_WIDGET(viewer->scrollwin));
	gtk_widget_show(GTK_WIDGET(viewer->html_view));
	g_object_ref(GTK_WIDGET(viewer->html_view));

	ui_manager = gtk_ui_manager_new();
	action_group = cm_menu_create_action_group_full(ui_manager,"GtkHtmlPopup", gtkhtml_popup_entries,
			G_N_ELEMENTS(gtkhtml_popup_entries), (gpointer)viewer);

	MENUITEM_ADDUI_MANAGER(ui_manager, "/", "Menus", "Menus", GTK_UI_MANAGER_MENUBAR)
	MENUITEM_ADDUI_MANAGER(ui_manager, "/Menus", "GtkHtmlLink", "GtkHtmlPopup", GTK_UI_MANAGER_MENU)
	MENUITEM_ADDUI_MANAGER(ui_manager, "/Menus/GtkHtmlLink", "OpenBrowser", "GtkHtmlPopup/OpenBrowser", GTK_UI_MANAGER_MENUITEM)
	MENUITEM_ADDUI_MANAGER(ui_manager, "/Menus/GtkHtmlLink", "CopyLink", "GtkHtmlPopup/CopyLink", GTK_UI_MANAGER_MENUITEM)
	viewer->link_popupmenu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(
				gtk_ui_manager_get_widget(ui_manager, "/Menus/GtkHtmlLink")) );

	MENUITEM_ADDUI_MANAGER(ui_manager, "/Menus", "GtkHtmlZoom", "GtkHtmlPopup", GTK_UI_MANAGER_MENU)
	MENUITEM_ADDUI_MANAGER(ui_manager, "/Menus/GtkHtmlZoom", "ZoomIn", "GtkHtmlPopup/ZoomIn", GTK_UI_MANAGER_MENUITEM)
	MENUITEM_ADDUI_MANAGER(ui_manager, "/Menus/GtkHtmlZoom", "ZoomOut", "GtkHtmlPopup/ZoomOut", GTK_UI_MANAGER_MENUITEM)
	viewer->context_popupmenu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(
				gtk_ui_manager_get_widget(ui_manager, "/Menus/GtkHtmlZoom")) );
	viewer->filename = NULL;

	return (MimeViewer *) viewer;
}

static gchar *content_types[] =
	{"text/html", NULL};

static MimeViewerFactory gtkhtml2_viewer_factory =
{
	content_types,
	0,

	gtkhtml2_viewer_create,
};

gint plugin_init(gchar **error)
{
/*	make_url tests - leaky 
	printf("%s\n", make_url("http://abs_url_w_host", NULL));
	printf("%s\n", make_url("http://abs_url_w_host", "ftp://base/"));
	printf("%s\n", make_url("/abs_url", NULL));
	printf("%s\n", make_url("/abs_url", "ftp://base"));
	printf("%s\n", make_url("/abs_url", "ftp://base/a"));
	printf("%s\n", make_url("/abs_url", "ftp://base/a/"));
	printf("%s\n", make_url("rel_url", NULL));
	printf("%s\n", make_url("rel_url", "ftp://base"));
	printf("%s\n", make_url("rel_url", "ftp://base/a/b"));
	printf("%s\n", make_url("rel_url", "ftp://base/a/b/"));
*/
#ifdef G_OS_UNIX
	bindtextdomain(TEXTDOMAIN, LOCALEDIR);
#else
	bindtextdomain(TEXTDOMAIN, get_locale_dir());
#endif
	bind_textdomain_codeset(TEXTDOMAIN, "UTF-8");

	gtkhtml2_viewer_tmpdir = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S,
				"gtkhtml2_viewer", NULL);

	if (!check_plugin_version(MAKE_NUMERIC_VERSION(3,1,0,62),
				VERSION_NUMERIC, _("GtkHtml2 HTML Viewer"), error))
		return -1;

#ifdef HAVE_LIBCURL
	gtkhtml_prefs_init();
	curl_global_init(CURL_GLOBAL_DEFAULT);
#endif
	mimeview_register_viewer_factory(&gtkhtml2_viewer_factory);

	if (!is_dir_exist(gtkhtml2_viewer_tmpdir))
		make_dir_hier(gtkhtml2_viewer_tmpdir);

	return 0;
}

void gtkhtml2_viewer_clear_cache(void)
{
	remove_dir_recursive(gtkhtml2_viewer_tmpdir);
	make_dir_hier(gtkhtml2_viewer_tmpdir);
}

gboolean plugin_done(void)
{
	if (gtkhtml_prefs.clear_cache)
		remove_dir_recursive(gtkhtml2_viewer_tmpdir);
	g_free(gtkhtml2_viewer_tmpdir);
	gtkhtml2_viewer_tmpdir = NULL;

#ifdef HAVE_LIBCURL
	gtkhtml_prefs_done();
#endif
	mimeview_unregister_viewer_factory(&gtkhtml2_viewer_factory);
	return FALSE;
}

const gchar *plugin_name(void)
{
	return _("GtkHtml2 HTML Viewer");
}

const gchar *plugin_desc(void)
{
	return _("This plugin renders HTML mail using the gtkhtml2"
		"rendering widget.");
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
		{ {PLUGIN_MIMEVIEWER, "text/html"},
		  {PLUGIN_NOTHING, NULL}};
	return features;
}

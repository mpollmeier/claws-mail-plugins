/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2006 Hiroyuki Yamamoto and the Sylpheed-Claws Team
 * This file Copyright (C) 2006 iSteve <isteve@bofh.cz>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <unistd.h>
#include <stdio.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <libgtkhtml/gtkhtml.h>
#include "common/sylpheed.h"
#include "common/version.h"
#include "main.h"
#include "plugin.h"
#include "utils.h"
#include "mimeview.h"
#include "messageview.h"
#include "prefs_common.h"
#include "gtkhtml2_prefs.h"
#include "log.h"
#include "codeconv.h"

#include "pluginconfig.h"
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
};

static MimeViewerFactory gtkhtml2_viewer_factory;
static gchar *gtkhtml2_viewer_tmpdir = NULL;

static void scrolled_cb (GtkAdjustment *adj, GtkHtml2Viewer *viewer)
{
}


static GtkWidget *gtkhtml2_get_widget(MimeViewer *_viewer)
{
	GtkHtml2Viewer *viewer = (GtkHtml2Viewer *) _viewer;

	debug_print("gtkhtml2_get_widget: %p\n", viewer->scrollwin);

	return GTK_WIDGET(viewer->scrollwin);
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

	viewer->loading = 1;
	debug_print("gtkhtml2_show_mimepart\n");

	if (viewer->filename != NULL) {
		g_unlink(viewer->filename);
		g_free(viewer->filename);
	}
	g_free(viewer->base);

	viewer->base = NULL;
	viewer->mimeinfo = NULL;
	if (messageview) {
		NoticeView *noticeview = messageview->noticeview;
		noticeview_hide(noticeview);
	}

	viewer->filename = procmime_get_tmp_file_name(partinfo);
	html_document_clear(viewer->html_doc);

	if (!(procmime_get_part(viewer->filename, partinfo) < 0)) {

		if (_viewer && _viewer->mimeview &&
		    _viewer->mimeview->messageview->forced_charset)
			charset = _viewer->mimeview->messageview->forced_charset;
		else
			charset = procmime_mimeinfo_get_parameter(partinfo, "charset");
		if (charset == NULL)
			charset = conv_get_locale_charset_str();

		debug_print("using charset %s\n", charset);
		
		if (html_document_open_stream(viewer->html_doc, "text/html")) {
			gboolean got_charset = FALSE;
			fp = fopen(viewer->filename, "r");

			if (fp == NULL) {
				html_document_close_stream(viewer->html_doc);
				goto out;
			}

			while ((loaded = fread(buf, 1, 4096, fp)) > 0) {
				if (strcasestr(buf, "<meta") &&
				    strcasestr(buf, "http-equiv") &&
				    strcasestr(buf, "charset"))
					got_charset = TRUE; /* hack */
				
				g_mutex_lock(viewer->mutex);
				if (!viewer->stop_previous && !sylpheed_is_exiting() &&
				    strcasestr(buf, "</head>") && got_charset == FALSE) {
					gchar *meta_charset = g_strdup_printf(
						"<meta http-equiv=Content-Type content=\"text/html; charset=%s\">",
						charset);
					html_document_write_stream(
						viewer->html_doc, meta_charset, strlen(meta_charset));
					debug_print("injected %s\n", meta_charset);
					g_free(meta_charset);
				}
				if (!viewer->stop_previous && !sylpheed_is_exiting())
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
	return FALSE;
}

static gint gtkhtml2_show_mimepart_prepare(MimeViewer *_viewer)
{
	GtkHtml2Viewer *viewer = (GtkHtml2Viewer *) _viewer;

	if (!g_mutex_trylock(viewer->mutex)) {
		if (viewer->loading)
			viewer->stop_previous = TRUE;
		return TRUE;
	}

	if (viewer->tag > 0) {
		gtk_timeout_remove(viewer->tag);
		viewer->tag = -1;
		if (viewer->loading) {
			viewer->stop_previous = TRUE;
		}
	}
	if (viewer->stop_previous) {
		g_mutex_unlock(viewer->mutex);
		return TRUE;
	}
	viewer->tag = gtk_timeout_add(5, (GtkFunction)gtkhtml2_show_mimepart_real, viewer);
	g_mutex_unlock(viewer->mutex);
	return FALSE;
}

static void gtkhtml2_show_mimepart(MimeViewer *_viewer,
				const gchar *infile,
				MimeInfo *partinfo)
{
	GtkHtml2Viewer *viewer = (GtkHtml2Viewer *) _viewer;
	viewer->to_load = partinfo;
	gtk_timeout_add(5, (GtkFunction)gtkhtml2_show_mimepart_prepare, viewer);
}

static void reload_with_img(NoticeView *noticeview, GtkHtml2Viewer *viewer)
{
	viewer->force_image_loading = TRUE;
	gtkhtml2_show_mimepart((MimeViewer *)viewer, NULL, viewer->mimeinfo);
}

static void gtkhtml2_clear_viewer(MimeViewer *_viewer)
{
	debug_print("gtkhtml2_clear_viewer\n");
}

static void gtkhtml2_destroy_viewer(MimeViewer *_viewer)
{
	GtkHtml2Viewer *viewer = (GtkHtml2Viewer *) _viewer;

	debug_print("gtkhtml2_destroy_viewer\n");

	gtk_widget_unref(GTK_WIDGET(viewer->html_view));
	gtk_widget_unref(GTK_WIDGET(viewer->scrollwin));
	g_unlink(viewer->filename);
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
		} else { /* relative */
			if (strrchr(real_base, '/'))
				*(strrchr(real_base, '/')+1) = '\0';
			else
				insert_sep = TRUE;
		}
		result = g_strdup_printf("%s%s%s%s", prefix, real_base, insert_sep?"/":"", url);
		g_free(prefix);
		g_free(real_base);
		return result;
	}
}

/* DESCRIPTION:
 * This callback gets executed if a user clicks a link, that is, if he
 * apparently attempts to move onto another document. Commonly handle
 * this by loading a document in same fashion as gtkhtml2_show_mimepart,
 * or just ignore it. */
void link_clicked(HtmlDocument *doc, const gchar *url, gpointer data) {
	gchar *real_url = make_url(url, ((GtkHtml2Viewer *)data)->base);
	if (real_url)
		open_uri(real_url, prefs_common.uri_cmd);
	else
		open_uri(url, prefs_common.uri_cmd);
	g_free(real_url);
}


/* DESCRIPTION:
 * This callback informs whenever user has a pointer over any URL. Probably
 * only useful for debugging and for statusbar update.
 */
static void on_url(GtkWidget *widget, const gchar *url, gpointer data)
{
	gchar *trimmed_uri = NULL;
	MimeViewer *viewer = (MimeViewer *)data;
	MessageView *messageview = viewer->mimeview ? viewer->mimeview->messageview : NULL;

	g_return_if_fail(messageview != NULL);

	if (url != NULL) {
		gchar *real_url = make_url(url, ((GtkHtml2Viewer *)data)->base);
		trimmed_uri = trim_string(real_url?real_url:url, 60);
		if (messageview->statusbar)
			gtk_statusbar_push(GTK_STATUSBAR(messageview->statusbar),
				   messageview->statusbar_cid, trimmed_uri);
		g_free(real_url);
		g_free(trimmed_uri);
	} else {
		if (messageview->statusbar)
			gtk_statusbar_pop(GTK_STATUSBAR(messageview->statusbar),
				   messageview->statusbar_cid);
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
		f = fopen(template, "wb");
	if (f == NULL) {
		perror("fdopen");
		ctx->ready = TRUE;
		g_unlink(template);
		g_free(template);
		return NULL;
	}

	eh = curl_easy_init();

	if (eh == NULL) {
		g_warning("can't init curl");
		ctx->ready = TRUE;
		g_unlink(template);
		g_free(template);
		return NULL;
	}

	curl_easy_setopt(eh, CURLOPT_URL, ctx->url);
	curl_easy_setopt(eh, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(eh, CURLOPT_MUTE, 1);
	curl_easy_setopt(eh, CURLOPT_WRITEDATA, f);
	curl_easy_setopt(eh, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(eh, CURLOPT_MAXREDIRS, 3);
#ifndef USE_PTHREAD
	curl_easy_setopt(eh, CURLOPT_TIMEOUT, prefs_common.io_timeout_secs);
#endif
#if LIBCURL_VERSION_NUM >= 0x070a00
	curl_easy_setopt(eh, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(eh, CURLOPT_SSL_VERIFYHOST, 0);
#endif
	curl_easy_setopt(eh, CURLOPT_USERAGENT,
		"Sylpheed-Claws GtkHtml2 plugin "PLUGINVERSION
		" (http://claws.sylpheed.org/plugins.php)");

	res = curl_easy_perform(eh);

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
#ifdef USE_PTHREAD
	pthread_t pt;
#endif
#ifdef HAVE_LIBCURL
	void *tmpfile = NULL;
	GtkHtmlThreadCtx *ctx = NULL;
	gint loaded = 0;
	char buffer[4096];
	time_t start_time = time(NULL);
	gboolean killed = FALSE;
	gboolean remote_not_loaded = FALSE;
	GtkHtml2Viewer *viewer = (GtkHtml2Viewer *)data;

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
		gchar *real_url = NULL;
		gchar *cache_file = NULL;
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
		real_url = NULL;
		cache_file = NULL;
                if (!viewer->force_image_loading && (gtkhtml_prefs.local || prefs_common.work_offline)) {
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
				if (link(cache_file, tmpfile) == 0) {
					g_free(cache_file);
					g_free(real_url);
					goto found_local;
				}
			} 
			debug_print("cache file not found (%s)\n", cache_file);
		}
	        ctx = g_new0(GtkHtmlThreadCtx, 1);
	        ctx->url = real_url;
	        ctx->ready = FALSE;
		debug_print("final URL: %s\n", ctx->url);
#ifdef USE_PTHREAD
	        if( pthread_create(&pt, PTHREAD_CREATE_JOINABLE, gtkhtml_fetch_feed_threaded,
				        (void *)ctx) != 0 ) {
		        /* Bummer, couldn't create thread. Continue non-threaded */
		        tmpfile = gtkhtml_fetch_feed_threaded(ctx);
	        } else {
		        /* Thread created, let's wait until it finishes */
		        debug_print("gtkhtml: waiting for thread to finish\n");
		        while( !ctx->ready ) {
			        sylpheed_do_idle();
				if (time(NULL) - start_time > prefs_common.io_timeout_secs) {
					log_error(_("Timeout connecting to %s\n"), url);
					pthread_cancel(pt);
					ctx->ready = TRUE;
					killed = TRUE;
				} 
				if (viewer->stop_previous || sylpheed_is_exiting()) {
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
			link(tmpfile, cache_file);
			debug_print("cache file created (%s)\n", cache_file);
			g_free(cache_file);
		}
        }

found_local:
	debug_print("file %s\n", (char *)tmpfile);
	if (tmpfile) {
		FILE *fp = fopen(tmpfile, "r");
		if (fp == NULL) {
			html_stream_close(stream);
			g_unlink(tmpfile);
			g_free(ctx);
			return;
		}

		while ((loaded = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
			if (viewer->stop_previous || sylpheed_is_exiting())
				break;
			html_stream_write(stream, buffer, loaded);
			while (gtk_events_pending())
				gtk_main_iteration();
		}
		fclose(fp);
		g_unlink(tmpfile);
	}
	g_free(ctx);
#endif
fail:
	if (remote_not_loaded) {
		MessageView *messageview = ((MimeViewer *)viewer)->mimeview 
						? ((MimeViewer *)viewer)->mimeview->messageview 
						: NULL;
		if (messageview) {
			gchar *text = NULL;
			NoticeView *noticeview = messageview->noticeview;
			if (gtkhtml_prefs.local) {
				text = _("Remote images exist, but weren't loaded\naccording to your preferences.");
			} else if (prefs_common.work_offline) {
				text = _("Remote images exist, but weren't loaded\nbecause you are offline.");
			} else {
				text = _("Remote images exist, but loading them failed.");
			}
			noticeview_set_text(noticeview, text);
			noticeview_set_button_text(noticeview, _("Load images"));
			noticeview_set_button_press_callback(noticeview,
				     G_CALLBACK(reload_with_img), (gpointer)viewer);

			noticeview_set_2ndbutton_text(noticeview, NULL);
			noticeview_show(noticeview);
		}
	}
	html_stream_close(stream);
}

static gchar *gtkhtml2_get_selection(MimeViewer *_viewer)
{
	GtkHtml2Viewer *viewer = (GtkHtml2Viewer *)_viewer;
	
	if (viewer->html_view == NULL)
		return NULL;
	return html_selection_get_text(viewer->html_view);
}

static MimeViewer *gtkhtml2_viewer_create(void)
{
	GtkHtml2Viewer *viewer;
	GtkAdjustment *adj;

	debug_print("gtkhtml2_viewer_create\n");

	viewer = g_new0(GtkHtml2Viewer, 1);
	viewer->mimeviewer.factory = &gtkhtml2_viewer_factory;
	viewer->mimeviewer.get_widget = gtkhtml2_get_widget;
	viewer->mimeviewer.show_mimepart = gtkhtml2_show_mimepart;
	viewer->mimeviewer.clear_viewer = gtkhtml2_clear_viewer;
	viewer->mimeviewer.destroy_viewer = gtkhtml2_destroy_viewer;
	viewer->mimeviewer.get_selection = gtkhtml2_get_selection;

	viewer->html_doc = html_document_new();
	viewer->html_view = html_view_new();
	viewer->scrollwin = gtk_scrolled_window_new(NULL, NULL);
	viewer->base      = NULL;
	viewer->mimeinfo  = NULL;
	viewer->force_image_loading = FALSE;
	viewer->tag       = -1;
	viewer->mutex     = g_mutex_new();

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

	gtk_widget_show(GTK_WIDGET(viewer->scrollwin));
	gtk_widget_ref(GTK_WIDGET(viewer->scrollwin));
	gtk_widget_show(GTK_WIDGET(viewer->html_view));
	gtk_widget_ref(GTK_WIDGET(viewer->html_view));

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
	gtkhtml2_viewer_tmpdir = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S,
				"gtkhtml2_viewer", NULL);

	if ((sylpheed_get_version() > VERSION_NUMERIC)) {
		*error = g_strdup("Your version of Sylpheed-Claws is newer than the version the Gtkhtml2Viewer plugin was built with");
		return -1;
	}

	if ((sylpheed_get_version() < MAKE_NUMERIC_VERSION(2, 4, 0, 78))) {
		*error = g_strdup("Your version of Sylpheed-Claws is too old for the Gtkhtml2Viewer plugin");
		return -1;
	}

#ifdef HAVE_LIBCURL
	gtkhtml_prefs_init();
#endif
	mimeview_register_viewer_factory(&gtkhtml2_viewer_factory);
	curl_global_init(CURL_GLOBAL_DEFAULT);

	if (!is_dir_exist(gtkhtml2_viewer_tmpdir))
		make_dir_hier(gtkhtml2_viewer_tmpdir);

	return 0;
}

void gtkhtml2_viewer_clear_cache(void)
{
	remove_dir_recursive(gtkhtml2_viewer_tmpdir);
	make_dir_hier(gtkhtml2_viewer_tmpdir);
}

void plugin_done(void)
{
	if (gtkhtml_prefs.clear_cache)
		remove_dir_recursive(gtkhtml2_viewer_tmpdir);
	g_free(gtkhtml2_viewer_tmpdir);
	gtkhtml2_viewer_tmpdir = NULL;

#ifdef HAVE_LIBCURL
	gtkhtml_prefs_done();
#endif
	mimeview_unregister_viewer_factory(&gtkhtml2_viewer_factory);
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
	return "GPL";
}

const gchar *plugin_version(void)
{
	return PLUGINVERSION;
}

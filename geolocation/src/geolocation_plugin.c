/* GeoLocation plugin for Claws-Mail
 * Copyright (C) 2009 Holger Berndt
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#include "pluginconfig.h"

#include <glib.h>

#include "pixmap_earth.h"

#include "common/plugin.h"
#include "common/version.h"
#include "common/utils.h"
#include "common/hooks.h"
#include "prefs_common.h"
#include "messageview.h"
#include "procheader.h"
#include "main.h"

#include "gettext.h"

#include <libxml/nanohttp.h>
#include <libxml/xpath.h>

#include <champlain/champlain.h>
#include <champlain-gtk/champlain-gtk.h>
#include <clutter-gtk/gtk-clutter-embed.h>

#include <string.h>


/* For now, make extracting the ip address string from the Received header
 * as simple as possible. This should be made more accurate, some day.
 * Logic:
 * Find four 1-3 digit numbers, separated by dots, encosed in word boundaries,
 * where the word "from" must be ahead, and the word "by" must follow, but must
 * not be between the "from" and the digits.
 * The result will be in backreference 2, the individual numbers in 3 to 6 */
#define EXTRACT_IP_FROM_RECEIVED_HEADER_REGEX "\\bfrom\\b((?!\\bby\\b).)*(\\b(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\b).*\\bby\\b"

#define EXTRACT_LAT_LON_FROM_HOSTIP_RESPONSE_REGEX "<gml:coordinates>(-?[\\d.]+),(-?[\\d.]+)</gml:coordinates>"

#define GEOLOCATION_CONTAINER_NAME "cm_geolocation_container"
#define GEOLOCATION_CONTAINER_DATA_TOGGLEBUTTON "togglebutton"
#define GEOLOCATION_CONTAINER_DATA_LABEL "label"

#define GEOLOCATION_TOGGLE_BUTTON_DATA_CB_ID "toggled_cb_id"
#define GEOLOCATION_TOGGLE_BUTTON_DATA_IP "ip"

#define GEOLOCATION_NOTEBOOK_DATA_MAPWIDGET "mapwidget"

#define GEOLOCATION_MAP_DATA_VIEW "view"
#define GEOLOCATION_MAP_DATA_ORIGIN_MARKER "originmarker"

#define BUFFSIZE 8192

#define HOSTIP_URL "http://api.hostip.info/"

static gboolean my_messageview_show_hook(gpointer, gpointer);
static guint hook_messageview_show;

static GRegex *ip_from_received_header_regex = NULL;
static GRegex *lat_lon_from_hostip_response_regex = NULL;

static GSList *container_list = NULL;


static gchar* get_ip_from_received_header(gchar *received)
{
  GMatchInfo *match_info;
  gchar *ip;
  gchar *n1;

  g_regex_match(ip_from_received_header_regex, received, 0, &match_info);
  ip = g_match_info_fetch(match_info, 2);

  if (ip) {
    /* check for loopback and local subnetworks */
    if(!strcmp(ip, "127.0.0.1")) {
      g_free(ip);
      ip = NULL;
    }
    else {
      n1 = g_match_info_fetch(match_info, 3);
      if(n1) {
        /* 10.x.x.x */
        if(!strcmp(n1, "10")) {
          g_free(ip);
          ip = NULL;
        }
        /* 192.168.x.x */
        else if(!strcmp(n1, "192")) {
          gchar *n2;
          n2 = g_match_info_fetch(match_info, 4);
          if(n2 && !strcmp(n2, "168")) {
            g_free(ip);
            ip = NULL;
          }
        }
        /* 172.16.xx to 172.31.x.x */
        else if(!strcmp(n1, "172")) {
          gchar *n2;
          int i2;
          n2 = g_match_info_fetch(match_info, 4);
          if(n2) {
            i2 = atoi(n2);
            if((i2 >= 16) && (i2 <= 31)) {
              g_free(ip);
              ip = NULL;
            }
          }
        }
      }
    }
  }

  g_match_info_free(match_info);
  return ip;
}

enum {
  H_RECEIVED = 0
};

static HeaderEntry hentry[] = {
    {"Received:", NULL, TRUE},
    {NULL, NULL, FALSE}};

static gchar* get_ip_from_msginfo(MsgInfo *msginfo)
{
  gchar *file;
  struct stat ss;
  FILE *fp;
  gchar buf[BUFFSIZE];
  gint hnum;
  gchar *hp;
  gchar *ip;
  gchar *new_ip;

  file = procmsg_get_message_file_path(msginfo);
  g_return_val_if_fail(file, NULL);

  if(g_stat(file, &ss) < 0) {
    FILE_OP_ERROR(file, "stat");
    return NULL;
  }
  if(!S_ISREG(ss.st_mode)) {
    g_warning("mail file is not a regular file\n");
    return NULL;
  }

  if((fp = g_fopen(file, "rb")) == NULL) {
    FILE_OP_ERROR(file, "fopen");
    return NULL;
  }

  ip = NULL;
  new_ip = NULL;
  while((hnum = procheader_get_one_field(buf, sizeof(buf), fp, hentry)) != -1) {
    switch(hnum) {
    case H_RECEIVED:
      hp = buf + strlen(hentry[hnum].name);
      while(*hp == ' ' || *hp == '\t') hp++;
      new_ip = get_ip_from_received_header(hp);
      if(new_ip) {
        g_free(ip);
        ip = new_ip;
      }
      break;
    default:
      break;
    }
  }

  fclose(fp);

  g_free(file);
  return ip;
}

/* inspired by GeoClue */
static gboolean get_lat_lon_from_ip(const gchar *ip, double *lat, double *lon)
{
  gchar *url;
  void* ctxt = NULL;
  xmlBuffer *output;
  gint len;
  guchar *response;
  gint response_length;
  xmlChar buf[1024];
  GMatchInfo *match_info;
  gchar *slat;
  gchar *slon;

  /* fetch data from hostip.info */
  url = g_strdup_printf("%s?ip=%s", HOSTIP_URL, ip);
  xmlNanoHTTPInit();
  ctxt = xmlNanoHTTPMethod(url, "GET", NULL, NULL, NULL, 0);
  g_free(url);
  if(!ctxt) {
    g_print("xmlNanoHTTPMethod did not get a response from %s\n", url);
    return FALSE;
  }

  output = xmlBufferCreate();
  while((len = xmlNanoHTTPRead(ctxt, buf, sizeof(buf))) > 0) {
    if(xmlBufferAdd(output, buf, len) != 0) {
      xmlNanoHTTPClose(ctxt);
      xmlBufferFree(output);
      g_print("xmlBufferAdd failed\n");
      return FALSE;
    }
  }
  xmlNanoHTTPClose(ctxt);

  response_length = xmlBufferLength(output);
  response = g_malloc(response_length+1);
  memcpy(response, xmlBufferContent(output), response_length);
  response[response_length] = '\0';
  xmlBufferFree(output);

  /* extract longitude and latitude from */
  g_regex_match(lat_lon_from_hostip_response_regex, response, 0, &match_info);
  slon = g_match_info_fetch(match_info, 1);
  slat = g_match_info_fetch(match_info, 2);
  g_match_info_free(match_info);

  if(!slat || !slon)
    return FALSE;

  *lat = g_ascii_strtod(slat, NULL);
  *lon = g_ascii_strtod(slon, NULL);

  return TRUE;
}

static GtkWidget* create_map_widget()
{
  GtkWidget *map;
  ChamplainLayer *layer;
  ChamplainView *view;
  ClutterActor *origin_marker;
  ClutterColor orange = { 0xf3, 0x94, 0x07, 0xbb };

  /* create map widget */
  map = gtk_champlain_embed_new();
  view = gtk_champlain_embed_get_view(GTK_CHAMPLAIN_EMBED(map));
  g_object_set(G_OBJECT(view), "scroll-mode", CHAMPLAIN_SCROLL_MODE_KINETIC, "zoom-level", 8, NULL);

  /* mail origin marker */
  layer = champlain_layer_new();
  origin_marker = champlain_marker_new_with_text("mail origin", "Serif 14", NULL, NULL);
  champlain_marker_set_color(CHAMPLAIN_MARKER(origin_marker), &orange);
  clutter_container_add(CLUTTER_CONTAINER(layer), origin_marker, NULL);
  champlain_view_add_layer(view, layer);

  clutter_actor_show(CLUTTER_ACTOR(view));
  clutter_actor_show(CLUTTER_ACTOR(layer));
  clutter_actor_show(origin_marker);

  g_object_set_data(G_OBJECT(map), GEOLOCATION_MAP_DATA_VIEW, view);
  g_object_set_data(G_OBJECT(map), GEOLOCATION_MAP_DATA_ORIGIN_MARKER, origin_marker);

  return map;
}

static int get_notebook_map_page_num(GtkWidget *notebook)
{
  GtkWidget *wid;
  int page_num = 0;

  wid = g_object_get_data(G_OBJECT(notebook), GEOLOCATION_NOTEBOOK_DATA_MAPWIDGET);

  if(!wid) {
    /* create page */
    GtkWidget *map;
    map = create_map_widget();
    page_num = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), map, NULL);
    gtk_widget_show_all(map);
    if(page_num == -1)
      page_num = 0;
    else
      g_object_set_data(G_OBJECT(notebook), GEOLOCATION_NOTEBOOK_DATA_MAPWIDGET, map);
  }
  else {
    /* find page */
    page_num = gtk_notebook_page_num(GTK_NOTEBOOK(notebook), wid);
    if(page_num == -1)
      page_num = 0;
  }

  return page_num;
}

static void geolocation_button_toggled_cb(GtkToggleButton *button, gpointer data)
{
  if(gtk_toggle_button_get_active(button)) {
    gchar *ip;
    double lat, lon;
    gchar *text = NULL;
    GtkWidget *container, *label;

    ip = g_object_get_data(G_OBJECT(button), GEOLOCATION_TOGGLE_BUTTON_DATA_IP);
    container = gtk_widget_get_parent(GTK_WIDGET(button));
    g_return_if_fail(container);
    label = g_object_get_data(G_OBJECT(container), GEOLOCATION_CONTAINER_DATA_LABEL);
    g_return_if_fail(label);

    if(ip && get_lat_lon_from_ip(ip, &lat, &lon)) {
      GtkWidget *notebook;
      GtkWidget *map;
      MessageView *messageview;
      ClutterActor *view;
      ClutterActor *origin_marker;
      gint page_num;

      /* TRANSLATORS: The two numbers are latitude and longitude coordinates */
      text = g_strdup_printf(_("Found location: (%.2f,%.2f)"), lat, lon);
      gtk_label_set_text(GTK_LABEL(label), text);
      debug_print("%s\n", text);

      /* switch to map widget, and center on found location */
      messageview = data;
      notebook = messageview->mimeview->mime_notebook;
      page_num = get_notebook_map_page_num(notebook);
      map = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page_num);
      view = g_object_get_data(G_OBJECT(map), GEOLOCATION_MAP_DATA_VIEW);
      origin_marker = g_object_get_data(G_OBJECT(map), GEOLOCATION_MAP_DATA_ORIGIN_MARKER);
      champlain_view_center_on(CHAMPLAIN_VIEW(view), lat, lon);
      champlain_base_marker_set_position(CHAMPLAIN_BASE_MARKER(origin_marker), lat, lon);
      gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page_num);
    }
    else {
      /* TRANSLATORS: The IP address is appended to the string */
      text = g_strconcat(_("Could not resolve location of IP address "), ip, NULL);
      gtk_label_set_text(GTK_LABEL(label), text);
      debug_print("%s\n", text);

      gtk_toggle_button_set_active(button, FALSE);
    }
    g_free(text);
  }
  else {
    /* swich to first notebook page */
    MessageView *messageview;
    messageview = data;
    gtk_notebook_set_current_page(GTK_NOTEBOOK(messageview->mimeview->mime_notebook), 0);
  }
}

static void container_finalize_cb(gpointer data, GObject *where_the_object_was)
{
  container_list = g_slist_remove(container_list, where_the_object_was);
}

static GtkWidget* get_geolocation_container_from_messageview(MessageView *messageview)
{
  GtkWidget *container = NULL;
  GtkWidget *vbox;
  GList *children, *walk;
  GtkWidget *button;
  gulong *cb_id;

  g_return_val_if_fail(messageview && messageview->mimeview && messageview->mimeview->paned, NULL);

  vbox = gtk_paned_get_child2(GTK_PANED(messageview->mimeview->paned));

  /* check if there is already a geolocation container */
  children = gtk_container_get_children(GTK_CONTAINER(vbox));
  for(walk = children; walk; walk = walk->next) {
    GtkWidget *child = walk->data;
    if(!strcmp(gtk_widget_get_name(child), GEOLOCATION_CONTAINER_NAME)) {
      container = child;
      break;
    }
  }
  g_list_free(children);

  /* if not, create one */
  if(!container) {
    GtkWidget *label;
    GtkWidget *image;
    GdkPixbuf *pixbuf, *scaled_pixbuf;

    container = gtk_hbox_new(FALSE, 0);
    gtk_widget_set_name(container, GEOLOCATION_CONTAINER_NAME);

    /* button */
    pixbuf = gdk_pixbuf_new_from_inline(-1, pixmap_earth, FALSE, NULL);
    scaled_pixbuf = gdk_pixbuf_scale_simple(pixbuf, 16, 16, GDK_INTERP_BILINEAR);
    g_object_unref(pixbuf);
    image = gtk_image_new_from_pixbuf(scaled_pixbuf);
    gtk_widget_show(image);
    g_object_unref(scaled_pixbuf);
    button = gtk_toggle_button_new();
    gtk_widget_show(button);
    gtk_container_add(GTK_CONTAINER(button), image);
    gtk_box_pack_start(GTK_BOX(container), button, FALSE, FALSE, 0);
    g_object_set_data(G_OBJECT(container), GEOLOCATION_CONTAINER_DATA_TOGGLEBUTTON, button);

    /* label */
    label = gtk_label_new("");
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(container), label, FALSE, FALSE, 0);
    g_object_set_data(G_OBJECT(container), GEOLOCATION_CONTAINER_DATA_LABEL, label);

    gtk_box_pack_start(GTK_BOX(vbox), container, FALSE, FALSE, 0);

    /* add new container to list */
    container_list = g_slist_prepend(container_list, container);
    g_object_weak_ref(G_OBJECT(container), container_finalize_cb, NULL);
  }
  /* if it existed, changed the button callback data */
  else {
    /* disconnect old signal handler from button */
    gulong *id;
    button = g_object_get_data(G_OBJECT(container), GEOLOCATION_CONTAINER_DATA_TOGGLEBUTTON);
    g_return_val_if_fail(button, NULL);
    id = g_object_get_data(G_OBJECT(button), GEOLOCATION_TOGGLE_BUTTON_DATA_CB_ID);
    g_signal_handler_disconnect(button, *id);
  }

  /* connect new signal handler to button, and store its id */
  cb_id = g_new(gulong, 1);
  *cb_id = g_signal_connect(button, "toggled", G_CALLBACK(geolocation_button_toggled_cb), messageview);
  g_object_set_data_full(G_OBJECT(button), GEOLOCATION_TOGGLE_BUTTON_DATA_CB_ID, cb_id, g_free);

  return container;
}

static gboolean my_messageview_show_hook(gpointer source, gpointer data)
{
  MessageView *messageview;
  GtkWidget *container;
  GtkWidget *button;
  gchar *ip;

  messageview = source;

  g_return_val_if_fail(messageview, FALSE);

  container = get_geolocation_container_from_messageview(messageview);
  g_return_val_if_fail(container, FALSE);

  /* make sure toggle button is turned off */
  button = g_object_get_data(G_OBJECT(container), GEOLOCATION_CONTAINER_DATA_TOGGLEBUTTON);
  g_return_val_if_fail(button, FALSE);
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
  }

  /* do nothing except hiding the button if claws mail is in offline mode */
  if(prefs_common.work_offline) {
    gtk_widget_hide(container);
    return FALSE;
  }

  /* try to get ip address from message */
  if(!messageview->msginfo) {
    debug_print("Warning: Messageview does not have a MsgInfo\n");
    return FALSE;
  }
  ip = get_ip_from_msginfo(messageview->msginfo);
  if(!ip) {
    debug_print("Could not get IP address from message\n");
    gtk_widget_hide(container);
  }
  else {
    GtkWidget *label;
    debug_print("Found sender ip address: %s\n", ip);
    label = g_object_get_data(G_OBJECT(container), GEOLOCATION_CONTAINER_DATA_LABEL);
    if(label) {
      gchar *text;
      text = g_strconcat(_("Try to locate sender"), NULL);
      gtk_label_set_text(GTK_LABEL(label), text);
      g_free(text);
    }
    /* button takes ownership of ip string */
    g_object_set_data_full(G_OBJECT(button), GEOLOCATION_TOGGLE_BUTTON_DATA_IP, ip, g_free);
    gtk_widget_show(container);
  }

  return FALSE;
}

gint plugin_init(gchar **error)
{
  GError *err = NULL;

#ifdef G_OS_UNIX
	bindtextdomain(TEXTDOMAIN, LOCALEDIR);
#else
	bindtextdomain(TEXTDOMAIN, get_locale_dir());
#endif
  bind_textdomain_codeset(TEXTDOMAIN, "UTF-8");

  /* Version check */
  if(!check_plugin_version(MAKE_NUMERIC_VERSION(3,7,1,55),
			   VERSION_NUMERIC, _("GeoLocation"), error))
    return -1;

  ip_from_received_header_regex = g_regex_new(EXTRACT_IP_FROM_RECEIVED_HEADER_REGEX, 0, 0, &err);
  if(err) {
    *error = g_strdup_printf(_("Could not create regular expression: %s\n"), err->message);
    g_error_free(err);
    return -1;
  }

  lat_lon_from_hostip_response_regex = g_regex_new(EXTRACT_LAT_LON_FROM_HOSTIP_RESPONSE_REGEX, 0, 0, &err);
  if(err) {
    *error = g_strdup_printf(_("Could not create regular expression: %s\n"), err->message);
    g_error_free(err);
    g_regex_unref(ip_from_received_header_regex);
    return -1;
  }

  hook_messageview_show = hooks_register_hook(MESSAGE_VIEW_SHOW_DONE_HOOKLIST, my_messageview_show_hook, NULL);
  if(hook_messageview_show == (guint) -1) {
    *error = g_strdup(_("Failed to register messageview_show hook in the GeoLocation plugin"));
    return -1;
  }

  gtk_clutter_init(0, NULL);

  debug_print("GeoLocation plugin loaded\n");

  return 0;
}

gboolean plugin_done(void)
{

  hooks_unregister_hook(MESSAGE_VIEW_SHOW_DONE_HOOKLIST, hook_messageview_show);

  if(!claws_is_exiting()) {
    GSList *copy, *walk;

    copy = g_slist_copy(container_list);
    for(walk = copy; walk; walk = walk->next) {
      GtkWidget *wid = walk->data;
      GtkWidget *button = g_object_get_data(G_OBJECT(wid), GEOLOCATION_CONTAINER_DATA_TOGGLEBUTTON);
      if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
      gtk_widget_destroy(GTK_WIDGET(wid));
    }
    g_slist_free(copy);
    if(container_list) {
      g_slist_free(container_list);
      container_list = NULL;
    }

    if(ip_from_received_header_regex) {
      g_regex_unref(ip_from_received_header_regex);
      ip_from_received_header_regex = NULL;
    }

    if(lat_lon_from_hostip_response_regex) {
      g_regex_unref(lat_lon_from_hostip_response_regex);
      lat_lon_from_hostip_response_regex = NULL;
    }
  }

  debug_print("GeoLocation plugin unloaded\n");

  /* returning FALSE because dependant libraries may not be unload-safe. */
  return FALSE;
}

const gchar *plugin_name(void)
{
  return _("GeoLocation");
}

const gchar *plugin_desc(void)
{
  return _("This plugin provides GeoLocation functionality "
	   "for Claws Mail.\n\n"
	   "Warning: It is technically impossible to derive the geographic "
	   "location of senders from their E-Mails with any amount of certainty. "
	   "The results presented by "
	   "this plugin are only rough estimates. In particular, mailing list managers "
	   "often strip sender information from the mails, so mails from mailing lists "
	   "may be assigned to the location of the mailing list server instead of the "
	   "mail sender.\n"
	   "When in doubt, don't trust the results of this plugin, and don't rely on this "
	   "information to divorce your spouse.\n"
           "\nFeedback to <berndth@gmx.de> is welcome "
	   "(but only if it's not about marital quarrels).");
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
    { {PLUGIN_UTILITY, N_("GeoLocation integration")},
      {PLUGIN_NOTHING, NULL}};
  return features;
}

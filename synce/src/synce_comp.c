/* SynCE plugin -- SynCE Support for Claws Mail
 *
 * Copyright (C) 2005-2007 Holger Berndt
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

#include "addritem.h"
#include "addressbook.h"
#include "alertpanel.h"
#include "prefs_gtk.h"
#include "common/defs.h"

#include "synce_comp.h"
#include "synce_rapi.h"


static void     free_email_in_wince_hash(void);
static void     insert_email_key(gchar*, SynCERecord*);
static gint     collect_claws_email(ItemPerson*, const gchar*);
static void     add_to_claws_addressbook(gpointer, gpointer, gpointer);
static gboolean free_email_in_wince_hash_key(gpointer, gpointer, gpointer);
static void     build_list(gpointer, gpointer, gpointer);
static gboolean free_email_not_in_wince_hash_key(gpointer, gpointer, gpointer);
static void     synce_plugin_save_config(void);
static gboolean compare_email(gpointer, gpointer, gpointer);

static GHashTable *email_in_wince_hash     = NULL;
static GHashTable *email_not_in_wince_hash = NULL;
static GHashTable *repeated_wince_hash     = NULL;
static GHashTable *repeated_local_hash     = NULL;

/* configuration */
static SynCEPluginConfig config;

static PrefParam param[] = {
  {"synce_log_level", "2", &config.synce_log_level,
   P_INT, NULL, NULL, NULL},
  {"offer_add_to_local", "TRUE",
   &config.offer_add_to_local, P_BOOL,
   NULL, NULL, NULL},
  {"warn_double_addresses_local", "TRUE",
   &config.warn_double_addresses_local, P_BOOL,
   NULL, NULL, NULL},
  {"warn_double_addresses_wince", "TRUE",
   &config.warn_double_addresses_wince, P_BOOL,
   NULL, NULL, NULL},
  {NULL, NULL, NULL, P_OTHER, NULL, NULL, NULL}
};


gboolean synce_comp(void)
{
  GSList *walk;
  AlertValue val;
  gchar *message;
  gchar **email_addresses;
  gchar **eap;
  gchar *email_list;
  gchar *rcpath;

  if(email_in_wince_hash == NULL)
    email_in_wince_hash = g_hash_table_new(g_str_hash, g_str_equal);
  if(email_not_in_wince_hash == NULL)
    email_not_in_wince_hash = g_hash_table_new(g_str_hash, g_str_equal);

  rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, COMMON_RC, NULL);
  prefs_read_config(param, "SynCEPlugin", rcpath, NULL);
  g_free(rcpath);
  synce_plugin_save_config();


  if(config.warn_double_addresses_wince && (repeated_wince_hash == NULL))
    repeated_wince_hash = g_hash_table_new(g_str_hash, g_str_equal);
  if(config.warn_double_addresses_local && (repeated_local_hash == NULL))
    repeated_local_hash = g_hash_table_new(g_str_hash, g_str_equal);

  if(query_wince(config.synce_log_level) == FALSE) {
    AlertValue val;

    g_hash_table_destroy(email_in_wince_hash);
    email_in_wince_hash = NULL;
    g_hash_table_destroy(email_not_in_wince_hash);
    email_not_in_wince_hash = NULL;
    
    if(repeated_local_hash) {
      g_hash_table_destroy(repeated_local_hash);
      repeated_local_hash = NULL;
    }
    if(repeated_wince_hash) {
      g_hash_table_destroy(repeated_wince_hash);
      repeated_wince_hash = NULL;
    }
    
    val = alertpanel_full("SynCE Plugin",
			  "Error connecting to Windows CE (tm) device",
			  GTK_STOCK_CLOSE, NULL, NULL, FALSE,
			  NULL, ALERT_ERROR, G_ALERTDEFAULT);
    return FALSE;
  }

  if(config.warn_double_addresses_wince) {
    gint number;

    email_addresses = g_new0(gchar*, g_hash_table_size(repeated_wince_hash)+1);
    eap = email_addresses;
    g_hash_table_foreach(repeated_wince_hash, build_list, &eap);
    *eap = NULL;
    if(*email_addresses != NULL) {

      eap = email_addresses;
      while(*eap != NULL) {
	number = GPOINTER_TO_INT(g_hash_table_lookup(repeated_wince_hash, *eap));
	*eap = g_strdup_printf("%s (%d times)", *eap, number);
	*eap++;
      }

      email_list = g_strjoinv("\n", email_addresses);

      eap = email_addresses;
      while(*eap != NULL) {
	g_free(*eap);
	*eap++;
      }

      message = g_strconcat("The following email address(es) were found "
			    "multiple times in the Windows CE (tm) device:\n",
			    email_list, NULL);
      val = alertpanel_full("SynCE Plugin", message,
			    GTK_STOCK_CLOSE, NULL, NULL, FALSE,
			    NULL, ALERT_WARNING, G_ALERTDEFAULT);
      g_free(email_list);
      g_free(message);
    }
    g_free(email_addresses);
    g_hash_table_destroy(repeated_wince_hash);
    repeated_wince_hash = NULL;
  }

  addrindex_load_person_attribute(NULL, collect_claws_email);

  if(config.warn_double_addresses_local) {
    gint number;

    email_addresses = g_new0(gchar*, g_hash_table_size(repeated_local_hash)+1);
    eap = email_addresses;
    g_hash_table_foreach(repeated_local_hash, build_list, &eap);
    *eap = NULL;
    if(*email_addresses != NULL) {

      eap = email_addresses;
      while(*eap != NULL) {
	number = GPOINTER_TO_INT(g_hash_table_lookup(repeated_local_hash, *eap));
	*eap = g_strdup_printf("%s (%d times)", *eap, number);
	*eap++;
      }

      email_list = g_strjoinv("\n", email_addresses);

      eap = email_addresses;
      while(*eap != NULL) {
	g_free(*eap);
	*eap++;
      }

      message = g_strconcat("The following email address(es) were found "
			    "multiple times in a local addressbook:\n",
			    email_list, NULL);
      val = alertpanel_full("SynCE Plugin", message,
			    GTK_STOCK_CLOSE, NULL, NULL, FALSE,
			    NULL, ALERT_WARNING, G_ALERTDEFAULT);
      g_free(email_list);
      g_free(message);
    }
    g_free(email_addresses);
    g_hash_table_destroy(repeated_local_hash);
    repeated_local_hash = NULL;
  }

  g_hash_table_foreach_remove(email_not_in_wince_hash,
			      compare_email, NULL);

  /* Now email_not_in_wince_hash holds a list of email addresses from Claws'
     addressbook which are not in wince, and email_in_wince_hash holds a hash
     of email addresses from wince which are not in Claws. */
  if(config.offer_add_to_local) {
    g_hash_table_foreach(email_in_wince_hash, add_to_claws_addressbook, NULL);
  }
  else {
    email_addresses = g_new0(gchar*, g_hash_table_size(email_in_wince_hash)+1);
    eap = email_addresses;
    g_hash_table_foreach(email_in_wince_hash, build_list, &eap);
    *eap = NULL;

    if(*email_addresses != NULL) {
      email_list = g_strjoinv("\n", email_addresses);
      message = g_strconcat("The following email address(es) were not found in "
			    "a local addressbook:\n", email_list, NULL);
      val = alertpanel_full("SynCE Plugin", message,
			    GTK_STOCK_CLOSE, NULL, NULL, FALSE,
			    NULL, ALERT_WARNING, G_ALERTDEFAULT);
      g_free(email_list);
      g_free(message);
    }
    g_free(email_addresses);
    
  }
  g_hash_table_foreach_remove(email_in_wince_hash, free_email_in_wince_hash_key, NULL);
  g_hash_table_destroy(email_in_wince_hash);
  email_in_wince_hash = NULL;

  email_addresses = g_new0(gchar*, g_hash_table_size(email_not_in_wince_hash)+1);
  eap = email_addresses;
  g_hash_table_foreach(email_not_in_wince_hash, build_list, &eap);
  *eap = NULL;

  if(*email_addresses != NULL) {
    email_list = g_strjoinv("\n", email_addresses);
    message = g_strconcat("The following email address(es) were not found in "
			  "the Windows CE (tm) device:\n", email_list, NULL);
    val = alertpanel_full("SynCE Plugin", message,
			  GTK_STOCK_CLOSE, NULL, NULL, FALSE,
			  NULL, ALERT_WARNING, G_ALERTDEFAULT);
    g_free(email_list);
    g_free(message);
  }
  g_free(email_addresses);

  g_hash_table_foreach_remove(email_not_in_wince_hash,
			      free_email_not_in_wince_hash_key, NULL);
  g_hash_table_destroy(email_not_in_wince_hash);
  email_not_in_wince_hash = NULL;

  message = g_strdup("SynCE Plugin done.");
  val = alertpanel_full("SynCE Plugin", message,
			GTK_STOCK_CLOSE, NULL, NULL, FALSE,
			NULL, ALERT_WARNING, G_ALERTDEFAULT);
  g_free(message);

  return TRUE;
}

gboolean collect_record(SynCERecord *record)
{
  GSList *walk;
  gchar *address;

  if(record == NULL)
    return TRUE;

  walk = record->email_addresses;
  while(walk) {
    if((address = (gchar*)walk->data) != NULL) {
      insert_email_key(address, record);
    }
    walk = g_slist_next(walk);
  }
  return TRUE;
}

static gboolean free_email_in_wince_hash_key(gpointer key, gpointer value,
				    gpointer user_data)
{
  SynCEPerson *person;

  person = (SynCEPerson *) value;
  if(person != NULL) {
    if(person->first_name != NULL) {
      g_free(person->first_name);
      person->first_name = NULL;
    }
    if(person->last_name != NULL) {
      g_free(person->last_name);
      person->last_name = NULL;
    }
    g_free(person);
    person = NULL;
  }
  if(key != NULL) {
    g_free(key);
    key = NULL;
  }
  return TRUE;
}

static gboolean free_email_not_in_wince_hash_key(gpointer key, gpointer value,
				    gpointer user_data)
{
  if(key != NULL) {
    g_free(key);
    key = NULL;
  }
  return TRUE;
}

static void insert_email_key(gchar *address, SynCERecord *record)
{
  AlertValue val;
  gchar *addr;
  gchar *message;
  SynCEPerson *person;

  addr = g_strdup(address);
  g_strdown(addr);

  if((person = g_hash_table_lookup(email_in_wince_hash, addr)) != NULL) {
    gpointer origkey;

    if(config.warn_double_addresses_wince) {
      gint number;
      gpointer value;
      
      g_hash_table_lookup_extended(email_in_wince_hash, addr,
				   &origkey, &value);
      value = g_hash_table_lookup(repeated_wince_hash, addr);
      if(value == NULL)
	g_hash_table_insert(repeated_wince_hash, origkey, GINT_TO_POINTER(2));
      else {	  
	number = GPOINTER_TO_INT(value);
	g_hash_table_insert(repeated_wince_hash, origkey, GINT_TO_POINTER(++number));
      }
    }
    g_free(addr);
    return;
  }

  person = g_new0(SynCEPerson, 1);
  if(record->first_name != NULL)
    person->first_name = g_strdup(record->first_name);
  if(record->last_name != NULL)
    person->last_name  = g_strdup(record->last_name);
  g_hash_table_insert(email_in_wince_hash, addr, person);
}

static gint collect_claws_email(ItemPerson *itemperson, const gchar *bookname)
{
  gchar *addr;
  GList *nodeM;
  gpointer origkey;
  gpointer value;

  /* Process each E-Mail address */
  nodeM = itemperson->listEMail;
  while(nodeM) {
    ItemEMail *email = nodeM->data;

    addr = g_strdup(email->address);
    g_strdown(addr);
    value = g_hash_table_lookup(email_not_in_wince_hash, addr);
    if(value == NULL)
      g_hash_table_insert(email_not_in_wince_hash, addr, GINT_TO_POINTER(1));
    else {
      gpointer origkey;

      if(config.warn_double_addresses_local) {
	gint number;
	gpointer value;
	
	g_hash_table_lookup_extended(email_not_in_wince_hash, addr,
				     &origkey, &value);
	value = g_hash_table_lookup(repeated_local_hash, addr);
	if(value == NULL)
	  g_hash_table_insert(repeated_local_hash, origkey, GINT_TO_POINTER(2));
	else {	  
	  number = GPOINTER_TO_INT(value);
	  g_hash_table_insert(repeated_local_hash, origkey, GINT_TO_POINTER(++number));
	}
      }
      g_free(addr);
    }
    nodeM = g_list_next(nodeM);
  }
  return 0;
}

static void add_to_claws_addressbook(gpointer key, gpointer value,
				     gpointer user_data)
{
  SynCEPerson *person;
  gchar *fromname;

  person = (SynCEPerson*)value;
  fromname = g_strdup_printf("%s%s%s", (person->first_name)?(person->first_name):"",
			               (person->first_name)?" "                 :"",
			               (person->last_name) ?(person->last_name) :"");
  addressbook_add_contact(fromname, (gchar*)key, NULL);
  g_free(fromname);
}

static void build_list(gpointer key, gpointer value,
				    gpointer user_data)
{
  gchar ***eap;

  eap = user_data;
  **eap = key;
  (*eap)++;
}

static void synce_plugin_save_config(void)
{
  PrefFile *pfile;
  gchar *rcpath;

  debug_print("Saving SynCE Plugin Configuration\n");

  rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, COMMON_RC, NULL);
  pfile = prefs_write_open(rcpath);
  g_free(rcpath);
  if (!pfile || (prefs_set_block_label(pfile, "SynCEPlugin") < 0))
    return;
  
  if (prefs_write_param(param, pfile->fp) < 0) {
    g_warning("SynCE Plugin: Failed to write SynCE Plugin configuration to file");
    prefs_file_close_revert(pfile);
    return;
  }
  fprintf(pfile->fp, "\n");
  prefs_file_close(pfile);
}

static gboolean compare_email(gpointer key, gpointer value,
			      gpointer user_data)
{
  SynCEPerson *person;
  gpointer origkey;
  gpointer value_wince;
  
  person = (SynCEPerson*) g_hash_table_lookup(email_in_wince_hash, key);

  /* email is not in pda -> leave it in email_not_in_wince_hash */
  if(person == NULL)
    return FALSE;
    
  /* email is in both -> remove from both hashes */
  g_hash_table_lookup_extended(email_in_wince_hash, key,
			       &origkey, &value_wince);
  g_hash_table_remove(email_in_wince_hash, origkey);
  free_email_in_wince_hash_key(origkey, value_wince, NULL);

  g_hash_table_lookup_extended(email_not_in_wince_hash, key,
			       &origkey, &value_wince);
  g_free(origkey);
  return TRUE;
}

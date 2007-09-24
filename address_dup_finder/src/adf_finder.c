/* Address duplicate finder plugin for Claws-Mail
 * Copyright (C) 2007 Holger Berndt
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

#include "adf_finder.h"
#include "adf_prefs.h"
#include "adf_gtk.h"

#include "addritem.h"
#include "addressbook.h"
#include "alertpanel.h"

#include <gtk/gtk.h>
#include <string.h>

/* email address -> list of address books */
static GHashTable *addr_hash;

static void destroy_hash_val(gpointer);
static GSList* deep_copy_hash_val(GSList*);
static void fill_hash_table(GHashTable*);

static gint collect_emails(ItemPerson*, const gchar*);
static gboolean is_not_duplicate(gpointer, gpointer, gpointer);
static gint books_compare(gconstpointer, gconstpointer);


static gint books_compare(gconstpointer a, gconstpointer b)
{
  return strcmp((const char*)a,(const char*)b);
}

void adf_finder_find(void)
{
  if(!addr_hash)
    addr_hash = g_hash_table_new_full(g_str_hash, g_str_equal,
				      g_free, destroy_hash_val);
  fill_hash_table(addr_hash);
  adf_present_finder_results(addr_hash);
  addr_hash = NULL;
}

static void fill_hash_table(GHashTable *addr_hash)
{
  addrindex_load_person_attribute(NULL, collect_emails);
  g_hash_table_foreach_remove(addr_hash,is_not_duplicate, NULL);
}

static gboolean is_not_duplicate(gpointer key, gpointer value,
				 gpointer user_data)
{
  gboolean is_in_same_book;
  gboolean is_in_other_books;
  GSList *books;
  GSList *walk;
  gboolean retval;
  GSList *list = (GSList*)value;;

  /* remove everything that is just in one book */
  if(g_slist_length(list) <= 1)
    return TRUE;

  /* work on a shallow copy */
  books = g_slist_copy(list);

  /* sorting the list makes it easier to check for books */
  books = g_slist_sort(books, books_compare);

  /* check is a book appears twice */
  is_in_same_book = FALSE;
  for(walk = books; walk && walk->next; walk = walk->next) {
    if(strcmp((const char*)walk->data, (const char*)walk->next->data) == 0) {
      is_in_same_book = TRUE;
      break;
    }
  }

  /* check is at least two different books appear in the list */
  is_in_other_books = FALSE;
  if(books && books->next) {
    for(walk = books->next; walk; walk = walk->next) {
      if(strcmp((const char*)walk->data, (const char*)books->data) != 0) {
	is_in_other_books = TRUE;
	break;
      }
    }
  }

  /* delete the shallow copy */
  g_slist_free(books);

  retval = FALSE;
  if(is_in_same_book && adf_config.include_same_book)
    retval = TRUE;
  if(is_in_other_books && adf_config.include_other_books)
    retval = TRUE;
  retval = !retval;

  return retval;
}

static void destroy_hash_val(gpointer data)
{
  GSList *list = (GSList*) data;
  GSList *walk;
  
  for(walk = list; walk; walk = walk->next) {
    AdfListEntry *entry = (AdfListEntry*) walk->data;
    if(entry->book)
      g_free(entry->book);
    if(entry)
      g_free(entry);
  }
  if(list)
    g_slist_free(list);
}

static GSList* deep_copy_hash_val(GSList *in)
{
  GSList *walk;
  GSList *out = NULL;

  out = g_slist_copy(in);
  for(walk = out; walk; walk = walk->next) {
    AdfListEntry *out_entry;
    AdfListEntry *in_entry = (AdfListEntry*)walk->data;

    out_entry = g_new0(AdfListEntry,1);
    out_entry->person = in_entry->person;
    out_entry->book   = g_strdup(in_entry->book);
    walk->data = out_entry;
  }

  return out;
}

static gint collect_emails(ItemPerson *itemperson, const gchar *book)
{
  gchar *addr;
  GList *nodeM;
  GSList *old_val;
  GSList *new_val;
  AdfListEntry *entry;

  /* Process each E-Mail address */
  nodeM = itemperson->listEMail;
  while(nodeM) {
    ItemEMail *email = nodeM->data;

    addr = g_strdup(email->address);
    g_strdown(addr);
    old_val = (GSList*)g_hash_table_lookup(addr_hash, addr);
    if(old_val)
      new_val = deep_copy_hash_val(old_val);
    else
      new_val = NULL;

    entry = g_new0(AdfListEntry,1);
    entry->person = itemperson;
    entry->book = g_strdup(book);

    new_val = g_slist_prepend(new_val, entry);
    g_hash_table_insert(addr_hash, addr, new_val);
    
    nodeM = g_list_next(nodeM);
  }
  return 0;
}

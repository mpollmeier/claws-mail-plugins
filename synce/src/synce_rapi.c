/*
 * synce_plugin -- SynCE Support for Claws Mail
 *
 * Copyright (C) 2005   Holger Berndt
 *
 * Sylpheed is a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2005 Hiroyuki Yamamoto and the Claws Mail Team
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

#include <rapi.h>
#include <synce_log.h>

#include "contact_ids.h"
#include "synce_rapi.h"
#include "synce_comp.h"

static gboolean process_database(HANDLE, DWORD);
static gboolean process_record(PCEPROPVAL, DWORD);
static gboolean process_property(PCEPROPVAL);
static void free_SynCERecord(void);
static void clear_record(void);

static SynCERecord *record = NULL;


gboolean query_wince(int synce_log_level)
{
  CEDB_FIND_DATA *find_data = NULL;
  WORD db_count = 0;
  WORD iword;
  HANDLE db;

  record = g_new0(SynCERecord,1);
  record->first_name = NULL;
  record->last_name  = NULL;
  record->email_addresses = NULL;

  synce_log_set_level(synce_log_level);

  if(CeRapiInit() != S_OK) {
    g_warning("SynCE Plugin: CeRapiInit failed");
    return FALSE;
  }

  if(CeFindAllDatabases(0, 0xffff, &db_count, &find_data) == FALSE) {
    g_warning("SynCE Plugin: CeFindAllDatabases failed");
    return FALSE;
  }

  for(iword = 0; iword < db_count; iword++) {
    if((db = CeOpenDatabase(&find_data[iword].OidDb, NULL,
			    0, CEDB_AUTOINCREMENT, 0)) == INVALID_HANDLE_VALUE) {
      g_warning("SynCE Plugin: CeOpenDatabase failed");
      continue;
    }

    if(process_database(db, find_data[iword].DbInfo.wNumRecords) == FALSE)
      g_warning("SynCE Plugin: process_database failed");

    if(CeCloseHandle(db) == FALSE)
      g_warning("SynCE Plugin: CeCloseHandle failed");
  }

  if(CeRapiFreeBuffer(find_data) != S_OK)
    g_warning("SynCE Plugin: CeRapiFreeBuffer failed");
    
  if(CeRapiUninit() != S_OK)
    g_warning("SynCE Plugin: CeRapiUninit failed");

  free_SynCERecord();

  return TRUE;
}

static gboolean process_database(HANDLE db, DWORD num_records)
{
  LPBYTE values = NULL;
  DWORD buffer_size = 0;

  unsigned int ui;

  WORD property_count;
  CEOID oid;

  for(ui = 0; ui < num_records; ui++) {

    if((oid = CeReadRecordProps(db, CEDB_ALLOWREALLOC, &property_count, NULL,
					(LPBYTE*)&values, &buffer_size)) == 0) {
      g_warning("SynCE Plugin: CeReadRecordProps failed");
      return FALSE;
    }

    if((process_record((PCEPROPVAL)values, property_count)) == FALSE) {
      g_warning("SynCE Plugin: process_record failed");
      return FALSE;
    }
  }
  return TRUE;
}

static gboolean process_record(PCEPROPVAL value, DWORD property_count)
{
  DWORD idword;

  for(idword = 0; idword < property_count; idword++) {
    if((process_property(value+idword)) == FALSE) {
      g_warning("SynCE Plugin: process_properties failed");
      return FALSE;
    }
  }

  if(collect_record(record) == FALSE)
    g_warning("SynCE Plugin: collect_record failed");
  clear_record();
  
  return TRUE;
}

static gboolean process_property(PCEPROPVAL value)
{
  unsigned int id;
  unsigned int type;

  id   = value->propid >> 16;
  type = value->propid & 0xffff;

  if(type == CEVT_LPWSTR) {
    switch(id) {
    case ID_FIRST_NAME:
      record->first_name = wstr_to_ascii(value->val.lpwstr);
      break;
    case ID_LAST_NAME:
      record->last_name = wstr_to_ascii(value->val.lpwstr);
      break;
    case ID_EMAIL:
    case ID_EMAIL2:
    case ID_EMAIL3:
      record->email_addresses = g_slist_prepend(record->email_addresses,
						wstr_to_ascii(value->val.lpwstr));
      break;
    }
  }  
  return TRUE;
}

static void free_SynCERecord(void)
{
  clear_record();
  g_free(record);
  record = NULL;
}

static void clear_record(void)
{
  if(record->first_name != NULL) {
    free(record->first_name);
    record->first_name = NULL;
  }
  if(record->last_name != NULL) {
    free(record->last_name);
    record->last_name = NULL;
  }
  if(record->email_addresses != NULL) {
    GSList *walk;
    gchar *address;
    walk = record->email_addresses;
    for(; walk != NULL; walk = g_slist_next(walk)) {
      address = (gchar*) walk->data;
      if(address != NULL) {
	free(address);
	address = NULL;
      }
    }
    g_slist_free(record->email_addresses);
    record->email_addresses = NULL;
  }
}

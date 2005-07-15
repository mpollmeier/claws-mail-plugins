/*
 * synce_plugin -- SynCE Support for Sylpheed-Claws
 *
 * Copyright (C) 2005   Holger Berndt
 *
 * Sylpheed is a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2005 Hiroyuki Yamamoto and the Sylpheed-Claws Team
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

#ifndef SC_SYNCE_COMP_H
#define SC_SYNCE_COMP_H SC_SYNCE_COMP_H

#include <glib.h>

#include "synce_rapi.h"

typedef struct {
  gchar *first_name;
  gchar *last_name;
} SynCEPerson;

typedef struct {
  gint synce_log_level;
  gboolean offer_add_to_local;
  gboolean warn_double_addresses_local;
  gboolean warn_double_addresses_wince;
} SynCEPluginConfig;


gboolean synce_comp(void);
gboolean collect_record(SynCERecord*);


#endif /* include guard */

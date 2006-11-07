/* Notification Plugin for Claws Mail
 * Copyright (C) 2005-2006 Holger Berndt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
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

#include "notification_pixbuf.h"

/* The following file was created from the .png file of the SC distribution
 * with the command 
 * gdk-pixbuf-csource --raw --name=<name> file.png > <name>.h
 */
#include "raw_claws_mail_logo_64x64.h"

GdkPixbuf *claws_mail_logo_64x64 = NULL;

GdkPixbuf* notification_pixbuf_get_logo_64x64(void)
{
  if(!claws_mail_logo_64x64) {
    claws_mail_logo_64x64 =
      gdk_pixbuf_new_from_inline(-1, raw_claws_mail_logo_64x64,
				 FALSE, NULL);
  }
  return claws_mail_logo_64x64;
}

void notification_pixbuf_free_all(void)
{
  if(claws_mail_logo_64x64) {
    g_object_unref(claws_mail_logo_64x64);
    claws_mail_logo_64x64 = NULL;
  }
}

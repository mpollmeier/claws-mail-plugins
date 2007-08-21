/* Notification plugin for Claws-Mail
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

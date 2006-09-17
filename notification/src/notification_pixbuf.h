/* Notification Plugin for Sylpheed-Claws
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

#ifndef NOTIFICATION_PIXBUF_H
#define NOTIFICATION_PIXBUF_H NOTIFICATION_PIXBUF_H

#include <gdk/gdk.h>

/* The reference to the returned GdkPixbuf's belongs to notification_pixbuf,
 * and shall not be removed outside. */
GdkPixbuf* notification_pixbuf_get_logo_64x64(void);

void notification_pixbuf_free_all(void);

#endif /* NOTIFICATION_PIXBUF_H */

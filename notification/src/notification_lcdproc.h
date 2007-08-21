/* Notification Plugin for Claws Mail
 * Copyright (C) 2005-2007 Holger Berndt
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

#ifndef NOTIFICATION_LCDPROC_H
#define NOTIFICATION_LCDPROC_H NOTIFICATION_LCDPROC_H

#include "pluginconfig.h"

#ifdef NOTIFICATION_LCDPROC

#define LCDPROC_SPECIFIC_FOLDER_ID_STR "lcd"

void notification_lcdproc_connect(void);
void notification_lcdproc_disconnect(void);

void notification_update_lcdproc(void);

#endif /* NOTIFICATION_LCDPROC */

#endif /* include guard */

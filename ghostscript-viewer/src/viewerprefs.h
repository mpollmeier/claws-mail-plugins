/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2003 Hiroyuki Yamamoto and the Sylpheed-Claws Team
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

#ifndef VIEWERPREFS_H
#define VIEWERPREFS_H

typedef struct _GhostscriptViewerPrefs GhostscriptViewerPrefs;

typedef enum
{
	ZOOM_FREE = 0,
	ZOOM_PAGE = 1,
	ZOOM_WIDTH = 2,
} ZoomMode;

#include <glib.h>

struct _GhostscriptViewerPrefs
{
	gboolean	antialiasing;
	gboolean	respect_eof;
	ZoomMode	autozoom;	
};

extern GhostscriptViewerPrefs ghostscriptviewerprefs;

void ghostscript_viewer_prefs_init	(void);
void ghostscript_viewer_prefs_done	(void);

#endif

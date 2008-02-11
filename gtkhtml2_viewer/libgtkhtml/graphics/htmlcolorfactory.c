/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
   Copyright (C) 2000 CodeFactory AB
   Copyright (C) 2000 Jonas Borgström <jonas@codefactory.se>
   Copyright (C) 2000 Anders Carlsson <andersca@codefactory.se>
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "htmlcolorfactory.h"

static GObjectClass *factory_parent_class = NULL;
static GObjectClass *color_parent_class = NULL;
gint color_usage = 0;

typedef struct {
	const gchar *name;
	gint red;
	gint green;
	gint blue;
} HtmlColorStandard;

static void finalize (GObject *object);

static HtmlColorStandard standard_colors [] = {
	{ "aliceblue",   240, 248, 255},
	{ "antiquewhite",   250, 235, 215},
	{ "aqua",     0, 255, 255},
	{ "aquamarine",   127, 255, 212},
	{ "azure",   240, 255, 255},
	{ "beige",   245, 245, 220},
	{ "bisque",   255, 228, 196},
	{ "black",     0,   0,   0},
	{ "blanchedalmond",   255, 235, 205},
	{ "blue",     0,   0, 255},
	{ "blueviolet",   138,  43, 226},
	{ "brown",   165,  42,  42},
	{ "burlywood",   222, 184, 135},
	{ "cadetblue",    95, 158, 160},
	{ "chartreuse",   127, 255,   0},
	{ "chocolate",   210, 105,  30},
	{ "coral",   255, 127,  80},
	{ "cornflowerblue",   100, 149, 237},
	{ "cornsilk",   255, 248, 220},
	{ "crimson",   220,  20,  60},
	{ "cyan",     0, 255, 255},
	{ "darkblue",     0,   0, 139},
	{ "darkcyan",     0, 139, 139},
	{ "darkgoldenrod",   184, 134,  11},
	{ "darkgray",   169, 169, 169},
	{ "darkgreen",     0, 100,   0},
	{ "darkgrey",   169, 169, 169},
	{ "darkkhaki",   189, 183, 107},
	{ "darkmagenta",   139,   0, 139},
	{ "darkolivegreen",    85, 107,  47},
	{ "darkorange",   255, 140,   0},
	{ "darkorchid",   153,  50, 204},
	{ "darkred",   139,   0,   0},
	{ "darksalmon",   233, 150, 122},
	{ "darkseagreen",   143, 188, 143},
	{ "darkslateblue",    72,  61, 139},
	{ "darkslategray",    47,  79,  79},
	{ "darkslategrey",    47,  79,  79},
	{ "darkturquoise",     0, 206, 209},
	{ "darkviolet",   148,   0, 211},
	{ "deeppink",   255,  20, 147},
	{ "deepskyblue",     0, 191, 255},
	{ "dimgray",   105, 105, 105},
	{ "dimgrey",   105, 105, 105},
	{ "dodgerblue",    30, 144, 255},
	{ "firebrick",   178,  34,  34},
	{ "floralwhite",   255, 250, 240},
	{ "forestgreen",    34, 139,  34},
	{ "fuchsia",   255,   0, 255},
	{ "gainsboro",   220, 220, 220},
	{ "ghostwhite",   248, 248, 255},
	{ "gold",   255, 215,   0},
	{ "goldenrod",   218, 165,  32},
	{ "gray",   128, 128, 128},
	{ "grey",   128, 128, 128},
	{ "green",     0, 128,   0},
	{ "greenyellow",   173, 255,  47},
	{ "honeydew",   240, 255, 240},
	{ "hotpink",   255, 105, 180},
	{ "indianred",   205,  92,  92},
	{ "indigo",    75,   0, 130},
	{ "ivory",   255, 255, 240},
	{ "khaki",   240, 230, 140},
	{ "lavender",   230, 230, 250},
	{ "lavenderblush",   255, 240, 245},
	{ "lawngreen",   124, 252,   0},
	{ "lemonchiffon",   255, 250, 205},
	{ "lightblue",   173, 216, 230},
	{ "lightcoral",   240, 128, 128},
	{ "lightcyan",   224, 255, 255},
	{ "lightgoldenrodyellow",   250, 250, 210},
	{ "lightgray",   211, 211, 211},
	{ "lightgreen",   144, 238, 144},
	{ "lightgrey",   211, 211, 211},
	{ "lightpink",   255, 182, 193},
	{ "lightsalmon",   255, 160, 122},
	{ "lightseagreen",    32, 178, 170},
	{ "lightskyblue",   135, 206, 250},
	{ "lightslategray",   119, 136, 153},
	{ "lightslategrey",   119, 136, 153},
	{ "lightsteelblue",   176, 196, 222},
	{ "lightyellow",   255, 255, 224},
	{ "lime",     0, 255,   0},
	{ "limegreen",    50, 205,  50},
	{ "linen",   250, 240, 230},
	{ "magenta",   255,   0, 255},
	{ "maroon",   128,   0,   0},
	{ "mediumaquamarine",   102, 205, 170},
	{ "mediumblue",     0,   0, 205},
	{ "mediumorchid",   186,  85, 211},
	{ "mediumpurple",   147, 112, 219},
	{ "mediumseagreen",    60, 179, 113},
	{ "mediumslateblue",   123, 104, 238},
	{ "mediumspringgreen",     0, 250, 154},
	{ "mediumturquoise",    72, 209, 204},
	{ "mediumvioletred",   199,  21, 133},
	{ "midnightblue",    25,  25, 112},
	{ "mintcream",   245, 255, 250},
	{ "mistyrose",   255, 228, 225},
	{ "moccasin",   255, 228, 181},
	{ "navajowhite",   255, 222, 173},
	{ "navy",     0,   0, 128},
	{ "oldlace",   253, 245, 230},
	{ "olive",   128, 128,   0},
	{ "olivedrab",   107, 142,  35},
	{ "orange",   255, 165,   0},
	{ "orangered",   255,  69,   0},
	{ "orchid",   218, 112, 214},
	{ "palegoldenrod",   238, 232, 170},
	{ "palegreen",   152, 251, 152},
	{ "paleturquoise",   175, 238, 238},
	{ "palevioletred",   219, 112, 147},
	{ "papayawhip",   255, 239, 213},
	{ "peachpuff",   255, 218, 185},
	{ "peru",   205, 133,  63},
	{ "pink",   255, 192, 203},
	{ "plum",   221, 160, 221},
	{ "powderblue",   176, 224, 230},
	{ "purple",   128,   0, 128},
	{ "red",   255,   0,   0},
	{ "rosybrown",   188, 143, 143},
	{ "royalblue",    65, 105, 225},
	{ "saddlebrown",   139,  69,  19},
	{ "salmon",   250, 128, 114},
	{ "sandybrown",   244, 164,  96},
	{ "seagreen",    46, 139,  87},
	{ "seashell",   255, 245, 238},
	{ "sienna",   160,  82,  45},
	{ "silver",   192, 192, 192},
	{ "skyblue",   135, 206, 235},
	{ "slateblue",   106,  90, 205},
	{ "slategray",   112, 128, 144},
	{ "slategrey",   112, 128, 144},
	{ "snow",   255, 250, 250},
	{ "springgreen",     0, 255, 127},
	{ "steelblue",    70, 130, 180},
	{ "tan",   210, 180, 140},
	{ "teal",     0, 128, 128},
	{ "thistle",   216, 191, 216},
	{ "tomato",   255,  99,  71},
	{ "turquoise",    64, 224, 208},
	{ "violet",   238, 130, 238},
	{ "wheat",   245, 222, 179},
	{ "white",   255, 255, 255},
	{ "whitesmoke",   245, 245, 245},
	{ "yellow",   255, 255,   0},
	{ "yellowgreen",   154, 205,  50}
};

static void
html_color_factory_class_init (HtmlColorFactoryClass *klass)
{
	factory_parent_class = g_type_class_peek_parent (klass);
}

static void
html_color_factory_init (HtmlColorFactory *color_factory)
{
	color_factory->colors = g_hash_table_new (g_int_hash, g_int_equal);
}

GType
html_color_factory_get_type (void)
{
	static GType html_color_factory_type = 0;

	if (!html_color_factory_type) {
		GTypeInfo html_color_factory_info = {
			sizeof (HtmlColorFactoryClass),
			NULL,
			NULL,
			(GClassInitFunc) html_color_factory_class_init,
			NULL,
			NULL,
			sizeof (HtmlColorFactory),
			1,
			(GInstanceInitFunc) html_color_factory_init,
		};

		html_color_factory_type = g_type_register_static (G_TYPE_OBJECT, "HtmlColorFactory", 
								  &html_color_factory_info, 0);
	}

	return html_color_factory_type;
}

static void
parse_name (const gchar *name, gint *red, gint *green, gint *blue)
{
	gint i;

	/* First, try colors of kind #rrggbb */
	if (strlen (name) == 7 && name[0] == '#') {
		gchar *str;

		str = g_strndup (name + 1, 2);
		*red = strtol (str, NULL, 16);
		g_free (str);

		str = g_strndup (name + 3, 2);
		*green = strtol (str, NULL, 16);
		g_free (str);

		str = g_strndup (name + 5, 2);
		*blue = strtol (str, NULL, 16);
		g_free (str);

	}
	
	/* then, try colors of kind rrggbb */
	else if (strlen (name) == 6 && 
		 isxdigit (name[0]) && isxdigit (name[1]) &&
		 isxdigit (name[2]) && isxdigit (name[3]) &&
		 isxdigit (name[4]) && isxdigit (name[5])) {
		gchar *str;
		
		str = g_strndup (name, 2);
		*red = strtol (str, NULL, 16);
		g_free (str);
		
		str = g_strndup (name + 2, 2);
		*green = strtol (str, NULL, 16);
		g_free (str);
		
		str = g_strndup (name + 4, 2);
		*blue = strtol (str, NULL, 16);
		g_free (str);
	}
	/* Next, try colors of kind #rgb that will expand to #rrggbb */
	else if (strlen (name) == 4 && name[0] == '#') {
		gchar *str;
		
		str = g_strndup (name + 1, 1);
		*red = strtol (str, NULL, 16);
		*red += *red * 16;
		g_free (str);

		str = g_strndup (name + 2, 1);
		*green = strtol (str, NULL, 16);
		*green += *green * 16;
		g_free (str);

		str = g_strndup (name + 3, 1);
		*blue = strtol (str, NULL, 16);
		*blue += *blue * 16;
		g_free (str);

	}
	/* Next, try rgb (r, g, b) */
	/* FIXME: Must be able to handle white-space correctly /ac */
	else if (strstr (name, "rgb")) {
		gchar *ptr;
		
		ptr = strstr (color_name, "(");
		if (ptr == NULL) return NULL;
		ptr++;
		while (*ptr && *ptr == ' ') ptr++;
		*red = strtol (ptr, &ptr, 10);
		ptr++;
		while (*ptr && *ptr == ' ') ptr++;
		while (*ptr && *ptr == ',') ptr++;
		while (*ptr && *ptr == ' ') ptr++;
		*green = strtol (ptr, &ptr, 10);
		ptr++;
		while (*ptr && *ptr == ' ') ptr++;
		while (*ptr && *ptr == ',') ptr++;
		while (*ptr && *ptr == ' ') ptr++;
		*blue = strtol (ptr, &ptr, 10);
	}
	/* Finally, try the standard colors */
	else {
		for (i = 0; i < sizeof (standard_colors) / sizeof (standard_colors [0]); i++) {
			if (g_strcasecmp (name, standard_colors [i].name) == 0) {
				*red = standard_colors [i].red;
				*green = standard_colors [i].green;
				*blue = standard_colors [i].blue;
			}
		}
	}
}

HtmlColor *
html_color_factory_get_color_rgb (HtmlColorFactory *color_factory, gint red, gint green, gint blue)
{
	HtmlColor *color;
	gint key;

	key = (red << 16) + (green << 8) + blue;
	
	color = g_hash_table_lookup (color_factory->colors, &key);
	if (color) {
		return (HtmlColor *)g_object_ref (G_OBJECT (color));
	}
	
	color_usage++;
	return HTML_COLOR_FACTORY_GET_CLASS (color_factory)->new_color (color_factory, red, green, blue);
}

HtmlColor *
html_color_factory_get_color (HtmlColorFactory *color_factory, const gchar *color_name)
{
	gint red = -1, blue = -1, green = -1;
	gint key;
	HtmlColor *color;
	
	parse_name (color_name, &red, &green, &blue);
	
	if (red == -1) {
		g_print ("Failed to parse color '%s'\n", color_name);
		return NULL;
	}

	key = (red << 16) + (green << 8) + blue;
	
	color = g_hash_table_lookup (color_factory->colors, &key);
	if (color) {
		return (HtmlColor *)g_object_ref (G_OBJECT (color));
	}
	
	color_usage++;
	return HTML_COLOR_FACTORY_GET_CLASS (color_factory)->new_color (color_factory, red, green, blue);
}

static void
html_color_class_init (HtmlColorClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	color_parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = finalize;
}

static void
html_color_init (HtmlColor *color)
{
}

GType
html_color_get_type (void)
{
	static GType html_color_type = 0;

	if (!html_color_type) {
		GTypeInfo html_color_info = {
			sizeof (HtmlColorClass),
			NULL,
			NULL,
			(GClassInitFunc) html_color_class_init,
			NULL,
			NULL,
			sizeof (HtmlColor),
			1,
			(GInstanceInitFunc) html_color_init,
		};

		html_color_type = g_type_register_static (G_TYPE_OBJECT, "HtmlColor", 
							  &html_color_info, 0);
	}
	return html_color_type;
}

static void
finalize (GObject *object)
{
	HtmlColor *color = HTML_COLOR (object);

	g_hash_table_remove (color->color_factory->colors, &color->key);

	color_usage--;

	G_OBJECT_CLASS (color_parent_class)->finalize (object);
}

HtmlColor *
html_color_transform (HtmlColor *color, gfloat ratio)
{
	return HTML_COLOR_GET_CLASS (color)->transform (color, ratio);
}





/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
   Copyright (C) 2000 CodeFactory AB
   Copyright (C) 2000 Jonas Borgstr\366m <jonas@codefactory.se>
   Copyright (C) 2000 Anders Carlsson <andersca@codefactory.se>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "htmlcolor.h"

typedef struct {
	const gchar *name;
	gint red;
	gint green;
	gint blue;
} HtmlColorStandard;

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

static HtmlColorStandard other_colors [] = {
	{ "linkblue",   0, 0,  254}
};

static HtmlColor *linkblue = NULL;

HtmlColor *
html_color_ref (HtmlColor *color)
{
	color->refcount++;
	return color;
}

void
html_color_unref (HtmlColor *color)
{
	color->refcount--;

	if (color->refcount == 0)
		html_color_destroy (color);
}


void
html_color_destroy (HtmlColor *color)
{
	g_free (color);
}

HtmlColor *
html_color_transparent_new (void)
{
	HtmlColor *result = g_new (HtmlColor, 1);

	result->refcount = 1;
	result->transparent = TRUE;

	return result;
}

HtmlColor *
html_color_dup (HtmlColor *color)
{
	HtmlColor *result;

	if (linkblue == NULL) {
		linkblue = html_color_new_from_name ("linkblue");
	}
	if (html_color_equal (color, linkblue)) {
		return html_color_ref (linkblue);
	}

	result = g_new (HtmlColor, 1);
	result->refcount = 1;
	result->red = color->red;
	result->green = color->green;
	result->blue = color->blue;
	result->transparent = color->transparent;

	return result;
}

HtmlColor *
html_color_new_from_rgb (gushort red, gushort green, gushort blue)
{
	HtmlColor *result = g_new (HtmlColor, 1);

	result->refcount = 1;
	result->transparent = FALSE;
	result->red = red;
	result->green = green;
	result->blue = blue;

	return result;
}

HtmlColor *
html_color_new_from_name (const gchar *color_name)
{
  	gint i;
	gshort red = -1, green = -1, blue = -1;
	HtmlColor *result;

	/* First, try colors of kind #rrggbb */
	if (strlen (color_name) == 7 && color_name[0] == '#') {
		gchar *str;
		
		str = g_strndup (color_name + 1, 2);
		red = strtol (str, NULL, 16);
		g_free (str);
		
		str = g_strndup (color_name + 3, 2);
		green = strtol (str, NULL, 16);
		g_free (str);
		
		str = g_strndup (color_name + 5, 2);
		blue = strtol (str, NULL, 16);
		g_free (str);
		
	}
	
	/* then, try colors of kind rrggbb */
	else if (strlen (color_name) == 6 && 
		 g_ascii_isxdigit (color_name[0]) && g_ascii_isxdigit (color_name[1]) &&
		 g_ascii_isxdigit (color_name[2]) && g_ascii_isxdigit (color_name[3]) &&
		 g_ascii_isxdigit (color_name[4]) && g_ascii_isxdigit (color_name[5])) {
		gchar *str;
		
		str = g_strndup (color_name, 2);
		red = strtol (str, NULL, 16);
		g_free (str);
		
		str = g_strndup (color_name + 2, 2);
		green = strtol (str, NULL, 16);
		g_free (str);
		
		str = g_strndup (color_name + 4, 2);
		blue = strtol (str, NULL, 16);
		g_free (str);
	}
	/* Next, try colors of kind #rgb that will expand to #rrggbb */
	else if (strlen (color_name) == 4 && color_name[0] == '#') {
		gchar *str;
		
		str = g_strndup (color_name + 1, 1);
		red = strtol (str, NULL, 16);
		red += red * 16;
		g_free (str);

		str = g_strndup (color_name + 2, 1);
		green = strtol (str, NULL, 16);
		green += green * 16;
		g_free (str);

		str = g_strndup (color_name + 3, 1);
		blue = strtol (str, NULL, 16);
		blue += blue * 16;
		g_free (str);

	}
	/* Next, try rgb (r, g, b) */
	/* FIXME: Must be able to handle white-space correctly /ac */
	else if (strstr (color_name, "rgb")) {
		gchar *ptr;
		
		ptr = strstr (color_name, "(");
		if (ptr == NULL) return NULL;
		ptr++;
		while (*ptr && *ptr == ' ') ptr++;
		red = strtol (ptr, &ptr, 10);
		ptr++;
		while (*ptr && *ptr == ' ') ptr++;
		while (*ptr && *ptr == ',') ptr++;
		while (*ptr && *ptr == ' ') ptr++;
		green = strtol (ptr, &ptr, 10);
		ptr++;
		while (*ptr && *ptr == ' ') ptr++;
		while (*ptr && *ptr == ',') ptr++;
		while (*ptr && *ptr == ' ') ptr++;
		blue = strtol (ptr, &ptr, 10);
	}
	/* Finally, try the standard colors */
	else {
		/* FIXME: Use this */
		for (i = 0; i < sizeof (standard_colors) / sizeof (standard_colors [0]); i++) {
			if (g_ascii_strcasecmp (color_name, standard_colors [i].name) == 0) {
				red = standard_colors [i].red;
				green = standard_colors [i].green;
				blue = standard_colors [i].blue;
				break;
			}
		}
	}

	if (red == -1 || green == -1 || blue == -1) {
		for (i = 0; i < sizeof (other_colors) / sizeof (other_colors [0]); i++) {
			if (g_ascii_strcasecmp (color_name, other_colors [i].name) == 0) {
				red = other_colors [i].red;
				green = other_colors [i].green;
				blue = other_colors [i].blue;
				break;
			}
		}
	}

	if (red == -1 || green == -1 || blue == -1)
		return NULL;
	
	result = g_new (HtmlColor, 1);

	result->refcount = 1;
	result->red = red;
	result->green = green;
	result->blue = blue;
	result->transparent = FALSE;
	
	return result;
}

gboolean
html_color_equal (HtmlColor *color1, HtmlColor *color2)
{
	if (color1 == color2)
		return TRUE;
	
	if (!color1 || !color2)
		return FALSE;
	
	if (color1->transparent == color2->transparent &&
	    color1->red == color2->red &&
	    color1->green == color2->green &&
	    color1->blue == color2->blue)
		return TRUE;

	return FALSE;
}

HtmlColor *
html_color_transform (HtmlColor *color, gfloat ratio)
{
	gint red, green, blue;
	HtmlColor *result;

	if (ratio > 0) {
		if (color->red * ratio < 0xff) {
			red = color->red * ratio;
			if (red == 0)
				red = 0x40 * ratio;
		}
		else
			red = 0xff;

		if (color->green * ratio < 0xff) {
			green = color->green * ratio;
			if (green == 0)
				green = 0x40 * ratio;
		}
		else
			green = 0xff;

		
		if (color->blue * ratio < 0xff) {
			blue = color->blue * ratio;

			if (blue == 0)
				blue = 0x40 * ratio;
		}
		else
			blue = 0xff;
	}
	else {
		if (color->red * ratio > 0)
			red = color->red * ratio;
		else
			red = 0;

		if (color->green * ratio > 0)
			green = color->green * ratio;
		else
			green = 0;
		
		if (color->blue * ratio > 0)
			blue = color->blue * ratio;
		else
			blue = 0;
	}

	result = html_color_new_from_rgb (red, green, blue);

	return result;
}

void
html_color_set_linkblue (gushort red, gushort green)
{
  	gint i;
	for (i = 0; i < sizeof (other_colors) / sizeof (other_colors [0]); i++) {
		if (g_ascii_strcasecmp ("linkblue", other_colors [i].name) == 0) {
			other_colors [i].red = red;
			other_colors [i].green = green;
			if (linkblue) {
				linkblue->red = red;
				linkblue->green = green;
				linkblue->blue = other_colors [i].blue;
			}
			break;
		}
	}
}

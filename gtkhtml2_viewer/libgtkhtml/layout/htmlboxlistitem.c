/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
   Copyright (C) 2000,2001 CodeFactory AB
   Copyright (C) 2000,2001 Jonas Borgström <jonas@codefactory.se>
   Copyright (C) 2000,2001 Anders Carlsson <andersca@codefactory.se>
   
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

#include <string.h>

#include "htmlboxtext.h"
#include "htmlboxlistitem.h"

static HtmlBoxClass *parent_class = NULL;


static char *
convert_to_roman (long decimal)
{
	char *roman[] = { "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX" };
	static char *result=NULL;

	if (!result)
		result = (char *) g_malloc(50);

	memset (result, 0, 50);

	if (decimal >= 4900 || decimal < 1)
	{
		printf ("Decimal value exceeds 4900 or less than 1\n");
		g_strlcat (result, "\0", sizeof(result));
		return result;
	}

	while (decimal >= 1000)
	{
		decimal -= 1000;
		g_strlcat (result, "M", sizeof(result));
	}
	if (decimal >= 900)
	{
		decimal -= 900;
		g_strlcat (result, "CM", sizeof(result));
	}
	if (decimal >=  500)
	{
		decimal -= 500;
		g_strlcat (result, "D", sizeof(result));
	}
	if (decimal >= 400)
	{
		decimal -= 400;
		g_strlcat (result, "CD", sizeof(result));
	}
	while (decimal >= 100)
	{
		decimal -= 100;
		g_strlcat (result, "C", sizeof(result));
	}
	if (decimal >= 90)
	{
		decimal -= 90;
		g_strlcat (result , "XC", sizeof(result));
	}
	if (decimal >= 50)
	{
		decimal -= 50;
		g_strlcat (result, "L", sizeof(result));
	}
	if (decimal >= 40)
	{
		decimal -= 40;
		g_strlcat (result, "XL", sizeof(result));
	}

	while (decimal >= 10)
	{
		decimal -= 10;
		g_strlcat (result, "X", sizeof(result));
	}
	if (decimal > 0 && decimal < 10)
		g_strlcat (result, *(roman + decimal - 1), sizeof(result));

	return result;

}


static gint
html_box_list_item_left_mbp_sum (HtmlBox *self, gint width)
{
	HtmlStyle *style = HTML_BOX_GET_STYLE (self);

	if (style->inherited->direction != HTML_DIRECTION_LTR || 
	    style->surround->margin.left.value != 0)
		return parent_class->left_mbp_sum (self, width);
	else
		return parent_class->left_mbp_sum (self, width) +
			2 * style->inherited->font_spec->size;
}

static gint
html_box_list_item_right_mbp_sum (HtmlBox *self, gint width)
{
	HtmlStyle *style = HTML_BOX_GET_STYLE (self);

	if (style->inherited->direction != HTML_DIRECTION_RTL || 
	    style->surround->margin.right.value != 0)
		return parent_class->right_mbp_sum (self, width);
	else
		return parent_class->right_mbp_sum (self, width) +
			2 * style->inherited->font_spec->size;
}

static void
html_box_list_item_init_counter (HtmlBox *self, HtmlRelayout *relayout)
{
	HtmlStyle *style = HTML_BOX_GET_STYLE (self);
	HtmlBoxListItem *item = HTML_BOX_LIST_ITEM (self);
	HtmlBox *box = self->prev;
    char lower_alpha='a', upper_alpha='A';
    char *roman_str;

	if (item->counter != 0)
		return;

	while (box && !HTML_IS_BOX_LIST_ITEM (box)) {
		box = box->prev;
	}
	if (box)
		item->counter = HTML_BOX_LIST_ITEM(box)->counter + 1;
	else
		item->counter = 1;

	/* FIXME: add more types here */
	switch (style->inherited->list_style_type) {
	case HTML_LIST_STYLE_TYPE_DECIMAL:
		item->str = g_strdup_printf ("%d. ", item->counter);
		break;
	case HTML_LIST_STYLE_TYPE_DECIMAL_LEADING_ZERO:
		item->str = g_strdup_printf ("%02d. ", item->counter);
		break;
	case HTML_LIST_STYLE_TYPE_LOWER_ALPHA:
		item->str = g_strdup_printf ("%c. ", (lower_alpha + item->counter - 1));
		break;
	case HTML_LIST_STYLE_TYPE_UPPER_ALPHA:
		item->str = g_strdup_printf ("%c. ", (upper_alpha + item->counter - 1));
		break;
	case HTML_LIST_STYLE_TYPE_LOWER_ROMAN:
		roman_str = convert_to_roman (item->counter);
		item->str = g_strdup_printf ("%s. ", g_ascii_strdown (roman_str, strlen(roman_str)));
		break;
	case HTML_LIST_STYLE_TYPE_UPPER_ROMAN:
		item->str = g_strdup_printf ("%s. ", convert_to_roman (item->counter));
		break;
	default:
		break;
	}
	if (item->str) {
		item->label = html_box_text_new (TRUE);
		html_box_text_set_text (HTML_BOX_TEXT (item->label), item->str);
		html_box_set_style (item->label, style);
		item->label->parent = self;
		html_box_relayout (item->label, relayout);
	}
}

static void
html_box_list_item_relayout (HtmlBox *self, HtmlRelayout *relayout)
{
	parent_class->relayout (self, relayout);

	html_box_list_item_init_counter (self, relayout);
}

static void
html_box_list_item_paint (HtmlBox *self, HtmlPainter *painter, GdkRectangle *area, gint tx, gint ty)
{
	HtmlStyle *style = HTML_BOX_GET_STYLE (self);
	HtmlBoxListItem *item = HTML_BOX_LIST_ITEM(self);
	gint offset = 0;

	parent_class->paint (self, painter, area, tx, ty);

	if (item->label) {
		switch (HTML_BOX_GET_STYLE (self)->inherited->direction) {
		case HTML_DIRECTION_LTR:
			offset = (style->surround->margin.left.value ? 
				  style->surround->margin.left.value : 
				  2 * style->inherited->font_spec->size) - item->label->width;
			break;
		case HTML_DIRECTION_RTL:
			offset = self->width - 
				(style->surround->margin.right.value ?
				 style->surround->margin.right.value :
				 2 * style->inherited->font_spec->size) + item->label->width;
			break;
		};
		html_box_paint (item->label, painter, area, tx + self->x + offset, ty + self->y);
	}
	else {
		gint square_size = style->inherited->font_spec->size / 3;
		
		switch (style->inherited->direction) {
		case HTML_DIRECTION_LTR:
			offset = (style->surround->margin.left.value ?
				  style->surround->margin.left.value : 
				  2 * style->inherited->font_spec->size) - 
				style->inherited->font_spec->size / 2 - square_size;
			break;
		case HTML_DIRECTION_RTL:
			offset = self->width - 
				(style->surround->margin.right.value ?
				 style->surround->margin.right.value :
				 2 * style->inherited->font_spec->size) + style->inherited->font_spec->size / 2;
			break;
		};
		html_painter_set_foreground_color (painter, style->inherited->color);
		
		if (style->inherited->list_style_type == HTML_LIST_STYLE_TYPE_DISC)
			html_painter_draw_arc (painter, area, tx + self->x + offset + 1, ty + self->y + square_size + 1, 									square_size + 1, square_size + 1, 0, 64 * 360, TRUE);
		else if (style->inherited->list_style_type == HTML_LIST_STYLE_TYPE_CIRCLE)
			html_painter_draw_arc (painter, area, tx + self->x + offset + 1, ty + self->y + square_size + 1, 									square_size + 1, square_size + 1, 0, 64 * 360, FALSE);
		else		
			html_painter_fill_rectangle (painter, area, tx + self->x + offset + 2, 																ty + self->y + square_size + 1, square_size, square_size);
	}
}

static void
html_box_list_item_finalize (GObject *object)
{
	HtmlBoxListItem *item = HTML_BOX_LIST_ITEM (object);
	if (item->str)
		g_free (item->str);
	if (item->label)
		g_object_unref (G_OBJECT(item->label));
	
	G_OBJECT_CLASS(parent_class)->finalize (object);
}

static void
html_box_list_item_class_init (HtmlBoxListItemClass *klass)
{
	HtmlBoxClass *box_class = (HtmlBoxClass *) klass;
	GObjectClass *object_class = (GObjectClass *) klass;

	box_class->paint      = html_box_list_item_paint;
	box_class->relayout   = html_box_list_item_relayout;
	box_class->left_mbp_sum = html_box_list_item_left_mbp_sum;
	box_class->right_mbp_sum = html_box_list_item_right_mbp_sum;
	object_class->finalize = html_box_list_item_finalize;

	parent_class = g_type_class_peek_parent (klass);
}

static void
html_box_list_item_init (HtmlBoxListItem *box)
{
}

GType 
html_box_list_item_get_type (void)
{
	static GType html_type = 0;

	if (!html_type) {
		static GTypeInfo type_info = {
			sizeof (HtmlBoxListItemClass),
			NULL,
			NULL,
			(GClassInitFunc) html_box_list_item_class_init,
			NULL,
			NULL,
			sizeof (HtmlBoxListItem),
			16,
			(GInstanceInitFunc) html_box_list_item_init
		};

		html_type = g_type_register_static (HTML_TYPE_BOX_BLOCK, "HtmlBoxListItem", &type_info, 0);
	}

	return html_type;
}

HtmlBox *
html_box_list_item_new (void)
{
	return g_object_new (HTML_TYPE_BOX_LIST_ITEM, NULL);
}

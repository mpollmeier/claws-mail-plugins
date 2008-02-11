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

#ifndef __HTMLSTYLE_H__
#define __HTMLSTYLE_H__

#include <glib.h>
#include <libxml/tree.h>

typedef struct _HtmlStyle HtmlStyle;
typedef struct _HtmlColor HtmlColor;
typedef struct _HtmlImage HtmlImage;
typedef struct _HtmlFont HtmlFont;
typedef struct _HtmlStyleBox HtmlStyleBox;
typedef struct _HtmlLengthBox HtmlLengthBox;
typedef struct _HtmlStyleVisual HtmlStyleVisual;
typedef struct _HtmlStyleBackground HtmlStyleBackground;
typedef struct _HtmlStyleBorderInfo HtmlStyleBorderInfo;
typedef struct _HtmlStyleInherited HtmlStyleInherited;

/*
 * HtmlLength stuff
 */

typedef enum {
	HTML_LENGTH_NONE = 0,
	HTML_LENGTH_TEMPORARY,
	HTML_LENGTH_FIXED,
	HTML_LENGTH_PERCENT,
} HtmlLengthType;

typedef struct {
	HtmlLengthType type:2;
	gint value;
	gint calculated_value;
} HtmlLength;

/*
 * Render styles
 */

typedef enum {
	HTML_BACKGROUND_ATTACHMENT_SCROLL = 0,
	HTML_BACKGROUND_ATTACHMENT_FIXED
} HtmlBackgroundAttachmentType;

typedef enum {
	HTML_BACKGROUND_REPEAT_REPEAT = 0,
	HTML_BACKGROUND_REPEAT_REPEAT_X,
	HTML_BACKGROUND_REPEAT_REPEAT_Y,
	HTML_BACKGROUND_REPEAT_NO_REPEAT,
	HTML_BACKGROUND_REPEAT_SCALE
} HtmlBackgroundRepeatType;

typedef enum {
	HTML_CLEAR_NONE = 0,
	HTML_CLEAR_LEFT,
	HTML_CLEAR_RIGHT,
	HTML_CLEAR_BOTH
} HtmlClearType;

typedef enum {
	HTML_OVERFLOW_VISIBLE = 0,
	HTML_OVERFLOW_HIDDEN,
	HTML_OVERFLOW_SCROLL,
	HTML_OVERFLOW_AUTO
} HtmlOverflowType;

typedef enum {
	HTML_VISIBILITY_VISIBLE = 0,
	HTML_VISIBILITY_HIDDEN,
	HTML_VISIBILITY_COLLAPSE
} HtmlVisibilityType;

typedef enum {
	HTML_TEXT_TRANSFORM_NONE = 0,
	HTML_TEXT_TRANSFORM_CAPITALIZE,
	HTML_TEXT_TRANSFORM_UPPERCASE,
	HTML_TEXT_TRANSFORM_LOWERCASE
} HtmlTextTransformType;

typedef enum {
	HTML_WHITE_SPACE_NORMAL = 0,
	HTML_WHITE_SPACE_PRE,
	HTML_WHITE_SPACE_NOWRAP
} HtmlWhiteSpace;

typedef enum {
	HTML_BORDER_STYLE_NONE = 0,
	HTML_BORDER_STYLE_HIDDEN,
	HTML_BORDER_STYLE_DOTTED,
	HTML_BORDER_STYLE_DASHED,
	HTML_BORDER_STYLE_SOLID,
	HTML_BORDER_STYLE_DOUBLE,
	HTML_BORDER_STYLE_GROOVE,
	HTML_BORDER_STYLE_RIDGE,
	HTML_BORDER_STYLE_INSET,
	HTML_BORDER_STYLE_OUTSET
} HtmlBorderStyle;

typedef enum {
	HTML_VERTICAL_ALIGN_BASELINE = 0,
	HTML_VERTICAL_ALIGN_SUB,
	HTML_VERTICAL_ALIGN_SUPER,
	HTML_VERTICAL_ALIGN_TOP,
	HTML_VERTICAL_ALIGN_TEXT_TOP,
	HTML_VERTICAL_ALIGN_MIDDLE,
	HTML_VERTICAL_ALIGN_TEXT_BOTTOM,
	HTML_VERTICAL_ALIGN_BOTTOM
	
} HtmlVerticalAlignType;

typedef enum {
	HTML_DISPLAY_INLINE = 0,
	HTML_DISPLAY_BLOCK,
	HTML_DISPLAY_LIST_ITEM,
	HTML_DISPLAY_RUN_IN,
	HTML_DISPLAY_COMPACT,
	HTML_DISPLAY_MARKER,
	HTML_DISPLAY_TABLE,
	HTML_DISPLAY_INLINE_TABLE,
	HTML_DISPLAY_TABLE_ROW_GROUP,
	HTML_DISPLAY_TABLE_HEADER_GROUP,
	HTML_DISPLAY_TABLE_FOOTER_GROUP,
	HTML_DISPLAY_TABLE_ROW,
	HTML_DISPLAY_TABLE_COLUMN_GROUP,
	HTML_DISPLAY_TABLE_COLUMN,
	HTML_DISPLAY_TABLE_CELL,
	HTML_DISPLAY_TABLE_CAPTION,
	HTML_DISPLAY_NONE
} HtmlDisplayType;

typedef enum {
	HTML_POSITION_STATIC = 0,
	HTML_POSITION_FIXED,
	HTML_POSITION_ABSOLUTE,
	HTML_POSITION_RELATIVE
} HtmlPositionType;

typedef enum {
	HTML_LIST_STYLE_TYPE_DISC = 0,
	HTML_LIST_STYLE_TYPE_CIRCLE,
	HTML_LIST_STYLE_TYPE_SQUARE,
	HTML_LIST_STYLE_TYPE_DECIMAL,
	HTML_LIST_STYLE_TYPE_DECIMAL_LEADING_ZERO,
	HTML_LIST_STYLE_TYPE_LOWER_ROMAN,
	HTML_LIST_STYLE_TYPE_UPPER_ROMAN,
	HTML_LIST_STYLE_TYPE_LOWER_GREEK,
	HTML_LIST_STYLE_TYPE_LOWER_ALPHA,
	HTML_LIST_STYLE_TYPE_LOWER_LATIN,
	HTML_LIST_STYLE_TYPE_UPPER_ALPHA,
	HTML_LIST_STYLE_TYPE_UPPER_LATIN,
	HTML_LIST_STYLE_TYPE_HEBREW,
	HTML_LIST_STYLE_TYPE_ARMENIAN,
	HTML_LIST_STYLE_TYPE_GEORGIAN,
	HTML_LIST_STYLE_TYPE_CJK_IDEOGRAPHIC,
	HTML_LIST_STYLE_TYPE_HIRAGANA,
	HTML_LIST_STYLE_TYPE_KATAKANA,
	HTML_LIST_STYLE_TYPE_HIRAGANA_IROHA,
	HTML_LIST_STYLE_TYPE_KATAKANA_IROHA,
	HTML_LIST_STYLE_TYPE_NONE
} HtmlListStyleTypeType;

typedef enum {
	HTML_TEXT_ALIGN_LEFT = 0,
	HTML_TEXT_ALIGN_RIGHT,
	HTML_TEXT_ALIGN_CENTER,
	HTML_TEXT_ALIGN_JUSTIFY,
	HTML_TEXT_ALIGN_STRING /* Fix this */
} HtmlTextAlignType;

typedef enum {
	HTML_FLOAT_NONE = 0,
	HTML_FLOAT_LEFT,
	HTML_FLOAT_RIGHT,
	HTML_FLOAT_CENTER
} HtmlFloatType;

struct _HtmlStyleBox {
	HtmlLength width;
	HtmlLength min_width;
	HtmlLength max_width;

	HtmlLength height;
	HtmlLength min_height;
	HtmlLength max_height;
};

struct _HtmlStyleBackground {
	HtmlColor *background_color;
	HtmlBackgroundAttachmentType background_attachment:2;
	HtmlBackgroundRepeatType background_repeat:3;
	HtmlImage *background_image;
};

struct _HtmlLengthBox {
	HtmlLength top;
	HtmlLength right;
	HtmlLength bottom;
	HtmlLength left;
};

struct _HtmlStyleVisual {
	HtmlLengthBox clip;
};

struct _HtmlStyleSurround {
	gint ref_count;
	
	HtmlLengthBox offset;
	HtmlLengthBox margin;
	HtmlLengthBox padding;
};

struct _HtmlStyleBorderInfo {
	gushort width;
	HtmlColor *color;
	HtmlBorderStyle border_style;
};

struct _HtmlStyleBorder {
	gint ref_count;
	
	HtmlStyleBorderInfo left;
	HtmlStyleBorderInfo right;
	HtmlStyleBorderInfo top;
	HtmlStyleBorderInfo bottom;
};

struct _HtmlStyleInherited {
	HtmlLength line_height;
	HtmlLength text_indent;

	gint word_spacing:8;
	gint letter_spacing:8;

	HtmlColor *color;
	HtmlFont *font;
};

struct _HtmlStyle {
	HtmlDisplayType display:6;
	HtmlVisibilityType visibility:2;

	HtmlListStyleTypeType list_style_type:5;
	HtmlTextAlignType text_align:3;

	HtmlVerticalAlignType vertical_align:5;
	HtmlPositionType position:3;

	HtmlFloatType Float:2;
	HtmlOverflowType overflow:2;
	HtmlWhiteSpace white_space:2;
	HtmlWhiteSpace text_transform:2;
	HtmlClearType clear:3;

	HtmlStyleBox *box;
	HtmlStyleVisual *visual;
	HtmlStyleBackground *bg;
	HtmlStyleSurround *surround;

	HtmlStyleInherited *inherited;
};


gint html_length_get_value (HtmlLength *length, gint max_value);
gint html_length_get_calculated_value (HtmlLength *length);
void html_length_set_value (HtmlLength *length, gint value, HtmlLengthType type);

//void html_style_inherit (HtmlCSSParser *css_parser, HtmlPainter *painter, HtmlStyle *style, HtmlStyle *parent);
//HtmlStyle *html_style_get (HtmlCSSParser *css_parser, HtmlCSSStylesheet *ss, HtmlPainter *painter, xmlNode *node, HtmlBox *parent_box);

#endif /* __HTML_ */



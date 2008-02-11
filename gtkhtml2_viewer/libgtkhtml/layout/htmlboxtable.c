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

#include <string.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>

#include "layout/htmlboxtable.h"
#include "layout/html/htmlboxform.h"
#include "layout/htmlboxtablerow.h"
#include "layout/htmlboxtablecell.h"
#include "layout/htmlboxtablecaption.h"
#include "layout/htmlrelayout.h"

static HtmlBoxClass *parent_class = NULL;

/**
 * calculate_col_min_max:
 * @self: a table
 * 
 * This function calculats the maximum and minimum width
 * of each column.
 **/
static void
calculate_col_min_max (HtmlBoxTable *table)
{
	gint col, i;

	for (col=0; col < table->cols; col++) 
		table->col_info[col].style_width.type = HTML_LENGTH_AUTO;

	/* Calculate col_min_width[] */
	for (col=0; col < table->cols; col++) {
		gint col_min_width = 0;
		gint col_max_width = 0;

		for (i=0; i< table->rows; i++) {
			gint cell_index = i * table->cols + col;

			if (table->cells[cell_index]) {
				gint span = html_box_table_cell_get_colspan (HTML_BOX_TABLE_CELL (table->cells[cell_index]));

				if (span + col > table->cols)
					span = table->cols - col;

				if (span == 1) {

					/* Get column min value */
					if (col_min_width < table->min_width[cell_index])
						col_min_width = table->min_width[cell_index];
					
					/* Get column max value */
					if (col_max_width < table->max_width[cell_index])
						col_max_width = table->max_width[cell_index];

					if (col_max_width < col_min_width)
						col_max_width = col_min_width;

					switch (table->col_info[col].style_width.type) {
					case HTML_LENGTH_AUTO:
						switch (HTML_BOX_GET_STYLE (table->cells[cell_index])->box->width.type) {
						case HTML_LENGTH_AUTO:
							table->col_info[col].style_width.type = HTML_LENGTH_AUTO;
							break;

						case HTML_LENGTH_FIXED:
							table->col_info[col].style_width.type = HTML_LENGTH_FIXED;
							table->col_info[col].style_width.value = MAX(col_min_width, 
												    HTML_BOX_GET_STYLE (table->cells[cell_index])->box->width.value + 
												     html_box_horizontal_mbp_sum (table->cells[cell_index]));
							break;

						case HTML_LENGTH_PERCENT:
							table->col_info[col].style_width.value = HTML_BOX_GET_STYLE (table->cells[cell_index])->box->width.value;
							table->col_info[col].style_width.type = HTML_LENGTH_PERCENT;
							break;
						}
						break;
					case HTML_LENGTH_FIXED:
						switch (HTML_BOX_GET_STYLE (table->cells[cell_index])->box->width.type) {
						case HTML_LENGTH_AUTO:
							break;
						case HTML_LENGTH_FIXED:
							table->col_info[col].style_width.value = MAX(table->col_info[col].style_width.value, 
												    HTML_BOX_GET_STYLE (table->cells[cell_index])->box->width.value +
												     html_box_horizontal_mbp_sum (table->cells[cell_index]));
							break;

						case HTML_LENGTH_PERCENT:
							table->col_info[col].style_width.value = HTML_BOX_GET_STYLE (table->cells[cell_index])->box->width.value;
							table->col_info[col].style_width.type = HTML_LENGTH_PERCENT;
							break;
						}
						break;
						break;
					case HTML_LENGTH_PERCENT:
						switch (HTML_BOX_GET_STYLE (table->cells[cell_index])->box->width.type) {
						case HTML_LENGTH_AUTO:
							break;
						case HTML_LENGTH_FIXED:
							col_min_width = MAX (col_min_width, HTML_BOX_GET_STYLE (table->cells[cell_index])->box->width.value +
									     html_box_horizontal_mbp_sum (table->cells[cell_index]));
							col_max_width = MAX (col_max_width, HTML_BOX_GET_STYLE (table->cells[cell_index])->box->width.value +
									     html_box_horizontal_mbp_sum (table->cells[cell_index]));
							break;
						case HTML_LENGTH_PERCENT:
							table->col_info[col].style_width.value = MAX(table->col_info[col].style_width.value, 
												    HTML_BOX_GET_STYLE (table->cells[cell_index])->box->width.value);
							break;
						}
						break;
					}
				} 
			}
		}
		table->col_info[col].min = col_min_width;
		table->col_info[col].max = col_max_width;

		/* A special case if you have an empty cell with fixed width */
		if (table->col_info[col].max == 0 && 
		    table->col_info[col].style_width.type == HTML_LENGTH_FIXED)
			table->col_info[col].max = table->col_info[col].style_width.value;
		    
		g_assert (col_min_width <= col_max_width);

	}

#define COL_RATIO(i) (table->col_info[i].min ? ((gfloat)(table->col_info[i].max - table->col_info[i].min) / (gfloat)table->col_info[i].min): 0.0001)

	/* Calculate col_min_width[] for span > 1*/
	for (col=0; col < table->cols; col++) {

		for (i=0; i< table->rows; i++) {
			gint cell_index = i * table->cols + col;

			if (table->cells[cell_index]) {
				gint span = html_box_table_cell_get_colspan (HTML_BOX_TABLE_CELL (table->cells[cell_index]));

				if (span + col > table->cols)
					span = table->cols - col;

				if (span > 1) {
					gint tmp = span, width;

					/* MIN width */

					width = (span - 1) * HTML_BOX_GET_STYLE (HTML_BOX(table))->inherited->border_spacing_horiz;
					if (table->cell_border)
						width += 2* (span - 1);

					while (tmp--)
						width += table->col_info[col + tmp].min;
				       
					if (table->min_width[cell_index] - width > 0) {
						gint i, num_temporary = 0;
						gint diff = table->min_width[cell_index] - width;
						gint left = diff;
						gfloat ratio = 0.0;

						for (i=col; i < span + col; i++)
							if (table->col_info[i].style_width.type == HTML_LENGTH_AUTO) {
								num_temporary++;
								ratio += COL_RATIO(i);
							}
						if (num_temporary) {
							for (i=col; i < span + col; i++) {
								if (table->col_info[i].style_width.type == HTML_LENGTH_AUTO) {

									if (num_temporary > 1 && ratio > 0) {
										/* add the extra space Proportially */
										gint toadd = (gint)(COL_RATIO(i) / ratio * (float)diff);
										g_assert (toadd >= 0);
										toadd = MIN(toadd,left);
										table->col_info[i].min += toadd;
										left -= toadd;
										num_temporary--;
										if (num_temporary == 1)
											diff = left;
									}
									else
										table->col_info[i].min += diff / num_temporary;

									table->col_info[i].max = MAX (table->col_info[i].min, table->col_info[i].max);
								}
								
								table->col_info[col].style_width.value = MAX(table->col_info[col].style_width.value,
													    table->col_info[i].min);
							}
						} 
						else {
							gint num_fixed = 0;
							for (i=col; i < span + col; i++)
								if (table->col_info[i].style_width.type == HTML_LENGTH_FIXED) {
									num_fixed++;
									ratio += COL_RATIO(i);
								}
							for (i=col; i < span + col; i++)
								if (table->col_info[i].style_width.type == HTML_LENGTH_FIXED) {
									if (num_fixed > 1 && ratio > 0) {
										/* add the extra space Proportially */
										gint toadd = (gint)(COL_RATIO(i) / ratio * (float)diff);
										if (toadd < 0) {
											g_print ("g_assert (toadd >= 0);\n");
										}

										toadd = MIN(toadd,left);
										table->col_info[i].min += toadd;
										left -= toadd;
										num_fixed--;
										if (num_fixed == 1)
											diff = left;
									}
									else
										table->col_info[i].min += diff / num_fixed;

									table->col_info[i].max = MAX (table->col_info[i].min, table->col_info[i].max);
								}
						}
					}

					/* MAX width */

					width = (span - 1) * HTML_BOX_GET_STYLE (HTML_BOX(table))->inherited->border_spacing_horiz;
					if (table->cell_border)
						width += 2* (span - 1);
					tmp = span;
					while (tmp--)
						width += table->col_info[col + tmp].max;
				       
					if (table->max_width[cell_index] - width > 0) {
						gint i, num_temporary = 0;
						gint diff = table->max_width[cell_index] - width;
						gint left = diff;
						gfloat ratio = 0.0;

						for (i=col; i < span + col; i++)
							if (table->col_info[i].style_width.type == HTML_LENGTH_AUTO) {
								num_temporary++;
								ratio += COL_RATIO(i);
							}

						if (num_temporary) {
							for (i=col; i < span + col; i++)
								if (table->col_info[i].style_width.type == HTML_LENGTH_AUTO) {
									if (num_temporary > 1 && ratio > 0) {
										/* add the extra space Proportially */
										gint toadd = (gint)(COL_RATIO(i) / ratio * (float)diff);

										g_assert (toadd >= 0);
										toadd = MIN(toadd,left);
										left -= toadd;
										table->col_info[i].max += toadd;
										left -= toadd;
										num_temporary--;
										if (num_temporary == 1)
											diff = left;
									}
									else
										table->col_info[i].max += diff / num_temporary;
									
									table->col_info[i].max = MAX (table->col_info[i].min, table->col_info[i].max);
								}
						}
						else {
							gint num_fixed = 0;/*, tmp_width = 0;*/
							gfloat ratio = 0.0;
							for (i=col; i < span + col; i++)
								if (table->col_info[i].style_width.type == HTML_LENGTH_FIXED) {
									num_fixed++;
									ratio += COL_RATIO(i);
								}

							for (i=col; i < span + col; i++) {
								if (table->col_info[i].style_width.type == HTML_LENGTH_FIXED) {
									if (num_fixed > 1 && ratio > 0) {
										/* add the extra space Proportially */
										gint toadd = (gint)(COL_RATIO(i) / ratio * (float)diff);
										g_assert (toadd >= 0);
										toadd = MIN(toadd,left);
										table->col_info[i].max += toadd;
										left -= toadd;
										num_fixed--;
										if (num_fixed == 1)
											diff = left;
									}
									else
										table->col_info[i].max += diff / num_fixed;
									
									table->col_info[i].max = MAX (table->col_info[i].min, table->col_info[i].max);
								}
							}
						}
					}
				} 
			}
		}
	}
}

static void
count_rows_and_cols (HtmlBoxTable *table, GSList *list, gint **spaninfo)
{
	gint tmp_cols, extra_cols = 0, i;
	
	while (list) {
		HtmlBoxTableRow *row = HTML_BOX_TABLE_ROW (list->data);
		
		g_return_if_fail (HTML_IS_BOX_TABLE_ROW (row));

		if (*spaninfo) {
			for (i = 0; i < table->cols; i++)
				if ((*spaninfo)[i])
					extra_cols++;
		}
		
		tmp_cols = html_box_table_row_get_num_cols (HTML_BOX (row), table->rows) + extra_cols;
		
		if (tmp_cols > table->cols) {
			
			*spaninfo = g_renew (gint, *spaninfo, tmp_cols);
			/* Set the new memory to Zero */
			memset (*spaninfo + table->cols, 0, (tmp_cols - table->cols) * sizeof (gint));
			table->cols = tmp_cols;
		}
		html_box_table_row_update_spaninfo (row, *spaninfo);
		/* Decrese by one */
		for (i = 0; i < table->cols; i++)
			if ((*spaninfo)[i])
				(*spaninfo)[i]--;
		
		table->rows++;
		
		list = list->next;
	}
}

static void
update_cells_info (HtmlBoxTable *table, GSList *list, gint *spaninfo, gint *rownum)
{
	while (list) {
		HtmlBoxTableRow *row = HTML_BOX_TABLE_ROW (list->data);
		gint i;
			
		g_return_if_fail (HTML_IS_BOX_TABLE_ROW (row));

		html_box_table_row_fill_cells_array (HTML_BOX(row), &table->cells[*rownum * table->cols], spaninfo);
		html_box_table_row_update_spaninfo (row, spaninfo);
		
		/* Decrese by one */
		for (i = 0; i < table->cols; i++)
			if (spaninfo[i])
				spaninfo[i]--;

		list = list->next;
		(*rownum)++;
	}
}

/* Remove empty columns on the right side of the table
 */
static void
remove_not_needed_columns (HtmlBoxTable *table)
{
	gint r, real_cols = table->cols;

	for (;;) {
		gint row = 0;
		
		while (row < table->rows && table->cells[row * table->cols + real_cols -1] == NULL)
			row++;
		
		if (row == table->rows && real_cols > 1)
			real_cols--;
		else
			break;
	}
	
	if (real_cols != table->cols) {
		for (r = 1; r < table->rows;r++) {
			memmove (&table->cells[r * real_cols],
				 &table->cells[r * table->cols], 
				 sizeof (HtmlBox *) * real_cols);
		}
		table->cols = real_cols;
	}
}

static void
update_info (HtmlBoxTable *table, HtmlRelayout *relayout)
{
	gint *spaninfo = NULL;
	gint array_size, row = 0;

	table->cols = table->rows = 0;

	/* First find the number of rows and cols */
	count_rows_and_cols (table, table->header_list, &spaninfo);
	count_rows_and_cols (table, table->body_list,   &spaninfo);
	count_rows_and_cols (table, table->footer_list, &spaninfo);

	array_size = table->rows * table->cols;

	/* Don't bother if there aren't any cells */
	if (array_size == 0)
		return;

	/* Allocate or reallocate some memory */
	table->cells = g_renew (HtmlBox *, table->cells, array_size);
	memset (table->cells, 0, array_size * sizeof (HtmlBoxTableCell *));
	table->min_width = g_renew (gint, table->min_width, array_size);
	memset (table->min_width, 0, array_size * sizeof (gint));
	table->max_width = g_renew (gint, table->max_width, array_size);
	memset (table->max_width, 0, array_size * sizeof (gint));
	table->col_info = g_renew (ColumnInfo, table->col_info, table->cols);
	memset (table->col_info, 0, table->cols * sizeof (ColumnInfo));
	table->row_height = g_renew (gint, table->row_height, table->rows);
	memset (table->row_height, 0, table->rows * sizeof (gint));
	memset (spaninfo, 0, table->cols * sizeof (gint));

	/* Fill the table->cells array with usefull info */
	update_cells_info (table, table->header_list, spaninfo, &row);
	update_cells_info (table, table->body_list,   spaninfo, &row);
	update_cells_info (table, table->footer_list, spaninfo, &row);

	g_free (spaninfo);

	remove_not_needed_columns (table);
}

/**
 * html_box_table_get_total_percent:
 * @self: A table
 * 
 * Return value: The total percentage of all the percentage columns.
 **/
static gint
html_box_table_get_total_percent (HtmlBoxTable *table)
{
	gint i, total = 0;

	for (i=0; i< table->cols; i++) {
		if (table->col_info[i].style_width.type == HTML_LENGTH_PERCENT)
			total += table->col_info[i].style_width.value;
	}
	return total > 100 ? 100 : total;
}

/**
 * html_box_table_take_space:
 * @self: a table
 * @type: a length type (none, variable or fixed, not percentage)
 * @space: The space in pixels to distribute.
 * @leave: The space in pixels to leave.
 * 
 * This function will distribute the @space to the columns that have
 * the same type as @type
 * 
 * Return value: It will return the amount of space in pixels it didn't use.
 **/
static gint
html_box_table_take_space (HtmlBoxTable *table, HtmlLengthType type, gint space, gint leave)
{
	gint i, count = 0, tmp;
	gint max_width = 0;
	space -= leave;

	for (i=0; i< table->cols; i++) {
		if (table->col_info[i].style_width.type == type) {
			int to_add = table->col_info[i].min - table->col_info[i].width;
			if (to_add > 0) {
				table->col_info[i].width += to_add;
				space -= to_add;
			}
			max_width += table->col_info[i].max;
			count++;
		}
	}
	while (space > 0) {
		tmp = space;

		for (i=0; i< table->cols; i++) {
			if (table->col_info[i].style_width.type == type) {
				/* Make the space added proportional against the max width of the column */
				gint to_add = (gint)((float) table->col_info[i].max / (float) max_width * (float)tmp);
				if (to_add == 0)
					to_add = 1;
				
				/* ugly hack */
				if (type == HTML_LENGTH_FIXED) {
					if (table->col_info[i].width + to_add > table->col_info[i].style_width.value)
						to_add = table->col_info[i].style_width.value - table->col_info[i].width;
				}
				else {
					if (table->col_info[i].width + to_add > table->col_info[i].max)
						to_add = table->col_info[i].max - table->col_info[i].width;
				}
				table->col_info[i].width += to_add;
				space -= to_add;

				if (space == 0)
					break;
			}
			if (space == 0)
				break;
		}
		if (tmp == space)
			break;
	}
	return space + leave;
}

static gint
html_box_table_take_space_percent (HtmlBoxTable *table, gint space, gint leave, gint total)
{
	gint i, count = 0, tmp;

	space -= leave;

	if (space < 0)
		return 0;

	for (i=0; i< table->cols; i++) {
		if (table->col_info[i].style_width.type == HTML_LENGTH_PERCENT) {

			gint to_add = table->col_info[i].min - table->col_info[i].width;
			if (to_add > 0) {
				table->col_info[i].width += to_add;
				space -= to_add;
			}
			count++;
		}
	}
	while (space > 0) {
		tmp = space;

		for (i=0; i< table->cols; i++) {
			if (table->col_info[i].style_width.type == HTML_LENGTH_PERCENT) {
				
				gint to_add = tmp / count;
				if (to_add == 0)
					to_add = 1;
				
				if (table->col_info[i].width + to_add > total * table->col_info[i].style_width.value / 100)
					to_add = total * table->col_info[i].style_width.value / 100 - table->col_info[i].width;
				
				if (to_add < 0)
					to_add = 0;

				table->col_info[i].width += to_add;
				space -= to_add;

				if (space == 0)
					break;
			}
			if (space == 0)
				break;
		}
		if (tmp == space)
			break;
	}
	return space + leave;
}

static void
layout_fixed (HtmlBoxTable *table, HtmlRelayout *relayout, gint *boxwidth)
{
	HtmlBox *self = HTML_BOX(table);
	gint i, diff, width = 0;
	gint num_fixed = 0;

	/* Reset all values */
	for (i = 0; i < table->cols; i++)
		table->col_info[i].width = 0;
		
	/* Set value on all colums where the first cell has a non 'auto' width */
	for (i = 0; i < table->cols; i++) {
		HtmlBox *box = table->cells[i];

		if (box && HTML_BOX_GET_STYLE (box)->box->width.type != HTML_LENGTH_AUTO) {

			gint span = html_box_table_cell_get_colspan (HTML_BOX_TABLE_CELL (box));
			gint span2 = span;
			gint cell_width = html_length_get_value (&HTML_BOX_GET_STYLE (box)->box->width, *boxwidth) + html_box_horizontal_mbp_sum (box);
			cell_width -= (span - 1) * HTML_BOX_GET_STYLE (self)->inherited->border_spacing_horiz;
			width += cell_width;
			
			while (span2--)
				table->col_info[i + span2].width = cell_width / span;

			num_fixed++;
		}
	}
	*boxwidth -= HTML_BOX_GET_STYLE (self)->inherited->border_spacing_horiz * (table->cols + 1);
	/* If we have more space then divide it equally to all the columns */
	diff = *boxwidth - width;
	if (diff > 0) {
		for (i = 0; i < table->cols; i++) {
			if (table->cells[i] && HTML_BOX_GET_STYLE (table->cells[i])->box->width.type == HTML_LENGTH_AUTO)
				table->col_info[i].width += diff / (table->cols - num_fixed);
		}
	}
	else
		*boxwidth = width;

	*boxwidth += HTML_BOX_GET_STYLE (self)->inherited->border_spacing_horiz * (table->cols + 1);
}

/**
 * layout_auto:
 * @self: a table box
 * @relayout: a layout context
 * @boxwidth: the availible width
 * 
 * This function tries to calculate the width of the different type of columns in
 * the table.
 **/
static void
layout_auto (HtmlBoxTable *table, HtmlRelayout *relayout, gint *boxwidth)
{
	HtmlBox *self = HTML_BOX (table);
	gint fixed_min = 0, fixed_max = 0;
	gint percent_min = 0, percent_max = 0;
	gint variable_min = 0, variable_max = 0;
	gint left, left_fixed_none;
	gint percent_percent, space_percent2, percent_other;
	gint space_other, space_percent;
	gint i;

	/* Calculate max and min width of the different columnt types */
	for (i=0; i< table->cols; i++) {
		switch (table->col_info[i].style_width.type) {
		case HTML_LENGTH_AUTO:
			variable_min += table->col_info[i].min;
			variable_max += table->col_info[i].max;
			break;
		case HTML_LENGTH_FIXED:
			fixed_min += table->col_info[i].min;
			fixed_max += table->col_info[i].style_width.value;
			break;
		case HTML_LENGTH_PERCENT:
			percent_min += table->col_info[i].min;
			percent_max += table->col_info[i].max;
			break;
		}
		table->col_info[i].width = 0;
	}

	*boxwidth -= HTML_BOX_GET_STYLE (self)->inherited->border_spacing_horiz * (table->cols + 1);

	percent_percent = html_box_table_get_total_percent (table);
	percent_other   = 100 - percent_percent;

	/* Min width special case */
	if (relayout->get_min_width && HTML_BOX_GET_STYLE (self)->box->width.type != HTML_LENGTH_FIXED) {
		space_other   = variable_min + fixed_min;
		space_percent = percent_min;
		*boxwidth = space_percent + space_other;
	}
	/* Max width special case */
	else if (relayout->get_max_width && HTML_BOX_GET_STYLE (self)->box->width.type != HTML_LENGTH_FIXED) {
		gint total = MAX(fixed_min + variable_min + percent_min, 
				 percent_percent ? (percent_max * 100 / percent_percent) : fixed_max + variable_max);
		
		space_percent = MAX (percent_min, total * percent_percent / 100);
		space_other   = MAX (variable_min + fixed_min, total * percent_other / 100);
		
		*boxwidth = space_percent + space_other;
	} else {
		/* If the table has width = NONE, then it should only take as
		 * much space as it needs */

		if (HTML_BOX_GET_STYLE (self)->box->width.type == HTML_LENGTH_AUTO) {
			if (percent_percent == 0) {
				if (fixed_max + variable_max < *boxwidth)
					*boxwidth = fixed_max + variable_max;
			} else {
				gint total, width = percent_max * 100 / percent_percent;
				total = percent_max + MAX (fixed_min + variable_min, width * percent_other / 100);
				
				if (total < *boxwidth)
					*boxwidth = total;
			}
		}

		space_percent = MAX (percent_min, *boxwidth * percent_percent / 100);
		space_other = MAX (fixed_min + variable_min, *boxwidth - space_percent);

		if (space_other + space_percent > *boxwidth) {
			
			gint extra = space_other + space_percent - *boxwidth;

			if (space_percent > percent_min)
				space_percent = MAX (percent_min, space_percent - extra);
			else
				space_other = MAX (fixed_min + variable_min, space_other - extra);

			*boxwidth = space_other + space_percent;
		} 
	}

	left = html_box_table_take_space (table, HTML_LENGTH_FIXED, space_other, variable_min);
	left = html_box_table_take_space (table, HTML_LENGTH_AUTO, left, 0);

	left_fixed_none = left;

	/* save this value for later use */
	space_percent2 = space_percent;

	if (space_percent) {
		left = html_box_table_take_space_percent (table, space_percent, 0, *boxwidth);
		space_percent -= left;
		left += left_fixed_none;
	} 

	if (left > 0 && relayout->get_max_width == FALSE) {
		gint space;

		space_other = left;

		if (space_percent) {
			if (fixed_max + variable_max == 0)
				space = left;
			else
				/* Give some space to the percent columns */
				space = ((space_other + space_percent2 + left) * percent_percent / 100) - space_percent;
			if (space > 0)
				space_other -= space;

			while (space > 0) {
				gint tmp = space;
				gint total = 0;
				
				for (i=0; i< table->cols; i++) 
					if (table->col_info[i].style_width.type == HTML_LENGTH_PERCENT)
						total += table->col_info[i].width;
				
				if (total == 0)
					total = 1;
				
				for (i=0; i< table->cols; i++) {
					if (table->col_info[i].style_width.type == HTML_LENGTH_PERCENT) {
						gint to_add = 100 * table->col_info[i].width / total * tmp / 100; 
						
						if (left && to_add == 0)
							to_add = 1;
						if (to_add > left)
							to_add = space;
						
						table->col_info[i].width += to_add;
						space -= to_add;
						if (space == 0)
							break;
					}
				}
			}
		}
		while (space_other > 0) {
			gint tmp = space_other;
			gint total = 0;
			gboolean only_floats = TRUE;
			HtmlLengthType type = HTML_LENGTH_AUTO;
			
			/*
			 * Ok, we have some "extra" space that we have to distribute between
			 * some columns. The "variable" columns (auto) will be used if 
			 * we have any, else we'll take the fixed width ones / jb
			 */

			for (i=0; i< table->cols; i++) 
				if (table->col_info[i].style_width.type == HTML_LENGTH_AUTO)
					total += table->col_info[i].width;
			
			if (total == 0) {
				type = HTML_LENGTH_FIXED;

				for (i=0; i< table->cols; i++) 
					if (table->col_info[i].style_width.type == HTML_LENGTH_FIXED)
						total += table->col_info[i].width;
			}
			if (total == 0)
				total = 1;
			
			for (i=0; i< table->cols; i++) {
				if (table->col_info[i].style_width.type == type) {
					gint to_add = 100 * table->col_info[i].width / total * tmp / 100; 
					
					only_floats = FALSE;

					if (space_other && to_add == 0)
						to_add = 1;
					if (to_add > space_other)
						to_add = space_other;
					
					table->col_info[i].width += to_add;
					space_other -= to_add;
					if (space_other == 0)
						break;
				}
			}
			if (only_floats)
				break;
		}
	}
	*boxwidth +=  HTML_BOX_GET_STYLE (self)->inherited->border_spacing_horiz * (table->cols + 1);
}

static void
calculate_row_height (HtmlBoxTable *table, HtmlRelayout *relayout)
{
	gint row, i;

	/* Calculate row_height[] */
	for (row=0; row < table->rows; row++) {
		gint row_height = 0;

		for (i=0; i< table->cols; i++) {
			gint cell_index = row * table->cols + i;
			HtmlBox *box = table->cells[cell_index];
			if (box) {
				gint width = 0, span = html_box_table_cell_get_colspan (HTML_BOX_TABLE_CELL (box));

				if (span + i > table->cols)
					span = table->cols - i;

				width = (span - 1) * HTML_BOX_GET_STYLE (HTML_BOX(table))->inherited->border_spacing_horiz;
				while (span--) {
					width += table->col_info[i + span].width;
				}
				html_box_table_cell_relayout_width (HTML_BOX_TABLE_CELL (box), relayout, width);
				
				if (html_box_table_cell_get_rowspan (HTML_BOX_TABLE_CELL (box)) == 1 &&
				    row_height < box->height)
					row_height = box->height;
			}
		}
		table->row_height[row] = row_height;
	}

	/* Calculate row_height[] on rowspan > 1*/
	for (row=0; row < table->rows; row++) {

		for (i=0; i< table->cols; i++) {
			gint span, cell_index = row * table->cols + i;
			HtmlBox *box = table->cells[cell_index];

			if (box && (span = html_box_table_cell_get_rowspan (HTML_BOX_TABLE_CELL (box))) > 1) {
				gint height = 0, span2 = span;
				
				if (span + row > table->rows)
					span2 = span = table->rows - row;

				height = (span - 1) * HTML_BOX_GET_STYLE (HTML_BOX(table))->inherited->border_spacing_vert;
				while (span--) {
					height += table->row_height[row + span];
				}
				if (height < table->cells[cell_index]->height) {
					gint j, rest = table->cells[cell_index]->height - height;

					for (j=row; j < (span2 + row); j++) {
						table->row_height[j] += rest / span2;
					}
				}
			}
		}
	}
}

static void
align_cells_ltr (HtmlBoxTable *table, HtmlRelayout *relayout, gint *boxwidth, gint *boxheight)
{
	HtmlStyle *style = HTML_BOX_GET_STYLE (HTML_BOX (table));
	gint col, row, cur_x = 0;
	gint cur_y = style->inherited->border_spacing_vert;

	for (row = 0; row < table->rows; row++) {
		cur_x = style->inherited->border_spacing_horiz;
		for (col = 0; col < table->cols; col++) {
			if (table->cells[row * table->cols + col]) {
				table->cells[row * table->cols + col]->x = cur_x;
				table->cells[row * table->cols + col]->y = 0;
			}
			cur_x += table->col_info[col].width;
			cur_x += style->inherited->border_spacing_horiz;

			if (cur_x > *boxwidth)
				*boxwidth = cur_x;
		}
		cur_y += table->row_height[row];
		cur_y += style->inherited->border_spacing_vert;
	}

	if (cur_x > *boxwidth || style->box->width.type == HTML_LENGTH_AUTO || relayout->get_max_width == TRUE)
		*boxwidth = cur_x;
	if (cur_y > *boxheight)
		*boxheight = cur_y;
}

static void
align_cells_rtl (HtmlBoxTable *table, HtmlRelayout *relayout, gint *boxwidth, gint *boxheight)
{
	HtmlStyle *style = HTML_BOX_GET_STYLE (HTML_BOX (table));
	gint col, row, cur_x = 0;
	gint cur_y = style->inherited->border_spacing_vert;

	for (row = 0; row < table->rows; row++) {
		cur_x = style->inherited->border_spacing_horiz;
		for (col = table->cols - 1; col >= 0; col--) {
			if (table->cells[row * table->cols + col]) {
				table->cells[row * table->cols + col]->x = cur_x;
				table->cells[row * table->cols + col]->y = 0;
			}
			cur_x += table->col_info[col].width;
			cur_x += style->inherited->border_spacing_horiz;

			if (cur_x > *boxwidth)
				*boxwidth = cur_x;
		}
		cur_y += table->row_height[row];
		cur_y += style->inherited->border_spacing_vert;
	}

	if (cur_x > *boxwidth || style->box->width.type == HTML_LENGTH_AUTO || relayout->get_max_width == TRUE)
		*boxwidth = cur_x;
	if (cur_y > *boxheight)
		*boxheight = cur_y;
}

static void
set_cell_heights (HtmlBoxTable *table)
{
	gint row, i;

	for (row=0; row < table->rows; row++) {
		for (i=0; i< table->cols; i++) {
			HtmlBox *box = table->cells[row * table->cols + i];
			if (box) {
				gint height = 0, span = html_box_table_cell_get_rowspan (HTML_BOX_TABLE_CELL (box));

				if (span + row > table->rows)
					span = table->rows - row;
				
				height = (span - 1) * HTML_BOX_GET_STYLE (HTML_BOX (table))->inherited->border_spacing_vert;
				while (span-- && span + row < table->rows)
					height += table->row_height[row + span];
				html_box_table_cell_do_valign (HTML_BOX_TABLE_CELL (box), height);
			}
		}
	}
}

static void
update_row_geometry (HtmlBoxTable *table, GSList *list, gint boxwidth, gint boxheight, gint *row, gint cur_x, gint *cur_y)
{
	HtmlStyle *style = HTML_BOX_GET_STYLE (HTML_BOX (table));

	while (list) {
		HtmlBox *box = (HtmlBox *)(list->data);

		if (HTML_IS_BOX_TABLE_ROW (box)) {
			box->width = boxwidth;
			box->height = table->row_height[*row];
			box->x = cur_x;
			box->y = *cur_y;
			(*cur_y) += style->inherited->border_spacing_vert;
			(*cur_y) += table->row_height[*row];
			(*row)++;
		}
		list = list->next;
	}
}

static void
update_min_max (HtmlBoxTable *table, HtmlRelayout *relayout, gboolean force)
{
	gint i, array_size = table->cols * table->rows;
	gboolean changed = FALSE;

	/* Get min width */
	for (i=0; i< array_size; i++) {
		HtmlBox *box = table->cells[i];
		if (box && (force || box->is_relayouted == FALSE)) {

			table->min_width[i] = html_box_table_cell_get_min_width (HTML_BOX_TABLE_CELL (box), relayout);
			table->max_width[i] = html_box_table_cell_get_max_width (HTML_BOX_TABLE_CELL (box), relayout);
			changed = TRUE;
		}
	}
	if (changed)
		calculate_col_min_max (table);
}

static void
relayout_caption (HtmlBoxTable *table, HtmlRelayout *relayout, gint boxwidth)
{
	HtmlBox *self = HTML_BOX (table);

	if (table->caption == NULL)
		return;

	html_box_table_caption_relayout_width (table->caption, relayout, html_box_get_containing_block_width (self) - html_box_horizontal_mbp_sum (self));
}

static void
place_caption (HtmlBoxTable *table, HtmlRelayout *relayout, gint boxheight)
{
	if (table->caption == NULL)
		return;

	HTML_BOX (table->caption)->x = 0;
	switch (HTML_BOX_GET_STYLE (HTML_BOX (table->caption))->inherited->caption_side) {
	case HTML_CAPTION_SIDE_TOP:
		HTML_BOX (table->caption)->y = 0;
		break;
	case HTML_CAPTION_SIDE_BOTTOM:
		HTML_BOX (table->caption)->y = boxheight - HTML_BOX (table->caption)->height + html_box_top_mbp_sum (HTML_BOX (table), -1);
		break;
	default:
		g_print ("caption-side: %d not supported\n", HTML_BOX_GET_STYLE (HTML_BOX (table->caption))->inherited->caption_side);
	}
}

static void
init_boxwidth_boxheight (HtmlBox *self, HtmlRelayout *relayout, gint *boxwidth, gint *boxheight)
{
	HtmlStyle *style = HTML_BOX_GET_STYLE (self);

	if (style->box->width.type == HTML_LENGTH_AUTO)
		*boxwidth = html_box_get_containing_block_width (self) - 
			html_box_horizontal_mbp_sum (self) - self->x;
	else
		*boxwidth = html_length_get_value (&style->box->width, html_box_get_containing_block_width (self) - self->x);

	*boxheight = html_length_get_value (&style->box->height, html_box_get_containing_block_height (self) - html_box_vertical_mbp_sum (self));
	/* These can't have negative values */ 
	*boxwidth  = MAX (0, *boxwidth);
	*boxheight = MAX (0, *boxheight);
	self->width  = *boxwidth  + html_box_horizontal_mbp_sum (self);
	self->height = *boxheight + html_box_vertical_mbp_sum (self);
}

static void
html_box_table_h_align (HtmlBox *self, gint boxwidth)
{
	gint offset = 0;
	HtmlStyle *style = HTML_BOX_GET_STYLE (self);

#define GET_WIDTH gint width = html_box_get_containing_block_width (self); width = MAX (width, boxwidth);

	/* Center */
	if (style->surround->margin.left.type == HTML_LENGTH_AUTO &&
		 style->surround->margin.right.type == HTML_LENGTH_AUTO) {
		
		GET_WIDTH;

		offset = (width - boxwidth - html_box_horizontal_mbp_sum (self)) / 2;
	}
	/* Left */
	else if (style->surround->margin.left.type != HTML_LENGTH_AUTO &&
		 style->surround->margin.right.type == HTML_LENGTH_AUTO) {

		offset = html_box_left_mbp_sum (self, -1);
	}
	/* Right */
	else if (style->surround->margin.left.type == HTML_LENGTH_AUTO &&
		 style->surround->margin.right.type != HTML_LENGTH_AUTO) {

		GET_WIDTH;

		offset = width - boxwidth - html_box_horizontal_mbp_sum (self);
	}
	/* Default */
	else {

		if (style->inherited->direction == HTML_DIRECTION_RTL) {

			GET_WIDTH;
				
			offset = width - boxwidth - html_box_horizontal_mbp_sum (self);
		}
	}
	if (offset > 0)
		self->x += offset;

#undef GET_WIDTH
}

static void
html_box_table_relayout (HtmlBox *self, HtmlRelayout *relayout)
{
	HtmlBoxTable *table = HTML_BOX_TABLE (self);
	HtmlTableLayoutType table_layout = HTML_BOX_GET_STYLE (self)->table_layout;
	gint boxwidth = 0, boxheight = 0, cur_x = 0, cur_y, row;

	init_boxwidth_boxheight (self, relayout, &boxwidth, &boxheight);

#if 1
	/* We can't use fixed layout on tables with auto width */
	if (HTML_BOX_GET_STYLE (self)->box->width.type == HTML_LENGTH_AUTO)
		table_layout = HTML_TABLE_LAYOUT_AUTO;

	if (table->up_to_date == FALSE) {

		update_info (table, relayout);

		table->up_to_date = TRUE;

		if (table_layout == HTML_TABLE_LAYOUT_AUTO)
			update_min_max (table, relayout, TRUE);
	}
	else {
		if (table_layout == HTML_TABLE_LAYOUT_AUTO)
			update_min_max (table, relayout, FALSE);
	}
#else
	update_info (table, relayout);
	update_min_max (table, relayout, TRUE);
	init_boxwidth_boxheight (self, relayout, &boxwidth, &boxheight);
#endif
	if (table->rows == 0 || table->cols == 0) {
		self->width  = boxwidth  + html_box_horizontal_mbp_sum (self);
		self->height = boxheight + html_box_top_mbp_sum (self, -1);
		return;
	}

	/* Use the layout method specified in the "table-layout:" property */
	if (table_layout == HTML_TABLE_LAYOUT_AUTO)
		layout_auto (table, relayout, &boxwidth);
	else
		layout_fixed (table, relayout, &boxwidth);

	calculate_row_height (table, relayout);
	relayout_caption (table, relayout, boxwidth);

	switch (HTML_BOX_GET_STYLE (self)->inherited->direction) {
	case HTML_DIRECTION_LTR:
		align_cells_ltr (table, relayout, &boxwidth, &boxheight);
		break;
	case HTML_DIRECTION_RTL:
		align_cells_rtl (table, relayout, &boxwidth, &boxheight);
		break;
	};
	place_caption (table, relayout, boxheight);
	set_cell_heights (table);
	row = 0;

	cur_y = HTML_BOX_GET_STYLE (self)->inherited->border_spacing_vert;

	update_row_geometry (table, table->header_list, boxwidth, boxheight, &row, cur_x, &cur_y);
	update_row_geometry (table, table->body_list,   boxwidth, boxheight, &row, cur_x, &cur_y);
	update_row_geometry (table, table->footer_list, boxwidth, boxheight, &row, cur_x, &cur_y);
#if 0	
	for (i=0; i< table->cols; i++)
		g_print ("%d: min=%d, max=%d width = %d\n", i, table->col_info[i].min, table->col_info[i].max, table->col_info[i].width);
	for (i=0; i< table->rows; i++)
		g_print ("row %d:  height = %d\n", i, table->row_height[i]);
#endif
	self->width  = boxwidth  + html_box_horizontal_mbp_sum (self);
	self->height = boxheight + html_box_vertical_mbp_sum (self);

	html_box_table_h_align (self, boxwidth);
}

static void
paint_rows (HtmlBox *self, HtmlPainter *painter, GdkRectangle *area, gint tx, gint ty, GSList *list)
{
	while (list) {
		HtmlBox *box = (HtmlBox *)(list->data);

		if (!HTML_IS_BOX_TABLE (box->parent) && !HTML_IS_BOX_FORM (box->parent))
			html_box_paint (box->parent, painter, area, self->x + tx, self->y + ty);

		html_box_paint (box, painter, area, self->x + tx, self->y + ty);

		list = list->next;
	}
}

static void
html_box_table_paint (HtmlBox *self, HtmlPainter *painter, GdkRectangle *area, gint tx, gint ty)
{
	HtmlBoxTable *table = HTML_BOX_TABLE (self);
	gint row;

	tx += html_box_left_mbp_sum (self, -1);
	ty += html_box_top_mbp_sum (self, -1);

	if (table->caption)
		html_box_paint (HTML_BOX (table->caption), painter, area, tx + self->x, ty + self->y);

	paint_rows (self, painter, area, tx, ty, table->header_list);
	paint_rows (self, painter, area, tx, ty, table->body_list);
	paint_rows (self, painter, area, tx, ty, table->footer_list);
	row = 0;
}

void
html_box_table_add_thead (HtmlBoxTable *table, HtmlBoxTableRow *row)
{
	table->header_list = g_slist_append (table->header_list, row);
	table->up_to_date = FALSE;
}

static void
html_box_table_remove_thead (HtmlBoxTable *table, HtmlBoxTableRow *row)
{
	table->header_list = g_slist_remove (table->header_list, row);
	table->up_to_date = FALSE;
}

void
html_box_table_add_tbody (HtmlBoxTable *table, HtmlBoxTableRow *row)
{
	table->body_list = g_slist_append (table->body_list, row);
	table->up_to_date = FALSE;
}

static void
html_box_table_remove_tbody (HtmlBoxTable *table, HtmlBoxTableRow *row)
{
	table->body_list = g_slist_remove (table->body_list, row);
	table->up_to_date = FALSE;
}

void
html_box_table_add_tfoot (HtmlBoxTable *table, HtmlBoxTableRow *row)
{
	table->footer_list = g_slist_append (table->footer_list, row);
	table->up_to_date = FALSE;
}

static void
html_box_table_remove_tfoot (HtmlBoxTable *table, HtmlBoxTableRow *row)
{
	table->footer_list = g_slist_remove (table->footer_list, row);
	table->up_to_date = FALSE;
}

void
html_box_table_remove_caption (HtmlBoxTable *table, HtmlBoxTableCaption *caption)
{
	if (table->caption == caption)
		table->caption = NULL;
}

void
html_box_table_cell_added (HtmlBoxTable *table)
{
	table->up_to_date = FALSE;
}

void
html_box_table_remove_row (HtmlBoxTable *table, HtmlBoxTableRow *row)
{
	html_box_table_remove_thead (table, row);
	html_box_table_remove_tbody (table, row);
	html_box_table_remove_tfoot (table, row);
}

static void
html_box_table_append_child (HtmlBox *self, HtmlBox *child)
{
	switch (HTML_BOX_GET_STYLE (child)->display) {
	case HTML_DISPLAY_TABLE_CELL:
		/* If we find a table cell without a row as a parent
		 * we have to create a new row
		 */
		{
		HtmlBox *row = NULL;
		GSList *last;
		if ((last = g_slist_last (HTML_BOX_TABLE (self)->body_list)))
			row = last->data;

		if (row == NULL) {
			row = html_box_table_row_new ();
			html_box_set_style (row, html_style_new (HTML_BOX_GET_STYLE (child)));
			HTML_BOX_GET_STYLE (row)->display = HTML_DISPLAY_TABLE_ROW;
			html_box_append_child (self, row);
		}
		html_box_append_child (row, child);
		}
		break;
	case HTML_DISPLAY_TABLE_CAPTION:

		HTML_BOX_TABLE (self)->caption = HTML_BOX_TABLE_CAPTION (child);
		parent_class->append_child (self, child);
		break;
	case HTML_DISPLAY_TABLE_ROW:

		html_box_table_add_tbody (HTML_BOX_TABLE (self), HTML_BOX_TABLE_ROW (child));
		parent_class->append_child (self, child);
		break;
	default:
		parent_class->append_child (self, child);
		break;
	}
}

static void
html_box_table_finalize (GObject *object)
{
	HtmlBoxTable *table = HTML_BOX_TABLE (object);
	g_slist_free (table->body_list);
	g_slist_free (table->header_list);
	g_slist_free (table->footer_list);

	g_free (table->cells);
	g_free (table->col_info);
	g_free (table->min_width);
	g_free (table->max_width);
	g_free (table->row_height);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
html_box_table_handle_html_properties (HtmlBox *self, xmlNode *n) 
{
	HtmlBoxTable *table = HTML_BOX_TABLE (self);
	gchar *str;
	
	if ((str = xmlGetProp (n, "cellpadding"))) {
		table->cell_padding = atoi (str);
		xmlFree (str);
	}
	if ((str = xmlGetProp (n, "border"))) {
		gint border_width;
		if (*str == 0)
			border_width = 1;
		else
			border_width = atoi (str);

		table->cell_border = (border_width > 0);
		xmlFree (str);
	}
}

static void
html_box_table_class_init (HtmlBoxClass *klass)
{
	GObjectClass *object_class = (GObjectClass *)klass;

	object_class->finalize = html_box_table_finalize;
	
	klass->relayout       = html_box_table_relayout;
	klass->paint          = html_box_table_paint;
	klass->append_child   = html_box_table_append_child;
#if 0
	klass->left_mbp_sum   = html_box_table_left_mbp_sum;
	klass->right_mbp_sum  = html_box_table_right_mbp_sum;
	klass->top_mbp_sum    = html_box_table_top_mbp_sum;
	klass->bottom_mbp_sum = html_box_table_bottom_mbp_sum;
#endif
	klass->handle_html_properties = html_box_table_handle_html_properties;

	parent_class = g_type_class_peek_parent (klass);
}

static void
html_box_table_init (HtmlBoxTable *table)
{
	table->cell_border = FALSE;
	table->cell_padding = 0;
}

GType
html_box_table_get_type (void)
{
       static GType html_type = 0;

       if (!html_type) {
               static GTypeInfo type_info = {
                       sizeof (HtmlBoxTableClass),
		       NULL,
		       NULL,
		       (GClassInitFunc) html_box_table_class_init,
		       NULL,
		       NULL,
		       sizeof (HtmlBoxTable),
		       16,
                       (GInstanceInitFunc) html_box_table_init
               };

               html_type = g_type_register_static (HTML_TYPE_BOX, "HtmlBoxTable", &type_info, 0);
       }
       
       return html_type;
}

HtmlBox *
html_box_table_new (void)
{
	return g_object_new (HTML_TYPE_BOX_TABLE, NULL);
}



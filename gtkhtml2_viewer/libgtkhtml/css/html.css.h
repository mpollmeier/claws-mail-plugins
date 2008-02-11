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

const gchar html_css [] =
"*     { margin: 0px }"
"      ADDRESS,"
"      BLOCKQUOTE, "
"      BODY, DD, DIV, "
"      DL, DT, "
"      FIELDSET, FORM,"
"      FRAME, FRAMESET,"
"      H1, H2, H3, H4, "
"      H5, H6, IFRAME, "
"      NOFRAMES, HTML, " /* FIXME, remove HTML */
"      OBJECT, OL, P, "
"      UL, APPLET, "
"      CENTER, DIR, "
"      HR, MENU, PRE   { display: block }"
"BR            { display: block }"
"BR:first-child2, BR + BR       { height: 1em }"
"NOBR            { white-space: nowrap }"
"HTML          { line-height: 1.33; }"
"head,script,style, title  { display: none }"
"BODY            { margin: 8px; min-height: 100%}"
"TEXTAREA        { font-family: monospace }"
"TABLE           { display: table ; text-align: left }"
"TR              { display: table-row }"
"THEAD           { display: table-header-group }"
"TBODY           { display: table-row-group }"
"TFOOT           { display: table-footer-group }"
"COL             { display: table-column }"
"COLGROUP        { display: table-column-group }"
"TD, TH          { display: table-cell }"
"CAPTION         { display: table-caption }"
"A[href]         { color: linkblue; text-decoration: underline; cursor: pointer }"
"img:focus,A[href]:focus   { outline: 2px dotted invert }"
"OPTION          { display: none}"
"B               { font-weight: bold }"
"TH              { font-weight: bold }"
"I               { font-style: italic }"
"CENTER          { text-align: center }"
"H1              { font-size: 2em; margin-top: 0.67em; margin-bottom: 0.67em}"
"H2              { font-size: 1.5em; margin-top: 0.83em; margin-bottom: 0.83em}"
"H3              { font-size: 1.17em; margin-top: 1em; margin-bottom: 1em}"
"H5              { font-size: 0.83em; line-height: 1.17em; margin-top: 1.67em; margin-bottom: 1.67em}"
"H4, P,"
"BLOCKQUOTE, UL,"
"FIELDSET, FORM,"
"OL, DL, DIR,"
"MENU            { margin: 1.33em 0 }"
"TD > H1:first-child, TD > H2:first-child,"
"TD > H3:first-child, TD > H4:first-child,"
"TD > H5:first-child, TD > P:first-child,"
"TD > FIELDSET:first-child, TD > FORM:first-child,"
"TD > OL:first-child, TD > D:first-child,"
"TD > MENU:first-child, TD > DIR:first-child,"
"TD > BLOCKQUOTE:first-child, TD > UL:first-child,"
"LI > P:first-child"
"{ margin-top: 0 }"
"TD > H1:last-child, TD > H2:last-child,"
"TD > H3:last-child, TD > H4:last-child,"
"TD > H5:last-child, TD > P:last-child,"
"TD > FIELDSET:last-child, TD > FORM:last-child,"
"TD > OL:last-child, TD > D:last-child,"
"TD > MENU:last-child, TD > DIR:last-child,"
"TD > BLOCKQUOTE:last-child, TD > UL:last-child,"
"{ margin-bottom: 0 }"
"TABLE > FORM, TR > FORM { margin: 0 }"
"H6              { font-size: 0.67em; margin-top: 2.33em; margin-bottom: 2.33em }"
"STRONG          { font-weight: bold }"
"I, CITE, EM,"
"VAR, ADDRESS    { font-style: italic }"
"PRE             { white-space: pre; font-family: monospace }"
"BIG             { font-size: 1.17em }"
"SMALL, SUB, SUP { font-size: 0.83em }"
"HR              { display: block; margin-top: 0.5em; margin-bottom:0.5em; border: outset 1px }" /* Fix me */
"LI              { display: list-item }"
"OL              { list-style-type: decimal }"
"UL              { list-style-type: disc }"
"SCRIPT          { display: none }"
"BLINK           { text-decoration: blink }"
"ABBR, ACRONYM   { font-variant: small-caps; letter-spacing: 0.1em }"
"BDO[DIR=\"ltr\"]  { direction: ltr; unicode-bidi: bidi-override }"
"BDO[DIR=\"rtl\"]  { direction: rtl; unicode-bidi: bidi-override }"
"*[DIR=\"ltr\"]    { direction: ltr; unicode-bidi: embed }"
"*[DIR=\"rtl\"]    { direction: rtl; unicode-bidi: embed }"
"table { border-spacing: 2px }"
"table[align=\"center\"] { margin-left: auto; margin-right: auto }"
"td { vertical-align: inherit }"
"td[valign=\"top\"],tr[valign=\"top\"] { vertical-align: top }"
"td[valign=\"bottom\"],tr[valign=\"bottom\"] { vertical-align: bottom }"
"td[valign=\"middle\"],tr[valign=\"middle\"] { vertical-align: middle }"
"td[valign=\"baseline\"],tr[valign=\"baseline\"] { vertical-align: baseline }"
"td[nowrap],th[nowrap] { white-space: nowrap }"
"*[clear=\"left\"] { clear: left }"
"*[clear=\"right\"] { clear: right }"
"*[clear=\"both\"] { clear: both }"
"*[clear=\"all\"] { clear: both }"
"img[align=\"left\"] { float: left }"
"img[align=\"right\"] { float: right }";


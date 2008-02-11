SUBDIRS = html

libgtkhtmlincludedir = $(includedir)/gtkhtml-2.0/libgtkhtml/layout

INCLUDES = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libgtkhtml \
	$(GTKHTML2_CFLAGS) \
	-DG_LOG_DOMAIN=\"HtmlLayout\"

noinst_LTLIBRARIES = libhtmllayout.la

libhtmllayout_la_SOURCES = 	\
	htmllinebox.c		\
	htmlboxfactory.c	\
	htmlboxfactory.h	\
	htmlbox.c		\
	htmlboxinline.c		\
	htmlboxinline.h		\
	htmlboxroot.c		\
	htmlboxtable.c		\
	htmlboxtablecell.c	\
	htmlboxtablerow.c	\
	htmlboxtext.c		\
	htmlboxblock.c		\
	htmlboxtablecaption.c	\
	htmlboxtablerowgroup.c	\
	htmlboxlistitem.c	\
	htmlrelayout.c		\
	htmlstyle.c		\
	htmlstylebackground.c	\
	htmlstyleborder.c	\
	htmlstyleoutline.c	\
	htmlstylebox.c		\
	htmlstyleinherited.c	\
	htmlstylesurround.c

libgtkhtmlinclude_HEADERS =	\
	htmlbox.h		\
	htmlboxtext.h		\
	htmllinebox.h		\
	htmlstyle.h		\
	htmlboxroot.h		\
	htmlboxlistitem.h	\
	htmlboxblock.h		\
	htmlboxtable.h		\
	htmlboxtablecell.h	\
	htmlboxtablecaption.h	\
	htmlboxtablerow.h	\
	htmlboxtablerowgroup.h	\
	htmlrelayout.h

libhtmllayout_la_LIBADD = $(top_builddir)/libgtkhtml/layout/html/libhtmllayouthtml.la

MAINTAINERCLEANFILES =	\
	Makefile.in

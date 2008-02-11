libgtkhtmlincludedir = $(includedir)/gtkhtml-2.0/libgtkhtml/view

INCLUDES = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libgtkhtml \
	$(GTKHTML2_CFLAGS) \
	-DG_LOG_DOMAIN=\"HtmlView\"

noinst_LTLIBRARIES = libhtmlview.la

libhtmlview_la_SOURCES = \
	htmlevent.c \
	htmlview.c \
	htmlselection.c

libgtkhtmlinclude_HEADERS = \
	htmlevent.h \
	htmlview.h \
	htmlselection.h

MAINTAINERCLEANFILES =	\
	Makefile.in

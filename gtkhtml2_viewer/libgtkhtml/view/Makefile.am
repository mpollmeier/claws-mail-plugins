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

noinst_HEADERS = \
	htmlevent.h \
	htmlview.h \
	htmlselection.h

MAINTAINERCLEANFILES =	\
	Makefile.in

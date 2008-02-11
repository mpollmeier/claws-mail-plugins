libgtkhtmlincludedir = $(includedir)/gtkhtml-2.0/libgtkhtml/dom/views

INCLUDES = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	$(GTKHTML2_CFLAGS) \
	-DG_LOG_DOMAIN=\"DomViews\"

noinst_LTLIBRARIES = libdomviews.la

libdomviews_la_SOURCES = \
	dom-abstractview.c \
	dom-documentview.c 

libgtkhtmlinclude_HEADERS = \
	dom-abstractview.h \
	dom-documentview.h

MAINTAINERCLEANFILES =	\
	Makefile.in

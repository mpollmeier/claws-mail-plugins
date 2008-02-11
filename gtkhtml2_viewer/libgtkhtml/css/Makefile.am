libgtkhtmlincludedir = $(includedir)/gtkhtml-2.0/libgtkhtml/css

INCLUDES = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libgtkhtml \
	$(GTKHTML2_CFLAGS) \
	-DG_LOG_DOMAIN=\"HtmlCss\"

noinst_LTLIBRARIES = libhtmlcss.la

libhtmlcss_la_SOURCES = \
	cssdebug.h \
	cssmatcher.c \
	cssparser.c \
	cssstylesheet.c \
	cssvalue.c \
	html.css.h


libgtkhtmlinclude_HEADERS = \
	cssmatcher.h \
	cssparser.h \
	cssstylesheet.h \
	cssvalue.h 

MAINTAINERCLEANFILES =	\
	Makefile.in

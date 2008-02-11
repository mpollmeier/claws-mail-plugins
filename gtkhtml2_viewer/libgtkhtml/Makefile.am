SUBDIRS = util css document dom layout graphics view a11y

libgtkhtmlincludedir = $(includedir)/gtkhtml-2.0/libgtkhtml

INCLUDES = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libgtkhtml \
	$(GTKHTML2_CFLAGS) \
	-DG_LOG_DOMAIN=\"GtkHtml\"

DGNOMELOCALEDIR=\"$(datadir)/locale\" \
	-I../intl -I$(top_srcdir)/intl

noinst_LTLIBRARIES = libgtkhtml-2.la

libgtkhtml_2_la_SOURCES = \
	gtkhtmlcontext.c 


libgtkhtmlinclude_HEADERS =  \
	gtkhtmlcontext.h \
	gtkhtml.h

if ENABLE_ACCESSIBILITY
A11Y_LT_LIB = $(top_builddir)/libgtkhtml/a11y/libhtmla11y.la
else
A11Y_LT_LIB =
endif

libgtkhtml_2_la_LIBADD = \
	$(top_builddir)/libgtkhtml/css/libhtmlcss.la \
	$(top_builddir)/libgtkhtml/document/libhtmldocument.la \
	$(top_builddir)/libgtkhtml/dom/libdom.la \
	$(top_builddir)/libgtkhtml/graphics/libhtmlgraphics.la \
	$(top_builddir)/libgtkhtml/layout/libhtmllayout.la \
	$(top_builddir)/libgtkhtml/util/libhtmlutil.la \
	$(top_builddir)/libgtkhtml/view/libhtmlview.la \
	$(A11Y_LT_LIB) $(GTKHTML2_LIBS) $(GAILUTIL_LIBS)

MAINTAINERCLEANFILES =	\
	Makefile.in

libgtkhtmlincludedir = $(includedir)/gtkhtml-2.0/libgtkhtml/graphics

INCLUDES = -I$(top_srcdir) \
	-I$(top_srcdir)/libgtkhtml \
	$(GTKHTML2_CFLAGS) \
	-DG_LOG_DOMAIN=\"HtmlGraphics\"

noinst_LTLIBRARIES = libhtmlgraphics.la

libhtmlgraphics_la_SOURCES = \
	htmlimage.c \
	htmlimagefactory.c \
	htmlfontspecification.c \
	htmlpainter.c \
	htmlgdkpainter.c \
	htmlstylepainter.c \
	htmlcolor.c 

libgtkhtmlinclude_HEADERS = \
	htmlcolor.h \
	htmlfontspecification.h \
	htmlgdkpainter.h \
	htmlimagefactory.h \
	htmlimage.h \
	htmlpainter.h \
	htmlstylepainter.h

MAINTAINERCLEANFILES =	\
	Makefile.in

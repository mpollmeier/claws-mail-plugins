INCLUDES = \
	-I$(top_srcdir) 		\
	-I$(top_srcdir)/src		\
	$(GTKHTML2_CFLAGS) 

DGNOMELOCALEDIR=\"$(datadir)/locale\" \
	-I../intl -I$(top_srcdir)/intl

noinst_LTLIBRARIES = libdomstylesheets.la

libdomstylesheets_la_SOURCES = 	\
	dom-stylesheet.c

libdomstylesheets_la_LIBADD = $(GTKHTML2_LIBS)

MAINTAINERCLEANFILES =	\
	Makefile.in

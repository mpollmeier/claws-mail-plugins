libgtkhtmlincludedir = $(includedir)/gtkhtml-2.0/libgtkhtml/dom

SUBDIRS = core html events views traversal

INCLUDES = \
	-I$(top_srcdir) \
	$(GTKHTML2_CFLAGS)

noinst_LTLIBRARIES = libdom.la

libdom_la_SOURCES = \
	dom.h

libgtkhtmlinclude_HEADERS = \
	dom-types.h

libdom_la_LIBADD = \
	$(top_builddir)/libgtkhtml/dom/core/libdomcore.la	\
	$(top_builddir)/libgtkhtml/dom/html/libdomhtml.la	\
	$(top_builddir)/libgtkhtml/dom/events/libdomevents.la  \
	$(top_builddir)/libgtkhtml/dom/views/libdomviews.la	\
	$(top_builddir)/libgtkhtml/dom/traversal/libdomtraversal.la

MAINTAINERCLEANFILES =	\
	Makefile.in

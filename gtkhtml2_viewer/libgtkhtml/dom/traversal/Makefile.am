libgtkhtmlincludedir = $(includedir)/gtkhtml-2.0/libgtkhtml/dom/traversal

INCLUDES = \
	-I$(top_srcdir) 		\
	-I$(top_srcdir)/src/dom		\
	-I$(top_srcdir)/src/dom/core	\
	$(GTKHTML2_CFLAGS) 

noinst_LTLIBRARIES = libdomtraversal.la

libdomtraversal_la_SOURCES = 	\
	dom-documenttraversal.c	\
	dom-documenttraversal.h	\
	dom-nodefilter.c	\
	dom-nodefilter.h	\
	dom-nodeiterator.c	\
	dom-nodeiterator.h	\
	dom-traversal-utils.c	\
	dom-traversal-utils.h

libgtkhtmlinclude_HEADERS = 	\
	dom-documenttraversal.h	\
	dom-nodefilter.h	\
	dom-nodeiterator.h	\
	dom-traversal-utils.h

libdomtraversal_la_LIBADD = $(GTKHTML2_LIBS) 

MAINTAINERCLEANFILES =	\
	Makefile.in

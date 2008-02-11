libgtkhtmlincludedir = $(includedir)/gtkhtml-2.0/libgtkhtml/dom/core

INCLUDES = \
	-I$(top_srcdir) 		\
	-I$(top_srcdir)/libgtkhtml	\
	$(GTKHTML2_CFLAGS) 

DGNOMELOCALEDIR=\"$(datadir)/locale\" \
	-I../intl -I$(top_srcdir)/intl

noinst_LTLIBRARIES = libdomcore.la

libdomcore_la_SOURCES = 	\
	dom-attr.c		\
	dom-characterdata.c	\
	dom-comment.c		\
	dom-core-utils.c	\
	dom-domimplementation.c	\
	dom-domimplementation.h \
	dom-document.c		\
	dom-documenttype.c	\
	dom-element.c		\
	dom-entity.c		\
	dom-entity.h		\
	dom-namednodemap.c	\
	dom-nodelist.c		\
	dom-node.c		\
	dom-text.c		\
	dom-text.h

libgtkhtmlinclude_HEADERS = 	\
	dom-attr.h		\
	dom-characterdata.h	\
	dom-comment.h		\
	dom-core-utils.h	\
	dom-document.h		\
	dom-documenttype.h	\
	dom-element.h		\
	dom-namednodemap.h	\
	dom-node.h		\
	dom-nodelist.h

libdomcore_la_LIBADD = $(GTKHTML2_LIBS)

MAINTAINERCLEANFILES =	\
	Makefile.in

libgtkhtmlincludedir = $(includedir)/gtkhtml-2.0/libgtkhtml/dom/html

INCLUDES = \
	-I$(top_srcdir) 		\
	-I$(top_srcdir)/libgtkhtml	\
	$(GTKHTML2_CFLAGS) 

noinst_LTLIBRARIES = libdomhtml.la

libdomhtml_la_SOURCES = 	\
	dom-htmlanchorelement.c	\
	dom-htmlelement.c	\
	dom-htmlformelement.c	\
	dom-htmlinputelement.c	\
	dom-htmlselectelement.c	\
	dom-htmloptionelement.c	\
	dom-htmlobjectelement.c	\
	dom-htmltextareaelement.c	\
	dom-htmlcollection.c

libgtkhtmlinclude_HEADERS = 	\
	dom-htmlanchorelement.h	\
	dom-htmlelement.h	\
	dom-htmlformelement.h	\
	dom-htmlinputelement.h	\
	dom-htmlselectelement.h	\
	dom-htmloptionelement.h	\
	dom-htmlobjectelement.h	\
	dom-htmltextareaelement.h	\
	dom-htmlcollection.h

MAINTAINERCLEANFILSE =	\
	Makefile.in

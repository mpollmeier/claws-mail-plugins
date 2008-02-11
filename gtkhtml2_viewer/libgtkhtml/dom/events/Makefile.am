libgtkhtmlincludedir = $(includedir)/gtkhtml-2.0/libgtkhtml/dom/events

INCLUDES = \
	-I$(top_srcdir) 		\
	-I$(top_srcdir)/libgtkhtml	\
	$(GTKHTML2_CFLAGS) 

DGNOMELOCALEDIR=\"$(datadir)/locale\" \
	-I../intl -I$(top_srcdir)/intl

noinst_LTLIBRARIES = libdomevents.la

libdomevents_la_SOURCES = 	\
	dom-documentevent.c	\
	dom-documentevent.h	\
	dom-event.c		\
	dom-eventlistener.c	\
	dom-eventtarget.c	\
	dom-event-utils.c	\
	dom-event-utils.h	\
	dom-mouseevent.c	\
	dom-mouseevent.h	\
	dom-mutationevent.c	\
	dom-mutationevent.h	\
	dom-styleevent.c	\
	dom-styleevent.h	\
	dom-uievent.c		\
	dom-uievent.h

libgtkhtmlinclude_HEADERS = 	\
	dom-event.h		\
	dom-eventlistener.h	\
	dom-eventtarget.h

libdomevents_la_LIBADD = $(GTKHTML2_LIBS)

MAINTAINERCLEANFILES =	\
	Makefile.in

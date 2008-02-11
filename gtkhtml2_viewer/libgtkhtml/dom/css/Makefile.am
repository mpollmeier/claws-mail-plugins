INCLUDES = \
	-I$(top_srcdir) 		\
	-I$(top_srcdir)/src		\
	$(GTKHTML2_CFLAGS) 

DGNOMELOCALEDIR=\"$(datadir)/locale\" \
	-I../intl -I$(top_srcdir)/intl

noinst_LTLIBRARIES = libdomcss.la

libdomcss_la_SOURCES = 	  	\
	dom-cssrule.c		\
	dom-cssrulelist.c	\
	dom-cssstyledeclaration.c	\
	dom-cssstylerule.c	\
	dom-cssstylesheet.c	\
	dom-cssvalue.c

MAINTAINERCLEANFILES =	\
	Makefile.in

noinst_LTLIBRARIES = libgtkgs.la

libgtkgs_la_SOURCES = \
    ggvutils.c ggvutils.h \
    gsdefaults.c gsdefaults.h \
    gtkgs.c gtkgs.h \
    ps.c ps.h

INCLUDES = \
	-I$(sylpheed_dir)/ \
	-I$(sylpheed_dir)/src/common

AM_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)

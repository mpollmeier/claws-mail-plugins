lcdprocdir = $(pkglibdir)/lcdprocs

lcdproc_LTLIBRARIES = lcdproc.la

lcdproc_la_SOURCES = \
	lcdproc.c

lcdproc_la_LDFLAGS = \
	-avoid-version -module \
	$(GTK_LIBS)

INCLUDES = \
	-I$(sylpheed_dir)/ \
	-I$(sylpheed_dir)/src/ \
	-I$(sylpheed_dir)/src/common \
	-I$(sylpheed_dir)/src/gtk

CPPFLAGS = \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)

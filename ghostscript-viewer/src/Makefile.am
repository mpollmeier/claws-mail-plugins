SUBDIRS = libgtkgs

plugindir = $(SYLPHEED_CLAWS_PLUGINDIR)

plugin_LTLIBRARIES = ghostscript_viewer.la

ghostscript_viewer_la_SOURCES = \
	plugin.c \
	viewer.c viewer.h \
	viewerprefs.c viewerprefs.h

ghostscript_viewer_la_LIBADD = \
	libgtkgs/libgtkgs.la

ghostscript_viewer_la_LDFLAGS = \
	-avoid-version -module \
	$(GTK_LIBS)

INCLUDES = \
	-Ilibgtkgs

AM_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)

SUBDIRS = libgtkgs

plugindir = $(libdir)/sylpheed/plugins

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
	-I$(sylpheed_dir)/ \
	-I$(sylpheed_dir)/src/ \
	-I$(sylpheed_dir)/src/common \
	-I$(sylpheed_dir)/src/gtk \
	-Ilibgtkgs

AM_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)

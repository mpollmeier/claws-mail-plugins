plugindir = $(CLAWS_MAIL_PLUGINDIR)

INCLUDES = @GLIB_CFLAGS@ \
		   -I$(top_srcdir) -I$(top_builddir)

plugin_LTLIBRARIES = archive.la

archive_la_SOURCES = \
	      libarchive_archive.c \
		  libarchive_archive.h \
		  archiver.c \
		  archiver.h \
		  archiver_gtk.c \
		  archiver_prefs.c \
	 	  gettext.h

archiveincludedir = $(includedir)/claws-mail/plugins/@PACKAGE@
archiveinclude_HEADERS = \
	archiver.h \
	archiver_prefs.h

archive_la_LDFLAGS = \
		-avoid-version -module \
		$(GTK_LIBS)

archive_la_LIBADD = \
		@ARCHIVE_LIBS@

AM_CPPFLAGS = \
		$(CLAWS_MAIL_CFLAGS) \
		$(GLIB_CFLAGS) \
		$(GTK_CFLAGS) \
		-DLOCALEDIR=\""$(localedir)"\"

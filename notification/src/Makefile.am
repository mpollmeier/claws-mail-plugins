CFLAGS += "-Wall"

plugindir = $(CLAWS_MAIL_PLUGINDIR)

plugin_LTLIBRARIES = notification_plugin.la

notification_plugin_la_SOURCES = \
	gettext.h \
	notification_plugin.c notification_plugin.h \
	notification_core.c notification_core.h \
	notification_popup.c notification_popup.h \
	notification_banner.c notification_banner.h \
	notification_command.c notification_command.h \
	notification_lcdproc.c notification_lcdproc.h \
	notification_trayicon.c notification_trayicon.h \
	notification_prefs.c notification_prefs.h \
	notification_foldercheck.c notification_foldercheck.h \
	notification_pixbuf.c notification_pixbuf.h \
	raw_claws_mail_logo_64x64.h \
	raw_trayicon_newmail.h \
	raw_trayicon_newmail_offline.h \
	raw_trayicon_newmarkedmail.h \
	raw_trayicon_newmarkedmail_offline.h \
	raw_trayicon_nomail.h \
	raw_trayicon_nomail_offline.h \
	raw_trayicon_unreadmail.h \
	raw_trayicon_unreadmail_offline.h \
	raw_trayicon_unreadmarkedmail.h \
	raw_trayicon_unreadmarkedmail_offline.h


notification_plugin_la_LDFLAGS = \
	-avoid-version -module

notification_plugin_la_LIBADD = \
	$(libnotify_LIBS)

notification_plugin_la_CPPFLAGS = \
	$(NOTIFY_CFLAGS) \
	$(CLAWS_MAIL_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	-DLOCALEDIR=\""$(localedir)"\" \
	$(libnotify_CFLAGS)

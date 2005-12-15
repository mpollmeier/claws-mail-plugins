plugindir = $(SYLPHEED_CLAWS_PLUGINDIR)

plugin_LTLIBRARIES = acpi_notifier.la

acpi_notifier_la_SOURCES = \
	acpi_notifier.c \
	plugin_version.h

acpi_notifier_la_LDFLAGS = \
	-avoid-version -module

acpi_notifier_la_LIBADD = \
	$(GTK_LIBS)

AM_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)

EXTRA_DIST = \
	plugin_version.h.in

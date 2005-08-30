plugindir = $(SYLPHEED_CLAWS_PLUGINDIR)

plugin_LTLIBRARIES = acpi_notifier.la

acpi_notifier_la_SOURCES = \
	acpi_notifier.c

acpi_notifier_la_LDFLAGS = \
	-avoid-version -module

acpi_notifier_la_LIBADD = \
	$(GTK_LIBS)

INCLUDES = \
	-I$(sylpheed_dir)/ \
	-I$(sylpheed_dir)/src/ \
	-I$(sylpheed_dir)/src/common \
	-I$(sylpheed_dir)/src/gtk

AM_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)

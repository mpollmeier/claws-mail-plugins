#ifndef __RSSYL_PREFS
#define __RSSYL_PREFS

#define PREFS_BLOCK_NAME	"rssyl"

#define RSSYL_NUM_PREFS		3

#define RSSYL_PREF_DEFAULT_REFRESH	"180"
#define RSSYL_PREF_DEFAULT_EXPIRED	"-1"
#define RSSYL_PREF_DEFAULT_TIMEOUT	"20"

typedef struct _RSSylPrefs RSSylPrefs;

struct _RSSylPrefs {
	gint refresh;
	gint expired;
	gint timeout;
};

typedef struct _RSSylPrefsPage RSSylPrefsPage;

struct _RSSylPrefsPage {
	PrefsPage page;
	GtkWidget *refresh;
	GtkWidget *expired;
	GtkWidget *timeout;
};

void rssyl_prefs_init(void);
void rssyl_prefs_done(void);
RSSylPrefs *rssyl_prefs_get(void);

#endif /* __RSSYL_PREFS */
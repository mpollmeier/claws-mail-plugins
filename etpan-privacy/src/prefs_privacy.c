#include "prefs_privacy.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "defs.h"
#include "utils.h"
#include "prefs.h"
#include "prefs_gtk.h"
#include "filesel.h"

#include "gtk/gtkutils.h"
#include "gtk/prefswindow.h"

#include "manage_window.h"
#include "etpan_privacy.h"

typedef struct _EtPanPrivacyPage
{
	PrefsPage page;

	GtkWidget *window;		/* do not modify */

	GtkWidget *entry_smime_cert_dir;
	GtkWidget *entry_smime_CA_dir;
	GtkWidget *entry_smime_private_keys_dir;
	GtkWidget *checkbtn_smime_CA_check;
	GtkWidget *checkbtn_smime_auto_store_cert;
} EtPanPrivacyPage;

typedef struct _EtPanPrefs {
	gchar * smime_cert_dir;
	gchar * smime_CA_dir;
	gchar * smime_private_keys_dir;
	gboolean smime_CA_check;
	gboolean smime_auto_store_cert;
} EtPanPrivacyPrefs;

static EtPanPrivacyPrefs etpan_privacy_prefs;

static EtPanPrivacyPage etpan_privacy_page;

#define PREFS_BLOCK_NAME "EtpanPrivacy"

static PrefParam param[] = {
	{"smime_auto_store_cert", "TRUE",
	 &etpan_privacy_prefs.smime_CA_check, P_BOOL,
	 &etpan_privacy_page.checkbtn_smime_auto_store_cert,
	 prefs_set_data_from_toggle, prefs_set_toggle},
	{"smime_CA_check", "TRUE",
	 &etpan_privacy_prefs.smime_CA_check, P_BOOL,
	 &etpan_privacy_page.checkbtn_smime_CA_check,
	 prefs_set_data_from_toggle, prefs_set_toggle},
	{"smime_CA_dir", "",
	 &etpan_privacy_prefs.smime_CA_dir, P_STRING,
	 &etpan_privacy_page.entry_smime_CA_dir,
	 prefs_set_data_from_entry, prefs_set_entry},
	{"smime_cert_dir", "",
	 &etpan_privacy_prefs.smime_cert_dir, P_STRING,
	 &etpan_privacy_page.entry_smime_cert_dir,
	 prefs_set_data_from_entry, prefs_set_entry},
	{"smime_private_keys_dir", "",
	 &etpan_privacy_prefs.smime_private_keys_dir, P_STRING,
	 &etpan_privacy_page.entry_smime_private_keys_dir,
	 prefs_set_data_from_entry, prefs_set_entry},
        {0,0,0,0,0,0,0}
};

static void get_directory(GtkButton *button, GtkEntry *entry)
{
	gchar *file;

	file = filesel_select_file_open(_("Select Directory"), "");
	if (file == NULL)
		return;
	
	gtk_entry_set_text(entry, file);
}

static void create_widget(PrefsPage *_page, GtkWindow *window, 
			  gpointer data)
{
	EtPanPrivacyPage *prefs_etpan_privacy = (EtPanPrivacyPage *) _page;

	GtkWidget *table;
	GtkWidget *entry_smime_cert_dir;
	GtkWidget *entry_smime_CA_dir;
	GtkWidget *entry_smime_private_keys_dir;
	GtkWidget *checkbtn_smime_CA_check;
	GtkWidget *checkbtn_smime_auto_store_cert;
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *button;
	
	table = gtk_table_new(8, 3, FALSE);
	gtk_widget_show(table);
	gtk_container_set_border_width(GTK_CONTAINER(table), 8);
	gtk_table_set_row_spacings(GTK_TABLE(table), 4);
	gtk_table_set_col_spacings(GTK_TABLE(table), 8);

	/* S/MIME certificates */
	
	label = gtk_label_new (_("S/MIME certificates directory"));
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
			 (GtkAttachOptions) (GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);

	entry_smime_cert_dir = gtk_entry_new ();
	gtk_widget_show (entry_smime_cert_dir);
	gtk_table_attach (GTK_TABLE (table), entry_smime_cert_dir, 1, 2, 0, 1,
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);
	gtk_entry_set_text(GTK_ENTRY(entry_smime_cert_dir),
			   etpan_privacy_prefs.smime_cert_dir);

	button = gtk_button_new_with_label (" ... ");

	gtk_widget_show (button);
	gtk_table_attach (GTK_TABLE (table), button, 2, 3, 0, 1,
			  0, 0, 0, 0);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (get_directory),
			    entry_smime_cert_dir);
	
	/* S/MIME CA directory */
	
	label = gtk_label_new (_("S/MIME certificate authority directory"));
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
			 (GtkAttachOptions) (GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);

	entry_smime_CA_dir = gtk_entry_new ();
	gtk_widget_show (entry_smime_CA_dir);
	gtk_table_attach (GTK_TABLE (table), entry_smime_CA_dir, 1, 2, 1, 2,
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);
	gtk_entry_set_text(GTK_ENTRY(entry_smime_CA_dir),
			   etpan_privacy_prefs.smime_CA_dir);

	button = gtk_button_new_with_label (" ... ");

	gtk_widget_show (button);
	gtk_table_attach (GTK_TABLE (table), button, 2, 3, 1, 2,
			  0, 0, 0, 0);
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (get_directory),
			    entry_smime_CA_dir);
	
	/* S/MIME private keys directory */
	
	label = gtk_label_new (_("S/MIME private keys directory"));
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
			 (GtkAttachOptions) (GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);

	entry_smime_private_keys_dir = gtk_entry_new ();
	gtk_widget_show (entry_smime_private_keys_dir);
	gtk_table_attach (GTK_TABLE (table),
			  entry_smime_private_keys_dir, 1, 2, 2, 3,
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);
	gtk_entry_set_text(GTK_ENTRY(entry_smime_private_keys_dir),
			   etpan_privacy_prefs.smime_private_keys_dir);

	button = gtk_button_new_with_label (" ... ");

	gtk_widget_show (button);
	gtk_table_attach (GTK_TABLE (table), button, 2, 3, 2, 3,
			  0, 0, 0, 0);
	g_signal_connect (G_OBJECT (button), "clicked",
			  G_CALLBACK (get_directory),
			  entry_smime_private_keys_dir);

	/* help */

	label = gtk_label_new (_("S/MIME private keys should be stored in\n files named [e-mail address]-private-key.pem"));
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 0, 2, 3, 4,
			 (GtkAttachOptions) (GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	
	/* check buttons */
	
	vbox = gtk_vbox_new(FALSE, VSPACING_NARROW);
	gtk_widget_show(vbox);
	gtk_table_attach (GTK_TABLE (table), vbox, 0, 4, 4, 5,
			 (GtkAttachOptions) (GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);

	PACK_CHECK_BUTTON (vbox, checkbtn_smime_CA_check,
			   _("Check signature"));
	PACK_CHECK_BUTTON (vbox, checkbtn_smime_auto_store_cert,
			   _("Store certificates"));
	
	prefs_etpan_privacy->window		        = GTK_WIDGET(window);
	prefs_etpan_privacy->entry_smime_cert_dir	= entry_smime_cert_dir;
	prefs_etpan_privacy->entry_smime_CA_dir	= entry_smime_CA_dir;
	prefs_etpan_privacy->entry_smime_private_keys_dir = entry_smime_private_keys_dir;
	prefs_etpan_privacy->checkbtn_smime_CA_check	= checkbtn_smime_CA_check;
	prefs_etpan_privacy->checkbtn_smime_auto_store_cert = checkbtn_smime_auto_store_cert;

	prefs_etpan_privacy->page.widget = table;
	
	prefs_set_dialog(param);
}


static void destroy_widget(PrefsPage *page)
{
	/* do nothing */
}

static void apply_config(void)
{
	struct mailprivacy * privacy;
	
	privacy = get_etpan_privacy();
	
	mailprivacy_smime_set_cert_dir(privacy, etpan_privacy_prefs.smime_cert_dir);
	mailprivacy_smime_set_CA_dir(privacy, etpan_privacy_prefs.smime_CA_dir);
	mailprivacy_smime_set_private_keys_dir(privacy, etpan_privacy_prefs.smime_private_keys_dir);
	
	mailprivacy_smime_set_CA_check(privacy, etpan_privacy_prefs.smime_CA_check);
	mailprivacy_smime_set_store_cert(privacy, etpan_privacy_prefs.smime_auto_store_cert);
}

void etpan_privacy_prefs_read_config(void)
{
	gchar *rcpath;
	prefs_set_default(param);
	rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, COMMON_RC, NULL);
	prefs_read_config(param, PREFS_BLOCK_NAME, rcpath, NULL);
	g_free(rcpath);
	apply_config();
}

void etpan_privacy_prefs_write_config(void)
{
	PrefFile *pfile;
	gchar *rcpath;

	rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, COMMON_RC, NULL);
	pfile = prefs_write_open(rcpath);
	g_free(rcpath);
	if (!pfile || (prefs_set_block_label(pfile, PREFS_BLOCK_NAME) < 0))
		return;

	if (prefs_write_param(param, pfile->fp) < 0) {
		g_warning("failed to write " PREFS_BLOCK_NAME " configuration to file\n");
		prefs_file_close_revert(pfile);
		return;
	}
	fprintf(pfile->fp, "\n");

	prefs_file_close(pfile);
}


static void save_prefs(PrefsPage *page)
{
	prefs_set_data_from_dialog(param);
	apply_config();
	etpan_privacy_prefs_write_config();
}


static void set_default_dir(gchar ** p_dirname, char * default_basename)
{
	gchar * dirname;
	
	dirname = * p_dirname;
	
	if (dirname != NULL) {
		if (dirname == '\0') {
			g_free(dirname);
			dirname = NULL;
		}
	}

	if (dirname == NULL) {
		gchar default_dir[4096];
		
		g_snprintf(default_dir, sizeof(default_dir), "%s/%s",
			   get_rc_dir(), default_basename);
		dirname = g_strdup(default_dir);
	}
	
	* p_dirname = dirname;
	
	make_dir_hier(dirname);
}


void etpan_privacy_prefs_init(void)
{
	static gchar *path[3];
	
        prefs_set_default(param);
	
	etpan_privacy_prefs_read_config();
	
	set_default_dir(&etpan_privacy_prefs.smime_CA_dir,
			PREFS_PRIVACY_DEFAULT_CA_DIR);
	set_default_dir(&etpan_privacy_prefs.smime_cert_dir,
			PREFS_PRIVACY_DEFAULT_CERT_DIR);
	set_default_dir(&etpan_privacy_prefs.smime_private_keys_dir,
			PREFS_PRIVACY_DEFAULT_PRIVATE_KEYS_DIR);
        
	path[0] = _("Plugins");
	path[1] = _("etpan-Privacy");
	path[2] = NULL;
	
        etpan_privacy_page.page.path = path;
        etpan_privacy_page.page.create_widget = create_widget;
        etpan_privacy_page.page.destroy_widget = destroy_widget;
        etpan_privacy_page.page.save_page = save_prefs;
        
        prefs_gtk_register_page((PrefsPage *) &etpan_privacy_page);
}

void etpan_privacy_prefs_done(void)
{
	g_free(etpan_privacy_prefs.smime_cert_dir);
	g_free(etpan_privacy_prefs.smime_CA_dir);
	g_free(etpan_privacy_prefs.smime_private_keys_dir);
        prefs_gtk_unregister_page((PrefsPage *) &etpan_privacy_page);
}

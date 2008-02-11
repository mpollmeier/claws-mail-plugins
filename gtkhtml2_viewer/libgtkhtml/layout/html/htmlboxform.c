#include <string.h>

#include "dom/html/dom-htmlformelement.h"
#include "layout/htmlboxtable.h"
#include "layout/htmlboxtablerow.h"
#include "layout/htmlboxtablecell.h"
#include "layout/html/htmlboxform.h"
#include "layout/htmlrelayout.h"

static HtmlBoxClass *parent_class = NULL;                                                                                                      

GSList *
html_box_form_get_radio_group (HtmlBoxForm *form, gchar *id)
{
	return g_hash_table_lookup (form->radio_groups, id);
}

void
html_box_form_set_radio_group (HtmlBoxForm *form, gchar *id, GSList *list)
{
	g_hash_table_insert (form->radio_groups, g_strdup (id), list);
}

static void
html_box_form_append_child (HtmlBox *self, HtmlBox *child)
{
	/*
	 * This is a workaround for the fact that you can have a
	 * <form> tag between a <table> and a <tr> tag.
	 */
	if (HTML_IS_BOX_TABLE (self->parent) &&
	    HTML_IS_BOX_TABLE_ROW (child)) {
		HtmlBoxTable *table = HTML_BOX_TABLE (self->parent);
		
		html_box_table_add_tbody (table, HTML_BOX_TABLE_ROW (child));
		parent_class->append_child (self, child);
	}
	else
		parent_class->append_child (self, child);
}

static gboolean
free_hash (gpointer key, gpointer value, gpointer user_data)
{
	g_free (key);

	return TRUE;
}

static void
html_box_form_finalize (GObject *object)
{
	HtmlBoxForm *form = HTML_BOX_FORM (object);

	g_hash_table_foreach_remove (form->radio_groups, free_hash, NULL);
	g_hash_table_destroy (form->radio_groups);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
html_box_form_class_init (GObjectClass *klass)
{
	HtmlBoxClass *box_class = (HtmlBoxClass *)klass;

	box_class->append_child = html_box_form_append_child;

	klass->finalize = html_box_form_finalize;

	parent_class = g_type_class_peek_parent (klass);
}

static void
html_box_form_init (HtmlBoxForm *form)
{
	form->radio_groups = g_hash_table_new (g_str_hash, g_str_equal);
}

GType
html_box_form_get_type (void)
{
	static GType html_type = 0;

       if (!html_type) {
	       static GTypeInfo type_info = {
                       sizeof (HtmlBoxFormClass),
		       NULL,
		       NULL,
                       (GClassInitFunc) html_box_form_class_init,		       
		       NULL,
		       NULL,
                       sizeof (HtmlBoxForm),
		       16, 
                       (GInstanceInitFunc) html_box_form_init
               };

               html_type = g_type_register_static (HTML_TYPE_BOX_BLOCK, "HtmlBoxForm", &type_info, 0);
       }
       
       return html_type;
}

HtmlBox *
html_box_form_new (void)
{
	return HTML_BOX (g_type_create_instance (HTML_TYPE_BOX_FORM));
}



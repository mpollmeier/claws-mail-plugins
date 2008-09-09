/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

#include <config.h>
#include <gtk/gtk.h>
#include "debug.h"
#include "htmldocument.h"
#include "htmlview.h"
#include "htmlbox.h"
#include "htmlboxtext.h"
#include "htmlboxblock.h"
#include "htmlboxinline.h"
#include "htmlboxinline.h"

HtmlView *view;

static gint
cb_delete (GtkWidget *widget, gpointer data)
{
	html_box_destroy (view->root);

	debug_dump_box_usage ();
	gtk_main_quit ();

	return FALSE;
}

static void
cb_dump_boxes (gpointer data, guint action, GtkWidget *widget)
{
	debug_dump_boxes (view->root, 0, FALSE, NULL);
}

static void
cb_dump_box_usage (gpointer data, guint action, GtkWidget *widget)
{
	debug_dump_box_usage ();
}

static void
test_text_align_right (gpointer data, guint action, GtkWidget *widget)
{
	HtmlBox *text;

	html_box_destroy (view->root);

	view->root = html_box_block_new ();
	view->root->style_text_align = HTML_TEXT_ALIGN_RIGHT;

	text = html_box_text_new ();
	html_box_text_set_text (text,  "This is a text box. And since we now have very cool scrolling we can have an even longer line here. Hey, why not even toss in some odd characters, like this: åäö ÅÄÖ (This text should be right aligned)");
 
	html_box_append_child (view->root, text);
	html_box_set_margin (view->root, 10);
	html_box_set_border (view->root, 5);
	html_box_set_padding (view->root, 5);

	html_view_relayout (view);
	gtk_widget_queue_draw (GTK_WIDGET (view));
}

static void
test_text_align_center (gpointer data, guint action, GtkWidget *widget)
{
	HtmlBox *text;

	html_box_destroy (view->root);

	view->root = html_box_block_new ();
	view->root->style_text_align = HTML_TEXT_ALIGN_CENTER;

	text = html_box_text_new ();
	html_box_text_set_text (text,  "This is a text box. And since we now have very cool scrolling we can have an even longer line here. Hey, why not even toss in some odd characters, like this: åäö ÅÄÖ (This text should be floated around the floats)");
 
	html_box_append_child (view->root, text);
	html_box_set_margin (view->root, 10);
	html_box_set_border (view->root, 5);
	html_box_set_padding (view->root, 5);

	html_view_relayout (view);
	gtk_widget_queue_draw (GTK_WIDGET (view));
}

static void
test_float (gpointer data, guint action, GtkWidget *widget)
{
	HtmlBox *text, *text2, *Float, *Float2, *Float3, *Float4,
	*Float5, *Float6, *text3,
	*text5, *text6, *text7, *text8, *text9, *block, *block2;

	html_box_destroy (view->root);

	view->root = html_box_block_new ();
	Float = html_box_block_new ();
	Float->style_float = HTML_FLOAT_RIGHT;
	Float->style_width = 200;
	Float->style_height = 100;

	Float2 = html_box_block_new ();
	Float2->style_float = HTML_FLOAT_RIGHT;
	Float2->style_width = 200;
	Float2->style_height = 100;

	Float3 = html_box_block_new ();
	Float3->style_float = HTML_FLOAT_RIGHT;
	Float3->style_width = 200;
	Float3->style_height = 100;

	Float4 = html_box_block_new ();
	Float4->style_float = HTML_FLOAT_LEFT;
	Float4->style_width = 200;
	Float4->style_height = 100;

	Float5 = html_box_block_new ();
	Float5->style_float = HTML_FLOAT_LEFT;
	Float5->style_width = 200;
	Float5->style_height = 100;

	Float6 = html_box_block_new ();
	Float6->style_float = HTML_FLOAT_LEFT;
	Float6->style_width = 200;
	Float6->style_height = 100;

	text = html_box_text_new ();
	html_box_text_set_text (text,  "This is a text box. And since we now have very cool scrolling we can have an even longer line here. Hey, why not even toss in some odd characters, like this: åäö ÅÄÖ (This text should be left_aligned)");

	text9 = html_box_text_new ();
	html_box_text_set_text (text9,  "This is a text box. And since we now have very cool scrolling we can have an even longer line here. Hey, why not even toss in some odd characters, like this: åäö ÅÄÖ (This text should be left_aligned)");

	text2 = html_box_text_new ();
	html_box_text_set_text (text2,  "Float: Right1.");

	text3 = html_box_text_new ();
	html_box_text_set_text (text3,  "Float: Right2.");

	text5 = html_box_text_new ();
	html_box_text_set_text (text5,  "Float: Right3.");
 
	text6 = html_box_text_new ();
	html_box_text_set_text (text6,  "Float: Left1.");
 
	text7 = html_box_text_new ();
	html_box_text_set_text (text7,  "Float: Left2.");
 
	text8 = html_box_text_new ();
	html_box_text_set_text (text8,  "Float: Left3.");
 
	html_box_append_child (Float, text2);
	html_box_append_child (Float2, text3);
	html_box_append_child (Float3, text5);

	html_box_append_child (Float4, text6);
	html_box_append_child (Float5, text7);
	html_box_append_child (Float6, text8);

	/* Create a block because the anonymouse boxes doesn't inherit
	   styles yet */
	block2 = html_box_block_new ();
	block2->style_text_align = HTML_TEXT_ALIGN_LEFT;
#if 1 /* Text before floats */
	html_box_append_child (block2, text9);
	html_box_append_child (view->root, block2);
#endif
	html_box_append_child (view->root, Float4);/* Left */
	html_box_append_child (view->root, Float); /* Right */
	html_box_append_child (view->root, Float5);/* Left */
	html_box_append_child (view->root, Float2);/* Right */
	html_box_append_child (view->root, Float6);/* Left */
	html_box_append_child (view->root, Float3);/* Right */
	/* Create a block because the anonymouse boxes doesn't inherit
	   styles yet */
	block = html_box_block_new ();
	block->style_text_align = HTML_TEXT_ALIGN_LEFT;
#if 1 /* Text after floats */
	html_box_append_child (block, text);
	html_box_append_child (view->root, block);
#endif

	html_box_set_border (Float, 3);
	html_box_set_border (Float2, 3);
	html_box_set_border (Float3, 3);
	html_box_set_border (Float4, 3);
	html_box_set_border (Float5, 3);
	html_box_set_border (Float6, 3);

	html_box_set_margin (Float, 5);
	html_box_set_margin (Float2, 5);
	html_box_set_margin (Float3, 5);
	html_box_set_margin (Float4, 5);
	html_box_set_margin (Float5, 5);
	html_box_set_margin (Float6, 5);

	html_view_relayout (view);
	gtk_widget_queue_draw (GTK_WIDGET (view));
}

static void
test_recursive_floats (gpointer data, guint action, GtkWidget *widget)
{
	HtmlBox *text, *parent_float, *child_float,
		*child_text, *parent_text;

	html_box_destroy (view->root);

	view->root = html_box_block_new ();
	view->root->style_text_align = HTML_TEXT_ALIGN_LEFT;

	child_float = html_box_block_new ();
	child_float->style_text_align = HTML_TEXT_ALIGN_LEFT;
	child_float->style_width = 200;
	child_float->style_height = 150;
	child_float->style_float = HTML_FLOAT_RIGHT;
	child_text = html_box_text_new ();
	html_box_text_set_text (child_text,  "Child float");
	html_box_append_child (child_float, child_text);


	parent_float = html_box_block_new ();
	parent_float->style_width = 500;
	parent_float->style_height = 80;
	parent_float->style_float = HTML_FLOAT_LEFT;
	parent_text = html_box_text_new ();
	html_box_text_set_text (parent_text,  "Parent float abcdefghijklmno");
	html_box_append_child (parent_float, parent_text);
#if 1
	html_box_append_child (parent_float, child_float);
#endif
	text = html_box_text_new ();
	html_box_text_set_text (text,  "This is a text box. And since we now have very cool scrolling we can have an even longer line here. Hey, why not even toss in some odd characters, like this: åäö ÅÄÖ (This text should be floated around the floats)");
 
	html_box_set_border (child_float, 10);
	html_box_set_margin (parent_float, 25);
	html_box_set_border (parent_float, 5);

	html_box_append_child (view->root, parent_float);
	html_box_append_child (view->root, text);

	html_view_relayout (view);
	gtk_widget_queue_draw (GTK_WIDGET (view));
}

static GtkItemFactoryEntry menu_items[] = {
	{ "/Debug",          NULL,         0, 0, "<Branch>" },
	{ "/Debug/Show object list", NULL, cb_dump_boxes, 0, NULL },
	{ "/Debug/Show box usage", NULL, cb_dump_box_usage, 0, NULL },
	{ "/Test",          NULL,         0, 0, "<Branch>" },
	{ "/Test/Text-align:right", NULL, test_text_align_right, 0, NULL },
	{ "/Test/Text-align:center", NULL, test_text_align_center, 0, NULL },
	{ "/Test/Float:left|right", NULL, test_float, 0, NULL },
	{ "/Test/recursive floats", NULL, test_recursive_floats, 0, NULL }
};

gint
main (gint argc, gchar **argv)
{
	HtmlDocument *doc;
	HtmlBox *text;
	GtkHtml *html;
	GtkWidget *scr, *vbox;
	GtkItemFactory *item_factory;
	
	GtkWidget *app;

	gtk_init (&argc, &argv);

	app = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size (GTK_WINDOW (app), 800, 400);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (app), vbox);
	
	item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", NULL);
	gtk_item_factory_create_items (item_factory, G_N_ELEMENTS (menu_items), menu_items, app);

	gtk_box_pack_start (GTK_BOX (vbox),
			    gtk_item_factory_get_widget (item_factory, "<main>"),
			    FALSE, FALSE, 0);

	html = gtk_html_new ();
	doc = gtk_html_create_document (html);
			    
	view = gtk_html_create_view (html, doc);
	text = html_box_text_new ();
	html_box_text_set_text (text, "Welcome to testgtkhtml2, please select a test from the menubar. (Resize the window to see the result)");

	html_box_append_child (view->root, text);

	scr = gtk_scrolled_window_new (gtk_layout_get_hadjustment (GTK_LAYOUT (view)),
				       gtk_layout_get_vadjustment (GTK_LAYOUT (view)));
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scr), GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	
	gtk_container_add (GTK_CONTAINER (scr), GTK_WIDGET (view));
	
	gtk_box_pack_start (GTK_BOX (vbox),
			    scr,
			    TRUE, TRUE, 0);

	gtk_signal_connect (GTK_OBJECT (app), "delete_event",
			    G_CALLBACK (cb_delete), NULL);
	
	gtk_widget_show_all (app);
	html_view_relayout (view);
	
	gtk_main ();

	return 0;
}

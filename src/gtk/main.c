/* NVClock 0.8 - Linux overclocker for NVIDIA cards
 *
 * Copyright(C) 2001-2008 Roderick Colenbrander
 *
 * site: http://NVClock.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <gtk/gtk.h>

#include "banner.h"
#include "interface.h"
#include "nvclock.h"

enum
{
	NAME_COLUMN = 0,
	TYPE_COLUMN,
	WIDGET_COLUMN,
	NUM_COLUMNS
};

struct MainWindow *main_window;

void nvclock_quit()
{
	char *filename = g_strdup_printf("%s/config", nvclock.path);
	write_config(nvclock.cfg, filename);
	gtk_main_quit();
	free(filename);
}


GtkWidget* create_window_main (void)
{
	GtkWidget *table;
	GtkWidget *window_nvclock;
	GtkWidget *vbox;
	GtkWidget *hbox;

	window_nvclock = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window_nvclock), 540, 280);
	gtk_window_set_title(GTK_WINDOW(window_nvclock), "NVClock 0.8 (Beta4)");

	main_window = (struct MainWindow*)calloc(1, sizeof(struct MainWindow));

	/* Create a treeview to contain a list of "pages" */
	main_window->treeview = gtk_tree_view_new();
	/* Set the minimum size to 200 */
	gtk_widget_set_size_request(main_window->treeview, 225, -1);
	/* Don't display the headers */
	g_object_set(main_window->treeview, "headers-visible", FALSE, NULL);

	/* Create a scrolled window to pack the treeview in */
	main_window->swindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(main_window->swindow), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(main_window->swindow), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(main_window->swindow), main_window->treeview);

	/* Vbox in which to package banner + 'page view' */
	vbox = gtk_vbox_new(FALSE, 2);

	/* Banner containing the name of the current page*/
	main_window->frame_banner = gtk_frame_new(NULL);
	main_window->banner = banner_new(300, 60);
	gtk_container_add(GTK_CONTAINER(main_window->frame_banner), main_window->banner);

	/* Pack the banner in a hbox to make sure it won't 'move' to the left when resizing the window. */
	hbox = gtk_hbox_new(FALSE, 1);
	gtk_box_pack_start(GTK_BOX(hbox), main_window->frame_banner, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	main_window->page = NULL;

	/* Use this to display pages in */
	main_window->view = gtk_vbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), main_window->view, TRUE, TRUE, 0);

	/* The hpaned will split the window in a treeview part and in a "page" part */
	main_window->hpaned = gtk_hpaned_new();
	table = gtk_table_new(2,1, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(table), 2);

	/* Left page */
	gtk_paned_pack1(GTK_PANED(main_window->hpaned), GTK_WIDGET(main_window->swindow), FALSE, FALSE);

	/* Right page */
	gtk_paned_pack2(GTK_PANED(main_window->hpaned), vbox, TRUE, TRUE);
	gtk_table_attach (GTK_TABLE (table), GTK_WIDGET(main_window->hpaned), 0, 1, 0, 1,
		(GtkAttachOptions) (GTK_FILL | GTK_EXPAND), (GtkAttachOptions) (GTK_FILL | GTK_EXPAND), 3, 0);

	/* Create a buttonbox to pack buttons like Quit */
	main_window->bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(main_window->bbox), GTK_BUTTONBOX_END);
	gtk_table_attach (GTK_TABLE (table), GTK_WIDGET(main_window->bbox), 0, 1, 1, 2,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	/* Quit button */
	main_window->btn_quit = gtk_button_new_from_stock ("gtk-quit");
	gtk_container_add(GTK_CONTAINER(main_window->bbox), main_window->btn_quit);

	gtk_container_add(GTK_CONTAINER(window_nvclock), table);

	gtk_widget_show_all(window_nvclock);

	g_signal_connect (G_OBJECT (window_nvclock), "delete_event", G_CALLBACK(nvclock_quit), NULL);
	g_signal_connect (G_OBJECT (main_window->btn_quit), "clicked", G_CALLBACK (nvclock_quit), NULL);

	return window_nvclock;
}


void add(GtkTreeIter *child, GtkTreeIter *parent, char *name, int type, gpointer widget)
{
	/* Make sure the widget won't be deleted when we later do a gtk_container_remove as this call destroys the widget if no references exist */
	g_object_ref(G_OBJECT(widget));

	gtk_tree_store_append(main_window->tree_store, child, parent);
	gtk_tree_store_set(main_window->tree_store, child, NAME_COLUMN, name, TYPE_COLUMN, type, WIDGET_COLUMN, widget, -1);
}


void switch_page(GtkTreeView *list, gpointer user_data)
{
	GtkTreeIter iter;
	char *str;
	int type;
	GtkTreeSelection *selection =  gtk_tree_view_get_selection(GTK_TREE_VIEW(list));

	if(gtk_tree_selection_get_selected(selection, NULL, &iter))
	{
		GtkWidget *page;
		gtk_tree_model_get(GTK_TREE_MODEL(main_window->tree_store), &iter, NAME_COLUMN, &str, TYPE_COLUMN, &type, WIDGET_COLUMN, &page,  -1);

		if(main_window->page)
		{
			gtk_container_remove(GTK_CONTAINER(main_window->view), main_window->page);
			main_window->page = NULL;
		}

		main_window->page = page;
		gtk_container_add(GTK_CONTAINER(main_window->view), page);

		banner_set_text(GTK_BANNER(main_window->banner), str, type);
		g_free(str);
	}
}


void gui_init()
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;
	GtkTreePath *path;
	GtkTooltips *tips = gtk_tooltips_new();
	config *conf = (config*)calloc(1, sizeof(config));
	cfg_entry *entry;

	conf->cfg = (void*)nvclock.cfg;

#ifdef HAVE_NVCONTROL
	entry = lookup_entry(&nvclock.cfg, "gtk", "save_opengl_changes");
	if(entry)
	{
		conf->save_opengl_changes=entry->value;
	}
	/* By default enable saving of opengl changes unless the user disables it */
	else
	{
		change_entry(&nvclock.cfg, "gtk", "save_opengl_changes", 1);
		conf->save_opengl_changes=1;
	}

	entry = lookup_entry(&nvclock.cfg, "gtk", "use_lowlevel_backend");
	if(entry)
	{
		conf->use_lowlevel_backend=entry->value;
	}
	/* By default use the nvcontrol backend when available */
	else
	{
		change_entry(&nvclock.cfg, "gtk", "use_lowlevel_backend", 0);
		conf->use_lowlevel_backend=0;
	}
#endif

	/* Check if we should show tooltips */
	entry = lookup_entry(&nvclock.cfg, "gtk", "show_tooltips");
	if(entry)
	{
		conf->show_tooltips=entry->value;
	}
	/* By default show tooltips unlress unless the user disables it */
	else
	{
		change_entry(&nvclock.cfg, "gtk", "show_tooltips", 1);
		conf->show_tooltips=1;
	}

	if(!conf->show_tooltips)
		gtk_tooltips_disable(tips);

	/* name, column number, data */
	main_window->tree_store = gtk_tree_store_new( NUM_COLUMNS, G_TYPE_STRING, G_TYPE_INT, G_TYPE_POINTER);
	column = gtk_tree_view_column_new();
	cell = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, cell, FALSE);
	gtk_tree_view_column_set_attributes(column, cell,
		"text", NAME_COLUMN,
		NULL);
	gtk_tree_view_set_model(GTK_TREE_VIEW(main_window->treeview), GTK_TREE_MODEL(main_window->tree_store));
	gtk_tree_view_append_column(GTK_TREE_VIEW(main_window->treeview), column);

	/* When a different row is selected, we want to select a different page */
	g_signal_connect(G_OBJECT(main_window->treeview), "cursor-changed", G_CALLBACK(switch_page), NULL);

	nvclock.dpy = (void*)XOpenDisplay("");
	gui_hw_init(conf, tips);
#ifdef HAVE_NVCONTROL
	gui_gl_init(conf, tips);
#endif
	gui_settings_init(conf, tips);

	gtk_widget_show_all(main_window->treeview);
	gtk_tree_view_expand_all(GTK_TREE_VIEW(main_window->treeview));
	gtk_tree_view_columns_autosize(GTK_TREE_VIEW(main_window->treeview));

	/* Set the cursor the first row and give it focus */
	path = gtk_tree_path_new_from_string ("0");
	gtk_tree_view_set_cursor (GTK_TREE_VIEW (main_window->treeview), path, NULL, FALSE);
	gtk_widget_grab_focus(main_window->treeview);
	gtk_tree_path_free (path);
}


int main (int argc, char *argv[])
{
	GtkWidget *window_nvclock;

	gtk_set_locale ();
	gtk_init (&argc, &argv);

	/* Initialize nvclock. This must be done here instead of in the hardware backend
	/  because of the configuration file which gets initialized by init_nvclock.
	*/
	if(!init_nvclock())
	{
		char buf[80];
		GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", get_error(buf, 80));
		gtk_dialog_run(GTK_DIALOG(dialog));
		g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
		gtk_widget_destroy(dialog);

		return 0;
	}

	window_nvclock = create_window_main ();
	gtk_widget_show (window_nvclock);

	/* Build the real gui and put data on it */
	gui_init();

	gtk_main ();

	return 0;
}

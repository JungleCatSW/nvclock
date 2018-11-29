/* NVClock 0.8 - Linux overclocker for NVIDIA cards
 *
 * Copyright(C) 2001-2005 Roderick Colenbrander
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

#ifndef INTERFACE_H
#define INTERFACE_H

#include <gtk/gtk.h>
#include "config.h"

struct MainWindow
{
	GtkWidget *hpaned;
	/* Left side of the paned */
	GtkWidget *swindow;
	GtkWidget *treeview; /* Contains the names of all pages */
	GtkTreeStore *tree_store;

	/* Right side of the paned */
	GtkWidget *frame_banner;
	GtkWidget *banner; /* Contains a banner containing an image and the name of the current page */

	GtkWidget *view; /* Used to view pages */
	GtkWidget *page;

	/* Bottom of the window */
	GtkWidget *bbox;
	GtkWidget *btn_quit;
};

enum banner_type
{
	BANNER_GL=1,
	BANNER_HW,
	BANNER_SETTINGS
};

typedef struct
{
	void *cfg;
	int save_opengl_changes;
	int show_tooltips;
	int use_lowlevel_backend;
} config;

extern GtkWidget *window_nvclock;

/* General */
extern struct MainWindow *main_window;

void add(GtkTreeIter *child, GtkTreeIter *parent, char *name, int type, gpointer widget);
int gui_gl_init(config *conf, GtkTooltips *tips);
int gui_hw_init(config *conf, GtkTooltips *tips);
int gui_settings_init(config *conf, GtkTooltips *tips);
int gui_x_init();
#endif /* INTERFACE_H */

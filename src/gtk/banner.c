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
 *
 * Gtk banner code
 */

#include <gtk/gtk.h>
#include "banner.h"
#include "banner_gl.h"
#include "banner_hw.h"
#include "interface.h"

GType banner_get_type (void)
{
	static GType banner_type = 0;

	if (!banner_type)
	{
		static const GTypeInfo banner_info =
		{
			sizeof (BannerClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			NULL,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (Banner),
			0,		/* n_preallocs */
			NULL		/* instance_init */
		};

		banner_type = g_type_register_static (GTK_TYPE_DRAWING_AREA, "Banner", &banner_info, 0);
	}

	return banner_type;
}


static void banner_expose(GtkWidget * drawing_area, GdkEventExpose * event, gpointer data)
{
	GError *error = NULL;
	PangoAttrList *attrs;
	char *markup_text;
	char *text;
	Banner *banner = (Banner*)data;

	/* Do we need to free the old pixmap? */
	switch(banner->type)
	{
		case BANNER_GL:
			banner->pixmap = gdk_pixmap_create_from_xpm_d(drawing_area->window, NULL, NULL, banner_gl_xpm);
			break;
		case BANNER_HW:
		case BANNER_SETTINGS:
			banner->pixmap = gdk_pixmap_create_from_xpm_d(drawing_area->window, NULL, NULL, banner_hw_xpm);
			break;
	}

	markup_text = g_strdup_printf("<span size=\"x-large\" weight=\"bold\">%s</span>", banner->text);
	pango_parse_markup(markup_text, -1, 0, &attrs, &text, NULL, NULL);
	pango_layout_set_markup(banner->pango_layout, markup_text, -1);
	g_free(markup_text);

	gdk_draw_drawable(drawing_area->window,
		drawing_area->style->white_gc,
		banner->pixmap,
		0, 0,
		0, 0,
		banner->width,
		banner->height);

	/* Create a shadow */
	gdk_draw_layout(drawing_area->window, drawing_area->style->black_gc, 13, 13, banner->pango_layout);
	gdk_draw_layout(drawing_area->window, drawing_area->style->white_gc, 10, 10, banner->pango_layout);
}


GtkWidget *banner_new(int width, int height)
{
	Banner *banner = g_object_new(GTK_TYPE_BANNER, NULL);

	banner->pango_layout = gtk_widget_create_pango_layout(GTK_WIDGET(banner), NULL);
	banner->width = width;
	banner->height = height;

	gtk_widget_set_size_request(GTK_WIDGET(banner), banner->width, banner->height);
	gtk_signal_connect(GTK_OBJECT(banner), "expose_event",GTK_SIGNAL_FUNC(banner_expose), banner);

	return GTK_WIDGET(banner);
}


void banner_set_text(Banner *banner, char *text, int type)
{
	if(banner->text)
		g_free(banner->text);

	banner->text = g_strdup_printf(text);
	banner->type = type;
	/* Refresh the banner */
	banner_expose(GTK_WIDGET(banner), NULL, banner);
}

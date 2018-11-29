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
 * Gtk banner header
 */

#include <gtk/gtk.h>

#define GTK_TYPE_BANNER                  (banner_get_type ())
#define GTK_BANNER(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_BANNER, Banner))
#define GTK_BANNER_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_BANNER, BannerClass))
#define GTK_IS_BANNER(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_BANNER))
#define GTK_IS_BANNER_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_BANNER))
#define GTK_BANNER_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_BANNER, BannerClass))

typedef struct _Banner       Banner;
typedef struct _BannerClass  BannerClass;

struct _Banner
{
	GtkDrawingArea parent;
	GdkPixmap *pixmap;
	PangoLayout *pango_layout;
	int width;
	int height;
	char *text;
	int type;
};

struct _BannerClass
{
	GtkDrawingAreaClass parent_class;
};

GType banner_get_type(void) G_GNUC_CONST;
GtkWidget *banner_new(int width, int height);
void banner_set_text(Banner *banner, char *text, int type);

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

#include "interface.h"
#include "nvclock.h"
#ifdef HAVE_NVCONTROL
#include "nvcontrol.h"
#endif

#include <gdk/gdk.h>
#include <gtk/gtkmisc.h>

#define NV_TYPE_SETTINGS                  (nv_settings_get_type ())
#define NV_SETTINGS(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), NV_TYPE_SETTINGS, NVSettings))
#define NV_SETTINGS_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), NV_TYPE_SETTINGS, NVSettingsClass))
#define NV_IS_SETTINGS(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NV_TYPE_SETTINGS))
#define NV_IS_SETTINGS_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), NV_TYPE_SETTINGS))
#define NV_SETTINGS_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), NV_TYPE_SETTINGS, NVSettingsClass))

typedef struct _NVSettings       NVSettings;
typedef struct _NVSettingsClass  NVSettingsClass;

struct _NVSettings
{
	GtkVBox parent;

	config *conf;
	GtkTooltips *tips;

	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *chk_show_tooltips;
	GtkWidget *chk_save_opengl;
	GtkWidget *chk_backend;
};

struct _NVSettingsClass
{
	GtkVBoxClass parent_class;
};

GType      nv_settings_get_type   (void) G_GNUC_CONST;
GtkWidget* nv_settings_new        (config *conf, GtkTooltips *tips);

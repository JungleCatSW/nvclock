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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "interface.h"
#include "settings.h"

#include "nvclock.h"

GType nv_settings_get_type (void)
{
	static GType settings_type = 0;

	if (!settings_type)
	{
		static const GTypeInfo settings_info =
		{
			sizeof (NVSettingsClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			NULL,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (NVSettings),
			0,			/* n_preallocs */
			NULL		/* instance_init */
		};

		settings_type = g_type_register_static (GTK_TYPE_VBOX, "NVSettings",
			&settings_info, 0);
	}

	return settings_type;
}


void chk_backend_toggled(GtkToggleButton *button, gpointer data)
{
	config *conf = (config*)data;

	change_entry((cfg_entry**)&conf->cfg, "gtk", "aapje", 1);
	if(gtk_toggle_button_get_active(button))
	{
		change_entry((cfg_entry**)&conf->cfg, "gtk", "use_lowlevel_backend", 1);
		conf->use_lowlevel_backend=1;
	}
	else
	{
		change_entry((cfg_entry**)&conf->cfg, "gtk", "use_lowlevel_backend", 0);
		conf->use_lowlevel_backend=0;
	}
}


void chk_show_tooltips_toggled(GtkToggleButton *button, gpointer data)
{
	NVSettings *settings = NV_SETTINGS(data);

	if(gtk_toggle_button_get_active(button))
	{
		change_entry((cfg_entry**)&settings->conf->cfg, "gtk", "show_tooltips", 1);
		gtk_tooltips_enable(settings->tips);
		settings->conf->show_tooltips=1;
	}
	else
	{
		change_entry((cfg_entry**)&settings->conf->cfg, "gtk", "show_tooltips", 0);
		gtk_tooltips_disable(settings->tips);
		settings->conf->show_tooltips=0;
	}
}


void chk_save_opengl_toggled(GtkToggleButton *button, gpointer data)
{
	config *conf = (config*)data;

	if(gtk_toggle_button_get_active(button))
	{
		change_entry((cfg_entry**)&conf->cfg, "gtk", "save_opengl_changes", 1);
		conf->save_opengl_changes=1;
	}
	else
	{
		change_entry((cfg_entry**)&conf->cfg, "gtk", "save_opengl_changes", 0);
		conf->save_opengl_changes=0;
	}
}


GtkWidget* nv_settings_new (config *conf, GtkTooltips *tips)
{
	NVSettings *settings = g_object_new (NV_TYPE_SETTINGS, NULL);

	settings->conf = conf;
	settings->tips = tips;

	settings->frame = gtk_frame_new("NVClock Configuration");
	gtk_box_pack_start(GTK_BOX(settings), settings->frame, FALSE, FALSE, 0);

	settings->vbox = gtk_vbox_new(2, FALSE);
	gtk_container_add(GTK_CONTAINER(settings->frame), settings->vbox);

	settings->chk_show_tooltips = gtk_check_button_new_with_label("Show tooltips");
	gtk_tooltips_set_tip(GTK_TOOLTIPS(settings->tips), settings->chk_show_tooltips, \
		"Show small information messages when the mouse is moved over some settings.", NULL);

	gtk_box_pack_start(GTK_BOX(settings->vbox), settings->chk_show_tooltips, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->chk_show_tooltips), conf->show_tooltips);
	g_signal_connect(GTK_TOGGLE_BUTTON(settings->chk_show_tooltips), "toggled", G_CALLBACK (chk_show_tooltips_toggled), settings);

#ifdef HAVE_NVCONTROL
	settings->chk_save_opengl = gtk_check_button_new_with_label("Save OpenGL settings");
	gtk_tooltips_set_tip(GTK_TOOLTIPS(settings->tips), settings->chk_save_opengl, \
		"When enabled OpenGL settings will be saved to the NVClock config file. "
		"The settings will be reloaded the next time you run NVClock. "
		"The configuration file behaviour might conflict with nvidia-settings "
		"as that program also restores its settings when you start it. "
		"An alternative is to disable saving to the config file in which case "
		"the actual OpenGL settings will be shown.", NULL);
	gtk_box_pack_start(GTK_BOX(settings->vbox), settings->chk_save_opengl, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->chk_save_opengl), conf->save_opengl_changes);
	g_signal_connect(GTK_TOGGLE_BUTTON(settings->chk_save_opengl), "toggled", G_CALLBACK (chk_save_opengl_toggled), conf);

	settings->chk_backend = gtk_check_button_new_with_label("Use low-level overclocking backend");
	gtk_tooltips_set_tip(GTK_TOOLTIPS(settings->tips), settings->chk_backend, \
		"By default NVClock uses Coolbits for overclocking of GeforceFX/6/7 cards but " \
		"NVClock also offers a low-level backend. The advantage of Coolbits is that it can " \
		"handle 2D/3D clocks which are used on some of these cards. In some cases you might " \
		"want to use the low-level backend. Using this option you can use it but note that " \
		"Coolbits is likely more stable at the moment.", NULL);
	gtk_box_pack_start(GTK_BOX(settings->vbox), settings->chk_backend, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->chk_backend), conf->use_lowlevel_backend);
	g_signal_connect(GTK_TOGGLE_BUTTON(settings->chk_backend), "toggled", G_CALLBACK (chk_backend_toggled), conf);
#endif

	gtk_widget_show_all(GTK_WIDGET(settings));
	return GTK_WIDGET (settings);
}


int gui_settings_init(config *conf, GtkTooltips *tips)
{
	GtkTreeIter grandparent;
	GtkWidget *settings = nv_settings_new(conf, tips);
	add(&grandparent, NULL, "Settings", BANNER_SETTINGS, settings);

	return 1;
}

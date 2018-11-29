/* NVClock 0.8 - Linux overclocker for NVIDIA cards
 *
 * Copyright(C) 2001-2007 Roderick Colenbrander
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
 * Hardware page containing card info, hardware monitoring, overclocking ..
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "interface.h"
#include "hw.h"

#include "nvclock.h"
#ifdef HAVE_NVCONTROL
#include "nvcontrol.h"
#endif

void reset_speeds(GtkButton *button, gpointer data);
gboolean update_speeds(GtkWidget *widget);
void test_speeds(gpointer data);
gboolean thermal_update(NVThermal *thermal);
gboolean timeout_callback(gpointer data);

GType nv_agp_get_type (void)
{
	static GType agp_type = 0;

	if (!agp_type)
	{
		static const GTypeInfo agp_info =
		{
			sizeof (NVAgpClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			NULL,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (NVAgp),
			0,		/* n_preallocs */
			NULL		/* instance_init */
		};

		agp_type = g_type_register_static (GTK_TYPE_VBOX, "NVAgp",
			&agp_info, 0);
	}

	return agp_type;
}


GType nv_bios_get_type (void)
{
	static GType bios_type = 0;

	if (!bios_type)
	{
		static const GTypeInfo bios_info =
		{
			sizeof (NVBiosClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			NULL,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (NVBios),
			0,		/* n_preallocs */
			NULL		/* instance_init */
		};

		bios_type = g_type_register_static (GTK_TYPE_VBOX, "NVBios",
			&bios_info, 0);
	}

	return bios_type;
}


GType nv_info_get_type (void)
{
	static GType info_type = 0;

	if (!info_type)
	{
		static const GTypeInfo info =
		{
			sizeof (NVInfoClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			NULL,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (NVInfo),
			0,		/* n_preallocs */
			NULL		/* instance_init */
		};

		info_type = g_type_register_static (GTK_TYPE_VBOX, "NVInfo",
			&info, 0);
	}

	return info_type;
}


GType nv_overclock_get_type (void)
{
	static GType overclock_type = 0;

	if (!overclock_type)
	{
		static const GTypeInfo overclock_info =
		{
			sizeof (NVOverclockClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			NULL,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (NVOverclock),
			0,		/* n_preallocs */
			NULL		/* instance_init */
		};

		overclock_type = g_type_register_static (GTK_TYPE_VBOX, "NVOverclock",
			&overclock_info, 0);
	}

	return overclock_type;
}


GType nv_pipeline_get_type (void)
{
	static GType pipeline_type = 0;

	if (!pipeline_type)
	{
		static const GTypeInfo pipeline_info =
		{
			sizeof (NVPipelineClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			NULL,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (NVPipeline),
			0,		/* n_preallocs */
			NULL		/* instance_init */
		};

		pipeline_type = g_type_register_static (GTK_TYPE_VBOX, "NVPipeline",
			&pipeline_info, 0);
	}

	return pipeline_type;
}


GType nv_shader_get_type (void)
{
	static GType shader_type = 0;

	if (!shader_type)
	{
		static const GTypeInfo shader_info =
		{
			sizeof (NVShaderClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			NULL,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (NVShader),
			0,		/* n_preallocs */
			NULL		/* instance_init */
		};

		shader_type = g_type_register_static (GTK_TYPE_VBOX, "NVShader",
			&shader_info, 0);
	}

	return shader_type;
}


GType nv_thermal_get_type (void)
{
	static GType thermal_type = 0;

	if (!thermal_type)
	{
		static const GTypeInfo thermal_info =
		{
			sizeof (NVThermalClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			NULL,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (NVThermal),
			0,		/* n_preallocs */
			NULL		/* instance_init */
		};

		thermal_type = g_type_register_static (GTK_TYPE_VBOX, "NVThermal",
			&thermal_info, 0);
	}

	return thermal_type;
}


void change_fanspeed(GtkButton *button, gpointer data)
{
	NVThermal *thermal = NV_THERMAL(data);

	float fanspeed = gtk_range_get_value(GTK_RANGE((thermal->scale_dutycycle)));

	if(thermal->card->number != nv_card->number)
		set_card(thermal->card->number);

	/* First process cards with 'advanced' sensors as both I2C_/GPU_FANSPEED_MONITORING are set */
	if(nv_card->caps & I2C_FANSPEED_MONITORING)
	{
		nv_card->set_i2c_fanspeed_pwm(nv_card->sensor, fanspeed);
	}
	else if(nv_card->caps & GPU_FANSPEED_MONITORING)
	{
		nv_card->set_fanspeed(fanspeed);
	}
}


/* This function overclocks the card and updates the speeds on the gui. */
void change_speeds(GtkButton *button, gpointer data)
{
	NVOverclock *overclock = NV_OVERCLOCK(data);

	if(overclock->card->number != nv_card->number)
		set_card(overclock->card->number);

	if((int)nv_card->get_gpu_speed() != gtk_range_get_value(GTK_RANGE((overclock->scale_gpu))))
	{
		gint nvclk = gtk_range_get_value(GTK_RANGE((overclock->scale_gpu)));
		nv_card->set_gpu_speed(nvclk);
	}

	if((int)nv_card->get_memory_speed() != gtk_range_get_value(GTK_RANGE((overclock->scale_mem))))
	{
		gint memclk = gtk_range_get_value(GTK_RANGE((overclock->scale_mem)));
		nv_card->set_memory_speed(memclk);
	}

	overclock->clock_changes=0;
	update_speeds(data);

	/* Let the user decide if the speeds work fine */
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(overclock->chk_test_speeds)))
	{
		test_speeds(data);
	}
}


void enable_overclocking(GtkToggleButton *button, NVOverclock *overclock)
{
	char *section = g_strdup_printf("hw%d", overclock->card->number);
	change_entry((cfg_entry**)&overclock->conf->cfg, section, "enable_overclocking", 1);
	if(gtk_toggle_button_get_active(button))
	{
		GtkWidget *dialog;

		/* In some cases We need to ask the user if he really wants to enable overclocking */
		if(nv_card->gpu == MOBILE)
		{
			dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, "%s", "Overclocking on Laptops is more dangerous than normal overclocking because they are more sensitive to heat.\n\nAre you sure you want to enable overclocking?");
		}
		else if((nv_card->arch & (NV3X | NV4X | NV5X)) && nv_card->gpu == DESKTOP && !(nv_card->caps & COOLBITS_OVERCLOCKING))
		{
			dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, "%s", "NVClock supports GeforceFX/6/7 overclocking through a low-level backend and through the Nvidia drivers (Coolbits).\nThe low-level backend should work in all cases but can give issues if your card uses different clocks in 2D and 3D. The Coolbits backend on the other hand supports 2D/3D clocks but requires Nvidia driver 1.0-7664. The Coolbits backend can't be used on your system right now either because the driver is too old or the option isn't enabled in the X config file.\n\nAre you sure you want to use the low-level backend?");
		}
		else
		{
			change_entry((cfg_entry**)&overclock->conf->cfg, section, "enable_overclocking", 1);
			gtk_widget_set_sensitive(overclock->vbox, TRUE);
			return;
		}

		int res = gtk_dialog_run(GTK_DIALOG(dialog));
		switch(res)
		{
			case GTK_RESPONSE_YES:
				change_entry((cfg_entry**)&overclock->conf->cfg, section, "enable_overclocking", 1);
				gtk_widget_set_sensitive(overclock->vbox, TRUE);
				break;
			case GTK_RESPONSE_NO:
			case GTK_RESPONSE_DELETE_EVENT:
				change_entry((cfg_entry**)&overclock->conf->cfg, section, "enable_overclocking", 0);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(overclock->chk_overclock), FALSE);
		}

		g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
		gtk_widget_destroy(dialog);
	}
	else
	{
		change_entry((cfg_entry**)&overclock->conf->cfg, section, "enable_overclocking", 0);
		gtk_widget_set_sensitive(overclock->vbox, FALSE);

#ifdef HAVE_NVCONTROL
		if((nv_card->caps & COOLBITS_OVERCLOCKING) && (nv_card->state != STATE_LOWLEVEL))
		{
			NVSetAttribute(overclock->dpy, 0, 0, NV_GPU_OVERCLOCKING_STATE, 0);
		}
#endif
	}
}


void enable_fanspeed_adjustments(GtkToggleButton *button, NVThermal *thermal)
{
	char *section = g_strdup_printf("hw%d", thermal->card->number);
	cfg_entry *entry = lookup_entry((cfg_entry**)&thermal->conf->cfg, section, "enable_fanspeed_adjustments");

	if(gtk_toggle_button_get_active(button))
	{
		GtkWidget *dialog;
		if(entry)
		{
			/* The user enabled fanspeed adjustments during a previous session, so don't show any dialogs */
			if(entry->value)
				return;
		}

		dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, "%s", "Adjusting the fanspeed of your videocard is dangerous and should only be done when you know what you are doing.\nAre you sure you want to enable fanspeed adjustments?");

		int res = gtk_dialog_run(GTK_DIALOG(dialog));
		switch(res)
		{
			case GTK_RESPONSE_YES:
				change_entry((cfg_entry**)&thermal->conf->cfg, section, "enable_fanspeed_adjustments", 1);
				gtk_widget_set_sensitive(thermal->vbx_dutycycle, TRUE);
				break;
			case GTK_RESPONSE_NO:
			case GTK_RESPONSE_DELETE_EVENT:
				change_entry((cfg_entry**)&thermal->conf->cfg, section, "enable_fanspeed_adjustments", 0);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(thermal->chk_fanspeed), FALSE);
		}

		g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
		gtk_widget_destroy(dialog);
	}
	else
	{
		if(nv_card->caps & I2C_AUTOMATIC_FANSPEED_CONTROL)
			nv_card->set_i2c_fanspeed_mode(nv_card->sensor, 0); /* Put the card back into automatic mode */

		change_entry((cfg_entry**)&thermal->conf->cfg, section, "enable_fanspeed_adjustments", 0);
		gtk_widget_set_sensitive(thermal->vbx_dutycycle, FALSE);
	}
	free(section);
}


void reset_speeds(GtkButton *button, gpointer data)
{
	NVOverclock *overclock = NV_OVERCLOCK(data);

	if(overclock->card->number != nv_card->number)
		set_card(overclock->card->number);

	nv_card->reset_gpu_speed();
	nv_card->reset_memory_speed();

	update_speeds(data);
}


/* Show a dialog using which the user can test the new clocks. If the clocks don't work
/  correctly the old ones will be restored.
*/
void test_speeds(gpointer data)
{
	GtkWidget *label;
	Timeout *timeout;
	timeout = g_malloc(sizeof(Timeout));

	timeout->dialog = GTK_DIALOG(gtk_dialog_new_with_buttons("Testing the new speeds",
		GTK_WINDOW(NULL), GTK_DIALOG_MODAL, GTK_STOCK_YES, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_NO, GTK_RESPONSE_REJECT, NULL));

	label = gtk_label_new("Are the new speeds working correctly?");
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(timeout->dialog)->vbox), label);
	gtk_widget_show(label);

	timeout->time = 5;
	timeout->label = gtk_label_new("Timeout in: 5 second(s)");
	gtk_container_add(GTK_CONTAINER(timeout->dialog->vbox), timeout->label);
	gtk_widget_show(GTK_WIDGET(timeout->label));

	/* Create the real timeout */
	timeout->timeout_id = g_timeout_add(1000, timeout_callback, timeout);

	gint result = gtk_dialog_run(GTK_DIALOG(timeout->dialog));

	/* Stop the timer because we got an answer back */
	g_source_remove(timeout->timeout_id);

	/* We receive ACCEPT when the Yes button was pressed. In all other cases it is REJECT */
	switch(result)
	{
		case GTK_RESPONSE_DELETE_EVENT:
		case GTK_RESPONSE_REJECT:
			/* Restore the default speeds */
			reset_speeds(NULL, data);
			break;
	}
	gtk_widget_destroy(GTK_WIDGET(timeout->dialog));
	g_free(timeout);
}


/* This function updates the speeds on the gui. */
int update_speeds(GtkWidget *widget)
{
	NVOverclock *overclock = NV_OVERCLOCK(widget);

	/* When the widget isn't visible anymore, return FALSE to stop the calling of this function */
	if(!GTK_WIDGET_MAPPED(overclock))
	{
		overclock->clock_changes = 0; /* Make sure the sliders aren't locked the next time we are visible */
		return FALSE;
	}

	if(overclock->card->number != nv_card->number)
		set_card(overclock->card->number);

	if(overclock->clock_changes)
		return TRUE;

	overclock->clock_changes=0;
	overclock->nvclk = nv_card->get_gpu_speed();
	overclock->memclk = nv_card->get_memory_speed();

	/* When the current clocks don't match the ones on the gui, update them */
	if(overclock->nvclk != gtk_range_get_value(GTK_RANGE(overclock->scale_gpu)))
	{
		gtk_range_set_value(GTK_RANGE((overclock->scale_gpu)), overclock->nvclk);
	}

	if(overclock->memclk != gtk_range_get_value(GTK_RANGE(overclock->scale_mem)))
	{
		gtk_range_set_value(GTK_RANGE((overclock->scale_mem)), overclock->memclk);
	}
	return TRUE;
}


void chk_test_speeds_toggled(GtkToggleButton *button, gpointer data)
{
	NVOverclock *overclock = NV_OVERCLOCK(data);
	char *section = g_strdup_printf("hw%d", overclock->card->number);
	change_entry(overclock->conf->cfg, section, "test_speeds", gtk_toggle_button_get_active(button));
}


/* When the overclock page is shown; update the clocks every second which is usefull for GeforceFX/Geforce6600GT cards
/  as for those the clocks change between 2d and 3d.
*/
void nv_overclock_mapped(GtkWidget *widget)
{
	NVOverclock *overclock = NV_OVERCLOCK(widget);

	if(overclock->card->number != nv_card->number)
		set_card(overclock->card->number);

	/* When the use_lowlevel_backend option is set, force LOWLEVEL */
	if(overclock->conf->use_lowlevel_backend && overclock->combo_speeds)
	{
		nv_card->set_state(STATE_LOWLEVEL);
		gtk_widget_set_sensitive(overclock->combo_speeds, 0);
	}
	/* When lowlevel is disabled, restore the correct state */
	else if((nv_card->caps & COOLBITS_OVERCLOCKING) && !overclock->conf->use_lowlevel_backend && nv_card->state == STATE_LOWLEVEL)
	{
		gtk_widget_set_sensitive(overclock->combo_speeds, 1);

		switch(gtk_combo_box_get_active(GTK_COMBO_BOX(overclock->combo_speeds)))
		{
			case 0:
				nv_card->set_state(STATE_2D);
				break;
			case 1:
				nv_card->set_state(STATE_3D);
				break;
			case 2:
				nv_card->set_state(STATE_BOTH);
				break;
		}
	}

	g_timeout_add(1000, (GSourceFunc)update_speeds, overclock);
}


gboolean timeout_callback(gpointer data)
{
	Timeout *timeout = (Timeout*)data;
	GtkLabel *label = GTK_LABEL(timeout->label);

	timeout->time--;

	/* When we ran out of time simulate "No" */
	if(timeout->time == 0)
	{
		gtk_dialog_response(timeout->dialog, GTK_RESPONSE_REJECT);
		return FALSE;
	}

	/* Refresh the time */
	gtk_label_set_text(label, g_strdup_printf("Timeout in: %d second(s)", timeout->time));

	return TRUE;
}


gboolean thermal_update(NVThermal *thermal)
{
	int core, ambient;
	gchar *tmp;

	/* When the widget isn't visible anymore, return FALSE to stop the calling of this function */
	if(!GTK_WIDGET_MAPPED(thermal))
		return FALSE;

	if(thermal->card->number != nv_card->number)
		set_card(thermal->card->number);

	core = nv_card->get_gpu_temp(nv_card->sensor);

	/* The progressbar can be set to values between 0 and 1. In the past its range could be set by
	/  accessing its adjustment but that has been deprecated. To create a bar that goes from lets say
	/  25 to 110 we can, we need to have 25 correspond with 0 and 110 with 1. The first step is to substract
	/  25 from the value. Next 110-30 gives 85, to normalize that to 1 we need to divide by 85. The reason for
	/  a range upto 110 is that because of driver bugs the Geforce6 can easily reach 100C.
	*/
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(thermal->pbar_core), ((float)core-25)/85);
	tmp = g_strdup_printf("%d C", core);
	gtk_label_set_text(GTK_LABEL(thermal->lbl_core), tmp);
	g_free(tmp);

	if(nv_card->caps & BOARD_TEMP_MONITORING)
	{
		ambient = nv_card->get_board_temp(nv_card->sensor);
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(thermal->pbar_ambient), ((float)ambient-25)/85);
		tmp = g_strdup_printf("%d C", ambient);
		gtk_label_set_text(GTK_LABEL(thermal->lbl_ambient), tmp);
		g_free(tmp);
	}

	/* Quit when there's no controllable fan */
	if(!(nv_card->caps & (GPU_FANSPEED_MONITORING | I2C_FANSPEED_MONITORING)))
		return TRUE;

	/* Only update the scale when it doesn't have focus else you can never adjust the fanspeed */
	if(GTK_WIDGET_HAS_FOCUS(thermal->scale_dutycycle))
	{
		/* Once the scale loses focus, the scale is quickly updated with the new value.
		/  To allow fanspeed adjustments wait 'timeout' seconds before updating the sliders
		/  else the user needs to click on Apply very quickly ;)
		*/
		thermal->timeout=3;
		return TRUE;
	}
	if(thermal->timeout)
	{
		thermal->timeout--;
		return TRUE;
	}

	/* First process cards with 'advanced' sensors as both I2C_/GPU_FANSPEED_MONITORING are set */
	if(nv_card->caps & I2C_FANSPEED_MONITORING)
	{
		int fanspeed = nv_card->get_i2c_fanspeed_rpm(nv_card->sensor);

		/* Set the fanspeed in the progress bar from 2000 to 6000 in RPM */
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(thermal->pbar_fanspeed), ((float)fanspeed-2000)/4000);

		/* Set the fanspeed in RPM */
		tmp = g_strdup_printf("%d RPM", fanspeed);
		gtk_label_set_text(GTK_LABEL(thermal->lbl_fanspeed), tmp);
		g_free(tmp);

		gtk_range_set_value(GTK_RANGE((thermal->scale_dutycycle)), (float)nv_card->get_i2c_fanspeed_pwm(nv_card->sensor));
	}
	else if(nv_card->caps & GPU_FANSPEED_MONITORING)
	{
		gtk_range_set_value(GTK_RANGE((thermal->scale_dutycycle)), (float)nv_card->get_fanspeed());
	}
	return TRUE;
}


void nv_thermal_mapped(GtkWidget *thermal)
{
	thermal_update(NV_THERMAL(thermal));
	g_timeout_add(1000, (GSourceFunc)thermal_update, thermal);
}


void combo_speeds_changed(GtkComboBox *combo, gpointer data)
{
	switch(gtk_combo_box_get_active(combo))
	{
		case 0:
			nv_card->set_state(STATE_2D);
			break;
		case 1:
			nv_card->set_state(STATE_3D);
			break;
		case 2:
			nv_card->set_state(STATE_BOTH);
			break;
	}

	NV_OVERCLOCK(data)->clock_changes = 0; /* Force the clocks to get updated */
}


/* When the gpu slider gets moved, set a flag that there are clock changes, so that our
/  refresh function won't update the clocks.
*/
void scale_gpu_changed(GtkToggleButton *button, gpointer data)
{
	NVOverclock *overclock = NV_OVERCLOCK(data);

	if(overclock->nvclk != gtk_range_get_value(GTK_RANGE((overclock->scale_gpu))))
		overclock->clock_changes = 1;
}


/* When the memory slider gets moved, set a flag that there are clock changes, so that our
/  refresh function won't update the clocks.
*/
void scale_mem_changed(GtkToggleButton *button, gpointer data)
{
	NVOverclock *overclock = NV_OVERCLOCK(data);

	if(overclock->memclk != gtk_range_get_value(GTK_RANGE((overclock->scale_mem))))
		overclock->clock_changes = 1;
}


gchar* value_to_mhz(GtkRange *range, gdouble value, int option)
{
	return g_strdup_printf("%.3f MHz ", value);
}


gchar* value_to_percent(GtkRange *range, gdouble value, int option)
{
	return g_strdup_printf("%.1f%%", value);
}


gchar* value_to_rpm(GtkRange *range, gdouble value, int option)
{
	return g_strdup_printf("%.1f RPM", value);
}


void set_agp_info(GtkWidget *widget)
{
	char *agprate;
	NVAgp *agp = NV_AGP(widget);

	if(agp->card->number != nv_card->number)
		set_card(agp->card->number);

	gtk_label_set_label(GTK_LABEL(agp->lbl_agp_txt), nv_card->get_agp_status());
	agprate = g_strdup_printf("%dX", nv_card->get_bus_rate());
	gtk_label_set_label(GTK_LABEL(agp->lbl_agprate_txt), agprate);
	gtk_label_set_label(GTK_LABEL(agp->lbl_agprates_txt), nv_card->get_agp_supported_rates());
	gtk_label_set_label(GTK_LABEL(agp->lbl_fw_txt), nv_card->get_agp_fw_status());
	gtk_label_set_label(GTK_LABEL(agp->lbl_sba_txt), nv_card->get_agp_sba_status());

	g_free(agprate);
}


/* Sets all Nvidia bios info */
void set_bios_info(GtkWidget *widget)
{
	char *tmp;
	int i;
	NVBios *bios = NV_BIOS(widget);
	GtkTreeIter iter;

	gtk_label_set_label(GTK_LABEL(bios->lbl_signonmsg_txt), nv_card->bios->signon_msg);

	for(i=0; i<nv_card->bios->perf_entries; i++)
	{
		int column = 0;
		gchar *nvclk, *memclk;
		
		if(nv_card->bios->perf_lst[i].delta)
			nvclk = g_strdup_printf("%d(+%d) MHz", nv_card->bios->perf_lst[i].nvclk, nv_card->bios->perf_lst[i].delta);
		else
			nvclk = g_strdup_printf("%d MHz", nv_card->bios->perf_lst[i].nvclk);
		memclk = g_strdup_printf("%d MHz", nv_card->bios->perf_lst[i].memclk);

		gtk_tree_store_append(bios->store_perf, &iter, NULL);
		gtk_tree_store_set(bios->store_perf, &iter, 0 /* column 0 */, nvclk, -1);
		column++;

		if(nv_card->arch & NV5X)
		{
			gchar *shaderclk = g_strdup_printf("%d MHz", nv_card->bios->perf_lst[i].shaderclk);
			gtk_tree_store_set(bios->store_perf, &iter, column, shaderclk, -1 );
			g_free(shaderclk);
			column++;
		}

		gtk_tree_store_set(bios->store_perf, &iter, column, memclk, -1 );
		column++;

		if(nv_card->bios->volt_entries)
		{
			gchar *voltage = g_strdup_printf("%.2f V", nv_card->bios->perf_lst[i].voltage);
			gtk_tree_store_set(bios->store_perf, &iter, column, voltage, -1 );
			g_free(voltage);
			column++;
		}
		if(nv_card->bios->perf_lst[0].fanspeed)
		{
			gchar *fanspeed = g_strdup_printf("%d%%", nv_card->bios->perf_lst[i].fanspeed);
			gtk_tree_store_set(bios->store_perf, &iter, column, fanspeed, -1 );
			g_free(fanspeed);
			column++;
		}
		g_free(nvclk);
		g_free(memclk);
	}

	if(nv_card->bios->volt_entries)
		gtk_label_set_label(GTK_LABEL(bios->lbl_volt), "Voltage Levels:");

	for(i=0; i<nv_card->bios->volt_entries; i++)
	{
		gchar *voltage, *vid;
		
		voltage = g_strdup_printf("%.2f V", nv_card->bios->volt_lst[i].voltage);
		vid = g_strdup_printf("%d", nv_card->bios->volt_lst[i].VID);

		gtk_tree_store_append(bios->store_volt, &iter, NULL);
		gtk_tree_store_set(bios->store_volt, &iter, 0 /* column 0 */, voltage, 1 /* column 1 */, vid, -1 );

		g_free(voltage);
		g_free(vid);
	}
}


/* This function sets overclocking info like the min/max values of the sliders. */
void set_overclock_info(GtkWidget *widget)
{
	NVOverclock *overclock = NV_OVERCLOCK(widget);
	GtkAdjustment *adj_core = gtk_range_get_adjustment(GTK_RANGE(overclock->scale_gpu));
	GtkAdjustment *adj_mem = gtk_range_get_adjustment(GTK_RANGE(overclock->scale_mem));

	char *section = g_strdup_printf("hw%d", overclock->card->number);
	cfg_entry *entry;
	int overclocking = 0;

	if((entry = lookup_entry((cfg_entry**)&overclock->conf->cfg, section, "enable_overclocking")))
	{
		overclocking = entry->value;
	}

	/* Force lowlevel backend if it was set in the config file */
	if(overclock->conf->use_lowlevel_backend && overclock->combo_speeds)
	{
		nv_card->set_state(STATE_LOWLEVEL);
		gtk_widget_set_sensitive(overclock->combo_speeds, 0);
	}
	else if(nv_card->caps & COOLBITS_OVERCLOCKING)
		nv_card->set_state(STATE_2D); /* Default to 2D for the combo */

	/* Disable overclocking for some types of hardware.
	/  By default:
	/  - Don't overclock mobile GPUs because it is more dangerous on this type of hardware
	/  - Allow only gpu overclocking on Nforce boards because the memory is system memory which we can't overclock
	*/
	if(nv_card->gpu == NFORCE)
		gtk_widget_set_sensitive(overclock->scale_mem, 0);
	/* In case of GeforceFX/Geforce6 cards we use NV-CONTROL for now as it allows us to set 2d/3d clocks. */
	if(nv_card->caps & COOLBITS_OVERCLOCKING)
	{
		int res=0;
#ifdef HAVE_NVCONTROL
		int overclocking_enabled;
		overclock->have_coolbits = NVGetAttribute(overclock->dpy, 0, 0, NV_GPU_OVERCLOCKING_STATE, &overclocking_enabled);

		/* Enable overclocking when it was enabled in the config file */
		if(overclocking && overclock->have_coolbits && !overclocking_enabled)
		{
			NVSetAttribute(overclock->dpy, 0, 0, NV_GPU_OVERCLOCKING_STATE, 1);
			NVGetAttribute(overclock->dpy, 0, 0, NV_GPU_OVERCLOCKING_STATE, &overclocking_enabled);
		}
		/* Disable overclocking when it was disabled in the config file */
		else if(!overclocking && overclock->have_coolbits && overclocking_enabled)
		{
			NVSetAttribute(overclock->dpy, 0, 0, NV_GPU_OVERCLOCKING_STATE, 0);
			NVGetAttribute(overclock->dpy, 0, 0, NV_GPU_OVERCLOCKING_STATE, &overclocking_enabled);
		}

		if(overclock->have_coolbits && overclocking_enabled)
		{
			gtk_widget_set_sensitive(overclock->vbox, TRUE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(overclock->chk_overclock), TRUE);
			res = 1;
		}
#else
		overclock->have_coolbits = 0;
#endif
		if(!res)
		{
			gtk_widget_set_sensitive(overclock->vbox, FALSE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(overclock->chk_overclock), FALSE);
		}
	}
	/* Disable mobile overclocking unless 'overclocking' is set */
	else if(((nv_card->gpu == MOBILE && !overclocking) || (nv_card->caps == 0)))
	{
		gtk_widget_set_sensitive(overclock->vbox, FALSE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(overclock->chk_overclock), FALSE);
	}
	else
	{
		gtk_widget_set_sensitive(overclock->vbox, TRUE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(overclock->chk_overclock), TRUE);
	}

	adj_mem->lower = nv_card->memclk_min;
	adj_mem->upper = nv_card->memclk_max;

	adj_core->lower = nv_card->nvclk_min;
	adj_core->upper = nv_card->nvclk_max;

	if((entry = lookup_entry((cfg_entry**)&overclock->conf->cfg, section, "test_speeds")))
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(overclock->chk_test_speeds), entry->value);
	}

	/* Show the speeds on the gui; we can't use update_speeds as it doesn't work on invisible windows. (related to the update thread) */
	gtk_range_set_value(GTK_RANGE((overclock->scale_gpu)), nv_card->get_gpu_speed());
	gtk_range_set_value(GTK_RANGE((overclock->scale_mem)), nv_card->get_memory_speed());
}


enum pipe_columns
{
	BIT_COLUMN,
	DESC_COLUMN,
	STATE_COLUMN,
	MASKED_COLUMN,
	N_PIPE_COLUMNS
};

void set_pipeline_info(GtkWidget *widget)
{
	NVPipeline *pipeline = NV_PIPELINE(widget);

	char *bit, *config, *desc, *masked, *state, *units;
	char pmask, vmask;
	int mask = 0; /* Mask containing all available units */
	int hw_masked = 0; /* HW masked units */
	int hw_state = 0; /* Currently activated units */
	int punits, total; /* Number of pixel units and total number of pixel units per pipeline */
	int i;
	GtkTreeIter iter;

	mask = nv_card->get_default_mask(&pmask, &vmask);
	if(nv_card->get_hw_masked_units(&pmask, &vmask))
		hw_masked = (vmask << 8) | pmask;

	punits = nv_card->get_pixel_pipelines(&pmask, &total);
	units = g_strdup_printf("Pixel: %dx%d\tVertex: %dx1", total, punits, nv_card->get_vertex_pipelines(&vmask));
	gtk_label_set_text(GTK_LABEL(pipeline->lbl_pipes_txt), units);
	g_free(units);

	hw_state = (vmask << 8) | pmask;
	config = g_strdup_printf("0x%04X", hw_state);
	gtk_label_set_text(GTK_LABEL(pipeline->lbl_config_txt), config);
	g_free(config);

	for(i=0; i<16; i++)
	{
		bit = g_strdup_printf("%d", i);

		if((i < 8) && (mask & 1<<i))
		{
			desc = g_strdup_printf("Pixel unit %d", i);
			if(hw_masked & 1<<i)
				masked = g_strdup_printf("Yes");
			else
				masked = g_strdup_printf("-");

			if(hw_state & 1<<i)
				state = g_strdup_printf("Enabled");
			else
				state = g_strdup_printf("Disabled");
		}
		else if(i < 8)
		{
			desc = g_strdup_printf("-");
			masked = g_strdup_printf("-");
			state = g_strdup_printf("");
		}

		if((i >= 8) && (mask & 1<<i))
		{
			desc = g_strdup_printf("Vertex unit %d", i-8);
			if(hw_masked & 1<<i)
				masked = g_strdup_printf("Yes");
			else
				masked = g_strdup_printf("-");

			if(hw_state & 1<<i)
				state = g_strdup_printf("Enabled");
			else
				state = g_strdup_printf("Disabled");
		}
		else if(i >= 8)
		{
			desc = g_strdup_printf("-");
			masked = g_strdup_printf("-");
			state = g_strdup_printf("");
		}
		gtk_tree_store_append(pipeline->store, &iter, NULL);
		gtk_tree_store_set(pipeline->store, &iter,
			BIT_COLUMN, bit,
			DESC_COLUMN, desc,
			STATE_COLUMN, state,
			MASKED_COLUMN, masked,
			-1 );
		g_free(bit);
		g_free(desc);
		g_free(state);
		g_free(masked);
	}
}


void set_shader_info(GtkWidget *widget)
{
	NVShader *shader = NV_SHADER(widget);
	char *clock_txt, *config_txt, *ropunits_txt, *streamunits_txt, rmask, smask, rmask_default, smask_default;
	short ropunits, streamunits;

	clock_txt = g_strdup_printf("%0.3f MHz", nv_card->get_shader_speed());
	gtk_label_set_text(GTK_LABEL(shader->lbl_clock_txt), clock_txt);
	g_free(clock_txt);

	streamunits = nv_card->get_stream_units(&smask, &smask_default);
	streamunits_txt = g_strdup_printf("%d", streamunits);
	gtk_label_set_text(GTK_LABEL(shader->lbl_streamunits_txt), streamunits_txt);
	g_free(streamunits_txt);

	ropunits = nv_card->get_rop_units(&rmask, &rmask_default);
	ropunits_txt = g_strdup_printf("%d", ropunits);
	gtk_label_set_text(GTK_LABEL(shader->lbl_ropunits_txt), ropunits_txt);
	g_free(ropunits_txt);

	config_txt = g_strdup_printf("0x%06X", rmask << 16 | smask);
	gtk_label_set_text(GTK_LABEL(shader->lbl_config_txt), config_txt);
	g_free(config_txt);
}


void set_thermal_info(GtkWidget *widget)
{
	NVThermal *thermal = NV_THERMAL(widget);
	char *section = g_strdup_printf("hw%d", thermal->card->number);
	cfg_entry *entry;

	gtk_label_set_label(GTK_LABEL(thermal->lbl_sensor_txt), nv_card->sensor_name);

	if(nv_card->caps & (GPU_FANSPEED_MONITORING | I2C_FANSPEED_MONITORING))
	{
		if((entry = lookup_entry((cfg_entry**)&thermal->conf->cfg, section, "enable_fanspeed_adjustments")))
		{
			gtk_widget_set_sensitive(thermal->vbx_dutycycle, entry->value);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(thermal->chk_fanspeed), entry->value);
		}
		else
		{
			gtk_widget_set_sensitive(thermal->vbx_dutycycle, FALSE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(thermal->chk_fanspeed), FALSE);
		}
	}
}


void set_videocard_info(GtkWidget *widget)
{
	char *arch, *bios, *vidmem, *irq, *mem_type;
	NVInfo *info = NV_INFO(widget);

	if(info->card->number != nv_card->number)
		set_card(info->card->number);

	gtk_label_set_label(GTK_LABEL(info->lbl_gpu_txt), nv_card->card_name);

	arch = g_strdup_printf("NV%X %X", nv_card->get_gpu_architecture(), nv_card->get_gpu_revision());
	gtk_label_set_label(GTK_LABEL(info->lbl_arch_txt), arch);

	/* Bios dumping doesn't allways work correctly */
	if(nv_card->bios)
		gtk_label_set_label(GTK_LABEL(info->lbl_bios_txt), nv_card->bios->version);
	else
	{
		bios = g_strdup_printf("Unknown");
		gtk_label_set_label(GTK_LABEL(info->lbl_bios_txt), bios);
		g_free(bios);
	}

	vidmem = g_strdup_printf("%d MB", nv_card->get_memory_size());
	gtk_label_set_label(GTK_LABEL(info->lbl_vidmem_txt), vidmem);

	if(strcmp(nv_card->get_bus_type(), "PCI-Express") == 0)
	{
		gchar* bus_type = g_strdup_printf("%s %dX", nv_card->get_bus_type(), nv_card->get_bus_rate());
		gtk_label_set_label(GTK_LABEL(info->lbl_bustype_txt), bus_type);
		g_free(bus_type);
	}
	else
		gtk_label_set_label(GTK_LABEL(info->lbl_bustype_txt), nv_card->get_bus_type());

	mem_type = g_strdup_printf("%d bit %s", nv_card->get_memory_width(), nv_card->get_memory_type());
	gtk_label_set_label(GTK_LABEL(info->lbl_memtype_txt), mem_type);
	irq = g_strdup_printf("%d", nv_card->irq);
	gtk_label_set_label(GTK_LABEL(info->lbl_irq_txt), irq);

	g_free(arch);
	g_free(vidmem);
	g_free(irq);
	g_free(mem_type);
}


GtkWidget* nv_agp_new (NVCard *card)
{
	NVAgp *agp = g_object_new (NV_TYPE_AGP, NULL);
	agp->card = card;
	agp->table = gtk_table_new(4,2, FALSE);

	agp->frame = gtk_frame_new("AGP Information");
	gtk_container_add(GTK_CONTAINER(agp->frame), agp->table);

	gtk_table_set_col_spacings(GTK_TABLE(agp->table), 2);
	gtk_table_set_row_spacings(GTK_TABLE(agp->table), 3);

	agp->lbl_agp = gtk_label_new("AGP Status:");
	gtk_misc_set_alignment (GTK_MISC (agp->lbl_agp), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(agp->table), GTK_WIDGET(agp->lbl_agp), 0, 1, 0, 1,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	agp->lbl_agp_txt = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (agp->lbl_agp_txt), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(agp->table), GTK_WIDGET(agp->lbl_agp_txt), 1, 2, 0, 1,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	agp->lbl_agprate = gtk_label_new("AGP rate:");
	gtk_misc_set_alignment (GTK_MISC (agp->lbl_agprate), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(agp->table), GTK_WIDGET(agp->lbl_agprate), 0, 1, 1, 2,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	agp->lbl_agprate_txt = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (agp->lbl_agprate_txt), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(agp->table), GTK_WIDGET(agp->lbl_agprate_txt), 1, 2, 1, 2,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	agp->lbl_agprates = gtk_label_new("Supported AGP Rates:");
	gtk_misc_set_alignment (GTK_MISC (agp->lbl_agprates), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(agp->table), GTK_WIDGET(agp->lbl_agprates), 0, 1, 2, 3,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	agp->lbl_agprates_txt = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (agp->lbl_agprates_txt), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(agp->table), GTK_WIDGET(agp->lbl_agprates_txt), 1, 2, 2, 3,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	agp->lbl_fw = gtk_label_new("Fast Writes:");
	gtk_misc_set_alignment (GTK_MISC (agp->lbl_fw), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(agp->table), GTK_WIDGET(agp->lbl_fw), 0, 1, 3, 4,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	agp->lbl_fw_txt = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (agp->lbl_fw_txt), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(agp->table), GTK_WIDGET(agp->lbl_fw_txt), 1, 2, 3, 4,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	agp->lbl_sba = gtk_label_new("Sideband Addressing:");
	gtk_misc_set_alignment (GTK_MISC (agp->lbl_sba), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(agp->table), GTK_WIDGET(agp->lbl_sba), 0, 1, 4, 5,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	agp->lbl_sba_txt = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (agp->lbl_sba_txt), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(agp->table), GTK_WIDGET(agp->lbl_sba_txt), 1, 2, 4, 5,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	gtk_box_pack_start(GTK_BOX(agp), agp->frame, FALSE, FALSE, 0);
	gtk_widget_show_all(GTK_WIDGET(agp));

	set_agp_info(GTK_WIDGET(agp));

	return GTK_WIDGET (agp);
}


GtkWidget* nv_bios_new (NVCard *card)
{
	int column = 0;
	NVBios *bios = g_object_new (NV_TYPE_BIOS, NULL);
	bios->table = gtk_table_new(3,2, FALSE);

	gtk_table_set_col_spacings(GTK_TABLE(bios->table), 2);
	gtk_table_set_row_spacings(GTK_TABLE(bios->table), 3);

	bios->frame = gtk_frame_new("VideoBios Information");
	gtk_container_add(GTK_CONTAINER(bios->frame), bios->table);

	bios->lbl_signonmsg = gtk_label_new("Title:");
	gtk_misc_set_alignment (GTK_MISC (bios->lbl_signonmsg), 0.02, 0);
	gtk_table_attach (GTK_TABLE(bios->table), GTK_WIDGET(bios->lbl_signonmsg), 0, 1, 0, 1,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	bios->lbl_signonmsg_txt = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (bios->lbl_signonmsg_txt), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(bios->table), GTK_WIDGET(bios->lbl_signonmsg_txt), 1, 2, 0, 1,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	bios->lbl_perf = gtk_label_new("Perf. Levels:");
	gtk_misc_set_alignment (GTK_MISC (bios->lbl_perf), 0.02, 0);
	gtk_table_attach (GTK_TABLE(bios->table), GTK_WIDGET(bios->lbl_perf), 0, 1, 1, 2,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	/* Create a treeview for storing the performance table */
	bios->tree_view_perf = gtk_tree_view_new();
	bios->store_perf = gtk_tree_store_new(5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING );
	gtk_tree_view_set_model(GTK_TREE_VIEW(bios->tree_view_perf), GTK_TREE_MODEL(bios->store_perf));

	/* Create GPU clock column */
	bios->column_perf = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(bios->column_perf, "GPU");
	bios->renderer_perf = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(bios->column_perf, bios->renderer_perf, FALSE);
	gtk_tree_view_column_set_attributes(bios->column_perf, bios->renderer_perf, "text", column, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(bios->tree_view_perf), bios->column_perf);
	column++;

	/* Create Shader clock column for NV5x */
	if(card->arch & NV5X)
	{
		bios->column_perf = gtk_tree_view_column_new();
		gtk_tree_view_column_set_title(bios->column_perf, "Shader");
		bios->renderer_perf = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start(bios->column_perf, bios->renderer_perf, FALSE);
		gtk_tree_view_column_set_attributes(bios->column_perf, bios->renderer_perf, "text", column, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(bios->tree_view_perf), bios->column_perf);
		column++;
	}

	/* Create Memory clock column */
	bios->column_perf = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(bios->column_perf, "Memory");
	bios->renderer_perf = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(bios->column_perf, bios->renderer_perf, FALSE);
	gtk_tree_view_column_set_attributes(bios->column_perf, bios->renderer_perf, "text", column, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(bios->tree_view_perf), bios->column_perf);
	column++;

	/* Create GPU Voltage column if the bios contains voltage entires*/
	if(card->bios->volt_entries)
	{
		bios->column_perf = gtk_tree_view_column_new();
		gtk_tree_view_column_set_title(bios->column_perf, "Voltage");
		bios->renderer_perf = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start(bios->column_perf, bios->renderer_perf, FALSE);
		gtk_tree_view_column_set_attributes(bios->column_perf, bios->renderer_perf, "text", column, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(bios->tree_view_perf), bios->column_perf);
		column++;
	}

	/* Create Fanspeed column if the bios contains duty cycle entires*/
	if(card->bios->perf_lst[0].fanspeed)
	{
		bios->column_perf = gtk_tree_view_column_new();
		gtk_tree_view_column_set_title(bios->column_perf, "Fanspeed");
		bios->renderer_perf = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start(bios->column_perf, bios->renderer_perf, FALSE);
		gtk_tree_view_column_set_attributes(bios->column_perf, bios->renderer_perf, "text", column, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(bios->tree_view_perf), bios->column_perf);
		column++;
	}

	/* Allow an alternating color for the rows in the list */
	g_object_set(bios->tree_view_perf, "rules-hint", TRUE, NULL);
	gtk_table_attach (GTK_TABLE(bios->table), GTK_WIDGET(bios->tree_view_perf), 1, 2, 1, 2,
		(GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);


	bios->lbl_volt = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (bios->lbl_volt), 0.02, 0);
	gtk_table_attach (GTK_TABLE(bios->table), GTK_WIDGET(bios->lbl_volt), 0, 1, 2, 3,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	/* Create a treeview for storing the performance table */
	bios->tree_view_volt = gtk_tree_view_new();
	bios->store_volt = gtk_tree_store_new(N_PIPE_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING );
	gtk_tree_view_set_model(GTK_TREE_VIEW(bios->tree_view_volt), GTK_TREE_MODEL(bios->store_volt));

	/* Create GPU clock column */
	bios->column_volt = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(bios->column_volt, "Voltage");
	bios->renderer_volt = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(bios->column_volt, bios->renderer_volt, FALSE);
	gtk_tree_view_column_set_attributes(bios->column_volt, bios->renderer_volt, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(bios->tree_view_volt), bios->column_volt);

	bios->column_volt = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(bios->column_volt, "VID");
	bios->renderer_volt = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(bios->column_volt, bios->renderer_volt, FALSE);
	gtk_tree_view_column_set_attributes(bios->column_volt, bios->renderer_volt, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(bios->tree_view_volt), bios->column_volt);

	/* Allow an alternating color for the rows in the list */
	g_object_set(bios->tree_view_volt, "rules-hint", TRUE, NULL);
	gtk_table_attach (GTK_TABLE(bios->table), GTK_WIDGET(bios->tree_view_volt), 1, 2, 2, 3,
		(GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	gtk_box_pack_start(GTK_BOX(bios), bios->frame, FALSE, FALSE, 0);
	gtk_widget_show_all(GTK_WIDGET(bios));

	set_bios_info(GTK_WIDGET(bios));

	return GTK_WIDGET (bios);
}


GtkWidget* nv_info_new (NVCard *card)
{
	NVInfo *info = g_object_new (NV_TYPE_INFO, NULL);
	info->card = card;
	info->table = gtk_table_new(7,2, FALSE);

	info->frame = gtk_frame_new("Videocard Information");
	gtk_container_add(GTK_CONTAINER(info->frame), info->table);

	gtk_table_set_col_spacings(GTK_TABLE(info->table), 2);
	gtk_table_set_row_spacings(GTK_TABLE(info->table), 3);

	info->lbl_gpu = gtk_label_new("Graphics Processor:");
	gtk_misc_set_alignment (GTK_MISC (info->lbl_gpu), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(info->table), GTK_WIDGET(info->lbl_gpu), 0, 1, 0, 1,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	info->lbl_gpu_txt = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (info->lbl_gpu_txt), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(info->table), GTK_WIDGET(info->lbl_gpu_txt), 1, 2, 0, 1,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	info->lbl_arch = gtk_label_new("Architecture:");
	gtk_misc_set_alignment (GTK_MISC (info->lbl_arch), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(info->table), GTK_WIDGET(info->lbl_arch), 0, 1, 1, 2,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	info->lbl_arch_txt = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (info->lbl_arch_txt), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(info->table), GTK_WIDGET(info->lbl_arch_txt), 1, 2, 1, 2,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	info->lbl_bios = gtk_label_new("Bios Version:");
	gtk_misc_set_alignment (GTK_MISC (info->lbl_bios), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(info->table), GTK_WIDGET(info->lbl_bios), 0, 1, 2, 3,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	info->lbl_bios_txt = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (info->lbl_bios_txt), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(info->table), GTK_WIDGET(info->lbl_bios_txt), 1, 2, 2, 3,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	info->lbl_bustype = gtk_label_new("Bus Type:");
	gtk_misc_set_alignment (GTK_MISC (info->lbl_bustype), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(info->table), GTK_WIDGET(info->lbl_bustype), 0, 1, 3, 4,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	info->lbl_bustype_txt = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (info->lbl_bustype_txt), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(info->table), GTK_WIDGET(info->lbl_bustype_txt), 1, 2, 3, 4,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	info->lbl_vidmem = gtk_label_new("Video Memory:");
	gtk_misc_set_alignment (GTK_MISC (info->lbl_vidmem), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(info->table), GTK_WIDGET(info->lbl_vidmem), 0, 1, 4, 5,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	info->lbl_vidmem_txt = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (info->lbl_vidmem_txt), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(info->table), GTK_WIDGET(info->lbl_vidmem_txt), 1, 2, 4, 5,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	info->lbl_memtype = gtk_label_new("Memory Type:");
	gtk_misc_set_alignment (GTK_MISC (info->lbl_memtype), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(info->table), GTK_WIDGET(info->lbl_memtype), 0, 1, 5, 6,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	info->lbl_memtype_txt = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (info->lbl_memtype_txt), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(info->table), GTK_WIDGET(info->lbl_memtype_txt), 1, 2, 5, 6,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	info->lbl_irq = gtk_label_new("IRQ:");
	gtk_misc_set_alignment (GTK_MISC (info->lbl_irq), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(info->table), GTK_WIDGET(info->lbl_irq), 0, 1, 6, 7,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	info->lbl_irq_txt = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (info->lbl_irq_txt), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(info->table), GTK_WIDGET(info->lbl_irq_txt), 1, 2, 6, 7,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	gtk_box_pack_start(GTK_BOX(info), info->frame, FALSE, FALSE, 0);
	gtk_widget_show_all(GTK_WIDGET(info));

	set_videocard_info(GTK_WIDGET(info));

	return GTK_WIDGET (info);
}


GtkWidget* nv_overclock_new (config *conf, GtkTooltips *tips, NVCard *card, void *dpy)
{
	NVOverclock *overclock = g_object_new (NV_TYPE_OVERCLOCK, NULL);
	GtkWidget *alignment;

	overclock->card = card;
	overclock->conf = conf;
	overclock->clock_changes = 0;
	overclock->memclk = 0;
	overclock->nvclk = 0;
	overclock->tips = tips;
#ifdef HAVE_NVCONTROL
	overclock->dpy = (Display*)dpy;
#endif

	/* Create a frame in which we will store all overclocking related stuff as it needs to be possible to disable overclocking */
	overclock->frame = gtk_frame_new(NULL);
	overclock->chk_overclock = gtk_check_button_new_with_label("Enable overclocking");
	gtk_frame_set_label_widget(GTK_FRAME(overclock->frame), overclock->chk_overclock);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(overclock->tips), overclock->chk_overclock, \
		"Through overclocking you can increase the performance of your videocard a lot. "
		"While overclocking increases performance it comes at a great risk. The problem "
		"is that the temperature of the GPU and memory increases a lot. Without proper "
		"cooling this can seriously damage your hardware. Enabling of overclocking is "
		"at your own risk!", NULL);

	/* Vbox in which to store gpu/memory scales, overclock buttons .. */
	overclock->vbox = gtk_vbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(overclock->frame), overclock->vbox);
	gtk_container_set_border_width(GTK_CONTAINER(overclock->vbox), 2);
	gtk_box_pack_start(GTK_BOX(overclock), overclock->frame, TRUE, TRUE, 0);
	gtk_widget_set_sensitive(overclock->vbox, FALSE);

	/* Overclocking of GeforceFX/Geforce6 hardware can happen through two backends. The first
	/  uses the NV-CONTROL X extension and the other is a low-level backend. We try to default
	/  to NV-CONTROL as it allows you to specify 2d/3d clocks. Note that for example MOBILE GPUs
	/  aren't by NV-CONTROL.
	/
	/  To allow switching between 2d/3d clocks we need a combobox but only if Coolbits is supported.
	*/
	if(nv_card->caps & COOLBITS_OVERCLOCKING)
	{
		overclock->combo_speeds = gtk_combo_box_new_text();

		gtk_combo_box_append_text(GTK_COMBO_BOX(overclock->combo_speeds), "2D Clocks");
		gtk_combo_box_append_text(GTK_COMBO_BOX(overclock->combo_speeds), "3D Clocks");
		gtk_combo_box_append_text(GTK_COMBO_BOX(overclock->combo_speeds), "2D + 3D Clocks");
		gtk_combo_box_set_active(GTK_COMBO_BOX(overclock->combo_speeds), 0);
		gtk_box_pack_start(GTK_BOX(overclock->vbox), overclock->combo_speeds, FALSE, FALSE, 0);
		g_signal_connect(GTK_COMBO_BOX(overclock->combo_speeds), "changed", G_CALLBACK(combo_speeds_changed), overclock);
	}

	overclock->frm_gpu = gtk_frame_new("GPU clock");
	gtk_box_pack_start(GTK_BOX(overclock->vbox), overclock->frm_gpu, FALSE, FALSE, 0);

	overclock->scale_gpu = gtk_hscale_new_with_range(0, 100, 1);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(overclock->tips), overclock->scale_gpu, "Adjusts the clock frequency at which the GPU runs", NULL);
	gtk_container_add(GTK_CONTAINER(overclock->frm_gpu), overclock->scale_gpu);
	g_signal_connect(GTK_RANGE(overclock->scale_gpu), "format-value", G_CALLBACK(value_to_mhz), NULL);
	g_signal_connect(GTK_RANGE(overclock->scale_gpu), "value-changed", G_CALLBACK(scale_gpu_changed), overclock);
	gtk_scale_set_value_pos (GTK_SCALE (overclock->scale_gpu), GTK_POS_RIGHT);

	overclock->frm_mem = gtk_frame_new("Memory clock");
	gtk_box_pack_start(GTK_BOX(overclock->vbox), overclock->frm_mem, FALSE, FALSE, 0);

	overclock->scale_mem = gtk_hscale_new_with_range(0, 100, 1);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(overclock->tips), overclock->scale_mem, "Adjusts the clock frequency at which the memory runs", NULL);
	gtk_container_add(GTK_CONTAINER(overclock->frm_mem), overclock->scale_mem);
	g_signal_connect(GTK_RANGE(overclock->scale_mem), "format-value", G_CALLBACK(value_to_mhz), NULL);
	g_signal_connect(GTK_RANGE(overclock->scale_mem), "value-changed", G_CALLBACK(scale_mem_changed), overclock);
	gtk_scale_set_value_pos (GTK_SCALE (overclock->scale_mem), GTK_POS_RIGHT);

	overclock->chk_test_speeds = gtk_check_button_new_with_label("Test speeds before applying");
	gtk_tooltips_set_tip(GTK_TOOLTIPS(overclock->tips), overclock->chk_test_speeds, \
		"Test the clocks during a clock change. When the clocks don't work properly you can restore the previous ones.", NULL);
	gtk_box_pack_start(GTK_BOX(overclock->vbox), overclock->chk_test_speeds, FALSE, FALSE, 10);

	overclock->bbox = gtk_hbutton_box_new();
	gtk_box_set_spacing(GTK_BOX(overclock->bbox), 5);
	alignment = gtk_alignment_new(1, 1, 0, 0);
	gtk_container_add(GTK_CONTAINER(alignment), overclock->bbox);
	gtk_box_pack_start(GTK_BOX(overclock->vbox), alignment, TRUE, TRUE, 0);

	/* Change Speeds button */
	overclock->btn_change_speeds = gtk_button_new_with_label("Change Speeds");
	gtk_tooltips_set_tip(GTK_TOOLTIPS(overclock->tips), overclock->btn_change_speeds, "Apply any GPU/memory clock changes", NULL);
	gtk_container_add(GTK_CONTAINER(overclock->bbox), overclock->btn_change_speeds);

	overclock->btn_reset_speeds = gtk_button_new_with_label("Reset Speeds");
	gtk_tooltips_set_tip(GTK_TOOLTIPS(overclock->tips), overclock->btn_reset_speeds, "Restore the original GPU/memory clocks", NULL);
	gtk_container_add(GTK_CONTAINER(overclock->bbox), overclock->btn_reset_speeds);

	gtk_button_box_set_layout(GTK_BUTTON_BOX(overclock->bbox), GTK_BUTTONBOX_END);

	gtk_widget_show_all(GTK_WIDGET(overclock));

	set_overclock_info(GTK_WIDGET(overclock));

	g_signal_connect(G_OBJECT(overclock->btn_change_speeds), "clicked", G_CALLBACK(change_speeds), overclock);
	g_signal_connect(G_OBJECT(overclock->btn_reset_speeds), "clicked", G_CALLBACK(reset_speeds), overclock);
	g_signal_connect(GTK_TOGGLE_BUTTON(overclock->chk_overclock), "toggled", G_CALLBACK (enable_overclocking), overclock);
	g_signal_connect(GTK_TOGGLE_BUTTON(overclock->chk_test_speeds), "toggled", G_CALLBACK (chk_test_speeds_toggled), overclock);
	g_signal_connect(overclock, "map", G_CALLBACK(nv_overclock_mapped), NULL);

	return GTK_WIDGET (overclock);
}


GtkWidget *nv_pipeline_new(config *conf, NVCard *card)
{
	NVPipeline *pipeline = g_object_new(NV_TYPE_PIPELINE, NULL);

	pipeline->frame = gtk_frame_new("Pipeline information");
	gtk_box_pack_start(GTK_BOX(pipeline), pipeline->frame, FALSE, FALSE, 2);

	pipeline->table = gtk_table_new(2,2, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(pipeline->table), 2);
	gtk_table_set_row_spacings(GTK_TABLE(pipeline->table), 3);
	gtk_container_add(GTK_CONTAINER(pipeline->frame), pipeline->table);

	pipeline->lbl_pipes = gtk_label_new("Active units:");
	gtk_misc_set_alignment (GTK_MISC (pipeline->lbl_pipes), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(pipeline->table), GTK_WIDGET(pipeline->lbl_pipes), 0, 1, 0, 1,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	pipeline->lbl_pipes_txt = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (pipeline->lbl_pipes_txt), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(pipeline->table), GTK_WIDGET(pipeline->lbl_pipes_txt), 1, 2, 0, 1,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	pipeline->lbl_config = gtk_label_new("Configuration:");
	gtk_misc_set_alignment (GTK_MISC (pipeline->lbl_config), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(pipeline->table), GTK_WIDGET(pipeline->lbl_config), 0, 1, 1, 2,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	pipeline->lbl_config_txt = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (pipeline->lbl_config_txt), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(pipeline->table), GTK_WIDGET(pipeline->lbl_config_txt), 1, 2, 1, 2,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	pipeline->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	pipeline->tree_view = gtk_tree_view_new();
	pipeline->store = gtk_tree_store_new(N_PIPE_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING );

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW (pipeline->scrolled_window), pipeline->tree_view);
	/* Only show vertical scrollbars when needed */
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (pipeline->scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_tree_view_set_model(GTK_TREE_VIEW(pipeline->tree_view), GTK_TREE_MODEL(pipeline->store));

	pipeline->column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(pipeline->column, "Bit");
	pipeline->renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(pipeline->column, pipeline->renderer, FALSE);
	gtk_tree_view_column_set_attributes(pipeline->column, pipeline->renderer, "text", BIT_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(pipeline->tree_view), pipeline->column);

	pipeline->column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(pipeline->column, "Description");
	pipeline->renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(pipeline->column, pipeline->renderer, FALSE);
	gtk_tree_view_column_set_attributes(pipeline->column, pipeline->renderer, "text", DESC_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(pipeline->tree_view), pipeline->column);

	pipeline->column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(pipeline->column, "State");
	pipeline->renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(pipeline->column, pipeline->renderer, FALSE);
	gtk_tree_view_column_set_attributes(pipeline->column, pipeline->renderer, "text", STATE_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(pipeline->tree_view), pipeline->column);

	pipeline->column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(pipeline->column, "HW Masked");
	pipeline->renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(pipeline->column, pipeline->renderer, FALSE);
	gtk_tree_view_column_set_attributes(pipeline->column, pipeline->renderer,"text", MASKED_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(pipeline->tree_view), pipeline->column);

	/* Allow an alternating color for the rows in the list */
	g_object_set(pipeline->tree_view, "rules-hint", TRUE, NULL);

	gtk_box_pack_start(GTK_BOX(pipeline), pipeline->scrolled_window, TRUE, TRUE, 2);
	gtk_widget_show_all(GTK_WIDGET(pipeline));

	set_pipeline_info(GTK_WIDGET(pipeline));

	return GTK_WIDGET(pipeline);
}


GtkWidget *nv_shader_new(config *conf, NVCard *card)
{
	NVShader *shader = g_object_new(NV_TYPE_SHADER, NULL);

	shader->frame = gtk_frame_new("Shader information");
	gtk_box_pack_start(GTK_BOX(shader), shader->frame, FALSE, FALSE, 2);

	shader->table = gtk_table_new(2,2, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(shader->table), 2);
	gtk_table_set_row_spacings(GTK_TABLE(shader->table), 3);
	gtk_container_add(GTK_CONTAINER(shader->frame), shader->table);

	shader->lbl_clock = gtk_label_new("Clock:");
	gtk_misc_set_alignment (GTK_MISC (shader->lbl_clock), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(shader->table), GTK_WIDGET(shader->lbl_clock), 0, 1, 0, 1,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	shader->lbl_clock_txt = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (shader->lbl_clock_txt), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(shader->table), GTK_WIDGET(shader->lbl_clock_txt), 1, 2, 0, 1,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	shader->lbl_streamunits = gtk_label_new("Stream units:");
	gtk_misc_set_alignment (GTK_MISC (shader->lbl_streamunits), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(shader->table), GTK_WIDGET(shader->lbl_streamunits), 0, 1, 1, 2,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	shader->lbl_streamunits_txt = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (shader->lbl_streamunits_txt), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(shader->table), GTK_WIDGET(shader->lbl_streamunits_txt), 1, 2, 1, 2,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	shader->lbl_ropunits = gtk_label_new("ROP units:");
	gtk_misc_set_alignment (GTK_MISC (shader->lbl_ropunits), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(shader->table), GTK_WIDGET(shader->lbl_ropunits), 0, 1, 2, 3,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	shader->lbl_ropunits_txt = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (shader->lbl_ropunits_txt), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(shader->table), GTK_WIDGET(shader->lbl_ropunits_txt), 1, 2, 2, 3,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	shader->lbl_config = gtk_label_new("Configuration:");
	gtk_misc_set_alignment (GTK_MISC (shader->lbl_config), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(shader->table), GTK_WIDGET(shader->lbl_config), 0, 1, 3, 4,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	shader->lbl_config_txt = gtk_label_new("");
	gtk_misc_set_alignment (GTK_MISC (shader->lbl_config_txt), 0.02, 0.5);
	gtk_table_attach (GTK_TABLE(shader->table), GTK_WIDGET(shader->lbl_config_txt), 1, 2, 3, 4,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	gtk_widget_show_all(GTK_WIDGET(shader));

	set_shader_info(GTK_WIDGET(shader));

	return GTK_WIDGET(shader);
}


GtkWidget *nv_thermal_new(config *conf, GtkTooltips *tips, NVCard *card)
{
	NVThermal *thermal = g_object_new(NV_TYPE_THERMAL, NULL);
	GtkWidget *hbox;
	int have_ambient;

	thermal->card = card;
	thermal->conf = conf;
	thermal->tips = tips;
	thermal->frame = gtk_frame_new("Sensor information");
	gtk_box_pack_start(GTK_BOX(thermal), thermal->frame, FALSE, FALSE, 0);

	thermal->vbox = gtk_vbox_new(FALSE, 2);
	gtk_container_set_border_width(GTK_CONTAINER(thermal->vbox), 2);
	gtk_container_add(GTK_CONTAINER(thermal->frame), thermal->vbox);

	hbox = gtk_hbox_new(FALSE, 2);
	thermal->lbl_sensor = gtk_label_new("Sensor chip: ");
	gtk_misc_set_alignment(GTK_MISC(thermal->lbl_sensor), 0.02, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), thermal->lbl_sensor, FALSE, FALSE, 0);
	thermal->lbl_sensor_txt = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(thermal->lbl_sensor_txt), 0.02, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), thermal->lbl_sensor_txt, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(thermal->vbox), hbox, FALSE, FALSE, 0);

	thermal->frm_core = gtk_frame_new("GPU Temperature");
	hbox = gtk_hbox_new(FALSE, 2);
	gtk_container_add(GTK_CONTAINER(thermal->frm_core), hbox);
	thermal->pbar_core = gtk_progress_bar_new();
	thermal->lbl_core = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(hbox), thermal->pbar_core, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), thermal->lbl_core, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(thermal->vbox), thermal->frm_core, FALSE, FALSE, 0);

	gtk_container_set_border_width(GTK_CONTAINER(hbox), 2);

	if(nv_card->caps & BOARD_TEMP_MONITORING)
	{
		thermal->frm_ambient = gtk_frame_new("Ambient Temperature");
		hbox = gtk_hbox_new(FALSE, 2);
		thermal->pbar_ambient = gtk_progress_bar_new();
		thermal->lbl_ambient = gtk_label_new("");
		gtk_container_add(GTK_CONTAINER(thermal->frm_ambient), hbox);
		gtk_box_pack_start(GTK_BOX(hbox), thermal->pbar_ambient, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(hbox), thermal->lbl_ambient, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(thermal->vbox), thermal->frm_ambient, FALSE, FALSE, 0);
		gtk_container_set_border_width(GTK_CONTAINER(hbox), 2);
	}

	/* Create a frame containing the 'real' fanspeed in RPM which is supported on some i2c sensors */
	if(nv_card->caps & I2C_FANSPEED_MONITORING)
	{
		thermal->frm_fanspeed = gtk_frame_new("Fanspeed");
		hbox = gtk_hbox_new(FALSE, 2);
		thermal->pbar_fanspeed = gtk_progress_bar_new();
		thermal->lbl_fanspeed = gtk_label_new("");

		gtk_container_add(GTK_CONTAINER(thermal->frm_fanspeed), hbox);
		gtk_box_pack_start(GTK_BOX(hbox), thermal->pbar_fanspeed, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(hbox), thermal->lbl_fanspeed, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(thermal->vbox), thermal->frm_fanspeed, FALSE, FALSE, 0);
		gtk_container_set_border_width(GTK_CONTAINER(hbox), 2);
	}

	/* Create a frame with a slider using which fanspeed can be adjusted by changing the duty cycle */
	if(nv_card->caps & (GPU_FANSPEED_MONITORING | I2C_FANSPEED_MONITORING))
	{
		GtkWidget *alignment;
		thermal->frm_fanspeed = gtk_frame_new(NULL);
		gtk_box_pack_start(GTK_BOX(thermal), thermal->frm_fanspeed, FALSE, FALSE, 0);
		thermal->chk_fanspeed = gtk_check_button_new_with_label("Enable fanspeed adjustments");
		gtk_tooltips_set_tip(GTK_TOOLTIPS(thermal->tips), thermal->chk_fanspeed, "Various cards feature the ability to adjust the speed of the fans on the card to reduce noise / increase cooling.", NULL);
		gtk_frame_set_label_widget(GTK_FRAME(thermal->frm_fanspeed), thermal->chk_fanspeed);
		g_signal_connect(GTK_TOGGLE_BUTTON(thermal->chk_fanspeed), "toggled", G_CALLBACK (enable_fanspeed_adjustments), thermal);

		thermal->frm_dutycycle = gtk_frame_new("Fan - Duty cycle");
		gtk_container_set_border_width(GTK_CONTAINER(thermal->frm_dutycycle), 2);
		gtk_container_add(GTK_CONTAINER(thermal->frm_fanspeed), thermal->frm_dutycycle);
		thermal->vbx_dutycycle = gtk_vbox_new(FALSE, 2);
		gtk_container_add(GTK_CONTAINER(thermal->frm_dutycycle), thermal->vbx_dutycycle);

		thermal->scale_dutycycle = gtk_hscale_new_with_range(0, 100, .1);
		gtk_tooltips_set_tip(GTK_TOOLTIPS(thermal->tips), thermal->scale_dutycycle, \
			"Using this option you can adjust the duty cycle of the fanspeed.\n\n"
			"Note that the duty cycle is NOT the fanspeed. The duty cycle controls "
			"the percentage of time in a fixed time interval in which the fan is 'on'. "
			"By choosing a higher value the fan is enabled a larger part of the time "
			"which results in a higher fanspeed. Changing the duty cycle from 20% to 40% "
			"doesn't double the noise or fanspeed.\n", NULL);

		gtk_scale_set_value_pos (GTK_SCALE (thermal->scale_dutycycle), GTK_POS_RIGHT);
		gtk_box_pack_start(GTK_BOX(thermal->vbx_dutycycle), thermal->scale_dutycycle, FALSE, FALSE, 0);
		g_signal_connect(GTK_RANGE(thermal->scale_dutycycle), "format-value", G_CALLBACK(value_to_percent), NULL);

		thermal->bbox = gtk_hbutton_box_new();
		gtk_box_set_spacing(GTK_BOX(thermal->bbox), 5);
		alignment = gtk_alignment_new(1, 1, 0, 0);
		gtk_container_add(GTK_CONTAINER(alignment), thermal->bbox);
		gtk_box_pack_start(GTK_BOX(thermal->vbx_dutycycle), alignment, FALSE, FALSE, 0);

		thermal->btn_apply = gtk_button_new_with_label("Apply");
		gtk_tooltips_set_tip(GTK_TOOLTIPS(thermal->tips), thermal->btn_apply, "Apply fanspeed changes.", NULL);
		gtk_container_add(GTK_CONTAINER(thermal->bbox), thermal->btn_apply);
		g_signal_connect(G_OBJECT(thermal->btn_apply), "clicked", G_CALLBACK(change_fanspeed), thermal);
	}

	gtk_widget_show_all(GTK_WIDGET(thermal));

	/* Makes sure the temperature gets updated every second */
	g_signal_connect(thermal, "map", G_CALLBACK(nv_thermal_mapped), NULL);

	set_thermal_info(GTK_WIDGET(thermal));

	return GTK_WIDGET(thermal);
}


int gui_hw_init(config *conf, GtkTooltips *tips)
{
	GtkTreeIter parent, child;
	int i;

#ifdef HAVE_NVCONTROL
	int irq=0;
	Display *dpy = XOpenDisplay("");

	/* If opengl stuff isn't supported don't show it on the gui */
	if(init_nvcontrol(dpy))
	{
		int tmp;
		NVGetAttribute(dpy, 0, 0, NV_IRQ, &irq);
	}
#else
	void *dpy;
#endif
	for(i=0; i < nvclock.num_cards; i++)
	{
		set_card(i);

		add(&parent, NULL, nvclock.card[i].card_name, BANNER_HW, nv_info_new(&nvclock.card[i]));
		add(&child, &parent, "Overclocking", BANNER_HW, nv_overclock_new(conf, tips, &nvclock.card[i], dpy));
		/* fixme: backend should know about the bustype; if pci we shouldn't show agp */
		if(strcmp(nv_card->get_bus_type(), "AGP") == 0)
			add(&child, &parent, "AGP", BANNER_HW, nv_agp_new(&nvclock.card[i]));

		/* Show currently only bios info for recent models and if there's a bios dump available */
		if(nv_card->bios)
			if(nv_card->bios->perf_entries)
				add(&child, &parent, "VideoBios", BANNER_HW, nv_bios_new(&nvclock.card[i]));

		if(nv_card->caps & GPU_TEMP_MONITORING)
			add(&child, &parent, "Hardware Monitoring", BANNER_HW, nv_thermal_new(conf, tips, &nvclock.card[i]));

		if(nv_card->arch & NV4X)
			add(&child, &parent, "Pipelines", BANNER_HW, nv_pipeline_new(conf, &nvclock.card[i]));

		if(nv_card->arch & NV5X)
			add(&child, &parent, "Shaders", BANNER_HW, nv_shader_new(conf, &nvclock.card[i]));
	}

	return 1;
}

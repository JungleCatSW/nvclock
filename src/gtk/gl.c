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
 * OpenGL interface containing information and tweaking of OpenGL settings
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <X11/X.h>
#include <X11/Xutil.h>
#include "gl.h"
#include "interface.h"
#include "nvclock.h"
#include "nvcontrol.h"

static int initialize = 0;
const char* (*pglGetString)(int) = (void *) 0xdeadbeef;

XVisualInfo* (*pglXChooseVisual)(Display *dpy, int screen, int *attribList) = (void*) 0xdeadbeef;
GLXContext (*pglXCreateContext)(Display *dpy, XVisualInfo *vis, GLXContext shareList, int direct) = (void*) 0xdeadbeef;
void (*pglXDestroyContext)(Display *dpy, GLXContext ctx) = (void*) 0xdeadbeef;
int (*pglXIsDirect)(Display *dpy, GLXContext ctx) = (void*) 0xdeadbeef;
int (*pglXMakeCurrent)(Display *dpy, GLXDrawable drawable, GLXContext ctx) = (void*) 0xdeadbeef;
const char* (*pglXQueryServerString)(Display *dpy, int screen, int name) = (void*) 0xdeadbeef;

GType nv_opengl_get_type (void)
{
	static GType opengl_type = 0;

	if (!opengl_type)
	{
		static const GTypeInfo opengl_info =
		{
			sizeof (NVOpenglClass),
			NULL,			/* base_init */
			NULL,			/* base_finalize */
			NULL,
			NULL,			/* class_finalize */
			NULL,			/* class_data */
			sizeof (NVOpengl),
			0,			/* n_preallocs */
			NULL			/* instance_init */
		};

		opengl_type = g_type_register_static (GTK_TYPE_VBOX, "NVOpengl",
			&opengl_info, 0);
	}

	return opengl_type;
}


GType nv_glgeneral_get_type (void)
{
	static GType glgeneral_type = 0;

	if (!glgeneral_type)
	{
		static const GTypeInfo glgeneral_info =
		{
			sizeof (NVGlGeneralClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			NULL,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (NVGlGeneral),
			0,		/* n_preallocs */
			NULL		/* instance_init */
		};

		glgeneral_type = g_type_register_static (GTK_TYPE_VBOX, "NVGlGeneral",
			&glgeneral_info, 0);
	}

	return glgeneral_type;
}


GType nv_glquality_get_type (void)
{
	static GType glquality_type = 0;

	if (!glquality_type)
	{
		static const GTypeInfo glquality_info =
		{
			sizeof (NVGlQualityClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			NULL,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (NVGlQuality),
			0,		/* n_preallocs */
			NULL		/* instance_init */
		};

		glquality_type = g_type_register_static (GTK_TYPE_VBOX, "NVGlQuality",
			&glquality_info, 0);
	}

	return glquality_type;
}


/* Wrapper around NVGetAttribute used for configuration file support */
int GLGetAttribute(config *conf, Display *dpy, int screen, unsigned int disp_mask, unsigned int option, int *value)
{
	/* Verify if the option is available before continuing */
	if(!NVGetAttribute(dpy, screen, disp_mask, option, value))
		return 0;

	/* When initialize is set, we will bypass NVGetAttribute and read the values
	/  from the configuration file.
	*/
	if(initialize && conf->save_opengl_changes)
	{
		NVOptionList *opt = nvcontrol_lookup_option(option);
		char *name;

		/* Fsaa/Aniso application control aren't stored in our database,
		/  so if the requested option can't be found check if it is one
		/  of the control options.
		*/
		if(!opt)
		{
			if(option == NV_FSAA_APP_CONTROLLED)
			{
				name = g_strdup_printf("fsaa_app_controlled");
			}
			if(option == NV_ANISO_APP_CONTROLLED)
			{
				name = g_strdup_printf("aniso_app_controlled");
			}
		}
		else
			name = (char*)opt->name;

		if(name)
		{
			cfg_entry *entry = lookup_entry((cfg_entry**)&conf->cfg, "opengl", name);
			if(entry)
			{
				NVSetAttribute(dpy, screen, disp_mask, option, entry->value);
				*value = entry->value;
				return 1;
			}
			/* When a setting isn't stored in the config file retrieve the current value and store it */
			else
			{
				if(NVGetAttribute(dpy, screen, disp_mask, option, value))
				{
					add_entry(&nvclock.cfg, "opengl", name, *value);
					return 1;
				}
				return 0;
			}
		}
	}
	return NVGetAttribute(dpy, screen, disp_mask, option, value);
}


void GLSetAttribute(config *conf, Display *dpy, int screen, unsigned int disp_mask, unsigned int option, int value)
{
	if(conf->save_opengl_changes)
	{
		NVOptionList *opt = nvcontrol_lookup_option(option);
		char *name;

		/* Fsaa/Aniso application control aren't stored in our database,
		/  so if the requested option can't be found check if it is one
		/  of the control options.
		*/
		if(!opt)
		{
			if(option == NV_FSAA_APP_CONTROLLED)
			{
				name = g_strdup_printf("fsaa_app_controlled");
			}
			if(option == NV_ANISO_APP_CONTROLLED)
			{
				name = g_strdup_printf("aniso_app_controlled");
			}
		}
		else
			name = (char*)opt->name;

		if(name)
		{
			change_entry(&nvclock.cfg, "opengl", name, value);
		}
	}
	NVSetAttribute(dpy, screen, disp_mask, option, value);
}


/* Convert the value an option is set to to a string */
gchar* aniso_value_to_str(GtkRange *range, gdouble value, GtkWidget *widget)
{
	return g_strdup_printf(" %dX ", 1 << (int)value);
}


gchar* fsaa_value_to_str(GtkRange *range, gdouble value, GtkWidget *widget)
{
	NVGlQuality *quality = NV_GLQUALITY(widget);
	int val = (int)gtk_range_get_value(range);
	NVOptionList *opt = nvcontrol_lookup_option(NV_FSAA);

	return g_strdup_printf(" %s ", opt->values[quality->fsaa_lst[val]].name);
}

gchar* image_quality_value_to_str(GtkRange *range, gdouble value, GtkWidget *widget)
{
	NVGlQuality *quality = NV_GLQUALITY(widget);
	int val = (int)gtk_range_get_value(range);
	NVOptionList *opt = nvcontrol_lookup_option(NV_IMAGE_QUALITY);

	return g_strdup_printf(" %s ", opt->values[val].name);
}


void aniso_set_app_controlled(GtkToggleButton *button, NVGlQuality *quality)
{
	if(gtk_toggle_button_get_active(button))
	{
		GLSetAttribute(quality->conf, quality->dpy, 0, 0, NV_ANISO_APP_CONTROLLED, 1);
		gtk_widget_set_sensitive(GTK_WIDGET(quality->scale_aniso), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(quality->lbl_aniso_perf), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(quality->lbl_aniso_quality), FALSE);
	}
	else
	{
		GLSetAttribute(quality->conf, quality->dpy, 0, 0, NV_ANISO_APP_CONTROLLED, 0);
		gtk_widget_set_sensitive(GTK_WIDGET(quality->scale_aniso), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(quality->lbl_aniso_perf), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(quality->lbl_aniso_quality), TRUE);
	}
}


void aniso_set_value(GtkRange *range, GtkWidget *widget)
{
	int value = (int)gtk_range_get_value(range);
	NVGlQuality *quality = NV_GLQUALITY(widget);

	GLSetAttribute(quality->conf, quality->dpy, 0, 0, NV_LOG_ANISO, value);
}


void fsaa_set_app_controlled(GtkToggleButton *button, NVGlQuality *quality)
{
	if(gtk_toggle_button_get_active(button))
	{
		GLSetAttribute(quality->conf, quality->dpy, 0, 0, NV_FSAA_APP_CONTROLLED, 1);
		gtk_widget_set_sensitive(GTK_WIDGET(quality->scale_fsaa), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(quality->lbl_fsaa_perf), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(quality->lbl_fsaa_quality), FALSE);
	}
	else
	{
		GLSetAttribute(quality->conf, quality->dpy, 0, 0, NV_FSAA_APP_CONTROLLED, 0);
		gtk_widget_set_sensitive(GTK_WIDGET(quality->scale_fsaa), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(quality->lbl_fsaa_perf), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(quality->lbl_fsaa_quality), TRUE);
	}
}


void fsaa_set_value(GtkRange *range, GtkWidget *widget)
{
	int value = (int)gtk_range_get_value(range);
	NVGlQuality *quality = NV_GLQUALITY(widget);

	GLSetAttribute(quality->conf, quality->dpy, 0, 0, NV_FSAA, quality->fsaa_lst[value]);
}


int fsaa_nvcontrol_value_to_scale_value(GtkWidget *widget, int value)
{
	NVGlQuality *quality = NV_GLQUALITY(widget);
	int i;

	for(i=0; i<quality->fsaa_entries; i++)
	{
		if(quality->fsaa_lst[i] == value)
		{
			return i;
		}
	}
	return 0;
}

void aa_line_gamma_set_value(GtkToggleButton *button, GtkWidget *widget)
{
	int value =  gtk_toggle_button_get_active(button);
	NVGlGeneral *general = NV_GLGENERAL(widget);

	GLSetAttribute(general->conf, general->dpy, 0, 0, NV_OPENGL_AA_LINE_GAMMA, value);
}

void generic_cpu_set_value(GtkToggleButton *button, GtkWidget *widget)
{
	int value =  gtk_toggle_button_get_active(button);
	NVGlGeneral *general = NV_GLGENERAL(widget);

	GLSetAttribute(general->conf, general->dpy, 0, 0, NV_FORCE_GENERIC_CPU, value);
}


void flipping_set_value(GtkToggleButton *button, GtkWidget *widget)
{
	int value =  gtk_toggle_button_get_active(button);
	NVGlGeneral *general = NV_GLGENERAL(widget);

	GLSetAttribute(general->conf, general->dpy, 0, 0, NV_FLIPPING_ALLOWED, value);
}


void image_quality_set_value(GtkRange *range, GtkWidget *widget)
{
	int value = (int)gtk_range_get_value(range);
	NVGlQuality *quality = NV_GLQUALITY(widget);

	GLSetAttribute(quality->conf, quality->dpy, 0, 0, NV_IMAGE_QUALITY, value);
}

void slihud_set_value(GtkToggleButton *button, GtkWidget *widget)
{
	int value =  gtk_toggle_button_get_active(button);
	NVGlGeneral *general = NV_GLGENERAL(widget);

	GLSetAttribute(general->conf, general->dpy, 0, 0, NV_SHOW_SLI_HUD, value);
}


void tex_sharpen_set_value(GtkToggleButton *button, GtkWidget *widget)
{
	int value =  gtk_toggle_button_get_active(button);
	NVGlQuality *quality = NV_GLQUALITY(widget);

	GLSetAttribute(quality->conf, quality->dpy, 0, 0, NV_TEXTURE_SHARPEN, value);
}


void tex_clamping_set_value(GtkToggleButton *button, GtkWidget *widget)
{
	int value =  gtk_toggle_button_get_active(button);
	NVGlGeneral *general = NV_GLGENERAL(widget);

	GLSetAttribute(general->conf, general->dpy, 0, 0, NV_TEXTURE_CLAMPING, value);
}


void vsync_set_value(GtkToggleButton *button, GtkWidget *widget)
{
	int value =  gtk_toggle_button_get_active(button);
	NVGlGeneral *general = NV_GLGENERAL(widget);

	GLSetAttribute(general->conf, general->dpy, 0, 0, NV_SYNC_VBLANK, value);
}


GtkWidget *nv_opengl_new(Display* dpy, int screen)
{
	NVOpengl *opengl = g_object_new(NV_TYPE_OPENGL, NULL);
	char *glx_vendor, *glx_version, *glx_direct;
	char *gl_vendor, *gl_renderer, *gl_version;

	Window win;
	Window root;
	GLXContext ctx;
	XVisualInfo *visinfo;
	XSetWindowAttributes attr;
	unsigned long mask;
	int width = 100;
	int height = 100;
	int attribSingle[] = {GLX_RGBA, GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1, None};
	int attribDouble[] = {GLX_RGBA, GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1, GLX_DOUBLEBUFFER, None};

	root = RootWindow(dpy, screen);

	visinfo = pglXChooseVisual(dpy, screen, attribSingle);
	if (!visinfo)
	{
		visinfo = pglXChooseVisual(dpy, screen, attribDouble);
		if (!visinfo)
		{
			fprintf(stderr, "Error: couldn't find RGB GLX visual\n");
			return NULL;
		}
	}

	attr.background_pixel = 0;
	attr.border_pixel = 0;
	attr.colormap = XCreateColormap(dpy, root, visinfo->visual, AllocNone);
	attr.event_mask = StructureNotifyMask | ExposureMask;
	mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
	win = XCreateWindow(dpy, root, 0, 0, width, height, 0, visinfo->depth, InputOutput, visinfo->visual, mask, &attr);

	ctx = pglXCreateContext( dpy, visinfo, NULL, 1);
	if (!ctx)
	{
		fprintf(stderr, "Error: glXCreateContext failed\n");
		XDestroyWindow(dpy, win);
		return NULL;
	}

	if(pglXMakeCurrent(dpy, win, ctx))
	{
		glx_vendor = (char*)pglXQueryServerString(dpy, screen, GLX_VENDOR);
		glx_version = (char*)pglXQueryServerString(dpy, screen, GLX_VERSION);
		glx_direct = (char*)(pglXIsDirect(dpy, ctx) ? "Yes" : "No");
		gl_vendor = (char*)pglGetString(GL_VENDOR);
		gl_renderer = (char*)pglGetString(GL_RENDERER);
		gl_version = (char*)pglGetString(GL_VERSION);

		pglXDestroyContext(dpy, ctx);
		XDestroyWindow(dpy, win);
	}

	opengl->frm_glx = gtk_frame_new("GLX Information");
	opengl->tbl_glx = gtk_table_new(3,2, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(opengl->tbl_glx), 2);
	gtk_table_set_row_spacings(GTK_TABLE(opengl->tbl_glx), 3);
	gtk_container_add(GTK_CONTAINER(opengl->frm_glx), opengl->tbl_glx);
	gtk_box_pack_start(GTK_BOX(opengl), opengl->frm_glx, FALSE, FALSE, 0);

	opengl->lbl_glx_vendor = gtk_label_new("Vendor:");
	gtk_misc_set_alignment (GTK_MISC (opengl->lbl_glx_vendor), 0.02, 0);
	gtk_table_attach (GTK_TABLE(opengl->tbl_glx), GTK_WIDGET(opengl->lbl_glx_vendor), 0, 1, 0, 1,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);
	opengl->lbl_glx_vendor_txt = gtk_label_new(glx_vendor);
	gtk_misc_set_alignment (GTK_MISC (opengl->lbl_glx_vendor_txt), 0.02, 0);
	gtk_table_attach (GTK_TABLE(opengl->tbl_glx), GTK_WIDGET(opengl->lbl_glx_vendor_txt), 1, 2, 0, 1,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	opengl->lbl_glx_version = gtk_label_new("Version:");
	gtk_misc_set_alignment (GTK_MISC (opengl->lbl_glx_version), 0.02, 0);
	gtk_table_attach (GTK_TABLE(opengl->tbl_glx), GTK_WIDGET(opengl->lbl_glx_version), 0, 1, 1, 2,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);
	opengl->lbl_glx_version_txt = gtk_label_new(glx_version);
	gtk_misc_set_alignment (GTK_MISC (opengl->lbl_glx_version_txt), 0.02, 0);
	gtk_table_attach (GTK_TABLE(opengl->tbl_glx), GTK_WIDGET(opengl->lbl_glx_version_txt), 1, 2, 1, 2,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	opengl->lbl_glx_dri = gtk_label_new("Direct rendering:");
	gtk_misc_set_alignment (GTK_MISC (opengl->lbl_glx_dri), 0.02, 0);
	gtk_table_attach (GTK_TABLE(opengl->tbl_glx), GTK_WIDGET(opengl->lbl_glx_dri), 0, 1, 2, 3,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);
	opengl->lbl_glx_dri_txt = gtk_label_new(glx_direct);
	gtk_misc_set_alignment (GTK_MISC (opengl->lbl_glx_dri_txt), 0.02, 0);
	gtk_table_attach (GTK_TABLE(opengl->tbl_glx), GTK_WIDGET(opengl->lbl_glx_dri_txt), 1, 2, 2, 3,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	opengl->frm_gl = gtk_frame_new("GL Information");
	opengl->tbl_gl = gtk_table_new(3,2, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(opengl->tbl_gl), 2);
	gtk_table_set_row_spacings(GTK_TABLE(opengl->tbl_gl), 3);
	gtk_container_add(GTK_CONTAINER(opengl->frm_gl), opengl->tbl_gl);
	gtk_box_pack_start(GTK_BOX(opengl), opengl->frm_gl, FALSE, FALSE, 0);

	opengl->lbl_gl_vendor = gtk_label_new("Vendor:");
	gtk_misc_set_alignment (GTK_MISC (opengl->lbl_gl_vendor), 0.02, 0);
	gtk_table_attach (GTK_TABLE(opengl->tbl_gl), GTK_WIDGET(opengl->lbl_gl_vendor), 0, 1, 0, 1,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);
	opengl->lbl_gl_vendor_txt = gtk_label_new(gl_vendor);
	gtk_misc_set_alignment (GTK_MISC (opengl->lbl_gl_vendor_txt), 0.02, 0);
	gtk_table_attach (GTK_TABLE(opengl->tbl_gl), GTK_WIDGET(opengl->lbl_gl_vendor_txt), 1, 2, 0, 1,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	opengl->lbl_gl_renderer = gtk_label_new("Renderer:");
	gtk_misc_set_alignment (GTK_MISC (opengl->lbl_gl_renderer), 0.02, 0);
	gtk_table_attach (GTK_TABLE(opengl->tbl_gl), GTK_WIDGET(opengl->lbl_gl_renderer), 0, 1, 1, 2,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);
	opengl->lbl_gl_renderer_txt = gtk_label_new(gl_renderer);
	gtk_misc_set_alignment (GTK_MISC (opengl->lbl_gl_renderer_txt), 0.02, 0);
	gtk_table_attach (GTK_TABLE(opengl->tbl_gl), GTK_WIDGET(opengl->lbl_gl_renderer_txt), 1, 2, 1, 2,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	opengl->lbl_gl_version = gtk_label_new("Version:");
	gtk_misc_set_alignment (GTK_MISC (opengl->lbl_gl_version), 0.02, 0);
	gtk_table_attach (GTK_TABLE(opengl->tbl_gl), GTK_WIDGET(opengl->lbl_gl_version), 0, 1, 2, 3,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);
	opengl->lbl_gl_version_txt = gtk_label_new(gl_version);
	gtk_misc_set_alignment (GTK_MISC (opengl->lbl_gl_version_txt), 0.02, 0);
	gtk_table_attach (GTK_TABLE(opengl->tbl_gl), GTK_WIDGET(opengl->lbl_gl_version_txt), 1, 2, 2, 3,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 3, 0);

	gtk_widget_show_all(GTK_WIDGET(opengl));
	return GTK_WIDGET(opengl);
}


GtkWidget *nv_glgeneral_new(config *conf, GtkTooltips *tips, Display *dpy, int screen)
{
	NVGlGeneral *general = g_object_new(NV_TYPE_GLGENERAL, NULL);
	GtkWidget *frame;
	GtkWidget *vbox;
	int val;

	general->conf = conf;
	general->dpy = dpy;
	general->screen = screen;
	general->tips = tips;

	frame = gtk_frame_new("Performance");
	vbox = gtk_vbox_new(FALSE, 2);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_box_pack_start(GTK_BOX(general), frame, FALSE, FALSE, 0);
	if(GLGetAttribute(conf, dpy, 0, 0, NV_SYNC_VBLANK, &val))
	{
		general->chk_vsync = gtk_check_button_new_with_label("Sync to VBlank");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(general->chk_vsync), val);
		g_signal_connect(GTK_TOGGLE_BUTTON(general->chk_vsync), "toggled", G_CALLBACK (vsync_set_value), general);
		gtk_box_pack_start(GTK_BOX(vbox), general->chk_vsync, FALSE, FALSE, 0);

		gtk_tooltips_set_tip (GTK_TOOLTIPS (general->tips), general->chk_vsync, "Sync video buffer swaps to monitor refresh rate. Enabling this option limits the maximum framerate but can improve image quality.", NULL);
	}

	if(GLGetAttribute(conf, dpy, 0, 0, NV_FLIPPING_ALLOWED, &val))
	{
		general->chk_flipping = gtk_check_button_new_with_label("Allow Flipping");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(general->chk_flipping), val);
		g_signal_connect(GTK_TOGGLE_BUTTON(general->chk_flipping), "toggled", G_CALLBACK (flipping_set_value), general);
		gtk_box_pack_start(GTK_BOX(vbox), general->chk_flipping, FALSE, FALSE, 0);

		gtk_tooltips_set_tip (GTK_TOOLTIPS (general->tips), general->chk_flipping, "Allows OpenGL to swap buffers by flipping instead of the slower blitting.", NULL);
	}

	frame = gtk_frame_new("Miscellaneous");
	vbox = gtk_vbox_new(FALSE, 2);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_box_pack_start(GTK_BOX(general), frame, FALSE, FALSE, 0);
	if(GLGetAttribute(conf, dpy, 0, 0, NV_FORCE_GENERIC_CPU, &val))
	{
		general->chk_generic_cpu = gtk_check_button_new_with_label("Disable cpu optimizations");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(general->chk_generic_cpu), val);
		g_signal_connect(GTK_TOGGLE_BUTTON(general->chk_generic_cpu), "toggled", G_CALLBACK (generic_cpu_set_value), general);
		gtk_box_pack_start(GTK_BOX(vbox), general->chk_generic_cpu, FALSE, FALSE, 0);

		gtk_tooltips_set_tip (GTK_TOOLTIPS (general->tips), general->chk_generic_cpu, "When enabled cpu optimizations like MMX, 3dnow! and SSE will be disabled. Disabling this option reduces performance but helps applications like Valgrind which is a memory debugger.", NULL);
	}

	if(GLGetAttribute(conf, dpy, 0, 0, NV_OPENGL_AA_LINE_GAMMA, &val))
	{
		general->chk_aa_line_gamma = gtk_check_button_new_with_label("Enable gamma correction for antialiased lines");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(general->chk_aa_line_gamma), val);
		g_signal_connect(GTK_TOGGLE_BUTTON(general->chk_aa_line_gamma), "toggled", G_CALLBACK (aa_line_gamma_set_value), general);
		gtk_box_pack_start(GTK_BOX(vbox), general->chk_aa_line_gamma, FALSE, FALSE, 0);
	}

	if(GLGetAttribute(conf, dpy, 0, 0, NV_TEXTURE_CLAMPING, &val))
	{
		general->chk_tex_clamping = gtk_check_button_new_with_label("Texture Clamping");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(general->chk_tex_clamping), val);
		g_signal_connect(GTK_TOGGLE_BUTTON(general->chk_tex_clamping), "toggled", G_CALLBACK (tex_clamping_set_value), general);
		gtk_box_pack_start(GTK_BOX(vbox), general->chk_tex_clamping, FALSE, FALSE, 0);

		gtk_tooltips_set_tip (GTK_TOOLTIPS (general->tips), general->chk_tex_clamping, "By default the drivers use a technic for texture clamping which isn't conformant to the OpenGL specs. Enabling this option will make the drivers follow the specs. Note that on older hardware this goes at the cost of performance.", NULL);
	}

	if(GLGetAttribute(conf, dpy, 0, 0, NV_SHOW_SLI_HUD, &val))
	{
		general->chk_slihud = gtk_check_button_new_with_label("Show SLI information");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(general->chk_slihud), val);
		g_signal_connect(GTK_TOGGLE_BUTTON(general->chk_slihud), "toggled", G_CALLBACK (slihud_set_value), general);
		gtk_box_pack_start(GTK_BOX(vbox), general->chk_slihud, FALSE, FALSE, 0);

		gtk_tooltips_set_tip (GTK_TOOLTIPS (general->tips), general->chk_slihud, "When enabled the OpenGL drivers will draw information about the SLI mode on the screen.", NULL);
	}

	gtk_widget_show_all(GTK_WIDGET(general));
	return GTK_WIDGET(general);
}


GtkWidget *nv_glquality_new(config *conf, GtkTooltips *tips, Display *dpy, int screen)
{
	NVGlQuality *quality = g_object_new(NV_TYPE_GLQUALITY, NULL);
	int val;

	quality->conf = conf;
	quality->dpy = dpy;
	quality->screen = screen;
	quality->tips = tips;

	if(GLGetAttribute(conf, dpy, 0, 0, NV_LOG_ANISO, &val))
	{
		validated *res;
		int app_controlled=0;
		GtkWidget *vbox2 = gtk_vbox_new(FALSE, 2);
		GtkWidget *hbox2 = gtk_hbox_new(FALSE, 2);

		quality->frm_aniso = gtk_frame_new("Anisotropic Filtering");
		gtk_container_add(GTK_CONTAINER(quality->frm_aniso), vbox2);
		gtk_box_pack_start(GTK_BOX(quality), quality->frm_aniso, FALSE, FALSE, 0);

		quality->chk_aniso_app_control = gtk_check_button_new_with_label("Let the application decide");
		GLGetAttribute(conf, dpy, 0, 0, NV_ANISO_APP_CONTROLLED, &app_controlled);
		if(app_controlled)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(quality->chk_aniso_app_control), TRUE);
		g_signal_connect(GTK_TOGGLE_BUTTON(quality->chk_aniso_app_control), "toggled", G_CALLBACK(aniso_set_app_controlled), quality);
		gtk_container_add(GTK_CONTAINER(vbox2), quality->chk_aniso_app_control);

		NVGetValidAttributeValues(dpy, 0, 0, NV_LOG_ANISO, &res);
		quality->scale_aniso = gtk_hscale_new_with_range(0, res->data2, 1);
		g_free(res);
		gtk_range_set_value(GTK_RANGE(quality->scale_aniso), val);
		g_signal_connect(GTK_RANGE(quality->scale_aniso), "format-value", G_CALLBACK(aniso_value_to_str), quality);
		g_signal_connect(GTK_RANGE(quality->scale_aniso), "value-changed", G_CALLBACK(aniso_set_value), quality);
		gtk_container_add(GTK_CONTAINER(vbox2), quality->scale_aniso);

		quality->lbl_aniso_perf = gtk_label_new(" Performance");
		gtk_misc_set_alignment(GTK_MISC(quality->lbl_aniso_perf), 0, .5);
		gtk_container_add(GTK_CONTAINER(hbox2), quality->lbl_aniso_perf);

		quality->lbl_aniso_quality = gtk_label_new("Quality ");
		gtk_misc_set_alignment(GTK_MISC(quality->lbl_aniso_quality), 1, .5);
		gtk_container_add(GTK_CONTAINER(hbox2), quality->lbl_aniso_quality);

		gtk_container_add(GTK_CONTAINER(vbox2), hbox2);

		if(app_controlled)
		{
			gtk_widget_set_sensitive(GTK_WIDGET(quality->scale_aniso), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(quality->lbl_aniso_perf), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(quality->lbl_aniso_quality), FALSE);
		}

		gtk_tooltips_set_tip (GTK_TOOLTIPS (quality->tips), quality->chk_aniso_app_control, "By default OpenGL applications can decide themselves to use Anistropic Filtering or not. Disabling this option gives you control over what level of Anisotropic Filtering to use.", NULL);
		gtk_tooltips_set_tip (GTK_TOOLTIPS (quality->tips), quality->scale_aniso, "Anisotropic filtering is a technique that can improve the quality of textures. The only downside is that enabling this option reduces the 3D performance.", NULL);
	}

	if(GLGetAttribute(conf, dpy, 0, 0, NV_FSAA, &val))
	{
		validated *res;
		int i, n, app_controlled=0;
		GtkWidget *vbox2 = gtk_vbox_new(FALSE, 2);
		GtkWidget *hbox2 = gtk_hbox_new(FALSE, 2);

		quality->frm_fsaa = gtk_frame_new("Full Scene AntiAliasing");
		gtk_container_add(GTK_CONTAINER(quality->frm_fsaa), vbox2);
		gtk_box_pack_start(GTK_BOX(quality), quality->frm_fsaa, FALSE, FALSE, 0);

		quality->chk_fsaa_app_control = gtk_check_button_new_with_label("Let the application decide");
		GLGetAttribute(conf, dpy, 0, 0, NV_FSAA_APP_CONTROLLED, &app_controlled);
		if(app_controlled)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(quality->chk_fsaa_app_control), TRUE);
		g_signal_connect(GTK_TOGGLE_BUTTON(quality->chk_fsaa_app_control), "toggled", G_CALLBACK(fsaa_set_app_controlled), quality);
		gtk_container_add(GTK_CONTAINER(vbox2), quality->chk_fsaa_app_control);

		NVGetValidAttributeValues(dpy, 0, 0, NV_FSAA, &res);
		quality->fsaa_entries=0;
		for(i=n=0; i<=NV_FSAA_MODE_MAX; i++)
		{
			if((res->data1 >> i) & 0x1)
			{
				/* Three of the possible FSAA values are 8x (7), 8xS (9) and 16x (8).
				/  Unfortunately the option 8xS was added to the NV-CONTROL
				/  extension some time after 16x. Because of this we need to do some swapping.
				*/
				if((i == NV_FSAA_MODE_8xS) && (quality->fsaa_lst[n-1] == NV_FSAA_MODE_16x))
				{
					quality->fsaa_lst[n] = NV_FSAA_MODE_16x;
					quality->fsaa_lst[n-1] = NV_FSAA_MODE_8xS;
				}
				else if((i == NV_FSAA_MODE_8xQ) && (quality->fsaa_lst[n-1] == NV_FSAA_MODE_16x))
				{
					quality->fsaa_lst[n] = NV_FSAA_MODE_16x;
					quality->fsaa_lst[n-1] = NV_FSAA_MODE_8xQ;
				}
				else
					quality->fsaa_lst[n] = i;

				n++;
			}
		}
		g_free(res);

		quality->fsaa_entries=n;
		val = fsaa_nvcontrol_value_to_scale_value(GTK_WIDGET(quality), val);
		quality->scale_fsaa = gtk_hscale_new_with_range(0, n-1, 1);
		gtk_range_set_value(GTK_RANGE(quality->scale_fsaa), val);

		g_signal_connect(GTK_RANGE(quality->scale_fsaa), "format-value", G_CALLBACK(fsaa_value_to_str), quality);
		g_signal_connect(GTK_RANGE(quality->scale_fsaa), "value-changed", G_CALLBACK(fsaa_set_value), quality);
		gtk_container_add(GTK_CONTAINER(vbox2), quality->scale_fsaa);

		quality->lbl_fsaa_perf = gtk_label_new(" Performance");
		gtk_misc_set_alignment(GTK_MISC(quality->lbl_fsaa_perf), 0, .5);
		gtk_container_add(GTK_CONTAINER(hbox2), quality->lbl_fsaa_perf);

		quality->lbl_fsaa_quality = gtk_label_new("Quality ");
		gtk_misc_set_alignment(GTK_MISC(quality->lbl_fsaa_quality), 1, .5);
		gtk_container_add(GTK_CONTAINER(hbox2), quality->lbl_fsaa_quality);

		gtk_container_add(GTK_CONTAINER(vbox2), hbox2);

		if(app_controlled)
		{
			gtk_widget_set_sensitive(GTK_WIDGET(quality->scale_fsaa), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(quality->lbl_fsaa_perf), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(quality->lbl_fsaa_quality), FALSE);
		}

		gtk_tooltips_set_tip (GTK_TOOLTIPS (quality->tips), quality->chk_fsaa_app_control, "By default OpenGL applications can decide themselves to use AntiAliasing or not. Disabling this option gives you control over what level of AntiAliasing to use.", NULL);
		gtk_tooltips_set_tip (GTK_TOOLTIPS (quality->tips), quality->scale_fsaa, "Full Scene AntiAliasing is a technique that makes the edges of 3D objects smoother. For instance it removes the stairsteps which you often see along the edges of objects. Enabling this option can greatly improve the image quality of applications but it also goes at the cost a big performance drop depending on the level of FSAA.", NULL);
	}

	if(GLGetAttribute(conf, dpy, 0, 0, NV_IMAGE_QUALITY, &val))
	{
		quality->frm_image_quality = gtk_frame_new("Image Quality");
		gtk_box_pack_start(GTK_BOX(quality), quality->frm_image_quality, FALSE, FALSE, 0);

		quality->scale_image_quality = gtk_hscale_new_with_range(0, 3, 1);
		gtk_range_set_value(GTK_RANGE(quality->scale_image_quality), val);
		g_signal_connect(GTK_RANGE(quality->scale_image_quality), "format-value", G_CALLBACK(image_quality_value_to_str), quality);
		g_signal_connect(GTK_RANGE(quality->scale_image_quality), "value-changed", G_CALLBACK(image_quality_set_value), quality);
		gtk_container_add(GTK_CONTAINER(quality->frm_image_quality), quality->scale_image_quality);
	}
	
	if(GLGetAttribute(conf, dpy, 0, 0, NV_TEXTURE_SHARPEN, &val))
	{
		quality->frm_tex_sharpen = gtk_frame_new("Texture Quality");
		gtk_box_pack_start(GTK_BOX(quality), quality->frm_tex_sharpen, FALSE, FALSE, 0);

		quality->chk_tex_sharpen = gtk_check_button_new_with_label("Sharpen textures");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(quality->chk_tex_sharpen), val);
		g_signal_connect(GTK_TOGGLE_BUTTON(quality->chk_tex_sharpen), "toggled", G_CALLBACK (tex_sharpen_set_value), quality);
		gtk_container_add(GTK_CONTAINER(quality->frm_tex_sharpen), quality->chk_tex_sharpen);

		gtk_tooltips_set_tip (GTK_TOOLTIPS (quality->tips), quality->chk_tex_sharpen, "Enabling this option improves the sharpness of textures in case Full Scene AntiAliasing is enabled.", NULL);
	}

	gtk_widget_show_all(GTK_WIDGET(quality));
	return GTK_WIDGET(quality);
}


gboolean nvevent_check(GSource *source)
{
	NVSource *nvsource = (NVSource*)source;
	return XPending(nvsource->dpy);
}


gboolean nvevent_dispatch(GSource *source,GSourceFunc callback, gpointer user_data)
{
	XEvent event;
	NVSource *nvsource = (NVSource*)source;
	XNextEvent(nvsource->dpy, &event);

	if(event.type == nvsource->event_base)
	{
		NVOpengl *opengl = NV_OPENGL(nvsource->widget);
		NVAttributeChangedEvent *nvevent = (NVAttributeChangedEvent *) &event;

		/* Check if the option that changed is one we show on the gui. If it is we first
		/  block the changed events on the corresponding widget to make sure we don't change
		/  the option ourselves. After this we update the value on the gui and unblock the
		/  changed events.
		*/
		switch(nvevent->option)
		{
			case NV_FLIPPING_ALLOWED:
				g_signal_handlers_block_by_func(NV_GLGENERAL(opengl->general)->chk_flipping, flipping_set_value, NV_GLGENERAL(opengl->general));
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(NV_GLGENERAL(opengl->general)->chk_flipping), nvevent->value);
				g_signal_handlers_unblock_by_func(NV_GLGENERAL(opengl->general)->chk_flipping, flipping_set_value, NV_GLGENERAL(opengl->general));
				break;
			case NV_FORCE_GENERIC_CPU:
				g_signal_handlers_block_by_func(NV_GLGENERAL(opengl->general)->chk_generic_cpu, generic_cpu_set_value, NV_GLGENERAL(opengl->general));
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(NV_GLGENERAL(opengl->general)->chk_generic_cpu), nvevent->value);
				g_signal_handlers_unblock_by_func(NV_GLGENERAL(opengl->general)->chk_generic_cpu, generic_cpu_set_value, NV_GLGENERAL(opengl->general));
				break;
			case NV_FSAA:
				g_signal_handlers_block_by_func(NV_GLQUALITY(opengl->quality)->scale_fsaa, fsaa_set_value, NV_GLQUALITY(opengl->quality));
				gtk_range_set_value(GTK_RANGE(NV_GLQUALITY(opengl->quality)->scale_fsaa), fsaa_nvcontrol_value_to_scale_value(opengl->quality, nvevent->value));
				g_signal_handlers_unblock_by_func(NV_GLQUALITY(opengl->quality)->scale_fsaa, fsaa_set_value, NV_GLQUALITY(opengl->quality));
				break;
			case NV_FSAA_APP_CONTROLLED:
				g_signal_handlers_block_by_func(NV_GLQUALITY(opengl->quality)->chk_fsaa_app_control, fsaa_set_app_controlled, NV_GLQUALITY(opengl->quality));
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(NV_GLQUALITY(opengl->quality)->chk_fsaa_app_control), nvevent->value);
				g_signal_handlers_unblock_by_func(NV_GLQUALITY(opengl->quality)->chk_fsaa_app_control, fsaa_set_app_controlled, NV_GLQUALITY(opengl->quality));
				break;
			case NV_IMAGE_QUALITY:
				g_signal_handlers_block_by_func(NV_GLQUALITY(opengl->quality)->scale_image_quality, image_quality_set_value, NV_GLQUALITY(opengl->quality));
				gtk_range_set_value(GTK_RANGE(NV_GLQUALITY(opengl->quality)->scale_image_quality), nvevent->value);
				g_signal_handlers_unblock_by_func(NV_GLQUALITY(opengl->quality)->scale_image_quality, image_quality_set_value, NV_GLQUALITY(opengl->quality));
				break;
			case NV_LOG_ANISO:
				g_signal_handlers_block_by_func(NV_GLQUALITY(opengl->quality)->scale_aniso, aniso_set_value, NV_GLQUALITY(opengl->quality));
				gtk_range_set_value(GTK_RANGE(NV_GLQUALITY(opengl->quality)->scale_aniso), nvevent->value);
				g_signal_handlers_unblock_by_func(NV_GLQUALITY(opengl->quality)->scale_aniso, aniso_set_value, NV_GLQUALITY(opengl->quality));
				break;
			case NV_ANISO_APP_CONTROLLED:
				g_signal_handlers_block_by_func(NV_GLQUALITY(opengl->quality)->chk_aniso_app_control, aniso_set_app_controlled, NV_GLQUALITY(opengl->quality));
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(NV_GLQUALITY(opengl->quality)->chk_aniso_app_control), nvevent->value);
				g_signal_handlers_unblock_by_func(NV_GLQUALITY(opengl->quality)->chk_aniso_app_control, aniso_set_app_controlled, NV_GLQUALITY(opengl->quality));
				break;
			case NV_OPENGL_AA_LINE_GAMMA:
				g_signal_handlers_block_by_func(NV_GLGENERAL(opengl->general)->chk_aa_line_gamma, aa_line_gamma_set_value, NV_GLGENERAL(opengl->general));
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(NV_GLGENERAL(opengl->general)->chk_aa_line_gamma), nvevent->value);
				g_signal_handlers_unblock_by_func(NV_GLGENERAL(opengl->general)->chk_aa_line_gamma, aa_line_gamma_set_value, NV_GLGENERAL(opengl->general));
				break;
			case NV_SYNC_VBLANK:
				g_signal_handlers_block_by_func(NV_GLGENERAL(opengl->general)->chk_vsync, vsync_set_value, NV_GLGENERAL(opengl->general));
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(NV_GLGENERAL(opengl->general)->chk_vsync), nvevent->value);
				g_signal_handlers_unblock_by_func(NV_GLGENERAL(opengl->general)->chk_vsync, vsync_set_value, NV_GLGENERAL(opengl->general));
				break;
			case NV_SHOW_SLI_HUD:
				g_signal_handlers_block_by_func(NV_GLGENERAL(opengl->general)->chk_slihud, slihud_set_value, NV_GLGENERAL(opengl->general));
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(NV_GLGENERAL(opengl->general)->chk_slihud), nvevent->value);
				g_signal_handlers_unblock_by_func(NV_GLGENERAL(opengl->general)->chk_slihud, slihud_set_value, NV_GLGENERAL(opengl->general));
				break;
			case NV_TEXTURE_CLAMPING:
				g_signal_handlers_block_by_func(NV_GLGENERAL(opengl->general)->chk_tex_clamping, tex_clamping_set_value, NV_GLGENERAL(opengl->general));
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(NV_GLGENERAL(opengl->general)->chk_tex_clamping), nvevent->value);
				g_signal_handlers_unblock_by_func(NV_GLGENERAL(opengl->general)->chk_generic_cpu, tex_clamping_set_value, NV_GLGENERAL(opengl->general));
				break;
			case NV_TEXTURE_SHARPEN:
				g_signal_handlers_block_by_func(NV_GLQUALITY(opengl->quality)->chk_tex_sharpen, tex_sharpen_set_value, NV_GLQUALITY(opengl->quality));
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(NV_GLQUALITY(opengl->quality)->chk_tex_sharpen), nvevent->value);
				g_signal_handlers_unblock_by_func(NV_GLQUALITY(opengl->quality)->chk_tex_sharpen, tex_sharpen_set_value, NV_GLQUALITY(opengl->quality));
				break;
		}
	}
	return TRUE;
}


gboolean nvevent_prepare(GSource *source, gint *timeout)
{
	*timeout = -1;
	return FALSE;
}


void nv_enable_nvcontrol_events(Display *dpy, GtkWidget *widget, int event_base)
{
	NVSource *source;
	static GSourceFuncs funcs = { nvevent_prepare, nvevent_check, nvevent_dispatch, NULL };

	/* We will poll the current X display for NV-CONTROL events.
	/  The glib poll function receives a modded GSource structure
	/  containing the display to watch, the gtk widget to update
	/  and finally a filedescriptor to poll.
	/
	/  dpy, screen, type (0=changes), toggle (true/false)
	*/
	NVSelectNotify(dpy, 0, 0, 1);

	source = (NVSource*)g_source_new(&funcs , sizeof(NVSource));
	source->dpy = dpy;
	source->widget = widget;
	source->fd.fd = ConnectionNumber(dpy);
	source->fd.events = G_IO_IN;
	source->event_base = event_base;

	g_source_add_poll((GSource*)source, &source->fd);
	g_source_attach((GSource*)source, NULL);
}


int gui_gl_init(config *conf, GtkTooltips *tips)
{
	GtkTreeIter grandparent, parent;
	NVOpengl *opengl;
	NVGlGeneral *general;
	NVGlQuality *quality;
	void *libgl;
	int event_base, error_base;

	Display *dpy = XOpenDisplay("");

	libgl = dlopen("libGL.so.1", RTLD_LAZY);
	if(libgl == NULL)
		return 0;

	pglGetString = dlsym(libgl, "glGetString");
	pglXChooseVisual = dlsym(libgl, "glXChooseVisual");
	pglXCreateContext = dlsym(libgl, "glXCreateContext");
	pglXDestroyContext = dlsym(libgl, "glXDestroyContext");
	pglXIsDirect = dlsym(libgl, "glXIsDirect");
	pglXMakeCurrent = dlsym(libgl, "glXMakeCurrent");
	pglXQueryServerString = dlsym(libgl, "glXQueryServerString");

	/* If opengl stuff isn't supported don't show it on the gui */
	if(!init_nvcontrol(dpy))
	{
		GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", "Can't find NV-CONTROL extension!\nDisabling OpenGL tweaking..");
		gtk_dialog_run(GTK_DIALOG(dialog));
		g_signal_connect_swapped(GTK_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), GTK_OBJECT(dialog));
		gtk_widget_destroy(dialog);
		return 0;
	}

	/* Retrieve the event_base at which NV-CONTROL is located.
	/  This is needed to be able to receive changes done by other NV-CONTROL clients.
	*/
	if(!NVQueryExtension(dpy, &event_base, &error_base))
		return 0;

	initialize=1;
	opengl = (NVOpengl*)nv_opengl_new(dpy, 0);
	add(&grandparent, NULL, "OpenGL", BANNER_GL, opengl);
	general = (NVGlGeneral*)nv_glgeneral_new(conf, tips, dpy, 0);
	add(&parent, &grandparent, "General", BANNER_GL, general);
	quality = (NVGlQuality*)nv_glquality_new(conf, tips, dpy, 0);
	add(&parent, &grandparent, "Image Quality", BANNER_GL, quality);
	initialize=0;

	opengl->general = (GtkWidget*)general;
	opengl->quality = (GtkWidget*)quality;

	nv_enable_nvcontrol_events(dpy, (GtkWidget*)opengl, event_base);
	return 1;
}

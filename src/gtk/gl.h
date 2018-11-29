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
 * Header containing datastructures for building the OpenGL pages
 */

#ifndef NVGL_H
#define NVGL_H

#include "interface.h"
#include "nvclock.h"
#include <gdk/gdk.h>
#include <gtk/gtkmisc.h>

typedef XID GLXDrawable;
typedef void *GLXContext;

#define GLX_VENDOR 0x1
#define GLX_VERSION 0x2

#define GLX_RGBA 4
#define GLX_DOUBLEBUFFER 5
#define GLX_RED_SIZE 8
#define GLX_GREEN_SIZE 9
#define GLX_BLUE_SIZE 10

#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02

typedef struct
{
	GSource source;
	Display *dpy;
	GtkWidget *widget;
	GPollFD fd;
	int event_base;
} NVSource;

#define NV_TYPE_OPENGL                  (nv_opengl_get_type ())
#define NV_OPENGL(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), NV_TYPE_OPENGL, NVOpengl))
#define NV_OPENGL_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), NV_TYPE_OPENGL, NVOpenglClass))
#define NV_IS_OPENGL(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NV_TYPE_OPENGL))
#define NV_IS_OPENGL_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), NV_TYPE_OPENGL))
#define NV_OPENGL_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), NV_TYPE_OPENGL, NVOpenglClass))

#define NV_TYPE_GLGENERAL                  (nv_glgeneral_get_type ())
#define NV_GLGENERAL(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), NV_TYPE_GLGENERAL, NVGlGeneral))
#define NV_GLGENERAL_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), NV_TYPE_GLGENERAL, NVGlGeneralClass))
#define NV_IS_GLGENERAL(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NV_TYPE_GLGENERAL))
#define NV_IS_GLGENERAL_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), NV_TYPE_GLGENERAL))
#define NV_GLGENERAL_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), NV_TYPE_GLGENERAL, NVGlGeneralClass))

#define NV_TYPE_GLQUALITY                  (nv_glquality_get_type ())
#define NV_GLQUALITY(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), NV_TYPE_GLQUALITY, NVGlQuality))
#define NV_GLQUALITY_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), NV_TYPE_GLQUALITY, NVGlQualityClass))
#define NV_IS_GLQUALITY(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NV_TYPE_GLQUALITY))
#define NV_IS_GLQUALITY_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), NV_TYPE_GLQUALITY))
#define NV_GLQUALITY_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), NV_TYPE_GLQUALITY, NVGlQualityClass))

struct _NVOpengl
{
	GtkVBox parent;

	Display *dpy;
	int screen;

	GtkWidget *general;
	GtkWidget *quality;

	GtkWidget *frm_glx;
	GtkWidget *tbl_glx;
	GtkWidget *lbl_glx_vendor;
	GtkWidget *lbl_glx_vendor_txt;
	GtkWidget *lbl_glx_version;
	GtkWidget *lbl_glx_version_txt;
	GtkWidget *lbl_glx_dri;
	GtkWidget *lbl_glx_dri_txt;

	GtkWidget *frm_gl;
	GtkWidget *tbl_gl;
	GtkWidget *lbl_gl_vendor;
	GtkWidget *lbl_gl_vendor_txt;
	GtkWidget *lbl_gl_renderer;
	GtkWidget *lbl_gl_renderer_txt;
	GtkWidget *lbl_gl_version;
	GtkWidget *lbl_gl_version_txt;
};

struct _NVGlGeneral
{
	GtkVBox parent;

	Display *dpy;
	int screen;

	config *conf;
	GtkTooltips *tips;

	GtkWidget *chk_aa_line_gamma;
	GtkWidget *chk_generic_cpu;
	GtkWidget *chk_flipping;
	GtkWidget *chk_vsync;
	GtkWidget *chk_tex_clamping;
	GtkWidget *chk_slihud;
};

struct _NVGlQuality
{
	GtkVBox parent;

	Display *dpy;
	int screen;

	config *conf;
	GtkTooltips *tips;

	GtkWidget *frm_fsaa;
	GtkWidget *chk_fsaa_app_control;
	GtkWidget *scale_fsaa;
	GtkWidget *lbl_fsaa_perf;
	GtkWidget *lbl_fsaa_quality;
	int fsaa_entries;
	int fsaa_lst[9];

	GtkWidget *frm_aniso;
	GtkWidget *chk_aniso_app_control;
	GtkWidget *scale_aniso;
	GtkWidget *lbl_aniso_perf;
	GtkWidget *lbl_aniso_quality;

	GtkWidget *frm_image_quality;
	GtkWidget *scale_image_quality;

	GtkWidget *frm_tex_sharpen;
	GtkWidget *chk_tex_sharpen;
};

struct _NVOpenglClass
{
	GtkVBoxClass parent_class;
};

struct _NVGlGeneralClass
{
	GtkVBoxClass parent_class;
};

struct _NVGlQualityClass
{
	GtkVBoxClass parent_class;
};

typedef struct _NVOpengl       NVOpengl;
typedef struct _NVOpenglClass  NVOpenglClass;

typedef struct _NVGlGeneral       NVGlGeneral;
typedef struct _NVGlGeneralClass  NVGlGeneralClass;

typedef struct _NVGlQuality       NVGlQuality;
typedef struct _NVGlQualityClass  NVGlQualityClass;

GType      nv_opengl_get_type   (void) G_GNUC_CONST;
GtkWidget* nv_opengl_new     (Display *dpy, int screen);

GType      nv_glgeneral_get_type   (void) G_GNUC_CONST;
GtkWidget* nv_glgeneral_new     (config *conf, GtkTooltips *tooltips, Display *dpy, int screen);

GType      nv_glquality_get_type   (void) G_GNUC_CONST;
GtkWidget* nv_glquality_new     (config *conf, GtkTooltips *tooltips, Display *dpy, int screen);
#endif /* NVGL_H */

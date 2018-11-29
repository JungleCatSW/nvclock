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
 * Header containing datastructures for building the Hardware page
 */

#include "interface.h"
#include "nvclock.h"
#ifdef HAVE_NVCONTROL
#include "nvcontrol.h"
#endif

#include <gdk/gdk.h>
#include <gtk/gtkmisc.h>

#define NV_TYPE_AGP                  (nv_agp_get_type ())
#define NV_AGP(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), NV_TYPE_AGP, NVAgp))
#define NV_AGP_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), NV_TYPE_AGP, NVAgpClass))
#define NV_IS_AGP(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NV_TYPE_AGP))
#define NV_IS_AGP_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), NV_TYPE_AGP))
#define NV_AGP_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), NV_TYPE_AGP, NVAgpClass))

#define NV_TYPE_BIOS                  (nv_bios_get_type ())
#define NV_BIOS(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), NV_TYPE_BIOS, NVBios))
#define NV_BIOS_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), NV_TYPE_BIOS, NVBiosClass))
#define NV_IS_BIOS(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NV_TYPE_BIOS))
#define NV_IS_BIOS_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), NV_TYPE_BIOS))
#define NV_BIOS_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), NV_TYPE_BIOS, NVBiosClass))

#define NV_TYPE_INFO                  (nv_info_get_type ())
#define NV_INFO(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), NV_TYPE_INFO, NVInfo))
#define NV_INFO_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), NV_TYPE_INFO, NVInfoClass))
#define NV_IS_INFO(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NV_TYPE_INFO))
#define NV_IS_INFO_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), NV_TYPE_INFO))
#define NV_INFO_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), NV_TYPE_INFO, NVInfoClass))

#define NV_TYPE_OVERCLOCK                  (nv_overclock_get_type ())
#define NV_OVERCLOCK(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), NV_TYPE_OVERCLOCK, NVOverclock))
#define NV_OVERCLOCK_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), NV_TYPE_OVERCLOCK, NVOverclockClass))
#define NV_IS_OVERCLOCK(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NV_TYPE_OVERCLOCK))
#define NV_IS_OVERCLOCK_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), NV_TYPE_OVERCLOCK))
#define NV_OVERCLOCK_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), NV_TYPE_OVERCLOCK, NVOverclockClass))

#define NV_TYPE_PIPELINE                  (nv_pipeline_get_type ())
#define NV_PIPELINE(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), NV_TYPE_PIPELINE, NVPipeline))
#define NV_PIPELINE_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), NV_TYPE_PIPELINE, NVPipelineClass))
#define NV_IS_PIPELINE(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NV_TYPE_PIPELINE))
#define NV_IS_PIPELINE_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), NV_TYPE_PIPELINE))
#define NV_PIPELINE_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), NV_TYPE_PIPELINE, NVPipelineClass))

#define NV_TYPE_SHADER                  (nv_shader_get_type ())
#define NV_SHADER(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), NV_TYPE_SHADER, NVShader))
#define NV_SHADER_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), NV_TYPE_SHADER, NVShaderClass))
#define NV_IS_SHADER(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NV_TYPE_SHADER))
#define NV_IS_SHADER_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), NV_TYPE_SHADER))
#define NV_SHADER_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), NV_TYPE_SHADER, NVPipelineClass))

#define NV_TYPE_THERMAL                  (nv_thermal_get_type ())
#define NV_THERMAL(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), NV_TYPE_THERMAL, NVThermal))
#define NV_THERMAL_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), NV_TYPE_THERMAL, NVThermalClass))
#define NV_IS_THERMAL(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NV_TYPE_THERMAL))
#define NV_IS_THERMAL_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), NV_TYPE_THERMAL))
#define NV_THERMAL_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), NV_TYPE_THERMAL, NVThermalClass))

typedef struct
{
	int timeout_id;
	int time;
	GtkDialog *dialog;
	GtkWidget *label; /* The label containing the time */
} Timeout;

typedef struct _NVAgp       NVAgp;
typedef struct _NVAgpClass  NVAgpClass;

typedef struct _NVBios       NVBios;
typedef struct _NVBiosClass  NVBiosClass;

typedef struct _NVInfo       NVInfo;
typedef struct _NVInfoClass  NVInfoClass;

typedef struct _NVOverclock       NVOverclock;
typedef struct _NVOverclockClass  NVOverclockClass;

typedef struct _NVPipeline  NVPipeline;
typedef struct _NVPipelineClass NVPipelineClass;

typedef struct _NVShader       NVShader;
typedef struct _NVShaderClass  NVShaderClass;

typedef struct _NVThermal       NVThermal;
typedef struct _NVThermalClass  NVThermalClass;

struct _NVAgp
{
	GtkVBox parent;

	NVCard *card; /* Nvidia card */

	GtkWidget *frame;
	GtkWidget *table;

	GtkWidget *lbl_agp;
	GtkWidget *lbl_agp_txt;
	GtkWidget *lbl_agprate;
	GtkWidget *lbl_agprate_txt;
	GtkWidget *lbl_agprates;
	GtkWidget *lbl_agprates_txt;
	GtkWidget *lbl_fw;
	GtkWidget *lbl_fw_txt;
	GtkWidget *lbl_sba;
	GtkWidget *lbl_sba_txt;
};

struct _NVBios
{
	GtkVBox parent;

	GtkWidget *frame;
	GtkWidget *table;

	GtkWidget *lbl_signonmsg;
	GtkWidget *lbl_signonmsg_txt;
	GtkWidget *lbl_perf;
	GtkWidget *lbl_volt;

	GtkWidget *tree_view_perf;
	GtkCellRenderer *renderer_perf;
	GtkTreeViewColumn *column_perf;
	GtkTreeStore *store_perf;

	GtkWidget *tree_view_volt;
	GtkCellRenderer *renderer_volt;
	GtkTreeViewColumn *column_volt;
	GtkTreeStore *store_volt;
};

struct _NVInfo
{
	GtkVBox parent;

	NVCard *card; /* Nvidia card */

	GtkWidget *frame;
	GtkWidget *table;

	GtkWidget *lbl_gpu;
	GtkWidget *lbl_gpu_txt;
	GtkWidget *lbl_arch;
	GtkWidget *lbl_arch_txt;
	GtkWidget *lbl_bustype;
	GtkWidget *lbl_bustype_txt;
	GtkWidget *lbl_bios;
	GtkWidget *lbl_bios_txt;
	GtkWidget *lbl_vidmem;
	GtkWidget *lbl_vidmem_txt;
	GtkWidget *lbl_memtype;
	GtkWidget *lbl_memtype_txt;
	GtkWidget *lbl_irq;
	GtkWidget *lbl_irq_txt;
};

struct _NVOverclock
{
	GtkVBox parent;

	NVCard *card; /* Nvidia card */
	config *conf;
	float memclk;
	float nvclk;
	int clock_changes;
#ifdef HAVE_NVCONTROL
	Display *dpy;
#endif
	int have_coolbits;
	GtkTooltips *tips;

	GtkWidget *frame;
	GtkWidget *chk_overclock;
	GtkWidget *combo_speeds;
	GtkWidget *vbox;

	GtkWidget *frm_gpu;
	GtkWidget *frm_mem;
	GtkWidget *scale_gpu;
	GtkWidget *scale_mem;
	GtkWidget *chk_test_speeds;
	GtkWidget *bbox;
	GtkWidget *btn_change_speeds;
	GtkWidget *btn_reset_speeds;
};

struct _NVPipeline
{
	GtkVBox parent;

	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *lbl_pipes;
	GtkWidget *lbl_pipes_txt;
	GtkWidget *lbl_config;
	GtkWidget *lbl_config_txt;

	GtkWidget *tree_view;
	GtkWidget *scrolled_window;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeStore *store;
};

struct _NVShader
{
	GtkVBox parent;

	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *lbl_clock;
	GtkWidget *lbl_clock_txt;
	GtkWidget *lbl_streamunits;
	GtkWidget *lbl_streamunits_txt;
	GtkWidget *lbl_ropunits;
	GtkWidget *lbl_ropunits_txt;
	GtkWidget *lbl_config;
	GtkWidget *lbl_config_txt;
};

struct _NVThermal
{
	GtkVBox parent;

	NVCard *card; /* Nvidia card */
	config *conf;
	int timeout;
	GtkTooltips *tips;

	GtkWidget *frame;
	GtkWidget *vbox;

	GtkWidget *lbl_sensor;
	GtkWidget *lbl_sensor_txt;

	GtkWidget *frm_core;
	GtkWidget *pbar_core;
	GtkWidget *lbl_core;

	GtkWidget *frm_ambient;
	GtkWidget *pbar_ambient;
	GtkWidget *lbl_ambient;

	GtkWidget *chk_fanspeed;
	GtkWidget *frm_fanspeed;
	GtkWidget *lbl_fanspeed;
	GtkWidget *pbar_fanspeed;

	GtkWidget *frm_dutycycle;
	GtkWidget *vbx_dutycycle;
	GtkWidget *scale_dutycycle;

	GtkWidget *bbox;
	GtkWidget *btn_apply;
};

struct _NVAgpClass
{
	GtkVBoxClass parent_class;
};

struct _NVBiosClass
{
	GtkVBoxClass parent_class;
};

struct _NVInfoClass
{
	GtkVBoxClass parent_class;
};

struct _NVOverclockClass
{
	GtkVBoxClass parent_class;
};

struct _NVPipelineClass
{
	GtkVBoxClass parent_class;
};

struct _NVShaderClass
{
	GtkVBoxClass parent_class;
};

struct _NVThermalClass
{
	GtkVBoxClass parent_class;
};

GType      nv_agp_get_type   (void) G_GNUC_CONST;
GtkWidget* nv_agp_new        (NVCard *card);

GType      nv_bios_get_type   (void) G_GNUC_CONST;
GtkWidget* nv_bios_new        (NVCard *card);

GType      nv_info_get_type   (void) G_GNUC_CONST;
GtkWidget* nv_info_new        (NVCard *card);

GType      nv_overclock_get_type   (void) G_GNUC_CONST;
GtkWidget* nv_overclock_new        (config *conf, GtkTooltips *tips, NVCard *card, void* dpy);

GType      nv_pipeline_get_type   (void) G_GNUC_CONST;
GtkWidget* nv_pipeline_new        (config *conf, NVCard *card);

GType      nv_shader_get_type   (void) G_GNUC_CONST;
GtkWidget* nv_shader_new        (config *conf, NVCard *card);

GType      nv_thermal_get_type   (void) G_GNUC_CONST;
GtkWidget* nv_thermal_new     (config *conf, GtkTooltips *tips, NVCard *card);

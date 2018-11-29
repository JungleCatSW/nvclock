/* NVClock 0.8 - Linux overclocker for NVIDIA cards
 * 
 * Copyright(C) 2001-2007 Roderick Colenbrander
 *
 * site: http://NVClock.sourceforge.net
 *
 *
 * The nvEvent, NVEvent, and NVAttributeChangedEvent structures
 * were copied from Nvidia's GPLed nvidia-settings to allow NVClock
 * to receive notification of NV-CONTROL update events. To make Nvidia happy:
 * 	nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * 	and Linux systems.
 *
 * 	Copyright (C) 2004 NVIDIA Corporation.
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


#ifndef LIBNVCONTROL_H
#define LIBNVCONTROL_H

#include <X11/Xlib.h>
#include <X11/Xmd.h>

#define nv_QueryVersion 0
#define nv_GetAttribute 2
#define nv_SetAttribute 3
#define nv_GetStringAttribute 4
#define nv_GetValidAttributeValues 5
#define nv_SelectNotify 6

#define NV_CRT  0xff
#define NV_TV   0xff00
#define NV_DFP  0xff0000

#define CRT_0           0x1
#define CRT_1           0x2
#define CRT_2           0x4
#define CRT_3           0x8
#define CRT_4           0x10
#define CRT_5           0x20
#define CRT_6           0x40
#define CRT_7           0x80
#define TV_0            0x100
#define TV_1            0x200
#define TV_2            0x400
#define TV_3            0x800
#define TV_4            0x1000
#define TV_5            0x2000
#define TV_6            0x4000
#define TV_7            0x8000
#define DFP_0           0x10000
#define DFP_1           0x20000
#define DFP_2           0x40000
#define DFP_3           0x80000
#define DFP_4           0x100000
#define DFP_5           0x200000
#define DFP_6           0x400000
#define DFP_7           0x800000


enum {
NV_PAGEFLIP = 0,
NV_WINDOWFLIP,
NV_FLATPANEL_SCALE,
NV_FLATPANEL_DITHERING,
NV_DIGITALVIBRANCE,
NV_BUSTYPE,
NV_VIDEORAM,
NV_IRQ,
NV_OPERATING_SYSTEM,
NV_SYNC_VBLANK,
NV_LOG_ANISO, 
NV_FSAA,
NV_TEXTURE_SHARPEN,
NV_UBB,
NV_OVERLAY,
NV_CLIPIDS,
NV_STEREO,
NV_EMULATE,
NV_TWINVIEW,
NV_CONNECTED_DISPLAYS, /* Returns a bitmask DFP[7-0],TV[7-0],CRT[7-0] */
NV_ENABLED_DISPLAYS, /* Returns a bitmask DFP[7-0],TV[7-0],CRT[7-0] */
NV_OPENGL_QUALITY_ENHANCHEMENTS = 36,
NV_FORCE_GENERIC_CPU,
NV_OPENGL_AA_LINE_GAMMA,
NV_FLIPPING_ALLOWED=40,
NV_CPU_ARCH,
NV_TEXTURE_CLAMPING,
NV_CURSOR_SHADOW,
NV_CURSOR_SHADOW_ALPHA,
NV_CURSOR_SHADOW_RED,
NV_CURSOR_SHADOW_GREEN,
NV_CURSOR_SHADOW_BLUE,
NV_CURSOR_SHADOW_X_OFFSET,
NV_CURSOR_SHADOW_Y_OFFSET,
NV_FSAA_APP_CONTROLLED,
NV_ANISO_APP_CONTROLLED,
NV_IMAGE_SHARPENING,
NV_TV_OVERSCAN,
NV_TV_FLICKER,
NV_TV_BRIGHTNESS,
NV_TV_HUE,
NV_TV_CONTRAST,
NV_TV_SATURATION,
NV_TV_RESET,
NV_GPU_TEMPERATURE,
NV_GPU_THRESHOLD,
NV_GPU_DEFAULT_THRESHOLD,
NV_GPU_MAX_THRESHOLD,
NV_AMBIENT_TEMPERATURE,
NV_GPU_OVERCLOCKING_STATE=88,
NV_GPU_2D_CLOCK,
NV_GPU_3D_CLOCK,
NV_GPU_DEFAULT_2D_CLOCK,
NV_GPU_DEFAULT_3D_CLOCK,
NV_GPU_CURRENT_CLOCK,
NV_GPU_OPTIMAL_CLOCK,
NV_GPU_OPTIMAL_CLOCK_DETECTION,
NV_GPU_OPTIMAL_CLOCK_DETECTION_STATE,
NV_IMAGE_QUALITY=221,
NV_SHOW_SLI_HUD=225,
NV_FSAA_APP_ENHANCED=255
};

#define NV_FSAA_MODE_NONE 0
#define NV_FSAA_MODE_2x 1
#define NV_FSAA_MODE_2x_5t 2
#define NV_FSAA_MODE_15x15 3
#define NV_FSAA_MODE_2x2 4
#define NV_FSAA_MODE_4x 5
#define NV_FSAA_MODE_4x_9t 6
#define NV_FSAA_MODE_8x 7
#define NV_FSAA_MODE_16x 8
#define NV_FSAA_MODE_8xS 9
#define NV_FSAA_MODE_8xQ 10
#define NV_FSAA_MODE_16xS 11
#define NV_FSAA_MODE_16xQ 12
#define NV_FSAA_MODE_32xS 13
#define NV_FSAA_MODE_MAX NV_FSAA_MODE_32xS

enum {
NV_STR_CARDTYPE = 0,
NV_STR_VBIOS,
NV_STR_CRASHX, /* Using this seems to crash x, so don't use it */
NV_STR_VERSION,
NV_STR_DEVICE_NAME,
NV_STR_TV_ENC
};

#define NVGetReq(name, req) \
    WORD64ALIGN\
    if ((dpy->bufptr + SIZEOF(xNV##name##Req)) > dpy->bufmax)\
	_XFlush(dpy);\
    req = (xNV##name##Req *)(dpy->last_req = dpy->bufptr);\
    req->reqType = dpyinfo->codes->major_opcode;\
    req->nvReqType = nv_##name; \
    req->length = (SIZEOF(xNV##name##Req))>>2;\
    dpy->bufptr += SIZEOF(xNV##name##Req);\
    dpy->request++

typedef struct {
    unsigned char reqType;
    unsigned char nvReqType;
    short length;
} xNVQueryVersionReq;
#define sz_xNVQueryVersionReq 4

typedef struct {
    unsigned char reqType;
    unsigned char nvReqType;
    short length ;
    int screen; /* X screen */
    unsigned int disp_mask; /* display number as a bitmask; 1 for primary CRT-0; 256 for TV-0 .. */
    unsigned int option;
} xNVGetAttributeReq;
#define sz_xNVGetAttributeReq 16

typedef struct {
    unsigned char reqType;
    unsigned char nvReqType;
    short length ;
    int screen;
    unsigned int disp_mask;
    unsigned int option;
    int	value;
} xNVSetAttributeReq;
#define sz_xNVSetAttributeReq 20

typedef struct {
    unsigned char reqType;
    unsigned char nvReqType;
    short length;
    int screen;
    unsigned int disp_mask;
    unsigned int option;
} xNVGetStringAttributeReq;
#define sz_xNVGetStringAttributeReq 16

typedef struct {
    unsigned char reqType;
    unsigned char nvReqType;
    short length ;
    int screen;
    unsigned int disp_mask;
    unsigned int option;
} xNVGetValidAttributeValuesReq;
#define sz_xNVGetValidAttributeValuesReq 16

#if 0
typedef struct {
    unsigned char reqType;
    unsigned char nvReqType;
    short length;
    int screen;
    short type;
    short toggle; /* enable/disable event notification */
} xNVSelectNotifyReq;
#endif

typedef struct {
    CARD8 reqType;
    CARD8 nvReqType;
    CARD16 length B16;
    CARD32 screen B32;
    CARD16 type B16;
    CARD16 toggle B16;
} xNVSelectNotifyReq;

#define sz_xNVSelectNotifyReq 12

typedef struct {
    unsigned char type; /* X_Reply */
    unsigned char data;
    short seq_number;
    int length;
    short major;
    short minor;
    int pad[5];
} xNVQueryVersionReply;

typedef struct {
    unsigned char type; /* X_Reply */
    unsigned char data;
    short seq_number;
    int length;
    int	flags;
    int value;
    int pad[4];
} xNVGetAttributeReply;


typedef struct {
    unsigned char type; /* X_Reply */
    unsigned char data;
    short seq_number;
    int length;
    int	flags;
    int value;
    int pad[4];
} xNVGetStringAttributeReply;

typedef struct {
    char type; /* X_Reply */
    char data; /* depends on reply type */
    short sequenceNumber; /* of last request received by server */
    int length; /* 4 byte quantities beyond size of GenericReply */
    int flags; /* for example tells if an option is supported or not */
    int attr_type; /* tells if the option is a bitmask, boolean or range */
    int data_range_min;
    int data_range_max;
    int data_bitmask;
    int data_perm; /* contains a bitmask containing permissions like if the option is read-only option or not */
} xNVGetValidAttributeValuesReply;
					    
typedef struct {
    char type; /* boolean, bitmask, .. */
    char flags;
    int	data1;
    int data2;
} validated;

typedef struct {
    union {
        struct {
            unsigned char type;
            unsigned char detail;
            short sequenceNumber;
        } u;
        struct {
            unsigned char type;
            unsigned char detail;
            short sequenceNumber;
            int time;
            int screen;
            int disp_mask;
            int option;
            int value;
            int pad[2];
        } attribute_changed;
    } u;
} nvEvent;


typedef struct {
    int type;
    unsigned long serial;
    char send_event;
    Display *dpy;
    Time time;
    int screen;
    unsigned int disp_mask;
    unsigned int option;
    int value;
} NVAttributeChangedEvent;

typedef union {
    int type;
    NVAttributeChangedEvent attribute_changed;
    long pad[24];
} NVEvent;


int NVGetAttribute(Display *dpy, int screen, unsigned int disp_mask, unsigned int option, int *value);
int NVGetValidAttributeValues(Display *dpy, int screen, unsigned int disp_mask, unsigned int option, validated** res);
int NVQueryExtension(Display *dpy, int *event_base, int *error_base);
int NVQueryVersion(Display *dpy, int *major, int *minor);
int NVGetStringAttribute(Display *dpy, int screen, unsigned int disp_mask, unsigned int option, char **res);
void NVSetAttribute(Display *dpy, int screen, unsigned int disp_mask, unsigned int option, int value);
int NVSelectNotify(Display *dpy, int screen, int type, Bool toggle);

#endif /* LIBNVCONTROL_H */


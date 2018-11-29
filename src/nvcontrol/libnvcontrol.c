/* NVClock 0.8 - Linux overclocker for NVIDIA cards
 * 
 * Copyright(C) 2001-2004 Roderick Colenbrander
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

/* This library was written at the end of 2003 before nvidia released the source to their NV-CONTROL library.
/  The working of the library was figured out using a small LD_PRELOAD library which captured various X request/reply messages
/  of the leaked nvidia-settings program. With the help of various other xfree86 extensions this library was written.
/  Ofcourse the library wasn't 100% bugfree as it was hard to figure out the meaning of everything. Most bugs were fixed
/  when nvidia opensourced their own library. Lots of things were done quite similar in both libraries and there were
/  only a few small bugs. They were mainly related to the reply of NVGetValidAttributeValues which contained info
/  if options were supported and if so what values it supported, if it was display specific ..
*/

#include <dlfcn.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/extensions/extutil.h>
#include <stdlib.h>
#include <getopt.h>

#include "nvcontrol.h"

static Bool wire_to_event (Display *dpy, XEvent *host, xEvent *wire);

static /* const */ XExtensionHooks Hooks = {
    NULL,				/* create_gc */
    NULL,				/* copy_gc */
    NULL,				/* flush_gc */
    NULL,				/* free_gc */
    NULL,				/* create_font */
    NULL,				/* free_font */
    NULL,				/* close_display */
    wire_to_event,			/* wire_to_event */
    NULL,				/* event_to_wire */
    NULL,				/* error */
    NULL				/* error_string */
};
XExtensionInfo *NV_extinfo = NULL;
static XEXT_GENERATE_FIND_DISPLAY(NVFindDisplay,NV_extinfo,"NV-CONTROL", &Hooks,1,NULL);

/* QueryExtension */
int NVQueryExtension(Display *dpy, int *event_base, int *error_base)
{
    XExtDisplayInfo *dpyinfo = NVFindDisplay(dpy);

    if((NV_extinfo == NULL) || (dpyinfo == NULL))
    {
	XMissingExtension(dpy, "NV-CONTROL");
	return 0;
    }

    if(XextHasExtension(dpyinfo))
    {
	*event_base = dpyinfo->codes->first_event;
	*error_base = dpyinfo->codes->first_error;
	return 1;
    }
    
    return 0;
}

/* QueryVersion */
int NVQueryVersion(Display *dpy, int *major, int *minor)
{
    xNVQueryVersionReq *req;
    xNVQueryVersionReply rep;
    XExtDisplayInfo *dpyinfo = NVFindDisplay(dpy);

    if((NV_extinfo == NULL) || (dpyinfo == NULL))
    {
	XMissingExtension(dpy, "NV-CONTROL");
	return 0;
    }
    /* For some reason in the case an extension doesn't exist dpyinfo isn't NULL (atleast on this box) */
    else if(dpyinfo->codes == 0)
    {
	XMissingExtension(dpy, "NV-CONTROL");
	return 0;
    }
    else
    {
        LockDisplay(dpy);
	
	NVGetReq(QueryVersion, req);

        if(_XReply(dpy, (xReply*)&rep, 0, 1) != 0)
	{						
	    *major = rep.major & 65535;
	    *minor = rep.minor & 65535;
	}

	UnlockDisplay(dpy);
	SyncHandle();
	return 1;
    }
}

int NVGetAttribute(Display *dpy, int screen, unsigned int disp_mask, unsigned int option, int *value)
{
    xNVGetAttributeReq *req;
    xNVGetAttributeReply rep;
    XExtDisplayInfo *dpyinfo = NVFindDisplay(dpy);

    if((NV_extinfo == NULL) || (dpyinfo == NULL))
    {
	XMissingExtension(dpy, "NV-CONTROL");
	return 0;
    }
    else
    {
        LockDisplay(dpy);
	NVGetReq(GetAttribute, req);
	req->disp_mask = disp_mask;
	req->screen = screen;
	req->option = option;

        if(_XReply(dpy, (xReply*)&rep, 0, 1) == 0)
	{					
	    UnlockDisplay(dpy);
	    SyncHandle();
	    return 0;
	}

	UnlockDisplay(dpy);
	SyncHandle();

	/* Return succes only when the option exists */
	if(rep.flags)
	{
	    *value = rep.value;
	    return 1;
	}
    }
    return 0;
}

int NVGetStringAttribute(Display *dpy, int screen, unsigned int disp_mask, unsigned int option, char **res)
{
    xNVGetStringAttributeReq *req;
    xNVGetStringAttributeReply rep;
    XExtDisplayInfo *dpyinfo = NVFindDisplay(dpy);

    if((NV_extinfo == NULL) || (dpyinfo == NULL))
    {
	XMissingExtension(dpy, "NV-CONTROL");
	return 0;
    }
    else
    {
	char *buf;
        LockDisplay(dpy);
	
	NVGetReq(GetStringAttribute, req);
	req->disp_mask = disp_mask;
	req->screen = screen;
	req->option = option;
        if(_XReply(dpy, (xReply*)&rep, 0, 0) != 0)
	{
	    buf = malloc(rep.length << 2);

	    if(buf != NULL)
	    {
		_XRead(dpy, buf, rep.length << 2);
		UnlockDisplay(dpy);
		SyncHandle();

		*res = buf;
		return 1;
	    }
	}

	UnlockDisplay(dpy);
	SyncHandle();
	return 0;
    }
}

int NVGetValidAttributeValues(Display *dpy, int screen, unsigned int disp_mask, unsigned int option, validated **out)
{
    validated *res = malloc(sizeof(validated));
    xNVGetValidAttributeValuesReq *req;
    xNVGetValidAttributeValuesReply rep;
    XExtDisplayInfo *dpyinfo = NVFindDisplay(dpy);

    if((NV_extinfo == NULL) || (dpyinfo == NULL))
    {
	XMissingExtension(dpy, "NV-CONTROL");
	return 0;
    }
    else
    {
        LockDisplay(dpy);
	
	NVGetReq(GetValidAttributeValues, req);
	req->disp_mask = disp_mask;
	req->screen = screen;
	req->option = option;

        if(_XReply(dpy, (xReply*)&rep, 0, 1) != 0)
	{
	    res->type = rep.attr_type;
	    res->flags = rep.flags; /* contains if the option is supported or not */

	    /* Option is not supported by this card, so return Null */
	    if(res->flags == 0)
		return 0;
	    
	    /* integer attribute */
	    if(rep.attr_type == 1)
	    {
		/* Do nothing as there doesn't seem to be a limit */
	    }
	    /* bitmask */
	    else if(res->type == 2)
		res->data1 = rep.data_bitmask;
	    /* boolean */
	    else if(res->type == 3)
	    {
		/* Do nothing, values 0 and 1 */
	    }
	    /* range */
	    else if(rep.attr_type == 4)
	    {
		res->data1 = rep.data_range_min;
		res->data2 = rep.data_range_max;
	    }
	    /* Another type of bitmask used for example for FSAA */
	    else if(rep.attr_type == 5)
		res->data1 = rep.data_bitmask;
	}

	*out = res;
	UnlockDisplay(dpy);
	SyncHandle();
    }
    return 1;
}


void NVSetAttribute(Display *dpy, int screen, unsigned int disp_mask, unsigned int option, int value)
{
    xNVSetAttributeReq *req;
    XExtDisplayInfo *dpyinfo = NVFindDisplay(dpy);

    if((NV_extinfo == NULL) || (dpyinfo == NULL))
    {
	XMissingExtension(dpy, "NV-CONTROL");
    }
    else
    {
        LockDisplay(dpy);
	
	NVGetReq(SetAttribute, req);
	req->disp_mask = disp_mask;
	req->screen = screen;
	req->option = option;
	req->value = value;

	UnlockDisplay(dpy);
	SyncHandle();
    }
    /* make sure the attribute is indeed set */
    _XFlush(dpy);
}

#define X_NVSelectNotify 6

int NVSelectNotify(Display *dpy, int screen, int type, Bool toggle)
{
    xNVSelectNotifyReq *req;
    XExtDisplayInfo *dpyinfo = NVFindDisplay(dpy);
    
    if((NV_extinfo == NULL) || (dpyinfo == NULL))
    {
	XMissingExtension(dpy, "NV-CONTROL");
	return 0;
    }
    else
    {
	LockDisplay(dpy);
	
	NVGetReq(SelectNotify, req);

	req->screen = screen;
	req->type = type;
	req->toggle = toggle;

	UnlockDisplay(dpy);
	SyncHandle();
    }
    return 1;
}

static Bool wire_to_event (Display *dpy, XEvent *host, xEvent *wire)
{
    XExtDisplayInfo *dpyinfo = NVFindDisplay (dpy);
    NVEvent *re = (NVEvent *) host;
    nvEvent *event = (nvEvent *) wire;

    if((NV_extinfo == NULL) || (dpyinfo == NULL))
    {
	XMissingExtension(dpy, "NV-CONTROL");
	return False;
    }

    switch ((event->u.u.type & 0x7F) - dpyinfo->codes->first_event)
    {
	case 0:
    	    re->attribute_changed.type = event->u.u.type & 0x7F;
    	    re->attribute_changed.serial =
        	_XSetLastRequestRead(dpy, (xGenericReply*) event);
    	    re->attribute_changed.send_event = ((event->u.u.type & 0x80) != 0);
    	    re->attribute_changed.dpy = dpy;
    	    re->attribute_changed.time = event->u.attribute_changed.time;
    	    re->attribute_changed.screen = event->u.attribute_changed.screen;
    	    re->attribute_changed.disp_mask = event->u.attribute_changed.disp_mask;
    	    re->attribute_changed.option = event->u.attribute_changed.option;
    	    re->attribute_changed.value = event->u.attribute_changed.value;
		break;
	default:
    	    return False;
    }
	    
    return True;
}

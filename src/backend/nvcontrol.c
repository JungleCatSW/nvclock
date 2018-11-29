/* NVClock 0.8 - Linux overclocker for NVIDIA cards
 *
 * site: http://nvclock.sourceforge.net
 *
 * Copyright(C) 2001-2004 Roderick Colenbrander
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

#include <stdio.h>
#include "libnvcontrol.h"
#include "backend.h"

static void nvcontrol_toggle_overclocking(int toggle)
{
	int state;
	
	if(NVGetAttribute(nvclock.dpy, 0, 0, NV_GPU_OVERCLOCKING_STATE, &state))
	{
		if(state != toggle)
			NVSetAttribute(nvclock.dpy, 0, 0, NV_GPU_OVERCLOCKING_STATE, toggle);
	}
}

float nvcontrol_get_gpu_speed()
{
	int clocks, nvclk;

	if(nv_card->state & STATE_3D)
		NVGetAttribute(nvclock.dpy, 0, 0, NV_GPU_3D_CLOCK, &clocks);
	else
		NVGetAttribute(nvclock.dpy, 0, 0, NV_GPU_2D_CLOCK, &clocks);
	
	nvclk = (clocks >> 16) & 0xffff;
	
	return (float)nvclk;
}

void nvcontrol_set_gpu_speed(unsigned int nvclk)
{
	int clocks, memclk;
	
	/* The nvcontrol commands change both clocks at the same time.
	/  In order to only reset the gpu clock, we need to set the
	/  same memory clock again.
	*/
	memclk = (int)nvcontrol_get_memory_speed();
	clocks = (nvclk << 16) + memclk;

	nvcontrol_toggle_overclocking(1); /* Make sure overclocking is on or else we get X failures */

	/* The 2d clock is set in the nv_card->states 2d and both; the same for 3d */	
	if(nv_card->state & STATE_2D)
	NVSetAttribute(nvclock.dpy, 0, 0, NV_GPU_2D_CLOCK, clocks);
	if(nv_card->state & STATE_3D)
	NVSetAttribute(nvclock.dpy, 0, 0, NV_GPU_3D_CLOCK, clocks);	
}

float nvcontrol_get_memory_speed()
{
	int clocks, memclk;

	/* In nv_card->state 3d and both, we return the 3d clocks */
	if(nv_card->state & STATE_3D)
		NVGetAttribute(nvclock.dpy, 0, 0, NV_GPU_3D_CLOCK, &clocks);
	else
		NVGetAttribute(nvclock.dpy, 0, 0, NV_GPU_2D_CLOCK, &clocks);

	memclk = clocks & 0xffff;
	
	return (float)memclk;
}

void nvcontrol_set_memory_speed(unsigned int memclk)
{
	int clocks, nvclk;

	/* The nvcontrol commands change both clocks at the same time.
	/  In order to only reset the gpu clock, we need to set the
	/  same memory clock again.
	*/
	nvclk = (int)nvcontrol_get_gpu_speed();
	clocks = (nvclk << 16) + memclk;
	
	nvcontrol_toggle_overclocking(1); /* Make sure overclocking is on or else we get X failures */

	/* The 2d clock is set in the nv_card->states 2d and both; the same for 3d */	
	if(nv_card->state & STATE_2D)
	NVSetAttribute(nvclock.dpy, 0, 0, NV_GPU_2D_CLOCK, clocks);
	if(nv_card->state & STATE_3D)
	NVSetAttribute(nvclock.dpy, 0, 0, NV_GPU_3D_CLOCK, clocks);	
}

void nvcontrol_reset_gpu_speed()
{
	int clocks, memclk, nvclk;

	nvcontrol_toggle_overclocking(1); /* Make sure overclocking is on or else we get X failures */
	if(nv_card->state & STATE_2D)
	{
		/* We need to set the current memory clock again as we can't change just one clock */
		NVGetAttribute(nvclock.dpy, 0, 0, NV_GPU_2D_CLOCK, &clocks);
		memclk = clocks & 0xffff;

		NVGetAttribute(nvclock.dpy, 0, 0, NV_GPU_DEFAULT_2D_CLOCK, &clocks);
		nvclk = (clocks >> 16) & 0xffff;

		clocks = (nvclk << 16) + memclk;
		NVSetAttribute(nvclock.dpy, 0, 0, NV_GPU_2D_CLOCK, clocks);
	}
	if(nv_card->state & STATE_3D)
	{
		NVGetAttribute(nvclock.dpy, 0, 0, NV_GPU_3D_CLOCK, &clocks);
		memclk = clocks & 0xffff;

		NVGetAttribute(nvclock.dpy, 0, 0, NV_GPU_DEFAULT_3D_CLOCK, &clocks);
		nvclk = (clocks >> 16) & 0xffff;	

		clocks = (nvclk << 16) + memclk;
		NVSetAttribute(nvclock.dpy, 0, 0, NV_GPU_3D_CLOCK, clocks);
	}	
}

void nvcontrol_reset_memory_speed()
{
	int clocks, memclk, nvclk;

	nvcontrol_toggle_overclocking(1); /* Make sure overclocking is on or else we get X failures */
	if(nv_card->state & STATE_2D)
	{
		/* We need to set the current gpu clock again as we can't change just one clock */
		NVGetAttribute(nvclock.dpy, 0, 0, NV_GPU_2D_CLOCK, &clocks);
		nvclk = (clocks >> 16) & 0xffff;

		NVGetAttribute(nvclock.dpy, 0, 0, NV_GPU_DEFAULT_2D_CLOCK, &clocks);
		memclk = clocks & 0xffff;

		clocks = (nvclk << 16) + memclk;
		NVSetAttribute(nvclock.dpy, 0, 0, NV_GPU_2D_CLOCK, clocks);
	}
	if(nv_card->state & STATE_3D)
	{
		NVGetAttribute(nvclock.dpy, 0, 0, NV_GPU_3D_CLOCK, &clocks);
		nvclk = (clocks >> 16) & 0xffff;	

		NVGetAttribute(nvclock.dpy, 0, 0, NV_GPU_DEFAULT_3D_CLOCK, &clocks);
		memclk = clocks & 0xffff;

		clocks = (nvclk << 16) + memclk;
		NVSetAttribute(nvclock.dpy, 0, 0, NV_GPU_3D_CLOCK, clocks);
	}	
}

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
 */

#include <stdio.h>
#include "nvcontrol.h"

NVOption fsaa_list[] = {
    {"Disabled", 0},
    {"2x Bilinear", 1},
    {"2x Quincunx", 2},
    {"1.5x1.5 Supersampling", 3},
    {"2x2 Supersampling", 4},
    {"4x Bilinear", 5},
    {"4x Gaussian", 6},
    {"8x", 7},
    {"16x", 8},
    {"8xS", 9},
    {"8xQ", 10},
    {"16xS", 11},
    {"16xQ", 12},
    {"32xS", 13},
    {NULL, -1}
};

NVOption aniso_list[] = {
    {"1x", 0},
    {"2x", 1},
    {"4x", 2},
    {"8x", 3},
    {"16x",4},
    {NULL, -1}
};

/* Is this correct ? */
NVOption isample_list[] = {
    {"Disabled", 0},
    {"Quality", 1},
    {"Performance", 2},
    {NULL, -1}
};

NVOption quality_list[] = {
    {"High Quality", 0},
    {"Quality", 1},
    {"Performance", 2},
    {"High Performance", 3},
    {NULL, -1}
};

NVOption boolean_list[] = {
    {"Disabled", 0},
    {"Enabled", 1},
    {NULL, -1}
};

NVOptionList option_list[] = {
    {"fsaa", 4, "Fullscene Antialiasing", NV_FSAA, 0, (NVOption*)&fsaa_list },
    {"aniso", 5, "Anisotropic Filtering", NV_LOG_ANISO, 0, (NVOption*)&aniso_list },
    {"intellisample", 13, "Intellisampling", NV_OPENGL_QUALITY_ENHANCHEMENTS, 0, (NVOption*)&isample_list },
    {"quality", 7, "Image Qualty", NV_IMAGE_QUALITY, 0, (NVOption*)&quality_list },
    {"slihud", 6, "Show SLI HUD", NV_SHOW_SLI_HUD, 0, (NVOption*)&boolean_list },
    {"vsync", 5, "Sync to VBlank", NV_SYNC_VBLANK, 0, (NVOption*)&boolean_list },
    {"pageflip", 8, "PageFlipping", NV_PAGEFLIP, 0, (NVOption*)&boolean_list },
    {"windowflip", 10, "WindowFlipping", NV_WINDOWFLIP, 0, (NVOption*)&boolean_list },
    {"flipping", 8, "Allow Flipping", NV_FLIPPING_ALLOWED, 0, (NVOption*)&boolean_list },
    {"ubb", 3, "Unified Back Buffer", NV_UBB, 0, (NVOption*)&boolean_list },
    {"cpu", 3, "Disable CPU optimizations", NV_FORCE_GENERIC_CPU, 0, (NVOption*)&boolean_list },
    {"nvemulate", 9, "Emulate Nvidia GPU", NV_EMULATE, 0, NULL },
    {"texsharpen", 10, "Texture Sharpening", NV_TEXTURE_SHARPEN, 0, (NVOption*)&boolean_list },
    {"texclamping", 11, "Texture Clamping", NV_TEXTURE_CLAMPING, 0, (NVOption*)&boolean_list }, 
    {"linegamma", 9, "AA Line Gamma", NV_OPENGL_AA_LINE_GAMMA, 0, (NVOption*)&boolean_list },
    {"vibrance", 8, "Digital Vibrance", NV_DIGITALVIBRANCE, NV_CRT|NV_DFP|NV_TV, NULL },
    {"sharpen", 7, "Image Sharpening", NV_IMAGE_SHARPENING, NV_CRT|NV_DFP|NV_TV, NULL },
    {"dfpscaling", 10, "Flatpanel Scaling", NV_FLATPANEL_SCALE, NV_DFP, NULL },
    {"dfpdithering", 12, "Flatpanel Dithering", NV_FLATPANEL_DITHERING, NV_DFP, NULL },
    {"tvoverscan", 10, "TV Overscan", NV_TV_OVERSCAN, NV_TV, NULL },
    {"tvflicker", 9, "TV Flicker", NV_TV_FLICKER, NV_TV, NULL },
    {"tvbrightness", 12, "TV Brightness", NV_TV_BRIGHTNESS, NV_TV, NULL },
    {"tvhue", 5, "TV Hue", NV_TV_HUE, NV_TV, NULL },
    {"tvcontrast", 10, "TV Contrast", NV_TV_CONTRAST, NV_TV, NULL },
    {"tvsaturation", 12, "TV Saturation", NV_TV_SATURATION, NV_TV, NULL },
    {"tvreset", 7, "TV Reset", NV_TV_RESET, NV_TV, NULL },
    {NULL, 0, NULL, 0, 0, 0}
};


/* Convert an index of the option list to a opcode; "0" -> NV_FSAA */
static int index_to_option(int index)
{
    return option_list[index].option;
}

/* Convert the opcode of an option to an index; NV_FSAA -> 0 */
static int option_to_index(int option)
{
    int i;
    for(i=0; i <= OPTION_LIST_SIZE; i++) {
	if(option_list[i].option == option)
    	    return i;
    }
    return -1;
}

/* Convert a value index to a real value; (fsaa 4x gaussian) 4 -> 6 */
static int index_to_value(int index, int value)
{
    return option_list[index].values[value].value;
}

/* Convert a real value into a index value for the scales; (fsaa 4x gaussian) 6 -> 4  */
static int value_to_index(int index, int value)
{
    int i;
    for(i=0; i < 10; i++) {
	printf("%d\n", index);
	if(option_list[index].values[i].value == value)
	    return i;
    }
    return 0;
}

NVOptionList *nvcontrol_lookup_option(int option)
{
    int index = option_to_index(option);
    if(index >= 0)
	return &option_list[index];

    return NULL;
}

int init_nvcontrol(Display *dpy)
{
    int major, minor;

    /* We need some version check */
    if(!NVQueryVersion(dpy, &major, &minor))
	return 0;

    return 1;
}


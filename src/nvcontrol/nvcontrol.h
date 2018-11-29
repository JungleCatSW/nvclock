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


#ifndef NVCONTROL_H
#define NVCONTROL_H

#include "libnvcontrol.h"

typedef struct
{
    char *name;
    int value;
} NVOption;

typedef struct
{
    const char *name;
    const short size;
    const char *description;
    const int option;
    const int flags;
    const NVOption *values; /* list containing names for some values */
} NVOptionList;

#define OPTION_LIST_SIZE 27
NVOptionList option_list[OPTION_LIST_SIZE];
int init_nvcontrol();
NVOptionList *nvcontrol_lookup_option(int option);

#endif /* NVCONTROL_H */

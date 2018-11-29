/* NVClock Smartdimmer 0.8
 *
 * Originally SmartDimmer was a friendly fork of NVClock to allow adjustment
 * of backlight level on some Sony laptops. Initially there was no code for this
 * in NVClock. When NVClock added the code SmartDimmer became obsolete but people
 * kept using SmartDimmer while it didn't get updated. For that reason SmartDimmer
 * is now part of NVClock.
 *
 * Copyright(C) 2001-2008 Roderick Colenbrander
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

/******************************************************************************
 * smartdimmer.c                                                              *
 *                                                                            *
 * SmartDimmer adjustment tool.                                               *
 * Thanks to Thunderbird and sk1p from #nvclock @ freenode for their help :)  *
 *                                                                            *
 * July 23, 2005                                                              *
 * Erik Waling <erikw@acc.umu.se>                                             *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "nvclock.h"

/* for command line arguments */
#define SET_BIT    1
#define GET_BIT    2
#define INC_BIT    4
#define DEC_BIT    8

static struct option long_opts[] = {
    { "get"     , no_argument      , 0, 'g' },
    { "set"     , required_argument, 0, 's' },
    { "increase", no_argument      , 0, 'i' },
    { "decrease", no_argument      , 0, 'd' },
    { "help"    , no_argument      , 0, 'h' },
    { 0         , 0                , 0, 0   }
};
  
void sd_usage(char *argv0)
{   
    printf("NVClock SmartDimmer adjustment tool version 0.8b4.\n\n");
    printf("Usage: %s [OPTION]...\n\n", argv0);
    printf("Options:\n");
    printf("\t-g  --get\t\tQuery brightness level.\n");
    printf("\t-s  --set <level>\tSet brightness level (15-100)\n");
    printf("\t-i  --increase\t\tIncrease brightness with one level.\n");
    printf("\t-d  --decrease\t\tDecrease brightness with one level.\n");
    printf("\t-h  --help\t\tPrints this help text.\n\n");
            
}

int sd_init()
{
    if (!init_nvclock()) 
        return -1;
    
    if (!set_card(0)) 
        return -2;
    
    return 0;
}

int main(int argc, char *argv[])
{   
    int optind = 0, options = 0, setvalue = 0;
    int c;
    
    if (argc < 2)
    {
        sd_usage(argv[0]);
        return 0;
    }
    
    while ((c = getopt_long(argc, argv, "gs:idh", long_opts, &optind)) != -1)
    {
        switch (c)
        {
            case '?':
                fprintf(stderr, "\nTry `%s --help' for help.\n", argv[0]);
                return 1;
            case 'h':
                sd_usage(argv[0]);
                return 0;
            case 'g':
               options |= GET_BIT;
               break;
            case 's':
               if (isdigit(*optarg)) {
                   setvalue = atoi(optarg);
                   options |= SET_BIT;
               } else {
                   fprintf(stderr, "Illegal option value (-s): "
                           "Value has to be a non-negative number.\n");
                   return 1;
               }
               break;
            case 'i':
               options |= INC_BIT;
               break;
            case 'd':
               options |= DEC_BIT;
               break;
        }
    } 
    
    if (!options)
    {
        sd_usage(argv[0]);
        return 1;
    }
        
    switch (sd_init())
    {
        case -1:
            fprintf(stderr, "init_nvclock() failed!\n");
            return 2;
        case -2:
            fprintf(stderr, "set_card() failed!\n");
            return 3;
    }

    if(!(nv_card->caps & SMARTDIMMER))
    {
        fprintf(stderr, "Error!\n");
        fprintf(stderr, "Smartdimmer is only supported on certain (HP/SamsungSony/Zepto) laptops using a Geforce 6200/7x00Go/8x00Go. If you want support on your laptop contact the author.\n");
        return 0;
    }
    
    if (options & INC_BIT)
        nv_card->set_smartdimmer(nv_card->get_smartdimmer() + 1);
    
    if (options & DEC_BIT)
        nv_card->set_smartdimmer(nv_card->get_smartdimmer() - 1);
    
    if (options & SET_BIT)
        nv_card->set_smartdimmer(setvalue);
    
    if (options & GET_BIT)
        printf("SmartDimmer level: %d\n", nv_card->get_smartdimmer());
    
    return 0;
}

/* NVClock 0.8 - Linux overclocker for NVIDIA cards
 * 
 * Copyright(C) 2001-2007 Roderick Colenbrander
 *
 * site: http://nvclock.sourceforge.net
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

#include <netinet/in.h> /* needed for htonl */
#include <sys/mman.h> 
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "backend.h"

NVClock nvclock;
NVCard *nv_card = NULL;

static int IsVideoCard(unsigned short devbusfn);

static int probe_devices();
static void *map_dev_mem (int fd, unsigned long Base, unsigned long Size);
static void unmap_dev_mem (unsigned long Base, unsigned long Size);

/* Check if we are using the closed source Nvidia drivers */
static int check_driver()
{
	FILE *proc;
	char buffer[80];

	proc = fopen("/proc/modules", "r");

	/* Don't crash when there's no /proc/modules */
	if(proc == NULL)
		return 0;

	while(fgets(buffer, 80, proc) != NULL)
	{
		char name[80];
		int size;
		int used;

		/* Check to see if NVdriver/nvidia is loaded and if it is used.
		/  For various versions the driver isn't initialized when X hasn't
		/  been started and it can crash then.
		*/
		if(sscanf(buffer,"%s %d %d",&name, &size, &used) != 3) continue;
		{
			if(strcmp(name, "NVdriver") == 0)
			{
				fclose(proc);
				if(used)
					return 1;

					return 0;
			}

			if(strcmp(name, "nvidia") == 0)
			{
				fclose(proc);
				if(used)
					return 2;

				return 0;
			}
		}
	}
	fclose(proc);

	return 0;
}

int init_nvclock()
{
	int nv_driver = check_driver();

	/* Check if the nvidia drivers are available and if not check if the user is root */
	if((!nv_driver) && (getuid() != 0 && geteuid() != 0))
	{
		set_error(NV_ERR_NO_DRIVERS_FOUND);
		return 0;
	}

	/* Detect all nvidia cards; this needs to be done before creating directory and config file as that code needs card access */
	if(!probe_devices())
	{
	/* probe_devices takes care of the error as it isn't certain it failed because of there are no nvidia cards */
		return 0;
	}

	if(!open_config())
		return 0;

	return 1;
}

static int probe_devices()
{
	int dev, irq, reg_addr, i=0;
	unsigned short devbusfn;
	char buf[256];
	FILE *proc;

	proc = fopen("/proc/bus/pci/devices", "r");
	if(!proc) 
	{
		set_error_str("Can't open /proc/bus/pci/devices to detect your videocard.");
		return 0;
	}

	while(fgets(buf, sizeof(buf)-1, proc)) 
	{
		if(sscanf(buf,"%hx %x %x %x",&devbusfn, &dev, &irq, &reg_addr) != 4) continue;

		/* Check if the card contains an Nvidia chipset */	
		if((dev>>16) == 0x10de)
		{
			/*
			When we enter this block of code we know that the device contains some 
			chip designed by Nvidia. In the past Nvidia only produced videochips but
			now they also make various other devices. Because of this we need to find
			out if the device is a videocard or not. There are two ways to do this. We can
			create a list of all Nvidia videochips or we can check the pci header of the device.
			We will read the pci header from /proc/bus/pci/(bus)/(function).(device). When
			the card is in our card database we report the name of the card and else we say
			it is an unknown card.
			*/

			if(!IsVideoCard(devbusfn))
				continue;

			nvclock.card[i].device_id = (0x0000ffff & dev);
			nvclock.card[i].arch = get_gpu_arch(nvclock.card[i].device_id);
			nvclock.card[i].number = i;
			nvclock.card[i].card_name = (char*)get_card_name(nvclock.card[i].device_id, &nvclock.card[i].gpu);
			nvclock.card[i].devbusfn = devbusfn;
			nvclock.card[i].irq = irq;
			nvclock.card[i].state = 0;

			/*
			Thanks to all different driver version this is needed now.
			When nv_driver > 1 the nvidia kernel module is loaded. 
			For driver versions < 1.0-40xx the register offset could be set to 0.
			Thanks to a rewritten kernel module in 1.0-40xx the register offset needs
			to be set again to the real offset. 
			*/
			switch(check_driver())
			{
				case 0:
					nvclock.card[i].dev_name = (char*)strdup("/dev/mem");
					nvclock.card[i].reg_address = reg_addr;
					break;
				case 1:
					nvclock.card[i].dev_name = calloc(13, sizeof(char));
					sprintf(nvclock.card[i].dev_name, "/dev/nvidia%d", nvclock.card[i].number);
					nvclock.card[i].reg_address = 0;
					break;
				case 2:
					nvclock.card[i].dev_name = calloc(13, sizeof(char));
					sprintf(nvclock.card[i].dev_name, "/dev/nvidia%d", nvclock.card[i].number);
					nvclock.card[i].reg_address = reg_addr;
					break;
			}

			i++;
		}
	}
	fclose(proc);
    
	if(i==0)
	{
		set_error(NV_ERR_NO_DEVICES_FOUND);
		return 0;
	}

	nvclock.num_cards = i;
	return 1;
}

/* Check if the device is a videocard */
static int IsVideoCard(unsigned short devbusfn)
{
	int32_t pci_class = pciReadLong(devbusfn, 0x9);
	/* When the id isn't 0x03 the card isn't a vga card return 0 */
	if(((htonl(pci_class) >> 8) & 0xf) != 0x03)
		return 0;
	else
		return 1;
}

int32_t pciReadLong(unsigned short devbusfn, long offset)
{
	char file[25];
	FILE *device;
	short bus = PCI_GET_BUS(devbusfn);
	short dev = PCI_GET_DEVICE(devbusfn);
	short function = PCI_GET_FUNCTION(devbusfn);

	snprintf(file, sizeof(file), "/proc/bus/pci/%02x/%02x.%x", bus, dev, function);
	if((device = fopen(file, "r")) != NULL)
	{
		int32_t buffer;
		fseek(device, offset, SEEK_SET); 	    			    
		fread(&buffer, sizeof(int32_t), 1, device);
		fclose(device);

		return buffer;
	}
    
	return -1;
}

int map_mem(const char *dev_name)
{
	int fd;

	if( (fd = open(dev_name, O_RDWR)) == -1 )
	{
		char err[80];
		sprintf(err, "Can't open %s", dev_name);
		set_error_str(err);
		return 0;
	}
    
	/* Map the registers of the nVidia chip */
	nv_card->PEXTDEV = map_dev_mem(fd, nv_card->reg_address + 0x101000, 0x1000);
	nv_card->PFB     = map_dev_mem(fd, nv_card->reg_address + 0x100000, 0x1000);
	/* normally pmc is till 0x2000 but extended it for nv40 */
	nv_card->PMC     = map_dev_mem(fd, nv_card->reg_address + 0x000000, 0x2ffff);
	nv_card->PCIO    = map_dev_mem(fd, nv_card->reg_address + 0x601000, 0x2000);
	nv_card->PDISPLAY = map_dev_mem(fd, nv_card->reg_address + NV_PDISPLAY_OFFSET, NV_PDISPLAY_SIZE);
	nv_card->PRAMDAC = map_dev_mem(fd, nv_card->reg_address + 0x680000, 0x2000);
	nv_card->PRAMIN  = map_dev_mem(fd, nv_card->reg_address + NV_PRAMIN_OFFSET, NV_PRAMIN_SIZE);
	nv_card->PROM    = map_dev_mem(fd, nv_card->reg_address + 0x300000, 0xffff);

	/* On Geforce 8xxx cards it appears that the pci config header has been moved */
	if(nv_card->arch & NV5X)
		nv_card->PBUS = map_dev_mem(fd, nv_card->reg_address + 0x88000, 0x100);
	else
		nv_card->PBUS = nv_card->PMC + 0x1800/4;

	nv_card->mem_mapped = 1;
	close(fd);
	return 1;
}

void unmap_mem()
{
	unmap_dev_mem((unsigned long)nv_card->PEXTDEV, 0x1000);
	unmap_dev_mem((unsigned long)nv_card->PFB, 0x1000);
	unmap_dev_mem((unsigned long)nv_card->PMC, 0xffff);
	unmap_dev_mem((unsigned long)nv_card->PCIO, 0x2000);
	unmap_dev_mem((unsigned long)nv_card->PDISPLAY, NV_PDISPLAY_SIZE);
	unmap_dev_mem((unsigned long)nv_card->PRAMDAC, 0x2000);
	unmap_dev_mem((unsigned long)nv_card->PRAMIN, NV_PRAMIN_SIZE);
	unmap_dev_mem((unsigned long)nv_card->PROM, 0xffff);
}

/* -------- mmap on devices -------- */
/* This piece of code is from nvtv a linux program for tvout */
/* The author of nvtv got this from xfree86's os-support/linux/lnx_video.c */
/* and he modified it a little  */
static void *map_dev_mem (int fd, unsigned long Base, unsigned long Size)
{
	void *base;
	int mapflags = MAP_SHARED;
	unsigned long realBase, alignOff;

	realBase = Base & ~(getpagesize() - 1);
	alignOff = Base - realBase;

	base = mmap((caddr_t)0, Size + alignOff, PROT_READ|PROT_WRITE,
	mapflags, fd, (off_t)realBase);
	return (void *) ((char *)base + alignOff);
}

static void unmap_dev_mem (unsigned long Base, unsigned long Size)
{
	unsigned long alignOff = Base - (Base & ~(getpagesize() - 1));
	munmap((caddr_t)(Base - alignOff), (Size + alignOff));
	nv_card->mem_mapped = 0;
}

/* NVClock 0.8 - FreeBSD overclocker for NVIDIA cards
 *
 * Site: http://nvclock.sourceforge.net
 *
 * Copyright(C) 2001-2005 Roderick Colenbrander
 * Portions Copyright(C) 2003 Samy Al Bahra  <samy@kerneled.com>
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

#include <errno.h>
#include <fcntl.h>
#include <osreldate.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/linker.h>
#include <sys/mman.h>

#include "backend.h"

#if __FreeBSD_version < 430000
#  include <pci/pci_ioctl.h>
#else
#  include <sys/pciio.h>
#endif

#define NV_VENDOR 0x10de
#define VGA 0x03
#define SIZE 255

NVClock nvclock;
NVCard *nv_card = NULL;

static int probe_devices();
static void *map_dev_mem (int fd, unsigned long Base, unsigned long Size);
static void unmap_dev_mem (unsigned long Base, unsigned long Size);

int init_nvclock()
{
	/* Check if the user is root */
	if(getuid() != 0 && geteuid() != 0)
	{
		set_error(NV_ERR_NOT_ENOUGH_PERMISSIONS);
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
	struct pci_conf_io pcidev;
	struct pci_match_conf patterns;
	struct pci_conf matches[SIZE];
	struct pci_io pi;
	int pcid, counter;
  
	if((pcid=open("/dev/pci", O_RDWR))==-1)
	{
		set_error_str("Could not open /dev/pci");
		return 0;
	}

	memset(&pcidev,0,sizeof(pcidev));
	pcidev.pat_buf_len=sizeof(patterns);
	patterns.pc_vendor=NV_VENDOR;
	patterns.pc_class=VGA;
	patterns.flags=PCI_GETCONF_MATCH_VENDOR|PCI_GETCONF_MATCH_CLASS;
	pcidev.patterns=&patterns;
	pcidev.num_patterns=1;
	pcidev.pat_buf_len=sizeof(patterns);
	pcidev.match_buf_len=sizeof(matches);
	pcidev.matches=matches;
  
	if(ioctl(pcid, PCIOCGETCONF, &pcidev)==-1)
	{
		set_error_str("Could not get configuration of /dev/pci");
		close(pcid);
		return 0;
	}
    
	if(pcidev.status==PCI_GETCONF_LIST_CHANGED)
	{
		set_error_str("PCI device list has changed\n");
		close(pcid);
		return 0;
	}
	else if(pcidev.status==PCI_GETCONF_ERROR)
	{
		set_error_str("General error encountered while trying to get PCI information\n");
		close(pcid);
		return 0;
	}
  
	if(!pcidev.num_matches)
	{
		set_error(NV_ERR_NO_DEVICES_FOUND);
		close(pcid);
		return 0;
	}
  
	for(counter=0;counter<MAX_CARDS && counter < pcidev.num_matches;counter++)
	{
		nvclock.card[counter].device_id=(matches+counter)->pc_device;

		pi.pi_sel.pc_bus=(matches+counter)->pc_sel.pc_bus;
		pi.pi_sel.pc_dev=(matches+counter)->pc_sel.pc_dev;
		pi.pi_sel.pc_func=(matches+counter)->pc_sel.pc_func;
		pi.pi_reg=0x10;
		pi.pi_width=sizeof(unsigned int);
		if(ioctl(pcid, PCIOCREAD, &pi)==-1)
		{
			set_error_str("Could not read data from /dev/pci");
			close(pcid);
			return 0;
		}
		nvclock.card[counter].reg_address=pi.pi_data;
		nvclock.card[counter].dev_name= "/dev/mem";
		nvclock.card[counter].card_name = (char*)get_card_name(nvclock.card[counter].device_id, &nvclock.card[counter].gpu);
		nvclock.card[counter].arch = get_gpu_arch(nvclock.card[counter].device_id);
		nvclock.card[counter].number = counter;
		nvclock.card[counter].devbusfn = PCI_GET_DEVBUSFN(pi.pi_sel.pc_dev, pi.pi_sel.pc_bus, pi.pi_sel.pc_func);
		nvclock.card[counter].state = 0;
	}
  
	close(pcid);
  
	nvclock.num_cards = counter;
	return 1;
}

int32_t pciReadLong(unsigned short devbusfn, long offset)
{
	int fd;
	struct pci_io pi;
	pi.pi_sel.pc_bus = PCI_GET_BUS(devbusfn);
	pi.pi_sel.pc_dev = PCI_GET_DEVICE(devbusfn);
	pi.pi_sel.pc_func = PCI_GET_FUNCTION(devbusfn);
	pi.pi_reg = offset;
	pi.pi_width = 4;

	if((fd=open("/dev/pci", O_RDWR)) == -1)
	{
		set_error_str("Could not open /dev/pci");
		return -1;
	}

	if(ioctl(fd, PCIOCREAD, &pi) == -1)
	{
		set_error_str("Could not read data from /dev/pci");
		close(fd);
		return -1;
	}

	close(fd);	
	return pi.pi_data;
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

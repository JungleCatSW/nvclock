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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "backend.h"

#include "MemAcc.h"

void *lib_winio = NULL;
int (__stdcall *pOpenLibrary)();
int (__stdcall *pGetLastState)(char *sStatus);
int (__stdcall *pGetDeviceBaseAddress)(unsigned short* VendorID, unsigned short*
DeviceID, LONG nIndex, BADDR baddrBuffer[6]);
int (__stdcall *pGetPCIDeviceInfo)(unsigned short* VendorID, unsigned short* DeviceID, long nIndex, unsigned long* Bus, unsigned
long* Device, unsigned long* Function, PCI_CONFIG_HEADER *pcfg);

void* (__stdcall *pMapPhysToLinear)(DWORD PhAddr, DWORD dwSize, HANDLE* hMem);


static int probe_devices();

NVClock nvclock;
NVCard *nv_card = NULL;

int init_nvclock()
{
	void *lib_memacc = LoadLibrary("MemAcc.dll");

	int nResult, nIndex;
	int VendorID, DeviceID;
	char* s;
	int i;
	char sError[255];

	if(lib_memacc == NULL)
	{
		set_error_str("Can't open MemAcc.dll\n");
	return 0;
	}

	pOpenLibrary = (void*)GetProcAddress(lib_memacc, "maOpenLibrary");
	pGetLastState = (void*)GetProcAddress(lib_memacc, "maGetLastState");
	pGetDeviceBaseAddress = (void*)GetProcAddress(lib_memacc, "maGetDeviceBaseAddress");
	pGetPCIDeviceInfo = (void*)GetProcAddress(lib_memacc, "maGetPCIDeviceInfo");
	pMapPhysToLinear = (void*)GetProcAddress(lib_memacc, "maMapPhysToLinear");

	if(!pOpenLibrary())
	{
		char buf[80];
		pGetLastState(buf);
		printf("Loading of MemAcc.dll failed: %s \n", buf);
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
	unsigned short vendor_id=0xffff, device_id=0xffff;
	int index=0;
	int i=0;
	unsigned long bus, dev, function;
	PCI_CONFIG_HEADER pCfg;
	BADDR baddrAddress[6];

	while(pGetPCIDeviceInfo(&vendor_id, &device_id, index, &bus, &dev, &function, &pCfg) == 1)
	{
		/* Check whether the vendor is nvidia and the BaseClass == VGA */
		if(vendor_id == 0x10de && pCfg.BaseClass == 0x3)
		{
			printf("Found VendorID: 0x%x DeviceID: 0x%x\r\n", vendor_id, device_id);
			
			nvclock.card[i].device_id = device_id;
			nvclock.card[i].arch = get_gpu_arch(nvclock.card[i].device_id);
			nvclock.card[i].number = i;
			nvclock.card[i].card_name = (char*)get_card_name(nvclock.card[i].device_id, &nvclock.card[i].gpu);
			nvclock.card[i].reg_address = pCfg.BaseAddresses[0];
//			nvclock.card[i].devbusfn = devbusfn;
			nvclock.card[i].irq = pCfg.InterruptLine;
			nvclock.card[i].state = 0;
			i++;
		}

		index++;
		vendor_id = 0xffff;
		device_id = 0xffff;
	}
	nvclock.num_cards = i;
	return 1;		
}

int32_t pciReadLong(unsigned short devbusfn, long offset)
{
	return -1;
}

int map_mem(const char *dev_name)
{
	void *hmem; // do nothing with this for now
	/* Map the registers of the nVidia chip */
	nv_card->PEXTDEV = pMapPhysToLinear(nv_card->reg_address + 0x101000, 0x1000, &hmem);
	nv_card->PFB	 = pMapPhysToLinear(nv_card->reg_address + 0x100000, 0x1000, &hmem);
	/* normally pmc is till 0x2000 but extended it for nv40 */
	nv_card->PMC	 = pMapPhysToLinear(nv_card->reg_address + 0x000000, 0x2ffff, &hmem);
	nv_card->PCIO	= pMapPhysToLinear(nv_card->reg_address + 0x601000, 0x2000, &hmem);
	nv_card->PDISPLAY = pMapPhysToLinear(nv_card->reg_address + NV_PDISPLAY, NV_PDISPLAY_LENGTH, &hmem);
	nv_card->PRAMDAC = pMapPhysToLinear(nv_card->reg_address + 0x680000, 0x2000, &hmem);
	nv_card->PROM	= pMapPhysToLinear(nv_card->reg_address + 0x300000, 0xffff, &hmem);

	/* On Geforce 8xxx cards it appears that the pci config header has been moved */
	if(nv_card->arch & NV5X)
		nv_card->PBUS = pMapPhysToLinear(nv_card->reg_address + 0x88000, 0x100, &hmem);
	else
		nv_card->PBUS = nv_card->PMC + 0x1800/4;

	nv_card->mem_mapped = 1;
	return 1;
}

void unmap_mem()
{
#if 0
	pUnMapIO(winio, (void*)nv_card->PEXTDEV);
	pUnMapIO(winio, (void*)nv_card->PFB);
	pUnMapIO(winio, (void*)nv_card->PMC);
	pUnMapIO(winio, (void*)nv_card->PCIO);
	pUnMapIO(winio, (void*)nv_card->PRAMDAC);
	pUnMapIO(winio, (void*)nv_card->PROM);
	nv_card->mem_mapped = 0;
#endif
}

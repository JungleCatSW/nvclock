/* NVClock 0.8 - Linux overclocker for NVIDIA cards
 *
 * site: http://nvclock.sourceforge.net
 *
 * Copyright(C) 2001-2007 Roderick Colenbrander
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

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "backend.h"
#include "nvclock.h"

static void insert_entry(cfg_entry **cfg, cfg_entry *entry);

/* Free the config file structure */
void destroy(cfg_entry **cfg)
{
	cfg_entry *pCfg, *pPrevious;

	pCfg = *cfg;
	while(pCfg)
	{
		pPrevious = pCfg;
		free(pPrevious->name);
		free(pPrevious->section);
		pCfg = pCfg->next;
		free(pPrevious);
	}
}


/* Add a new config file entry section.name=value */
void add_entry(cfg_entry **cfg, char *section, char *name, int value)
{
	cfg_entry *entry;

	entry = (cfg_entry*)calloc(1, sizeof(cfg_entry));
	entry->name = (char*)strdup(name);
	entry->section = (char*)strdup(section);
	entry->value = value;
	entry->next = NULL;

	insert_entry(cfg, entry);
}


/* Change the value of an entry or add an entry when it doesn't exist */
void change_entry(cfg_entry **cfg, char *section, char *name, int value)
{
	cfg_entry *entry = lookup_entry(cfg, section, name);

	/* When an entry is found update it */
	if(entry)
	{
		entry->value=value;
	}
	/* Create a new entry */
	else
	{
		add_entry(cfg, section, name, value);
	}
}


/* Insert an entry into the config file structure */
static void insert_entry(cfg_entry **cfg, cfg_entry *entry)
{
	cfg_entry *pCfg = NULL;
	cfg_entry *pPrev = NULL;
	cfg_entry *pNext = NULL;

	/* When the cfg list is still empty, add the first entry */
	if(!*cfg)
	{
		*cfg = entry;
		return;
	}

	/* Sort the list by section and add the entry */
	for(pCfg = *cfg; pCfg != NULL; pPrev=pCfg, pCfg = pCfg->next, pNext = pCfg ? pCfg->next : NULL )
	{
		/* Check if the entry belongs to a section below the current section */
		if(strcmp(entry->section, pCfg->section) < 0)
		{
			if(!pPrev)
			{
				*cfg = entry;
				entry->next = pCfg;
			}
			else
			{
				pPrev->next = entry;
				entry->next = pCfg;
			}
			return;
		}

		if(strcmp(entry->section, pCfg->section) == 0)
		{
			/* The entry needs to be placed before the current entry */
			if(strcmp(entry->name, pCfg->name) < 0)
			{
				if(!pPrev)
				{
					*cfg = entry;
					entry->next = pCfg;
				}
				else
				{
					pPrev->next = entry;
					entry->next = pCfg;
				}
				return;
			}
			/* When there's a next entry, check if it belongs to the same section or
			/  else add the option to the current section.
			*/
			if(pNext)
			{
				/* The sections don't match, so add the option to the current one */
				if(strcmp(entry->section, pNext->section) != 0)
				{
					pCfg->next = entry;
					entry->next = pNext;
					return;
				}
			}
		}

		/* Entry should become the last one */
		if(!pCfg->next)
		{
			pCfg->next = entry;
			return;
		}
		continue;
	}
}


/* Look up section.name */
cfg_entry* lookup_entry(cfg_entry **cfg, char *section, char *name)
{
	cfg_entry *entry = *cfg;

	while(entry)
	{
		/* If everything matches, the option is found */
		if(!strcmp(entry->section, section) && !strcmp(entry->name, name))
		{
			return entry;
		}

		entry = (cfg_entry*)entry->next;
	}
	return NULL;
}


/* Helper function that splits lines of the form section.name=value */
static int split_line(const char *line, char **section, char **name, char **value)
{
	char *a, *b;

	if(!(a = strchr(line, '.')))
		return 0;

	/* overwrite '.' with '\0' */
	a[0] = '\0';
	*section = (char*)strdup(line);

	a++;

	if(!(b = strchr(a, '=')))
		return 0;
	/* overwrite '=' with '\0' */
	b[0] = '\0';
	*name = (char*)strdup(a);
	b++;

	/* overwrite '\n' with '\0' if '\n' present */
	if((a = strchr(b, '\n')))
	{
		a[0] = '\0';
	}
	*value = (char*)strdup(b);

	return 1;
}


/* Reads the config file from disc and stores it in cfg */
int read_config(cfg_entry **cfg, char *file)
{
	char line[80];
	FILE *fp;

	fp = fopen(file, "r");

	if(!fp)
	{
		return 0;
	}

	while(fgets(line, 80, fp) != NULL)
	{
		char *name, *section, *value;
		cfg_entry *entry = NULL;
		if((line[0] == '#'))
			continue; /* Skip comments */

		if((line[0] == '<'))
			continue; /* There's no section on this line */

		if((line[0] == '\n'))
			continue; /* There's no section on this line */

		if(strchr(line, '=') == NULL)
			continue;

		if(!split_line(line, &section, &name, &value))
		{
			free(name);
			free(section);
			free(value);
			continue;
		}

		/* Add the entry when it doesn't exist; we don't want double options*/
		if(!(entry = lookup_entry(cfg, section, name)))
		{
			/* Use strtoul instead of atoi as on some nv40 cards we get issues regarding signed/unsigned */
			add_entry(cfg, section, name, strtoul(value, (char**)NULL, 10));
		}

		free(name);
		free(section);
		free(value);
	}
	fclose(fp);
	return 1;
}


/* Looks if a config file exists and then makes sure it will be read and parsed.
/  When no config file exists create it.
*/
int open_config()
{
	char *file = NULL;
	char *home = NULL;
	struct stat stat_buf;

	if(!(home = getenv("HOME")))
	{
		/* This code should only get entered when nvclock is started at system startup.
		/  The rest of the code and mainly the GTK version isn't prepared for this yet.
		*/
		return 1;
	}

	nvclock.path = malloc(strlen(home) + strlen("/.nvclock") +1);
	sprintf(nvclock.path, "%s/.nvclock", home);
	file = malloc(strlen(nvclock.path) + strlen("/config") +1);
	sprintf(file, "%s/config", nvclock.path);

	/* Check if our ~/.nvclock directory exists if not create it */
	if(stat(nvclock.path, &stat_buf))
	{
		if(mkdir(nvclock.path, 0755))
		{
			char buf[80];
			sprintf(buf, "Can't create '%s'. Do you have sufficient permissions?\n", nvclock.path);
			set_error_str(buf);
			return 0;
		}
		stat(nvclock.path, &stat_buf);
	}

	/* Check if .nvclock really is a directory. For some users it was a file and this led to a segfault. */
	if(!S_ISDIR(stat_buf.st_mode))
	{
		char buf[80];
		sprintf(buf, "Can't open '%s'. Is it really a directory?\n", nvclock.path);
		set_error_str(buf);
		return 0;
	}

	/* If there's no config or if the config is corrupt create a new one */
	if(!parse_config(file))
	{
		create_config(file);
	}
	free(file);
	return 1;
}


void write_config(cfg_entry *cfg, char *file)
{
	FILE *fp = fopen(file, "w+");
	cfg_entry *pCfg = NULL;

	pCfg = cfg;

	fprintf(fp, "#This is NVClock's config file. Don't edit the hw and general section!\n");
	while(pCfg != NULL)
	{
		fprintf(fp, "%s.%s=%u\n", pCfg->section, pCfg->name, pCfg->value);
		pCfg = pCfg->next;
	}
	fclose(fp);
}


/* Some basic check to see if frequencies can be valid */
static int validate_clock(int arch, int freq)
{
	if(arch & (NV5))
	{
		if((freq > 75) && (freq < 250))
			return 1;
	}
	/* Geforce2/2MX/4MX */
	else if(arch & (NV1X))
	{
		if((freq > 125) && (freq < 500))
			return 1;
	}
	/* Geforce3/3Ti/4Ti/FX5200/FX5500 */
	else if(arch & (NV2X))
	{
		if((freq > 200) && (freq < 800))
			return 1;
	}
	/* GeforceFX */
	else if(arch & (NV3X))
	{
		if((freq > 250) && (freq < 1100))
			return 1;
	}
	/* Geforce6/7/8*/
	else if(arch & (NV4X | NV5X))
	{
		if((freq > 250) && (freq < 1200))
			return 1;
	}
	return 0;
}


/* Some basic check to verify if a stored pll can be correct */
static int validate_pll(int arch, int base_freq, unsigned int pll, unsigned int pll2)
{
	int freq;

	if(arch & (NV5 | NV1X | NV2X))
	{
		freq = (int)GetClock(base_freq, pll);
		if(validate_clock(arch, freq))
			return 1;
	}
	else if(arch & (NV3X))
	{
		freq = (int)GetClock_nv30(base_freq, pll, pll2);
		if(validate_clock(arch, freq))
			return 1;
	}
	else if(arch & (NV4X))
	{
		freq = (int)GetClock_nv40(base_freq, pll, pll2);
		if(validate_clock(arch, freq))
			return 1;
	}
	else if(arch & (NV5X))
	{
		freq = (int)GetClock_nv50(base_freq, pll, pll2);
		if(validate_clock(arch, freq))
			return 1;
	}
	return 0;
}


/* Parse the config file and do something with its contents */
int parse_config(char *file)
{
	int i;
	cfg_entry *general;

	if(!read_config(&nvclock.cfg, file))
		return 0;

	if((general = (cfg_entry*)lookup_entry(&nvclock.cfg, "general", "cards")) == NULL)
	{
		return 0;
	}
	else
	{
		/* Check if we have the same number of videocards as before */
		if(general->value != nvclock.num_cards)
			return 0;

		/* Walk through all detected cards */
		for(i=0; i < nvclock.num_cards; i++)
		{
			cfg_entry *entry;
			char section[4];
			char filename[80];
			int base_freq=0;
			struct stat tmp;
			struct nvbios *bios;

			if(!set_card_info(i))
				return 0; /* Make us fail; set_card_info already set the error */

			sprintf(section, "hw%d", i);

			if((entry = (cfg_entry*)lookup_entry(&nvclock.cfg, section, "card")))
			{
				/* The order in wich the detected cards are listed should match the order of the ones in the config file. Mask the last digit for device_id modding purposes.*/
				if((nvclock.card[i].device_id & 0xfff0) != (entry->value & 0xfff0))
					return 0;
			}
			else
			{
				return 0;
			}

			if((entry = (cfg_entry*)lookup_entry(&nvclock.cfg, section, "basefreq")))
			{
				base_freq = entry->value;
			}
			else
				return 0;

/* The bios works differently on ppc and other architectures; it is also stored in a different place */
#if defined(__i386__) || defined(__ia64__) || defined(__x86_64__)
			/* During this stage we also need to parse the nvidia bios.
			/  This can't be done in probe_devices as it depends on the config file
			/  which might not exist at that time yet
			*/
			sprintf(filename, "%s/bios%d.rom", nvclock.path, i);

			/* Redump the bios when the file doesn't exist */
			if(stat(filename, &tmp))
				return 0;

			/* Read the bios. Note the result can be NULL in case the
			/  bios is corrupt. We don't redump the bios because the bios
			/  is not dumpable on some systems. For example on various laptops
			/  the bios is stored at a different place not reachable by us.
			*/
			bios = read_bios(filename); /* GCC 4.0.1 (what about others?) doesn't like it when we directly do nclock.card[i].bios = readbios(filename); works fine without optimizations */
			nvclock.card[i].bios = bios;
#else
			nvclock.card[i].bios = NULL;
#endif

			if((entry = (cfg_entry*)lookup_entry(&nvclock.cfg, section, "mpll")))
				nvclock.card[i].mpll = entry->value;
			else
				/* corrupted config file */
				return 0;

			if((entry = (cfg_entry*)lookup_entry(&nvclock.cfg, section, "nvpll")))
				nvclock.card[i].nvpll = entry->value;
			else
				return 0;

			/* NV31, NV4X and NV5X cards have extra pll registers; in case of NV30 the second pll is ignored but that happens in GetClock_nv30 */
			if(nv_card->arch & (NV3X | NV4X | NV5X))
			{
				if((entry = (cfg_entry*)lookup_entry(&nvclock.cfg, section, "mpll2")))
					nvclock.card[i].mpll2 = entry->value;
				else
					return 0;

				if((entry = (cfg_entry*)lookup_entry(&nvclock.cfg, section, "nvpll2")))
					nvclock.card[i].nvpll2 = entry->value;
				else
					return 0;
			}

			/* Do some basic check on mpll/nvpll to see if they can be correct */
			if(!validate_pll(nvclock.card[i].arch, base_freq, nvclock.card[i].mpll, nvclock.card[i].mpll2))
				return 0;
			if(!validate_pll(nvclock.card[i].arch, base_freq, nvclock.card[i].nvpll, nvclock.card[i].nvpll2))
				return 0;
		}
	}

	/* Reset the nv_card object else things might go wrong */
	unset_card();

	/* Return succes */
	return 1;
}


/* Create a config file based on info we get from the low-level part of nvclock */
int create_config(char *file)
{
	int i;

	if(nvclock.cfg)
	{
		destroy(&nvclock.cfg);
		nvclock.cfg = NULL;
	}
	add_entry(&nvclock.cfg, "general", "cards", nvclock.num_cards);

	/* Write the config file */
	for(i=0; i < nvclock.num_cards; i++)
	{
		char section[4];
		char bios[80];
		int base_freq;

		/* Set the nv_card object to the card; Note this is a bit basic; function pointers can't be used */
		if(!set_card_info(i))
			return 0; /* Make us fail; set_card_info already set the error */

		sprintf(section, "hw%d", i);
		add_entry(&nvclock.cfg, section, "card", nv_card->device_id);

#if defined(__i386__) || defined(__ia64__) || defined(__x86_64__)
		/* needs to be changed to dump the file in the home dir; further we need some CRC check */
		sprintf(bios, "%s/bios%d.rom", nvclock.path, i);
		dump_bios(bios);
		nvclock.card[i].bios = read_bios(bios);
#else
		nvclock.card[i].bios = NULL;
#endif

		base_freq = (nv_card->PEXTDEV[0x0000/4] & 0x40) ? 14318 : 13500;
		if(nv_card->arch & (NV17 | NV25 | NV3X | NV4X | NV5X))
		{
			if (nv_card->PEXTDEV[0x0000/4] & (1<<22))
				base_freq = 27000;
		}
		add_entry(&nvclock.cfg, section, "basefreq", base_freq);

		/* TNT(1/2) and Geforce(1/2/3/4) */
		if(nv_card->arch & (NV5 | NV1X | NV2X))
		{
			add_entry(&nvclock.cfg, section, "mpll", nv_card->PRAMDAC[0x504/4]);
			add_entry(&nvclock.cfg, section, "nvpll", nv_card->PRAMDAC[0x500/4]);
			nvclock.card[i].nvpll = nv_card->PRAMDAC[0x500/4];
			nvclock.card[i].mpll = nv_card->PRAMDAC[0x504/4];
		}
		/* GeforceFX 5200/5500/5600/5700/5800/5900, note that FX5600/5700 require two PLLs */
		else if(nv_card->arch & NV3X)
		{
			add_entry(&nvclock.cfg, section, "mpll", nv_card->PRAMDAC[0x504/4]);
			add_entry(&nvclock.cfg, section, "mpll2", nv_card->PRAMDAC[0x574/4]);
			add_entry(&nvclock.cfg, section, "nvpll", nv_card->PRAMDAC[0x500/4]);
			add_entry(&nvclock.cfg, section, "nvpll2", nv_card->PRAMDAC[0x570/4]);
			nvclock.card[i].mpll = nv_card->PRAMDAC[0x504/4];
			nvclock.card[i].mpll2 = nv_card->PRAMDAC[0x574/4];
			nvclock.card[i].nvpll = nv_card->PRAMDAC[0x500/4];
			nvclock.card[i].nvpll2 = nv_card->PRAMDAC[0x570/4];
		}
		/* Geforce6/7 */
		else if(nv_card->arch & NV4X)
		{
			add_entry(&nvclock.cfg, section, "mpll", nv_card->PMC[0x4020/4]);
			add_entry(&nvclock.cfg, section, "mpll2", nv_card->PMC[0x4024/4]);
			add_entry(&nvclock.cfg, section, "nvpll", nv_card->PMC[0x4000/4]);
			add_entry(&nvclock.cfg, section, "nvpll2", nv_card->PMC[0x4004/4]);
			nvclock.card[i].mpll = nv_card->PMC[0x4020/4];
			nvclock.card[i].mpll2 = nv_card->PMC[0x4024/4];
			nvclock.card[i].nvpll = nv_card->PMC[0x4000/4];
			nvclock.card[i].nvpll2 = nv_card->PMC[0x4004/4];
		}
		/* Geforce8 */
		else if(nv_card->arch & NV5X)
		{
			add_entry(&nvclock.cfg, section, "mpll", nv_card->PMC[0x4008/4]);
			add_entry(&nvclock.cfg, section, "mpll2", nv_card->PMC[0x400c/4]);
			add_entry(&nvclock.cfg, section, "nvpll", nv_card->PMC[0x4028/4]);
			add_entry(&nvclock.cfg, section, "nvpll2", nv_card->PMC[0x402c/4]);
			nvclock.card[i].mpll = nv_card->PMC[0x4008/4];
			nvclock.card[i].mpll2 = nv_card->PMC[0x400c/4];
			nvclock.card[i].nvpll = nv_card->PMC[0x4028/4];
			nvclock.card[i].nvpll2 = nv_card->PMC[0x402c/4];
		}

		unset_card();
	}
	write_config(nvclock.cfg, file);
	return 1;
}

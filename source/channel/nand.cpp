/***************************************************************************
 * Copyright (C) 2011 by Miigotu
 *           (C) 2012 by OverjoY
 *
 * Rewritten code from Mighty Channels and Triiforce
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any
 * damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any
 * purpose, including commercial applications, and to alter it and
 * redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you
 * must not claim that you wrote the original software. If you use
 * this software in a product, an acknowledgment in the product
 * documentation would be appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and
 * must not be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 * distribution.
 *
 * Nand/Emulation Handling Class for Wiiflow
 *
 ***************************************************************************/
 
#include <ogc/machine/processor.h>
#include <stdio.h>
#include <ogcsys.h>
#include <string.h>
#include <cstdlib>
#include <stdarg.h>
#include <dirent.h>
#include <malloc.h>

#include "nand.hpp"
#include "identify.h"
#include "fileOps/fileOps.h"
#include "gecko/gecko.hpp"
#include "gui/text.hpp"
#include "loader/alt_ios.h"
#include "loader/cios.h"
#include "loader/fs.h"
#include "loader/sys.h"
#include "loader/wbfs.h"
#include "memory/memory.h"
#include "wiiuse/wpad.h"

u8 *confbuffer = (u8*)NULL;
u8 CCode[0x1008];
char *txtbuffer = (char *)NULL;

config_header *cfg_hdr;

bool tbdec = false;
bool configloaded = false;
bool emu_enabled = false;

Nand NandHandle;

static NandDevice NandDeviceList[] = 
{
	{ "Disable",						0,	0x00,	0x00 },
	{ "SD/SDHC Card",					1,	0xF0,	0xF1 },
	{ "USB 2.0 Mass Storage Device",	2,	0xF2,	0xF3 },
};

void Nand::Init()
{
	MountedDevice = 0;
	EmuDevice = WII_NAND;
	Partition = 0;
	FullMode = 0x100;
	memset(NandPath, 0, sizeof(NandPath));
	isfs_inited = false;
}
/*
bool Nand::LoadDefaultIOS(void)
{
	Patch_AHB(); // apply a patch so the new IOS will also have AHBPROT disabled
	s32 ret = IOS_ReloadIOS(IOS_GetPreferredVersion()); // reload to preferred IOS, not sure what wiiflows preferred IOS is.
	loadIOS(IOS_GetVersion(), false); // this basically does nothing (well very little), definitely doesn't load a IOS or shutdown anything.
	Init_ISFS();
	return (ret == 0);
}
*/
void Nand::SetNANDEmu(u32 partition)
{
	EmuDevice = partition == 0 ? EMU_SD : EMU_USB;
	Partition = partition > 0 ? partition - 1 : partition;
}

s32 Nand::Nand_Mount(NandDevice *Device)
{
	gprintf("Device: %s\n", Device->Name);

	s32 fd = IOS_Open("fat", 0);
	if(fd < 0)
		return fd;

	static ioctlv vector[1] ATTRIBUTE_ALIGN(32);	

	vector[0].data = &Partition;
	vector[0].len = sizeof(u32);

	s32 ret = IOS_Ioctlv(fd, Device->Mount, 1, 0, vector);
	IOS_Close(fd);

	return ret;
}

s32 Nand::Nand_Unmount(NandDevice *Device)
{
	s32 fd = IOS_Open("fat", 0);
	if(fd < 0)
		return fd;

	s32 ret = IOS_Ioctlv(fd, Device->Unmount, 0, 0, NULL);
	IOS_Close(fd);

	return ret;
}

s32 Nand::Nand_Enable(NandDevice *Device)
{
	gprintf("Enabling NAND Emulator...");
	s32 fd = IOS_Open("/dev/fs", 0);
	if(fd < 0)
	{
		gprintf(" failed!\n");
		return fd;
	}

	int NandPathlen = strlen(NandPath) + 1;

	static ioctlv vector[2] ATTRIBUTE_ALIGN(32);

	static u32 mode ATTRIBUTE_ALIGN(32) = Device->Mode | FullMode;

	vector[0].data = &mode;
	vector[0].len = sizeof(u32);
	vector[1].data = NandPath;
	vector[1].len = NandPathlen;

	s32 ret = IOS_Ioctlv(fd, 100, 2, 0, vector);
	IOS_Close(fd);
	
	gprintf(" %s!\n", ret < 0 ? "failed" : "OK");
	return ret;
} 

s32 Nand::Nand_Disable(void)
{
	gprintf("Disabling NAND Emulator\n");
	s32 fd = IOS_Open("/dev/fs", 0);
	if(fd < 0)
		return fd;

	u32 inbuf ATTRIBUTE_ALIGN(32) = 0;
	s32 ret = IOS_Ioctl(fd, 100, &inbuf, sizeof(inbuf), NULL, 0);
	IOS_Close(fd);

	return ret;
} 

s32 Nand::Enable_Emu()
{
	if(emu_enabled)
		return 0;
	NandDevice *Device = &NandDeviceList[EmuDevice];

	s32 ret = Nand_Mount(Device);
	if(ret < 0)
		return ret;

	ret = Nand_Enable(Device);
	if(ret < 0)
		return ret;

	MountedDevice = EmuDevice;

	emu_enabled = true;
	return 0;
}	

s32 Nand::Disable_Emu()
{
	if(MountedDevice == 0 || !emu_enabled)
	{
		emu_enabled = false;
		return 0;
	}
	emu_enabled = false;
	NandDevice *Device = &NandDeviceList[MountedDevice];

	Nand_Disable();
	Nand_Unmount(Device);

	MountedDevice = 0;
	return 0;
}

bool Nand::EmulationEnabled(void)
{
	return emu_enabled;
}

void Nand::__Dec_Enc_TB(void) 
{	
	u32 key = 0x73B5DBFA;
    int i;
    
	for( i=0; i < 0x100; ++i ) 
	{    
		txtbuffer[i] ^= key&0xFF;
		key = (key<<1) | (key>>31);
	}
	
	tbdec = tbdec ? false : true;	
}

void Nand::__configshifttxt(char *str)
{
	const char *ptr = str;
	char *ctr = str;
	int i;
	int j = strlen(str);
	for( i=0; i<j; ++i )
	{		
		if( strncmp( str+(i-3), "PALC", 4 ) == 0 )
			*ctr = 0x0d;
		else if( strncmp( str+(i-2), "LUH", 3 ) == 0 )
			*ctr = 0x0d;
		else if( strncmp( str+(i-2), "LUM", 3 ) == 0 )	
			*ctr = 0x0d;
		else 
			*ctr = str[i];

		ctr++;
	}	
	*ctr = *ptr;
	*ctr = '\0';
}

void Nand::__GetNameList(const char *source, namelist **entries, int *count)
{
	u32 i, j, k, l;
	u32 numentries = 0;	
	char *names = NULL;
	char curentry[ISFS_MAXPATH]; // 64
	char entrypath[ISFS_MAXPATH]; // 64

	s32 ret = ISFS_ReadDir(source, NULL, &numentries);
	names = (char *)memalign(32, ALIGN32((ISFS_MAXPATH) * numentries));
	if(names == NULL)
		return;

	ret = ISFS_ReadDir(source, names, &numentries);	
	*count = numentries;

	if(*entries != NULL)
	{
		free(*entries);
		*entries = NULL;
	}

	*entries = (namelist *)malloc(sizeof(namelist) * numentries);
	if(*entries == NULL)
	{
		free(names);
		return;
	}

	for(i = 0, k = 0; i < numentries; i++)
	{
		for(j = 0; names[k] != 0; j++, k++)
			curentry[j] = names[k];

		curentry[j] = 0;
		k++;

		strcpy((*entries)[i].name, curentry);

		/* This could still cause a buffer overrun */
		strcpy(entrypath, source);
		if(source[strlen(source)-1] != '/')
			strcat(entrypath, "/");
		strcat(entrypath, curentry);
		
		ret = ISFS_ReadDir(entrypath, NULL, &l);		
		(*entries)[i].type = ret < 0 ? 0 : 1;
	}	
	free(names);
}

u32 Nand::__GetSystemMenuRegion(void) // not used anymore
{
	u32 Region = EUR;
	char *source = (char *)memalign(32, 256);
	strcpy(source, SMTMDPATH);

	s32 fd = ISFS_Open(source, ISFS_OPEN_READ);
	if(fd >= 0)
	{
		fstats *status = (fstats *)memalign(32, ALIGN32(sizeof(fstats)));
		ISFS_GetFileStats(fd, status);
		char *TMD = (char*)memalign(32, status->file_length);
		ISFS_Read(fd, TMD, status->file_length);
		Region = *(u16*)(TMD+0x1DC) & 0xF;
		ISFS_Close(fd);
		// free(TMD); // commented out to fix an issue on game boot
		free(status);
	}
	free(source);
	return Region;
}

s32 Nand::__configclose(void) // not used anymore
{
	__Dec_Enc_TB();
	free(confbuffer);
	free(txtbuffer);
	configloaded = !configloaded;
	return configloaded;
}

s32 Nand::__configread(void)
{	
	confbuffer = (u8 *)memalign(32, 0x4000);
	txtbuffer = (char *)memalign(32, 0x100);
	if(confbuffer == NULL || txtbuffer == NULL)
		return -1;
		
	char *source = (char *)memalign(32, 256);
	strcpy(source, SYSCONFPATH);
	
	s32 fd = ISFS_Open(source, ISFS_OPEN_READ);
	if(fd < 0)
	{
		free(confbuffer);
		free(txtbuffer);
		free(source);
		return 0;
	}

	ISFS_Read(fd, confbuffer, 0x4000);
	ISFS_Close(fd);
	
	strcpy(source, TXTPATH);
	fd = ISFS_Open(source, ISFS_OPEN_READ);
	if(fd < 0)
	{
		free(confbuffer);
		free(txtbuffer);
		free(source);
		return 0;
	}

	ISFS_Read(fd, txtbuffer, 0x100);
	ISFS_Close(fd);
	free(source);

	cfg_hdr = (config_header *)confbuffer;

	__Dec_Enc_TB();

	configloaded = configloaded ? false : true;

	if(tbdec && configloaded)
		return 1;

	return 0;
}

s32 Nand::__configwrite(bool realnand)
{
	if(configloaded)
	{
		__Dec_Enc_TB();

		if(!tbdec)
		{
			if(realnand)
			{
				char *dest = (char *)memalign(32, 256);
				strcpy(dest, TSYSCONFPATH);
				ISFS_Delete(dest);
				ISFS_CreateFile(dest, 0, 3, 3, 3);
				s32 fd = ISFS_Open(dest, ISFS_OPEN_RW);
				if(fd < 0)
				{
					free(confbuffer);
					free(txtbuffer);
					free(dest);
					configloaded = !configloaded;
					return 0;
				}
				ISFS_Write(fd, confbuffer, 0x4000);
				ISFS_Close(fd);
				
				strcpy(dest, TTXTPATH);
				ISFS_Delete(dest);
				ISFS_CreateFile(dest, 0, 3, 3, 3);
				fd = ISFS_Open(dest, ISFS_OPEN_RW);
				if(fd < 0)
				{
					free(confbuffer);
					free(txtbuffer);
					free(dest);
					configloaded = !configloaded;
					return 0;
				}
				ISFS_Write(fd, txtbuffer, 0x100);
				ISFS_Close(fd);
				configloaded = !configloaded;
				free(dest);				
			}
			else
			{		
				/* SYSCONF */
				fsop_WriteFile(cfgpath, confbuffer, 0x4000);
				/* setting.txt */
				fsop_WriteFile(settxtpath, txtbuffer, 0x100);

				configloaded = !configloaded;
			}
			if(!tbdec && !configloaded)
			{
				free(confbuffer);
				free(txtbuffer);
				return 1;
			}
		}
	}
	free(confbuffer);
	free(txtbuffer);
	return 0;
}

u32 Nand::__configsetbyte(const char *item, u8 val)
{
	u32 i;
	for(i = 0; i < cfg_hdr->ncnt; ++i)
	{
		if(memcmp(confbuffer+(cfg_hdr->noff[i] + 1), item, strlen(item)) == 0)
		{
			*(u8*)(confbuffer+cfg_hdr->noff[i] + 1 + strlen(item)) = val;
			break;
		}
	}
	return 0;
}

u32 Nand::__configsetbigarray(const char *item, void *val, u32 size)
{
	u32 i;
	for(i = 0; i < cfg_hdr->ncnt; ++i)
	{
		if(memcmp(confbuffer+(cfg_hdr->noff[i] + 1), item, strlen(item)) == 0)
		{
			memcpy(confbuffer+cfg_hdr->noff[i] + 3 + strlen(item), val, size);
			break;
		}
	}
	return 0;
}

u32 Nand::__configsetsetting(const char *item, const char *val)
{		
	char *curitem = strstr(txtbuffer, item);
	char *curstrt, *curend;
	if(curitem == NULL)
		return 0;
	
	curstrt = strchr(curitem, '=');
	curend = strchr(curitem, 0x0d);

	if(curstrt && curend)
	{
		curstrt += 1;
		u32 len = curend - curstrt;
		if(strlen(val) > len)
		{
			static char buffer[0x100];
			u32 nlen;
			nlen = txtbuffer-(curstrt+strlen(val));
			strcpy(buffer, txtbuffer + nlen);
			memcpy(curstrt, val, strlen(val)); //
			curstrt += strlen(val); 
			memcpy(curstrt, buffer, strlen(buffer)); //
		}
		else
		{
			memcpy(curstrt, val, strlen(val)); //
		}

		__configshifttxt(txtbuffer);

		return 1;
	}
	return 0;
}

void Nand::__FATify(char *ptr, const char *str)
{
	char ctr;
	while((ctr = *(str++)) != '\0') 
	{
		const char *esc;
		switch(ctr) 
		{
			case '"':
				esc = "&qt;";
			break;
			case '*':
				esc = "&st;";
			break;
			case ':':
				esc = "&cl;";
			break;
			case '<':
				esc = "&lt;";
			break;
			case '>':
				esc = "&gt;";
			break;
			case '?':
				esc = "&qm;";
			break; 
			case '|':
				esc = "&vb;";
			break;
			default:
				*(ptr++) = ctr;
				continue;
		}
		strcpy(ptr, esc);
		ptr += 4;
	}
	*ptr = '\0';
}

void Nand::__NANDify(char *str)
{
	char *src = str;
	char *dst = str;
	char c;

	while((c = *(src++)) != '\0') 
	{
		if(c == '&') 
		{
			if(!strncmp(src, "qt;", 3))
				c = '"';
			else if(!strncmp(src, "st;", 3))
				c = '*';
			else if(!strncmp(src, "cl;", 3))
				c = ':';
			else if(!strncmp(src, "lt;", 3))
				c = '<';
			else if(!strncmp(src, "gt;", 3))
				c = '>';
			else if(!strncmp(src, "qm;", 3))
				c = '?';
			else if(!strncmp(src, "vb;", 3))
				c = '|';

			if (c != '&')
				src += 3;
		} 
		*(dst++) = c;
	}
	*dst = '\0';
}

s32 Nand::__FlashNandFile(const char *source, const char *dest)
{
	s32 ret;
	FILE *file = fopen(source, "rb");
	if(!file) 
	{
		gprintf("Error opening source: \"%s\"\n", source);
		return 0;
	}

	fseek(file, 0, SEEK_END);
	u32 fsize = ftell(file);
	fseek(file, 0, SEEK_SET);

	if(fake)
	{
		NandSize += fsize;
		if(showprogress)
			dumper(NandSize, 0x1f400000, 0x1f400000, NandSize, FilesDone, FoldersDone, "", data);
		fclose(file);
		return 0;
	}

	gprintf("Flashing: %s (%uKB) to nand...", dest, (fsize / 0x400)+1);

	ISFS_Delete(dest);
	ISFS_CreateFile(dest, 0, 3, 3, 3);
	s32 fd = ISFS_Open(dest, ISFS_OPEN_RW);
	if(fd < 0)
	{
		gprintf(" failed\nError: ISFS_OPEN(%s, %d) %d\n", dest, ISFS_OPEN_RW, fd);
		fclose(file);
		return fd;
	}

	u8 *buffer = (u8 *)memalign(32, ALIGN32(BLOCK));
	if(buffer == NULL)
		return -1;

	u32 toread = fsize;
	while(toread > 0)
	{
		u32 size = BLOCK;
		if(toread < BLOCK)
			size = toread;

		ret = fread(buffer, 1, size, file);
		if(ret <= 0) 
		{
			gprintf(" failed\nError: fread(%p, 1, %d, %s) %d\n", buffer, size, source, ret);
			ISFS_Close(fd);
			fclose(file);
			free(buffer);
			return ret;
		}

		ret = ISFS_Write(fd, buffer, size);
		if(ret <= 0) 
		{
			gprintf(" failed\nError: ISFS_Write(%d, %p, %d) %d\n", fd, buffer, size, ret);
			ISFS_Close(fd);
			fclose(file);
			free(buffer);
			return ret;
		}
		toread -= size;
		NandDone += size;
		FileDone += size;

		if(showprogress)
		{
			const char *file = strrchr(dest, '/')+1;
			dumper(NandDone, NandSize, fsize, FileDone, FilesDone, FoldersDone, file, data);
		}
	}
	gprintf(" done!\n");
	FilesDone++;
	if(showprogress)
	{
		const char *file = strrchr(dest, '/')+1;
		dumper(NandDone, NandSize, fsize, FileDone, FilesDone, FoldersDone, file, data);
	}
	ISFS_Close(fd);
	free(buffer);
	fclose(file);
	return 1;
}

s32 Nand::__DumpNandFile(const char *source, const char *dest)
{
	FileDone = 0;
	s32 fd = ISFS_Open(source, ISFS_OPEN_READ);
	if (fd < 0) 
	{
		gprintf("Error: IOS_OPEN(%s, %d) %d\n", source, ISFS_OPEN_READ, fd);
		return fd;
	}

	fstats *status = (fstats *)memalign(32, ALIGN32(sizeof(fstats)));
	if(status == NULL)
		return -1;

	s32 ret = ISFS_GetFileStats(fd, status);
	if(ret < 0)
	{
		gprintf("Error: ISFS_GetFileStats(%d) %d\n", fd, ret);
		ISFS_Close(fd);
		free(status);
		return ret;
	}

	if(fake)
	{
		NandSize += status->file_length;
		if(showprogress)
			dumper(NandSize, 0x1f400000, 0x1f400000, NandSize, FilesDone, FoldersDone, "", data);
		ISFS_Close(fd);
		free(status);
		return 0;
	}

	if(fsop_FileExist(dest)) //
		fsop_deleteFile(dest);

	FILE *file = fopen(dest, "wb");
	if(!file)
	{
		gprintf("Error opening destination: \"%s\"\n", dest);
		ISFS_Close(fd);
		free(status);
		return 0;
	}

	gprintf("Dumping: %s (%ukb)...", source, (status->file_length / 0x400)+1);

	u8 *buffer = (u8 *)memalign(32, ALIGN32(BLOCK));
	if(buffer == NULL)
	{
		free(status);
		return -1;
	}

	u32 toread = status->file_length;
	while(toread > 0)
	{
		u32 size = BLOCK;
		if (toread < BLOCK)
			size = toread;

		ret = ISFS_Read(fd, buffer, size);
		if (ret < 0)
		{
			gprintf(" failed\nError: ISFS_Read(%d, %p, %d) %d\n", fd, buffer, size, ret);
			ISFS_Close(fd);
			fclose(file);
			free(status);
			free(buffer);
			return ret;
		}

		ret = fwrite(buffer, 1, size, file);
		if(ret < 0) 
		{
			gprintf(" failed\nError writing to destination: \"%s\" (%d)\n", dest, ret);
			ISFS_Close(fd);
			fclose(file);
			free(status);
			free(buffer);
			return ret;
		}
		toread -= size;
		NandDone += size;
		FileDone += size;

		if(showprogress)
		{
			const char *file = strrchr(source, '/')+1;
			dumper(NandDone, NandSize, status->file_length, FileDone, FilesDone, FoldersDone, file, data);
		}
	}
	FilesDone++;
	if(showprogress)
	{
		const char *file = strrchr(source, '/')+1;
		dumper(NandDone, NandSize, status->file_length, FileDone, FilesDone, FoldersDone, file, data);
	}
	gprintf(" done!\n");
	fclose(file);
	ISFS_Close(fd);
	free(status);
	free(buffer);

	return 0;
}


s32 Nand::__FlashNandFolder(const char *source, const char *dest)
{	
	char nsource[MAX_FAT_PATH];//1024
	char ndest[ISFS_MAXPATH];//64

	DIR *dir_iter;
	struct dirent *ent;

	dir_iter = opendir(source);
	if (!dir_iter)
		return 1;

	while((ent = readdir(dir_iter)) != NULL)
	{
		if(ent->d_name[0] == '.') 
			continue;

		/* This could still cause a buffer overrun */
		strcpy(ndest, dest);
		if(dest[strlen(dest)-1] != '/')
			strcat(ndest, "/");
		strcat(ndest, ent->d_name);

		if(source[strlen(source)-1] == '/')
			snprintf(nsource, sizeof(nsource), "%s%s", source, ent->d_name);
		else
			snprintf(nsource, sizeof(nsource), "%s/%s", source, ent->d_name);

		if(ent->d_type == DT_DIR)
		{
			__NANDify(ndest);
			if(!fake)
			{
				ISFS_CreateDir(ndest, 0, 3, 3, 3);
				FoldersDone++;
			}
			__FlashNandFolder(nsource, ndest);
		}
		else
		{
			__NANDify(ndest);
			__FlashNandFile(nsource, ndest);
		}
	}
	return 0;
}

s32 Nand::__DumpNandFolder(const char *source, const char *dest)
{
	namelist *names = NULL;
	int cnt, i;
	char nsource[ISFS_MAXPATH];
	char ndest[MAX_FAT_PATH];
	char tdest[MAX_FAT_PATH];

	__GetNameList(source, &names, &cnt);

	for(i = 0; i < cnt; i++) 
	{
		/* This could still cause a buffer overrun */
		strcpy(nsource, source);
		if(source[strlen(source)-1] != '/')
			strcat(nsource, "/");
		strcat(nsource, names[i].name);
		
		if(!names[i].type)
		{
			__FATify(tdest, nsource);
			snprintf(ndest, sizeof(ndest), "%s%s", dest, tdest);
			__DumpNandFile(nsource, ndest);
		}
		else
		{
			if(!fake)
			{
				__FATify(tdest, nsource);
				CreatePath("%s%s", dest, tdest);
				FoldersDone++;
			}
			
			__DumpNandFolder(nsource, dest);
		}
	}
	free(names);
	return 0;
}

void Nand::CreatePath(const char *path, ...)
{
	char *folder = NULL;
	va_list args;
	va_start(args, path);
	if((vasprintf(&folder, path, args) >= 0) && folder)
	{
		if(folder[strlen(folder)-1] == '/')
			folder[strlen(folder)-1] = 0;

		char *check = folder;
		while(true)
		{
			check = strstr(folder, "//");
			if (check != NULL)
				strcpy(check, check + 1);
			else
				break;
		}
		__makedir(folder);
		free(folder);
	}
	va_end(args);
}

void Nand::CreateTitleTMD(dir_discHdr *hdr)
{
	wbfs_disc_t *disc = WBFS_OpenDisc((u8 *)&hdr->id, (char *)hdr->path);
	if(!disc) 
		return;

	u8 *titleTMD = NULL;
	u32 tmd_size = wbfs_extract_file(disc, (char*)"TMD", (void**)&titleTMD);
	WBFS_CloseDisc(disc);

	if(titleTMD == NULL)
		return;

	unsigned long highTID = *(unsigned long*)(titleTMD+0x18c);
	unsigned long lowTID = *(unsigned long*)(titleTMD+0x190);

	CreatePath("%s/title/%08x/%08x/data", FullNANDPath, highTID, lowTID);
	CreatePath("%s/title/%08x/%08x/content", FullNANDPath, highTID, lowTID);

	char nandpath[MAX_FAT_PATH];
	snprintf(nandpath, sizeof(nandpath), "%s/title/%08lx/%08lx/content/title.tmd", FullNANDPath, highTID, lowTID);

	if(fsop_FileExist(nandpath))
	{
		free(titleTMD);
		gprintf("%s exists!\n", nandpath);
		return;
	}

	gprintf("Creating title TMD: %s\n", nandpath);
	bool res = fsop_WriteFile(nandpath, titleTMD, tmd_size);
	if(!res)
		gprintf("Creating title TMD: %s failed\n", nandpath);
	free(titleTMD);
}

s32 Nand::FlashToNAND(const char *source, const char *dest, dump_callback_t i_dumper, void *i_data)
{	
	ISFS_CreateDir(dest, 0, 3, 3, 3);
	data = i_data;
	dumper = i_dumper;
	fake = false;
	showprogress = true;
	__FlashNandFolder(source, dest);
	return 0;
}

s32 Nand::DoNandDump(const char *source, const char *dest, dump_callback_t i_dumper, void *i_data)
{	
	data = i_data;
	dumper = i_dumper;
	fake = false;
	showprogress = true;
	u32 temp = 0;
	s32 ret = ISFS_ReadDir(source, NULL, &temp);
	if(ret < 0)
	{
		char ndest[MAX_FAT_PATH];
		snprintf(ndest, sizeof(ndest), "%s%s", dest, source);
		CreatePath(dest);
		
		__DumpNandFile(source, ndest);
	}
	else
		__DumpNandFolder(source, dest);
	return 0;
}

s32 Nand::CalcFlashSize(const char *source, dump_callback_t i_dumper, void *i_data)
{	
	data = i_data;
	dumper = i_dumper;
	fake = true;
	showprogress = true;
	__FlashNandFolder(source, "");
	return NandSize;
}

s32 Nand::CalcDumpSpace(const char *source, dump_callback_t i_dumper, void *i_data)
{	
	data = i_data;
	dumper = i_dumper;
	fake = true;
	showprogress = true;

	u32 temp = 0;

	s32 ret = ISFS_ReadDir(source, NULL, &temp);
	if(ret < 0)
		__DumpNandFile(source, "");
	else
		__DumpNandFolder(source, "");

	return NandSize;
}

void Nand::ResetCounters(void)
{
	NandSize = 0;
	FilesDone = 0;
	FoldersDone = 0;
	NandDone = 0;
}
 
s32 Nand::CreateConfig()
{
	CreatePath(FullNANDPath);
	CreatePath("%s/shared2", FullNANDPath);
	CreatePath("%s/shared2/sys", FullNANDPath);
	CreatePath("%s/title", FullNANDPath);
	CreatePath("%s/title/00000001", FullNANDPath);
	CreatePath("%s/title/00000001/00000002", FullNANDPath);
	CreatePath("%s/title/00000001/00000002/data", FullNANDPath);

	fake = false;
	showprogress = false;

	memset(cfgpath, 0, sizeof(cfgpath));
	snprintf(cfgpath, sizeof(cfgpath), "%s%s", FullNANDPath, SYSCONFPATH);
	__DumpNandFile(SYSCONFPATH, cfgpath);

	memset(settxtpath, 0, sizeof(settxtpath));
	snprintf(settxtpath, sizeof(settxtpath), "%s%s", FullNANDPath, TXTPATH);
	__DumpNandFile(TXTPATH, settxtpath);

	return 0;
}

s32 Nand::PreNandCfg(bool miis, bool realconfig)
{
	/* Create paths only if they don't exist */
	CreatePath(FullNANDPath);
	CreatePath("%s/shared2", FullNANDPath);
	CreatePath("%s/shared2/sys", FullNANDPath);
	CreatePath("%s/shared2/menu", FullNANDPath);
	CreatePath("%s/shared2/menu/FaceLib", FullNANDPath);
	CreatePath("%s/title", FullNANDPath);
	CreatePath("%s/title/00000001", FullNANDPath);
	CreatePath("%s/title/00000001/00000002", FullNANDPath);
	CreatePath("%s/title/00000001/00000002/data", FullNANDPath);

	char dest[MAX_FAT_PATH];
	
	fake = false;
	showprogress = false;

	if(realconfig)
	{
		snprintf(dest, sizeof(dest), "%s%s", FullNANDPath, SYSCONFPATH);
		__DumpNandFile(SYSCONFPATH, dest);

		snprintf(dest, sizeof(dest), "%s%s", FullNANDPath, TXTPATH);
		__DumpNandFile(TXTPATH, dest);
	}
	if(miis)
	{
		snprintf(dest, sizeof(dest), "%s%s", FullNANDPath, MIIPATH);
		__DumpNandFile(MIIPATH, dest);
	}
	return 0;
}

bool Nand::Do_Region_Change(string id, bool realnand)
{	
/* check of system menu region sometimes prevents games from launching
 this check is not essential since the patch is applied on a per-game basis
 patching a PAL Wii to PAL will only lead to a forced UK conf anyway */
	if(__configread() == 1)
	{
		switch(id[3])
		{
			case 'J':
				// if(realnand && __GetSystemMenuRegion() == JAP)
					// return __configclose();

				gprintf("Switching region to NTSC-J \n");
				CCode[0] = 1;
				__configsetbyte("IPL.LNG", 0);
				__configsetbigarray("SADR.LNG", CCode, 0x1007);
				__configsetsetting("AREA", "JPN");
				__configsetsetting("MODEL", "RVL-001(JPN)");
				__configsetsetting("CODE", "LJM");
				__configsetsetting("VIDEO", "NTSC");
				__configsetsetting("GAME", "JP");
				break;
			case 'E':
				// if(realnand && __GetSystemMenuRegion() == USA)
					// return __configclose();
					
				gprintf("Switching region to NTSC-U \n");
				CCode[0] = 31;
				__configsetbyte("IPL.LNG", 1);
				__configsetbigarray("IPL.SADR", CCode, 0x1007);
				__configsetsetting("AREA", "USA");
				__configsetsetting("MODEL", "RVL-001(USA)");
				__configsetsetting("CODE", "LU");
				__configsetsetting("VIDEO", "NTSC");
				__configsetsetting("GAME", "US");
				break;
			case 'D': // Germany
			case 'F': // France
			case 'I': // Italy
			case 'M': // American Import
			case 'P': // Regular
			case 'S': // Spain
			case 'U': // United Kingdom
			case 'L': // Japanese Import
			case 'H': //
			case 'X': //
			case 'Y': //
			case 'Z': //
				// if(realnand && __GetSystemMenuRegion() == EUR)
					// return __configclose();

				gprintf("Switching region to PAL \n");
				CCode[0] = 110; // UK
				__configsetbyte("IPL.LNG", 1);
				__configsetbigarray("IPL.SADR", CCode, 0x1007);
				__configsetsetting("AREA", "EUR");
				__configsetsetting("MODEL", "RVL-001(EUR)");
				__configsetsetting("CODE", "LEH");
				__configsetsetting("VIDEO", "PAL");
				__configsetsetting("GAME", "EU");
				break;
			case 'K':
			case 'Q':
			case 'T':
				// if(realnand && __GetSystemMenuRegion() == KOR)
					// return __configclose();

				gprintf("Switching region to NTSC-K \n");
				CCode[0] = 137;
				__configsetbyte("IPL.LNG", 9);
				__configsetbigarray("IPL.SADR", CCode, 0x1007);
				__configsetsetting( "AREA", "KOR" );
				__configsetsetting("MODEL", "RVL-001(KOR)");
				__configsetsetting("CODE", "LKM");
				__configsetsetting("VIDEO", "NTSC");
				__configsetsetting("GAME", "KR");
				break;
		}
	}
	__configwrite(realnand);
	return true;
}

/** This deletes nand:/sys/wiiflow.reg left by Do_Region_Change()**/
void Nand::Clear_Region_Patch(void)
{
	char *dest = (char *)memalign(32, 256);
	strcpy(dest, TSYSCONFPATH);
	ISFS_Delete(dest);
	free(dest);
	return;
}

void Nand::Init_ISFS()
{
	if(isfs_inited)
		return;
	PatchIOS(IOS_GetVersion() < 222, isWiiVC);
	usleep(1000);
	gprintf("Init ISFS\n");
	ISFS_Initialize();
	isfs_inited = true;
}

void Nand::DeInit_ISFS()
{
	gprintf("Deinit ISFS\n");
	ISFS_Deinitialize();
	isfs_inited = false;
	usleep(1000);
}

/** Thanks to postloader for that patch **/
#define ES_MODULE_START	(u16*)0x939F0000

static const u16 ticket_check[] = {
    0x685B,          // ldr r3,[r3,#4] ; get TMD pointer
    0x22EC, 0x0052,  // movls r2, 0x1D8
    0x189B,          // adds r3, r3, r2; add offset of access rights field in TMD
    0x681B,          // ldr r3, [r3]   ; load access rights (haxxme!)
    0x4698,          // mov r8, r3     ; store it for the DVD video bitcheck later
    0x07DB           // lsls r3, r3, #31; check AHBPROT bit
};

void Nand::PatchAHB()
{
	for(u16 *patchme = ES_MODULE_START; patchme < ES_MODULE_START + 0x4000; patchme++) 
	{
		if(!memcmp(patchme, ticket_check, sizeof(ticket_check))) 
		{
			// write16/uncached poke doesn't work for this. Go figure.
			patchme[4] = 0x23FF; // li r3, 0xFF
			DCFlushRange(patchme + 4, 2);
			break;
		}
	}
}

/* If AHB protection is currently disabled then call PatchAHB above 
to set the ES_MODULE to keep it disabled for the next IOS */
void Nand::Patch_AHB()
{
	if(AHBPROT_Patched())
	{
		// Disable memory protection
		write16(MEM_PROT, 0);
		// Do patches
		PatchAHB();
		// Enable memory protection
		write16(MEM_PROT, 1);
	}
}

u8 *Nand::GetEmuFile(const char *path, u32 *size, s32 len)
{
	if(path == NULL)
		return NULL;

	char *tmp_path = fmt_malloc("%s%s", FullNANDPath, path);
	if(tmp_path == NULL)
		return NULL;
	u32 filesize = 0;
	bool ret = fsop_GetFileSizeBytes(tmp_path, &filesize);
	if(ret == false || filesize == 0)
	{
		MEM2_free(tmp_path);
		return NULL;
	}
	if(len > 0)
		filesize = min(filesize, (u32)len);
	u8 *tmp_buf = (u8*)MEM2_alloc(filesize);
	if(tmp_buf != NULL)
	{
		FILE *f = fopen(tmp_path, "rb");
		fread(tmp_buf, filesize, 1, f);
		DCFlushRange(tmp_buf, filesize);
		*size = filesize;
		fclose(f);
	}
	MEM2_free(tmp_path);
	return tmp_buf;
}

u64 *Nand::GetChannels(u32 *count)
{
	u32 size = 0;
	u8 *uid_buf = GetEmuFile("/sys/uid.sys", &size);
	if(uid_buf == NULL || size == 0)
		return NULL;

	uid *uid_file = (uid*)uid_buf;
	u32 chans = size/sizeof(uid);
	u64 *title_buf = (u64*)MEM2_alloc(chans*sizeof(u64));
	for(u32 i = 0; i < chans; ++i)
		title_buf[i] = uid_file[i].TitleID;
	MEM2_free(uid_buf);

	DCFlushRange(title_buf, chans);
	*count = chans;
	return title_buf;
}

u8 *Nand::GetTMD(u64 title, u32 *size)
{
	u8 *tmd_buf = NULL;
	u32 tmd_size = 0;
	char *tmd_path = fmt_malloc("/title/%08x/%08x/content/title.tmd", TITLE_UPPER(title), TITLE_LOWER(title));
	if(tmd_path != NULL)
	{
		tmd_buf = GetEmuFile(tmd_path, &tmd_size);
		MEM2_free(tmd_path);
	}
	*size = tmd_size;
	return tmd_buf;
}

void Nand::SetPaths(const char *emuPath, const char *currentPart)
{
	/* emuPath should = /nands/nand_name */

	/* Set wiiflow full nand path */
	snprintf(FullNANDPath, sizeof(FullNANDPath), "%s:%s", currentPart, emuPath);
	// gprintf("Emu NAND Full Path = %s\n", FullNANDPath);	
	
	/* Set IOS compatible NAND Path */
	strncpy(NandPath, emuPath, sizeof(NandPath));
	NandPath[sizeof(NandPath) - 1] = '\0';
	if(strlen(NandPath) == 0)
		strcat(NandPath, "/");
	// gprintf("IOS Compatible NAND Path = %s\n", NandPath);
}

/**
   part of miniunz.c
   Version 1.01e, February 12th, 2005

   Copyright (C) 1998-2005 Gilles Vollant
**/

#include <errno.h>
#include <fcntl.h>
#include <utime.h>

struct stat exists;
static int mymkdir(const char* dirname) 
{
	if(stat(dirname, &exists) == 0)
		return 0;
	return mkdir(dirname, S_IREAD | S_IWRITE);
}

int Nand::__makedir(char *newdir)
{
	if(stat(newdir, &exists) == 0)
		return 0;

	int len = (int)strlen(newdir);
	if(len <= 0)
		return 0;

	char *buffer = (char*)MEM2_alloc(len + 1);
	strcpy(buffer, newdir);

	if(buffer[len-1] == '/')
		buffer[len-1] = '\0';
	if(mymkdir(buffer) == 0)
	{
		MEM2_free(buffer);
		return 1;
	}

	char *p = buffer + 1;
	while(1)
	{
		char hold;
		while(*p && *p != '\\' && *p != '/')
			p++;
		hold = *p;
		*p = 0;
		if((mymkdir(buffer) == -1) && (errno == ENOENT))
		{
			gprintf("couldn't create directory %s\n",buffer);
			MEM2_free(buffer);
			return 0;
		}
		if(hold == 0)
			break;
		*p++ = hold;
	}
	MEM2_free(buffer);
	return 1;
}

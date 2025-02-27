/****************************************************************************
 * Copyright (C) 2012 FIX94
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/
 
#include <malloc.h>
#include "memory/mem2.hpp"
#include "nand_save.hpp"
#include "nand.hpp"
#include "identify.h"
#include "gecko/gecko.hpp"
#include "loader/fs.h"
#include "loader/sys.h"
#include "banner/AnimatedBanner.h"
#include "unzip/U8Archive.h"

extern const u8 save_bin[];
extern const u32 save_bin_size;

NandSave InternalSave;
bool cur_load = false;
u8 cur_ios = 0;

#define BANNER_PATH		"/title/00010000/57465346/data/banner.bin"
#define IOS_SAVE_PATH	"/title/00010000/57465346/data/ios"
#define PORT_SAVE_PATH	"/title/00010000/57465346/data/port"
#define SD_SAVE_PATH	"/title/00010000/57465346/data/sdonly"

NandSave::NandSave()
{
	ret = 0;
	fd = 0;
	memset(&ISFS_Path, 0, ISFS_MAXPATH);
	loaded = false;
}

/* Checks for wiiflow save WFSF by searching for the banner.bin. if found then wiiflow save found. 
If not found then we use save.bin to create banner.bin, tik.bin, and tmd.bin which are compressed into save.bin.
The IOS and port settings are only created if they are changed in startup settings menu.
Also the ticket and its folder are deleted after the tmd is created. */
bool NandSave::CheckSave()
{
	/* 10 million variables */
	u32 u8_bin_size = 0;
	u8 *u8_bin = NULL;
	u32 certSize = 0;
	signed_blob *certBuffer = NULL;
	u32 tmd_bin_size = 0;
	const signed_blob *tmd_bin = NULL;
	u32 tik_bin_size = 0;
	const signed_blob *tik_bin = NULL;
	u32 banner_bin_size = 0;
	const u8 *banner_bin = NULL;
	u32 entries = 0;
	/* Maybe our banner already exist */
	memset(&ISFS_Path, 0, ISFS_MAXPATH);
	strcpy(ISFS_Path, BANNER_PATH);
	fd = ISFS_Open(ISFS_Path, ISFS_OPEN_READ);
	if(fd >= 0)
	{
		ISFS_Close(fd);
		gprintf("Found WiiFlow Save\n");
		goto done;
	}
	// save_bin only contains the tik.bin, tmd.bin, and banner.bin
	// tik.bin and tmd.bin are used to install "/title/00010000/57465346/data/banner.bin"
	// "/title/00010000/57465346/data/ios and port" are not written until you change them in startup settings menu.
	/* Extract our archive */
	u8_bin = DecompressCopy(save_bin, save_bin_size, &u8_bin_size);
	if(u8_bin == NULL || u8_bin_size == 0)
		goto error;
	/* Grab cert.sys */
	memset(&ISFS_Path, 0, ISFS_MAXPATH);
	strcpy(ISFS_Path, "/sys/cert.sys");
	certBuffer = (signed_blob*)ISFS_GetFile(ISFS_Path, &certSize, -1);
	if(certBuffer == NULL || certSize == 0)
		goto error;
	/* Install tik and tmd */
	tik_bin = (const signed_blob*)u8_get_file(u8_bin, "tik.bin", &tik_bin_size);
	if(tik_bin == NULL || tik_bin_size == 0)
		goto error;
	ret = ES_AddTicket(tik_bin, tik_bin_size, certBuffer, certSize, NULL, 0);
	if(ret < 0)
		goto error;
	tmd_bin = (const signed_blob*)u8_get_file(u8_bin, "tmd.bin", &tmd_bin_size);
	if(tmd_bin == NULL || tmd_bin_size == 0)
		goto error;
	ret = ES_AddTitleStart(tmd_bin, tmd_bin_size, certBuffer, certSize, NULL, 0);
	if(ret < 0)
		goto error;
	ret = ES_AddTitleFinish();
	if(ret < 0)
		goto error;
	/* WARNING dirty, delete tik again */
	memset(&ISFS_Path, 0, ISFS_MAXPATH);
	strcpy(ISFS_Path, "/ticket/00010000/57465346.tik");
	ret = ISFS_Delete(ISFS_Path);
	if(ret < 0)
		goto error;
	/* Delete the unused ticket folder */
	memset(&ISFS_Path, 0, ISFS_MAXPATH);
	strcpy(ISFS_Path, "/ticket/00010000");
	ret = ISFS_ReadDir(ISFS_Path, NULL, &entries);
	if(ret < 0)
		goto error;
	if(entries == 0)
	{
		ret = ISFS_Delete(ISFS_Path);
		if(ret < 0)
			goto error;
	}
	banner_bin = u8_get_file(u8_bin, "banner.bin", &banner_bin_size);
	if(banner_bin == NULL || banner_bin_size == 0)
		goto error;
	memset(&ISFS_Path, 0, ISFS_MAXPATH);
	strcpy(ISFS_Path, BANNER_PATH);
	/* Write our banner */
	ISFS_CreateFile(ISFS_Path, 0, 3, 3, 3);
	fd = ISFS_Open(ISFS_Path, ISFS_OPEN_WRITE);
	if(fd < 0)
		goto error;
	ret = ISFS_Write(fd, banner_bin, banner_bin_size);
	ISFS_Close(fd);
	if(ret < 0)
	{
		ISFS_Delete(ISFS_Path);
		goto error;
	}
	MEM2_free(certBuffer);
	if(u8_bin != save_bin)
		free(u8_bin);
	gprintf("Created WiiFlow save\n");
done:
	loaded = true;
	return loaded;

error:
	gprintf("Error while creating WiiFlow save\n");
	loaded = false;
	ES_AddTitleCancel();
	if(certBuffer != NULL)
		MEM2_free(certBuffer);
	certBuffer = NULL;
	if(u8_bin != NULL)
		free(u8_bin);
	u8_bin = NULL;
	tik_bin = NULL;
	tmd_bin = NULL;
	banner_bin = NULL;
	return loaded;
}

void NandSave::LoadSettings()
{
	if(loaded == false)
		return;

	u32 size = 0;
	memset(&ISFS_Path, 0, ISFS_MAXPATH);
	// on very first boot (no save file exist yet)
	// "/title/00010000/57465346/data/ios"
	// "/title/00010000/57465346/data/port"
	// will not exist. wiiflow will simply use useMainIOS and mainIOS defined in main.cpp
	// they are only created when you change settings in the startup settings menu.

	if(!useMetaArgIOS) // IOS not forced in meta.xml
	{
		strcpy(ISFS_Path, IOS_SAVE_PATH);
		ios_settings_t *file = (ios_settings_t*)ISFS_GetFile(ISFS_Path, &size, -1);
		if(file != NULL && size == sizeof(ios_settings_t))
		{
			cur_ios = file->cios;
			cur_load = file->use_cios;
			gprintf("Using IOS settings from WiiFlow save\n");
			if(cur_ios > 0)
				mainIOS = cur_ios;
			useMainIOS = cur_load;
		}
		if(file != NULL)
			MEM2_free(file);
	}

	strcpy(ISFS_Path, PORT_SAVE_PATH);
	u8 *port = ISFS_GetFile(ISFS_Path, &size, -1);
	if(port != NULL && size == sizeof(u8))
	{
		gprintf("Using port setting from WiiFlow save\n");
		currentPort = port[0] & 1;
	}
	if(port != NULL)
		MEM2_free(port);
	
	strcpy(ISFS_Path, SD_SAVE_PATH);
	u8 *sdonly = ISFS_GetFile(ISFS_Path, &size, -1);
	if(sdonly != NULL && size == sizeof(u8))
	{
		gprintf("Using SD only setting from WiiFlow save\n");
		sdOnly = ((sdonly[0] & 1) == 1);
	}
	if(sdonly != NULL)
		MEM2_free(sdonly);
}

void NandSave::SaveIOS()
{
	if(loaded == false)
		return;
	memset(&ios_settings, 0, sizeof(ios_settings_t));
	ios_settings.cios = cur_ios;
	ios_settings.use_cios = cur_load;
	gprintf("Saving IOS settings to WiiFlow save\n");
	WriteFile(IOS_SAVE_PATH, (u8*)&ios_settings, sizeof(ios_settings_t));
}

void NandSave::SavePort(u8 port)
{
	if(loaded == false)
		return;
	gprintf("Saving port setting to WiiFlow save\n");
	WriteFile(PORT_SAVE_PATH, &port, sizeof(port));
}

void NandSave::SaveSDOnly()
{
	if(loaded == false)
		return;
	gprintf("Saving SD only setting to WiiFlow save\n");
	u8 sdonly = sdOnly ? 1 : 0;
	WriteFile(SD_SAVE_PATH, &sdonly, sizeof(sdonly));
}

void NandSave::WriteFile(const char *file_name, u8 *content, u32 size)
{
	memset(&ISFS_Path, 0, ISFS_MAXPATH);
	if(file_name == NULL || content == NULL || size == 0)
		return;
	strcpy(ISFS_Path, file_name);
	ISFS_CreateFile(ISFS_Path, 0, 3, 3, 3);
	fd = ISFS_Open(ISFS_Path, ISFS_OPEN_WRITE);
	if(fd < 0)
		return;
	ret = ISFS_Write(fd, content, size);
	ISFS_Close(fd);
	if(ret < 0)
		ISFS_Delete(ISFS_Path);
}

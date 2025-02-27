#ifndef _NAND_H_
#define _NAND_H_

#include <gccore.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <string>

#include "loader/disc.h"

#define WII_NAND	0
#define EMU_SD		1
#define EMU_USB		2

#define DOWNLOADED_CHANNELS	0x00010001
#define SYSTEM_CHANNELS		0x00010002
#define RF_NEWS_CHANNEL		0x48414741
#define RF_FORECAST_CHANNEL	0x48414641

#define SYSCONFPATH 	"/shared2/sys/SYSCONF"
#define MIIPATH 		"/shared2/menu/FaceLib/RFL_DB.dat"
#define TXTPATH 		"/title/00000001/00000002/data/setting.txt"
#define SMTMDPATH 		"/title/00000001/00000002/content/title.tmd"
#define TSYSCONFPATH 	"/sys/wiiflow.reg"
#define TTXTPATH		"/tmp/setting.txt"

#define BLOCK 2048

typedef void (*dump_callback_t)(int dumpstat, int dumpprog, int filestat, int fileprog, int files, int folders, const char *tmess, void *user_data);

/* 'NAND Device' structure */
typedef struct nandDevice
{
	const char *Name;
	u32 Mode;
	u32 Mount;
	u32 Unmount;
} NandDevice; 

typedef struct _config_header 
{
	u8		magic[4];
	u16		ncnt;
	u16		noff[];
} config_header;

typedef struct _namelist
{
	char name[ISFS_MAXPATH];
	int type;
} namelist;

typedef struct _uid
{
	u64 TitleID;
	u16 padding;
	u16 uid;
} ATTRIBUTE_PACKED uid;

using std::string;
using std::min;

class Nand
{
public:
	void Init();
	bool LoadDefaultIOS(void);

	/* Prototypes */
	void SetNANDEmu(u32 partition);
	s32 Enable_Emu();
	s32 Disable_Emu();
	bool EmulationEnabled(void);

	void Set_Partition(u32 partition) { Partition = partition; };
	void Set_FullMode(bool fullmode) { FullMode = fullmode ? 0x100 : 0; }; // 00000001 00000000
	void Set_RCMode(bool rcmode) { FullMode = rcmode ? 0x40 : 0; }; // 00000000 01000000
	void Set_SSMode(bool ssmode) { FullMode = ssmode ? 0x60 : 0; }; // 00000000 01100000

	void Patch_AHB();
	void Init_ISFS();
	void DeInit_ISFS();

	const char *Get_NandPath(void) { return NandPath; };
	u32 Get_Partition(void) { return Partition; };

	u64 *GetChannels(u32 *count);
	u8 *GetTMD(u64 title, u32 *size);
	u8 *GetEmuFile(const char *path, u32 *size, s32 len = -1);
	void SetPaths(const char *emuPath, const char *currentPart);
	const char *GetPath(void) { return FullNANDPath; };

	void CreatePath(const char *path, ...);
	void CreateTitleTMD(dir_discHdr *hdr);
	s32 CreateConfig();

	s32 PreNandCfg(bool miis, bool realconfig);
	bool Do_Region_Change(string id, bool realnand);
	s32 FlashToNAND(const char *source, const char *dest, dump_callback_t i_dumper, void *i_data);
	s32 DoNandDump(const char *source, const char *dest, dump_callback_t i_dumper, void *i_data);
	s32 CalcFlashSize(const char *source, dump_callback_t i_dumper, void *i_data);
	s32 CalcDumpSpace(const char *source, dump_callback_t i_dumper, void *i_data);
	void ResetCounters(void);
	void Clear_Region_Patch(void); //

private:
	enum GameRegion 
	{
		JAP = 0,
		USA,
		EUR,
		KOR,
		ASN,
		LTN,
		UNK,
		ALL,
	};
	/* Prototypes */
	s32 Nand_Mount(NandDevice *Device);
	s32 Nand_Unmount(NandDevice *Device);
	s32 Nand_Enable(NandDevice *Device);
	s32 Nand_Disable(void);	

	void PatchAHB(void);
	bool ISFS_Check(void);

	void __Dec_Enc_TB(void);
	void __configshifttxt(char *str);
	void __GetNameList(const char *source, namelist **entries, int *count);
	u32 __GetSystemMenuRegion(void);
	s32 __configclose(void);
	s32 __configread(void);
	s32 __configwrite(bool realnand);
	u32 __configsetbyte(const char *item, u8 val);
	u32 __configsetbigarray(const char *item, void *val, u32 size);
	u32 __configsetsetting(const char *item, const char *val);
	void __NANDify(char *str);
	void __FATify(char *dst, const char *src);
	s32 __Unescaped2x(const char *path);
	s32 __FlashNandFile(const char *source, const char *dest);
	s32 __FlashNandFolder(const char *source, const char *dest);
	s32 __DumpNandFile(const char *source, const char *dest);
	s32 __DumpNandFolder(const char *source, const char *dest);
	int __makedir(char *newdir);

	u32 MountedDevice;
	u32 EmuDevice;
	u32 NandSize;
	u32 NandDone;
	u32 FileDone;
	u32 FilesDone;
	u32 FoldersDone;
	bool fake;
	bool showprogress;
	bool isfs_inited;

	void *data;
	dump_callback_t dumper;
	u32 Partition ATTRIBUTE_ALIGN(32);
	u32 FullMode ATTRIBUTE_ALIGN(32);
	char NandPath[32] ATTRIBUTE_ALIGN(32);
	char FullNANDPath[64] ATTRIBUTE_ALIGN(32);
	char cfgpath[1024];
	char settxtpath[1024];
};

extern Nand NandHandle;
#endif

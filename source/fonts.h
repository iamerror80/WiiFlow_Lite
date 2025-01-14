// Can put this in defines.h later, but its easier
// not having to recompile the whole menu for now by doing it this way

// #define WIIFONT_NAME			"WiiNTLG-Regular.ttc"
// #define WIIFONT_NAME_KOR		"Wii-kr_Round Gothic B.ttf"
const u8 WIIFONT_HASH[] =		{0x32, 0xb3, 0x39, 0xcb, 0xbb, 0x50, 0x7d, 0x50, 0x27, 0x79, 0x25, 0x9a, 0x78, 0x66, 0x99, 0x5d, 0x03, 0x0b, 0x1d, 0x88};
const u8 WIIFONT_HASH_KOR[] =	{0xb7, 0x15, 0x6d, 0xf0, 0xf4, 0xae, 0x07, 0x8f, 0xd1, 0x53, 0x58, 0x3e, 0x93, 0x6e, 0x07, 0xc0, 0x98, 0x77, 0x49, 0x0e};
const u8 WFB_HASH[] =			{0x4f, 0xad, 0x97, 0xfd, 0x4a, 0x28, 0x8c, 0x47, 0xe0, 0x58, 0x7f, 0x3b, 0xbd, 0x29, 0x23, 0x79, 0xf8, 0x70, 0x9e, 0xb9};

#define FONT_BOLD	32u
#define FONT_NOBOLD	8u

#define TITLEFONT_PT_SZ	26u
#define BTNFONT_PT_SZ	18u
#define LBLFONT_PT_SZ	18u
#define TEXTFONT_PT_SZ	18u

// args for _dfltFont(u32 fontSize, u32 lineSpacing, u32 weight, u32 index, const char *genKey)
#define TITLEFONT	TITLEFONT_PT_SZ, TITLEFONT_PT_SZ - 4, FONT_NOBOLD, 1, "title_font"
#define BUTTONFONT	BTNFONT_PT_SZ, BTNFONT_PT_SZ - 6, FONT_NOBOLD, 1, "button_font"
#define LABELFONT	LBLFONT_PT_SZ, LBLFONT_PT_SZ, FONT_NOBOLD, 1, "label_font"
#define TEXTFONT	TEXTFONT_PT_SZ, TEXTFONT_PT_SZ + 4, FONT_NOBOLD, 1, "text_font"

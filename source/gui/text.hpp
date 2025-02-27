
#ifndef __TEXT_HPP
#define __TEXT_HPP

#include <vector>
#include <string>

#include "fmt.h"
#include "FreeTypeGX.h"
#include "video.hpp"
#include "wstringEx/wstringEx.hpp"

using std::string;
using std::vector;

class SFont // should be either struct SFont or class CFont
{
public:
	SFont(void) : font(NULL), fSize(0), lineSpacing(0), weight(0), index(0), data(NULL), dataSize(0) { memset(name, 0, 128); };
	~SFont(void) { };
	void ClearData(void);
	bool fromBuffer(const u8 *buffer, const unsigned int bufferSize, unsigned int size, unsigned int lspacing, unsigned int w = 0, unsigned int idx = 0, const char *fontname = "");
	bool fromFile(const char *path, unsigned int size, unsigned int lspacing, unsigned int w = 0, unsigned int idx = 0, const char *fontname = "");
	FreeTypeGX *font;
	u32 fSize;
	u32 lineSpacing;
	u32 weight;
	u32 index;
	char name[128];
private:
	u8 *data;
	u32 dataSize;
};

struct SWord
{
	wstringEx text;
	Vector3D pos;
	Vector3D targetPos;
};
typedef vector<SWord> CLine;

class CText
{
public:
	void setText(const SFont &font, const wstringEx &t);
	void setText(const SFont &font, const wstringEx &t, u32 startline);
	void setColor(const CColor &c);
	void setFrame(float width, u16 style, bool ignoreNewlines = false, bool instant = false);
	void tick(void);
	void draw(void);
	int getTotalHeight();
private:
	vector<CLine> m_lines;
	SFont m_font;
	CColor m_color;
	u32 firstLine;
	u32 totalHeight;
};

// Nothing to do with CText. Q&D helpers for string formating.

string sfmt(const char *format, ...);
wstringEx wfmt(const wstringEx &format, ...);
bool checkFmt(const wstringEx &ref, const wstringEx &format);
string vectorToString(const vector<std::string> &vect, std::string sep);
wstringEx vectorToString(const vector<wstringEx> &vect, char sep);
vector<wstringEx> stringToVector(const wstringEx &text, char sep);
vector<std::string> stringToVector(const string &text, char sep);
string upperCase(string text);
string capitalize(string text);
string lowerCase(string text);
// string ltrim(string s);
// string rtrim(string s);
string removeSpaceDash(string s);
bool wchar_cmp(const wchar_t *first, const wchar_t *second, u32 first_len, u32 second_len);
bool char_cmp(const char *first, const char *second, u32 first_len, u32 second_len);

#endif // !defined(__TEXT_HPP)

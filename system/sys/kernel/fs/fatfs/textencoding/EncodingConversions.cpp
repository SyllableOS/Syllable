/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "UnicodeMappings.h"
#include "EncodingConversions.h"

#include <atheos/types.h>
#include <posix/errno.h>

extern "C" {
#include "../util.h"
}
//#include <UTF8.h>


#define B_NO_ERROR 0

// Pierre's Uber Macro
#define u_to_utf8(str, uni_str)\
{\
	if ((uni_str[0]&0xff80) == 0)\
		*str++ = *uni_str++;\
	else if ((uni_str[0]&0xf800) == 0) {\
		str[0] = 0xc0|(uni_str[0]>>6);\
		str[1] = 0x80|((*uni_str++)&0x3f);\
		str += 2;\
	} else if ((uni_str[0]&0xfc00) != 0xd800) {\
		str[0] = 0xe0|(uni_str[0]>>12);\
		str[1] = 0x80|((uni_str[0]>>6)&0x3f);\
		str[2] = 0x80|((*uni_str++)&0x3f);\
		str += 3;\
	} else {\
		int   val;\
		val = ((uni_str[0]-0xd7c0)<<10) | (uni_str[1]&0x3ff);\
		str[0] = 0xf0 | (val>>18);\
		str[1] = 0x80 | ((val>>12)&0x3f);\
		str[2] = 0x80 | ((val>>6)&0x3f);\
		str[3] = 0x80 | (val&0x3f);\
		uni_str += 2; str += 4;\
	}\
}	

// Another Uber Macro
#define utf8_to_u(str, uni_str) \
{\
	if ((str[0]&0x80) == 0)\
		*uni_str++ = *str++;\
	else if ((str[1] & 0xC0) != 0x80) {\
        *uni_str++ = 0xfffd;\
		str+=1;\
	} else if ((str[0]&0x20) == 0) {\
		*uni_str++ = ((str[0]&31)<<6) | (str[1]&63);\
		str+=2;\
	} else if ((str[2] & 0xC0) != 0x80) {\
        *uni_str++ = 0xfffd;\
		str+=2;\
	} else if ((str[0]&0x10) == 0) {\
		*uni_str++ = ((str[0]&15)<<12) | ((str[1]&63)<<6) | (str[2]&63);\
		str+=3;\
	} else if ((str[3] & 0xC0) != 0x80) {\
        *uni_str++ = 0xfffd;\
		str+=3;\
	} else {\
		int   val;\
		val = ((str[0]&7)<<18) | ((str[1]&63)<<12) | ((str[2]&63)<<6) | (str[3]&63);\
		uni_str[0] = 0xd7c0+(val>>10);\
		uni_str[1] = 0xdc00+(val&0x3ff);\
		uni_str += 2;\
		str += 4;\
	}\
}


// Return whether a unicode char is hankaku katakana
inline bool
is_hankata(uint16 u)
{
	return ((u > 0xFF60) && (u < 0xFFA0));
}

// Count the number of bytes of a UTF-8 character
inline int32
utf8_char_len(uchar c)
{
	return (((0xE5000000 >> ((c >> 3) & 0x1E)) & 3) + 1); 
}

// Shift-JIS, gotta love it
inline bool
is_initial_sjis_byte(uchar c)
{	
	return ( ((c >= 0x81) && (c <= 0x9F)) || 
			 ((c >= 0xE0) && (c <= 0xEF)) ); 
}

// SJIS han-kata
inline bool
is_hankata_sjis_byte(uchar c)
{
	return ((c >= 0xA1) && (c <= 0xDF));
}

// EUC
inline bool
is_initial_euc_set1_byte(uchar c)
{
	return ((c >= 0xA1) && (c <= 0xFE));
}

// More EUC
inline bool
is_initial_euc_set2_byte(uchar c)
{
	return (c == 0x8E);
}

// Parsing for JIS escape sequences
inline int32
skip_jis_escape(
	char	theChar,
	bool	*multibyte)
{
	// skip JIS escape sequences
	int32 offset = 0;

	if ((theChar == '$') || (theChar == '('))
		offset = 3;

	if ((theChar == 'K') || (theChar == '$'))
		*multibyte = true;
	else
		*multibyte = false;

	return (offset);
} 

// SJIS to Unicode conversion done with 3 mapping tables
inline uint16
sjis_to_u(uint16 s)
{
	if (/* (s >= 0x0000) && */ (s <= 0x00FF))
		return (sjis00tou[s - 0x0000]);
	else if ((s >= 0xE000) && (s <= 0xF000))
		return (sjise0tou[s - 0xE000]);
	else if ((s >= 0x8100) && (s <= 0xA000))
		return (sjis81tou[s - 0x8100]);

	return (s);
}


// Sneaky struct for interating thru multiple tables (SJIS)
struct table_segment {
	const uint16	*table;
	uint16			offset;
};


const int32		kInKanjiMode = 'kanj';
const uint16	kHankataStart = 0xFF61;
const uint16	kHankataToZenkataTable[] = {
0x3002, 0x300C, 0x300D, 0x3001, 0x30FB, 0x30F2, 0x30A1, 0x30A3,
0x30A5, 0x30A7, 0x30A9, 0x30E3, 0x30E5, 0x30E7, 0x30C3, 0x30FC,
0x30A2, 0x30A4, 0x30A6, 0x30A8, 0x30AA, 0x30AB, 0x30AD, 0x30AF,
0x30B1, 0x30B3, 0x30B5, 0x30B7, 0x30B9, 0x30BB, 0x30BD, 0x30BF,
0x30C1, 0x30C4, 0x30C6, 0x30C8, 0x30CA, 0x30CB, 0x30CC, 0x30CD,
0x30CE, 0x30CF, 0x30D2, 0x30D5, 0x30D8, 0x30DB, 0x30DE, 0x30DF,
0x30E0, 0x30E1, 0x30E2, 0x30E4, 0x30E6, 0x30E8, 0x30E9, 0x30EA,
0x30EB, 0x30EC, 0x30ED, 0x30EF, 0x30F3, 0x309B, 0x309C 
};


// Single-byte encoding to UTF-8 (generic) 
static status_t one_to_utf8(const uint16	*table, 
					 		const char		*src, 
					 		int32			*srcLen,
					 		char			*dst,
					 		int32			*dstLen,
					 		char			substitute);

// UTF-8 to single-byte encoding (generic)
static status_t utf8_to_one(const uint16	*table,
					 		const char		*src,
						 	int32			*srcLen,
					 		char			*dst,
					 		int32			*dstLen,
					 		char			substitute);

// SJIS to UTF-8
static status_t sjis_to_utf8(const char	*src,
						  	 int32		*srcLen,
					  		 char		*dst,
					  		 int32		*dstLen,
					  		 char		substitute);

// UTF-8 to SJIS
static status_t utf8_to_sjis(const char	*src,
					  		 int32		*srcLen,
					  		 char		*dst,
					  		 int32		*dstLen,
							 char		substitute);

// EUC (pronounced 'yuck') to UTF-8
static status_t euc_to_utf8(const char	*src,
					 		int32		*srcLen,
							char		*dst,
					 		int32		*dstLen,
					 		char		substitute);

// UTF-8 to EUC
static status_t utf8_to_euc(const char	*src,
					 		int32		*srcLen,
					 		char		*dst,
					 		int32		*dstLen,
					 		char		substitute);

// JIS to UTF-8
static status_t jis_to_utf8(const char	*src,
					 		int32		*srcLen,
					 		char		*dst,
					 		int32		*dstLen,
					 		int32		*state,
					 		char		substitute);

// UTF-8 to JIS
static status_t utf8_to_jis(const char	*src,
					 		int32		*srcLen,
					 		char		*dst,
					 		int32		*dstLen,
					 		int32		*state,
					 		char		substitute);

// Unicode to UTF-8
static status_t unicode_to_utf8(const char	*src,
						 		int32		*srcLen,
						 		char		*dst,
						 		int32		*dstLen,
						 		char		substitute);

// UTF-8 to Unicode
static status_t utf8_to_unicode(const char	*src,
						 		int32		*srcLen,
						 		char		*dst,
						 		int32		*dstLen,
						 		char		substitute);

// EUC-KR to UTF-8
static status_t euc_kr_to_utf8(const char	*src, 
							   int32		*srcLen, 
							   char			*dst,
							   int32		*dstLen,
							   char			substitute);

// UTF-8 to EUC-KR
static status_t utf8_to_euc_kr(const char	*src,
							   int32		*srcLen,
							   char			*dst,
							   int32		*dstLen,
							   char			substitute);


// Public function
status_t convert_to_utf8( uint32 srcEncoding, const char *src, int32 *srcLen, char *dst, int32 *dstLen, int32 *state, char substitute )
{
	if ((src == NULL) || (dst == NULL))
		return (-EINVAL);

	switch (srcEncoding) {
		case B_ISO1_CONVERSION:
			return (one_to_utf8(iso1tou, src, srcLen, dst, dstLen, substitute));
			
		case B_ISO2_CONVERSION:
			return (one_to_utf8(iso2tou, src, srcLen, dst, dstLen, substitute));

		case B_ISO3_CONVERSION:
			return (one_to_utf8(iso3tou, src, srcLen, dst, dstLen, substitute));

		case B_ISO4_CONVERSION:
			return (one_to_utf8(iso4tou, src, srcLen, dst, dstLen, substitute));

		case B_ISO5_CONVERSION:
			return (one_to_utf8(iso5tou, src, srcLen, dst, dstLen, substitute));

		case B_ISO6_CONVERSION:
			return (one_to_utf8(iso6tou, src, srcLen, dst, dstLen, substitute));

		case B_ISO7_CONVERSION:
			return (one_to_utf8(iso7tou, src, srcLen, dst, dstLen, substitute));

		case B_ISO8_CONVERSION:
			return (one_to_utf8(iso8tou, src, srcLen, dst, dstLen, substitute));

		case B_ISO9_CONVERSION:
			return (one_to_utf8(iso9tou, src, srcLen, dst, dstLen, substitute));

		case B_ISO10_CONVERSION:
			return (one_to_utf8(iso10tou, src, srcLen, dst, dstLen, substitute));

		case B_MAC_ROMAN_CONVERSION:
			return (one_to_utf8(macromantou, src, srcLen, dst, dstLen, substitute));

		case B_SJIS_CONVERSION:
			return (sjis_to_utf8(src, srcLen, dst, dstLen, substitute));

		case B_EUC_CONVERSION:
			return (euc_to_utf8(src, srcLen, dst, dstLen, substitute));

		case B_JIS_CONVERSION:
			return (jis_to_utf8(src, srcLen, dst, dstLen, state, substitute));

		case B_MS_WINDOWS_CONVERSION:
			return (one_to_utf8(mswintou, src, srcLen, dst, dstLen, substitute));

		case B_UNICODE_CONVERSION:
			return (unicode_to_utf8(src, srcLen, dst, dstLen, substitute));

		case B_KOI8R_CONVERSION:
			return (one_to_utf8(koi8rtou, src, srcLen, dst, dstLen, substitute));

		case B_MS_WINDOWS_1251_CONVERSION:
			return (one_to_utf8(mswin1251tou, src, srcLen, dst, dstLen, substitute));

		case B_MS_DOS_866_CONVERSION:
			return (one_to_utf8(msdos866tou, src, srcLen, dst, dstLen, substitute));

		case B_MS_DOS_CONVERSION:
			return (one_to_utf8(msdostou, src, srcLen, dst, dstLen, substitute));

		case B_EUC_KR_CONVERSION:
			return (euc_kr_to_utf8(src, srcLen, dst, dstLen, substitute));
	}

	// default case comes here
	return (-EINVAL);
}


// Public function
status_t convert_from_utf8( uint32 dstEncoding, const char *src, int32 *srcLen, char *dst, int32 *dstLen, int32 *state, char substitute )
{
	if ((src == NULL) || (dst == NULL))
		return (-EINVAL);

	switch (dstEncoding) {
		case B_ISO1_CONVERSION:
			return (utf8_to_one(iso1tou, src, srcLen, dst, dstLen, substitute));
			
		case B_ISO2_CONVERSION:
			return (utf8_to_one(iso2tou, src, srcLen, dst, dstLen, substitute));

		case B_ISO3_CONVERSION:
			return (utf8_to_one(iso3tou, src, srcLen, dst, dstLen, substitute));

		case B_ISO4_CONVERSION:
			return (utf8_to_one(iso4tou, src, srcLen, dst, dstLen, substitute));

		case B_ISO5_CONVERSION:
			return (utf8_to_one(iso5tou, src, srcLen, dst, dstLen, substitute));

		case B_ISO6_CONVERSION:
			return (utf8_to_one(iso6tou, src, srcLen, dst, dstLen, substitute));

		case B_ISO7_CONVERSION:
			return (utf8_to_one(iso7tou, src, srcLen, dst, dstLen, substitute));

		case B_ISO8_CONVERSION:
			return (utf8_to_one(iso8tou, src, srcLen, dst, dstLen, substitute));

		case B_ISO9_CONVERSION:
			return (utf8_to_one(iso9tou, src, srcLen, dst, dstLen, substitute));

		case B_ISO10_CONVERSION:
			return (utf8_to_one(iso10tou, src, srcLen, dst, dstLen, substitute));

		case B_MAC_ROMAN_CONVERSION:
			return (utf8_to_one(macromantou, src, srcLen, dst, dstLen, substitute));

		case B_SJIS_CONVERSION:
			return (utf8_to_sjis(src, srcLen, dst, dstLen, substitute));

		case B_EUC_CONVERSION:
			return (utf8_to_euc(src, srcLen, dst, dstLen, substitute));

		case B_JIS_CONVERSION:
			return (utf8_to_jis(src, srcLen, dst, dstLen, state, substitute));

		case B_MS_WINDOWS_CONVERSION:
			return (utf8_to_one(mswintou, src, srcLen, dst, dstLen, substitute));

		case B_UNICODE_CONVERSION:
			return (utf8_to_unicode(src, srcLen, dst, dstLen, substitute));

		case B_KOI8R_CONVERSION:
			return (utf8_to_one(koi8rtou, src, srcLen, dst, dstLen, substitute));
		
		case B_MS_WINDOWS_1251_CONVERSION:
			return (utf8_to_one(mswin1251tou, src, srcLen, dst, dstLen, substitute));

		case B_MS_DOS_866_CONVERSION:
			return (utf8_to_one(msdos866tou, src, srcLen, dst, dstLen, substitute));

		case B_MS_DOS_CONVERSION:
			return (utf8_to_one(msdostou, src, srcLen, dst, dstLen, substitute));

		case B_EUC_KR_CONVERSION:
			return (utf8_to_euc_kr(src, srcLen, dst, dstLen, substitute));
	}

	// default case comes here
	return (-EINVAL);	
}


status_t
one_to_utf8(
	const uint16	*table,
	const char		*src,
	int32			*srcLen,
	char			*dst,
	int32			*dstLen,
	char)
{
	int32 srcLimit = *srcLen;
	int32 dstLimit = *dstLen;
	int32 srcCount = 0;
	int32 dstCount = 0;

	for (srcCount = 0; srcCount < srcLimit; srcCount++) {
		uint16	unicode = table[(uchar)src[srcCount]];
		uint16	*UNICODE = &unicode;
		uchar	utf8[4];
		uchar	*UTF8 = utf8;

		u_to_utf8(UTF8, UNICODE);

		int32 utf8Len = UTF8 - utf8;
		if ((dstCount + utf8Len) > dstLimit)
			break;

		for (int32 j = 0; j < utf8Len; j++)
			dst[dstCount + j] = utf8[j];
		dstCount += utf8Len;
	}

	*srcLen = srcCount;
	*dstLen = dstCount;

	return ((dstCount > 0) ? B_NO_ERROR : -EINVAL);
}


status_t
utf8_to_one(
	const uint16	*table,
	const char		*src,
	int32			*srcLen,
	char			*dst,
	int32			*dstLen,
	char			substitute)
{
	int32 srcLimit = *srcLen;
	int32 dstLimit = *dstLen;
	int32 srcCount = 0;
	int32 dstCount = 0;

	while ((srcCount < srcLimit) && (dstCount < dstLimit)) {
		if ((srcCount + utf8_char_len(src[srcCount])) > srcLimit)
			break; 

		uint16	unicode;
		uint16	*UNICODE = &unicode;
		uchar	*UTF8 = (uchar *)src + srcCount;

		utf8_to_u(UTF8, UNICODE);

		dst[dstCount] = substitute;		
		for (int32 i = 0; table[i] != 0xFFFF; i++) {
			if (unicode == table[i]) {
				dst[dstCount] = i;
				break;
			}
		}

		srcCount += UTF8 - ((uchar *)(src + srcCount));
		dstCount++;
	}

	*srcLen = srcCount;
	*dstLen = dstCount;

	return ((dstCount > 0) ? B_NO_ERROR : -EINVAL);
}


status_t
sjis_to_utf8(
	const char	*src,
	int32		*srcLen,
	char		*dst,
	int32		*dstLen,
	char)
{
	int32 srcLimit = *srcLen;
	int32 dstLimit = *dstLen;
	int32 srcCount = 0;
	int32 dstCount = 0;

	while (srcCount < srcLimit) {
		bool	multibyte = false;
		uchar	c = (uchar)src[srcCount];
		uint16	sjis = c;
		
		if (is_initial_sjis_byte(c)) {
			if ((srcCount + 1) < srcLimit) {
				sjis = (c << 8) | ((uchar)src[srcCount + 1]);
				multibyte = true;
			}
			else
				break;
		}		

		uint16	unicode = sjis_to_u(sjis);
		uint16	*UNICODE = &unicode;
		uchar	utf8[4];
		uchar	*UTF8 = utf8;

		u_to_utf8(UTF8, UNICODE);

		int32 utf8Len = UTF8 - utf8;
		if ((dstCount + utf8Len) > dstLimit)
			break;

		for (int32 j = 0; j < utf8Len; j++)
			dst[dstCount + j] = utf8[j];
		dstCount += utf8Len;

		srcCount += (multibyte) ? 2 : 1;
	}

	*srcLen = srcCount;
	*dstLen = dstCount;

	return ((dstCount > 0) ? B_NO_ERROR : -EINVAL);
}


status_t
utf8_to_sjis(
	const char	*src,
	int32		*srcLen,
	char		*dst,
	int32		*dstLen,
	char		substitute)
{
	int32 srcLimit = *srcLen;
	int32 dstLimit = *dstLen;
	int32 srcCount = 0;
	int32 dstCount = 0;

	while ((srcCount < srcLimit) && (dstCount < dstLimit)) {
		if ((srcCount + utf8_char_len(src[srcCount])) > srcLimit)
			break; 

		uint16	unicode;
		uint16	*UNICODE = &unicode;
		uchar	*UTF8 = (uchar *)src + srcCount;

		utf8_to_u(UTF8, UNICODE);

		bool				foundChar = false;
		bool				multibyte = false;
		const uint16		*table = NULL;	
		const table_segment	sjistables[] = { {sjis00tou, 0x0000},
											 {sjis81tou, 0x8100},
											 {sjise0tou, 0xE000},
											 {NULL, 0x0000} };

		for (int32 t = 0; (table = sjistables[t].table) != NULL; t++) {
			for (int32 i = 0; table[i] != 0xFFFF; i++) {
				if (unicode == table[i]) {
					uint16 offset = sjistables[t].offset;

					if (offset == 0x0000)
						dst[dstCount] = i; 
					else {
						if ((dstCount + 1) < dstLimit) {
							uint16 sjis = offset + i;
							dst[dstCount] = (sjis >> 8) & 0xFF;
							dst[dstCount + 1] = sjis & 0xFF;
							multibyte = true;
						}
						else
							goto Exit;	// this is terrible!
					}

					foundChar = true;
					break;
				}
			}

			if (foundChar)
				break;
		}

		if (!foundChar)
			dst[dstCount] = substitute;

		srcCount += UTF8 - ((uchar *)(src + srcCount));
		dstCount += (multibyte) ? 2 : 1;
	}

Exit:
	*srcLen = srcCount;
	*dstLen = dstCount;

	return ((dstCount > 0) ? B_NO_ERROR : -EINVAL);
}


status_t 
euc_to_utf8(
	const char	*src,
	int32		*srcLen,
	char		*dst,
	int32		*dstLen,
	char)
{
	int32 srcLimit = *srcLen;
	int32 dstLimit = *dstLen;
	int32 srcCount = 0;
	int32 dstCount = 0;

	while (srcCount < srcLimit) {
		bool	multibyte = false;
		uchar	c = (uchar)src[srcCount];
		uint16	sjis = c;
		
		if (is_initial_euc_set1_byte(c)) {
			if ((srcCount + 1) < srcLimit) {
				uchar c1 = c - 128;
				uchar c2 = ((uchar)src[srcCount + 1]) - 128;
				uchar rowOffset = (c1 < 95) ? 112 : 176;
				uchar cellOffset = (c1 % 2) ? ((c2 > 95) ? 32 : 31) : 126;
				c1 = ((c1 + 1) >> 1) + rowOffset;
				c2 += cellOffset;
				sjis = (c1 << 8) | (c2);
				multibyte = true;
			}
			else
				break;
		}		
		else {
			if (is_initial_euc_set2_byte(c)) {
				if ((srcCount + 1) < srcLimit) {
					sjis = (uchar)src[srcCount + 1];
					multibyte = true;
				}
				else
					break;
			}
		}

		uint16	unicode = sjis_to_u(sjis);
		uint16	*UNICODE = &unicode;
		uchar	utf8[4];
		uchar	*UTF8 = utf8;

		u_to_utf8(UTF8, UNICODE);

		int32 utf8Len = UTF8 - utf8;
		if ((dstCount + utf8Len) > dstLimit)
			break;

		for (int32 j = 0; j < utf8Len; j++)
			dst[dstCount + j] = utf8[j];
		dstCount += utf8Len;

		srcCount += (multibyte) ? 2 : 1;
	}

	*srcLen = srcCount;
	*dstLen = dstCount;

	return ((dstCount > 0) ? B_NO_ERROR : -EINVAL);
}


status_t
utf8_to_euc(
	const char	*src,
	int32		*srcLen,
	char		*dst,
	int32		*dstLen,
	char		substitute)
{
	int32 srcLimit = *srcLen;
	int32 dstLimit = *dstLen;
	int32 srcCount = 0;
	int32 dstCount = 0;

	while ((srcCount < srcLimit) && (dstCount < dstLimit)) {
		if ((srcCount + utf8_char_len(src[srcCount])) > srcLimit)
			break; 

		uint16	unicode;
		uint16	*UNICODE = &unicode;
		uchar	*UTF8 = (uchar *)src + srcCount;

		utf8_to_u(UTF8, UNICODE);

		bool				foundChar = false;
		bool				multibyte = false;
		const uint16		*table = NULL;	
		const table_segment	sjistables[] = { {sjis00tou, 0x0000},
											 {sjis81tou, 0x8100},
											 {sjise0tou, 0xE000},
											 {NULL, 0x0000} };

		for (int32 t = 0; (table = sjistables[t].table) != NULL; t++) {
			for (int32 i = 0; table[i] != 0xFFFF; i++) {
				if (unicode == table[i]) {
					uint16 offset = sjistables[t].offset;

					if (offset == 0x0000) {
						if (!is_hankata_sjis_byte(i)) 
							dst[dstCount] = i; 
						else {
							if ((dstCount + 1) < dstLimit) {
								dst[dstCount] = 142;
								dst[dstCount + 1] = i;
								multibyte = true;
							}
							else
								goto Exit;	// this is terrible!
						}
					}
					else {
						if ((dstCount + 1) < dstLimit) {
							uint16	sjis = offset + i;
							uchar	c1 = (sjis >> 8) & 0xFF;
							uchar	c2 = sjis & 0xFF;
							uchar	adjust = (c2 < 159) ? 1 : 0;
							uchar	rowOffset = (c1 < 160) ? 112 : 176;
							uchar	cellOffset = (adjust != 0) ? 
												 ((c2 > 127) ? 32 : 31) : 126;
							dst[dstCount] = ((c1 - rowOffset) << 1) - 
											adjust + 128;
							dst[dstCount + 1] = (c2 - cellOffset) + 128;
							multibyte = true;
						}
						else
							goto Exit;	// this is terrible!
					}

					foundChar = true;
					break;
				}
			}

			if (foundChar)
				break;
		}

		if (!foundChar)
			dst[dstCount] = substitute;

		srcCount += UTF8 - ((uchar *)(src + srcCount));
		dstCount += (multibyte) ? 2 : 1;
	}

Exit:
	*srcLen = srcCount;
	*dstLen = dstCount;

	return ((dstCount > 0) ? B_NO_ERROR : -EINVAL);
}


status_t 
jis_to_utf8(
	const char	*src,
	int32		*srcLen,
	char		*dst,
	int32		*dstLen,
	int32		*state,
	char)
{
	int32	srcLimit = *srcLen;
	int32	dstLimit = *dstLen;
	int32	srcCount = 0;
	int32	dstCount = 0;
	bool	multibyte = *state == kInKanjiMode;

	while (srcCount < srcLimit) {
		uchar	c = (uchar)src[srcCount];
		uint16	sjis = c;
		
		if ((c == 0x1B) && ((srcCount + 1) < srcLimit)) {
			int32 escapeOffset = skip_jis_escape((uchar)src[srcCount + 1], &multibyte);

			if (escapeOffset != 0) {
				if ((srcCount + escapeOffset) < srcLimit) {
					srcCount += escapeOffset;
					continue;
				}
				else
					break;
			}
		}

		if (multibyte) {
			if ((srcCount + 1) < srcLimit) {
				uchar c1 = c;
				uchar c2 = (uchar)src[srcCount + 1];
				uchar rowOffset = (c1 < 95) ? 112 : 176;
				uchar cellOffset = (c1 % 2) ? ((c2 > 95) ? 32 : 31) : 126;

				sjis = ((((c1 + 1) >> 1) + rowOffset) << 8) | (c2 + cellOffset);
			}
			else
				break;
		}		

		uint16	unicode = sjis_to_u(sjis);
		uint16	*UNICODE = &unicode;
		uchar	utf8[4];
		uchar	*UTF8 = utf8;

		u_to_utf8(UTF8, UNICODE);

		int32 utf8Len = UTF8 - utf8;
		if ((dstCount + utf8Len) > dstLimit)
			break;

		for (int32 j = 0; j < utf8Len; j++)
			dst[dstCount + j] = utf8[j];
		dstCount += utf8Len;

		srcCount += (multibyte) ? 2 : 1;
	}

	*srcLen = srcCount;
	*dstLen = dstCount;
	*state = (multibyte) ? kInKanjiMode : 0;

	return ((dstCount > 0) ? B_NO_ERROR : -EINVAL);	
}


status_t 
utf8_to_jis(
	const char	*src,
	int32		*srcLen,
	char		*dst,
	int32		*dstLen,
	int32		*state,
	char		substitute)
{
	int32	srcLimit = *srcLen;
	int32	dstLimit = *dstLen;
	int32	srcCount = 0;
	int32	dstCount = 0;
	bool	inKanjiMode = *state == kInKanjiMode;

	while ((srcCount < srcLimit) && (dstCount < dstLimit)) {
		if ((srcCount + utf8_char_len(src[srcCount])) > srcLimit)
			break; 

		uint16	unicode;
		uint16	*UNICODE = &unicode;
		uchar	*UTF8 = (uchar *)src + srcCount;

		utf8_to_u(UTF8, UNICODE);

		if (is_hankata(unicode))
			unicode = kHankataToZenkataTable[unicode - kHankataStart];

		bool				foundChar = false;
		bool				multibyte = false;
		const uint16		*table = NULL;	
		const table_segment	sjistables[] = { {sjis00tou, 0x0000},
											 {sjis81tou, 0x8100},
											 {sjise0tou, 0xE000},
											 {NULL, 0x0000} };

		for (int32 t = 0; (table = sjistables[t].table) != NULL; t++) {
			for (int32 i = 0; table[i] != 0xFFFF; i++) {
				if (unicode == table[i]) {
					uint16 offset = sjistables[t].offset;

					if (offset == 0x0000) {
						if (is_hankata_sjis_byte(i)) 
							// huh!?  shouldn't get here!!
							goto Exit;	// this is terrible!
						else {
							if (inKanjiMode) {
								if ((dstCount + 3) < dstLimit) {
									dst[dstCount++] = 0x1B;
									dst[dstCount++] = '(';
									dst[dstCount++] = 'J';
									inKanjiMode = false;
								} 
								else
									goto Exit;	// bah!
							}

							dst[dstCount] = i; 
						}
					}
					else {
						if ((dstCount + 1) < dstLimit) {
							if (!inKanjiMode) {
								if ((dstCount + 4) < dstLimit) {
									dst[dstCount++] = 0x1B;
									dst[dstCount++] = '$';
									dst[dstCount++] = 'B';
									inKanjiMode = true;
								}
								else
									goto Exit;	// bah!
							}

							uint16	sjis = offset + i;
							uchar	c1 = (sjis >> 8) & 0xFF;
							uchar	c2 = sjis & 0xFF;
							uchar	adjust = c2 < 159;
							uchar	rowOffset = (c1 < 160) ? 112 : 176;
							uchar	cellOffset = (adjust) ? (c2 > 127) ? 32 : 31 : 126;
							dst[dstCount] = ((c1 - rowOffset) << 1) - adjust;
							dst[dstCount + 1] = c2 - cellOffset;
							multibyte = true;
						}
						else
							goto Exit;	// this is terrible!
					}

					foundChar = true;
					break;
				}
			}

			if (foundChar)
				break;
		}

		if (!foundChar)
			dst[dstCount] = substitute;

		srcCount += UTF8 - ((uchar *)(src + srcCount));
		dstCount += (multibyte) ? 2 : 1;
	}

Exit:
	*srcLen = srcCount;
	*dstLen = dstCount;
	*state = (inKanjiMode) ? kInKanjiMode : 0;

	return ((dstCount > 0) ? B_NO_ERROR : -EINVAL);
}


status_t
unicode_to_utf8(
	const char	*src,
	int32		*srcLen,
	char		*dst,
	int32		*dstLen,
	char)
{
	int32 srcLimit = *srcLen;
	int32 dstLimit = *dstLen;
	int32 srcCount = 0;
	int32 dstCount = 0;

	for (srcCount = 0; srcCount < srcLimit; srcCount += 2) {
		uint16	unicode = ((uchar)src[srcCount] << 8) | ((uchar)src[srcCount + 1] & 0xFF);
		uint16	*UNICODE = &unicode;
		uchar	utf8[4];
		uchar	*UTF8 = utf8;

		u_to_utf8(UTF8, UNICODE);

		int32 utf8Len = UTF8 - utf8;
		if ((dstCount + utf8Len) > dstLimit)
			break;

		for (int32 j = 0; j < utf8Len; j++)
			dst[dstCount + j] = utf8[j];
		dstCount += utf8Len;
	}

	*srcLen = srcCount;
	*dstLen = dstCount;

	return ((dstCount > 0) ? B_NO_ERROR : -EINVAL);
}


status_t 
utf8_to_unicode(
	const char	*src,
	int32		*srcLen,
	char		*dst,
	int32		*dstLen,
	char)
{
	int32 srcLimit = *srcLen;
	int32 dstLimit = *dstLen - 1;
	int32 srcCount = 0;
	int32 dstCount = 0;

	while ((srcCount < srcLimit) && (dstCount < dstLimit)) {
		if ((srcCount + utf8_char_len(src[srcCount])) > srcLimit)
			break; 

		uint16	unicode;
		uint16	*UNICODE = &unicode;
		uchar	*UTF8 = (uchar *)src + srcCount;

		utf8_to_u(UTF8, UNICODE);

		dst[dstCount++] = unicode >> 8;		
		dst[dstCount++] = unicode & 0xFF;

		srcCount += UTF8 - ((uchar *)(src + srcCount));
	}

	*srcLen = srcCount;
	*dstLen = dstCount;

	return ((dstCount > 0) ? B_NO_ERROR : -EINVAL);
}


status_t 
euc_kr_to_utf8(
	const char	*src,
	int32		*srcLen,
	char		*dst,
	int32		*dstLen,
	char		substitute)
{
	int32 srcLimit = *srcLen;
	int32 dstLimit = *dstLen;
	int32 srcCount = 0;
	int32 dstCount = 0;

	while (srcCount < srcLimit) {
		if ((uchar)src[srcCount] < 0x80) 
			dst[dstCount++] = src[srcCount++];
		else {
			if ((srcCount + 1) >= srcLimit)
				break;

			uint16 ks = (((uchar)src[srcCount]) << 8) | ((uchar)src[srcCount + 1]);
			uint16 uni = substitute;

			if ((ks >= 0xA1A1) && (ks <= 0xACFE))
				uni = EUCKR_SPECIAL[(((ks & 0xFF00) >> 8) - 0xA1) * 94 + (ks & 0x00FF) - 0xA1];
			else if ((ks >= 0xB0A1) && (ks <= 0xC8FE))
				uni = EUCKR_HANGUL[(((ks & 0xFF00) >> 8) - 0xB0) * 94 + (ks & 0x00FF) - 0xA1];
			else if ((ks >= 0xCAA1) && (ks <= 0xFDFE))
				uni = EUCKR_HANJA[(((ks & 0xFF00) >> 8) - 0xCA) * 94 + (ks & 0x00FF) - 0xA1];
			else if (ks < 0x80)	
				uni = ks;

			uint16	*UNICODE = &uni;
			uchar	utf8[4];
			uchar	*UTF8 = utf8;

			u_to_utf8(UTF8, UNICODE);

			int32 utf8Len = UTF8 - utf8;
			
			if ((dstCount + utf8Len) > dstLimit)
				break;

			for (int32 j = 0; j < utf8Len; j++)
				dst[dstCount + j] = utf8[j];

			dstCount += utf8Len;
			srcCount += 2;
		} 
	}

	*srcLen = srcCount;
	*dstLen = dstCount;	

	return ((dstCount > 0) ? B_NO_ERROR : -EINVAL);
}


status_t
utf8_to_euc_kr(
	const char	*src,
	int32		*srcLen,
	char		*dst,
	int32		*dstLen,
	char		substitute)
{
	int32 srcLimit = *srcLen;
	int32 dstLimit = *dstLen;
	int32 srcCount = 0;
	int32 dstCount = 0;

	while ((srcCount < srcLimit) && (dstCount < dstLimit)) {
		if ((srcCount + utf8_char_len(src[srcCount])) > srcLimit)
			break; 

		uint16	uni;
		uint16	*UNICODE = &uni;
		uchar	*UTF8 = (uchar *)src + srcCount;

		utf8_to_u(UTF8, UNICODE);

		uint16	ks = substitute;
		int32	ksAddLen = 0;

		if (uni < 0x80) {
			ks = uni;
			goto Converted;
		}

		for (int32 i = 0; i < 1128; i++) {
			if (uni == EUCKR_SPECIAL[i]) {
				ks = 0xA1A1 + (0x0100 * (int)(i / 94)) + i % 94;
				ksAddLen = 1;
				goto Converted;
			}
		}

		for(int32 i = 0; i < 2350; i++) {
			if(uni == EUCKR_HANGUL[i]) {
				ks = 0xB0A1 + (0x0100 * (int)(i / 94)) + i % 94;
				ksAddLen = 1;
				goto Converted;
			}
		}

		for (int32 i = 0; i < 4888; i++) {
			if (uni == EUCKR_HANJA[i]) {
				ks = 0xCAA1 + (0x0100 * (int)(i / 94)) + i % 94;
				ksAddLen = 1;
				goto Converted;
			}
		}

Converted:
		if ((dstCount + ksAddLen) < dstLimit) {
			if (ksAddLen == 0)
				dst[dstCount++] = ks;
			else {
				dst[dstCount++] = (ks >> 8) & 0xFF;
				dst[dstCount++] = ks & 0xFF;
			}

			srcCount += UTF8 - ((uchar *)(src + srcCount));
		}
		else
			break;
	}

	*srcLen = srcCount;
	*dstLen = dstCount;

	return ((dstCount > 0) ? B_NO_ERROR : -EINVAL);
}


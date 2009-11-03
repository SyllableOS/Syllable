#ifndef __F_KERNEL_NLS_H__
#define __F_KERNEL_NLS_H__

#include <kernel/types.h>

enum code_page
{
    NLS_UTF8 = 0,
    NLS_ASCII,
    NLS_CP1250,
    NLS_CP1251,
    NLS_CP1255,
    NLS_CP437,
    NLS_CP737,
    NLS_CP775,
    NLS_CP850,
    NLS_CP852,
    NLS_CP855,
    NLS_CP857,
    NLS_CP860,
    NLS_CP861,
    NLS_CP862,
    NLS_CP863,
    NLS_CP864,
    NLS_CP865,
    NLS_CP866,
    NLS_CP869,
    NLS_CP874,
    NLS_CP932,
    NLS_CP936,
    NLS_CP949,
    NLS_CP950,
    NLS_EUC_JP,
    NLS_ISO8859_1,
    NLS_ISO8859_2,
    NLS_ISO8859_3,
    NLS_ISO8859_4,
    NLS_ISO8859_5,
    NLS_ISO8859_6,
    NLS_ISO8859_7,
    NLS_ISO8859_9,
    NLS_ISO8859_13,
    NLS_ISO8859_14,
    NLS_ISO8859_15,
    NLS_KOI8_R,
    NLS_KOI8_RU,
    NLS_KOI8_U,
    NLS_UTF16,
    NLS_UTF16_BE,	// Big Endian UTF-16
    __NLS_CP_MAX
};

int nls_utf8_char_len(uint8_t nFirstByte);

enum code_page nls_find_code_page(const char * pzName);

int nls_conv_cp_to_utf8(enum code_page eSrcCodePage,
                        const char * pzSource,
			char * pzDest,
			int nSrcLen,
			int nDstLen);

int nls_conv_utf8_to_cp(enum code_page eDstCodePage,
		        const char * pzSource,
			char * pzDest,
			int nSrcLen,
			int nDestLen);

#endif /* __F_KERNEL_NLS_H__ */

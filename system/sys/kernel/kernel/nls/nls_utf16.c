#include"nls.h"

static int utf16_char_to_wchar(const char * pzRawString, int nBoundLen, uint32_t * pnUni)
{
 if(nBoundLen < 2) return -ENOSPC;
 *pnUni = (((unsigned char *)pzRawString)[0]) + ((((unsigned char *)pzRawString)[1]) << 8);
 return 2;
}

static int utf16_wchar_to_char(uint32_t nUni, char * pzRawString, int nBoundLen)
{
 if(nBoundLen < 2) return -ENOSPC;
 ((unsigned char *)pzRawString)[0] = nUni & 0xff;
 ((unsigned char *)pzRawString)[1] = (nUni & 0xff00) >> 8;
 return 2;
}

static int utf16_be_char_to_wchar(const char * pzRawString, int nBoundLen, uint32_t * pnUni)
{
 if(nBoundLen < 2) return -ENOSPC;
 *pnUni = (((unsigned char *)pzRawString)[1]) + ((((unsigned char *)pzRawString)[0]) << 8);
 return 2;
}

static int utf16_be_wchar_to_char(uint32_t nUni, char * pzRawString, int nBoundLen)
{
 if(nBoundLen < 2) return -ENOSPC;
 ((unsigned char *)pzRawString)[1] = nUni & 0xff;
 ((unsigned char *)pzRawString)[0] = (nUni & 0xff00) >> 8;
 return 2;
}

NLSTable_s g_NLS_UTF16 = {
    "utf16",
    NULL,
    NLS_UTF16,
    utf16_char_to_wchar,
    utf16_wchar_to_char
};

NLSTable_s g_NLS_UTF16_BE = {
    "utf16-be",
    NULL,
    NLS_UTF16_BE,
    utf16_be_char_to_wchar,
    utf16_be_wchar_to_char
};

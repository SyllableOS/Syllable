#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/smp.h>
#include <atheos/kdebug.h>
#include <atheos/atomic.h>
#include <atheos/nls.h>
#include <atheos/list.h>
#include <posix/errno.h>
#include <macros.h>

typedef struct {
    char *		nt_pzCharset;
    char *		nt_pzAlias;
    enum code_page	nt_eCodePage;
    int (* char_to_wchar)(const char * pzRawString, int nBoundLen, uint32_t * pnUni);
    int (* wchar_to_char)(uint32_t nUni, char * pzRawString, int nBoundLen);
} NLSTable_s;

extern NLSTable_s g_NLS_ISO8859_1;
extern NLSTable_s g_NLS_ISO8859_2;
extern NLSTable_s g_NLS_ISO8859_3;
extern NLSTable_s g_NLS_ASCII;
extern NLSTable_s g_NLS_UTF16;
extern NLSTable_s g_NLS_UTF16_BE;
extern NLSTable_s g_NLS_CP1250;
extern NLSTable_s g_NLS_CP1251;
extern NLSTable_s g_NLS_CP1255;
extern NLSTable_s g_NLS_KOI8_U;
extern NLSTable_s g_NLS_KOI8_R;
extern NLSTable_s g_NLS_CP437;
extern NLSTable_s g_NLS_CP850;
extern NLSTable_s g_NLS_CP852;

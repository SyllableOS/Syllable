#include"nls.h"

static NLSTable_s * g_apsCodePages[__NLS_CP_MAX] = {
    [ NLS_ASCII ] 	= &g_NLS_ASCII,
    [ NLS_ISO8859_1 ] 	= &g_NLS_ISO8859_1,
    [ NLS_ISO8859_2 ] 	= &g_NLS_ISO8859_2,
    [ NLS_ISO8859_3 ] 	= &g_NLS_ISO8859_3,
    [ NLS_UTF16 ] 	= &g_NLS_UTF16,
    [ NLS_UTF16_BE ] 	= &g_NLS_UTF16_BE,
    [ NLS_CP1250 ] 	= &g_NLS_CP1250,
    [ NLS_CP1251 ] 	= &g_NLS_CP1251,
    [ NLS_CP1255 ] 	= &g_NLS_CP1255,
    [ NLS_KOI8_U ] 	= &g_NLS_KOI8_U,
    [ NLS_KOI8_R ] 	= &g_NLS_KOI8_R,
    [ NLS_CP437 ] 	= &g_NLS_CP437,
    [ NLS_CP850 ] 	= &g_NLS_CP850,
    [ NLS_CP852 ] 	= &g_NLS_CP852,
};

int nls_utf8_char_len(uint8_t nFirstByte)
{
 return( ((0xe5000000 >> ((nFirstByte >> 3) & 0x1e)) & 3) + 1 );
}

enum code_page nls_find_code_page(const char * pzName)
{
 int i;
 for(i=0;i<__NLS_CP_MAX;i++)
 {
  NLSTable_s * psTable = g_apsCodePages[i];
  if(!psTable) continue;
  if(!strcmp(pzName, psTable->nt_pzCharset) ||
     (psTable->nt_pzAlias && !strcmp(pzName, psTable->nt_pzAlias)))
  {
   return psTable->nt_eCodePage;
  }
 }
 return NLS_ASCII;
}

static inline int unicode_to_utf8( uint8_t * pzDst, uint32 nChar )
{
 if ((nChar&0xff80) == 0) {
  *pzDst = nChar;
  return( 1 );
 } else if ((nChar&0xf800) == 0) {
  pzDst[0] = 0xc0|(nChar>>6);
  pzDst[1] = 0x80|((nChar)&0x3f);
  return( 2 );
 } else if ((nChar&0xfc00) != 0xd800) {
  pzDst[0] = 0xe0|(nChar>>12);
  pzDst[1] = 0x80|((nChar>>6)&0x3f);
  pzDst[2] = 0x80|((nChar)&0x3f);
  return( 3 );
 } else {
  int   nValue;
  nValue = ( ((nChar<<16)-0xd7c0) << 10 ) | (nChar & 0x3ff);
  pzDst[0] = 0xf0 | (nValue>>18);
  pzDst[1] = 0x80 | ((nValue>>12)&0x3f);
  pzDst[2] = 0x80 | ((nValue>>6)&0x3f);
  pzDst[3] = 0x80 | (nValue&0x3f);
  return( 4 );
 }
}

static inline int utf8_to_unicode( const uint8_t * pzSource )
{
 if ( (pzSource[0]&0x80) == 0 ) {
  return( *pzSource );
 } else if ((pzSource[1] & 0xc0) != 0x80) {
  return( 0xfffd );
 } else if ((pzSource[0]&0x20) == 0) {
  return( ((pzSource[0] & 0x1f) << 6) | (pzSource[1] & 0x3f) );
 } else if ( (pzSource[2] & 0xc0) != 0x80 ) {
  return( 0xfffd );
 } else if ( (pzSource[0] & 0x10) == 0 ) {
  return( ((pzSource[0] & 0x0f)<<12) | ((pzSource[1] & 0x3f)<<6) | (pzSource[2] & 0x3f) );
 } else if ((pzSource[3] & 0xC0) != 0x80) {
  return( 0xfffd );
 } else {
  int   nValue;
  nValue = ((pzSource[0] & 0x07)<<18) | ((pzSource[1] & 0x3f)<<12) | ((pzSource[2] & 0x3f)<<6) | (pzSource[3] & 0x3f);
  return( ((0xd7c0+(nValue>>10)) << 16) | (0xdc00+(nValue & 0x3ff)) );
 }    
}

int nls_conv_cp_to_utf8(enum code_page eSrcCodePage,
                        const char * __pzSource,
			char * __pzDest,
			int nSrcLen,
			int nDstLen)
{
 int i,j;
 uint32_t nChar, nTotal = 0;

 const uint8_t * pzSource = (const uint8_t *)__pzSource;
 uint8_t * pzDest = (uint8_t *)__pzDest;

 if(eSrcCodePage == NLS_UTF8)
 {
  if(nDstLen < nSrcLen) return -ENOSPC;
  nSrcLen = strlen((const char*)pzSource);
  memcpy(pzDest, pzSource, nSrcLen + 1);
  return nSrcLen;
 }

 NLSTable_s * psTable = g_apsCodePages[eSrcCodePage];
 if(!psTable) return -ENOSYS;
    
 while(nSrcLen > 0) {
//  if(pzSource[0] == '\0') break;
  if(nDstLen <= 0) return -ENOSPC;
  j = psTable->char_to_wchar((const char*)pzSource, nSrcLen, &nChar);
  if( j < 0 ) return -ENOSPC;
  i = unicode_to_utf8(pzDest, nChar);
  if( i > nDstLen ) return -ENOSPC;
 
  nSrcLen -= j;
  pzSource += j;

  nDstLen -= i;
  pzDest += i;

  nTotal += i;
 }
 pzDest[0] = '\0';
 return nTotal;
}

int nls_conv_utf8_to_cp(enum code_page eDstCodePage,
		        const char * __pzSource,
			char * __pzDest,
			int nSrcLen,
			int nDestLen)
{
 int i,j;
 uint32_t nChar, nTotal = 0;

 const uint8_t * pzSource = (const uint8_t *)__pzSource;
 uint8_t * pzDest = (uint8_t *)__pzDest;

 if(eDstCodePage == NLS_UTF8)
 {
  if(nDestLen < nSrcLen) return -ENOSPC;
  nSrcLen = strlen((const char*)pzSource);
  memcpy(pzDest, pzSource, nSrcLen + 1);
  return nSrcLen;
 }

 NLSTable_s * psTable = g_apsCodePages[eDstCodePage];
 if(!psTable) return -ENOSYS;
 
 while(nSrcLen > 0)
 {
  if( pzSource[0] == 0 ) break; // real string length < nSrcLen
  if(nDestLen <= 0) return -ENOSPC; // no space in output buffer
  i = nls_utf8_char_len( pzSource[0] );
  if( i > nSrcLen ) return -ENOSPC;
  nChar = utf8_to_unicode( pzSource );
  j = psTable->wchar_to_char(nChar, (char*)pzDest, nDestLen);
  if( j < 0 ) return -ENOSPC;
 
  nSrcLen -= i;
  pzSource += i;

  nDestLen -= j;
  pzDest += j;

  nTotal += j;
 }

 pzDest[0] = 0;
 if(eDstCodePage==NLS_UTF16 || eDstCodePage==NLS_UTF16_BE)
  pzDest[1] = 0;

 return nTotal;
}

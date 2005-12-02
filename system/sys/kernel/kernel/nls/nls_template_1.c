static int wchar_to_char(uint32_t nUni, char * pzRawString, int nBoundLen)
{
 unsigned char *uni2charset;
 unsigned char cl = nUni & 0x00ff;
 unsigned char ch = (nUni & 0xff00) >> 8;
 if (nBoundLen <= 0)
  return -1;
 uni2charset = page_uni2charset[ch];
 if (uni2charset && uni2charset[cl])
  ((unsigned char *)pzRawString)[0] = uni2charset[cl];
 else
  return -EINVAL;
 return 1;
}

static int char_to_wchar(const char * pzRawString, int nBoundLen, uint32_t * pnUni)
{
 *pnUni = charset2uni[((unsigned char *)pzRawString)[0]];
 if (*pnUni == 0x0000)
  return -EINVAL;
 return 1;
}

#define P1(a,b) a##b
#define P(a,b)	P1(a,b)

NLSTable_s P(g_NLS_,CPNAME) = {
    CPSTR,
#ifdef CPALIAS
    CPALIAS,
#else
    NULL,
#endif
    P(NLS_,CPNAME),
    char_to_wchar,
    wchar_to_char
};

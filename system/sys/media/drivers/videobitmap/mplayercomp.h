#ifndef _MPLAYERCOMP_H_
#define _MPLAYERCOMP_H_

#include "cpudetect.h"


#define ARCH_X86
#define HAVE_MMX2
#undef HAVE_MMX
#undef HAVE_3DNOW
#undef HAVE_SSE
#undef HAVE_SSE2

#define MSGT_SWS
#define MSGL_INFO
#define MSGL_WARN

#define mp_msg( nType, nFormat, zText...) printf( zText)


#endif




#ifndef _LINUXCOMP_H_
#define _LINUXCOMP_H_

#include <atheos/linux_compat.h>

/* Missing DSP ctrl values */
#define DSP_CAP_BIND		0x00008000	/* channel binding to front/rear/cneter/lfe */

#define SNDCTL_DSP_GETCHANNELMASK		_SIOWR('P', 64, int)
#define SNDCTL_DSP_BIND_CHANNEL		_SIOWR('P', 65, int)

#define DSP_BIND_QUERY		0x00000000
#define DSP_BIND_FRONT		0x00000001
#define DSP_BIND_SURR		0x00000002
#define DSP_BIND_CENTER_LFE	0x00000004
#define DSP_BIND_SPDIF		0x00000100

#define SNDCTL_DSP_SETSPDIF		_SIOW ('P', 66, int)
#define SNDCTL_DSP_GETSPDIF		_SIOR ('P', 67, int)

#define SPDIF_PRO	0x0001
#define SPDIF_N_AUD	0x0002
#define SPDIF_COPY	0x0004
#define SPDIF_PRE	0x0008
#define SPDIF_CC		0x07f0
#define SPDIF_L		0x0800
#define SPDIF_DRS	0x4000
#define SPDIF_V		0x8000


#endif


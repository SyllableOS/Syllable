/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/nv/nv_type.h,v 1.34 2002/03/18 21:47:48 mvojkovi Exp $ */

#ifndef __NV_STRUCT_H__
#define __NV_STRUCT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "riva_hw.h"

#define SetBitField(value,from,to) SetBF(to, GetBF(value,from))
#define SetBit(n) (1<<(n))
#define Set8Bits(value) ((value)&0xff)

#define MAX_CURS            32

typedef RIVA_HW_STATE* NVRegPtr;

typedef struct {
    RIVA_HW_INST        riva;
    RIVA_HW_STATE       SavedReg;
    RIVA_HW_STATE       ModeReg;
    int                 Chipset;
    int                 ChipRev;
    Bool                Primary;
    /* Color expansion */
    Bool                useFifo;
    unsigned char       *expandBuffer;
    unsigned char       *expandFifo;
    int                 expandWidth;
    int                 expandRows;
    /* Misc flags */
    int			FlatPanel;
    Bool		SecondCRTC;
    int			forceCRTC;
} NVRec, *NVPtr;


int RivaGetConfig(NVPtr);

#ifdef __cplusplus
}
#endif

#endif /* __NV_STRUCT_H__ */



#ifndef __FX_HW_H__
#define __FX_HW_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <atheos/types.h>
#include <atheos/isa_io.h>
#include <atheos/kdebug.h>
#include <appserver/pci_graphics.h>

#define NV_ARCH_10  0x10
#define NV_ARCH_20  0x20
#define NV_ARCH_30  0x30
#define NV_ARCH_40  0x40

/* Little macro to construct bitmask for contiguous ranges of bits */
#define BITMASK(t,b) (((unsigned)(1U << (((t)-(b)+1)))-1)  << (b))
#define MASKEXPAND(mask) BITMASK(1?mask,0?mask)

/* Macro to set specific bitfields (mask has to be a macro x:y) ! */
#define SetBF(mask,value) ((value) << (0?mask))
#define GetBF(var,mask) (((unsigned)((var) & MASKEXPAND(mask))) >> (0?mask) )
#define SetBitField(value,from,to) SetBF(to, GetBF(value,from))
#define SetBit(n) (1<<(n))
#define Set8Bits(value) ((value)&0xff)

#define MAX_CURS            64

enum {
	V_DBLSCAN
};

/*
 * Typedefs to force certain sized values.
 */
typedef unsigned char  U008;
typedef unsigned short U016;
typedef unsigned int   U032;
typedef bool			Bool;
typedef unsigned char  CARD8;
typedef unsigned int   CARD32;
/*
 * HW access macros.
 */
#define NV_WR08(p,i,d)	(((U008 *)(p))[i]=(d))
#define NV_RD08(p,i)	(((U008 *)(p))[i])
#define NV_WR16(p,i,d)	(((U016 *)(p))[(i)/2]=(d))
#define NV_RD16(p,i)	(((U016 *)(p))[(i)/2])
#define NV_WR32(p,i,d)	(((U032 *)(p))[(i)/4]=(d))
#define NV_RD32(p,i)	(((U032 *)(p))[(i)/4])
#define VGA_WR08(p,i,d)	NV_WR08(p,i,d)
#define VGA_RD08(p,i)	NV_RD08(p,i)



/* DMA macros */
#define DmaNext(pNv, data) \
     (pNv)->dmaBase[(pNv)->dmaCurrent++] = (data)

#define DmaStart(pNv, tag, size) {          \
     if((pNv)->dmaFree <= (size))             \
        DmaWait(pNv, size);                 \
     DmaNext(pNv, ((size) << 18) | (tag));  \
     (pNv)->dmaFree -= ((size) + 1);          \
}

#define _NV_FENCE() outb(0, 0x3D0);

#define WRITE_PUT(pNv, data) {       \
	volatile uint8 scratch;            \
	_NV_FENCE()                        \
	scratch = (pNv)->FbStart[0];       \
	(pNv)->FIFO[0x0010] = (data) << 2; \
}


#define READ_GET(pNv) ((pNv)->FIFO[0x0011] >> 2)


typedef struct _riva_hw_state
{
    U032 bpp;
    U032 width;
    U032 height;
    U032 interlace;
    U032 repaint0;
    U032 repaint1;
    U032 screen;
    U032 scale;
    U032 dither;
    U032 extra;
    U032 fifo;
    U032 pixel;
    U032 horiz;
    U032 arbitration0;
    U032 arbitration1;
    U032 pll;
    U032 pllB;
    U032 vpll;
    U032 vpll2;
    U032 vpllB;
    U032 vpll2B;
    U032 pllsel;
    U032 control;
    U032 general;
    U032 crtcOwner;
    U032 head;
    U032 head2;
    U032 config;
    U032 cursorConfig;
    U032 cursor0;
    U032 cursor1;
    U032 cursor2;
    U032 timingH;
    U032 timingV;
    U032 displayV;
    U032 crtcSync;
} FX_HW_STATE, *FXRegPtr;



typedef struct {
	int					Fd;
    FX_HW_STATE	        ModeReg;
    FX_HW_STATE*        CurrentState;
    uint32              Architecture;
    uint32              CursorStart;
    int                 Chipset;
    int                 ChipRev;
    bool                Primary;
    uint32              IOAddress;
    unsigned long       FbAddress;
    unsigned char *     FbBase;
    unsigned char *     FbStart;
    uint32              FbMapSize;
    uint32              FbUsableSize;
    uint32              ScratchBufferSize;
    uint32              ScratchBufferStart;
    unsigned char *     ShadowPtr;
    int                 ShadowPitch;
    uint32              MinVClockFreqKHz;
    uint32              MaxVClockFreqKHz;
    uint32              CrystalFreqKHz;
    uint32              RamAmountKBytes;

    volatile U032 *REGS;
    volatile U032 *PCRTC0;
    volatile U032 *PCRTC;
    volatile U032 *PRAMDAC0;
    volatile U032 *PFB;
    volatile U032 *PFIFO;
    volatile U032 *PGRAPH;
    volatile U032 *PEXTDEV;
    volatile U032 *PTIMER;
    volatile U032 *PMC;
    volatile U032 *PRAMIN;
    volatile U032 *FIFO;
    volatile U032 *CURSOR;
    volatile U008 *PCIO0;
    volatile U008 *PCIO;
    volatile U008 *PVIO;
    volatile U008 *PDIO0;
    volatile U008 *PDIO;
    volatile U032 *PRAMDAC;

   
    int			videoKey;
    int			FlatPanel;
    bool                FPDither;
    bool                Television;
    int			CRTCnumber;
    bool                alphaCursor;
    unsigned char       DDCBase;
    bool                twoHeads;
    bool                twoStagePLL;
    bool				fpScaler;
    int                 fpWidth;
    int                 fpHeight;
    uint32              fpSyncs;

    uint32              dmaPut;
    uint32				dmaCurrent;
    uint32       	  dmaFree;
    uint32              dmaMax;
    uint32              *dmaBase;
    Bool                LVDS;

    uint32              currentRop;
} NVRec, *NVPtr;



void NVLockUnlock (
    NVPtr pNv,
    Bool  Lock
);


int NVShowHideCursor (
    NVPtr pNv,
    int   ShowHide
);

void NVCalcStateExt (
    NVPtr pNv,
    FX_HW_STATE *state,
    int            bpp,
    int            width,
    int            hDisplaySize,
    int            height,
    int            dotClock,
    int		   flags 
);

void NVLoadStateExt (
    NVPtr pNv,
    FX_HW_STATE *state
);

void NVSetStartAddress (
    NVPtr   pNv,
    CARD32 start
);

#ifdef __cplusplus
}
#endif

#endif /* __FX_HW_H__ */




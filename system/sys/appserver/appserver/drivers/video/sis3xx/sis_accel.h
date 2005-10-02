
#ifndef _SIS_ACCEL_H
#define _SIS_ACCEL_H

/* Definitions for the SIS engine communication. */

#define PATREGSIZE      384  /* Pattern register size. 384 bytes @ 0x8300 */
#define BR(x)   (0x8200 | (x) << 2)
#define PBR(x)  (0x8300 | (x) << 2)

/* SiS300 engine commands */
#define BITBLT                  0x00000000  /* Blit */
#define COLOREXP                0x00000001  /* Color expand */
#define ENCOLOREXP              0x00000002  /* Enhanced color expand */
#define MULTIPLE_SCANLINE       0x00000003  /* ? */
#define LINE                    0x00000004  /* Draw line */
#define TRAPAZOID_FILL          0x00000005  /* Fill trapezoid */
#define TRANSPARENT_BITBLT      0x00000006  /* Transparent Blit */

/* Additional engine commands for 310/325 */
#define ALPHA_BLEND		0x00000007  /* Alpha blend ? */
#define A3D_FUNCTION		0x00000008  /* 3D command ? */
#define	CLEAR_Z_BUFFER		0x00000009  /* ? */
#define GRADIENT_FILL		0x0000000A  /* Gradient fill */
#define STRETCH_BITBLT		0x0000000B  /* Stretched Blit */

/* source select */
#define SRCVIDEO                0x00000000  /* source is video RAM */
#define SRCSYSTEM               0x00000010  /* source is system memory */
#define SRCCPUBLITBUF           SRCSYSTEM   /* source is CPU-driven BitBuffer (for color expand) */
#define SRCAGP                  0x00000020  /* source is AGP memory (?) */

/* Pattern flags */
#define PATFG                   0x00000000  /* foreground color */
#define PATPATREG               0x00000040  /* pattern in pattern buffer (0x8300) */
#define PATMONO                 0x00000080  /* mono pattern */

/* blitting direction (300 series only) */
#define X_INC                   0x00010000
#define X_DEC                   0x00000000
#define Y_INC                   0x00020000
#define Y_DEC                   0x00000000

/* Clipping flags */
#define NOCLIP                  0x00000000
#define NOMERGECLIP             0x04000000
#define CLIPENABLE              0x00040000
#define CLIPWITHOUTMERGE        0x04040000

/* Transparency */
#define OPAQUE                  0x00000000
#define TRANSPARENT             0x00100000

/* Subfunctions for Alpha Blended BitBlt */
#define A_CONSTANTALPHA         0x00000000
#define A_PERPIXELALPHA			0x00080000
#define A_NODESTALPHA			0x00100000
#define A_3DFULLSCENE			0x00180000

/* Destination */
#define DSTAGP                  0x02000000
#define DSTVIDEO                0x00000000

/* Line */
#define LINE_STYLE              0x00800000
#define NO_RESET_COUNTER        0x00400000
#define NO_LAST_PIXEL           0x00200000

/* Subfunctions for Color/Enhanced Color Expansion (310/325 only) */
#define COLOR_TO_MONO		0x00100000
#define AA_TEXT			0x00200000

/* Some general registers for 310/325 series */
#define SRC_ADDR		0x8200
#define SRC_PITCH		0x8204
#define AGP_BASE		0x8206 /* color-depth dependent value */
#define SRC_Y			0x8208
#define SRC_X			0x820A
#define DST_Y			0x820C
#define DST_X			0x820E
#define DST_ADDR		0x8210
#define DST_PITCH		0x8214
#define DST_HEIGHT		0x8216
#define RECT_WIDTH		0x8218
#define RECT_HEIGHT		0x821A
#define PAT_FGCOLOR		0x821C
#define PAT_BGCOLOR		0x8220
#define SRC_FGCOLOR		0x8224
#define SRC_BGCOLOR		0x8228
#define MONO_MASK		0x822C
#define LEFT_CLIP		0x8234
#define TOP_CLIP		0x8236
#define RIGHT_CLIP		0x8238
#define BOTTOM_CLIP		0x823A
#define COMMAND_READY		0x823C
#define FIRE_TRIGGER      	0x8240

#define PATTERN_REG		0x8300  /* 384 bytes pattern buffer */

/* Line registers */
#define LINE_X0			SRC_Y
#define LINE_X1			DST_Y
#define LINE_Y0			SRC_X
#define LINE_Y1			DST_X
#define LINE_COUNT		RECT_WIDTH
#define LINE_STYLE_PERIOD	RECT_HEIGHT
#define LINE_STYLE_0		MONO_MASK
#define LINE_STYLE_1		0x8230
#define LINE_XN			PATTERN_REG
#define LINE_YN			PATTERN_REG+2

/* Transparent bitblit registers */
#define TRANS_DST_KEY_HIGH	PAT_FGCOLOR
#define TRANS_DST_KEY_LOW	PAT_BGCOLOR
#define TRANS_SRC_KEY_HIGH	SRC_FGCOLOR
#define TRANS_SRC_KEY_LOW	SRC_BGCOLOR

/* Queue */
#define Q_BASE_ADDR		0x85C0  /* Base address of software queue (?) */
#define Q_WRITE_PTR		0x85C4  /* Current write pointer (?) */
#define Q_READ_PTR		0x85C8  /* Current read pointer (?) */
#define Q_STATUS		0x85CC  /* queue status */


#define MMIO_IN8(base, offset) \
	*(volatile uint8 *)(((uint8*)(base)) + (offset))
#define MMIO_IN16(base, offset) \
	*(volatile uint16 *)(void *)(((uint8*)(base)) + (offset))
#define MMIO_IN32(base, offset) \
	*(volatile uint32 *)(void *)(((uint8*)(base)) + (offset))
#define MMIO_OUT8(base, offset, val) \
	*(volatile uint8 *)(((uint8*)(base)) + (offset)) = (val)
#define MMIO_OUT16(base, offset, val) \
	*(volatile uint16 *)(void *)(((uint8*)(base)) + (offset)) = (val)
#define MMIO_OUT32(base, offset, val) \
	*(volatile uint32 *)(void *)(((uint8*)(base)) + (offset)) = (val)

/* ------------- SiS 300 series -------------- */

/* Macros to do useful things with the SIS BitBLT engine */

/* BR(16) (0x8420):

   bit 31 2D engine: 1 is idle,
   bit 30 3D engine: 1 is idle,
   bit 29 Command queue: 1 is empty

   bits 28:24: Current CPU driven BitBlt buffer stage bit[4:0]

   bits 15:0:  Current command queue length

*/

/* TW: BR(16)+2 = 0x8242 */

static int     CmdQueLen;

#define SiS300Idle \
  { \
  while( (MMIO_IN16(si.regs, BR(16)+2) & 0xE000) != 0xE000){}; \
  while( (MMIO_IN16(si.regs, BR(16)+2) & 0xE000) != 0xE000){}; \
  while( (MMIO_IN16(si.regs, BR(16)+2) & 0xE000) != 0xE000){}; \
  CmdQueLen=MMIO_IN16(si.regs, 0x8240); \
  }
/* TW: (do three times, because 2D engine seems quite unsure about whether or not it's idle) */

#define SiS300SetupSRCBase(base) \
                if (CmdQueLen <= 0)  SiS300Idle;\
                MMIO_OUT32(si.regs, BR(0), base);\
                CmdQueLen --;

#define SiS300SetupSRCPitch(pitch) \
                if (CmdQueLen <= 0)  SiS300Idle;\
                MMIO_OUT16(si.regs, BR(1), pitch);\
                CmdQueLen --;

#define SiS300SetupSRCXY(x,y) \
                if (CmdQueLen <= 0)  SiS300Idle;\
                MMIO_OUT32(si.regs, BR(2), (x)<<16 | (y) );\
                CmdQueLen --;

#define SiS300SetupDSTBase(base) \
                if (CmdQueLen <= 0)  SiS300Idle;\
                MMIO_OUT32(si.regs, BR(4), base);\
                CmdQueLen --;

#define SiS300SetupDSTXY(x,y) \
                if (CmdQueLen <= 0)  SiS300Idle;\
                MMIO_OUT32(si.regs, BR(3), (x)<<16 | (y) );\
                CmdQueLen --;

#define SiS300SetupDSTRect(x,y) \
                if (CmdQueLen <= 0)  SiS300Idle;\
                MMIO_OUT32(si.regs, BR(5), (y)<<16 | (x) );\
                CmdQueLen --;

#define SiS300SetupDSTColorDepth(bpp) \
                if (CmdQueLen <= 0)  SiS300Idle;\
                MMIO_OUT16(si.regs, BR(1)+2, bpp);\
                CmdQueLen --;

#define SiS300SetupRect(w,h) \
                if (CmdQueLen <= 0)  SiS300Idle;\
                MMIO_OUT32(si.regs, BR(6), (h)<<16 | (w) );\
                CmdQueLen --;

#define SiS300SetupPATFG(color) \
                if (CmdQueLen <= 0)  SiS300Idle;\
                MMIO_OUT32(si.regs, BR(7), color);\
                CmdQueLen --;

#define SiS300SetupPATBG(color) \
                if (CmdQueLen <= 0)  SiS300Idle;\
                MMIO_OUT32(si.regs, BR(8), color);\
                CmdQueLen --;

#define SiS300SetupSRCFG(color) \
                if (CmdQueLen <= 0)  SiS300Idle;\
                MMIO_OUT32(si.regs, BR(9), color);\
                CmdQueLen --;

#define SiS300SetupSRCBG(color) \
                if (CmdQueLen <= 0)  SiS300Idle;\
                MMIO_OUT32(si.regs, BR(10), color);\
                CmdQueLen --;

/* 0x8224 src colorkey high */
/* 0x8228 src colorkey low */
/* 0x821c dest colorkey high */
/* 0x8220 dest colorkey low */
#define SiS300SetupSRCTrans(color) \
                if (CmdQueLen <= 1)  SiS300Idle;\
                MMIO_OUT32(si.regs, 0x8224, color);\
		MMIO_OUT32(si.regs, 0x8228, color);\
		CmdQueLen -= 2;

#define SiS300SetupDSTTrans(color) \
		if (CmdQueLen <= 1)  SiS300Idle;\
		MMIO_OUT32(si.regs, 0x821C, color); \
		MMIO_OUT32(si.regs, 0x8220, color); \
                CmdQueLen -= 2;

#define SiS300SetupMONOPAT(p0,p1) \
                if (CmdQueLen <= 1)  SiS300Idle;\
                MMIO_OUT32(si.regs, BR(11), p0);\
                MMIO_OUT32(si.regs, BR(12), p1);\
                CmdQueLen -= 2;

#define SiS300SetupClipLT(left,top) \
                if (CmdQueLen <= 0)  SiS300Idle;\
                MMIO_OUT32(si.regs, BR(13), ((left) & 0xFFFF) | (top)<<16 );\
                CmdQueLen--;

#define SiS300SetupClipRB(right,bottom) \
                if (CmdQueLen <= 0)  SiS300Idle;\
                MMIO_OUT32(si.regs, BR(14), ((right) & 0xFFFF) | (bottom)<<16 );\
                CmdQueLen--;

/* General */
#define SiS300SetupROP(rop) \
                m_pHw->CmdReg = (rop) << 8;

#define SiS300SetupCMDFlag(flags) \
                m_pHw->CmdReg |= (flags);

#define SiS300DoCMD \
                if (CmdQueLen <= 1)  SiS300Idle;\
                MMIO_OUT32(si.regs, BR(15), m_pHw->CmdReg); \
                MMIO_OUT32(si.regs, BR(16), 0);\
                CmdQueLen -= 2;

/* Line */
#define SiS300SetupX0Y0(x,y) \
                if (CmdQueLen <= 0)  SiS300Idle;\
                MMIO_OUT32(si.regs, BR(2), (y)<<16 | (x) );\
                CmdQueLen--;

#define SiS300SetupX1Y1(x,y) \
                if (CmdQueLen <= 0)  SiS300Idle;\
                MMIO_OUT32(si.regs, BR(3), (y)<<16 | (x) );\
                CmdQueLen--;

#define SiS300SetupLineCount(c) \
                if (CmdQueLen <= 0)  SiS300Idle;\
                MMIO_OUT16(si.regs, BR(6), c);\
                CmdQueLen--;

#define SiS300SetupStylePeriod(p) \
                if (CmdQueLen <= 0)  SiS300Idle;\
                MMIO_OUT16(si.regs, BR(6)+2, p);\
                CmdQueLen--;

#define SiS300SetupStyleLow(ls) \
                if (CmdQueLen <= 0)  SiS300Idle;\
                MMIO_OUT32(si.regs, BR(11), ls);\
                CmdQueLen--;

#define SiS300SetupStyleHigh(ls) \
                if (CmdQueLen <= 0)  SiS300Idle;\
                MMIO_OUT32(si.regs, BR(12), ls);\
                CmdQueLen--;



/* ----------- SiS 310/325 series --------------- */

/* VRAM queue operation command header definitions */
#define SIS_SPKC_HEADER 	0x16800000L
#define SIS_BURST_HEADER0	0x568A0000L
#define SIS_BURST_HEADER1	0x62100000L
#define SIS_PACKET_HEARER0 	0x968A0000L
#define SIS_PACKET_HEADER1	0x62100000L
#define SIS_NIL_CMD		0x168F0000L

#define SIS_PACKET12_HEADER0	0x968A000CL
#define SIS_PACKET12_HEADER1	0x62100010L
#define SIS_PACKET12_LENGTH	80

/* Q_STATUS:
   bit 31 = 1: All engines idle and all queues empty
   bit 30 = 1: Hardware Queue (=HW CQ, 2D queue, 3D queue) empty
   bit 29 = 1: 2D engine is idle
   bit 28 = 1: 3D engine is idle
   bit 27 = 1: HW command queue empty
   bit 26 = 1: 2D queue empty
   bit 25 = 1: 3D queue empty
   bit 24 = 1: SW command queue empty
   bits 23:16: 2D counter 3
   bits 15:8:  2D counter 2
   bits 7:0:   2D counter 1

   Where is the command queue length (current amount of commands the queue
   can accept) on the 310/325 series? (The current implementation is taken
   from 300 series and certainly wrong...)
*/

#define SiSQEmpty \
  { \
     while( (MMIO_IN16(si.regs, Q_STATUS+2) & 0x0400) != 0x0400) {}; \
     while( (MMIO_IN16(si.regs, Q_STATUS+2) & 0x0400) != 0x0400) {}; \
  }
  
#define SiSResetCmd		si.CmdReg = 0;

#define SiSSetupCMDFlag(flags)  si.CmdReg |= (flags);

/* --- VRAM mode --- */

typedef char* pointer;

#define SiSWriteQueue(tt)
#define SIS_WQINDEX(i)  ((uint32 *)(tt))[(i)]
#define SIS_RQINDEX(i)  ((volatile uint32 *)(tt))[(i)]

#define SiSGetSwWP() (uint32)((si.cmdQ_SharedWritePort))
#define SiSGetHwRP() (uint32)(MMIO_IN32(si.regs, Q_READ_PTR))

#define SiSFlushCmdBuf  \
  if(si.chip >= SIS_660 && si.chip <= SIS_770) { \
     uint32 ttt = ((SiSGetSwWP()) - 4) & si.cmdQueueSizeMask;	\
     pointer tt = (char *)si.cmdQueueBase + ttt; 			\
     dummybuf = SIS_RQINDEX(0);						\
  }

#define SiSSyncWP    \
  SiSFlushCmdBuf;    \
  MMIO_OUT32(si.regs, Q_WRITE_PTR, (uint32)((si.cmdQ_SharedWritePort)));

#define SiSSetHwWP(p) \
  (si.cmdQ_SharedWritePort) = (p);   	\
  MMIO_OUT32(si.regs, Q_WRITE_PTR, (p));

#define SiSSetSwWP(p) (si.cmdQ_SharedWritePort) = (p);

#define SiSCheckQueue(amount)


#define SiSUpdateQueue \
      SiSWriteQueue(tt); \
      ttt += 16; \
      ttt &= si.cmdQueueSizeMask; \
      if(!ttt) { \
	 while(MMIO_IN32(si.regs, Q_READ_PTR) < si.cmdQueueSize_div4) {} \
      } else if(ttt == si.cmdQueueSize_div4) { \
	 uint32 temppp; \
	 do { \
	    temppp = MMIO_IN32(si.regs, Q_READ_PTR); \
	 } while(temppp >= ttt && temppp <= si.cmdQueueSize_div2); \
      } else if(ttt == si.cmdQueueSize_div2) { \
	 uint32 temppp; \
	 do { \
	    temppp = MMIO_IN32(si.regs, Q_READ_PTR); \
	 } while(temppp >= ttt && temppp <= si.cmdQueueSize_4_3); \
      } else if(ttt == si.cmdQueueSize_4_3) { \
	 while(MMIO_IN32(si.regs, Q_READ_PTR) > ttt) {} \
      }

/* Write-updates MUST be 128bit aligned. */
#define SiSNILandUpdateSWQueue \
      SIS_WQINDEX(2) = (uint32)(SIS_NIL_CMD); \
      SIS_WQINDEX(3) = (uint32)(SIS_NIL_CMD); \
      SiSUpdateQueue; \
      SiSSetSwWP(ttt);

#define SiSIdle \
  { \
     while( (MMIO_IN16(si.regs, Q_STATUS+2) & 0x8000) != 0x8000) {}; \
     while( (MMIO_IN16(si.regs, Q_STATUS+2) & 0x8000) != 0x8000) {}; \
     while( (MMIO_IN16(si.regs, Q_STATUS+2) & 0x8000) != 0x8000) {}; \
     while( (MMIO_IN16(si.regs, Q_STATUS+2) & 0x8000) != 0x8000) {}; \
  }

#define SiSSetupSRCDSTBase(srcbase,dstbase) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + SRC_ADDR); \
	 SIS_WQINDEX(1) = (uint32)(srcbase); 			\
	 SIS_WQINDEX(2) = (uint32)(SIS_SPKC_HEADER + DST_ADDR); \
	 SIS_WQINDEX(3) = (uint32)(dstbase); 			\
	 SiSUpdateQueue \
	 SiSSetSwWP(ttt); \
      }

#define SiSSetupSRCDSTXY(sx,sy,dx,dy) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + SRC_Y); 	\
	 SIS_WQINDEX(1) = (uint32)(((sx)<<16) | (sy));		\
	 SIS_WQINDEX(2) = (uint32)(SIS_SPKC_HEADER + DST_Y); 	\
	 SIS_WQINDEX(3) = (uint32)(((dx)<<16) | (dy)); 		\
	 SiSUpdateQueue \
	 SiSSetSwWP(ttt); \
      }

#define SiSSetupDSTXYRect(x,y,w,h) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + DST_Y); 		\
	 SIS_WQINDEX(1) = (uint32)(((x)<<16) | (y));	 		\
	 SIS_WQINDEX(2) = (uint32)(SIS_SPKC_HEADER + RECT_WIDTH); 	\
	 SIS_WQINDEX(3) = (uint32)(((h)<<16) | (w));			\
	 SiSUpdateQueue \
	 SiSSetSwWP(ttt); \
      }

#define SiSSetupSRCPitchDSTRect(pitch,x,y) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + SRC_PITCH); 	\
	 SIS_WQINDEX(1) = (uint32)(pitch);				\
	 SIS_WQINDEX(2) = (uint32)(SIS_SPKC_HEADER + DST_PITCH); 	\
	 SIS_WQINDEX(3) = (uint32)(((y)<<16) | (x));			\
	 SiSUpdateQueue \
	 SiSSetSwWP(ttt); \
      }

#define SiSSetupSRCBase(base) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + SRC_ADDR); 	\
	 SIS_WQINDEX(1) = (uint32)(base); 				\
	 SiSNILandUpdateSWQueue \
      }

#define SiSSetupSRCPitch(pitch) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + SRC_PITCH); 	\
	 SIS_WQINDEX(1) = (uint32)(pitch);				\
	 SiSNILandUpdateSWQueue \
      }

#define SiSSetupSRCXY(x,y) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + SRC_Y); 	\
	 SIS_WQINDEX(1) = (uint32)(((x)<<16) | (y));		\
	 SiSNILandUpdateSWQueue \
      }

#define SiSSetupDSTBase(base) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + DST_ADDR); 	\
	 SIS_WQINDEX(1) = (uint32)(base);				\
	 SiSNILandUpdateSWQueue \
      }

#define SiSSetupDSTXY(x,y) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + DST_Y); 	\
	 SIS_WQINDEX(1) = (uint32)(((x)<<16) | (y));	 	\
	 SiSNILandUpdateSWQueue \
      }

#define SiSSetupDSTRect(x,y) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + DST_PITCH); 	\
	 SIS_WQINDEX(1) = (uint32)(((y)<<16) | (x));		\
	 SiSNILandUpdateSWQueue \
      }

#define SiSSetupDSTRectBurstHeader(x,y,reg,num) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + DST_PITCH);	\
	 SIS_WQINDEX(1) = (uint32)(((y)<<16) | (x));			\
	 SIS_WQINDEX(2) = (uint32)(SIS_BURST_HEADER0 + reg); 		\
	 SIS_WQINDEX(3) = (uint32)(SIS_BURST_HEADER1 + num); 		\
	 SiSUpdateQueue \
	 SiSSetSwWP(ttt); \
      }

#define SiSSetupDSTColorDepth(bpp) \
      si.CmdReg = (((uint32)(bpp)) & (GENMASK(17:16)));

#define SiSSetupPATFGDSTRect(color,x,y) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + PAT_FGCOLOR); 	\
	 SIS_WQINDEX(1) = (uint32)(color);	 			\
	 SIS_WQINDEX(2) = (uint32)(SIS_SPKC_HEADER + DST_PITCH); 	\
	 SIS_WQINDEX(3) = (uint32)(((y)<<16) | (x));			\
	 SiSUpdateQueue \
	 SiSSetSwWP(ttt); \
      }

#define SiSSetupSRCFGDSTRect(color,x,y) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + SRC_FGCOLOR); 	\
	 SIS_WQINDEX(1) = (uint32)(color);	 			\
	 SIS_WQINDEX(2) = (uint32)(SIS_SPKC_HEADER + DST_PITCH); 	\
	 SIS_WQINDEX(3) = (uint32)(((y)<<16) | (x));			\
	 SiSUpdateQueue \
	 SiSSetSwWP(ttt); \
      }

#define SiSSetupRectSRCPitch(w,h,pitch) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + RECT_WIDTH); 	\
	 SIS_WQINDEX(1) = (uint32)(((h)<<16) | (w));			\
	 SIS_WQINDEX(2) = (uint32)(SIS_SPKC_HEADER + SRC_PITCH); 	\
	 SIS_WQINDEX(3) = (uint32)(pitch);				\
	 SiSUpdateQueue \
	 SiSSetSwWP(ttt); \
      }

#define SiSSetupRect(w,h) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + RECT_WIDTH); 	\
	 SIS_WQINDEX(1) = (uint32)(((h)<<16) | (w));			\
	 SiSNILandUpdateSWQueue \
      }

#define SiSSetupPATFG(color) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + PAT_FGCOLOR); 	\
	 SIS_WQINDEX(1) = (uint32)(color);	 			\
	 SiSNILandUpdateSWQueue \
      }

#define SiSSetupPATBG(color) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + PAT_BGCOLOR);	\
	 SIS_WQINDEX(1) = (uint32)(color);	 			\
	 SiSNILandUpdateSWQueue \
      }

#define SiSSetupSRCFG(color) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + SRC_FGCOLOR);	\
	 SIS_WQINDEX(1) = (uint32)(color);	 			\
	 SiSNILandUpdateSWQueue \
      }

#define SiSSetupSRCBG(color) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + SRC_BGCOLOR);	\
	 SIS_WQINDEX(1) = (uint32)(color);	 			\
	 SiSNILandUpdateSWQueue \
      }

#define SiSSetupSRCTrans(color) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + TRANS_SRC_KEY_HIGH);	\
	 SIS_WQINDEX(1) = (uint32)(color);	 				\
	 SIS_WQINDEX(2) = (uint32)(SIS_SPKC_HEADER + TRANS_SRC_KEY_LOW);	\
	 SIS_WQINDEX(3) = (uint32)(color);					\
	 SiSUpdateQueue \
	 SiSSetSwWP(ttt); \
      }

#define SiSSetupDSTTrans(color) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + TRANS_DST_KEY_HIGH);	\
	 SIS_WQINDEX(1) = (uint32)(color);	 				\
	 SIS_WQINDEX(2) = (uint32)(SIS_SPKC_HEADER + TRANS_DST_KEY_LOW);	\
	 SIS_WQINDEX(3) = (uint32)(color);					\
	 SiSUpdateQueue \
	 SiSSetSwWP(ttt); \
      }

#define SiSSetupMONOPAT(p0,p1) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + MONO_MASK);		\
	 SIS_WQINDEX(1) = (uint32)(p0);	 				\
	 SIS_WQINDEX(2) = (uint32)(SIS_SPKC_HEADER + MONO_MASK + 4);	\
	 SIS_WQINDEX(3) = (uint32)(p1);					\
	 SiSUpdateQueue \
	 SiSSetSwWP(ttt); \
      }

#define SiSSetupClip(left,top,right,bottom) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + LEFT_CLIP);	\
	 SIS_WQINDEX(1) = (uint32)(((left) & 0xFFFF) | ((top)<<16));   	\
	 SIS_WQINDEX(2) = (uint32)(SIS_SPKC_HEADER + RIGHT_CLIP);	\
	 SIS_WQINDEX(3) = (uint32)(((right) & 0xFFFF)|((bottom)<<16)); 	\
	 SiSUpdateQueue \
	 SiSSetSwWP(ttt); \
      }

#define SiSSetupDSTBaseDoCMD(base) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + DST_ADDR); 	\
	 SIS_WQINDEX(1) = (uint32)(base);				\
	 SIS_WQINDEX(2) = (uint32)(SIS_SPKC_HEADER + COMMAND_READY);	\
	 SIS_WQINDEX(3) = (uint32)(si.CmdReg); 			\
	 if(si.chip >= SIS_660 && si.chip <= SIS_770) dummybuf = SIS_RQINDEX(3);   		\
	 SiSUpdateQueue \
	 SiSSetHwWP(ttt); \
      }

#define SiSSetRectDoCMD(w,h) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + RECT_WIDTH); 	\
	 SIS_WQINDEX(1) = (uint32)(((h)<<16) | (w));	 		\
	 SIS_WQINDEX(2) = (uint32)(SIS_SPKC_HEADER + COMMAND_READY);	\
	 SIS_WQINDEX(3) = (uint32)(si.CmdReg); 			\
	 if(si.chip >= SIS_660 && si.chip <= SIS_770) dummybuf = SIS_RQINDEX(3);  		\
	 SiSUpdateQueue 	\
	 SiSSetHwWP(ttt); 	\
      }

#define SiSSetupROP(rop) \
      si.CmdReg |= (rop) << 8;

#define SiSDoCMD \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + COMMAND_READY);	\
	 SIS_WQINDEX(1) = (uint32)(si.CmdReg); 			\
	 SIS_WQINDEX(2) = (uint32)(SIS_NIL_CMD); 			\
	 SIS_WQINDEX(3) = (uint32)(SIS_NIL_CMD); 			\
	 if(si.chip >= SIS_660 && si.chip <= SIS_770) dummybuf = SIS_RQINDEX(3);  		\
	 SiSUpdateQueue \
	 SiSSetHwWP(ttt); \
      }

#define SiSDualPipe(disable) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 uint32 _tmp = MMIO_IN32(si.regs, FIRE_TRIGGER) & ~(1 << 10);	\
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + FIRE_TRIGGER);	\
	 SIS_WQINDEX(1) = (uint32)(_tmp | ((disable & 1) << 10)); 	\
	 SIS_WQINDEX(2) = (uint32)(SIS_NIL_CMD); 			\
	 SIS_WQINDEX(3) = (uint32)(SIS_NIL_CMD); 			\
	 if(si.chip >= SIS_660 && si.chip <= SIS_770) dummybuf = SIS_RQINDEX(3); \
	 SiSUpdateQueue \
	 SiSSetHwWP(ttt); \
      }

/* Line */

#define SiSSetupX0Y0X1Y1(x1,y1,x2,y2) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + LINE_X0);	\
	 SIS_WQINDEX(1) = (uint32)(((y1)<<16) | (x1)); 		\
	 SIS_WQINDEX(2) = (uint32)(SIS_SPKC_HEADER + LINE_X1);	\
	 SIS_WQINDEX(3) = (uint32)(((y2)<<16) | (x2)); 		\
	 SiSUpdateQueue \
	 SiSSetSwWP(ttt); \
      }

#define SiSSetupX0Y0(x,y) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + LINE_X0);	\
	 SIS_WQINDEX(1) = (uint32)(((y)<<16) | (x)); 		\
	 SiSNILandUpdateSWQueue \
      }

#define SiSSetupX1Y1(x,y) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + LINE_X1);	\
	 SIS_WQINDEX(1) = (uint32)(((y)<<16) | (x)); 		\
	 SiSNILandUpdateSWQueue \
      }

#define SiSSetupLineCountPeriod(c, p) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + LINE_COUNT);	\
	 SIS_WQINDEX(1) = (uint32)(((p) << 16) | (c)); 			\
	 SiSNILandUpdateSWQueue \
      }

#define SiSSetupStyle(ls,hs) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + LINE_STYLE_0);	\
	 SIS_WQINDEX(1) = (uint32)(ls);					\
	 SIS_WQINDEX(2) = (uint32)(SIS_SPKC_HEADER + LINE_STYLE_1);	\
	 SIS_WQINDEX(3) = (uint32)(hs); 				\
	 SiSUpdateQueue \
	 SiSSetSwWP(ttt); \
      }

/* Trapezoid */

#define SiSSetupYHLR(y,h,left,right) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + TRAP_YH);	\
	 SIS_WQINDEX(1) = (uint32)(((y)<<16) | (h)); 		\
	 SIS_WQINDEX(2) = (uint32)(SIS_SPKC_HEADER + TRAP_LR);	\
	 SIS_WQINDEX(3) = (uint32)(((right)<<16) | (left));	\
	 SiSUpdateQueue \
	 SiSSetSwWP(ttt); \
      }


#define SiSSetupdLdR(dxL,dyL,fxR,dyR) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + TRAP_DL);	\
	 SIS_WQINDEX(1) = (uint32)(((dyL)<<16) | (dxL)); 	\
	 SIS_WQINDEX(2) = (uint32)(SIS_SPKC_HEADER + TRAP_DR);	\
	 SIS_WQINDEX(3) = (uint32)(((dyR)<<16) | (dxR)); 	\
	 SiSUpdateQueue \
	 SiSSetSwWP(ttt); \
      }

#define SiSSetupELER(eL,eR) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + TRAP_EL);	\
	 SIS_WQINDEX(1) = (uint32)(eL);	 			\
	 SIS_WQINDEX(2) = (uint32)(SIS_SPKC_HEADER + TRAP_ER);	\
	 SIS_WQINDEX(3) = (uint32)(eR); 			\
	 SiSUpdateQueue \
	 SiSSetSwWP(ttt); \
      }

/* (Constant) Alpha blended BitBlt (alpha = 8 bit) */

#define SiSSetupAlpha(alpha) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + ALPHA_ALPHA);	\
	 SIS_WQINDEX(1) = (uint32)(alpha);	 			\
	 SiSNILandUpdateSWQueue \
      }

#define SiSSetPattern(num, value) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(SIS_SPKC_HEADER + (PATTERN_REG + (num * 4)));	\
	 SIS_WQINDEX(1) = (uint32)(value); 						\
	 SiSNILandUpdateSWQueue \
      }

#define SiSSetupPatternRegBurst(pat1, pat2, pat3, pat4) \
      { \
	 uint32 ttt = SiSGetSwWP(); \
	 pointer tt = (char *)si.cmdQueueBase + ttt; \
	 SIS_WQINDEX(0) = (uint32)(pat1);		\
	 SIS_WQINDEX(1) = (uint32)(pat2);		\
	 SIS_WQINDEX(2) = (uint32)(pat3);		\
	 SIS_WQINDEX(3) = (uint32)(pat4);		\
	 SiSUpdateQueue \
	 SiSSetSwWP(ttt); \
      }


#endif





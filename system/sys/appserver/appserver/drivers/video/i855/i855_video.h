#ifndef _I855_VIDEO_H_
#define _I855_VIDEO_H_

/*
 * OCMD - Overlay Command Register
 */
#define MIRROR_MODE		(0x3<<17)
#define MIRROR_HORIZONTAL	(0x1<<17)
#define MIRROR_VERTICAL		(0x2<<17)
#define MIRROR_BOTH		(0x3<<17)
#define OV_BYTE_ORDER		(0x3<<14)
#define UV_SWAP			(0x1<<14)
#define Y_SWAP			(0x2<<14)
#define Y_AND_UV_SWAP		(0x3<<14)
#define SOURCE_FORMAT		(0xf<<10)
#define RGB_888			(0x1<<10)
#define	RGB_555			(0x2<<10)
#define	RGB_565			(0x3<<10)
#define	YUV_422			(0x8<<10)
#define	YUV_411			(0x9<<10)
#define	YUV_420			(0xc<<10)
#define	YUV_422_PLANAR		(0xd<<10)
#define	YUV_410			(0xe<<10)
#define TVSYNC_FLIP_PARITY	(0x1<<9)
#define TVSYNC_FLIP_ENABLE	(0x1<<7)
#define BUF_TYPE		(0x1<<5)
#define BUF_TYPE_FRAME		(0x0<<5)
#define BUF_TYPE_FIELD		(0x1<<5)
#define TEST_MODE		(0x1<<4)
#define BUFFER_SELECT		(0x3<<2)
#define BUFFER0			(0x0<<2)
#define BUFFER1			(0x1<<2)
#define FIELD_SELECT		(0x1<<1)
#define FIELD0			(0x0<<1)
#define FIELD1			(0x1<<1)
#define OVERLAY_ENABLE		0x1

/* OCONFIG register */
#define CC_OUT_8BIT		(0x1<<3)
#define OVERLAY_PIPE_MASK	(0x1<<18)		
#define OVERLAY_PIPE_A		(0x0<<18)		
#define OVERLAY_PIPE_B		(0x1<<18)		

/* DCLRKM register */
#define DEST_KEY_ENABLE		(0x1<<31)

/* Polyphase filter coefficients */
#define N_HORIZ_Y_TAPS		5
#define N_VERT_Y_TAPS		3
#define N_HORIZ_UV_TAPS		3
#define N_VERT_UV_TAPS		3
#define N_PHASES		17
#define MAX_TAPS		5

/* Filter cutoff frequency limits. */
#define MIN_CUTOFF_FREQ		1.0
#define MAX_CUTOFF_FREQ		3.0

#define RGB16ToColorKey(c) \
	(((c & 0xF800) << 8) | ((c & 0x07E0) << 5) | ((c & 0x001F) << 3))

#define RGB15ToColorKey(c) \
        (((c & 0x7c00) << 9) | ((c & 0x03E0) << 6) | ((c & 0x001F) << 3))



#define PFIT_CONTROLS 0x61230
#define PFIT_AUTOVSCALE_MASK 0x200
#define PFIT_ON_MASK 0x80000000
#define PFIT_AUTOSCALE_RATIO 0x61238
#define PFIT_PROGRAMMED_SCALE_RATIO 0x61234


struct i855VideoRegs_s {
   uint32 OBUF_0Y;
   uint32 OBUF_1Y;
   uint32 OBUF_0U;
   uint32 OBUF_0V;
   uint32 OBUF_1U;
   uint32 OBUF_1V;
   uint32 OSTRIDE;
   uint32 YRGB_VPH;
   uint32 UV_VPH;
   uint32 HORZ_PH;
   uint32 INIT_PHS;
   uint32 DWINPOS;
   uint32 DWINSZ;
   uint32 SWIDTH;
   uint32 SWIDTHSW;
   uint32 SHEIGHT;
   uint32 YRGBSCALE;
   uint32 UVSCALE;
   uint32 OCLRC0;
   uint32 OCLRC1;
   uint32 DCLRKV;
   uint32 DCLRKM;
   uint32 SCLRKVH;
   uint32 SCLRKVL;
   uint32 SCLRKEN;
   uint32 OCONFIG;
   uint32 OCMD;
   uint32 RESERVED1;			/* 0x6C */
   uint32 AWINPOS;
   uint32 AWINSZ;
   uint32 RESERVED2;			/* 0x78 */
   uint32 RESERVED3;			/* 0x7C */
   uint32 RESERVED4;			/* 0x80 */
   uint32 RESERVED5;			/* 0x84 */
   uint32 RESERVED6;			/* 0x88 */
   uint32 RESERVED7;			/* 0x8C */
   uint32 RESERVED8;			/* 0x90 */
   uint32 RESERVED9;			/* 0x94 */
   uint32 RESERVEDA;			/* 0x98 */
   uint32 RESERVEDB;			/* 0x9C */
   uint32 FASTHSCALE;			/* 0xA0 */
   uint32 UVSCALEV;			/* 0xA4 */

   uint32 RESERVEDC[(0x200 - 0xA8) / 4];		   /* 0xA8 - 0x1FC */
   uint16 Y_VCOEFS[N_VERT_Y_TAPS * N_PHASES];		   /* 0x200 */
   uint16 RESERVEDD[0x100 / 2 - N_VERT_Y_TAPS * N_PHASES];
   uint16 Y_HCOEFS[N_HORIZ_Y_TAPS * N_PHASES];		   /* 0x300 */
   uint16 RESERVEDE[0x200 / 2 - N_HORIZ_Y_TAPS * N_PHASES];
   uint16 UV_VCOEFS[N_VERT_UV_TAPS * N_PHASES];		   /* 0x500 */
   uint16 RESERVEDF[0x100 / 2 - N_VERT_UV_TAPS * N_PHASES];
   uint16 UV_HCOEFS[N_HORIZ_UV_TAPS * N_PHASES];	   /* 0x600 */
   uint16 RESERVEDG[0x100 / 2 - N_HORIZ_UV_TAPS * N_PHASES];
} __attribute__ ((packed));


typedef struct {
   uint8 sign;
   uint16 mantissa;
   uint8 exponent;
} coeffRec, *coeffPtr;

#endif


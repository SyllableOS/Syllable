#include	"gx00_crtc1.h"
#include	<stdio.h>

#include <atheos/isa_io.h>
#define outp(a,b) outb(b,a)
#define inp(a) inb(a)

struct	SModeParms
{
	int	iXRes;
	int	iYRes;
	int	iBPP;
	float	fRefreshRate;
};

struct	SHWParms
{
	UBYTE	CRTC0;	//htotal<0:7>
    UBYTE	CRTC1;	//hdispend<0:7>
    UBYTE 	CRTC2;	//hblkstr<0:7>
    UBYTE   CRTC3;	//hblkend<0:4>
					//bit 5-6 = hdispskew<0:1>
    UBYTE  	CRTC4;	//hsyncstr<0:7>
    UBYTE   CRTC5;	//bit 0-4 = hsyncend<0:4>
					//bit 5-6 = hsyncdel<0:1>
    				//bit 7 = hblkend<5>
    UBYTE   CRTC6;	//vtotal<0:7>
    UBYTE   CRTC7;	//bit 0 = vtotal<8>
					//bit 1 = vdispend<8>
					//bit 2 = vsyncstr<8>
					//bit 3 = vblkstr<8>
					//bit 4 = linecomp<8>
					//bit 5 = vtotal<9>
					//bit 6 = vdispend<9>
					//bit 7 = vsyncstr<9>
    UBYTE	CRTC8;	//bit 5-6 = bytepan<0:1>
    UBYTE   CRTC9;	//bit 0-4 = maxscan
    				//bit 5 = vblkstr<9>
					//bit 6 = linecomp<9>
					//bit 7 = conv2t4
    UBYTE   CRTCC;	//startadd<8:15>
    UBYTE   CRTCD;	//startadd<0:7>
    UBYTE   CRTC10;	//vsyncstr<0:7>
    UBYTE   CRTC11;	//bit 0-3 = vsyncend<0:3>
					//bit 5 = disable vblank int
    UBYTE   CRTC12;	//vdispend<0:7>
    UBYTE   CRTC13;	//offset<8:0>
    UBYTE   CRTC14;	//bit 6 = dword
    UBYTE   CRTC15;	//vblkstr<0:7>
    UBYTE   CRTC16;	//vblkend<0:7>
    UBYTE   CRTC17;	//bit 0 = cms
    				//bit 1 = selrowscan
		    		//bit 2 = hsyncsel<0>
					//bit 6 = wbmode
					//bit 7 = allow syncs to run
    UBYTE   CRTC18;	//linecomp<0:7>
    UBYTE   CRTCEXT0;	//bit 0-3 = startadd<16:20>
						//bit 4-5 = offset<8:9>
    UBYTE   CRTCEXT1;	//bit 0 = htotal<8>
						//bit 1 = hblkstr<8>
						//bit 2 = hsyncstr<8>
						//bit 6 = hblkend<6>
    UBYTE   CRTCEXT2;	//bit 0-1 = vtotal<10:11>
						//bit 2 = vdispend<10>
						//bit 3-4 = vblkstr<10:11>
						//bit 5-6 = vsyncstr<10:11>
						//bit 7 = linecomp<10>
    UBYTE   CRTCEXT3;	//bit 0-2 = scale<0:2>
    UBYTE   SEQ1;		//bit 3 = dotclkrt
    UBYTE   XMULCTRL;	//bit 0-2 = depth
    UBYTE   XMISCCTRL;
    UBYTE   XZOOMCTRL;	//bit 0-1 = hzoom
    UBYTE   XPIXPLLM;	//bit 0-6
    UBYTE   XPIXPLLN;	//bit 0-6
    UBYTE   XPIXPLLP;	//bit 0-6
};

static long	imuldiv( long a, long b, long c )
{
	//	This function multiplies a and b, and divides the 64 bit result by c
/*
	_asm {
		mov		eax,a
		imul	b
		idiv	c
		mov		a,eax
	}

	return a;
*/
	return (int64(a)*int64(b))/int64(c);
}

bool	CGx00CRTC1::CalcMode( SHWParms *pDest, const SModeParms *pParms )
{
	if( pParms->iXRes==0 || pParms->iYRes==0 )
		return false;

	//
	//	Make this a possible resolution if x or y is below limits
	//	by using the hardware zoom factors.
	//	These limits are completely arbitrary, the doc is vague
	//

	int	iXRes=short(pParms->iXRes);
	int	iXShift=0;
	while( iXRes<512 )
	{
		iXShift+=1;
		iXRes<<=1;
	}

	int	iYRes=short(pParms->iYRes);
	int	iYShift=0;
	while( iYRes<350 )
	{
		iYShift+=1;
		iYRes<<=1;
	}

	if( iXShift>2 || iYShift>2 )
		return false;

	if( iXShift==2 )
		iXShift=3;

	if( iYShift==2 )
		iYShift=3;

	if( iXRes>2048 || iYRes>1536 )
		return false;

	//
	//	The usual h and v red tape, blah blah
	//

#if 0
	int	htotal=(iXRes>>3)+20-5;
#else
	int	htotal=(iXRes>>3)+20;
	if( iXRes>=1024 )
		htotal+=htotal>>3;
	if( iXRes>=1280 )
		htotal+=htotal>>4;
	htotal-=5;
#endif
	int hdispend=(iXRes>>3)-1;
	int	hblkstr=hdispend;
	int hblkend=htotal+5-1;
	int hsyncstr=(iXRes>>3)+1;
	int hsyncend=(hsyncstr+12)&0x1F;

	int	vtotal=(iYRes)+45-2;
	int	vdispend=iYRes-1;
	int	vblkstr=iYRes-1;
	int	vblkend=(vblkstr-1+46)&0xFF;
	int	vsyncstr=iYRes+9;
	int	vsyncend=(vsyncstr+2)&0x0F;
	int	linecomp=1023;

	//
	//	Set up bit depth dependant values
	//

	int	scale;
	int	depth;
	int	startaddfactor;
	int	vga8bit;

	switch( pParms->iBPP )
	{
		case	8:
			scale=0;
			depth=0;
			startaddfactor=3;
			vga8bit=0;
			break;
		case	15:
			scale=1;	//doc says 0
			depth=1;
			startaddfactor=3;
			vga8bit=8;
			break;
		case	16:
			scale=1;	//doc says 0
			depth=2;
			startaddfactor=3;
			vga8bit=8;
			break;
		case	24:
			scale=2;
			depth=3;
			startaddfactor=3;
			vga8bit=8;
			break;
		case	32:
			scale=3;	//doc says 1
			depth=7;
			startaddfactor=3;
			vga8bit=8;
			break;
	}

	//
	//	Convert the desired refreshrate to a pixelclock, incidentally
	//	this is also a VESA pixelclock, suitable for GTF calculations
	//

	long	iNormalPixelClock;
	iNormalPixelClock=((htotal+5)<<3)*(vtotal+2)*pParms->fRefreshRate;

	printf( "Desired pixelclock is %ld\n", iNormalPixelClock );

	//
	//	Now, determine values for the clock circuitry that will produce
	//	a pixelclock close enough to what we want
	//

	long	iBestPixelClock=0;
	short	iBestN, iBestM, iBestP, iBestS;
	short	iPixN, iPixM, iPixP;
	short	i;
	for( iPixM=1; iPixM<=31; iPixM+=1 )
	{
		for( i=0; i<=3; i+=1 )
		{
			iPixP=short((1<<i)-1);

			long	n;
			n=(iPixP+1)*(iPixM+1);
			n=imuldiv(n,iNormalPixelClock,27000000);
			iPixN=short(n-1);

			if( iPixN>=7 && iPixN<=127 )
			{
				long	iVCO;

				iVCO=imuldiv(27000000,iPixN+1,iPixM+1);
				n=imuldiv(1,iVCO,(iPixP+1));

				long	best,d;

				best=iBestPixelClock-iNormalPixelClock;
				d=n-iNormalPixelClock;

				if( best<0 )
					best=-best;
				if( d<0 )
					d=-d;
				if( d<best )
				{
					if( 50000000<=iVCO && iVCO<110000000 )
						iBestS=0;
					else if( 110000000<=iVCO && iVCO<170000000 )
						iBestS=1;
					else if( 170000000<=iVCO && iVCO<240000000 )
						iBestS=2;
					else if( 240000000<=iVCO && iVCO<310000000 )
						iBestS=3;
					else
						continue;

					iBestPixelClock=n;
					iBestN=iPixN;
					iBestM=iPixM;
					iBestP=iPixP;
				}
			}
		}
	}

	printf( "N,M,P,S=%d,%d,%d,%d\n", iBestN, iBestM, iBestP, iBestS );
	printf( "Clock=%ld\n", iBestPixelClock );


	//
	//	Now all we have to do is scatter all the bits out to the registers
	//

	short	bytedepth=short((pParms->iBPP+1)>>3);
	short	offset=short((pParms->iXRes*bytedepth+15)>>4);

	pDest->CRTC0=UBYTE(htotal);
	pDest->CRTC1=UBYTE(hdispend);
	pDest->CRTC2=UBYTE(hblkstr);
	pDest->CRTC3=UBYTE(hblkend&0x1F);
	pDest->CRTC4=UBYTE(hsyncstr);
	pDest->CRTC5=UBYTE((hsyncend&0x1F)|((hblkend&0x20)<<2));
	pDest->CRTC6=UBYTE(vtotal&0xFF);
	pDest->CRTC7=UBYTE(((vtotal&0x100)>>8)|((vtotal&0x200)>>4)|((vdispend&0x100)>>7)|((vdispend&0x200)>>3)|((vblkstr&0x100)>>5)|((vsyncstr&0x100)>>6)|((vsyncstr&0x200)>>2)|((linecomp&0x100)>>4));
	pDest->CRTC8=0;
	pDest->CRTC9=UBYTE(((vblkstr&0x200)>>4)|((linecomp&0x200)>>3)|(iYShift&0x1F));
	pDest->CRTCC=0;
	pDest->CRTCD=0;
	pDest->CRTC10=UBYTE(vsyncstr);
	pDest->CRTC11=UBYTE((vsyncend&0x0F)|0x20);
	pDest->CRTC12=UBYTE(vdispend);
	pDest->CRTC13=UBYTE(offset);
	pDest->CRTC14=0;
	pDest->CRTC15=UBYTE(vblkstr);
	pDest->CRTC16=UBYTE(vblkend);
	pDest->CRTC17=0xC3;
	pDest->CRTC18=UBYTE(linecomp);
	pDest->CRTCEXT0=UBYTE((offset&0x300)>>4);
	pDest->CRTCEXT1=UBYTE(((htotal&0x100)>>8)|((hblkstr&0x100)>>7)|((hsyncstr&0x100)>>6)|(hblkend&0x40));
	pDest->CRTCEXT2=UBYTE(((vtotal&0xC00)>>10)|((vdispend&0x400)>>8)|((vblkstr&0xC00)>>7)|((vsyncstr&0xC00)>>5)|((linecomp&0x400)>>3));
	pDest->CRTCEXT3=UBYTE(0x80|(scale&7));
	pDest->SEQ1=0x21;
	pDest->XMULCTRL=UBYTE(depth&7);
	pDest->XMISCCTRL=UBYTE(vga8bit|0x17);
	pDest->XZOOMCTRL=UBYTE(iXShift&3);
	pDest->XPIXPLLM=UBYTE(iBestM);
	pDest->XPIXPLLN=UBYTE(iBestN);
	pDest->XPIXPLLP=UBYTE(iBestP);

	return true;
}

bool	CGx00CRTC1::SetMode( int iXRes, int iYRes, int iBPP, float fRefreshRate )
{
	SModeParms	parms;
	parms.iXRes=iXRes;
	parms.iYRes=iYRes;
	parms.iBPP=iBPP;
	parms.fRefreshRate=fRefreshRate;

	SHWParms	hw;
	bool	r=CalcMode( &hw, &parms );

	if( r )
	{
		WaitVBlank();
		Display_Blank();
		SetMGAMode();
		CRTC_WriteEnable();

		//	Set up the registers calculated by CalcMode()

		outp( 0x3C2, 0xEF );
		GCTL_Write( 0x06, 0x05 );
		CRTC_Write( 0x00, hw.CRTC0 );
		CRTC_Write( 0x01, hw.CRTC1 );
		CRTC_Write( 0x02, hw.CRTC2 );
		CRTC_Write( 0x03, hw.CRTC3 );
		CRTC_Write( 0x04, hw.CRTC4 );
		CRTC_Write( 0x05, hw.CRTC5 );
		CRTC_Write( 0x06, hw.CRTC6 );
		CRTC_Write( 0x07, hw.CRTC7 );
		CRTC_Write( 0x08, hw.CRTC8 );
		CRTC_Write( 0x09, hw.CRTC9 );
		CRTC_Write( 0x0C, hw.CRTCC );
		CRTC_Write( 0x0D, hw.CRTCD );
		CRTC_Write( 0x10, hw.CRTC10 );
		CRTC_Write( 0x11, hw.CRTC11 );
		CRTC_Write( 0x12, hw.CRTC12 );
		CRTC_Write( 0x13, hw.CRTC13 );
		CRTC_Write( 0x14, hw.CRTC14 );
		CRTC_Write( 0x15, hw.CRTC15 );
		CRTC_Write( 0x16, hw.CRTC16 );
		CRTC_Write( 0x17, hw.CRTC17 );
		CRTC_Write( 0x18, hw.CRTC18 );
		CRTCEXT_Write( 0x00, hw.CRTCEXT0 );
		CRTCEXT_Write( 0x01, hw.CRTCEXT1 );
		CRTCEXT_Write( 0x02, hw.CRTCEXT2 );
		CRTCEXT_Write( 0x03, hw.CRTCEXT3 );
		CRTCEXT_Write( 0x04, 0 );
		SEQ_Write( 0x01, hw.SEQ1 );
		SEQ_Write( 0x00, 0x03 );
		SEQ_Write( 0x02, 0x0F );
		SEQ_Write( 0x03, 0x00 );
		SEQ_Write( 0x04, 0x0E );
		SEQ_Write( 0x04, 0x0E );
		X_Write( 0x19, hw.XMULCTRL );
		X_Write( 0x1E, hw.XMISCCTRL );
		X_Write( 0x38, hw.XZOOMCTRL );
		X_Write( 0x4C, hw.XPIXPLLM );
		X_Write( 0x4D, hw.XPIXPLLN );
		X_Write( 0x4E, hw.XPIXPLLP );

		//	Set up the VGA LUT. We could implement a gamma for
		//	15, 16, 24 and 32 bit if we wanted because they use the LUT

		switch( iBPP )
		{
			case 4:
				//	not implemented
				break;
			case 8:
				//	nothing to do
				break;
			case 15:
			{
				outp( 0x3C8, 0 );
				for( int i=0; i<256; i+=1 )
				{
					outp( 0x3C9, i<<3 );
					outp( 0x3C9, i<<3 );
					outp( 0x3C9, i<<3 );
				}
				break;
			}
			case 16:
			{
				outp( 0x3C8, 0 );
				for( int i=0; i<256; i+=1 )
				{
					outp( 0x3C9, i<<3 );
					outp( 0x3C9, i<<2 );
					outp( 0x3C9, i<<3 );
				}
				break;
			}
			case 24:
			case 32:
			{
				outp( 0x3C8, 0 );
				for( int i=0; i<256; i+=1 )
				{
					outp( 0x3C9, i );
					outp( 0x3C9, i );
					outp( 0x3C9, i );
				}
				break;
			}
		}

// 		m_Surface.m_iXRes=iXRes;
// 		m_Surface.m_iYRes=iYRes;
// 		m_Surface.m_iPitch=iXRes*((iBPP+7)>>3);
//		m_Surface.m_iBPP=iBPP;
//		m_Surface.m_pMemory=s_pMemory;

//		ClearMemory();
		WaitVBlank();
		Display_UnBlank();
	}

	return r;
}
// Copyright 1999 Jesper Hansen. Free for use under the Gnu Public License
//-----------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
//-------------------------------------
#include <gui/bitmap.h>
//-------------------------------------
#include "cpiacam.h"
//-----------------------------------------------------------------------------

#define CLIP(x) ((((x)>0xffffff)?0xff0000:(((x)<=0xffff)?0:(x)&0xff0000))>>16)

status_t CPiACam::ProcessFrame_YUV422_RGB32_NoComp(
	uint8 *src, size_t srcsize,
	uint8 *dst, size_t bpl,
	uint roileft, uint roiright,
	uint roitop, uint roibottom,
	uint decimation )
{
    for( uint iy=roitop; iy<roibottom; iy+=decimation )
    {
	if( srcsize < 2 )
	{
	    fprintf(stderr,"CPiACam::ProcessFrame_YUV422_RGB32_NoComp: srcsize:%d<2\n", (int)srcsize );
	    return -1;
	}
	srcsize -= 2;

	uint linesize = *src++;
	linesize |= (*src++)<<8;
	if( linesize > srcsize )
	{
	    fprintf(stderr,"CPiACam::ProcessFrame_YUV422_RGB32_NoComp: linesize:%d>srcsize:%d\n", (int)linesize, (int)srcsize );
	    return -1;
	}
	srcsize -= linesize;

	if( linesize-1 != (roiright-roileft)*2 )
	{
	    fprintf(stderr,"CPiACam::ProcessFrame_YUV422_RGB32_NoComp: linesize-1:%d!=(roiright-roileft)*2:%d\n", (int)linesize-1, (int)(roiright-roileft)*2 );
	    return -1;
	}

	uint32 *dstline = (uint32*)(dst+bpl*iy);

	for( uint ix=roileft; ix<roiright; ix+=2 )
	{
	      // ?RGB
	    int y1 = *src++ - 16;
	    int u =  *src++ -128;
	    int y2 = *src++ - 16;
	    int v =  *src++ -128;

	    int r = 104635 * v;
	    int g = -25690 * u + -53294 * v;
	    int b = 132278 * u;
	    y1 *= 76310;
	    y2 *= 76310;

#if BYTE_ORDER == __LITTLE_ENDIAN
	    uint32 col1 = CLIP(r+y1)<<16 | CLIP(g+y1)<<8 | CLIP(b+y1);
	    uint32 col2 = CLIP(r+y2)<<16 | CLIP(g+y2)<<8 | CLIP(b+y2);
#else
	    uint32 col1 = CLIP(r+y1)<<8 | CLIP(g+y1)<<16 | CLIP(b+y1)<<24;
	    uint32 col2 = CLIP(r+y2)<<8 | CLIP(g+y2)<<16 | CLIP(b+y2)<<24;
#endif
	    dstline[ix]   = col1;
	    dstline[ix+1] = col2;
	    if( decimation==2 )
	    {
		dstline[ix+bpl/4]   = col1;
		dstline[ix+1+bpl/4] = col2;
	    }
	}
	src += 1; // skip eol marker
    }
    return 0;
}

status_t CPiACam::ProcessFrame_YUV422_RGB32_Comp(
    uint8 *src, size_t srcsize,
    uint8 *dst, size_t bpl,
    uint roileft, uint roiright,
    uint roitop, uint roibottom,
    uint decimation )
{
    if( roileft!=0 || roitop!= 0 )
	printf( "%d %d\n", roileft, roitop );
    for( uint iy=roitop; iy<roibottom; iy+=decimation )
    {
	if( srcsize < 2 ) return -1;
	srcsize -= 2;

	uint linesize = *src++;
	linesize |= (*src++)<<8;
	if( linesize > srcsize ) return -1;
	srcsize -= linesize;

	uint32 *dstline = (uint32*)(dst+bpl*iy);
	uint32 *dstlinenext = (uint32*)(((uint8*)dstline)+bpl);
	uint ix = roileft;
	while( ix<roiright && linesize>0 )
	{
	    uint8 cc = *src;
	    if( (cc&1) )
	    {
		int count = cc>>1;
		ix += count;
		src++;
		linesize--;
	    }
	    else
	    {
		int y1 = (*src++) - 16;
		int u = (*src++)  - 128;
		int y2 = (*src++) - 16;
		int v = (*src++)  - 128;
		linesize -= 4;

		int r = 104635 * v;
		int g = -25690 * u + -53294 * v;
		int b = 132278 * u;
		y1 *= 76310;
		y2 *= 76310;


#if BYTE_ORDER == __LITTLE_ENDIAN			
		uint32 col1 = CLIP(r+y1)<<16 | CLIP(g+y1)<<8 | CLIP(b+y1);
		uint32 col2 = CLIP(r+y2)<<16 | CLIP(g+y2)<<8 | CLIP(b+y2);
#else
		uint32 col1 = CLIP(r+y1)<<8 | CLIP(g+y1)<<16 | CLIP(b+y1)<<24;
		uint32 col2 = CLIP(r+y2)<<8 | CLIP(g+y2)<<16 | CLIP(b+y2)<<24;
#endif
		dstline[ix] = col1;
		dstline[ix+1] = col2;
		if( decimation == 2 )
		{
		    dstlinenext[ix]   = col1;
		    dstlinenext[ix+1] = col2;
		}
		ix += 2;
	    }
	}
	if( ix != roiright ) return -1;
		
	src += 1; // skip eol marker
	if( --linesize != 0 ) return -1;
    }
	
    return 0;
}

//-----------------------------------------------------------------------------

status_t CPiACam::ProcessFrame_YUV422_YUV422_NoComp(
    uint8 *src, size_t srcsize,
    uint8 *dst, size_t bpl,
    uint roileft, uint roiright,
    uint roitop, uint roibottom,
    uint decimation )
{
    for( uint iy=roitop; iy<roibottom; iy+=decimation )
    {
	if( srcsize < 2 ) return -1;
	srcsize -= 2;

	uint linesize = *src++;
	linesize |= (*src++)<<8;
	if( linesize > srcsize ) return -1;
	srcsize -= linesize;

	if( linesize-1 != (roiright-roileft)*2 ) return -1;

	uint8 *dstline = (uint8*)(dst+bpl*iy);

	memcpy( dstline+roileft*2, src, (roiright-roileft)*2 );
	if( decimation==2 )
	    memcpy( dstline+roileft*2+bpl, src, (roiright-roileft)*2 );

	src += (roiright-roileft)*2 + 1; // skip eol marker
    }

    return 0;
}


status_t CPiACam::ProcessFrame_YUV422_YUV422_Comp(
    uint8 *src, size_t srcsize,
    uint8 *dst, size_t bpl,
    uint roileft, uint roiright,
    uint roitop, uint roibottom,
    uint decimation )
{
    for( uint iy=roitop; iy<roibottom; iy+=decimation )
    {
	if( srcsize < 2 ) return -1;
	srcsize -= 2;

	uint linesize = *src++;
	linesize |= (*src++)<<8;
	if( linesize > srcsize ) return -1;
	srcsize -= linesize;

	uint16 *dstline = (uint16*)(dst+bpl*iy);
	uint ix = roileft;
	while( ix<roiright && linesize>0 )
	{
	    uint8 cc = *src;
	    if( (cc&1) )
	    {
		int count = cc>>1;
		ix += count;
		src++;
		linesize--;
	    }
	    else
	    {
		uint32 y1uy2v = *((uint32*)src);
		src += 4;
		linesize -= 4;
				
		*((uint32*)(dstline+ix)) = y1uy2v;
		if( decimation == 2 )
		    *((uint32*)((uint8*)(dstline+ix)+bpl)) = y1uy2v;
		ix += 2;
	    }
	}
	if( ix != roiright ) return -1;
		
	src += 1; // skip eol marker
	if( --linesize != 0 ) return -1;
    }
	
    return 0;
}

//-----------------------------------------------------------------------------

#if 0 // Not up to date 
		// YUV 4:2:0
		if( !compression )
		{
			for( int iy=roitop; iy<roibottom; iy+=2 )
			{
				uint32 *bitmapline1 = (uint32*)(bitmapdata+bytesperline*iy);
				uint32 *bitmapline2 = (uint32*)(bitmapdata+bytesperline*(iy+1));
	
				cameradata += 2; // skip bytecount
	
				for( int ix=roileft; ix<roiright; ix+=2 )
				{
					// ?RGB
					int y1 = cameradata[(ix-roileft)*2] - 16;
					int u =  cameradata[(ix-roileft)*2+1]- 128;
					int y2 = cameradata[(ix-roileft)*2+2] - 16;
					int v =  cameradata[(ix-roileft)*2+3] -128;
					int y3 = cameradata[(ix-roileft)+((roiright-roileft)*2+3)] - 16;
					int y4 = cameradata[(ix-roileft)+1+((roiright-roileft)*2+3)] - 16;
	
					int r = 104635 * v;
					int g = -25690 * u + -53294 * v;
					int b = 132278 * u;
					y1 *= 76310;
					y2 *= 76310;
					y3 *= 76310;
					y4 *= 76310;
#if BYTE_ORDER == __LITTLE_ENDIAN			
					bitmapline1[ix]   = CLIP(r+y1)<<16 | CLIP(g+y1)<<8 | CLIP(b+y1);
					bitmapline1[ix+1] = CLIP(r+y2)<<16 | CLIP(g+y2)<<8 | CLIP(b+y2);
					bitmapline2[ix]   = CLIP(r+y3)<<16 | CLIP(g+y3)<<8 | CLIP(b+y3);
					bitmapline2[ix+1] = CLIP(r+y4)<<16 | CLIP(g+y4)<<8 | CLIP(b+y4);
#else
					bitmapline1[ix]   = CLIP(r+y1)<<8 | CLIP(g+y1)<<16 | CLIP(b+y1)<<24;
					bitmapline1[ix+1] = CLIP(r+y2)<<8 | CLIP(g+y2)<<16 | CLIP(b+y2)<<24;
					bitmapline2[ix]   = CLIP(r+y3)<<8 | CLIP(g+y3)<<16 | CLIP(b+y3)<<24;
					bitmapline2[ix+1] = CLIP(r+y4)<<8 | CLIP(g+y4)<<16 | CLIP(b+y4)<<24;
#endif
				}
				
				cameradata += (roiright-roileft)*2; // skip data
				cameradata += 1; // skip eol marker
	
				cameradata += 2; // skip bytecount
				cameradata += (roiright-roileft); // skip data
				cameradata += 1; // skip eol marker
			}
		}
		else
		{
			// compressed YUV 4:2:0
			assert( 0 );
		}
#endif
//		assert( 0 );
//	}

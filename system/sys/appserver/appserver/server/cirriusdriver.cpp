
/*
 *  The AtheOS application server
 *  Copyright (C) 1999  Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	<string.h>
#include	<stdlib.h>
#include	<stdio.h>

#include	<exec/types.h>
#include	<exec/ioports.h>
#include	<exec/interrup.h>
#include	<exec/execbase.h>
#include	<alist/alist.hpp>
#include	<alist/alist.inl>

#include	<devices/display/ddriver.hpp>
#include	<devices/display/sfont.hpp>

#include	<gui/bitmap.hpp>
#include	<gui/modeinfo.hpp>

area_id g_nFrameBufArea = -1;


long v_blit( long x1,		// top-left point of the source
	long y1,		//
	long x2,		// top-left point of the destination
	long y2,		//
	long width,		// size of the rect to move (from border included to
	long height );		// opposite border included).


#define		BLT_WIDTH_LOW		0x20
#define		BLT_WIDTH_HIG		0x21

#define		BLT_HEIGHT_LOW	0x22
#define		BLT_HEIGHT_HIG	0x23

#define		BLT_DPITCH_HIG	0x25
#define		BLT_DPITCH_LOW	0x24

#define		BLT_SPITCH_HIG	0x27
#define		BLT_SPITCH_LOW	0x26

#define		BLT_DST_LOW		0x28
#define		BLT_DST_MID		0x29
#define		BLT_DST_HIG		0x2a

#define		BLT_SRC_LOW		0x2c
#define		BLT_SRC_MID		0x2d
#define		BLT_SRC_HIG		0x2e

#define		BLT_MODE			0x30
#define		BLT_START			0x31
#define		BLT_RASTER_OP	0x32


int S3Blit( int x1, int y1, int x2, int y2, int width, int height );

void S3FillRect_32( long x1, long y1, long x2, long y2, uint color );

long S3DrawLine_32( long x1, long x2, long y1, long y2, uint color, bool useClip, short clipLeft, short clipTop, short clipRight, short clipBottom );
void InitS3( void );
void EnableS3( void );

static inline void WriteBltReg( int nIndex, int nVal )
{
	outw_p( ( nVal << 8 ) | nIndex, 0x3ce );
//      outb_p( nIndex, 0x3ce );
//      outb_p( nVal, 0x3cf );
}

static inline int ReadBltReg( int nIndex )
{
	outb_p( nIndex, 0x3ce );
	return ( inb_p( 0x3cf ) );
}

static inline void WaitBlit()
{
	while( ReadBltReg( BLT_START ) & 0x08 );
}

static inline void SetBltSrc( uint32 nAddress )
{
	WriteBltReg( BLT_SRC_LOW, nAddress & 0xff );
	WriteBltReg( BLT_SRC_MID, ( nAddress >> 8 ) & 0xff );
	WriteBltReg( BLT_SRC_HIG, ( nAddress >> 16 ) & 0xff );
}

static inline void SetBltDst( uint32 nAddress )
{
	WriteBltReg( BLT_DST_LOW, nAddress & 0xff );
	WriteBltReg( BLT_DST_MID, ( nAddress >> 8 ) & 0xff );
	WriteBltReg( BLT_DST_HIG, ( nAddress >> 16 ) & 0xff );
}

static inline void SetBltWidth( uint32 nWidth )
{
	WriteBltReg( BLT_WIDTH_LOW, nWidth & 0xff );
	WriteBltReg( BLT_WIDTH_HIG, ( nWidth >> 8 ) & 0xff );
}

static inline void SetBltHeight( uint32 nHeight )
{
	WriteBltReg( BLT_HEIGHT_LOW, nHeight & 0xff );
	WriteBltReg( BLT_HEIGHT_HIG, ( nHeight >> 8 ) & 0xff );
}

static inline void SetSrcPitch( int nPitch )
{
	WriteBltReg( BLT_SPITCH_LOW, nPitch & 0xff );
	WriteBltReg( BLT_SPITCH_HIG, ( nPitch >> 8 ) & 0xff );
}

static inline void SetDstPitch( int nPitch )
{
	WriteBltReg( BLT_DPITCH_LOW, nPitch & 0xff );
	WriteBltReg( BLT_DPITCH_HIG, ( nPitch >> 8 ) & 0xff );
}


DispMode_c::operator	 ModeInfo_s()
{
	ModeInfo_s sInfo;

	strcpy( sInfo.mi_zName, m_zName );	/* mode description                                                     */
	sInfo.mi_nIndex = m_nIndex;
	sInfo.mi_nRows = m_nRows;	/* number of visible rows                               */
	sInfo.mi_nScanLines = m_nScanLines;	/* number of visible scanlines  */
	sInfo.mi_nBytesPerLine = m_nBytesPerLine;	/* number of bytes per scan line (use this when calculating modulos!!)  */
	sInfo.mi_nBitsPerPixel = m_nBitsPerPixel;	/* number of bits per pixel                     */
	sInfo.mi_nRedBits = m_nRedBits;	/* bits on read cannon                                  */
	sInfo.mi_nGreenBits = m_nGreenBits;	/* bits on green cannon                                 */
	sInfo.mi_nBlueBits = m_nBlueBits;	/* bits on blue cannon                                  */
	sInfo.mi_nRedPos = m_nRedPos;	/* first bit in red field                               */
	sInfo.mi_nGreenPos = m_nGreenPos;	/* first bit in green field                     */
	sInfo.mi_nBluePos = m_nBluePos;	/* first bit in blue field                      */
	sInfo.mi_nPlanes = m_nPlanes;	/* number of bit planes                                 */
	sInfo.mi_nPaletteSize = m_nPaletteSize;	/* number of entries in palette */
	sInfo.mi_nFlags = m_nFlags;
	sInfo.mi_pFrameBuffer = m_pFrameBuffer;
	sInfo.mi_nFrameBufferSize = m_nFrameBufferSize;
	return ( sInfo );
}

VesaDriver_c::VesaDriver_c()
{
	m_pcMouse = NULL;
}

VesaDriver_c::~VesaDriver_c()
{
}

void VesaDriver_c::GetVesaModeInfo( struct VESA_Mode_Info *psVesaModeInfo, uint32 nModeNr )
{
	struct RMREGS rm;

	memset( &rm, 0, sizeof( struct RMREGS ) );

	rm.EAX = 0x4f01;
	rm.ECX = nModeNr;
	rm.EDI = CUL( psVesaModeInfo ) & 0x0f;
	rm.ES = CUL( psVesaModeInfo ) >> 4;

	realint( 0x10, &rm );
}

/******************************************************************************
 ******************************************************************************/

void VesaDriver_c::GetVesaInfo( struct Vesa_Info *psVesaBlk )
{
	struct RMREGS rm;

	memset( &rm, 0, sizeof( struct RMREGS ) );

	rm.EAX = 0x4f00;
	rm.ECX = 0;
	rm.EDI = CUL( psVesaBlk ) & 0x0f;
	rm.ES = CUL( psVesaBlk ) >> 4;

	Forbid();
	realint( 0x10, &rm );
	Permit();
}

bool VesaDriver_c::InitModes( void )
{
	struct Vesa_Info *MemPtr;
	struct VESA_Mode_Info *VMI;
	uint16 *ModePtr;
	DispMode_c *pcMode;
	int i = 0;
	int ModeNr = 0;

	pcMode = new DispMode_c;

	pcMode->m_nIndex = ModeNr++;
	pcMode->m_nRows = 320;
	pcMode->m_nScanLines = 200;
	pcMode->m_nBytesPerLine = 320;
	pcMode->m_nBitsPerPixel = 8;

	pcMode->m_nRedBits = 0;
	pcMode->m_nRedPos = 0;
	pcMode->m_nGreenBits = 0;
	pcMode->m_nGreenPos = 0;
	pcMode->m_nBlueBits = 0;
	pcMode->m_nBluePos = 0;
	pcMode->m_nPlanes = 1;
	pcMode->m_nPaletteSize = 256;
	pcMode->m_nFrameBufferSize = pcMode->m_nScanLines * pcMode->m_nBytesPerLine;

	if( pcMode->m_nBitsPerPixel <= 8 )
	{
		SETBITS( pcMode->m_nFlags, MIF_PALETTE );
	}

	pcMode->m_pPhysDispAddr = ( uint8 * )0xa0000;
	pcMode->m_nVesaMode = 0x13;

	sprintf( pcMode->m_zName, "VBE2:%dx%d %d BPP", pcMode->m_nRows, pcMode->m_nScanLines, pcMode->m_nBitsPerPixel );

	m_cModeList.AddTail( pcMode );

	if( MemPtr = ( Vesa_Info_s * ) AllocVec( sizeof( Vesa_Info_s ) + 256, MEMF_REAL | MEMF_CLEAR ) )
	{
		strcpy( MemPtr->VesaSignature, "VBE2" );

		GetVesaInfo( MemPtr );

		if( VMI = ( VESA_Mode_Info_s * ) AllocVec( sizeof( VESA_Mode_Info_s ) + 256, MEMF_REAL ) )
		{
			ModePtr = ( uint16 * )( ( ( CUL( MemPtr->VideoModePtr ) & 0xffff0000 ) >> 12 ) + ( CUL( MemPtr->VideoModePtr ) & 0x0ffff ) );

			while( ModePtr[i] != 0xffff )
			{
				bzero( VMI, sizeof( struct VESA_Mode_Info ) );

				GetVesaModeInfo( VMI, ModePtr[i] );

				if( VMI->PhysBasePtr )	/* We Have FlatMem       */
				{
					if( VMI->BitsPerPixel >= 8 )
					{
						if( VMI->NumberOfPlanes == 1 )
						{
							pcMode = new DispMode_c;

							pcMode->m_nIndex = ModeNr++;

							pcMode->m_nRows = VMI->XResolution;
							pcMode->m_nScanLines = VMI->YResolution;
							pcMode->m_nBytesPerLine = VMI->BytesPerScanLine;
							pcMode->m_nBitsPerPixel = VMI->BitsPerPixel;

							pcMode->m_nRedBits = VMI->RedMaskSize;
							pcMode->m_nRedPos = VMI->RedFieldPosition;
							pcMode->m_nGreenBits = VMI->GreenMaskSize;
							pcMode->m_nGreenPos = VMI->GreenFieldPosition;
							pcMode->m_nBlueBits = VMI->BlueMaskSize;
							pcMode->m_nBluePos = VMI->BlueFieldPosition;
							pcMode->m_nPlanes = VMI->NumberOfPlanes;
							pcMode->m_nPaletteSize = 256;
							pcMode->m_nFrameBufferSize = pcMode->m_nScanLines * pcMode->m_nBytesPerLine;

							if( pcMode->m_nBitsPerPixel <= 8 )
							{
								SETBITS( pcMode->m_nFlags, MIF_PALETTE );
							}

							pcMode->m_pPhysDispAddr = ( uint8 * )VMI->PhysBasePtr;
							pcMode->m_nVesaMode = ModePtr[i] | 0x4000;

/*
							dbprintf( "VBE2:%dx%d %d BPP (ADR = %lx)\n", pcMode->m_nRows, pcMode->m_nScanLines, pcMode->m_nBitsPerPixel, pcMode->m_nBytesPerLine );
*/
							sprintf( pcMode->m_zName, "VBE2:%dx%d %d BPP", pcMode->m_nRows, pcMode->m_nScanLines, pcMode->m_nBitsPerPixel );

							m_cModeList.AddTail( pcMode );
						}
					}
				}
				i++;
			}
			FreeVec( VMI );
		}
		FreeVec( MemPtr );

		return ( TRUE );
	}

	return ( FALSE );
}

void EnableVirge( void );

bool VesaDriver_c::Open( void )
{
	EnableS3();
//      EnableVirge();



	if( InitModes() )
	{
		m_nFrameBufferSize = 1024 * 1024 * 4;

		g_nFrameBufArea = create_area( "frame_buffer", &m_pFrameBuffer, AREA_ANY_ADDRESS, m_nFrameBufferSize, AREA_FULL_ACCESS, AREA_NO_LOCK );

//              g_nFrameBufArea = CreateArea( "frame_buffer", NULL, m_nFrameBufferSize, 0, 0 );

//              m_pFrameBuffer = GetAreaAddress( g_nFrameBufArea );

/*		if ( m_pFrameBuffer = (uint8*) AllocVec( m_nFrameBufferSize, MEMF_PUBLIC  | MEMF_PAGEALLIGN ) ) */
		{
//                      SysBase->ex_pDisplayAddr = m_pFrameBuffer;

//                      MemMapToDev( m_pFrameBuffer, GetModeDesc(0)->m_pPhysDispAddr, (m_nFrameBufferSize + MEM_PAGEMASK) & ~MEM_PAGEMASK, MPROT_USER | MPROT_READ | MPROT_WRITE );

			for( int i = 0, nCount = GetModeCount(); i < nCount; ++i )
			{
				DispMode_c *pcMode;

				if( pcMode = GetModeDesc( i ) )
				{
					pcMode->m_pFrameBuffer = m_pFrameBuffer;
				}
				else
				{
					dbprintf( "ERROR : No descriptor for mode %ld in 'VesaDriver_c'\n", i );
				}
			}
//                      outw_p( 0x1206, 0x3c4 );        // Enable extended registers
			return ( TRUE );
		}
	}
	return ( FALSE );
}

void VesaDriver_c::Close( void )
{
}

int VesaDriver_c::GetModeCount( void )
{
	return ( m_cModeList.Count() );
}

bool VesaDriver_c::GetModeInfo( ModeInfo_s * psInfo, int nIndex )
{
	DispMode_c *pcMode;

	if( pcMode = GetModeDesc( nIndex ) )
	{
		*psInfo = *pcMode;
		return ( TRUE );
	}
	return ( FALSE );
}

BitMap_c *VesaDriver_c::SetScreenMode( int nIndex )
{
	m_pcCurMode = GetModeDesc( nIndex );

	if( NULL != m_pcCurMode )
	{
		area_id nArea;


		RemapArea( g_nFrameBufArea, m_pcCurMode->m_pPhysDispAddr );

/*		MemMapToDev( m_pFrameBuffer, m_pcCurMode->m_pPhysDispAddr, (m_nFrameBufferSize + MEM_PAGEMASK) & ~MEM_PAGEMASK, MPROT_USER | MPROT_READ | MPROT_WRITE ); */

		dbprintf( "Set Screen mode %lx\n", m_pcCurMode->m_nVesaMode );

		if( SetVesaMode( m_pcCurMode->m_nVesaMode ) )
		{
			InitS3();

			ModeInfo_s sModeInfo = *m_pcCurMode;

			BitMap_c *pcBitmap = new BitMap_c( &sModeInfo, m_pFrameBuffer );

			pcBitmap->m_pcDriver = this;
			pcBitmap->m_bVideoMem = true;

			m_pcMouse = new MousePtr_c( pcBitmap, this, 8, 8 );

			return ( pcBitmap );
		}
	}
	return ( NULL );
}

void VesaDriver_c::MouseOn( void )
{
	if( NULL != m_pcMouse )
	{
		m_pcMouse->Show();
	}
}

void VesaDriver_c::MouseOff( void )
{
	if( NULL != m_pcMouse )
	{
		m_pcMouse->Hide();
	}
}

void VesaDriver_c::SetMousePos( Point2_c cNewPos )
{
	if( NULL != m_pcMouse )
	{
		m_pcMouse->MoveTo( cNewPos );
	}
}

bool VesaDriver_c::IntersectWithMouse( const Rect_c & cRect )
{
	if( NULL != m_pcMouse )
	{
		return ( cRect.DoIntersect( m_pcMouse->GetFrame() ) );
	}
	else
	{
		return ( false );
	}
}


BitMap_c *VesaDriver_c::CreateBitmap( Rect_c cBounds, int nMode )
{
	DispMode_c *pcCurMode = GetModeDesc( nMode );

	if( NULL != pcCurMode )
	{
		BitMap_c *pcBitmap = new BitMap_c( cBounds.Width(), cBounds.Height(  ), pcCurMode->m_nBitsPerPixel );

		pcBitmap->m_pcDriver = this;
		return ( pcBitmap );
	}
}


void VesaDriver_c::DeleteBitmap( BitMap_c * pcBitmap )
{
	delete pcBitmap;
}

void VesaDriver_c::SetColor( int nIndex, const Color32_s & sColor )
{
	outb_p( nIndex, 0x3c8 );
	outb_p( sColor.anRGBA[0] >> 2, 0x3c9 );
	outb_p( sColor.anRGBA[1] >> 2, 0x3c9 );
	outb_p( sColor.anRGBA[2] >> 2, 0x3c9 );
}

DispMode_c *VesaDriver_c::GetModeDesc( uint32 nIndex )
{
	return ( m_cModeList[nIndex] );
}

bool VesaDriver_c::SetVesaMode( uint32 nMode )
{
	struct RMREGS rm;

	memset( &rm, 0, sizeof( struct RMREGS ) );

	rm.EAX = 0x4f02;
	rm.EBX = nMode;

	realint( 0x10, &rm );

	return ( rm.EAX & 0xffff );
}

/******************************************************************************
 ******************************************************************************/

void VesaDriver_c::FillBlit8( uint8 *pDst, int nMod, int W, int H, int nColor )
{
	int X, Y;

	for( Y = 0; Y < H; Y++ )
	{
		for( X = 0; X < W; X++ )
		{
			*pDst++ = nColor;
		}
		pDst += nMod;
	}
}

/******************************************************************************
 ******************************************************************************/

void VesaDriver_c::FillBlit16( uint16 *pDst, int nMod, int W, int H, uint32 nColor )
{
	int X, Y;

	for( Y = 0; Y < H; Y++ )
	{
		for( X = 0; X < W; X++ )
		{
			*pDst++ = nColor;
		}
		pDst += nMod;
	}
}

/******************************************************************************
 ******************************************************************************/

void VesaDriver_c::FillBlit24( uint8 *pDst, int nMod, int W, int H, uint32 nColor )
{
	int X, Y;

	for( Y = 0; Y < H; Y++ )
	{
		for( X = 0; X < W; X++ )
		{
			*pDst++ = nColor & 0xff;
			*( ( uint16 * )pDst ) = nColor >> 8;
			pDst += 2;
		}
		pDst += nMod;
	}
}

uint32 VesaDriver_c::ConvertColor32( Color32_s sColor )
{
	uint32 nColor;

	nColor = ( sColor.anRGBA[0] >> ( 8 - m_pcCurMode->m_nRedBits ) ) << m_pcCurMode->m_nRedPos | ( sColor.anRGBA[1] >> ( 8 - m_pcCurMode->m_nGreenBits ) ) << m_pcCurMode->m_nGreenPos | ( sColor.anRGBA[2] >> ( 8 - m_pcCurMode->m_nBlueBits ) ) << m_pcCurMode->m_nBluePos;

	return ( nColor );
}

bool VesaDriver_c::WritePixel( BitMap_c * psBitMap, int X, int Y, const Color8_s & sColor )
{
	psBitMap->m_pRaster[X + Y * psBitMap->m_nBytesPerLine] = sColor.nIndex;
	return ( TRUE );
}


bool VesaDriver_c::DrawLine( BitMap_c * psBitMap, const Point2_c & cPnt1, const Point2_c & cPnt2, const Color8_s & sColor )
{
	int I;
	int X, Y;
	int dX, dY, Xs, Ys;

	dX = cPnt1.X - cPnt2.X;
	dY = cPnt1.Y - cPnt2.Y;

	if( dX || dY )
	{
		if( abs( dX ) > abs( dY ) )
		{
			X = cPnt2.X;
			Y = ( cPnt2.Y << 16 ) + 0x8000;

			Xs = dX < 0 ? -1 : 1;

			dY = ( dY * 65536 ) / abs( dX );

			for( I = 0; I <= abs( dX ); ++I )
			{
				WritePixel( psBitMap, X, Y >> 16, sColor );
				X += Xs;
				Y += dY;
			}
		}
		else
		{
			X = ( cPnt2.X << 16 ) + 0x8000;
			Y = cPnt2.Y;

			Ys = dY < 0 ? -1 : 1;
			dX = ( dX * 65536 ) / abs( dY );

			for( I = 0; I <= abs( dY ); ++I )
			{
				WritePixel( psBitMap, X >> 16, Y, sColor );
				X += dX;
				Y += Ys;
			}
		}
	}
	else
	{
		WritePixel( psBitMap, cPnt1.X, cPnt1.Y, sColor );
	}
	return ( TRUE );
}

bool VesaDriver_c::WritePixel16( BitMap_c * psBitMap, int X, int Y, uint32 nColor )
{
	( ( uint16 * )&psBitMap->m_pRaster[X * 2 + Y * psBitMap->m_nBytesPerLine] )[0] = nColor;
	return ( TRUE );
}


bool VesaDriver_c::DrawLine16( BitMap_c * psBitMap, const Rect_c & cClipRect, const Point2_c & cPnt1, const Point2_c & cPnt2, const Color32_s & sColor )
{
	uint32 nColor;

	nColor = ( sColor.anRGBA[0] >> ( 8 - m_pcCurMode->m_nRedBits ) ) << m_pcCurMode->m_nRedPos | ( sColor.anRGBA[1] >> ( 8 - m_pcCurMode->m_nGreenBits ) ) << m_pcCurMode->m_nGreenPos | ( sColor.anRGBA[2] >> ( 8 - m_pcCurMode->m_nBlueBits ) ) << m_pcCurMode->m_nBluePos;

	if( psBitMap->m_bVideoMem && 0 )
	{
		S3DrawLine_32( cPnt1.X, cPnt1.Y, cPnt2.X, cPnt2.Y, nColor, true, cClipRect.MinX, cClipRect.MinY, cClipRect.MaxX, cClipRect.MaxY );
	}
	else
	{
		int I;
		int X, Y;
		int dX, dY, Xs, Ys;

		dX = cPnt1.X - cPnt2.X;
		dY = cPnt1.Y - cPnt2.Y;

		if( dX || dY )
		{
			if( abs( dX ) > abs( dY ) )
			{
				X = cPnt2.X;
				Y = ( cPnt2.Y << 16 ) + 0x8000;

				Xs = dX < 0 ? -1 : 1;

				dY = ( dY * 65536 ) / abs( dX );

				for( I = 0; I <= abs( dX ); ++I )
				{
					if( cClipRect.DoIntersect( Point2_c( X, Y >> 16 ) ) )
					{
						WritePixel16( psBitMap, X, Y >> 16, nColor );
					}
					X += Xs;
					Y += dY;
				}
			}
			else
			{
				X = ( cPnt2.X << 16 ) + 0x8000;
				Y = cPnt2.Y;

				Ys = dY < 0 ? -1 : 1;
				dX = ( dX * 65536 ) / abs( dY );

				for( I = 0; I <= abs( dY ); ++I )
				{
					if( cClipRect.DoIntersect( Point2_c( X >> 16, Y ) ) )
					{
						WritePixel16( psBitMap, X >> 16, Y, nColor );
					}
					X += dX;
					Y += Ys;
				}
			}
		}
		else
		{
			if( cClipRect.DoIntersect( cPnt1 ) )
			{
				WritePixel16( psBitMap, cPnt1.X, cPnt1.Y, nColor );
			}
		}
	}
	return ( true );
}



bool VesaDriver_c::FillRect16( BitMap_c * psBitMap, const Rect_c & cRect, const Color32_s & sColor )
{
	int BltX, BltY, BltW, BltH;
	int nBytesPerPix = 2;

	uint32 nColor;

	nColor = ( ( sColor.anRGBA[0] >> ( 8 - m_pcCurMode->m_nRedBits ) ) << m_pcCurMode->m_nRedPos ) | ( ( sColor.anRGBA[1] >> ( 8 - m_pcCurMode->m_nGreenBits ) ) << m_pcCurMode->m_nGreenPos ) | ( ( sColor.anRGBA[2] >> ( 8 - m_pcCurMode->m_nBlueBits ) ) << m_pcCurMode->m_nBluePos );

	if( psBitMap->m_bVideoMem && 0 )
	{
		S3FillRect_32( cRect.MinX, cRect.MinY, cRect.MaxX, cRect.MaxY, nColor );
	}
	else
	{
		/*
		   if ( rp->rp_Clips[0].BitMap->BitsPerPixel > 8 )              BytesPerPix++;
		   if ( rp->rp_Clips[0].BitMap->BitsPerPixel > 16 )     BytesPerPix++;
		   if ( rp->rp_Clips[0].BitMap->BitsPerPixel > 24 )     BytesPerPix++;

		   if ( BytesPerPix > 1 )
		   {
		   Color = PenToRGB( RPort->BitMap->ColorMap, Color );
		   }
		 */

		BltX = cRect.Left();
		BltY = cRect.Top();
		BltW = cRect.Width();
		BltH = cRect.Height();

		switch ( nBytesPerPix )
		{
		case 1:
			FillBlit8( psBitMap->m_pRaster + ( ( BltY * psBitMap->m_nBytesPerLine ) + BltX ), psBitMap->m_nBytesPerLine - BltW, BltW, BltH, nColor );
			break;
		case 2:
			FillBlit16( ( uint16 * )&psBitMap->m_pRaster[BltY * psBitMap->m_nBytesPerLine + BltX * 2], psBitMap->m_nBytesPerLine / 2 - BltW, BltW, BltH, nColor );
			break;
		case 3:
			FillBlit24( &psBitMap->m_pRaster[BltY * psBitMap->m_nBytesPerLine + BltX * 3], psBitMap->m_nBytesPerLine - BltW * 3, BltW, BltH, nColor );
			break;
		}
	}
	return ( true );
}


bool VesaDriver_c::FillRect( BitMap_c * psBitMap, const Rect_c & cRect, const Color8_s & sColor )
{
	int BltX, BltY, BltW, BltH;
	int nBytesPerPix = 1;

	uint32 nColor = sColor.nIndex;


/*
	if ( rp->rp_Clips[0].BitMap->BitsPerPixel > 8 )		BytesPerPix++;
	if ( rp->rp_Clips[0].BitMap->BitsPerPixel > 16 )	BytesPerPix++;
	if ( rp->rp_Clips[0].BitMap->BitsPerPixel > 24 )	BytesPerPix++;

	if ( BytesPerPix > 1 )
	{
		Color = PenToRGB( RPort->BitMap->ColorMap, Color );
	}
*/

	BltX = cRect.Left();
	BltY = cRect.Top();
	BltW = cRect.Width();
	BltH = cRect.Height();

	switch ( nBytesPerPix )
	{
	case 1:
		FillBlit8( psBitMap->m_pRaster + ( ( BltY * psBitMap->m_nBytesPerLine ) + BltX ), psBitMap->m_nBytesPerLine - BltW, BltW, BltH, nColor );
		break;
	case 2:
		FillBlit16( ( uint16 * )&psBitMap->m_pRaster[BltY * psBitMap->m_nBytesPerLine + BltX * 2], psBitMap->m_nBytesPerLine / 2 - BltW, BltW, BltH, nColor );
		break;
	case 3:
		FillBlit24( &psBitMap->m_pRaster[BltY * psBitMap->m_nBytesPerLine + BltX * 3], psBitMap->m_nBytesPerLine - BltW * 3, BltW, BltH, nColor );
		break;
	}
	return ( TRUE );
}

void Blit( uint8 *Src, uint8 *Dst, int SMod, int DMod, int W, int H, bool Rev )
{
	int i;
	int X, Y;
	uint32 *LSrc, *LDst;

/*
	uint8		anLineBuf[4096];
	uint8*	pBufPtr;
*/
	if( Rev )
	{
		for( Y = 0; Y < H; Y++ )
		{
			for( X = 0; ( X < W ) && ( CUL( Src - 3 ) & 3 ); X++ )
			{
				*Dst-- = *Src--;
			}

			LSrc = ( uint32 * )( CUL( Src ) - 3 );
			LDst = ( uint32 * )( CUL( Dst ) - 3 );

			i = ( W - X ) / 4;

			X += i * 4;

			for( ; i; i-- )
			{
				*LDst-- = *LSrc--;
			}

			Src = ( uint8 * )( CUL( LSrc ) + 3 );
			Dst = ( uint8 * )( CUL( LDst ) + 3 );

			for( ; X < W; X++ )
			{
				*Dst-- = *Src--;
			}

			Dst -= CLO( DMod );
			Src -= CLO( SMod );
		}
	}
	else
	{
		for( Y = 0; Y < H; Y++ )
		{
			for( X = 0; ( X < W ) && ( CUL( Src ) & 3 ); ++X )
			{
				*Dst++ = *Src++;
			}

			LSrc = ( uint32 * )Src;
			LDst = ( uint32 * )Dst;

			i = ( W - X ) / 4;

			X += i * 4;

			for( ; i; i-- )
			{
				*LDst++ = *LSrc++;
			}

			Src = ( uint8 * )LSrc;
			Dst = ( uint8 * )LDst;

			for( ; X < W; X++ )
			{
				*Dst++ = *Src++;
			}

			Dst += DMod;
			Src += SMod;
		}
	}

/*
		for ( Y = 0 ; Y < H ; Y++ )
		{
			pBufPtr = anLineBuf;

			for ( X=0 ; (X < W) && (CUL(Src) & 3) ; X++ )
			{
				*pBufPtr++ = *Src++;
			}

			LSrc=(uint32*)Src;
			LDst=(uint32*)pBufPtr;

			i = (W - X) / 4;

			X += i * 4;

			for( ; i ; i-- )
			{
				*LDst++=*LSrc++;
			}

			Src	=	(uint8*) LSrc;
			pBufPtr	=	(uint8*) LDst;

			for ( ; X < W ; X++ )
			{
				*pBufPtr++ = *Src++;
			}

//			Dst += DMod;
			Src += SMod;
//		}

			pBufPtr = anLineBuf;

			for ( X=0 ; (X < W) && (CUL(pBufPtr) & 3) ; X++ )
			{
				*Dst++ = *pBufPtr++;
			}

			LSrc=(uint32*)pBufPtr;
			LDst=(uint32*)Dst;

			i = (W - X) / 4;

			X += i * 4;

			for( ; i ; i-- )
			{
				*LDst++=*LSrc++;
			}

			pBufPtr	=	(uint8*) LSrc;
			Dst	=	(uint8*) LDst;

			for ( ; X < W ; X++ )
			{
				*Dst++=*pBufPtr++;
			}

			Dst += DMod;
//			Src += SMod;
		}
	}
*/
}

void BitBlit( BitMap_c * sbm, BitMap_c * dbm, int sx, int sy, int dx, int dy, int w, int h )
{
	int Smod, Dmod;
	int BytesPerPix = 1;

	int InPtr, OutPtr;

	if( dbm->m_nBitsPerPixel > 8 )
		BytesPerPix++;
	if( dbm->m_nBitsPerPixel > 16 )
		BytesPerPix++;
	if( dbm->m_nBitsPerPixel > 24 )
		BytesPerPix++;

	sx *= BytesPerPix;
	dx *= BytesPerPix;
	w *= BytesPerPix;

	if( sx >= dx && 1 )
	{
		if( sy >= dy )
		{
			Smod = sbm->m_nBytesPerLine - w;
			Dmod = dbm->m_nBytesPerLine - w;
			InPtr = sy * sbm->m_nBytesPerLine + sx;
			OutPtr = dy * dbm->m_nBytesPerLine + dx;

			Blit( sbm->m_pRaster + InPtr, dbm->m_pRaster + OutPtr, Smod, Dmod, w, h, NULL );
		}
		else
		{
			Smod = -sbm->m_nBytesPerLine - w;
			Dmod = -dbm->m_nBytesPerLine - w;
			InPtr = ( ( sy + h - 1 ) * sbm->m_nBytesPerLine ) + sx;
			OutPtr = ( ( dy + h - 1 ) * dbm->m_nBytesPerLine ) + dx;

			Blit( sbm->m_pRaster + InPtr, dbm->m_pRaster + OutPtr, Smod, Dmod, w, h, NULL );
		}
	}
	else
	{
		if( sy > dy )
		{
			Smod = -( sbm->m_nBytesPerLine + w );
			Dmod = -( dbm->m_nBytesPerLine + w );
			InPtr = ( sy * sbm->m_nBytesPerLine ) + sx + w - 1;
			OutPtr = ( dy * dbm->m_nBytesPerLine ) + dx + w - 1;
			Blit( sbm->m_pRaster + InPtr, dbm->m_pRaster + OutPtr, Smod, Dmod, w, h, TRUE );
		}
		else
		{
			Smod = sbm->m_nBytesPerLine - w;
			Dmod = dbm->m_nBytesPerLine - w;
			InPtr = ( sy + h - 1 ) * sbm->m_nBytesPerLine + sx + w - 1;
			OutPtr = ( dy + h - 1 ) * dbm->m_nBytesPerLine + dx + w - 1;
			Blit( sbm->m_pRaster + InPtr, dbm->m_pRaster + OutPtr, Smod, Dmod, w, h, TRUE );
		}
	}
}

bool VesaDriver_c::BltBitMap( BitMap_c * dstbm, BitMap_c * srcbm, Rect_c cSrcRect, Point2_c cDstPos )
{
	if( dstbm->m_bVideoMem && srcbm->m_bVideoMem && 0 )
	{
#ifdef CIRRUS
		int nSrcAddr;
		int nDstAddr;
		int nMode;

		WriteBltReg( BLT_START, 0x04 );	// Reset blitter

		SetBltWidth( cSrcRect.Width() * 2 - 1 );
		SetBltHeight( cSrcRect.Height() - 1 );

		SetDstPitch( 800 * 2 );
		SetSrcPitch( 800 * 2 );

		if( cDstPos > cSrcRect.LeftTop() )
		{
			Rect_c cDstRect = cSrcRect.Bounds() + cDstPos;

			nDstAddr = cDstRect.MaxX * 2 + cDstRect.MaxY * 800 * 2 + 1;
			nSrcAddr = cSrcRect.MaxX * 2 + cSrcRect.MaxY * 800 * 2 + 1;
			nMode = 0x01;	// Reverse
		}
		else
		{
			nDstAddr = cDstPos.X * 2 + cDstPos.Y * 800 * 2;
			nSrcAddr = cSrcRect.MinX * 2 + cSrcRect.MinY * 800 * 2;
			nMode = 0x00;
		}

		SetBltDst( nDstAddr );
		SetBltSrc( nSrcAddr );

		WriteBltReg( BLT_MODE, nMode );

		WriteBltReg( BLT_RASTER_OP, 0x0d );	// Copy;

		WriteBltReg( BLT_START, 0x02 );	// Start blitter

		WaitBlit();
#else
		S3Blit( cSrcRect.MinX, cSrcRect.MinY, cDstPos.X, cDstPos.Y, cSrcRect.Width(), cSrcRect.Height(  ) );
//      v_blit( cSrcRect.MinX, cSrcRect.MinY, cDstPos.X, cDstPos.Y, cSrcRect.Width(), cSrcRect.Height() );
#endif
	}
	else
	{
		int sx = cSrcRect.MinX;
		int sy = cSrcRect.MinY;
		int dx = cDstPos.X;
		int dy = cDstPos.Y;
		int w = cSrcRect.Width();
		int h = cSrcRect.Height();

#if 0
		if( srcbm == NULL )
		{
			int BytesPerPix = 1;

			if( dstbm->m_nBitsPerPixel > 8 )
				BytesPerPix++;
			if( dstbm->m_nBitsPerPixel > 16 )
				BytesPerPix++;
			if( dstbm->m_nBitsPerPixel > 24 )
				BytesPerPix++;

			switch ( BytesPerPix )
			{
			case 1:
				FillBlit8( &dstbm->m_pRaster[dy * dstbm->m_nBytesPerLine + dx * BytesPerPix], dstbm->m_nBytesPerLine / BytesPerPix - w, w, h, 0 );
				return ( TRUE );
				/*
				   case 2:
				   FillBlit16( (APTR) &dstbm->Raster[dy * dstbm->BytesPerLine + dx * BytesPerPix], dstbm->BytesPerLine / BytesPerPix - w, w, h, PenToRGB( dstbm->ColorMap, 0 ) );
				   return( TRUE );
				   case 3:
				   FillBlit24( &dstbm->Raster[dy * dstbm->BytesPerLine + dx * 3], dstbm->BytesPerLine - w * 3, w, h, PenToRGB( dstbm->ColorMap, 0 ) );
				   return( TRUE );
				 */
			}
			return ( TRUE );
		}
#endif

		if( srcbm->m_nBitsPerPixel == dstbm->m_nBitsPerPixel )
		{
			BitBlit( srcbm, dstbm, sx, sy, dx, dy, w, h );
		}
		else
		{
#if 0
			if( srcbm->m_nBitsPerPixel == 8 )
			{
				switch ( dstbm->m_nBitsPerPixel )
				{
				case 15:
				case 16:
					BitBlit8_16( srcbm, dstbm, sx, sy, dx, dy, w, h );
					break;
				case 24:
					BitBlit8_24( srcbm, dstbm, sx, sy, dx, dy, w, h );
					break;
				}
			}
#endif
		}
		return ( TRUE );
	}
}

#if 0
bool VesaDriver_c::BltBitMap( BitMap_c * dstbm, BitMap_c * srcbm, Rect_c cSrcRect, Point2_c cDstPos )
{
	int sx = cSrcRect.MinX;
	int sy = cSrcRect.MinY;
	int dx = cDstPos.X;
	int dy = cDstPos.Y;
	int w = cSrcRect.Width();
	int h = cSrcRect.Height();

#if 0
	if( srcbm == NULL )
	{
		int BytesPerPix = 1;

		if( dstbm->m_nBitsPerPixel > 8 )
			BytesPerPix++;
		if( dstbm->m_nBitsPerPixel > 16 )
			BytesPerPix++;
		if( dstbm->m_nBitsPerPixel > 24 )
			BytesPerPix++;

		switch ( BytesPerPix )
		{
		case 1:
			FillBlit8( &dstbm->m_pRaster[dy * dstbm->BytesPerLine + dx * BytesPerPix], dstbm->BytesPerLine / BytesPerPix - w, w, h, 0 );
			return ( TRUE );

/*
			case 2:
				FillBlit16( (APTR) &dstbm->Raster[dy * dstbm->BytesPerLine + dx * BytesPerPix], dstbm->BytesPerLine / BytesPerPix - w, w, h, PenToRGB( dstbm->ColorMap, 0 ) );
				return( TRUE );
			case 3:
				FillBlit24( &dstbm->Raster[dy * dstbm->BytesPerLine + dx * 3], dstbm->BytesPerLine - w * 3, w, h, PenToRGB( dstbm->ColorMap, 0 ) );
				return( TRUE );
*/
		}
		return ( TRUE );
	}
#endif

	if( srcbm->BitsPerPixel == dstbm->BitsPerPixel )
	{
		BitBlit( srcbm, dstbm, sx, sy, dx, dy, w, h );
	}
	else
	{
#if 0
		if( srcbm->BitsPerPixel == 8 )
		{
			switch ( dstbm->BitsPerPixel )
			{
			case 15:
			case 16:
				BitBlit8_16( srcbm, dstbm, sx, sy, dx, dy, w, h );
				break;
			case 24:
				BitBlit8_24( srcbm, dstbm, sx, sy, dx, dy, w, h );
				break;
			}
		}
#endif
	}
	return ( TRUE );
}
#endif


bool VesaDriver_c::BltBitMapMask( BitMap_c * pcDstBitMap, BitMap_c * pcSrcBitMap, const Color32_s & sHighColor, const Color32_s & sLowColor, Rect_c cSrcRect, Point2_c cDstPos )
{
	int DX = cDstPos.X;
	int DY = cDstPos.Y;

	int SX = cSrcRect.MinX;
	int SY = cSrcRect.MinY;

	int W = cSrcRect.Width();
	int H = cSrcRect.Height();

	uint32 Fg = ConvertColor32( sHighColor );
	uint32 Bg = ConvertColor32( sLowColor );

	char CB;
	int X, Y, SBit;
	int BytesPerPix = 1;

	uint32 BPR, SByte, DYoff;

	BPR = pcSrcBitMap->m_nBytesPerLine;


	if( pcDstBitMap->m_nBitsPerPixel > 8 )
		BytesPerPix++;
	if( pcDstBitMap->m_nBitsPerPixel > 16 )
		BytesPerPix++;
	if( pcDstBitMap->m_nBitsPerPixel > 24 )
		BytesPerPix++;

/*
	if ( BytesPerPix > 1 )
	{
		Fg = PenToRGB( DBM->ColorMap, Fg );
		Bg = PenToRGB( DBM->ColorMap, Bg );
	}
*/
	switch ( BytesPerPix )
	{
	case 1:
	case 2:
		DYoff = DY * pcDstBitMap->m_nBytesPerLine / BytesPerPix;
		break;
	case 3:
		DYoff = DY * pcDstBitMap->m_nBytesPerLine;
		break;
	default:
		DYoff = DY * pcDstBitMap->m_nBytesPerLine;
		kassertw( 0 );
		break;
	}

	for( Y = 0; Y < H; Y++ )
	{
		SByte = ( SY * BPR ) + ( SX / 8 );
		CB = pcSrcBitMap->m_pRaster[SByte++];
		SBit = 7 - ( SX % 8 );

		switch ( BytesPerPix )
		{
		case 1:
			for( X = 0; X < W; X++ )
			{
				if( CB & ( 1L << SBit ) )
				{
					pcDstBitMap->m_pRaster[DYoff + DX] = Fg;
				}
				else
				{
					pcDstBitMap->m_pRaster[DYoff + DX] = Bg;
				}
				SX++;
				DX++;
				if( !SBit )
				{
					SBit = 8;
					CB = pcSrcBitMap->m_pRaster[SByte++];
				}
				SBit--;
			}
			break;
		case 2:
			for( X = 0; X < W; X++ )
			{
				if( CB & ( 1L << SBit ) )
				{
					( ( uint16 * )pcDstBitMap->m_pRaster )[DYoff + DX] = Fg;
				}
				else
				{
					( ( uint16 * )pcDstBitMap->m_pRaster )[DYoff + DX] = Bg;
				}
				SX++;
				DX++;
				if( !SBit )
				{
					SBit = 8;
					CB = pcSrcBitMap->m_pRaster[SByte++];
				}
				SBit--;
			}
			break;
		case 3:
			for( X = 0; X < W; X++ )
			{
				if( CB & ( 1L << SBit ) )
				{
					pcDstBitMap->m_pRaster[DYoff + DX * 3] = Fg & 0xff;
					( ( uint16 * )&pcDstBitMap->m_pRaster[DYoff + DX * 3 + 1] )[0] = Fg >> 8;
				}
				else
				{
					pcDstBitMap->m_pRaster[DYoff + DX * 3] = Bg & 0xff;
					( ( uint16 * )&pcDstBitMap->m_pRaster[DYoff + DX * 3 + 1] )[0] = Bg >> 8;
				}
				SX++;
				DX++;
				if( !SBit )
				{
					SBit = 8;
					CB = pcSrcBitMap->m_pRaster[SByte++];
				}
				SBit--;
			}
			break;
		}
		SX -= W;
		DX -= W;

		SY++;
		DY++;

		if( BytesPerPix == 2 )
			DYoff += pcDstBitMap->m_nBytesPerLine / 2;
		else
			DYoff += pcDstBitMap->m_nBytesPerLine;
	}
}

Color32_s VesaDriver_c::ClutToCol( uint32 nClut )
{
	Color32_s sColor;

	sColor.anRGBA[0] = ( nClut >> m_pcCurMode->m_nRedPos ) << ( 8 - m_pcCurMode->m_nRedBits );
	sColor.anRGBA[1] = ( nClut >> m_pcCurMode->m_nGreenPos ) << ( 8 - m_pcCurMode->m_nGreenBits );
	sColor.anRGBA[2] = ( nClut >> m_pcCurMode->m_nBluePos ) << ( 8 - m_pcCurMode->m_nBlueBits );

	return ( sColor );
}

bool VesaDriver_c::RenderGlyph( BitMap_c * pcBitmap, Glyph_c * pcGlyph, const Point2_c & cPos, const Rect_c & cClipRect, uint32 *anPalette )
{
	Rect_c cBounds = pcGlyph->m_cBounds + cPos;
	Rect_c cRect = cBounds & cClipRect;

	if( cRect.IsValid() )
	{
		int sx = cRect.MinX - cBounds.MinX;
		int sy = cRect.MinY - cBounds.MinY;

		int nWidth = cRect.Width();
		int nHeight = cRect.Height();

		int nSrcModulo = pcGlyph->m_nBytesPerLine - nWidth;
		int nDstModulo = pcBitmap->m_nBytesPerLine / 2 - nWidth;

		uint8 *pSrc = pcGlyph->m_pRaster + sx + sy * pcGlyph->m_nBytesPerLine;
		uint16 *pDst = ( uint16 * )pcBitmap->m_pRaster + cRect.MinX + ( cRect.MinY * pcBitmap->m_nBytesPerLine / 2 );

		for( int y = 0; y < nHeight; ++y )
		{
			for( int x = 0; x < nWidth; ++x )
			{
				int nPix = *pSrc++;

/*
				Color32_s		sCurCol;
				Color32_s		sFgColor;
 				Color32_s		sBgColor;

				*((uint32*)&sBgColor) = *pDst;
*/
				if( nPix > 0 )
				{

/*
					for ( int j = 0 ; j < 3 ; ++j ) {
						sCurCol.anRGBA[ j ] = sBgColor.anRGBA[ j ] + (sFgColor.anRGBA[ j ] - sBgColor.anRGBA[ j ]) * nPix / 4;
					}
					*pDst = ConvertColor32( sCurCol ); // anPalette[ nPix - 1 ];
*/
					*pDst = anPalette[nPix - 1];
				}
				pDst++;
			}
			pSrc += nSrcModulo;
			pDst += nDstModulo;
		}
	}
	return ( true );
}






bool VesaDriver_c::RenderGlyph( BitMap_c * pcBitmap, Glyph_c * pcGlyph, const Point2_c & cPos, const Rect_c & cClipRect, const Color32_s & sFgColor )
{
	Rect_c cBounds = pcGlyph->m_cBounds + cPos;
	Rect_c cRect = cBounds & cClipRect;

	if( cRect.IsValid() )
	{
		int sx = cRect.MinX - cBounds.MinX;
		int sy = cRect.MinY - cBounds.MinY;

		int nWidth = cRect.Width();
		int nHeight = cRect.Height();

		int nSrcModulo = pcGlyph->m_nBytesPerLine - nWidth;
		int nDstModulo = pcBitmap->m_nBytesPerLine / 2 - nWidth;

		uint8 *pSrc = pcGlyph->m_pRaster + sx + sy * pcGlyph->m_nBytesPerLine;
		uint16 *pDst = ( uint16 * )pcBitmap->m_pRaster + cRect.MinX + ( cRect.MinY * pcBitmap->m_nBytesPerLine / 2 );

		Color32_s sCurCol;
		Color32_s sBgColor;


		int nFgClut = ConvertColor32( sFgColor );

		for( int y = 0; y < nHeight; ++y )
		{
			for( int x = 0; x < nWidth; ++x )
			{
				int nPix = *pSrc++;

				if( nPix > 0 )
				{
					if( nPix == 4 )
					{
						*pDst = nFgClut;
					}
					else
					{
						int nClut = *pDst;

						sBgColor = ClutToCol( nClut );

						for( int j = 0; j < 3; ++j )
						{
							sCurCol.anRGBA[j] = sBgColor.anRGBA[j] + ( sFgColor.anRGBA[j] - sBgColor.anRGBA[j] ) * nPix / 4;
						}
						*pDst = ConvertColor32( sCurCol );
					}
				}
				pDst++;
			}
			pSrc += nSrcModulo;
			pDst += nDstModulo;
		}
	}
	return ( true );
}

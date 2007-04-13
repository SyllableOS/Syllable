// BMPTrans (C)opyright 2007 Jonas Jarvoll
//
// A BMP translator for the Syllable OS (www.syllable.org)
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

// Inspiration and code copying from GdkPixbuf library:

/*
 * GdkPixbuf library - PCX image loader
 *
 * Copyright (C) 2003 Josh A. Beam
 *
 * Authors: Josh A. Beam <josh@joshbeam.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

////////////////////////////////////////////////////////////////////////////////

// Suggestions and possible improvements:
// - Not test with 1, 2 or 4 bits PCX!!!

////////////////////////////////////////////////////////////////////////////////
//
// INCLUDES
//
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <atheos/kdebug.h>
#include <util/circularbuffer.h>
#include <translation/translator.h>

using namespace os;

//////////////////////////////////////////////////////////////////////////////////////////
//
// DEBUG
//
//////////////////////////////////////////////////////////////////////////////////////////

// Uncomment for debug info
// #define DEBUG_PCX

// Size of the PCX header
#define PCX_HEADER_SIZE  128

//////////////////////////////////////////////////////////////////////////////////////////
//
// PCX header structures
//
//////////////////////////////////////////////////////////////////////////////////////////

typedef struct s_pcxheader 
{
	uint8 manufacturer;
	uint8 version;
	uint8 encoding;
	uint8 bitsperpixel;
	int16 xmin;
	int16 ymin;
	int16 xmax;
	int16 ymax;
	uint16 horizdpi;
	uint16 vertdpi;
	uint8 palette[48];
	uint8 reserved;
	uint8 colorplanes;
	uint16 bytesperline;
	uint16 palettetype;
	uint16 hscrsize;
	uint16 vscrsize;
	uint8 filler[54];
	uint width;			//  Note! This is not a part of the PCX header
	uint height;		//  Note! This is not a part of the PCX header
} PCXheader;

//////////////////////////////////////////////////////////////////////////////////////////
//
// The translator for PCX
//
//////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////
//
// CLASS DEFINITION OF "PCXTrans"
//
//////////////////////////////////////////////////////////////////////////////////////////

class PCXTrans : public Translator
{
public:
    PCXTrans();
    virtual ~PCXTrans();

    virtual void     SetConfig( const Message& cConfig ) {}
    virtual status_t AddData( const void* pData, size_t nLen, bool bFinal );
    virtual ssize_t  AvailableDataSize();
    virtual ssize_t  Read( void* pData, size_t nLen );
    virtual void     Abort();
    virtual void     Reset();
   
private:
	status_t  _TranslatePCX();
	status_t  _Translate24( PCXheader& pcxheader );
	status_t  _Translate8( PCXheader& pcxheader );
	status_t  _Translate4( PCXheader& pcxheader );
	status_t  _Translate2( PCXheader& pcxheader );
	status_t  _Translate1( PCXheader& pcxheader );

	bool _ReadScanlineData( PCXheader& pcxheader, uint8* planes[], uint store_planes );

	status_t _ApplyPalette8( PCXheader& pcxheader, uint8* tmp_image );

	uint8 _ReadPixel4( uint8* data, uint offset );
	uint8 _ReadPixel1( uint8* data, uint offset);

	uint16 _ConvertTo16bit( uint8* src );
	int16 _ConvertTo16bitSigned( uint8* src );

	BitmapHeader		m_sBitmapHeader;
    BitmapFrameHeader	m_sCurrentFrame;

	CircularBuffer		m_cOutBuffer;
	CircularBuffer		m_cInBuffer;

	bool 				m_bEOF;				// True if we have all input data

	uint8*				m_ScanLines;
};

//////////////////////////////////////////////////////////////////////////////////////////
//
// PUBLIC METHODS IN "PCXTrans" CLASS
//
//////////////////////////////////////////////////////////////////////////////////////////


// The constructor
PCXTrans :: PCXTrans()
{
	m_bEOF = false;
};

// The destructor
PCXTrans :: ~PCXTrans()
{
}

status_t PCXTrans :: AddData( const void* pData, size_t nLen, bool bFinal )
{    
	m_bEOF = bFinal;
    m_cInBuffer.Write( pData, nLen );
	
	// We have now a complete read image in memory. Its about time to start decoding it.
	if( bFinal )
		return _TranslatePCX();
	
    return( 0 );
}

ssize_t PCXTrans :: AvailableDataSize()
{
	return( m_cOutBuffer.Size() );
}

ssize_t PCXTrans :: Read( void* pData, size_t nLen )
{
	if( m_bEOF == false )
		return( ERR_SUSPENDED );

    return( m_cOutBuffer.Read( pData, nLen ) );
}

void PCXTrans :: Abort()
{
}

void PCXTrans :: Reset()
{
}

////////////////////////////////////////////////////////////////////////////////////////////
//
// PRIVATE METHODS IN "PCXTrans" CLASS
//
////////////////////////////////////////////////////////////////////////////////////////////
// Translate the PCX to a CS_RGA32 image
status_t PCXTrans :: _TranslatePCX()
{
	uint8 buf[ PCX_HEADER_SIZE ];

#ifdef DEBUG_PCX
dbprintf( "********** PCX Translate %d\n", m_cInBuffer.Size() );
#endif

	// Read header from file
	if( m_cInBuffer.Read( buf, PCX_HEADER_SIZE ) != PCX_HEADER_SIZE )
	{
		dbprintf( "Unvalid header (wrong size)\n" );
		return ERR_INVALID_DATA;
	}

	// Header for the bitmap
	PCXheader pcxheader;

	// Copy data from buffer to header
	pcxheader.manufacturer = buf[0];
	pcxheader.version = buf[ 1 ];
	pcxheader.encoding = buf[ 2 ];
	pcxheader.bitsperpixel = buf[ 3 ] ;
	pcxheader.xmin = _ConvertTo16bitSigned( &buf[ 4 ] );
	pcxheader.ymin = _ConvertTo16bitSigned( &buf[ 6 ] );
	pcxheader.xmax = _ConvertTo16bitSigned( &buf[ 8 ] );
	pcxheader.ymax = _ConvertTo16bitSigned( &buf[ 10 ] );
	pcxheader.horizdpi =  _ConvertTo16bit( &buf[ 12 ] );
	pcxheader.vertdpi =  _ConvertTo16bit( &buf[ 14 ] );
	memcpy( pcxheader.palette, &buf[ 16 ], 48 );
	pcxheader.reserved = buf[ 64 ];
	pcxheader.colorplanes = buf[ 65 ];
	pcxheader.bytesperline = _ConvertTo16bit( &buf[ 66 ] );
	pcxheader.palettetype = _ConvertTo16bit( &buf[ 68 ] );
	pcxheader.hscrsize = _ConvertTo16bit( &buf[ 70 ] );
	pcxheader.vscrsize = _ConvertTo16bit( &buf[ 72] );
	memcpy( pcxheader.filler, &buf[ 74 ], 54 );

	pcxheader.width = ( pcxheader.xmax - pcxheader.xmin ) + 1;
	pcxheader.height = ( pcxheader.ymax - pcxheader.ymin ) + 1;


	// Special trick
	if( pcxheader.version == 5 &&  pcxheader.bitsperpixel == 8 && pcxheader.colorplanes == 3 )
		pcxheader.bitsperpixel = 24;
	
	// Validate PCX
	if( pcxheader.width <= 0 || pcxheader.height <= 0 )
	{
		dbprintf( "Sanity check of the header failed (invalid size)\n" );
		return ERR_INVALID_DATA;
	}

	switch( pcxheader.bitsperpixel )
	{
		case 1:
			if( pcxheader.colorplanes < 1 || pcxheader.colorplanes > 4 )
			{
				dbprintf( "Sanity check of the header failed (invalid size)\n" );
				return ERR_INVALID_DATA;
			}
			break;
		case 2:
		case 4:
		case 8:
			if( pcxheader.colorplanes != 1 )
			{
				dbprintf( "Sanity check of the header failed (invalid size)\n" );
				return ERR_INVALID_DATA;
			}
			break;
		case 24:
			break;
		default:
			dbprintf( "Sanity check of the header failed (unsupported BPP)\n" );
			return ERR_INVALID_DATA;
	}

#ifdef DEBUG_PCX
dbprintf( "**********Version %d Image size %d %d Bitperpixel %d Compression %d BytesPerLine %d NumColourPlanes %d\n", pcxheader.version,  pcxheader.width, pcxheader.height, pcxheader.bitsperpixel, pcxheader.encoding, pcxheader.bytesperline, pcxheader.colorplanes );
#endif

	// Set up bitmap header
	m_sBitmapHeader.bh_header_size = sizeof( m_sBitmapHeader );
	m_sBitmapHeader.bh_data_size = 0;
	m_sBitmapHeader.bh_bounds = IRect( pcxheader.xmin, pcxheader.ymin, pcxheader.xmax, pcxheader.ymax );
	m_sBitmapHeader.bh_frame_count = 1;
	m_sBitmapHeader.bh_bytes_per_row = pcxheader.width * 4;
	m_sBitmapHeader.bh_color_space = CS_RGB32;  // The result is always a RGB32

	// Write bitmapheader to output buffer
	m_cOutBuffer.Write( &m_sBitmapHeader, sizeof( m_sBitmapHeader ) );

	// Set up bitmap frame header
	m_sCurrentFrame.bf_header_size   = sizeof(m_sCurrentFrame);
	m_sCurrentFrame.bf_time_stamp    = 0;
	m_sCurrentFrame.bf_color_space   = m_sBitmapHeader.bh_color_space;
	m_sCurrentFrame.bf_frame	 	 = m_sBitmapHeader.bh_bounds;
	m_sCurrentFrame.bf_bytes_per_row = m_sBitmapHeader.bh_bytes_per_row;
	m_sCurrentFrame.bf_data_size     = m_sBitmapHeader.bh_bytes_per_row * pcxheader.height;

	// Write frame header to output buffer
	m_cOutBuffer.Write( &m_sCurrentFrame, sizeof( m_sCurrentFrame ) );

	// Allocate memory for our scanlines
	m_ScanLines = new uint8[ pcxheader.colorplanes * pcxheader.bytesperline * 10];
	
	// Uncompress and write the image to the output buffer
	status_t ret = ERR_OK;
	switch( pcxheader.bitsperpixel )
	{
		case 1:
			ret = _Translate1( pcxheader );			
			break;
		case 2:
			ret = _Translate2( pcxheader );
			break;
		case 4:
			ret = _Translate4( pcxheader );
			break;
		case 8:			
			ret = _Translate8( pcxheader );
			break;
		case 24:
			ret = _Translate24( pcxheader );
			break;
	}

	// Free our scanlines
	delete m_ScanLines;

	return ret;
}

// Translate a 24 BPP PCX to CS_RGBA32
// in 24-bit images, each scanline has three color planes
// for red, green, and blue, respectively.
status_t  PCXTrans :: _Translate24( PCXheader& pcxheader )
{
#ifdef DEBUG_PCX
dbprintf( "*** Read 24 BPP image.\n" );
#endif

	uint i;
	uint8* planes[3];
	uint line_bytes;
	uint y = 0;

	planes[0] = m_ScanLines;
	planes[1] = planes[0] + pcxheader.bytesperline;
	planes[2] = planes[1] + pcxheader.bytesperline;

	while( _ReadScanlineData( pcxheader, planes, 3 ) )
	{
		for( i = 0 ; i < pcxheader.width ; i++)
		{
			uint8 red, green, blue, alpha;
	
			red = planes[ 0 ][ i ];
			green = planes[ 1 ][ i ];
			blue = planes[ 2 ][ i ];

			uint32 colour = ( 255 << 24 ) | ( red << 16 ) | ( green << 8 ) | ( blue );

			m_cOutBuffer.Write( &colour, sizeof( uint32 ) );
		}

		y++;

		if( y >= pcxheader.height)
			return ERR_OK;
	}

	return ERR_OK;
}

status_t  PCXTrans :: _Translate8( PCXheader& pcxheader )
{
#ifdef DEBUG_PCX
dbprintf( "*** Read 8 BPP image.\n" );
#endif

	uint i;
	uint8* planes[1];
	uint line_bytes;
	uint y = 0;
	uint c = 0 ;
	uint8* tmp_image;
	status_t ret = ERR_OK;

	planes[0] = m_ScanLines;

	tmp_image = new uint8[ pcxheader.bytesperline * pcxheader.height ];
	if( tmp_image == NULL )
	{
		dbprintf( "Unable to allocate enough memory\n" );
		return ERR_INVALID_DATA;
	}

	while( _ReadScanlineData( pcxheader, planes, 1 ) )
	{
		for( i = 0 ; i < pcxheader.width ; i++)
		{	
			tmp_image[ c ] = planes[ 0 ][ i ];
			c++;
		}

		y++;

		if( y >= pcxheader.height)
		{
			// Load and apply palette
			ret = _ApplyPalette8( pcxheader, tmp_image );
			break;
		}
	}

	delete tmp_image;

	return ret;
}

status_t  PCXTrans :: _Translate4( PCXheader& pcxheader )
{
#ifdef DEBUG_PCX
dbprintf( "*** Read 4 BPP image.\n" );
#endif

	uint i;
	uint8* planes[1];
	uint line_bytes;
	uint y = 0;

	planes[0] = m_ScanLines;

	while( _ReadScanlineData( pcxheader, planes, 1 ) )
	{
		for( i = 0 ; i < pcxheader.width ; i++)
		{
			uint8 p = _ReadPixel4( planes[0], i ) & 0xf;

			uint8 red = pcxheader.palette[ p * 3 ];
			uint8 green = pcxheader.palette[ p * 3 + 1];
			uint8 blue = pcxheader.palette[ p * 3 + 2 ];

			uint32 colour = ( 255 << 24 ) | ( red << 16 ) | ( green << 8 ) | ( blue );

			m_cOutBuffer.Write( &colour, sizeof( uint32 ) );
		}

		y++;

		if( y >= pcxheader.height)
			return ERR_OK;
	}

	return ERR_OK;
}

status_t  PCXTrans :: _Translate2( PCXheader& pcxheader )
{
#ifdef DEBUG_PCX
dbprintf( "*** Read 2 BPP image.\n" );
#endif

	uint i;
	uint8* planes[1];
	uint line_bytes;
	uint y = 0;

	planes[0] = m_ScanLines;

	while( _ReadScanlineData( pcxheader, planes, 1 ) )
	{
		for( i = 0 ; i < pcxheader.width ; i++)
		{
			uint shift = 6 - 2 * ( i % 4 );
			uint h = ( planes[ 0 ][ i / 4 ] >> shift ) & 0x3;

			uint8 red = pcxheader.palette[ h * 3 ];
			uint8 green = pcxheader.palette[ h * 3 + 1];
			uint8 blue = pcxheader.palette[ h * 3 + 2 ];

			uint32 colour = ( 255 << 24 ) | ( red << 16 ) | ( green << 8 ) | ( blue );

			m_cOutBuffer.Write( &colour, sizeof( uint32 ) );
		}

		y++;

		if( y >= pcxheader.height)
			return ERR_OK;
	}

	return ERR_OK;
}

status_t  PCXTrans :: _Translate1( PCXheader& pcxheader )
{
#ifdef DEBUG_PCX
dbprintf( "*** Read 1 BPP image.\n" );
#endif

	uint i;
	uint8* planes[4];
	uint line_bytes;
	uint y = 0;
	uint store_planes;

	switch( pcxheader.colorplanes )
	{
		case 4:
			planes[0] = m_ScanLines;
			planes[1] = planes[0] + pcxheader.bytesperline;
			planes[2] = planes[1] + pcxheader.bytesperline;
			planes[3] = planes[2] + pcxheader.bytesperline;
			store_planes = 4;
			break;
		case 3:
			planes[0] = m_ScanLines;
			planes[1] = planes[0] + pcxheader.bytesperline;
			planes[2] = planes[1] + pcxheader.bytesperline;
			store_planes = 3;
			break;
		case 2:
			planes[0] = m_ScanLines;
			planes[1] = planes[0] + pcxheader.bytesperline;
			store_planes = 2;
			break;
		case 1:
			planes[0] = m_ScanLines;		
			store_planes = 1;
			break;
		default:
			dbprintf( "Unsupported number of colour planes (%d)\n", pcxheader.colorplanes );
			return ERR_INVALID_DATA;
	}

	while( _ReadScanlineData( pcxheader, planes, store_planes ) )
	{
		for( i = 0 ; i < pcxheader.width ; i++)
		{
			uint8 p;

			switch( pcxheader.colorplanes )
			{
				case 4:
					p = _ReadPixel1( planes[3], i );
					p <<= 1;
					p |= _ReadPixel1( planes[2], i );
					p <<= 1;
					p |= _ReadPixel1( planes[1], i );
					p <<= 1;
					p |= _ReadPixel1( planes[0], i );
					break;
				case 3:
					p = _ReadPixel1( planes[2], i );
					p <<= 1;
					p |= _ReadPixel1( planes[1], i );
					p <<= 1;
					p |= _ReadPixel1( planes[0], i );
					break;
				case 2:
					p = _ReadPixel1( planes[1], i );
					p <<= 1;
					p |= _ReadPixel1( planes[0], i );
					break;
				case 1:
					p = _ReadPixel1( planes[0], i );
					break;
				default:
					dbprintf( "Unsupported number of colour planes (%d)\n", pcxheader.colorplanes );
					return ERR_INVALID_DATA;
			}

			p &= 0xf;
		
			uint8 red = pcxheader.palette[ p * 3 ];
			uint8 green = pcxheader.palette[ p * 3 + 1];
			uint8 blue = pcxheader.palette[ p * 3 + 2 ];

			uint32 colour = ( 255 << 24 ) | ( red << 16 ) | ( green << 8 ) | ( blue );

			m_cOutBuffer.Write( &colour, sizeof( uint32 ) );
		}

		y++;

		if( y >= pcxheader.height)
			return ERR_OK;
	}

	return ERR_OK;
}

// read each plane of a single scanline. store_planes is
// the number of planes that can be stored in the planes array.
// data is the pointer to the block of memory to read
// from, size is the length of that data, and line_bytes
// is where the number of bytes read will be stored.
bool PCXTrans :: _ReadScanlineData( PCXheader& pcxheader, uint8* planes[], uint store_planes )
{
	uint i, j;
	uint p, count;
	uint d = 0;
	uint8 byte;

	for( p = 0 ; p < pcxheader.colorplanes ; p++ )
	{
		for( i = 0 ; i < pcxheader.bytesperline ; ) // i incremented when line byte set
		{ 
			// Read byte
			if( m_cInBuffer.Read( &byte, 1 ) != 1 )
			{
				dbprintf( "Unable to read image data\n" );
				return false;
			}

			// Calculate number of following bytes
			if( byte >> 6 == 0x3 )
			{
				count = byte & ~(0x3 << 6);
				
				if( count == 0 )
				{
					dbprintf( "Unable to read image data (count == 0 )\n" );
					return false;
				}

				if( m_cInBuffer.Read( &byte, 1 ) != 1 )
				{
					dbprintf( "Unable to read image data\n" );
					return false;
				}
			} 
			else 
				count = 1;

			// Add the bytes
			for( j = 0; j < count ; j++ )
			{
				if( p < store_planes )
					planes[ p ][ i++ ] = byte;
				else
					i++;

				if( i >= pcxheader.bytesperline )
				{
					p++;
					if( p >= pcxheader.colorplanes )					
						return true;

					i = 0;
				}
			}
		}
	}

	return true;
}

// Read last palette and apply to image. Only valid for 8 BPP images
status_t PCXTrans :: _ApplyPalette8( PCXheader& pcxheader, uint8* tmp_image )
{
	uint8 magic;
	uint8 palette[ 3 * 256 ];

	if( m_cInBuffer.Read( &magic, 1 ) != 1 && magic != 0x12 )
	{
		dbprintf( "Unable to read palette data\n" );
		return ERR_INVALID_DATA;
	}

	// Read the palette
	if( m_cInBuffer.Read( palette, 256 * 3 ) != 256 * 3 )
	{
		dbprintf( "Unable to read palette data\n" );
		return ERR_INVALID_DATA;
	}

	// Apply the palette to the image
	for( int i = 0 ; i < pcxheader.width * pcxheader.height ; i++)
	{
		uint8 red, green, blue, alpha;
		uint8 pal_no = tmp_image[ i ];

		red = palette[ pal_no * 3 ];
		green = palette[ pal_no * 3 + 1 ];
		blue = palette[ pal_no * 3 + 2 ];

		uint32 colour = ( 255 << 24 ) | ( red << 16 ) | ( green << 8 ) | ( blue );

		m_cOutBuffer.Write( &colour, sizeof( uint32 ) );		
	}

	return ERR_OK;
}

uint8 PCXTrans :: _ReadPixel4( uint8* data, uint offset )
{
	uint8 retval;

	if( !( offset % 2 ) ) 
	{
		offset /= 2;
		retval = data[ offset ] >> 4;
	} 
	else 
	{
		offset--;
		offset /= 2;
		retval = data[ offset ] & 0xf;
	}

	return retval;
}

uint8 PCXTrans :: _ReadPixel1( uint8* data, uint offset)
{
	uint8 retval;
	uint bit_offset;

	if( !( offset % 8 ) )
	{
		offset /= 8;
		retval = data[ offset ] >> 7;
	} 
	else 
	{
		bit_offset = offset % 8;
		offset -= bit_offset;
		offset /= 8;
		retval = ( data[ offset ] >> ( 7 - bit_offset ) ) & 0x1;
	}

	return retval;
}

// Convert a sequence of bytes to a uint16
uint16 PCXTrans :: _ConvertTo16bit( uint8* src )
{
	return src[0] | (src[1] << 8);
}

// Convert a sequence of bytes to a int16
int16 PCXTrans :: _ConvertTo16bitSigned( uint8* src )
{
	return src[0] | (src[1] << 8);
}


//////////////////////////////////////////////////////////////////////////////////////////
//
// CLASS DEFINITION OF "PCXTransNode" (INCL. CODE)
//
//////////////////////////////////////////////////////////////////////////////////////////

class PCXTransNode : public TranslatorNode
{
public:
    virtual int Identify( const String& cSrcType, const String& cDstType, const void* pData, int nLen )
	{
		if ( nLen < 3 )
		{
			return( TranslatorFactory::ERR_NOT_ENOUGH_DATA );
		}

		uint8* buf = (uint8*)pData;

		if( buf[0] == 0x0A && buf[ 1 ] < 6 &&  buf[ 2 ] == 0x01 )
			return 0;

		return( TranslatorFactory::ERR_UNKNOWN_FORMAT );
    }

    virtual TranslatorInfo GetTranslatorInfo()
    {
		static TranslatorInfo sInfo = { "image/x-pcx", TRANSLATOR_TYPE_IMAGE, 1.0f };
		return( sInfo );
    }

    virtual Translator* CreateTranslator() 
	{
		return( new PCXTrans );
    }

private:
	// Convert a sequence of bytes to a uint16
	uint16 _ConvertTo16bit( uint8* src )
	{
		return src[0] | (src[1] << 8);
	}

	// Convert a sequence of bytes to a int16
	int16 _ConvertTo16bitSigned( uint8* src )
	{
		return src[0] | (src[1] << 8);
	}
};

//////////////////////////////////////////////////////////////////////////////////////////
//
//  C BINDINGS
//
//////////////////////////////////////////////////////////////////////////////////////////

extern "C" int get_translator_count()
{
    return( 1 );
}

extern "C" TranslatorNode* get_translator_node( int nIndex )
{
    static PCXTransNode sNode;
    return( &sNode );
}


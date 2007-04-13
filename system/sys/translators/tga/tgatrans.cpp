// TGATrans (C)opyright 2007 Jonas Jarvoll
//
// A TGA translator for the Syllable OS (www.syllable.org)
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

////////////////////////////////////////////////////////////////////////////////
//
// License from GTK Project
//
////////////////////////////////////////////////////////////////////////////////

/* -*- mode: C; c-file-style: "linux" -*- */
/* 
 * GdkPixbuf library - TGA image loader
 * Copyright (C) 1999 Nicola Girardi <nikke@swlibero.org>
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
 *
 */

/*
 * Some NOTES about the TGA loader (2001/06/07, nikke@swlibero.org)
 *
 * - The TGAFooter isn't present in all TGA files.  In fact, there's an older
 *   format specification, still in use, which doesn't cover the TGAFooter.
 *   Actually, most TGA files I have are of the older type.  Anyway I put the 
 *   struct declaration here for completeness.
 *
 * - Error handling was designed to be very paranoid.
 */

////////////////////////////////////////////////////////////////////////////////

// Suggestions and possible improvements:

////////////////////////////////////////////////////////////////////////////////
//
// INCLUDES
//
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <atheos/kdebug.h>
#include <util/circularbuffer.h>
#include <translation/translator.h>
#include <storage/memfile.h>


using namespace os;

//////////////////////////////////////////////////////////////////////////////////////////
//
// DEBUG
//
//////////////////////////////////////////////////////////////////////////////////////////

// Uncomment for debug info
//#define DEBUG_TGA

//////////////////////////////////////////////////////////////////////////////////////////
//
// TGA header structures
//
//////////////////////////////////////////////////////////////////////////////////////////

#define TGA_INTERLEAVE_MASK     0xc0
#define TGA_INTERLEAVE_NONE     0x00
#define TGA_INTERLEAVE_2WAY     0x40
#define TGA_INTERLEAVE_4WAY     0x80

#define TGA_ORIGIN_MASK         0x30
#define TGA_ORIGIN_RIGHT        0x10
#define TGA_ORIGIN_UPPER        0x20

enum {
	TGA_TYPE_NODATA = 0,
	TGA_TYPE_PSEUDOCOLOR = 1,
	TGA_TYPE_TRUECOLOR = 2,
	TGA_TYPE_GRAYSCALE = 3,
	TGA_TYPE_RLE_PSEUDOCOLOR = 9,
	TGA_TYPE_RLE_TRUECOLOR = 10,
	TGA_TYPE_RLE_GRAYSCALE = 11
};

#define LE16(p) ((p)[0] + ((p)[1] << 8))

typedef struct _TGAHeader TGAHeader;
typedef struct _TGAFooter TGAFooter;

typedef struct _TGAColormap TGAColormap;
typedef struct _TGAColor TGAColor;

typedef struct _TGAContext TGAContext;

struct _TGAHeader {
	uint8 infolen;
	uint8 has_cmap;
	uint8 type;
	
	uint8 cmap_start[2];
	uint8 cmap_n_colors[2];
	uint8 cmap_bpp;
	
	uint8 x_origin[2];
	uint8 y_origin[2];
	
	uint8 width[2];
	uint8 height[2];
	uint8 bpp;
	
	uint8 flags;
};

struct _TGAFooter {
	uint32 extension_area_offset;
	uint32 developer_directory_offset;

	/* Standard TGA signature, "TRUEVISION-XFILE.\0". */
	union {
		char sig_full[18];
		struct {
			char sig_chunk[16];
			char dot, null;
		} sig_struct;
	} sig;
};

struct _TGAColormap {
	int size;
	TGAColor *cols;
};

struct _TGAColor {
	uint8 r, g, b, a;
};

//////////////////////////////////////////////////////////////////////////////////////////
//
// The translator for TGA
//
//////////////////////////////////////////////////////////////////////////////////////////





//////////////////////////////////////////////////////////////////////////////////////////
//
// CLASS DEFINITION OF "TGATrans"
//
//////////////////////////////////////////////////////////////////////////////////////////

class TGATrans : public Translator
{
public:
    TGATrans();
    virtual ~TGATrans();

    virtual void     SetConfig( const Message& cConfig ) {}
    virtual status_t AddData( const void* pData, size_t nLen, bool bFinal );
    virtual ssize_t  AvailableDataSize();
    virtual ssize_t  Read( void* pData, size_t nLen );
    virtual void     Abort();
    virtual void     Reset();
   
private:
	status_t  _TranslateTGA();
	bool _LoadColourMap( TGAHeader& header );
	bool _ParseRowData( TGAHeader& header );
	bool _ParseRLEData( TGAHeader& header );

	void _FlipRow( TGAHeader& header, uint8* row );
	void _FlipVertically( TGAHeader& header );
	void _WriteRLEData( TGAColor* color, uint8 rle_count );

	bool _ParseDataForRowTruecolor( TGAHeader& header );
	bool _ParseDataForRowGrayscale( TGAHeader& header );
	bool _ParseDataForRowPseudocolor( TGAHeader& header );

	bool _ParseRLEDataPseudocolor( TGAHeader& header );
	bool _ParseRLEDataTruecolor( TGAHeader& header );
	bool _ParseRLEDataGrayscale( TGAHeader& header );

	BitmapHeader		m_sBitmapHeader;
    BitmapFrameHeader	m_sCurrentFrame;

	CircularBuffer		m_cOutBuffer;
	MemFile				m_cInBuffer;

	bool 				m_bEOF;				// True if we have all input data	

	bool				m_RunLengthEncoded; // True if the TGA is RLE coded
	uint8* 				m_Image;			// We store our unpack image before it will 
											// be sent out to the output buffer
	bool				m_SkippedInfo;
	TGAColormap			m_ColourMap;		// The colour map
	
	uint8*				m_pptr;				// Pointer to current row in the image
};

//////////////////////////////////////////////////////////////////////////////////////////
//
// PUBLIC METHODS IN "TGATrans" CLASS
//
//////////////////////////////////////////////////////////////////////////////////////////


// The constructor
TGATrans :: TGATrans()
{
	m_bEOF = false;
	m_Image = NULL;
	m_ColourMap.cols = NULL;
	m_SkippedInfo = false;
};

// The destructor
TGATrans :: ~TGATrans()
{
	delete m_Image;
	delete m_ColourMap.cols;
}

status_t TGATrans :: AddData( const void* pData, size_t nLen, bool bFinal )
{
    m_bEOF = bFinal;
    m_cInBuffer.Write( pData, nLen );
	
	// We have now a complete read image in memory. Its about time to start decoding it.
	if( bFinal )
		return _TranslateTGA();
	
    return( 0 );
}

ssize_t TGATrans :: AvailableDataSize()
{
	return( m_cOutBuffer.Size() );
}

ssize_t TGATrans :: Read( void* pData, size_t nLen )
{
	if( m_bEOF == false )
		return( ERR_SUSPENDED );

    return( m_cOutBuffer.Read( pData, nLen ) );
}

void TGATrans :: Abort()
{
}

void TGATrans :: Reset()
{
}

////////////////////////////////////////////////////////////////////////////////////////////
//
// PRIVATE METHODS IN "TGATrans" CLASS
//
////////////////////////////////////////////////////////////////////////////////////////////


// Translate the TGA to a CS_RGA32 image
status_t TGATrans :: _TranslateTGA()
{
#ifdef DEBUG_TGA
dbprintf( "********** TGA Translate\n" );
#endif

	// Start from the beginning
	m_cInBuffer.Seek( 0, SEEK_SET);
	
	// Read in TGA file header
	TGAHeader header;
	if( m_cInBuffer.Read( &header, sizeof( TGAHeader ) ) != sizeof( TGAHeader ) )
	{
		dbprintf( "Unable to read image header\n" );
		return ERR_INVALID_DATA;
	}

#ifdef DEBUG_TGA
	dbprintf( "infolen %d "
			  "has_cmap %d "
			  "type %d "
			  "cmap_start %d "
			  "cmap_n_colors %d "
			  "cmap_bpp %d "
			  "x %d y %d width %d height %d bpp %d "
			  "flags %#x\n",
			  header.infolen,
			  header.has_cmap,
			  header.type,
			  LE16(header.cmap_start),
			  LE16(header.cmap_n_colors),
			  header.cmap_bpp,
			  LE16(header.x_origin),
			  LE16(header.y_origin),
			  LE16(header.width),
			  LE16(header.height),
			  header.bpp,
			  header.flags);
#endif

	if( LE16( header.width ) == 0 || LE16( header.height ) == 0)
	{
		dbprintf( "TGA image has invalid dimensions\n" );
		return ERR_INVALID_DATA;
	}
	if( ( header.flags & TGA_INTERLEAVE_MASK ) != TGA_INTERLEAVE_NONE )
	{
		dbprintf( "TGA image type not supported 6\n" );
		return ERR_INVALID_DATA;
	}

	switch( header.type )
	{
		case TGA_TYPE_PSEUDOCOLOR:
		case TGA_TYPE_RLE_PSEUDOCOLOR:
			if( header.bpp != 8 )
			{
				dbprintf( "TGA image type not supported 1\n" );
				return ERR_INVALID_DATA;
			}
			break;
		case TGA_TYPE_TRUECOLOR:
			if( header.bpp != 16 && header.bpp != 24 && header.bpp != 32 ) 
			{
				dbprintf( "TGA image type not supported 2\n" );
				return ERR_INVALID_DATA;
			}			      
			break;
		case TGA_TYPE_RLE_TRUECOLOR:
			if( header.bpp != 16 && header.bpp != 24 && header.bpp != 32 ) 
			{
				dbprintf( "TGA image type not supported 3\n" );
				return ERR_INVALID_DATA;
			}			      
			break;
		case TGA_TYPE_GRAYSCALE:
		case TGA_TYPE_RLE_GRAYSCALE:
			if( header.bpp != 8 && header.bpp != 16)
			{
				dbprintf( "TGA image type not supported 4\n" );
				return ERR_INVALID_DATA;
			}
			break;
		default:
			dbprintf( "TGA image type not supported 5\n" );
			return ERR_INVALID_DATA;
	}

	m_RunLengthEncoded = (( header.type == TGA_TYPE_RLE_PSEUDOCOLOR)
					 		  	 || ( header.type == TGA_TYPE_RLE_TRUECOLOR )
		 						 || ( header.type == TGA_TYPE_RLE_GRAYSCALE ) );

	// Skip the info part
	m_cInBuffer.Seek( header.infolen, SEEK_CUR);

	// Read the palette (if there is one)
	if( header.has_cmap )
	{
		if( !_LoadColourMap( header ) )
			return ERR_INVALID_DATA;
	}

	// Create our bitmap
	m_Image = new uint8[ LE16( header.width ) * LE16( header.height ) * 4 ];
	if( !m_Image )
	{
		dbprintf( "Unable to allocate enough memory\n" );
		return ERR_INVALID_DATA;
	}

	if( header.flags & TGA_ORIGIN_UPPER || m_RunLengthEncoded )
		m_pptr = m_Image;
	else
		m_pptr = m_Image + ( LE16( header.height ) - 1) * ( 4 * LE16( header.width ) );

	
	if( m_RunLengthEncoded)
	{
		if( !_ParseRLEData( header ) )
			return ERR_INVALID_DATA;
	} 
	else 
	{
		if( !_ParseRowData( header ) )
			return ERR_INVALID_DATA;
	}

	// Set up bitmap header
	m_sBitmapHeader.bh_header_size = sizeof( m_sBitmapHeader );
	m_sBitmapHeader.bh_data_size = 0;
	m_sBitmapHeader.bh_bounds = IRect( 0, 0, LE16( header.width ) - 1, LE16( header.height ) -1 );
	m_sBitmapHeader.bh_frame_count = 1;
	m_sBitmapHeader.bh_bytes_per_row = LE16( header.width ) * 4;
	m_sBitmapHeader.bh_color_space = CS_RGB32;  // The result is always a RGB32

	// Write bitmapheader to output buffer
	m_cOutBuffer.Write( &m_sBitmapHeader, sizeof( m_sBitmapHeader ) );

	// Set up bitmap frame header
	m_sCurrentFrame.bf_header_size   = sizeof( m_sCurrentFrame );
	m_sCurrentFrame.bf_time_stamp    = 0;
	m_sCurrentFrame.bf_color_space   = m_sBitmapHeader.bh_color_space;
	m_sCurrentFrame.bf_frame	 	 = m_sBitmapHeader.bh_bounds;
	m_sCurrentFrame.bf_bytes_per_row = m_sBitmapHeader.bh_bytes_per_row;
	m_sCurrentFrame.bf_data_size     = m_sBitmapHeader.bh_bytes_per_row * LE16( header.height );

	// Write frame header to output buffer
	m_cOutBuffer.Write( &m_sCurrentFrame, sizeof( m_sCurrentFrame ) );

	// Write image data
	m_cOutBuffer.Write( m_Image, LE16( header.width ) * LE16( header.height ) * 4 );

	// Wow, we have sucessfully parsed a TGA!
	return ERR_OK;
}

bool TGATrans :: _LoadColourMap( TGAHeader& header )
{	
	m_ColourMap.size = LE16( header.cmap_n_colors );
	m_ColourMap.cols = new TGAColor[ m_ColourMap.size ];
	if( !m_ColourMap.cols )
	{
		dbprintf( "Cannot allocate colormap entries\n" );
		return false;
	}

	for( int n = 0; n < m_ColourMap.size; n++ )
	{
		if( ( header.cmap_bpp == 15 ) || ( header.cmap_bpp == 16 ) )
		{
			uint8 p0, p1;
			
			m_cInBuffer.Read( &p0, sizeof( uint8 ) );
			m_cInBuffer.Read( &p1, sizeof( uint8 ) );

			uint16 col = p0 + ( p1 << 8 );
			m_ColourMap.cols[ n ].r = ( col >> 7 ) & 0xf8;
			m_ColourMap.cols[ n ].g = ( col >> 2 ) & 0xf8;
			m_ColourMap.cols[ n ].b = col << 3;
			m_ColourMap.cols[ n ].a = 0xff;

		}
		else if( ( header.cmap_bpp == 24 ) || ( header.cmap_bpp == 32 ) )
		{
			uint8 p0, p1, p2, p3;
			
			m_cInBuffer.Read( &m_ColourMap.cols[ n ].b, sizeof( uint8 ) );
			m_cInBuffer.Read( &m_ColourMap.cols[ n ].g, sizeof( uint8 ) );
			m_cInBuffer.Read( &m_ColourMap.cols[ n ].r, sizeof( uint8 ) );

			if( header.cmap_bpp == 32 )
				m_cInBuffer.Read( &m_ColourMap.cols[ n ].a, sizeof( uint8 ) );
			else
				m_ColourMap.cols[ n ].a = 0xff;
		} 
		else 
		{
			dbprintf( "Unexpected bitdepth for colourmap entries\n" );
			return false;
		}
	}

	return true;
}

void TGATrans :: _FlipRow( TGAHeader& header, uint8* row )
{
	uint8 *p, *s;
	uint8 tmp;
	int count;

	p = row;
	s = p + 4 * ( LE16( header.width ) - 1 );
	while (p < s) 
	{
		for( count = 4; count > 0; count--, p++, s++ )
		{
			tmp = *p;
			*p = *s;
			*s = tmp;
		}
		s -= 2 * 4;
	}		
}

void TGATrans :: _FlipVertically( TGAHeader& header )
{
	uint8 *ph, *sh, *p, *s;
	uint8 tmp;
	int count;
	uint rowstride;

	rowstride = LE16( header.width ) * 4;
	ph = m_Image;
	sh = m_Image + LE16( header.height ) * rowstride;
	while( ph < sh - rowstride ) 
	{
		p = ph;
		s = sh - rowstride;
		for( count = 4 * LE16( header.width ); count > 0; count--, p++, s++ )
		{
			tmp = *p;
			*p = *s;
			*s = tmp;
		}
		sh -= rowstride;
		ph += rowstride;
	}
}


void TGATrans :: _WriteRLEData( TGAColor* color, uint8 rle_count )
{
	for (; rle_count; (rle_count)--)
	{
		memcpy( m_pptr, (uint8 *) color, 4 );
		m_pptr += 4;		
	}
}

bool TGATrans :: _ParseRLEData( TGAHeader& header )
{
	bool ret = false;

	switch( header.type )
	{
		case TGA_TYPE_RLE_PSEUDOCOLOR:
			ret = _ParseRLEDataPseudocolor( header );
			break;
		case TGA_TYPE_RLE_TRUECOLOR:
			ret  = _ParseRLEDataTruecolor( header );
			break;
		case TGA_TYPE_RLE_GRAYSCALE:
			ret = _ParseRLEDataGrayscale( header );
			break;
		default:
				dbprintf( "Unknown image type when parsing %d\n", header.type );
				ret = false;
				break;
	}

	if( ! ret )
		return false;
	
	if( header.flags & TGA_ORIGIN_RIGHT )
	{
		uint8* row = m_Image;

		for( uint i = 0 ; i < LE16( header.height ) ; i++, row += LE16( header.width ) * 4 )
			_FlipRow( header, row );
	}

	if( !( header.flags & TGA_ORIGIN_UPPER ) )
		_FlipVertically( header );

	return true;
}

bool TGATrans :: _ParseRLEDataPseudocolor( TGAHeader& header )
{
	int size;

	TGAColor col;
	uint rle_num, raw_num;
	uint8 tag;

	size = LE16( header.width ) * LE16( header.height ) * 4;
	for( uint n = 0 ; n < size ; )
	{		
		m_cInBuffer.Read( &tag, sizeof( uint8 ) );
		
		if (tag & 0x80)
		{			
			uint8 t;
			m_cInBuffer.Read( &t, sizeof( uint8 ) );

			rle_num = (tag & 0x7f) + 1;		

			n += rle_num * 4;

			_WriteRLEData( &m_ColourMap.cols[ t ], rle_num);
		} 
		else 
		{
			raw_num = tag + 1;

			for (; raw_num; raw_num--)
			{
				uint8 t;
				m_cInBuffer.Read( &t, sizeof( uint8 ) );

				m_pptr[ 0 ] = m_ColourMap.cols[ t ].r;
				m_pptr[ 1 ] = m_ColourMap.cols[ t ].g;
				m_pptr[ 2 ] = m_ColourMap.cols[ t ].b;
				m_pptr[ 3 ] = 255;					
				n += 4;
				m_pptr += 4;
			}
		}
	}
	
	return true;
}

bool TGATrans :: _ParseRLEDataTruecolor( TGAHeader& header )
{
	int size;

	TGAColor col;
	uint rle_num, raw_num;
	uint8 tag;

	size = LE16( header.width ) * LE16( header.height ) * 4;
	for( uint n = 0 ; n < size ; )
	{		
		m_cInBuffer.Read( &tag, sizeof( uint8 ) );
		
		if( tag & 0x80 )
		{
			rle_num = (tag & 0x7f) + 1;

			switch( header.bpp )
			{
				case 16:
				{
					uint16 s;
		
					m_cInBuffer.Read( &s, sizeof( uint16 ) );
		
					// ARRRRRGG GGGBBBBB
					col.b = ( (uint8) s & 0x1f ) << 3;  // Blue
					col.g = ( (uint8) ( s >> 2 ) ) & 0xf8; // Green
					col.r = ( (uint8) ( s >> 7 ) & 0xf8 );	// Red
					break;
				}
				case 24:	
				case 32:
				{
					m_cInBuffer.Read( &col.r, sizeof( uint8 ) );
					m_cInBuffer.Read( &col.g, sizeof( uint8 ) );
					m_cInBuffer.Read( &col.b, sizeof( uint8 ) );
					if( header.bpp == 32 )
						m_cInBuffer.Read( &col.a, sizeof( uint8 ) );
					break;
				}
				default:
					dbprintf( "Unexpected bitdepth for gray colour image\n" );
					return false;
			}
			
			col.a = 255;

			n += rle_num * 4;

			_WriteRLEData( &col, rle_num);

		} 
		else 
		{
			raw_num = tag + 1;
			
			for (; raw_num; raw_num--)
			{
				switch( header.bpp )
				{
					case 16:
					{
						uint16 s;
		
						m_cInBuffer.Read( &s, sizeof( uint16 ) );

						// ARRRRRGG GGGBBBBB		
						m_pptr[ 0 ] = ( (uint8) s & 0x1f ) << 3;  // Blue
						m_pptr[ 1 ] = ( (uint8) ( s >> 2 ) ) & 0xf8; // Green
						m_pptr[ 2 ] = ( (uint8) ( s >> 7 ) & 0xf8 );	// Red
						break;
					}
					case 24:	
					case 32:
					{
						m_cInBuffer.Read( &m_pptr[0], sizeof( uint8 ) );
						m_cInBuffer.Read( &m_pptr[1], sizeof( uint8 ) );
						m_cInBuffer.Read( &m_pptr[2], sizeof( uint8 ) );
						if( header.bpp == 32 )
							m_cInBuffer.Read( &m_pptr[3], sizeof( uint8 ) );
						break;
					}
					default:
						dbprintf( "Unexpected bitdepth for gray colour image\n" );
						return false;
				}

				m_pptr[3] = 255;
					
				n += 4;
				m_pptr += 4;
			}
				
		}
	}

	return true;
}

bool TGATrans :: _ParseRLEDataGrayscale( TGAHeader& header )
{
	int size;

	TGAColor tone;
	uint rle_num, raw_num;
	uint8 tag;

	size = LE16( header.width ) * LE16( header.height ) * 4;
	for( uint n = 0 ; n < size ; )
	{		
		m_cInBuffer.Read( &tag, sizeof( uint8 ) );
		
		if( tag & 0x80 )
		{
			uint t;

			rle_num = (tag & 0x7f) + 1;

			m_cInBuffer.Read( &t, sizeof( uint8 ) );
			tone.r = tone.g = tone.b = t;
			tone.a = 0xff;

			if( header.bpp == 32 )
				m_cInBuffer.Read( &t, sizeof( uint8 ) );
			
			n += rle_num * 4;

			_WriteRLEData( &tone, rle_num);
		} 
		else 
		{
			raw_num = tag + 1;
			
			for (; raw_num; raw_num--)
			{
				uint8 t;

				m_cInBuffer.Read( &t, sizeof( uint8 ) );
				
				m_pptr[ 0 ] = m_pptr[1] = m_pptr[2] = t;	

				if( header.bpp == 32 )
					m_cInBuffer.Read( &t, sizeof( uint8 ) );
			
				m_pptr[3] = 255;
					
				n += 4;
				m_pptr += 4;
			}
				
		}
	}

	return true;
}

bool TGATrans :: _ParseRowData( TGAHeader& header )
{
	bool ret =false;

	for( int completed_lines = 0 ; completed_lines < LE16( header.height ); completed_lines++ )
	{
		switch( header.type )
		{
			case TGA_TYPE_PSEUDOCOLOR:
				ret = _ParseDataForRowPseudocolor( header );
				break;
			case TGA_TYPE_TRUECOLOR:
				ret  = _ParseDataForRowTruecolor( header );
				break;
			case TGA_TYPE_GRAYSCALE:
				ret = _ParseDataForRowGrayscale( header );
				break;
			default:
				dbprintf( "Unknown image type when parsing\n" );
				ret = false;
				break;
		}
		
		if( ! ret )
			return false;

		if( header.flags & TGA_ORIGIN_RIGHT )
			_FlipRow( header, m_pptr );
		if( header.flags & TGA_ORIGIN_UPPER )
			m_pptr += LE16( header.width ) * 4;
		else
			m_pptr -= LE16( header.width ) * 4;
	}

	return true;
}

bool TGATrans :: _ParseDataForRowTruecolor( TGAHeader& header )
{	
	switch( header.bpp )
	{
		case 16:
		{
			for( int i = 0; i < LE16( header.width ) ; i++ )
			{
				uint16 s;
		
				m_cInBuffer.Read( &s, sizeof( uint16 ) );

				// ARRRRRGG GGGBBBBB
				m_pptr[ i * 4 ] = ( (uint8) s & 0x1f ) << 3;  // Blue
				m_pptr[ i * 4 + 1 ] = ( (uint8) ( s >> 2 ) ) & 0xf8; // Green
				m_pptr[ i * 4 + 2 ] = ( (uint8) ( s >> 7 ) & 0xf8 );	// Red
				m_pptr[ i * 4 + 3 ] = 0xff;
			}
			break;
		}
		case 24:
			for( int i = 0; i < LE16( header.width ) ; i++ )
			{
				m_cInBuffer.Read( &m_pptr[ i * 4 + 0 ], sizeof( uint8 ) );
				m_cInBuffer.Read( &m_pptr[ i * 4 + 1 ], sizeof( uint8 ) );
				m_cInBuffer.Read( &m_pptr[ i * 4 + 2 ], sizeof( uint8 ) );
				m_pptr[ i * 4 + 3 ] = 0xff;
			}
			break;
		case 32:
			// Read the row from the buffer
			m_cInBuffer.Read( m_pptr, LE16( header.width ) * 4 );

			// Set alpha channel correctly
			for( int i = 0 ; i< LE16( header.width ) ; i++)
				m_pptr[ i * 4 +3 ] = 0xff;
			
			break;
		default:
			dbprintf( "Unexpected bitdepth for true colour image\n" );
			return false;
	}
	return true;
}

bool TGATrans :: _ParseDataForRowGrayscale( TGAHeader& header )
{
	for( int i = 0; i < LE16( header.width ) ; i++ )
	{
		uint8 t;

		// Read the row from the buffer

		if( header.bpp == 8 )
		{
			m_cInBuffer.Read( &t, sizeof( uint8 ) );
			m_pptr[ i * 4 ] = t;
			m_pptr[ i * 4 + 1 ] = t;
			m_pptr[ i * 4 + 2 ] = t;
			m_pptr[ i * 4 + 3 ] = 255;
		}
		else if( header.bpp == 16 )
		{
			uint alpha;
			m_cInBuffer.Read( &t, sizeof( uint8 ) );
			m_cInBuffer.Read( &alpha, sizeof( uint8 ) );
			m_pptr[ i * 4 ] = t;
			m_pptr[ i * 4 + 1 ] = t;
			m_pptr[ i * 4 + 2 ] = t;
			m_pptr[ i * 4 + 3 ] = alpha;
		}
		else
		{
			dbprintf( "Unexpected bitdepth for gray colour image\n" );
			return false;
		}
	}

	return true;	
}	

bool TGATrans :: _ParseDataForRowPseudocolor( TGAHeader& header )
{
	for( int i = 0; i < LE16( header.width ) ; i++ )
	{
		uint8 t;

		// Read the row from the buffer
		m_cInBuffer.Read( &t, sizeof( uint8 ) );

		m_pptr[ i * 4 ] = m_ColourMap.cols[ t ].r;
		m_pptr[ i * 4 + 1 ] = m_ColourMap.cols[ t ].g;
		m_pptr[ i * 4 + 2 ] = m_ColourMap.cols[ t ].b;

		if( header.cmap_bpp == 32)
			m_pptr[ i * 4 + 3 ] = m_ColourMap.cols[ t ].a;
		else
			m_pptr[ i * 4 + 3 ] = 255;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// CLASS DEFINITION OF "TGATransNode" (INCL. CODE)
//
//////////////////////////////////////////////////////////////////////////////////////////

class TGATransNode : public TranslatorNode
{
public:
    virtual int Identify( const String& cSrcType, const String& cDstType, const void* pData, int nLen )
	{
		if ( nLen < 3 )
		{
			return( TranslatorFactory::ERR_NOT_ENOUGH_DATA );
		}

		const uint8* buf = (const uint8*)pData;

		// Check colourmap
		if( buf[ 1 ] > 1 )
			return TranslatorFactory::ERR_UNKNOWN_FORMAT;
                
		// Check image type
        if( ( buf[ 2 ] > 3 && buf[ 2 ] < 9 ) || buf[ 2 ] > 11 )
			return TranslatorFactory::ERR_UNKNOWN_FORMAT;

		return( 0 );
    }

    virtual TranslatorInfo GetTranslatorInfo()
    {
		static TranslatorInfo sInfo = { "image/x-targa", TRANSLATOR_TYPE_IMAGE, 1.0f };
		return( sInfo );
    }

    virtual Translator* CreateTranslator() 
	{
		return( new TGATrans );
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
    static TGATransNode sNode;
    return( &sNode );
}


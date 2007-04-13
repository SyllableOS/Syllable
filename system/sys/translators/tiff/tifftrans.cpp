// TIFFTrans (C)opyright 2007 Jonas Jarvoll
//
// A TIFF translator for the Syllable OS (www.syllable.org)
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
// License from Haiku team
//
////////////////////////////////////////////////////////////////////////////////

/*
 * Copyright 2003-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *              Michael Wilber
 *              Stephan AÃŸmus <stippi@yellowbites.com> (write support)
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

extern "C" {
	#include <tiffio.h>
}

using namespace os;

//////////////////////////////////////////////////////////////////////////////////////////
//
// DEBUG
//
//////////////////////////////////////////////////////////////////////////////////////////

// Uncomment for debug info
//#define DEBUG_TIFF

////////////////////////////////////////////////////////////////////////////////////////////
//
// C A L L B A C K   F U N C T I O N S   F O R   L I B T I F F
//
////////////////////////////////////////////////////////////////////////////////////////////

void tiff_warning_handler( const char *mod, const char *fmt, va_list ap )
{
	dbprintf( fmt, mod, ap );
	dbprintf( "\n" );
}

void tiff_error_handler( const char *mod, const char *fmt, va_list ap )
{
	dbprintf( fmt, mod, ap );
	dbprintf( "\n" );
}

tsize_t tiff_read_proc(thandle_t stream, tdata_t buf, tsize_t size)
{
	MemFile* file = dynamic_cast< MemFile* >( (MemFile*)stream );
	
	if( file == NULL )
	{
		dbprintf( "Error in tiff_read_proc\n" );
		return 0;
	}

	return file->Read( buf, size );
}

tsize_t tiff_write_proc(thandle_t stream, tdata_t buf, tsize_t size)
{
	return 0;
}

toff_t tiff_seek_proc(thandle_t stream, toff_t off, int whence)
{
	MemFile* file = dynamic_cast< MemFile* >( (MemFile*)stream );

	if( file == NULL )
	{
		dbprintf( "Error in tiff_seek_proc\n" );
		return 0;
	}

	int t = file->Seek( off, whence );	// Seek returns the previous position first
	return file->Seek( 0, SEEK_CUR );
}

int tiff_close_proc(thandle_t stream)
{
	MemFile* file = dynamic_cast< MemFile* >( (MemFile*)stream );

	if( file == NULL )
	{
		dbprintf( "Error in tiff_close_proc\n" );
		return 0;
	}

	file->Seek( 0, SEEK_SET );
    return 0;
}

toff_t tiff_size_proc(thandle_t stream)
{
	MemFile* file = dynamic_cast< MemFile* >( (MemFile*)stream );

	if( file == NULL )
	{
		dbprintf( "Error in tiff_size_proc\n" );
		return 0;
	}

    
	return 0;
}

int tiff_map_file_proc(thandle_t stream, tdata_t *pbase, toff_t *psize)
{
	return 0;
}

void tiff_unmap_file_proc(thandle_t stream, tdata_t base, toff_t size)
{
	return;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// CLASS DEFINITION OF "TIFFTrans"
//
//////////////////////////////////////////////////////////////////////////////////////////

class TIFFTrans : public Translator
{
public:
    TIFFTrans();
    virtual ~TIFFTrans();

    virtual void     SetConfig( const Message& cConfig ) {}
    virtual status_t AddData( const void* pData, size_t nLen, bool bFinal );
    virtual ssize_t  AvailableDataSize();
    virtual ssize_t  Read( void* pData, size_t nLen );
    virtual void     Abort();
    virtual void     Reset();
   
private:
	status_t  _TranslateTIFF();

	BitmapHeader		m_sBitmapHeader;
    BitmapFrameHeader	m_sCurrentFrame;

	CircularBuffer		m_cOutBuffer;
	MemFile				m_cInBuffer;

	TIFF*				m_TIFF;
	bool 				m_bEOF;				// True if we have all input data	

	TIFFErrorHandler	m_OrigErrorHandler;
	TIFFErrorHandler	m_OrigWarningHandler;
};

//////////////////////////////////////////////////////////////////////////////////////////
//
// PUBLIC METHODS IN "TIFFTrans" CLASS
//
//////////////////////////////////////////////////////////////////////////////////////////

// The constructor
TIFFTrans :: TIFFTrans()
{
	m_TIFF = NULL;
	m_bEOF = false;

	// We redirect any messages from libtiff to our kernel debug
	m_OrigErrorHandler = TIFFSetErrorHandler( tiff_error_handler );
	m_OrigWarningHandler = TIFFSetWarningHandler( tiff_warning_handler );
};

// The destructor
TIFFTrans :: ~TIFFTrans()
{
	// Close tiff
	if( m_TIFF != NULL )
		TIFFClose( m_TIFF );

	// Set the orginal error handlers
	TIFFSetErrorHandler( m_OrigErrorHandler );
	TIFFSetWarningHandler( m_OrigWarningHandler );
}

status_t TIFFTrans :: AddData( const void* pData, size_t nLen, bool bFinal )
{
    m_bEOF = bFinal;
    m_cInBuffer.Write( pData, nLen );
	
	// We have now a complete read image in memory. Its about time to start decoding it.
	if( bFinal )
		return _TranslateTIFF();
	
    return( 0 );
}

ssize_t TIFFTrans :: AvailableDataSize()
{
	return( m_cOutBuffer.Size() );
}

ssize_t TIFFTrans :: Read( void* pData, size_t nLen )
{
	if( m_bEOF == false )
		return( ERR_SUSPENDED );

    return( m_cOutBuffer.Read( pData, nLen ) );
}

void TIFFTrans :: Abort()
{
}

void TIFFTrans :: Reset()
{
}

////////////////////////////////////////////////////////////////////////////////////////////
//
// PRIVATE METHODS IN "TIFFTrans" CLASS
//
////////////////////////////////////////////////////////////////////////////////////////////


// Translate the TIFF to a CS_RGA32 image
status_t TIFFTrans :: _TranslateTIFF()
{
#ifdef DEBUG_TIFF
dbprintf( "********** TIFF Translate\n" );
#endif

	// Start from the beginning
	m_cInBuffer.Seek( 0, SEEK_SET);

	m_TIFF = TIFFClientOpen("TIFFTranslator", "r", &m_cInBuffer,
                tiff_read_proc, tiff_write_proc, tiff_seek_proc, tiff_close_proc,
                tiff_size_proc, tiff_map_file_proc, tiff_unmap_file_proc); 
    if( !m_TIFF )
	{
		dbprintf( "Unable to open file\n" );
		return ERR_INVALID_DATA;;
	}
	
	uint32 width = 0, height = 0;
	if( !TIFFGetField( m_TIFF, TIFFTAG_IMAGEWIDTH, &width))
	{
		dbprintf( "Cannot find width of image\n" );
		return ERR_INVALID_DATA;;
	}
	if( !TIFFGetField( m_TIFF, TIFFTAG_IMAGELENGTH, &height ) )
	{
		dbprintf( "Cannot find height of image\n" );
		return ERR_INVALID_DATA;;
	}
#ifdef DEBUG_TIFF
	dbprintf( "TIFF image: %d %d\n", width, height );
#endif
	if( width == 0 || height == 0 )
	{
		dbprintf( "Invalid size of image\n" );
		return ERR_INVALID_DATA;
	}

	size_t npixels = width * height;
	uint32* praster = static_cast<uint32 *>( _TIFFmalloc( npixels * 4 ) );
	if( praster == NULL )
	{
		dbprintf( "Out of memory error\n" );
		return ERR_INVALID_DATA;;
	}
	if( !TIFFReadRGBAImage( m_TIFF, width, height, praster, 0 ) )
	{
		_TIFFfree( praster );
		dbprintf( "Unable to read image\n" );
		return ERR_INVALID_DATA;;
	}

	// Set up bitmap header
	m_sBitmapHeader.bh_header_size = sizeof( m_sBitmapHeader );
	m_sBitmapHeader.bh_data_size = 0;
	m_sBitmapHeader.bh_bounds = IRect( 0, 0, width - 1, height -1 );
	m_sBitmapHeader.bh_frame_count = 1;
	m_sBitmapHeader.bh_bytes_per_row = width * 4;
	m_sBitmapHeader.bh_color_space = CS_RGB32;  // The result is always a RGB32

	// Write bitmapheader to output buffer
	m_cOutBuffer.Write( &m_sBitmapHeader, sizeof( m_sBitmapHeader ) );

	// Set up bitmap frame header
	m_sCurrentFrame.bf_header_size   = sizeof( m_sCurrentFrame );
	m_sCurrentFrame.bf_time_stamp    = 0;
	m_sCurrentFrame.bf_color_space   = m_sBitmapHeader.bh_color_space;
	m_sCurrentFrame.bf_frame	 	 = m_sBitmapHeader.bh_bounds;
	m_sCurrentFrame.bf_bytes_per_row = m_sBitmapHeader.bh_bytes_per_row;
	m_sCurrentFrame.bf_data_size     = m_sBitmapHeader.bh_bytes_per_row * height;

	// Write frame header to output buffer
	m_cOutBuffer.Write( &m_sCurrentFrame, sizeof( m_sCurrentFrame ) );

	// Convert raw RGBA data to B_RGBA32 colorspace and write out the results
	uint8 *pbitsrow = new uint8[ width * 4 ];
	if( pbitsrow == NULL )
	{
		dbprintf( "Out of memory error\n" );
		return ERR_INVALID_DATA;;
	}

	uint8 *pras8 = reinterpret_cast<uint8 *>( praster );
	for( uint32 i = 0; i < height; i++ )
	{
		uint8 *pbits, *prgba;
		pbits = pbitsrow;
		prgba = pras8 + ((height - (i + 1)) * width * 4);
                                        
		for( uint32 k = 0; k < width; k++ )
		{
			pbits[0] = prgba[2];
			pbits[1] = prgba[1];
			pbits[2] = prgba[0];
			pbits[3] = prgba[3];
			pbits += 4;
			prgba += 4;
		}

		// Write row to output buffer
		m_cOutBuffer.Write( pbitsrow, width * 4 );
	}
	delete[] pbitsrow;
    
	_TIFFfree( praster );

	// Wow, we have sucessfully parsed a TIFF!
	return ERR_OK;
}


//////////////////////////////////////////////////////////////////////////////////////////
//
// CLASS DEFINITION OF "TIFFTransNode" (INCL. CODE)
//
//////////////////////////////////////////////////////////////////////////////////////////

class TIFFTransNode : public TranslatorNode
{
public:
    virtual int Identify( const String& cSrcType, const String& cDstType, const void* pData, int nLen )
	{
		if ( nLen < 4 )
		{
			return( TranslatorFactory::ERR_NOT_ENOUGH_DATA );
		}

		const uint8* p = (const uint8*)pData;

		if( ( p[0] == 'M' && p[1] == 'M' && p[ 2 ] == 0x00 && p[ 3 ] == 0x2A ) || 
			( p[0] == 'I' && p[1] == 'I' && p[ 2 ] == 0x2A && p[ 3 ] == 0x00 ) )
		{
			return 0;
		}

		return( TranslatorFactory::ERR_UNKNOWN_FORMAT );
    }

    virtual TranslatorInfo GetTranslatorInfo()
    {
		static TranslatorInfo sInfo = { "image/tiff", TRANSLATOR_TYPE_IMAGE, 1.0f };
		return( sInfo );
    }

    virtual Translator* CreateTranslator() 
	{
		return( new TIFFTrans );
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
    static TIFFTransNode sNode;
    return( &sNode );
}


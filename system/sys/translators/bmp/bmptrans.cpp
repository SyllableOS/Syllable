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

// Inspiration and code copying from OpenBeos project and
// GdkPixbuf library:

////////////////////////////////////////////////////////////////////////////////
//
// License from OpenBEos Project
//
////////////////////////////////////////////////////////////////////////////////

// BMPTranslator
// BMPTranslator.cpp
//
// This BTranslator based object is for opening and writing BMP files.
//
//
// Copyright (c) 2002 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

////////////////////////////////////////////////////////////////////////////////
//
// License from The Free Software Foundation
//
////////////////////////////////////////////////////////////////////////////////

// GdkPixbuf library - Windows Bitmap image loader
//
// Copyright (C) 1999 The Free Software Foundation
//
// Authors: Arjan van de Ven <arjan@fenrus.demon.nl>
//          Federico Mena-Quintero <federico@gimp.org>
//
// Based on io-ras.c
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.

////////////////////////////////////////////////////////////////////////////////

// Suggestions and possible improvements:
// - The image is decoded in a large memory buffer because it needs
//   to be flip before send to the circular output buffer
// - The image is completly parsed before writing anything to the
//   output buffer
// - Skip the OS/2 header when parsing and use the MS header directly
// - How to handle license when the source itself is a mixture from
//   different sources?


// Phew, finally here starts the code!

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

#include <vector>
#include <new>

using namespace os;

//////////////////////////////////////////////////////////////////////////////////////////
//
// DEBUG
//
//////////////////////////////////////////////////////////////////////////////////////////

// Uncomment for debug info
// #define DEBUG_BMP

//////////////////////////////////////////////////////////////////////////////////////////
//
// BMP header structures
//
//////////////////////////////////////////////////////////////////////////////////////////

struct BMPFileHeader {
	// for both MS and OS/2 BMP formats
	uint16 magic;                   // = 'BM'
	uint32 fileSize;                // file size in bytes
	uint32 reserved;                // equals 0
	uint32 dataOffset;              // file offset to actual image
};

struct MSInfoHeader 
{
	uint32 size;                    // size of this struct (40)
	uint32 width;                   // bitmap width
	uint32 height;                  // bitmap height
	uint16 planes;                  // number of planes, always 1?
	uint16 bitsperpixel;    		// bits per pixel, (1,4,8,16 or 24)
	uint32 compression;             // type of compression
	uint32 imagesize;               // size of image data if compressed
	uint32 xpixperm;                // horizontal pixels per meter
	uint32 ypixperm;                // vertical pixels per meter
	uint32 colorsused;              // number of actually used colors
	uint32 colorsimportant; 		// number of important colors, zero = all
};

struct OS2InfoHeader 
{
	uint32 size;                    // size of this struct (12)
	uint16 width;                   // bitmap width
	uint16 height;                  // bitmap height
	uint16 planes;                  // number of planes, always 1?
	uint16 bitsperpixel;    		// bits per pixel, (1,4,8,16 or 24)
};

//////////////////////////////////////////////////////////////////////////////////////////
//
// The translator for BMP
//
//////////////////////////////////////////////////////////////////////////////////////////

// Makes code easier to read
#define BMP_NO_COMPRESS 0
#define BMP_RLE8_COMPRESS 1
#define BMP_RLE4_COMPRESS 2

//////////////////////////////////////////////////////////////////////////////////////////
//
// CLASS DEFINITION OF "BMPTrans"
//
//////////////////////////////////////////////////////////////////////////////////////////

class BMPTrans : public Translator
{
public:
    BMPTrans();
    virtual ~BMPTrans();

    virtual void     SetConfig( const Message& cConfig ) {}
    virtual status_t AddData( const void* pData, size_t nLen, bool bFinal );
    virtual ssize_t  AvailableDataSize();
    virtual ssize_t  Read( void* pData, size_t nLen );
    virtual void     Abort();
    virtual void     Reset();
   
private:
	status_t  _TranslateBMP();
	uint16 _ConvertTo16bit( uint8* src );
	uint32 _ConvertTo32bit( uint8* src );
	status_t _TranslateFromBMPNoPalette( uint8* image, struct MSInfoHeader& msheader );
	status_t _TranslateFromBMPPalette( uint8* image, struct MSInfoHeader& msheader );
	status_t _TranslateFromBMPPalette_RLE( uint8* image, struct MSInfoHeader& msheader );
	int32 _GetRowBytes( uint32 width, uint16 bitsperpixel );
	int32 _GetPadding( uint32 width, uint16 bitsperpixel );
	void _Pixelcpy( uint8* dest, uint32 pixel, uint32 count );

	BitmapHeader		m_sBitmapHeader;
    BitmapFrameHeader	m_sCurrentFrame;

	CircularBuffer		m_cOutBuffer;
	CircularBuffer		m_cInBuffer;

	bool 				m_bEOF;				// True if we have all input data
	uint8 				m_Palette[ 1024 ];	// Place to save palette (if needed)
	bool				m_MSFormat;			// Set to true if the BMP is in MS format
	int					m_Pos2Skip;			// Number of bytes to skip (if BMP is on OS/2 format)
	
};

//////////////////////////////////////////////////////////////////////////////////////////
//
// PUBLIC METHODS IN "BMPTrans" CLASS
//
//////////////////////////////////////////////////////////////////////////////////////////


// The constructor
BMPTrans :: BMPTrans()
{
	m_bEOF = false;
};

// The destructor
BMPTrans :: ~BMPTrans()
{
}

status_t BMPTrans :: AddData( const void* pData, size_t nLen, bool bFinal )
{
    m_bEOF = bFinal;
    m_cInBuffer.Write( pData, nLen );
	
	// We have now a complete read image in memory. Its about time to start decoding it.
	if( bFinal )
		return _TranslateBMP();
	
    return( 0 );
}

ssize_t BMPTrans :: AvailableDataSize()
{
	return( m_cOutBuffer.Size() );
}

ssize_t BMPTrans :: Read( void* pData, size_t nLen )
{
	if( m_bEOF == false )
		return( ERR_SUSPENDED );

    return( m_cOutBuffer.Read( pData, nLen ) );
}

void BMPTrans :: Abort()
{
}

void BMPTrans :: Reset()
{
}

////////////////////////////////////////////////////////////////////////////////////////////
//
// PRIVATE METHODS IN "BMPTrans" CLASS
//
////////////////////////////////////////////////////////////////////////////////////////////

// Translates a non-palette BMP from input buffer to the CS_RGB32
// bits format.
status_t BMPTrans :: _TranslateFromBMPNoPalette( uint8* image, struct MSInfoHeader& msheader )
{
	int32 bitsRowBytes = msheader.width * 4;
	int32 bmpBytesPerPixel = msheader.bitsperpixel / 8;
	int32 bmpRowBytes = _GetRowBytes( msheader.width, msheader.bitsperpixel );
        
	// allocate row buffers
	uint8* bmpRowData = new uint8[ bmpRowBytes ];
	if( !bmpRowData )
	{
		dbprintf( "Unable to allocate enough memory\n" );
		return ERR_INVALID_DATA;
	}

	uint8 *bitsRowData = new uint8[ bitsRowBytes ];
	if( !bitsRowData )
	{
		delete[] bmpRowData;
		bmpRowData = NULL;
		dbprintf( "Unable to allocate enough memory\n" );
		return ERR_INVALID_DATA;
	}

	// perform the actual translation
	uint32 bmppixrow = 0;
	memset( bitsRowData, 0xff, bitsRowBytes );
	ssize_t rd = m_cInBuffer.Read( bmpRowData, bmpRowBytes ); 
	while( rd == static_cast< ssize_t >( bmpRowBytes ) )
	{
		uint8* pBitsPixel = bitsRowData;
		uint8* pBmpPixel = bmpRowData;
	
		for( uint32 i = 0 ; i < msheader.width ; i++)
		{
			memcpy( pBitsPixel, pBmpPixel, 3 );
			pBitsPixel += 4;
			pBmpPixel += bmpBytesPerPixel;
		}
                                
		memcpy( image, bitsRowData, bitsRowBytes );                
		image += bitsRowBytes;

		bmppixrow++;
		// if I've read all of the pixel data, break
		// out of the loop so I don't try to read 
		// non-pixel data
		if( bmppixrow == msheader.height )
			break;
        
		rd = m_cInBuffer.Read( bmpRowData, bmpRowBytes );       		
	}
        
	delete[] bmpRowData;
	bmpRowData = NULL;
	delete[] bitsRowData;
	bitsRowData = NULL;

	return ERR_OK;
}

// Translates an uncompressed, palette BMP from input buffer to 
// the CS_RGB32 bits format.
status_t BMPTrans :: _TranslateFromBMPPalette( uint8* image, struct MSInfoHeader& msheader )
{
	uint16 pixelsPerByte = 8 / msheader.bitsperpixel;
	uint16 bitsPerPixel = msheader.bitsperpixel;
	uint8 palBytesPerPixel;

	if( m_MSFormat )
		palBytesPerPixel = 4;
	else
		palBytesPerPixel = 3;
        
	uint8 mask = 1;
	mask = (mask << bitsPerPixel) - 1;

	int32 bmpRowBytes = _GetRowBytes( msheader.width, msheader.bitsperpixel );
	uint32 bmppixrow = 0;
	int32 bitsRowBytes = msheader.width * 4;
        
	// allocate row buffers
	uint8 *bmpRowData = new uint8[ bmpRowBytes ];
	if( !bmpRowData )
	{
		dbprintf( "Unable to allocate enough memory\n" );
		return ERR_INVALID_DATA;
	}

	uint8 *bitsRowData = new uint8[ bitsRowBytes ];
	if( !bitsRowData )
	{
		delete[] bmpRowData;
		bmpRowData = NULL;
		dbprintf( "Unable to allocate enough memory\n" );
		return ERR_INVALID_DATA;
	}

	memset( bitsRowData, 0xff, bitsRowBytes );
	ssize_t rd = m_cInBuffer.Read( bmpRowData, bmpRowBytes );

	while( rd == static_cast<ssize_t>( bmpRowBytes ) ) 
	{
		for( uint32 i = 0; i < msheader.width; i++)
		{
			uint8 indices = ( bmpRowData + ( i / pixelsPerByte ) )[ 0 ];
			uint8 index;
			index = ( indices >> ( bitsPerPixel * ( ( pixelsPerByte - 1 ) - ( i % pixelsPerByte ) ) ) ) & mask;
			memcpy( bitsRowData + (i * 4), m_Palette + (index * palBytesPerPixel), 3);
		}
     
		memcpy( image, bitsRowData, bitsRowBytes );                
		image += bitsRowBytes;

		bmppixrow++;

		// if I've read all of the pixel data, break
		// out of the loop so I don't try to read 
		// non-pixel data
		if( bmppixrow == msheader.height )
			break;

		rd = m_cInBuffer.Read( bmpRowData, bmpRowBytes );
	}

	delete[] bmpRowData;
	bmpRowData = NULL;
	delete[] bitsRowData;
	bitsRowData = NULL;

	return ERR_OK;
}

// Translates an RLE compressed, palette BMP from the input buffer to 
// the CS_RGB32 bits format. Currently, this code is not as
// memory effcient as it could be. It assumes that the BMP
// from input buffer is relatively small.
status_t BMPTrans :: _TranslateFromBMPPalette_RLE( uint8* image, struct MSInfoHeader& msheader )
{
#ifdef DEBUG_BMP
dbprintf( "***** Read palette image with compression\n" );
#endif

	uint16 pixelsPerByte = 8 / msheader.bitsperpixel;
	uint16 bitsPerPixel = msheader.bitsperpixel;
	uint8 mask = ( 1 << bitsPerPixel ) - 1;

	uint8 count, indices, index;

	int32 bitsRowBytes = msheader.width * 4;

	uint8* bitsRowData = new uint8[ bitsRowBytes ];
	if( !bitsRowData )
	{
		dbprintf( "Unable to allocate enough memory\n" );
		return ERR_INVALID_DATA;
	}	

	memset( bitsRowData, 0xff, bitsRowBytes );
        
	uint32 bmppixcol = 0, bmppixrow = 0;
	uint32 defaultcolor = 0;
        
	memcpy(&defaultcolor, m_Palette, 4);

	ssize_t rd = m_cInBuffer.Read( &count, 1 );
	while( rd > 0 )
	{
		// repeated color
		if( count )
		{
			// abort if all of the pixels in the row
			// have already been drawn to
			if( bmppixcol == msheader.width )
			{
				rd = -1;
				break;
			}

			// if count is greater than the number of
			// pixels remaining in the current row, 
			// only process the correct number of pixels
			// remaining in the row
			if( count + bmppixcol > msheader.width )
				count = msheader.width - bmppixcol;
                        
			rd = m_cInBuffer.Read( &indices, 1 );
			if( rd != 1 )
			{
				rd = -1;
				break;
			}
			for( uint8 i = 0 ; i < count ; i++ )
			{
				index = ( indices >> ( bitsPerPixel * ( ( pixelsPerByte - 1 ) - ( i % pixelsPerByte ) ) ) ) & mask;
				memcpy( bitsRowData + ( bmppixcol * 4 ), m_Palette + ( index * 4 ), 3 );
				bmppixcol++;
			}		
		} 
		// special code
		else 
		{
			uint8 code;
			rd = m_cInBuffer.Read( &code, 1 );
			if( rd != 1 )
			{
				rd = -1;
				break;
			}

			switch( code )
			{
				// end of line
				case 0:
					// if there are columns remaing on this
					// line, set them to the color at index zero
					if( bmppixcol < msheader.width )
						_Pixelcpy( bitsRowData + ( bmppixcol * 4 ), defaultcolor, msheader.width - bmppixcol );

					memcpy( image, bitsRowData, bitsRowBytes );                
					image += bitsRowBytes;
	
					bmppixcol = 0;
					bmppixrow++;					
					break;
                                
				// end of bitmap                                
				case 1:
					// if at the end of a row
					if( bmppixcol == msheader.width )
					{
						memcpy( image, bitsRowData, bitsRowBytes );                
						image += bitsRowBytes;
							
						bmppixcol = 0;
						bmppixrow++;
					}
                                        
					while(bmppixrow < msheader.height )
					{
						_Pixelcpy( bitsRowData + ( bmppixcol * 4), defaultcolor, msheader.width - bmppixcol);
                                 
						memcpy( image, bitsRowData, bitsRowBytes );                
						image += bitsRowBytes;
	               	
						bmppixcol = 0;
						bmppixrow++;					
					}
                                        
					rd = 0;                   
					break;
                                
				// delta, skip several rows and/or columns and
				// fill the skipped pixels with the default color                               
				case 2:
				{
					uint8 da[ 2 ], lastcol, dx, dy;
					rd = m_cInBuffer.Read( da, 2 );
					if( rd != 2 )
					{
						rd = -1;
						break;
					}
					dx = da[ 0 ];
					dy = da[ 1 ];
                                        
					// abort if dx or dy is too large
					if( ( dx + bmppixcol >= msheader.width ) ||
						( dy + bmppixrow >= msheader.height) ) 
					{
						rd = -1;
						break;
					}       
                                                                
					lastcol = bmppixcol;
                                                                
					// set all pixels to the first entry in
					// the palette, for the number of rows skipped
					while( dy > 0 )
					{
						_Pixelcpy( bitsRowData + ( bmppixcol * 4 ), defaultcolor, msheader.width - bmppixcol);

						memcpy( image, bitsRowData, bitsRowBytes );                
						image += bitsRowBytes;

						bmppixcol = 0;
						bmppixrow++;
						dy--;
					}
                                                                
					if( bmppixcol < static_cast<uint32>( lastcol + dx ) ) 
					{
						_Pixelcpy( bitsRowData + ( bmppixcol * 4 ), defaultcolor, dx + lastcol - bmppixcol);
						bmppixcol = dx + lastcol;
					}
                                        
					break;
				}

				// code >= 3
				// read code uncompressed indices
				default:
					// abort if all of the pixels in the row
					// have already been drawn to
					if( bmppixcol == msheader.width )
					{
						rd = -1;
						break;
					}
                                        
					// if code is greater than the number of
					// pixels remaining in the current row, 
					// only process the correct number of pixels
					// remaining in the row
					if( code + bmppixcol > msheader.width )
						code = msheader.width - bmppixcol;
                                        
					uint8 uncomp[ 256 ];
					int32 padding;
					if( !( code % pixelsPerByte ) )
						padding = ( code / pixelsPerByte ) % 2;
					else
						padding = ( ( code + pixelsPerByte - ( code % pixelsPerByte ) ) / pixelsPerByte ) % 2;
					
					int32 uncompBytes = ( code / pixelsPerByte ) + ( ( code % pixelsPerByte ) ? 1 : 0 ) + padding;
					
					rd = m_cInBuffer.Read( uncomp, uncompBytes );
					if( rd != uncompBytes ) 
					{
						rd = -1;
						break;
					}
                                        
					for( uint8 i = 0 ; i < code ; i++ ) 
					{
						indices = ( uncomp + ( i / pixelsPerByte ) )[ 0 ];
						index = ( indices >> ( bitsPerPixel * ( ( pixelsPerByte - 1 ) - ( i % pixelsPerByte ) ) ) ) & mask;
						memcpy( bitsRowData + ( bmppixcol * 4 ), m_Palette + ( index * 4 ), 3 );
						bmppixcol++;
					}

					break;
			}
		}

		if( rd > 0 )
			rd = m_cInBuffer.Read( &count, 1 );
	}
        
	delete[] bitsRowData;
	bitsRowData = NULL;
        
	if( !rd )
		return ERR_OK;
	else              
	{
		dbprintf( "Error while parsing compressed BMP\n" );
		return ERR_INVALID_DATA;
	}
}

// Translate the BMP to a CS_RGA32 image
status_t BMPTrans :: _TranslateBMP()
{
	uint8 buf[ 40 ];

#ifdef DEBUG_BMP
dbprintf( "********** BMP Translate %d\n", m_cInBuffer.Size() );
#endif

	struct BMPFileHeader fileheader;

	// Load file header (14 bytes)
	if( m_cInBuffer.Read( buf, 14 ) == 14 )
	{
#ifdef DEBUG_BMP
		dbprintf( "******* BMP Read %d fileheader data\n",  14 );
#endif
		// Copy buffer to the header 
		// We cannot read directly into the header due to padding and alignment done by the compiler
		// Is it not possible to turn off? I have tried with -fpack-struct but that doesnt seem
		// to do any difference
		fileheader.magic = _ConvertTo16bit( buf );
		fileheader.fileSize = _ConvertTo32bit( &buf[ 2 ] );
		fileheader.reserved = _ConvertTo32bit( &buf[ 6 ] );
		fileheader.dataOffset = _ConvertTo32bit( &buf[ 10 ] );
	}
	else
	{
		dbprintf( "Unable to read file header\n" );
		return ERR_INVALID_DATA;
	}


	// check type of header (MS or OS/2)
	uint32 headersize;
	if( m_cInBuffer.Read( &headersize, sizeof( uint32 ) ) != sizeof( uint32 ) )
	{
		dbprintf( "Unable to read header size\n" );
		return ERR_INVALID_DATA;
	}

	// Header for the bitmap (used both with MS format and with OS/2 format)
	struct MSInfoHeader msheader;

	// MS header
	if( headersize == sizeof( struct MSInfoHeader ) ) // 40 bytes
	{
#ifdef DEBUG_BMP
		dbprintf( "****** BMP MS header\n" );
#endif

		// The normal header is 40 bytes large but since the size (first uint32) has already
		// been read we decrease the amount of data to read to 36 bytes
		if( m_cInBuffer.Read( &buf, 40 - 4 ) != ( 40 - 4 ) )
		{
			dbprintf( "Unable to read file MS header\n" );
			return ERR_INVALID_DATA;
		}
		
		msheader.size = headersize;		// This we have already read once
		
		// Copy data from buffer to header
		msheader.width = _ConvertTo32bit( buf );
		msheader.height = _ConvertTo32bit( &buf[ 4 ] );
		msheader.planes = _ConvertTo16bit( &buf[ 8 ] );
		msheader.bitsperpixel = _ConvertTo16bit( &buf[ 10 ] );
		msheader.compression = _ConvertTo32bit( &buf[ 12 ] );
		msheader.imagesize = _ConvertTo32bit( &buf[ 16 ] );
		msheader.xpixperm = _ConvertTo32bit( &buf[ 20 ] );
		msheader.ypixperm = _ConvertTo32bit( &buf[ 24 ] );
		msheader.colorsused = _ConvertTo32bit( &buf[ 28 ] );
		msheader.colorsimportant = _ConvertTo32bit( &buf[ 32 ] );

		// Make a sanity check of the header
		if( msheader.width == 0 || msheader.height == 0 )
		{
			dbprintf( "Sanity check of the MS header failed (invalid size)\n" );
			return ERR_INVALID_DATA;
		}
		if( msheader.planes != 1 )
		{
			dbprintf( "Sanity check of the MS header failed (invalid number of planes)\n" );
			return ERR_INVALID_DATA;
		}
		if( ( msheader.bitsperpixel != 1 || msheader.compression != BMP_NO_COMPRESS ) &&
			( msheader.bitsperpixel != 4 || msheader.compression != BMP_NO_COMPRESS ) &&
			( msheader.bitsperpixel != 4 ||	msheader.compression != BMP_RLE4_COMPRESS ) &&
			( msheader.bitsperpixel != 8 || msheader.compression != BMP_NO_COMPRESS ) &&
			( msheader.bitsperpixel != 8 || msheader.compression != BMP_RLE8_COMPRESS ) &&
			( msheader.bitsperpixel != 24 || msheader.compression != BMP_NO_COMPRESS ) &&
			( msheader.bitsperpixel != 32 || msheader.compression != BMP_NO_COMPRESS ) )
		{
			dbprintf( "Sanity check of the MS header failed (invalid type)\n" );
			return ERR_INVALID_DATA;
		}
		if( !msheader.imagesize && msheader.compression )
		{
			dbprintf( "Sanity check of the MS header failed (invalid image size)\n" );
			return ERR_INVALID_DATA;
		}
		if( msheader.colorsimportant > msheader.colorsused )
		{
			dbprintf( "Sanity check of the MS header failed (invalid nujmber of colours used)\n" );
			return ERR_INVALID_DATA; 
		}

		m_MSFormat = true;
		m_Pos2Skip = 0;	// We dont need to skip any bytes as this is MS format
	}

	// OS/2 header 
	else if( headersize == sizeof( struct OS2InfoHeader ) )  // 12 bytes
	{
#ifdef DEBUG_BMP
		dbprintf( "****** BMP OS/2 header\n" );
#endif

		if( fileheader.dataOffset < 26 )
		{
			dbprintf( "Sanity check of the OS/2 header failed (wrong data offset %d bytes)\n", fileheader.dataOffset );
			return ERR_INVALID_DATA; 
		}
                
		OS2InfoHeader os2header;
		
		if( m_cInBuffer.Read( buf, 8 ) != 8 )
		{
			dbprintf( "Unable to read header" );
			return ERR_INVALID_DATA;
		}

		// Copy buffer data to header
		os2header.size = headersize;							// size of this struct (12)
		os2header.width = _ConvertTo16bit( buf );				// bitmap width
		os2header.height = _ConvertTo16bit( &buf[ 2 ] );		// bitmap height
		os2header.planes = _ConvertTo16bit( &buf[ 4 ] );		// number of planes, always 1?
		os2header.bitsperpixel = _ConvertTo16bit( &buf[ 6 ] );	// bits per pixel, (1,4,8,16 or 24)

		// check if osheader is valid
		if( os2header.width == 0 || os2header.height == 0 )
		{
			dbprintf( "Sanity check of the OS/2 header failed (invalid size)\n" );
			return ERR_INVALID_DATA;
		}
		if( os2header.planes != 1 )
		{
			dbprintf( "Sanity check of the OS/2 header failed (invalid number of planes)\n" );
			return ERR_INVALID_DATA;
		}
		if( os2header.bitsperpixel != 1 && 
			os2header.bitsperpixel != 4 && 
			os2header.bitsperpixel != 8 
			&& os2header.bitsperpixel != 24 )
 		{
			dbprintf( "Sanity check of the OS/2 header failed (invalid type)\n" );
			return ERR_INVALID_DATA;
		}
               
		// Copy OS/2 header to the MS header instead
		fileheader.magic = (uint16) ( 'M' << 8 | 'B' );
		fileheader.fileSize = 0;
		fileheader.reserved = 0;
		fileheader.dataOffset = 0;

		msheader.size = 40;
		msheader.width = os2header.width;
		msheader.height = os2header.height;
		msheader.planes = 1;
		msheader.bitsperpixel = os2header.bitsperpixel;
		msheader.compression = BMP_NO_COMPRESS;
		msheader.imagesize = 0;
		msheader.xpixperm = 2835; // 72 dpi horizontal
		msheader.ypixperm = 2835; // 72 dpi vertical
		msheader.colorsused = 0;
		msheader.colorsimportant = 0;
                        
		// determine fileSize / imagesize
		switch( msheader.bitsperpixel)
		{
			case 24:
				if( fileheader.dataOffset > 26 )
					m_Pos2Skip = fileheader.dataOffset - 26;
                                                
				fileheader.dataOffset = 54;
				msheader.imagesize = _GetRowBytes( msheader.width, msheader.bitsperpixel ) * msheader.height;
				fileheader.fileSize = fileheader.dataOffset + msheader.imagesize;
				break;
                                
			case 8:
			case 4:
			case 1:
			{
				uint16 ncolors = 1 << msheader.bitsperpixel;
				msheader.colorsused = ncolors;
				msheader.colorsimportant = ncolors;
				if( fileheader.dataOffset > static_cast<uint32> ( 26 + ( ncolors * 3 ) ) )
					m_Pos2Skip = fileheader.dataOffset - ( 26 + ( ncolors * 3 ) );
				fileheader.dataOffset = 54 + ( ncolors * 4 );
				msheader.imagesize = _GetRowBytes( msheader.width, msheader.bitsperpixel ) * msheader.height;
				fileheader.fileSize = fileheader.dataOffset + msheader.imagesize;
				break;
			}
			default:
				dbprintf( "Unsupported bits per pixel format in OS/2 header\n" );
				return ERR_INVALID_DATA;		
		}
                
		m_MSFormat = false; // This a a OS/2 BMP
	}

	// Unsupported header size
	else
	{
		dbprintf( "Unsupported header size (%d bytes)\n", headersize  );
		return ERR_INVALID_DATA;	
	}

#ifdef DEBUG_BMP
dbprintf( "**********Image size %d %d Bitperpixel %d Compression %d\n", msheader.width, msheader.height, msheader.bitsperpixel, msheader.compression  );
#endif

	// Set up bitmap header
	m_sBitmapHeader.bh_header_size = sizeof( m_sBitmapHeader );
	m_sBitmapHeader.bh_data_size = 0;
	m_sBitmapHeader.bh_bounds = IRect( 0, 0, msheader.width - 1, msheader.height -1 );
	m_sBitmapHeader.bh_frame_count = 1;
	m_sBitmapHeader.bh_bytes_per_row = msheader.width * 4;
	m_sBitmapHeader.bh_color_space = CS_RGB32;  // The result is always a RGB32

	// Write bitmapheader to output buffer
	m_cOutBuffer.Write( &m_sBitmapHeader, sizeof( m_sBitmapHeader ) );

	// Set up bitmap frame header
	m_sCurrentFrame.bf_header_size   = sizeof(m_sCurrentFrame);
	m_sCurrentFrame.bf_time_stamp    = 0;
	m_sCurrentFrame.bf_color_space   = m_sBitmapHeader.bh_color_space;
	m_sCurrentFrame.bf_frame	 	 = m_sBitmapHeader.bh_bounds;
	m_sCurrentFrame.bf_bytes_per_row = m_sBitmapHeader.bh_bytes_per_row;
	m_sCurrentFrame.bf_data_size     = m_sBitmapHeader.bh_bytes_per_row * msheader.height;

	// Write frame header to output buffer
	m_cOutBuffer.Write( &m_sCurrentFrame, sizeof( m_sCurrentFrame ) );

	// Uncompress and write the image to the output buffer

	// Read palette and/or skip non-BMP data
	off_t nskip = 0;
	if( msheader.bitsperpixel == 1 || msheader.bitsperpixel == 4 || msheader.bitsperpixel == 8 )
	{              
		uint8 palBytesPerPixel;
		if( m_MSFormat )
			palBytesPerPixel = 4;
		else
			palBytesPerPixel = 3;
                        
		if( !msheader.colorsused )
			msheader.colorsused = 1 << msheader.bitsperpixel;
                        
		if( m_cInBuffer.Read( m_Palette, msheader.colorsused * palBytesPerPixel )  != msheader.colorsused * palBytesPerPixel )
		{
			dbprintf("Not enough data for palette\n" );
			return ERR_INVALID_DATA;
		}
                                
		// skip over non-BMP data
		if( m_MSFormat )
		{
			if( fileheader.dataOffset > ( msheader.colorsused * palBytesPerPixel ) + 54 )
				nskip = fileheader.dataOffset - ( ( msheader.colorsused * palBytesPerPixel ) + 54 );
		} 
		else
			nskip = m_Pos2Skip;
		} 
	else if( fileheader.dataOffset > 54 )
		// skip over non-BMP data
		nskip = fileheader.dataOffset - 54;
        
	// skip number of bytes                
	if( nskip > 0 )
	{
		dbprintf( "Skiping %d number of bytes\n", nskip );

		uint8* tmp = new uint8[ nskip ];
		if( m_cInBuffer.Read( tmp, nskip ) != nskip )
		{
			dbprintf( "Unable to skip %d bytes\n", nskip );
			delete tmp;
			return ERR_INVALID_DATA;
		}
		delete tmp;
	}

	// Temporary place to store image
	uint8* image = new uint8[ m_sCurrentFrame.bf_data_size ];
	if( !image )
	{
		dbprintf( "Unable to allocate enough memory\n" );
		return ERR_INVALID_DATA;
	}

	// Write the image data
	status_t ret;
	switch( msheader.bitsperpixel )
	{
		case 32:
		case 24:
			ret = _TranslateFromBMPNoPalette( image, msheader );
			break;
		case 8:
			// 8 bit BMP with NO compression
			if( msheader.compression == BMP_NO_COMPRESS )
				ret = _TranslateFromBMPPalette( image, msheader );

			// 8 bit RLE compressed BMP
			else if (msheader.compression == BMP_RLE8_COMPRESS)
				ret = _TranslateFromBMPPalette_RLE( image, msheader );
			
			// Unsupported
			else
			{
				delete image;
				dbprintf( "Unsupported 8 BPP format\n" );
				return ERR_INVALID_DATA;
			}
			break;
		case 4:
			// 4 bit BMP with NO compression
			if( !msheader.compression )
				ret = _TranslateFromBMPPalette( image, msheader );

			// 4 bit RLE compressed BMP
			else if( msheader.compression == BMP_RLE4_COMPRESS )
				ret = _TranslateFromBMPPalette_RLE( image, msheader );

            // Unsupported
			else
			{
				delete image;
				dbprintf( "Unsupported 4 BPP format\n" );
				return ERR_INVALID_DATA;
			}  
			break;
		case 1:
			ret = _TranslateFromBMPPalette( image, msheader );
			break;
                       
		default:
		{
			delete image;
			dbprintf( "Unsupported bits per pixel format\n" );
			return ERR_INVALID_DATA;
		
		}
	}

	if( ret != ERR_OK )
	{
		delete image;
		return ret;
	}

	// Flip image and write image to circular output buffer
	uint8* tmp = image + m_sCurrentFrame.bf_data_size - m_sCurrentFrame.bf_bytes_per_row;

	for( int y = 0 ; y < msheader.height ; y++, tmp -= m_sCurrentFrame.bf_bytes_per_row )
		m_cOutBuffer.Write( tmp, m_sCurrentFrame.bf_bytes_per_row );

	// Clean up
	delete image;

	// Wow, we have sucessfully parsed a BMP!
	return ERR_OK;
}

// Convert a sequence of bytes to a uint32
uint32 BMPTrans :: _ConvertTo32bit( uint8* src )
{
	return src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);
}

// Convert a sequence of bytes to a uint16
uint16 BMPTrans :: _ConvertTo16bit( uint8* src )
{
	return src[0] | (src[1] << 8);
}

// Returns number of bytes required to store a row of BMP pixels
// with a width of width and a bit depth of bitsperpixel.
int32 BMPTrans :: _GetRowBytes( uint32 width, uint16 bitsperpixel )
{
	int32 rowbytes = 0;
	int32 padding = _GetPadding( width, bitsperpixel );
        
	if( bitsperpixel > 8 )
	{
		uint8 bytesPerPixel = bitsperpixel / 8;
		rowbytes = (width * bytesPerPixel) + padding;
	} 
	else 
	{
		uint8 pixelsPerByte = 8 / bitsperpixel;
		rowbytes = (width / pixelsPerByte) + ( ( width % pixelsPerByte ) ? 1 : 0) + padding;
	}
        
	return rowbytes;
}

// Returns number of bytes of padding required at the end of
// the row by the BMP format
int32 BMPTrans :: _GetPadding( uint32 width, uint16 bitsperpixel )
{
	int32 padding = 0;
        
	if( bitsperpixel > 8 )
	{
		uint8 bytesPerPixel = bitsperpixel / 8;
		padding = (width * bytesPerPixel) % 4;
	} 
	else 
	{
		uint8 pixelsPerByte = 8 / bitsperpixel;
		if( !( width % pixelsPerByte ) )
			padding = (width / pixelsPerByte) % 4;
		else
			padding = ((width + pixelsPerByte - ( width % pixelsPerByte ) ) / pixelsPerByte) % 4;
	}
        
	if( padding )
		padding = 4 - padding;
        
	return padding;
}

// Copies count 32-bit pixels with a color value of pixel to dest.
void BMPTrans :: _Pixelcpy( uint8* dest, uint32 pixel, uint32 count )
{
	for( uint32 i = 0 ; i < count ; i++)
	{
		memcpy( dest, &pixel, 3 );
		dest += 4;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// CLASS DEFINITION OF "BMPTransNode" (INCL. CODE)
//
//////////////////////////////////////////////////////////////////////////////////////////

class BMPTransNode : public TranslatorNode
{
public:
    virtual int Identify( const String& cSrcType, const String& cDstType, const void* pData, int nLen )
	{
		if ( nLen < 2 )
		{
			return( TranslatorFactory::ERR_NOT_ENOUGH_DATA );
		}

		const uint8* p = (const uint8*)pData;
		if( p[0] == 'B' && p[1] == 'M' )
		{
	    	return( 0 );
		}
			
		return( TranslatorFactory::ERR_UNKNOWN_FORMAT );
    }

    virtual TranslatorInfo GetTranslatorInfo()
    {
		static TranslatorInfo sInfo = { "image/bmp", TRANSLATOR_TYPE_IMAGE, 1.0f };
		return( sInfo );
    }

    virtual Translator* CreateTranslator() 
	{
		return( new BMPTrans );
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
    static BMPTransNode sNode;
    return( &sNode );
}


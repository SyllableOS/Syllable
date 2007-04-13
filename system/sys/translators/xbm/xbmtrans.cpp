// XBMTrans (C)opyright 2007 Jonas Jarvoll
//
// A XBM translator for the Syllable OS (www.syllable.org)
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

// Based on the code in GdkPixbuf:

/* GdkPixbuf library - XBM image loader
 *
 * Copyright (C) 1999 Mark Crichton
 * Copyright (C) 1999 The Free Software Foundation
 * Copyright (C) 2001 Eazel, Inc.
 *
 * Authors: Mark Crichton <crichton@gimp.org>
 *          Federico Mena-Quintero <federico@gimp.org>
 *          Jonathan Blandford <jrb@redhat.com>
 *	    John Harper <jsh@eazel.com>
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
// - NBot tested with 16-bits (short) XBM


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
// DEFINES
//
//////////////////////////////////////////////////////////////////////////////////////////

// Uncomment for debug info
// #define DEBUG_XBM

#define BUFFER_EOF (-1)

#define LOTS_OF_DATA 300

//////////////////////////////////////////////////////////////////////////////////////////
//
// CLASS DEFINITION OF "XBMTrans"
//
//////////////////////////////////////////////////////////////////////////////////////////

class XBMTrans : public Translator
{
public:
    XBMTrans();
    virtual ~XBMTrans();

    virtual void     SetConfig( const Message& cConfig ) {}
    virtual status_t AddData( const void* pData, size_t nLen, bool bFinal );
    virtual ssize_t  AvailableDataSize();
    virtual ssize_t  Read( void* pData, size_t nLen );
    virtual void     Abort();
    virtual void     Reset();
   
private:
	void _InitHexTable(void);
	status_t  _TranslateXBM();
	char _ReadChar();
	bool _ReadLine( String& buffer );
	int _NextInt();
	void _WriteValue( char value, int bit_write );
	int _ConvertToValue( char ch );

	BitmapHeader		m_sBitmapHeader;
    BitmapFrameHeader	m_sCurrentFrame;

	CircularBuffer		m_cOutBuffer;
	MemFile				m_cInBuffer;

	bool 				m_bEOF;				// True if we have all input data
	char				m_HexTable[256];	// conversion value
};

//////////////////////////////////////////////////////////////////////////////////////////
//
// PUBLIC METHODS IN "XBMTrans" CLASS
//
//////////////////////////////////////////////////////////////////////////////////////////


// The constructor
XBMTrans :: XBMTrans()
{
	_InitHexTable();
	m_bEOF = false;
};

// The destructor
XBMTrans :: ~XBMTrans()
{
}

status_t XBMTrans :: AddData( const void* pData, size_t nLen, bool bFinal )
{
    m_bEOF = bFinal;
    m_cInBuffer.Write( pData, nLen );
	
	// We have now a complete read image in memory. Its about time to start decoding it.
	if( bFinal )
		return _TranslateXBM();
	
    return( 0 );
}

ssize_t XBMTrans :: AvailableDataSize()
{
	return( m_cOutBuffer.Size() );
}

ssize_t XBMTrans :: Read( void* pData, size_t nLen )
{
	if( m_bEOF == false )
		return( ERR_SUSPENDED );

    return( m_cOutBuffer.Read( pData, nLen ) );
}

void XBMTrans :: Abort()
{
}

void XBMTrans :: Reset()
{
}

////////////////////////////////////////////////////////////////////////////////////////////
//
// PRIVATE METHODS IN "XBMTrans" CLASS
//
////////////////////////////////////////////////////////////////////////////////////////////

// Translate the XBM to a CS_RGA32 image
status_t XBMTrans :: _TranslateXBM()
{
	String line;
	char name_and_type[1024];	// an input line
	char *type;					// for parsing 
	bool version10p;			// old format
	bool padding;				// to handle alignment
	uint ww = 0;				// width
	uint hh = 0;				// height
	int hx = -1;				// x hotspot
	int hy = -1;				// y hotspot
	int value;					// from an input line

#ifdef DEBUG_XBM
dbprintf( "********** XBM Translate\n" );
#endif

	m_cInBuffer.Seek( 0, SEEK_SET );

	while( _ReadLine( line ) )
	{
#ifdef DEBUG_XBM
dbprintf( "**** Read line \"%s\"\n", line.c_str() );
#endif
		if( sscanf(line.c_str(), "#define %s %d", name_and_type, &value) == 2 )
		{
			if( ! (type = strrchr( name_and_type, '_' ) ) )
				type = name_and_type;
			else
				type++;

			if( !strcmp( "width", type ) )
				ww = (unsigned int) value;
			if( !strcmp( "height", type ) )
				hh = (unsigned int) value;
			if( !strcmp( "hot", type ) )
			{
				if( type-- == name_and_type|| type-- == name_and_type)
					continue;
				if( !strcmp( "x_hot", type ) )
					hx = value;
				if( !strcmp( "y_hot", type ) )
					hy = value;
			}
			continue;
		}

		if( sscanf(line.c_str(), "static short %s = {", name_and_type ) == 1 )
			version10p = true;
		else if( sscanf( line.c_str(), "static unsigned char %s = {", name_and_type ) == 1 )
			version10p = false;
		else if( sscanf( line.c_str(), "static char %s = {", name_and_type ) == 1 )
			version10p = false;
		else
			continue;

		if( !( type = strrchr( name_and_type, '_' ) ) )
			type = name_and_type;
		else
			type++;

		if( strcmp( "bits[]", type ) )
			continue;

		// Check sanity of the header
		if( ww <= 0 || hh <= 0 )
		{
			dbprintf( "Sanity check of the XBM image failed (invalid size)\n" );
			return ERR_INVALID_DATA;
		}

		if( ( ww % 16 ) && ( ( ww % 16 ) < 9 ) && version10p )
			padding = true;
		else
			padding = false;

		
		break;
	}

#ifdef DEBUG_XBM
dbprintf( "**********Image size %d %d\n", ww, hh );
#endif


	// Set up bitmap header
	m_sBitmapHeader.bh_header_size = sizeof( m_sBitmapHeader );
	m_sBitmapHeader.bh_data_size = 0;
	m_sBitmapHeader.bh_bounds = IRect( 0, 0, ww - 1, hh -1 );
	m_sBitmapHeader.bh_frame_count = 1;
	m_sBitmapHeader.bh_bytes_per_row = ww * 4;
	m_sBitmapHeader.bh_color_space = CS_RGB32;  // The result is always a RGB32

	// Write bitmapheader to output buffer
	m_cOutBuffer.Write( &m_sBitmapHeader, sizeof( m_sBitmapHeader ) );

	// Set up bitmap frame header
	m_sCurrentFrame.bf_header_size   = sizeof(m_sCurrentFrame);
	m_sCurrentFrame.bf_time_stamp    = 0;
	m_sCurrentFrame.bf_color_space   = m_sBitmapHeader.bh_color_space;
	m_sCurrentFrame.bf_frame	 	 = m_sBitmapHeader.bh_bounds;
	m_sCurrentFrame.bf_bytes_per_row = m_sBitmapHeader.bh_bytes_per_row;
	m_sCurrentFrame.bf_data_size     = m_sBitmapHeader.bh_bytes_per_row * hh;

	// Write frame header to output buffer
	m_cOutBuffer.Write( &m_sCurrentFrame, sizeof( m_sCurrentFrame ) );

	// Parse the rest of the file (image data)
	int bytes_per_line = ( ww + 7 ) / 8;
	if( padding )
		bytes_per_line++;

	int size = bytes_per_line * hh;
	int bytes;	
	
	if( version10p )
	{
		int bytes;
		int width = ww;

		for( bytes = 0 ; bytes < size ; ( bytes += 2 ) )
		{
			if( ( value = _NextInt() ) < 0 )
			{
				dbprintf( "XBM loading failed\n" );
				return ERR_INVALID_DATA;
			}

			width -= 8;

			if( width < 0 )
				_WriteValue( value, 8+width );								
			else
				_WriteValue( value, 8 );				

			if( width <= 0 )
				width = ww;
			
			if( !padding || ( ( bytes + 2 ) % bytes_per_line ) )
			{
				width -= 8;
	
				if( width < 0 )
					_WriteValue( value >> 8, 8+width );								
				else
					_WriteValue( value >> 8, 8 );				

				if( width <= 0 )
					width = ww;
			}
		}
	} 
	else 
	{
		int bytes;
		int width = ww;

		for( bytes = 0 ; bytes < size ; bytes++ ) 
		{
			if( ( value = _NextInt() ) < 0) 
			{
				dbprintf( "XBM loading failed 2\n" );
				return ERR_INVALID_DATA;
			}

			width -= 8;

			if( width < 0 )
				_WriteValue( value, 8+width );								
			else
				_WriteValue( value, 8 );				

			if( width <= 0 )
				width = ww;
		}
	}

	// Wow, we have sucessfully parsed a XBM!
	return ERR_OK;
}

// Read single character from input buffer.
// Returns the character or EOF if end of file
char XBMTrans :: _ReadChar()
{
	char buf[ 1 ];

	if( m_cInBuffer.Read( &buf, 1 ) != 1 )
		return BUFFER_EOF;

	return buf[ 0 ];
}

// Read a complete string
bool XBMTrans :: _ReadLine( String& buffer )
{
	char c;
	uint cnt = 0;
	bool ret = false;

	buffer = "";

	c = _ReadChar();

	while( c != '\n' && c != BUFFER_EOF)
	{
		buffer += c;
		c = _ReadChar();
	}

	if( c == BUFFER_EOF )
		return false;

	return true;
}

// Read next hex value in the input stream, return -1 if EOF
int XBMTrans :: _NextInt()
{
	int ch;
	int value = 0;
	int gotone = 0;
	int done = 0;
    
	/* loop, accumulate hex value until find delimiter 
	   skip any initial delimiters found in read stream */

	while( !done ) 
	{
		ch = _ReadChar();
		if( ch == BUFFER_EOF )
		{
			value = -1;
			done++;
		} 
		else 
		{
			// trim high bits, check type and accumulate
			ch &= 0xff;
			if( isxdigit( ch ) )
			{
				value = (value << 4) + _ConvertToValue( ch );
				gotone++;
			} 
			else if( ( m_HexTable[ ch ] ) < 0 && gotone )
			{
				done++;
			}
		}
	}

	return value;
}


// Table index for the hex values. Initialized once, first time.
// Used for translation value or delimiter significance lookup.
void XBMTrans :: _InitHexTable(void)
{
	/*
	 * We build the table at run time for several reasons:
	 *
	 * 1. portable to non-ASCII machines.
	 * 2. still reentrant since we set the init flag after setting table.
	 * 3. easier to extend.
	 * 4. less prone to bugs.
	 */

#ifdef DEBUG_XBM
dbprintf( "Building XBM table\n" );
#endif

	m_HexTable['0'] = 0;
	m_HexTable['1'] = 1;
	m_HexTable['2'] = 2;
	m_HexTable['3'] = 3;
	m_HexTable['4'] = 4;
	m_HexTable['5'] = 5;
	m_HexTable['6'] = 6;
	m_HexTable['7'] = 7;
	m_HexTable['8'] = 8;
	m_HexTable['9'] = 9;
	m_HexTable['A'] = 10;
	m_HexTable['B'] = 11;
	m_HexTable['C'] = 12;
	m_HexTable['D'] = 13;
	m_HexTable['E'] = 14;
	m_HexTable['F'] = 15;
	m_HexTable['a'] = 10;
	m_HexTable['b'] = 11;
	m_HexTable['c'] = 12;
	m_HexTable['d'] = 13;
	m_HexTable['e'] = 14;
	m_HexTable['f'] = 15;

	/* delimiters of significance are flagged w/ negative value */
	m_HexTable[' '] = -1;
	m_HexTable[','] = -1;
	m_HexTable['}'] = -1;
	m_HexTable['\n'] = -1;
	m_HexTable['\t'] = -1;
}

void XBMTrans :: _WriteValue( char value, int bit_write )
{
	uint8 channel;
	uint32 colour;

	for( int bits = 0 ; bits < bit_write ; bits ++ )			
	{
		channel = (value & 1) ? 0 : 255;
	
		colour = ( 255 << 24 ) | ( channel << 16 ) | ( channel << 8 ) | ( channel );

		m_cOutBuffer.Write( &colour, sizeof( uint32 ) );

		value >>= 1;
	}
}

int XBMTrans :: _ConvertToValue( char ch )
{
	if( ch >= '0' && ch<='9' )
		return ch - '0';

	if( ch >= 'A' && ch <= 'F' )
		return ch - 'A' + 10;

	if( ch >= 'a' && ch <= 'f' )
		return ch - 'a' + 10;

	return BUFFER_EOF;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// CLASS DEFINITION OF "XBMTransNode" (INCL. CODE)
//
//////////////////////////////////////////////////////////////////////////////////////////

class XBMTransNode : public TranslatorNode
{
public:
    virtual int Identify( const String& cSrcType, const String& cDstType, const void* pData, int nLen )
	{
		// Convert the data to a char
		const char* p = (const char*)pData;

		// Count number of returns found in the data
		int no_ret = 0;
		
		for( int i = 0 ; i < nLen ; i++ )
		{
			// If we found a none-ascii character in the data we assume this is not an XBM file
			if( !isascii( p[i] ) )
				return TranslatorFactory::ERR_UNKNOWN_FORMAT;

			if( p[i] == '\n' )
				no_ret++;
		}

		// We  have found fewer than two returns => we have not enough data yet
		if( no_ret < 2 && nLen < LOTS_OF_DATA )
			return( TranslatorFactory::ERR_NOT_ENOUGH_DATA );

		// However if we have read a lot of data and still not found the returns
		// we assume this it no a XBM file
		else if( no_ret < 2 && nLen > LOTS_OF_DATA )
			return TranslatorFactory::ERR_UNKNOWN_FORMAT;

		// We have found two returns, lets check for '_width' and '_height' defines
		String g( p );
		if( g.find( "_width" ) == std::string::npos || 
			g.find( "_height" ) == std::string::npos ) 
		{
			// If we cannot find this we assume it is not a XBM image
			return TranslatorFactory::ERR_UNKNOWN_FORMAT;
		}

		// Check if we have also 'static' and '_bits[] = {' in the string
		if( g.find( "static" ) == std::string::npos || 
			g.find( "_bits[] = {" ) == std::string::npos ) 
		{
			// If we cannot find this we check the number of returns found in the image
			// if have several returns and still not the above defined or
			// we have read much data we assume it is not a XBM
			if( no_ret > 6 || nLen > LOTS_OF_DATA )
				return TranslatorFactory::ERR_UNKNOWN_FORMAT;
			else
				return( TranslatorFactory::ERR_NOT_ENOUGH_DATA );
		}

		// We have found defines for the width and the height of the image
		// and we have found static and _bits defined => We think this is
		// a XBM image
		return( 0 );
    }

    virtual TranslatorInfo GetTranslatorInfo()
    {
		static TranslatorInfo sInfo = { "image/x-xbitmap", TRANSLATOR_TYPE_IMAGE, 1.0f };
		return( sInfo );
    }

    virtual Translator* CreateTranslator() 
	{
		return( new XBMTrans );
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
    static XBMTransNode sNode;
    return( &sNode );
}


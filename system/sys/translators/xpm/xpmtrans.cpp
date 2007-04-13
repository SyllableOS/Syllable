// XPMTrans (C)opyright 2007 Jonas Jarvoll
//
// A XPM translator for the Syllable OS (www.syllable.org)
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

/* GdkPixbuf library - XPM image loader
 *
 * Copyright (C) 1999 Mark Crichton
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Mark Crichton <crichton@gimp.org>
 *          Federico Mena-Quintero <federico@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

////////////////////////////////////////////////////////////////////////////////

// Suggestions and possible improvements:
// - Might not need to store complete XPMColour in hashtable. Only the actual
//   colour (uint32)

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
#include <ctype.h>

#include <map>

using namespace os;
using namespace std;

//////////////////////////////////////////////////////////////////////////////////////////
//
// DEFINES
//
//////////////////////////////////////////////////////////////////////////////////////////

// Uncomment for debug info
// #define DEBUG_XPM

#define BUFFER_EOF (-1)

enum buf_op {
        op_header,
        op_cmap,
        op_body
};

#define MIN( x, y ) ( x < y ? x : y )

//////////////////////////////////////////////////////////////////////////////////////////
//
// XPM structures
//
//////////////////////////////////////////////////////////////////////////////////////////

typedef struct 
{
	String colour_string;
	uint16 red;
	uint16 green;
	uint16 blue;
	bool transparent;
	uint32 colour;
} XPMColour;
 
typedef struct {
    uint16 name_offset;
    uchar red;
    uchar green;
    uchar blue;
} XPMColorEntry;

//////////////////////////////////////////////////////////////////////////////////////////
//
// CLASS DEFINITION OF "XPMTrans"
//
//////////////////////////////////////////////////////////////////////////////////////////

class XPMTrans : public Translator
{
public:
    XPMTrans();
    virtual ~XPMTrans();

    virtual void     SetConfig( const Message& cConfig ) {}
    virtual status_t AddData( const void* pData, size_t nLen, bool bFinal );
    virtual ssize_t  AvailableDataSize();
    virtual ssize_t  Read( void* pData, size_t nLen );
    virtual void     Abort();
    virtual void     Reset();
   
private:
	status_t  _TranslateXPM();
	bool _Read( enum buf_op op, String& buffer );
	bool _SeekChar( char c );
	bool _SeekString( String find );
	char _ReadChar();
	bool _ReadString( String& buffer );
	bool _ExtractColour( const char* buffer, String& colour );
	bool _ParseColour( const char* spec, XPMColour* colour );
	bool _FindColour( const String name, XPMColour* colorPtr);
	
	BitmapHeader		m_sBitmapHeader;
    BitmapFrameHeader	m_sCurrentFrame;

	CircularBuffer		m_cOutBuffer;
	MemFile				m_cInBuffer;

	// List of all colours
	map< String, XPMColour* > m_ListOfColours;

	bool m_bEOF;

	// List of defined colours in xpm-color-table.h
	static const char* color_names[];
	static const XPMColorEntry xColors[];
};


#include "xpm-color-table.h" // XPM colour list

//////////////////////////////////////////////////////////////////////////////////////////
//
// PUBLIC METHODS IN "XPMTrans" CLASS
//
//////////////////////////////////////////////////////////////////////////////////////////


// The constructor
XPMTrans :: XPMTrans()
{
	m_bEOF = false;
};

// The destructor
XPMTrans :: ~XPMTrans()
{
#ifdef DEBUG_XPM
dbprintf( "Deleting hash table\n" );
#endif

	// Delete hash table
	map< String, XPMColour* >::iterator it;

	for( it = m_ListOfColours.begin() ; it != m_ListOfColours.end(); it++ )
		delete (*it).second;

	m_ListOfColours.clear();
}

status_t XPMTrans :: AddData( const void* pData, size_t nLen, bool bFinal )
{
    m_bEOF = bFinal;
    m_cInBuffer.Write( pData, nLen );
	
	// We have now a complete read image in memory. Its about time to start decoding it.
	if( bFinal )
		return _TranslateXPM();
	
    return( 0 );
}

ssize_t XPMTrans :: AvailableDataSize()
{
	return( m_cOutBuffer.Size() );
}

ssize_t XPMTrans :: Read( void* pData, size_t nLen )
{
	if( m_bEOF == false )
		return( ERR_SUSPENDED );

    return( m_cOutBuffer.Read( pData, nLen ) );
}

void XPMTrans :: Abort()
{
}

void XPMTrans :: Reset()
{
}

////////////////////////////////////////////////////////////////////////////////////////////
//
// PRIVATE METHODS IN "XPMTrans" CLASS
//
////////////////////////////////////////////////////////////////////////////////////////////

// Translate the XPM to a CS_RGA32 image
status_t XPMTrans :: _TranslateXPM()
{

#ifdef DEBUG_XPM
dbprintf( "** Starting decoding XPM image\n" );
#endif

	String buffer;
	XPMColour* fallbackcolour;

	if( ! _Read( op_header, buffer ) )
	{
		dbprintf( "Unable to read XPM header\n" );
		return ERR_INVALID_DATA;
	}

	int w, h, n_col, cpp, x_hot, y_hot, items;
	
	items = sscanf( buffer.c_str(), "%d %d %d %d %d %d", &w, &h, &n_col, &cpp, &x_hot, &y_hot);

	if( items != 4 && items != 6 )
	{
		dbprintf( "Invalid XPM header\n" );
		return ERR_INVALID_DATA;
	}

	// Check sanity of the header
	if( w <= 0 || h <= 0 )
	{
		dbprintf( "Sanity check of the XPM image failed (invalid size)\n" );
		return ERR_INVALID_DATA;
	}

	if( cpp <= 0 || cpp >= 32 )
	{
		dbprintf( "Sanity check of the XPM image failed (invalid number of characters per pixel)\n" );
		return ERR_INVALID_DATA;
	}

#ifdef DEBUG_XPM
	dbprintf( "*** Image Width: %d Height: %d\n", w, h );
#endif

	// Read colour data
	for( int cnt = 0; cnt < n_col; cnt++ )
	{
		String colour_name;
		XPMColour* colour;

		if( ! _Read( op_cmap, buffer ) )
		{
			dbprintf( "Unable to read XPM colourmap\n" );
			return ERR_INVALID_DATA;
		}

		// Create new XPM colour
		colour = new XPMColour;
		colour->transparent = false;

		colour->colour_string = buffer.substr( 0, cpp );

		if( !_ExtractColour( &buffer[ cpp + 1 ], colour_name ) || colour_name == "None"  ||
			!_ParseColour( colour_name.c_str(), colour ) )
		{
				colour->transparent = true;
				colour->red = 0;
				colour->green = 0;
				colour->blue = 0;
		}

#ifdef DEBUG_XPM
	dbprintf( "*** Colour string: \"%s\" %d %d %d\n", colour->colour_string.c_str(), colour->red, colour->green, colour->blue );
#endif

		// Convert to uint32
		uint8 red, green, blue, alpha;

		( colour->transparent ) ? alpha = 0 : alpha = 255;
		red = (uint8) ( ( (float) colour->red / 65535.0f ) * 255.0f );
		green = (uint8) ( ( (float) colour->green / 65535.0f ) * 255.0f );
		blue = (uint8) ( ( (float) colour->blue / 65535.0f ) * 255.0f );

		colour->colour = ( alpha << 24 ) | ( red << 16 ) | ( green << 8 ) | ( blue );

		if( cnt == 0 )
			fallbackcolour = colour;

		// Insert in hash table
		m_ListOfColours.insert( pair < String, XPMColour* >( colour->colour_string, colour ) );	
	}

	// Set up bitmap header
	m_sBitmapHeader.bh_header_size = sizeof( m_sBitmapHeader );
	m_sBitmapHeader.bh_data_size = 0;
	m_sBitmapHeader.bh_bounds = IRect( 0, 0, w - 1, h -1 );
	m_sBitmapHeader.bh_frame_count = 1;
	m_sBitmapHeader.bh_bytes_per_row = w * 4;
	m_sBitmapHeader.bh_color_space = CS_RGBA32;  // The result is always a RGBA32

	// Write bitmapheader to output buffer
	m_cOutBuffer.Write( &m_sBitmapHeader, sizeof( m_sBitmapHeader ) );

	// Set up bitmap frame header
	m_sCurrentFrame.bf_header_size   = sizeof( m_sCurrentFrame );
	m_sCurrentFrame.bf_time_stamp    = 0;
	m_sCurrentFrame.bf_color_space   = m_sBitmapHeader.bh_color_space;
	m_sCurrentFrame.bf_frame	 	 = m_sBitmapHeader.bh_bounds;
	m_sCurrentFrame.bf_bytes_per_row = m_sBitmapHeader.bh_bytes_per_row;
	m_sCurrentFrame.bf_data_size     = m_sBitmapHeader.bh_bytes_per_row * h;

	// Write frame header to output buffer
	m_cOutBuffer.Write( &m_sCurrentFrame, sizeof( m_sCurrentFrame ) );

	// Read image
	int wbytes = w * cpp;

	for( int ycnt = 0; ycnt < h; ycnt++ )
	{
		if( ! _Read( op_cmap, buffer ) || buffer.size() < wbytes )
			continue;
		
		int n, cnt, xcnt;

		for( n = 0, cnt = 0, xcnt = 0; n < wbytes; n += cpp, xcnt++ )
		{
			String pixel = buffer.substr( n, cpp );

			XPMColour* colour = m_ListOfColours[ pixel ];

			// Bad XPM...punt
			if( !colour )
			{
#ifdef DEBUG_XPM
				dbprintf( "Unable to get colour \"%s\"\n", pixel.c_str() );
#endif
				colour = fallbackcolour;
			}

			m_cOutBuffer.Write( &colour->colour, sizeof( uint32 ) );
		}
	}

	// Wow, we have sucessfully parsed a BMP!
	return ERR_OK;
}

// Seek a character in the input buffer. Skip any comments
// Returns TRUE if the character is found
bool XPMTrans :: _SeekChar( char c )
{
	char b, oldb;

	while( ( b = _ReadChar() ) != BUFFER_EOF )
	{
		if( c != b && b == '/' )
		{
			b = _ReadChar();

			if(b == BUFFER_EOF )
				return false;
			else if( b == '*' ) /* we have a comment */
			{    
				b = -1;
                                
				do
				{
					oldb = b;
					b = _ReadChar();
					if( b == BUFFER_EOF )
						return false;
				} while( !( oldb == '*' && b == '/' ) );
			}
		} 
		else if( c == b )
			return true;
	}

	return false;
}

// Seek a string in the input buffer.
// Returns TRUE if the string is found
bool XPMTrans :: _SeekString( String find )
{
	char buf[ 1024 ];
	int i;

	m_cInBuffer.Seek( 0, SEEK_SET);

	while( ( i = m_cInBuffer.Read( &buf, 1023 ) ) > 0 )
	{
		if( String( buf ).find( find ) >= 0 )
		{
			m_cInBuffer.Seek( 0, SEEK_SET);
			return true;
		}
	}

	m_cInBuffer.Seek( 0, SEEK_SET);
	return false;
}

// Read single character from input buffer.
// Returns the character or EOF if end of file
char XPMTrans :: _ReadChar()
{
	char buf[ 1 ];

	if( m_cInBuffer.Read( &buf, 1 ) != 1 )
		return BUFFER_EOF;
	
	return buf[ 0 ];
}

// Read a complete string
bool XPMTrans :: _ReadString( String& buffer )
{
	char c;
	uint cnt = 0;
	bool ret = false;

	buffer = "";

	do
	{
		c = _ReadChar();
	} while( c != BUFFER_EOF && c != '"' );

	if( c != '"' )
		return false;;

	while( ( c = _ReadChar() ) != BUFFER_EOF )
	{	
		if(c != '"' )
			buffer += c;
		else
			return true;
	}       

	return false;
}

bool XPMTrans :: _Read( enum buf_op op, String& buffer )
{
	switch( op )
	{
        case op_header:
			if( _SeekString( "XPM" ) != true )
				break;

			if( _SeekChar( '{' ) != true )
				break;
		
			// Fall through to the next xpm_seek_char.
        case op_cmap:
			if( _SeekChar( '"' ) != true )
				break;

			m_cInBuffer.Seek( -1, SEEK_CUR);

			// Fall through to the xpm_read_string.

        case op_body:			
		{
			if ( _ReadString( buffer ) );
				return true;
		}

	}

	return false;
}

// Extract colour string
bool XPMTrans :: _ExtractColour( const char* buffer, String& colour )
{

#ifdef DEBUG_XPM
dbprintf( "Extract colour \"%s\"\n", buffer );
#endif

	const char* p = &buffer[0];
	int new_key = 0;
	int key = 0;
	int current_key = 1;
	int space = 128;
	char word[ 129 ], color[ 129 ], current_color[ 129 ];
	char* r; 
        
	word[ 0 ] = '\0';
	color[ 0 ] = '\0';
	current_color[ 0 ] = '\0';

	while (1) 
	{
		// skip whitespace
		for( ; *p != '\0' && isspace (*p); p++ )
		{
        } 

		// copy word
		for( r = word; *p != '\0' && !isspace(*p) && r - word < sizeof(word) - 1; p++, r++ )
		{
			*r = *p;
		}
                
		*r = '\0';
		if( *word == '\0' )
		{
			if( color[ 0 ] == '\0' )  // incomplete colormap entry
				return false;                            
			else  // end of entry, still store the last color
				new_key = 1;
		} 
		else if( key > 0 && color[ 0 ] == '\0' )  // next word must be a color name part
			new_key = 0;
		else
		{
			if( strcmp( word, "c" ) == 0 )
				new_key = 5;
			else if( strcmp( word, "g" ) == 0 )
				new_key = 4;
			else if( strcmp( word, "g4" ) == 0 )
				new_key = 3;
			else if( strcmp( word, "m" ) == 0 )
				new_key = 2;
			else if( strcmp( word, "s" ) == 0 )
				new_key = 1;
			else 
				new_key = 0;
		}
                
		if( new_key == 0 ) // word is a color name part
		{  
			if( key == 0 )  // key expected
				return false;

			// accumulate color name
			if( color[ 0 ] != '\0' ) 
			{
				strncat( color, " ", space );
				space -= MIN( space, 1 );
			}
                        
			strncat( color, word, space );
			space -= MIN( space, strlen( word ) );
		}
		else // word is a key 
		{  
			if( key > current_key )
			{
				current_key = key;
				strcpy( current_color, color );
			}

			space = 128;
			color[ 0 ] = '\0';
			key = new_key;
			if( *p == '\0' ) 
				break;
		}
                
	}

	if( current_key > 1 )
	{
		colour = String( current_color );
		return true;
	}
	else
		return false; 
}

bool XPMTrans :: _ParseColour( const char* spec, XPMColour* colorPtr )
{
#ifdef DEBUG_XPM
dbprintf( "Parsing \"%s\"\n", spec );
#endif

	if( spec[0] == '#' )
	{
		char fmt[16];
		int i, red, green, blue;

		if( ( i = strlen( spec + 1 ) ) % 3 )
		{
			return false;
		}
                
		i /= 3;

		snprintf( fmt, 16, "%%%dx%%%dx%%%dx", i, i, i );

		if(sscanf( spec + 1, fmt, &red, &green, &blue ) != 3 )
		{
			return false;
		}

		if( i == 4 )
		{
			colorPtr->red = red;
			colorPtr->green = green;
			colorPtr->blue = blue;
		} 
		else if( i == 1 )
		{
			colorPtr->red = (red * 65535) / 15;
			colorPtr->green = (green * 65535) / 15;
			colorPtr->blue = (blue * 65535) / 15;
		} 
		else if( i == 2 )
		{
			colorPtr->red = (red * 65535) / 255;
			colorPtr->green = (green * 65535) / 255;
			colorPtr->blue = (blue * 65535) / 255;
		} 
		else // if (i == 3)
		{
			colorPtr->red = (red * 65535) / 4095;
			colorPtr->green = (green * 65535) / 4095;
			colorPtr->blue = (blue * 65535) / 4095;
		}
	} 
	else
	{
		if( !_FindColour( String( spec ), colorPtr ) )
			return false;
	}
        
	return true;
}

bool XPMTrans :: _FindColour( const String name, XPMColour* colorPtr )
{
	int n = sizeof( xColors ) / sizeof( XPMColorEntry );

#ifdef DEBUG_XPM
dbprintf( "Find colour \"%s\" of total %d colours\n", name.c_str(), n );
#endif
 
	for( int i = 0 ; i < n; i++ )
	{
		if( strcmp( color_names[ i ], name.c_str() ) == 0 )
		{	
			colorPtr->red = ( xColors[i].red * 65535 ) / 255;
			colorPtr->green = ( xColors[i].green * 65535 ) / 255;
			colorPtr->blue = ( xColors[i].blue * 65535 ) / 255;

			return true;
		}

	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// CLASS DEFINITION OF "XPMTransNode" (INCL. CODE)
//
//////////////////////////////////////////////////////////////////////////////////////////

class XPMTransNode : public TranslatorNode
{
public:
    virtual int Identify( const String& cSrcType, const String& cDstType, const void* pData, int nLen )
	{
		if ( nLen < 20 )
		{
			return( TranslatorFactory::ERR_NOT_ENOUGH_DATA );
		}

		String s( (const char*)pData );
		if( s.find( "XPM" ) == String::npos )
			return( TranslatorFactory::ERR_UNKNOWN_FORMAT );
		
		return 0;
    }

    virtual TranslatorInfo GetTranslatorInfo()
    {
		static TranslatorInfo sInfo = { "image/x-xpixmap", TRANSLATOR_TYPE_IMAGE, 1.0f };
		return( sInfo );
    }

    virtual Translator* CreateTranslator() 
	{
		return( new XPMTrans );
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
    static XPMTransNode sNode;
    return( &sNode );
}


/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2004 The Syllable Team
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */

#ifndef __F_GUI_FONT_H__
#define __F_GUI_FONT_H__

#include <atheos/types.h>
#include <gui/point.h>
#include <util/locker.h>
#include <util/string.h>
#include <vector>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent


class View;


#define FONT_FAMILY_LENGTH 63
#define FONT_STYLE_LENGTH 63

#define DEFAULT_FONT_REGULAR	 "System/Regular"
#define DEFAULT_FONT_BOLD	 "System/Bold"
#define DEFAULT_FONT_FIXED	 "System/Fixed"
#define DEFAULT_FONT_WINDOW	 "System/Window"
#define DEFAULT_FONT_TOOL_WINDOW "System/ToolWindow"



inline bool is_first_utf8_byte( uint8 nByte )
{
    return( (nByte & 0x80) == 0 || (nByte & 0xc0) == 0xc0 );
    
}

inline int utf8_char_length( uint8 nFirstByte )
{
    return( ((0xe5000000 >> ((nFirstByte >> 3) & 0x1e)) & 3) + 1 );
}

inline int utf8_to_unicode( const char* pzSource )
{
    if ( (pzSource[0]&0x80) == 0 ) {
	return( *pzSource );
    } else if ((pzSource[1] & 0xc0) != 0x80) {
        return( 0xfffd );
    } else if ((pzSource[0]&0x20) == 0) {
	return( ((pzSource[0] & 0x1f) << 6) | (pzSource[1] & 0x3f) );
    } else if ( (pzSource[2] & 0xc0) != 0x80 ) {
        return( 0xfffd );
    } else if ( (pzSource[0] & 0x10) == 0 ) {
	return( ((pzSource[0] & 0x0f)<<12) | ((pzSource[1] & 0x3f)<<6) | (pzSource[2] & 0x3f) );
    } else if ((pzSource[3] & 0xC0) != 0x80) {
        return( 0xfffd );
    } else {
	int   nValue;
	nValue = ((pzSource[0] & 0x07)<<18) | ((pzSource[1] & 0x3f)<<12) | ((pzSource[2] & 0x3f)<<6) | (pzSource[3] & 0x3f);
	return( ((0xd7c0+(nValue>>10)) << 16) | (0xdc00+(nValue & 0x3ff)) );
    }
    
}

inline int unicode_to_utf8( char* pzDst, uint32 nChar )
{
    if ((nChar&0xff80) == 0) {
	*pzDst = nChar;
	return( 1 );
    } else if ((nChar&0xf800) == 0) {
	pzDst[0] = 0xc0|(nChar>>6);
	pzDst[1] = 0x80|((nChar)&0x3f);
	return( 2 );
    } else if ((nChar&0xfc00) != 0xd800) {
	pzDst[0] = 0xe0|(nChar>>12);
	pzDst[1] = 0x80|((nChar>>6)&0x3f);
	pzDst[2] = 0x80|((nChar)&0x3f);
	return( 3 );
    } else {
	int   nValue;
	nValue = ( ((nChar<<16)-0xd7c0) << 10 ) | (nChar & 0x3ff);
	pzDst[0] = 0xf0 | (nValue>>18);
	pzDst[1] = 0x80 | ((nValue>>12)&0x3f);
	pzDst[2] = 0x80 | ((nValue>>6)&0x3f);
	pzDst[3] = 0x80 | (nValue&0x3f);
	return( 4 );
    }
}




enum font_spacing {
    CHAR_SPACING,
    STRING_SPACING,
    FIXED_SPACING
};

enum font_direction {
    FONT_LEFT_TO_RIGHT,
    FONT_RIGHT_TO_LEFT,
    FONT_TOP_TO_BOTTOM,
    FONT_BOTTOM_TO_TOP
};


enum {
    TRUNCATE_END       = 0,
    TRUNCATE_BEGINNING = 1,
    TRUNCATE_MIDDLE    = 2,
    TRUNCATE_SMART     = 3
};

enum {
    UNICODE_UTF8,
    ISO_8859_1,
    ISO_8859_2,
    ISO_8859_3,
    ISO_8859_4,
    ISO_8859_5,
    ISO_8859_6,
    ISO_8859_7,
    ISO_8859_8,
    ISO_8859_9,
    ISO_8859_10,
    MACINTOSH_ROMAN
};


// Flags returned by Application::GetFontStyle()
enum {
    FONT_IS_FIXED        = 0x0001,
    FONT_HAS_TUNED_SIZES = 0x0002,
    FONT_IS_SCALABLE     = 0x0004
};

struct	font_height
{
    float ascender;   // Pixels from baseline to top of glyph (positive)
    float descender;  // Pixels from baseline to bottom of glyph (negative)
    float line_gap;   // Space between lines (positive)
};

// NOTE: These two structures does not seem to be used anywhere, and should probably be removed.
// Consider them obsolete!
struct	font_attribs
{
    float	vPointSize;
    float	vRotation;
    float	vShare;
};

struct edge_info
{
    float	left;
    float	right;
};


enum
{
    FPF_MONOSPACED = 0x00000001,
    FPF_SMOOTHED   = 0x00000002,	/** Antialiased */
    FPF_BOLD       = 0x00000004,
    FPF_ITALIC     = 0x00000008,
    FPF_SYSTEM     = 0x80000000
};

struct font_properties
{
    font_properties() : m_nFlags(FPF_SYSTEM), m_vSize(10.0f), m_vShear(0.0f), m_vRotation(0.0f) {}
    font_properties( const String& cFamily, const String& cStyle, uint32 nFlags = FPF_SYSTEM,
		     float vSize = 10.0f, float vShear = 0.0f, float vRotation = 0.0f )
	    : m_cFamily(cFamily), m_cStyle(cStyle), m_nFlags(nFlags), m_vSize(vSize), m_vShear(vShear), m_vRotation(vRotation) {}
    
    String	m_cFamily;
    String m_cStyle;
    uint32	m_nFlags;
    float	m_vSize;
    float	m_vShear;
    float	m_vRotation;
};

/** Text font class
 * \ingroup gui
 * \par Description:
 *	The os::View class need a text font to be able to render text.
 *	The font represent all the properties of the text like with and
 *	height of each glyph, rotation/shearing of the glyps and most
 *	importantly the actual graphical "image" of each glyph.
 *
 *	Font objects are reference counted to ease sharing between
 *	views. If the same font is set on two views they will both
 *	be affected by subsequent changes to the font.
 *
 *	A font keeps track of all view's it has been added to so
 *	it can notify them whenever one of the properties of the
 *	font changes.
 *
 *	AtheOS primarily use scalable truetype fonts but can also use
 *	various bitmap fonts. When using truetype font's the glyphs
 *	will be scaled to the requested point-size but When using
 *	bitmap fonts the size will be "snapped" to the closest size
 *	supported by the font.
 *
 *	When rendering truetype fonts the glyphs will be antialiazed
 *	to improve the quality of the fonts. The view can eighter
 *	antialiaze against a fixed background color to maximize speed
 *	or against the background it is actually rendered at.
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class Font
{
public:
    typedef std::vector<float> size_list_t;
public:
    Font();
    Font( const Font& font );
    Font( const font_properties& sProps );
    Font( const String& cConfigName );

    void	    AddRef();
    void	    Release();
  
    status_t	    SetProperties( const font_properties& sProps );
    status_t	    SetProperties( const String& cConfigName );
    status_t	    SetProperties( float vSize, float vShear = 0.0f, float vRotation = 0.0f );
    status_t	    SetFamilyAndStyle( const char* pzFamily, const char* pzStyle );
    void	    SetSize( float vSize );
    void	    SetShear( float vShear );
    void	    SetRotation( float vRotation );
    void	    SetSpacing( int nSpacing );
    void	    SetEncoding( int nEncoding );
    void	    SetFace( uint16 nFace );
    void	    SetFlags( uint32 nFlags );

    void	    GetFamilyAndStyle( const char* pzFamily, const char* pzStyle ) const;
    float	    GetSize() const;
    float	    GetShear() const;
    float	    GetRotation() const;
    int		    GetSpacing() const;
    int		    GetEncoding() const;
    uint32	    GetFlags() const;
    font_direction  GetDirection() const;

    void            GetTruncatedStrings( const char *stringArray[],
					 int32 numStrings,
					 uint32 mode,
					 float width,
					 char *resultArray[]) const;

    float	GetStringWidth( const char* pzString, int nLength = -1 ) const;
    float	GetStringWidth( const String& cString ) const;
    void	GetStringWidths( const char** apzStringArray, const int* anLengthArray,
				 int nStringCount, float* avWidthArray ) const;

    Point	GetTextExtent( const char* pzString, int nLength = -1, uint32 nFlags = 0 ) const;
    Point	GetTextExtent( const String& cString, uint32 nFlags = 0 ) const;
    void	GetTextExtents( const char** apzStringArray, const int* anLengthArray,
				 int nStringCount, Point* acExtentArray, uint32 nFlags ) const;

    int		GetStringLength( const char* pzString, float vWidth, bool bIncludeLast = false ) const;
    int		GetStringLength( const char* pzString, int nLength, float vWidth, bool bIncludeLast = false ) const;
    int		GetStringLength( const String& cString, float vWidth, bool bIncludeLast = false ) const;
    void	GetStringLengths( const char** apzStringArray, const int* anLengthArray, int nStringCount,
				  float vWidth, int anMaxLengthArray[], bool bIncludeLast = false ) const;

    void	GetHeight( font_height* psHeight ) const	{ *psHeight = m_sHeight;	}

    int		GetFontID( void ) const { return( m_hFontHandle ); }

    bool  operator==( const Font& cOther );
    bool  operator!=( const Font& cOther );
    Font& operator=( const Font& cOther );
    
      // Static members for obtaining/setting global info about the fonts.
    static status_t	GetConfigNames( std::vector<String>* pcTable );
    static status_t	GetDefaultFont( const String& cName, font_properties* psProps );
    static status_t	SetDefaultFont( const String& cName, const font_properties& sProps );
    static status_t	AddDefaultFont( const String& cName, const font_properties& sProps );
    
    static int		GetFamilyCount();
    static status_t	GetFamilyInfo( int nIndex, char* pzFamily );
    static int		GetStyleCount( const char* pzFamily );
    static status_t	GetStyleInfo( const char* pzFamily, int nIndex, char* pzStyle, uint32* pnFlags = NULL );
    static status_t	GetBitmapSizes( const String& cFamily, const String& cStyle, size_list_t* pcList );

    static bool		Rescan();
private:
    friend class View;

    ~Font();
    void _CommonInit();
    status_t _SetProperties( float vSize, float vShear, float vRotation, uint32 nFlags );

    int		 m_nRefCount;
    bool	 m_bIsValid;

    port_id	 m_hReplyPort;
	
    int		 m_hFontHandle;
    String	 m_cFamily;
    String	 m_cStyle;
    float	 m_vSize;
    float	 m_vShear;
    float	 m_vRotation;
    uint8	 m_nSpacing;
    uint8	 m_nEncoding;
    uint16	 __unused__;
    uint32	 m_nFlags;
    font_height	 m_sHeight;
};

}

#endif	// __F_GUI_FONT_H__

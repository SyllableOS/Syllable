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
#include <util/flattenable.h>
#include <util/typecodes.h>

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

/* we should add encoding, space,... to this and also the operators != and == */
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

/** Text Font class
 * \ingroup gui
 * \par Description:
 *	The os::View class needs a text font to be able to render text.
 *	The Font class represents all the properties of the text like width and
 *	height of each glyph, rotation/shearing of the glyps and most
 *	importantly the actual graphical "image" of each glyph.
 *
 *	Font objects are reference counted to ease sharing between
 *	views. If the same font is set on two views they will both
 *	be affected by subsequent changes to the font.
 *
 *	A Font keeps track of all view's it has been added to so
 *	it can notify them whenever one of the properties of the
 *	Font changes.
 *
 *	Syllable primarily uses scalable TrueType Fonts but can also use
 *	various bitmap Fonts. When using TrueType Font's the glyphs
 *	will be scaled to the requested point-size, but when using
 *	bitmap Fonts the size will be "snapped" to the closest size
 *	supported by the Font.
 *
 *	When rendering TrueType Fonts, the glyphs have the ability to be 
 *	antialiazed to improve the quality of the fonts. The view can either
 *	antialiaze against a fixed background color to maximize speed
 *	or against the background it is actually rendered at.
 *
 * \sa os::Flattenable
 * \author	Kurt Skauen (kurt@atheos.cx) with modifications by the Syllable team.
 *****************************************************************************/

class Font : public Flattenable
{
public:
    typedef std::vector<float> size_list_t;
public:

	/** Default Constructor...
     * \par Description:
     *	The Default Constructor will get all of its properties as it gets used,
     *
     * \author Kurt Skauen (kurt@atheos.cx) and Rick Caudill(syllable_desktop@hotpop.com)
	 **************************************************************************/
    Font();

	/** Constructor...
     * \par Description:
     *	This constructor will get all of its properties from the font that you 
	 *  passed to it.
	 *
	 * \author Kurt Skauen (kurt@atheos.cx) and Rick Caudill(syllable_desktop@hotpop.com)
	 **************************************************************************/
    Font( const Font& font );

	/** Constructor...
     * \par Description:
     * This constructor will get all of its major properties from the font_properties
     * that you passed to it.
     * \author Kurt Skauen (kurt@atheos.cx) and Rick Caudill(syllable_desktop@hotpop.com)
	 **************************************************************************/
    Font( const font_properties& sProps );

	/** Constructor...
     * \par Description:
     *	This constructor will pull all of its major properties from the os::String
     *  you passed to it.  The os::String should be a valid Font Config Name.
     *
     * \author Kurt Skauen (kurt@atheos.cx) and Rick Caudill(syllable_desktop@hotpop.com)
	 **************************************************************************/
    Font( const String& cConfigName );

	/** Adds a reference counter
     * \par Description:
     *  Adds a reference counter. Please call this method after calling a constructor.
     *
     * \sa Release()
     *    
     * \author 
	 **************************************************************************/
    void	    		AddRef();

	/** Releases all instances of a Font.
     * \par Description:
     *	This is the function you want to call when you want to delete a Font.  Don't just
     *  delete the Font as that could cause problems, instead
     *  call this method.
     *
     * \sa AddRef()
     *
     * \author 
	 **************************************************************************/
    void	    		Release();
 
 	/** Sets the properties of the Font.
     * \par Description:
     *	Sets the properties of the Font to the font_properties passed to it.
     * \author Kurt Skauen (kurt@atheos.cx) and Rick Caudill(syllable_desktop@hotpop.com)
	 **************************************************************************/ 
    status_t			SetProperties( const font_properties& sProps );
    
    /** Sets the properties of the Font
     * \par Description:
     *  Sets the properties of the Font to the Font name specified.
     * \author Kurt Skauen (kurt@atheos.cx) and Rick Caudill(syllable_desktop@hotpop.com)
	 **************************************************************************/
    status_t			SetProperties( const String& cConfigName );
    
	/** Sets the properties of the Font
     * \par Description:
     *	Sets the properties of the Font to the properties passed to it.
     * \author Kurt Skauen (kurt@atheos.cx) and Rick Caudill(syllable_desktop@hotpop.com)
	 **************************************************************************/    
    status_t			SetProperties( float vSize, float vShear = 0.0f, float vRotation = 0.0f );
    
	/** Sets the family and style of the Font.
     * \par Description:
     *	Sets the family(the name of the font) and the style(the type of the Font) of the Font.
     *
     * \sa SetProperties(),GetConfigNames() 
     * \author Kurt Skauen (kurt@atheos.cx) and Rick Caudill(syllable_desktop@hotpop.com)
	 **************************************************************************/    
    status_t			SetFamilyAndStyle( const char* pzFamily, const char* pzStyle );
    
	/** Sets the size...
     * \par Description:
     *	Sets the size of the Font in Points...  Remember that with non-TrueType Fonts, the Font will be resized
     *  to an approximation.  This normally is all right, but sometimes you will have a Font that is 8.5pt instead
     *  of a Font that is 9pt.
     * \author Kurt Skauen (kurt@atheos.cx) and Rick Caudill(syllable_desktop@hotpop.com)
	 **************************************************************************/	
	void	    		SetSize( float vSize );

	/** Sets the shear
     * \par Description:
     *  Sets the shear of the Font...
     * \author Kurt Skauen (kurt@atheos.cx) and Rick Caudill(syllable_desktop@hotpop.com)
	 **************************************************************************/
    void	    		SetShear( float vShear );

	/** Sets the rotation...
     * \par Description:
     *  Sets the rotation of the Font.  The higher the float passed, the higher the rotation.
     *  If you pass 90.0 to this method then the font will be rotated 90 degrees and etc...
     * \author Kurt Skauen (kurt@atheos.cx) and Rick Caudill(syllable_desktop@hotpop.com)
	 **************************************************************************/
    void	    		SetRotation( float vRotation );

	/** Sets the spacing of a font
     * \par Description:
     *	Sets the spacing of the Font. This function is not implemented yet!
     * \author Kurt Skauen (kurt@atheos.cx) and Rick Caudill(syllable_desktop@hotpop.com)
	 **************************************************************************/
    void	    		SetSpacing( int nSpacing );

	/** Sets the encoding of the Font
     * \par Description:
     *	Sets the encoding of the Font. This function is not implemented yet!
     * \author Kurt Skauen (kurt@atheos.cx) and Rick Caudill(syllable_desktop@hotpop.com)
	 **************************************************************************/
    void				SetEncoding( int nEncoding );

	/** Sets the face of the Font.
     * \par Description:
     *	Sets the face of the Font.  This function is not implemented yet!  
     * \author Kurt Skauen (kurt@atheos.cx) and Rick Caudill(syllable_desktop@hotpop.com)
	 **************************************************************************/
    void				SetFace( uint16 nFace );

	/** Sets the flags for the Font.
     * \par Description:
     *	Sets the flags for the Font.  
     *
     *  The flags that can be set are:
     *	FPF_MONOSPACED
     *	FPF_SMOOTHED  //Antialiased
     *	FPF_BOLD
     *	FPF_ITALIC
     *	FPF_SYSTEM
     *
     * \sa GetFlags()
     * \author Kurt Skauen (kurt@atheos.cx) and Rick Caudill(syllable_desktop@hotpop.com)
	 **************************************************************************/
    void				SetFlags( uint32 nFlags );

	/** Gets the Style of the Font...
     * \par Description:
     *	This returns the name of the style(ie: Regular, Bold,...)
     *
     * \sa SetFamilyAndStyle()
     *
     * \author Kurt Skauen (kurt@atheos.cx) and Rick Caudill(syllable_desktop@hotpop.com)
	 **************************************************************************/
    String				GetStyle() const;

	/** Gets the family of the Font...
     * \par Description:
     *  This returns the family of the Font(IE: Arial, Sans Serif...)
     *
     * \sa SetFamilyAndStyle()  
     * \author Kurt Skauen (kurt@atheos.cx) and Rick Caudill(syllable_desktop@hotpop.com)
	 **************************************************************************/
    String				GetFamily() const;

	/** Gets the size of the Font...
     * \par Description:
     *  This method returns the size of the Font.
     *
     * \sa SetSize()
     *
     * \author Kurt Skauen (kurt@atheos.cx) and Rick Caudill(syllable_desktop@hotpop.com)
	 **************************************************************************/
    float	    		GetSize() const;

	/** Gets the shear of the Font.
     * \par Description:
     *	This method returns the shear of the Font.
     *
     * \sa SetShear()
     *
     * \author Kurt Skauen (kurt@atheos.cx) and Rick Caudill(syllable_desktop@hotpop.com)
	 **************************************************************************/
    float	    		GetShear() const;

	/** Gets the rotation of the Font.
     * \par Description:
     *	This method returns the rotation of the Font.
     *
     * \sa SetRotation()
     * \author Kurt Skauen (kurt@atheos.cx) and Rick Caudill(syllable_desktop@hotpop.com)
	 **************************************************************************/
    float	    		GetRotation() const;

	/** Gets the spacing of the Font.
     * \par Description:
     *	This method returns the spacing of the Font.  This always returns 0 right now.
     *
     * \sa SetSpacing()
     *
     * \author Kurt Skauen (kurt@atheos.cx) and Rick Caudill(syllable_desktop@hotpop.com)
	 **************************************************************************/
    int		    		GetSpacing() const;

	/** Gets the encoding of the Font.
     * \par Description:
     *	This method returns the encoding of the Font.
     *
     * \sa SetEncoding()
     *
     * \author Kurt Skauen (kurt@atheos.cx) and Rick Caudill(syllable_desktop@hotpop.com)
	 **************************************************************************/
    int		    		GetEncoding() const;

	/** Gets the flags passed to the font.
     * \par Description:
     *	This method will get all the flags that have been passed to the Font
     *
     * \sa SetFlags()
     * \author Kurt Skauen (kurt@atheos.cx) and Rick Caudill(syllable_desktop@hotpop.com)
	 **************************************************************************/
    uint32	    		GetFlags() const;

	/** Gets the direction of the Font.
     * \par Description:
     *	Right now this function will always return FONT_LEFT_TO_RIGHT.
     *
     * \sa SetDirection()
     * \author Kurt Skauen (kurt@atheos.cx) and Rick Caudill(syllable_desktop@hotpop.com)
	 **************************************************************************/
    font_direction		GetDirection() const;


	/** Get truncated strings
     * \par Description:
     *
     *
     * \sa 
     * \author Kurt Skauen (kurt@atheos.cx)
	 **************************************************************************/
    void				GetTruncatedStrings( const char *stringArray[],int32 numStrings,uint32 mode,float width,char *resultArray[]) const;

	/** Gets the strings width....
     * \par Description:
     *	Gets the strings width.
     *
     * \sa
     * \author Kurt Skauen (kurt@atheos.cx)
	 **************************************************************************/
    float				GetStringWidth( const char* pzString, int nLength = -1 ) const;
    
    /** Gets the strings width...
     * \par Description:
     *	Gets the strings width.
     *
     * \sa
     * \author Kurt Skauen
	 **************************************************************************/
    float				GetStringWidth( const String& cString ) const;
    
	/** Gets multiple string widths
     * \par Description:
     *	Gets mulitple string widths.
     *
     * \sa
     * \author Kurt Skauen (kurt@atheos.cx)
	 **************************************************************************/    
    void				GetStringWidths( const char** apzStringArray, const int* anLengthArray,int nStringCount, float* avWidthArray ) const;

	/** 
     * \par Description:
     *	
     *
     * \sa 
     * \author
	 **************************************************************************/
    Point				GetTextExtent( const char* pzString, int nLength = -1, uint32 nFlags = 0, int nTargetWidth = -1 ) const;

	/** 
     * \par Description:
     *	
     *
     * \sa 
     * \author
	 **************************************************************************/
    Point				GetTextExtent( const String& cString, uint32 nFlags = 0, int nTargetWidth = -1 ) const;
    
	/** 
     * \par Description:
     *	
     *
     * \sa 
     * \author
	 **************************************************************************/    
    void				GetTextExtents( const char** apzStringArray, const int* anLengthArray,int nStringCount, Point* acExtentArray, uint32 nFlags, int nTargetWidth = -1 ) const;

	/** Gets the length of a string.
     * \par Description:
     *	Gets the length of a string.
     *
     * \sa 
     * \author Kurt Skauen (kurt@atheos.cx)
	 **************************************************************************/
    int					GetStringLength( const char* pzString, float vWidth, bool bIncludeLast = false ) const;

	/** Gets the length of a string
     * \par Description:
     *	Gets the length of a string.
     *
     * \sa
     * \author Kurt Skauen (kurt@atheos.cx)
	 **************************************************************************/
    int					GetStringLength( const char* pzString, int nLength, float vWidth, bool bIncludeLast = false ) const;

	/** Gets the length of a string.
     * \par Description:
     *	Gets the length of a string.
     *
     * \sa
     * \author Kurt Skauen (kurt@atheos.cx)
	 **************************************************************************/
    int					GetStringLength( const String& cString, float vWidth, bool bIncludeLast = false ) const;
    
	/** Gets the lengths of multiple strings.
     * \par Description:
     *	Gets the lengths of multiple strings.
     *
     * \sa
     * \author Kurt Skauen (kurt@atheos.cx)
	 **************************************************************************/    
    void				GetStringLengths( const char** apzStringArray, const int* anLengthArray, int nStringCount,float vWidth, int anMaxLengthArray[], bool bIncludeLast = false ) const;


	/** Gets the height of the Font.
     * \par Description:
     *	Gets the height of the Font.
     *
     * \param psHeight - a font_height pointer which will contain the height of the Font.
     * \sa
     * \author Kurt Skauen (kurt@atheos.cx)
	 **************************************************************************/
    void				GetHeight( font_height* psHeight ) const	{ *psHeight = m_sHeight;	}


	/** Gets the supported characters of the Font.
     * \par Description:
     *	Returns a vector of the supported characters that the Font can use.
     *
     * \sa
     * \author Henrik Isaksason
	 **************************************************************************/
	std::vector<uint32>	GetSupportedCharacters() const;
	
	
	/** Gets the Font handle ID.
     * \par Description:
     *	Gets the Font handle ID.
     *
     * \sa
     * \author Kurt Skauen (kurt@atheos.cx)
	 **************************************************************************/
    int					GetFontID( void ) const { return( m_hFontHandle ); }


    bool				operator==( const Font& cOther );  
    bool				operator!=( const Font& cOther );
    Font&				operator=( const Font& cOther );
    

    static status_t		GetConfigNames( std::vector<String>* pcTable );
    static status_t		GetDefaultFont( const String& cName, font_properties* psProps );     
    static status_t		SetDefaultFont( const String& cName, const font_properties& sProps );   
    static status_t		AddDefaultFont( const String& cName, const font_properties& sProps );   
    static int			GetFamilyCount();
    static status_t		GetFamilyInfo( int nIndex, char* pzFamily ); 
    static int			GetStyleCount( const char* pzFamily );
    static status_t		GetStyleInfo( const char* pzFamily, int nIndex, char* pzStyle, uint32* pnFlags = NULL );   
    static status_t		GetBitmapSizes( const String& cFamily, const String& cStyle, size_list_t* pcList );

	/** Scans for new fonts...
     * \par Description:
     *	This method calls the appserver to rescan for new Fonts.
     *
     * \sa
     *
     * \author Rick Caudill(syllable_desktop@hotpop.com)
	 **************************************************************************/
    static bool			Rescan();


	virtual size_t		GetFlattenedSize( void ) const;
	virtual status_t	Flatten( uint8* pBuffer, size_t nSize ) const;
	virtual status_t	Unflatten( const uint8* pBuffer);
	virtual int 		GetType(void) const { return T_FONT;}


private:
    friend class View;

	/**internal*/
    ~Font();

	
    void				_CommonInit();
    status_t			_SetProperties(const font_properties& sProps);
	void				_InitFontProperties();
    
	atomic_t			m_nRefCount;
    bool				m_bIsValid;

    port_id				m_hReplyPort;
	
    int					m_hFontHandle;
    uint8				m_nSpacing;
    uint8				m_nEncoding;
    uint16				__unused__;
    font_height			m_sHeight;
	font_properties		m_sProps;
};

}

#endif	// __F_GUI_FONT_H__




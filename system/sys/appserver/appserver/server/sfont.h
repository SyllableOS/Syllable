/*
 *  The AtheOS application server
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#ifndef	__F_SFONT_H__
#define	__F_SFONT_H__

#include <atheos/types.h>

#include <gui/font.h>
#include <gui/rect.h>
#include <gui/guidefines.h>
#include <util/resource.h>
#include <util/locker.h>

#include <freetype/freetype.h>
//#include <ft2build.h>
//#include FT_FREETYPE_H


#include <map>
#include <string>

class	FontFamily;
class	SFont;
class	SFontInstance;

extern os::Locker g_cFontLock;


struct Glyph
{
    int		   m_nAdvance;
    os::IRect	   m_cBounds;
    int		   m_nBytesPerLine;
    uint8*	   m_pRaster;
    SFontInstance* m_pcInstance;
};

class FontProperty
{
public:
    FontProperty() {}
    FontProperty( int nPointSize, int nShare, int nRotation ) {
	m_nPointSize = nPointSize; m_nShear = nShare; m_nRotation = nRotation;
    }
  
    bool operator < ( const FontProperty& cProp ) const {
/*    
      if ( m_vPointSize == cProp.m_vPointSize ) {
      if ( m_vShear == cProp.m_vShear ) {
      return( m_vRotation < cProp.m_vRotation );
      } else {
      return( m_vShear < cProp.m_vShear );
      }
      } else {*/
	return( m_nPointSize < cProp.m_nPointSize );
//    }
//    return( m_vPointSize < cProp.m_vPointSize || m_vShear < cProp.m_vShear || m_vRotation < cProp.m_vRotation );
    }
  
    int	m_nPointSize; // Fixed-point 26:6
    int	m_nShear;     // Fixed-point 26:6
    int	m_nRotation;  // Fixed-point 26:6
  
};

class SFont
{
public:
    SFont( FontFamily* pcFamily, FT_Face psFace );
    ~SFont();

    const std::string&	GetStyle() const { return( m_cStyle ); }
    FontFamily*		GetFamily() const { return( m_pcFamily ); }

    void		AddInstance( SFontInstance* pcInstance );
    void		RemoveInstance( SFontInstance* pcInstance );
    int			GetInstanceCount() const { return( m_cInstances.size() ); }
    SFontInstance* 	     OpenInstance( int nPointSize, int nRotation = 0, int nShare = 0 );

    bool		     IsFixedWidth() const		{ return( m_bFixedWidth );	}
    bool		     IsScalable() const 		{ return( m_bScalable ); }
    int		 	     GetGlyphCount( void ) const	{ return( m_nGlyphCount );	}
    FT_Face		     GetTTFace( void ) const		{ return( m_psFace );		}
    const os::Font::size_list_t& GetBitmapSizes() const { return( m_cBitmapSizes ); }
    int		 	     CharToUnicode( uint nChar );
    SFontInstance* 	     FindInstance( int nPointSize, int nRotation, int nShare ) const;

    void		     SetDeletedFlag( bool bDeleted ) { m_bDeleted = bDeleted; }
    bool		     IsDeleted() const { return( m_bDeleted ); }
private:
    friend class FontFamily;

    FontFamily*					m_pcFamily;
    std::string					m_cStyle;
    FT_Face					m_psFace;
    int						m_nGlyphCount;
    bool					m_bFixedWidth;
    bool					m_bScalable;
    bool					m_bDeleted;	// Set by the font-scanner if the font-file is deleted but the font is in use.
    os::Font::size_list_t			m_cBitmapSizes;
    std::map<FontProperty,SFontInstance*>	m_cInstances;
};

class SFontInstance : public Resource
{
public:
    SFontInstance( SFont* pcFont, int nPointSize, int nRotation, int nShare );

    SFont*	GetFont( void ) const { return( m_pcFont );	}
    status_t	SetProperties( const os::font_properties& sProps );
    status_t	SetProperties( int nPointSize, int nSheare, int nRotation );
    int		GetIdentifier()	const		{ return( m_nIdentifier );	}

    int		_GetPointSize( ) const		{ return( m_nPointSize ); }
    int		_GetShear() const 		{ return( m_nShear ); }
    int		_GetRotation() const 		{ return( m_nRotation ); }
  
    int		GetGlyphCount() const		{ return( m_nGlyphCount );	}
    Glyph* 	GetGlyph( int nIndex );
    int		CharToUnicode( int nChar )	{ return( m_pcFont->CharToUnicode( nChar ) );	}

    void	SetFixedWidth( bool bFixed ) 	{ m_bFixedWidth = bFixed; }
    bool	IsFixedWidth() 			{ return( m_bFixedWidth );		}

    int		GetNomWidth()	const		{ return( m_nNomWidth );	}
    int		GetNomHeight() const		{ return( m_nNomHeight ); 	}
    int		GetAscender() const 		{ return( m_nAscender );	}
    int		GetDescender() const		{ return( m_nDescender );	}
    int		GetLineGap() const		{ return( m_nLineGap );		}
    int		GetAdvance() const		{ return( m_nAdvance ); 	}
    int		GetStringWidth( const char* pzString, int nLength );
    int		GetStringLength( const char* pzString, int nLength, int nWidth, bool bIncludeLast );
private:
    ~SFontInstance();
    friend	class	SFont;
    friend	class	FontServer;

    SFont*	   m_pcFont;

    bool	   m_bFixedWidth;
    int		   m_nIdentifier;	// Unique identifier, sent to application's

    int		   m_nPointSize; // Fixed-point 26:6
    int		   m_nShear;     // Fixed-point 26:6
    int		   m_nRotation;  // Fixed-point 26:6
  
    int		   m_nNomWidth;
    int		   m_nNomHeight;

    int		   m_nAscender;		// Space above baseline in pixels
    int		   m_nDescender;	// Space below baseline in pixels
    int		   m_nLineGap;		// Space between lines in pixels
    int		   m_nAdvance;		// Max advance in pixels (Used for monospaced fonts)

    Glyph**	   m_ppcGlyphTable;	// Array of pointers to glyps. Each unloaded glyp has a NULL pointer.
    int		   m_nGlyphCount;
};


class	FontFamily
{
public:
    FontFamily( const char* pzName );
    ~FontFamily();
    const std::string& GetName() const { return( m_cName ); }
    
    void	AddFont( SFont*	pcFont );
    void	RemoveFont( SFont* pcFont );
    SFont*	FindStyle( const std::string& pzStyle );
private:
    friend class FontServer;

    std::string			 m_cName;
    std::map<std::string,SFont*> m_cFonts;
};


class FontServer
{
public:
    FontServer();
    ~FontServer();

    static bool		Lock();
    static void		Unlock();
    static FontServer*	GetInstance() { return( s_pcInstance ); }

    FT_Library		GetFTLib() const { return( m_hFTLib ); }
    void		ScanDirectory( const char* pzPath );

    SFont*	 	OpenFont( const std::string& cFamily, const std::string& cStyle );
    SFontInstance* 	OpenInstance( const std::string& cFamily, const std::string& pzStyle,
				      int nPointSize, int nRotation, int nShare );

    int			GetFamilyCount() const;
    int			GetFamily( int nIndex, char* pzFamily, uint32* pnFlags );
    int			GetStyleCount( const std::string& cFamily ) const;
    int			GetStyle( const std::string& cFamily, int nIndex, char* pzStyle, uint32* pnFlags ) const;


    void		RemoveFamily( FontFamily* pcFamily );
  
private:
    static FontServer*	s_pcInstance;
    FT_Library		m_hFTLib;

    FontFamily*	FindFamily( const std::string& cName ) const;

    std::map<std::string,FontFamily*> m_cFamilies;  // List of font families.
};



#endif	// __F_SFONT_H__

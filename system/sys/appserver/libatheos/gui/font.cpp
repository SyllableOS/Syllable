/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#include <string.h>
#include <assert.h>
#include <stdexcept>

#include <atheos/kernel.h>

#include <gui/font.h>
#include <util/application.h>
#include <util/message.h>
#include <util/messenger.h>

#include <appserver/protocol.h>

using namespace os;

void Font::_CommonInit()
{
    Message cReq( AR_CREATE_FONT );
    Message cReply;

    assert( Application::GetInstance() != NULL );

    m_nRefCount   = 1;
    m_vSize	= 0.0f;
    m_vShear	= 0.0f;
    m_vRotation	= 0.0f;
    m_nSpacing	= CHAR_SPACING;
    m_nEncoding	= 0;
    m_nFlags	= 0;
    m_hFontHandle	= -1;
    m_hReplyPort	= create_port( "font_reply", DEFAULT_PORT_SIZE );

    port_id hPort = Application::GetInstance()->GetAppPort();
    Messenger( hPort ).SendMessage( &cReq, &cReply );
    cReply.FindInt( "handle", &m_hFontHandle );
}

Font::Font()
{
    _CommonInit();
}

Font::Font( const Font& cOrig )
{
    _CommonInit();

      // Make sure SetProperties() realy takes action
    m_vSize     = -1.0f;
    m_vRotation = -2.0f;
    m_vShear    = -3.0f;
  
    SetFamilyAndStyle( cOrig.m_cFamily.c_str(), cOrig.m_cStyle.c_str() );
    SetProperties( cOrig.m_vSize, cOrig.m_vShear, cOrig.m_vRotation );
}

Font::Font( const std::string& cConfigName )
{
    _CommonInit();
    font_properties sProps;
    if ( GetDefaultFont( cConfigName, &sProps ) != 0 ) {
	throw( std::runtime_error( "Configuration not found" ) );
    }
    if ( SetProperties( sProps ) != 0 ) {
	throw( std::runtime_error( "Failed to set properties" ) );
    }
}

Font::Font( const font_properties& sProps )
{
    _CommonInit();
    if ( SetProperties( sProps ) != 0 ) {
	throw( std::runtime_error( "Failed to set properties" ) );
    }
}


Font::~Font()
{
    assert( Application::GetInstance() != NULL );
    assert( m_nRefCount == 0 );
    
    Message cReq( AR_DELETE_FONT );
    cReq.AddInt32( "handle", m_hFontHandle );
    
    port_id hPort = Application::GetInstance()->GetAppPort();
    Messenger( hPort ).SendMessage( &cReq );
    delete_port( m_hReplyPort );
}

bool Font::operator==( const Font& cOther )
{
    return( m_cFamily == cOther.m_cFamily &&
	    m_cStyle == cOther.m_cStyle &&
	    m_vSize == cOther.m_vSize &&
	    m_vShear == cOther.m_vShear &&
	    m_vRotation == cOther.m_vRotation &&
	    m_nSpacing == cOther.m_nSpacing &&
	    m_nEncoding == cOther.m_nEncoding &&
	    m_nFlags == cOther.m_nFlags );	
}

bool Font::operator!=( const Font& cOther )
{
    return( m_cFamily != cOther.m_cFamily ||
	    m_cStyle != cOther.m_cStyle ||
	    m_vSize != cOther.m_vSize ||
	    m_vShear != cOther.m_vShear ||
	    m_vRotation != cOther.m_vRotation ||
	    m_nSpacing != cOther.m_nSpacing ||
	    m_nEncoding != cOther.m_nEncoding ||
	    m_nFlags != cOther.m_nFlags );	
}

Font& Font::operator=( const Font& cOther )
{
    SetFamilyAndStyle( cOther.m_cFamily.c_str(), cOther.m_cStyle.c_str() );
    SetProperties( cOther.m_vSize, cOther.m_vShear, cOther.m_vRotation );
    return( *this );
}

void Font::AddRef()
{
    atomic_add( &m_nRefCount, 1 );
}

void Font::Release()
{
    if ( atomic_add( &m_nRefCount, -1 ) == 1 ) {
	delete this;
    }
}

status_t Font::SetProperties( const std::string& cConfigName )
{
    status_t nError;
    font_properties sProps;

    nError = GetDefaultFont( cConfigName, &sProps );
    if ( nError != 0 ) {
	return( nError );;
    }
    return( SetProperties( sProps ) );
}

status_t Font::SetProperties( const font_properties& sProps )
{
    status_t nError;
    nError = SetFamilyAndStyle( sProps.m_cFamily.c_str(), sProps.m_cStyle.c_str() );
    if ( nError < 0 ) {
	return( nError );
    }
    return( SetProperties( sProps.m_vSize, sProps.m_vShear, sProps.m_vRotation ) );
}

status_t Font::SetFamilyAndStyle( const char* pzFamily, const char* pzStyle )
{
    Message cReq( AR_SET_FONT_FAMILY_AND_STYLE );
    Message cReply;
    
    cReq.AddInt32( "handle", m_hFontHandle );
    cReq.AddString( "family", pzFamily );
    cReq.AddString( "style", pzStyle );

    port_id hPort = Application::GetInstance()->GetAppPort();
    Messenger( hPort ).SendMessage( &cReq, &cReply );

    int nError = -EINVAL;

    cReply.FindInt( "error", &nError );

    if ( nError >= 0 ) {
	int32 nAscender;
	int32 nDescender;
	int32 nLineGap;
	
	cReply.FindInt( "ascender",  &nAscender );
	cReply.FindInt( "descender", &nDescender );
	cReply.FindInt( "line_gap",  &nLineGap );

	m_sHeight.ascender  = nAscender;
	m_sHeight.descender = -nDescender;
	m_sHeight.line_gap   = nLineGap;
	
	m_cFamily = pzFamily;
	m_cStyle  = pzStyle;

	return( 0 );
    } else {
	errno = -nError;
	return( -1 );
    }
}

void Font::SetSize( float vSize )
{
    SetProperties( vSize, m_vShear, m_vRotation );
}

void Font::SetShear( float vShear )
{
    SetProperties( m_vSize, vShear, m_vRotation );
}

void Font::SetRotation( float vRotation )
{
    SetProperties( m_vSize, m_vShear, vRotation );
}

status_t Font::SetProperties( float vSize, float vShear, float vRotation )
{
    if ( vSize != m_vSize || vShear != m_vShear || vRotation != vRotation )
    {
	Message cReq( AR_SET_FONT_PROPERTIES );
	Message cReply;

	cReq.AddInt32( "handle", m_hFontHandle );
	cReq.AddFloat( "size", vSize );
	cReq.AddFloat( "rotation", vRotation );
	cReq.AddFloat( "shear", vShear );

	port_id hPort = Application::GetInstance()->GetAppPort();
	Messenger( hPort ).SendMessage( &cReq, &cReply );

	int nError = -EINVAL;

	cReply.FindInt( "error", &nError );

	if ( nError >= 0 ) {
	    int32 nAscender;
	    int32 nDescender;
	    int32 nLineGap;
	
	    cReply.FindInt( "ascender",  &nAscender );
	    cReply.FindInt( "descender", &nDescender );
	    cReply.FindInt( "line_gap",  &nLineGap );

	    m_sHeight.ascender  = nAscender;
	    m_sHeight.descender = -nDescender;
	    m_sHeight.line_gap  = nLineGap;
	    
	    m_vSize	= vSize;
	    m_vRotation	= vRotation;
	    m_vShear	= vShear;

	    return( 0 );
	} else {
	    errno = -nError;
	    return( -1 );
	}
    }
    return( 0 );
}

float Font::GetSize() const
{
    return( m_vSize );
}

float Font::GetShear() const
{
    return( m_vShear );
}

float Font::GetRotation() const
{
    return( m_vRotation );
}

int Font::GetSpacing() const
{
    return( 0 );
}

int Font::GetEncoding() const
{
    return( m_nEncoding );
}


uint32 Font::GetFlags() const
{
    return( m_nFlags );
}

font_direction Font::GetDirection() const
{
    return( FONT_LEFT_TO_RIGHT );
}

float Font::GetStringWidth( const char* pzString ) const
{
    return( GetStringWidth( pzString, strlen( pzString ) ) );
}

float Font::GetStringWidth( const char* pzString, int nLength ) const
{
    const char*	apzStrPtr[] = { pzString };
    float	vWidth;

    GetStringWidths( apzStrPtr, &nLength, 1, &vWidth );

    return( vWidth );
}

float Font::GetStringWidth( const std::string& cString ) const
{
    return( GetStringWidth( cString.c_str(), cString.size() ) );
}

void Font::GetStringWidths( const char** apzStringArray, const int* anLengthArray,
			    int nStringCount, float* avWidthArray ) const
{
    int	i;
      // The first string size, and one byte of the first string is already included
    int	nBufSize = sizeof( AR_GetStringWidths_s ) - sizeof( int ) - 1;	

    for ( i = 0 ; i < nStringCount ; ++i ) {
	nBufSize += anLengthArray[ i ] + sizeof( int );
    }

    AR_GetStringWidths_s* psReq;

    char* pBuffer = new char[ nBufSize ];
    psReq = (AR_GetStringWidths_s*) pBuffer;

    psReq->hReply	= m_hReplyPort;
    psReq->hFontToken	= m_hFontHandle;
    psReq->nStringCount	= nStringCount;

    int* pnLen = &psReq->sFirstHeader.nLength;

    for ( i = 0 ; i < nStringCount ; ++i ) {
	int	nLen =	anLengthArray[ i ];
	*pnLen = nLen;
	pnLen++;

	memcpy( pnLen, apzStringArray[i], nLen );
	pnLen = (int*) (((uint8*)pnLen) + nLen);
    }
    if ( send_msg( Application::GetInstance()->GetAppPort(), AR_GET_STRING_WIDTHS, psReq, nBufSize ) == 0 ) {
	AR_GetStringWidthsReply_s*	psReply = (AR_GetStringWidthsReply_s*) pBuffer;

	if ( get_msg( m_hReplyPort, NULL, psReply, nBufSize ) == 0 ) {
	    if ( 0 == psReply->nError ) {
		for ( i = 0 ; i < nStringCount ; ++i ) {
		    avWidthArray[ i ] = psReply->anLengths[ i ];
		}
	    }
	} else {
	    dbprintf( "Error: Font::GetStringWidths() failed to get reply from %d (%p) (%p)\n",
		      m_hReplyPort, &m_hReplyPort, this );
	}
    } else {
	dbprintf( "Error: Font::GetStringWidths() failed to send AR_GET_STRING_WIDTHS to server\n" );
    }
    delete[] pBuffer;
}

int Font::GetStringLength( const char* pzString, float vWidth, bool bIncludeLast ) const
{
    return( GetStringLength( pzString, strlen( pzString ), vWidth, bIncludeLast ) );
}

int Font::GetStringLength( const char* pzString, int nLength, float vWidth, bool bIncludeLast ) const
{
    const char*	apzStrPtr[] = { pzString };
    int		nMaxLength;

    GetStringLengths( apzStrPtr, &nLength, 1, vWidth, &nMaxLength, bIncludeLast );

    return( nMaxLength );
}

int Font::GetStringLength( const std::string& cString, float vWidth, bool bIncludeLast ) const
{
    const char*	apzStrPtr[] = { cString.c_str() };
    int		nMaxLength;
    int		nLength = cString.size();

    GetStringLengths( apzStrPtr, &nLength, 1, vWidth, &nMaxLength, bIncludeLast );

    return( nMaxLength );
}

void Font::GetStringLengths( const char** apzStringArray, const int* anLengthArray,
			       int nStringCount, float vWidth, int* anMaxLengthArray, bool bIncludeLast ) const
{
    int	i;
      // The first string size, and one byte of the first string is already included
    int	nBufSize = sizeof( AR_GetStringLengths_s ) - sizeof( int ) - 1;	

    for ( i = 0 ; i < nStringCount ; ++i ) {
	nBufSize += anLengthArray[ i ] + sizeof( int );
    }

    AR_GetStringLengths_s* psReq;

    char*	pBuffer = new char[ nBufSize ];
  
    psReq = (AR_GetStringLengths_s*) pBuffer;

    psReq->hReply 	= m_hReplyPort;
    psReq->hFontToken	= m_hFontHandle;
    psReq->nStringCount	= nStringCount;
    psReq->nWidth	= vWidth;
    psReq->bIncludeLast	= bIncludeLast;

    int* pnLen = &psReq->sFirstHeader.nLength;

    for ( i = 0 ; i < nStringCount ; ++i ) {
	int	nLen =	anLengthArray[ i ];
    
	*pnLen = nLen;
	pnLen++;

	memcpy( pnLen, apzStringArray[i], nLen );
	pnLen = (int*) (((uint8*)pnLen) + nLen);
    }

    if ( send_msg( Application::GetInstance()->GetAppPort(), AR_GET_STRING_LENGTHS, psReq, nBufSize ) == 0 ) {
	AR_GetStringLengthsReply_s*	psReply = (AR_GetStringLengthsReply_s*) pBuffer;

	if ( get_msg( m_hReplyPort, NULL, psReply, nBufSize ) == 0 ) {
	    if ( 0 == psReply->nError ) {
		for ( i = 0 ; i < nStringCount ; ++i ) {
		    anMaxLengthArray[ i ] = psReply->anLengths[ i ];
		}
	    }
	} else {
	    dbprintf( "Error: Font::GetStringLengths() failed to get reply\n" );
	}
    } else {
	dbprintf( "Error: Font::GetStringLengths() failed to send AR_GET_STRING_LENGTHS request to server\n" );
    }
    delete[] pBuffer;
}


/** Get a list of default font names.
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Font::GetConfigNames( std::vector<string>* pcTable )
{
    return( Application::GetInstance()->GetFontConfigNames( pcTable ) );
}

/** Get the properties of a default font.
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Font::GetDefaultFont( const std::string& cName, font_properties* psProps )
{
    return( Application::GetInstance()->GetDefaultFont( cName, psProps ) );
}

/** Set the properties of a default font.
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Font::SetDefaultFont( const std::string& cName, const font_properties& sProps )
{
    return( Application::GetInstance()->SetDefaultFont( cName, sProps ) );
}

/** Add a default font, or modify one if it already exists.
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Font::AddDefaultFont( const std::string& cName, const font_properties& sProps )
{
    return( Application::GetInstance()->AddDefaultFont( cName, sProps ) );
}

/** Get number of installed font families
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int Font::GetFamilyCount()
{
    return( Application::GetInstance()->GetFontFamilyCount() );
}

/** Get the name of a given font family
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Font::GetFamilyInfo( int nIndex, char* pzFamily )
{
    return( Application::GetInstance()->GetFontFamily( nIndex, pzFamily ) );
}

/** Get number of styles in a given family
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int Font::GetStyleCount( const char* pzFamily )
{
    return( Application::GetInstance()->GetFontStyleCount( pzFamily ) );
}

/** Get info about a given font style
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Font::GetStyleInfo( const char* pzFamily, int nIndex, char* pzStyle, uint32* pnFlags )
{
    return( Application::GetInstance()->GetFontStyle( pzFamily, nIndex, pzStyle, pnFlags ) );
}

status_t Font::GetBitmapSizes( const std::string& cFamily, const std::string& cStyle, Font::size_list_t* pcList )
{
    Message cReq( AR_GET_FONT_SIZES );
    Message cReply;

    cReq.AddString( "family", cFamily );
    cReq.AddString( "style", cStyle );
    
    if ( Messenger(Application::GetInstance()->GetAppPort()).SendMessage( &cReq, &cReply ) >= 0 ) {
	int    nError = -EINVAL;
	cReply.FindInt( "error", &nError );
	if ( nError < 0 ) {
	    errno = -nError;
	    return( -1 );
	}
	pcList->clear();
	float vSize;
	for ( uint i = 0 ; cReply.FindFloat( "size_table", &vSize, i ) == 0 ; ++i ) {
	    pcList->push_back( vSize );
	}
	return( 0 );
    } else {
	return( -1 );
    }
}

bool Font::Rescan()
{
    Message cReq( DR_RESCAN_FONTS );
    Message cReply;

    if ( Messenger(Application::GetInstance()->GetServerPort()).SendMessage( &cReq, &cReply ) >= 0 ) {
	bool bChanged = true;
	
	cReply.FindBool( "changed", &bChanged );
	return( bChanged );
    } else {
	dbprintf( "Font::Rescan() failed to send message to server: %s\n", strerror( errno ) );
	return( false );
    }
}

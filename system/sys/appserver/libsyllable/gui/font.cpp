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

struct flattened_font_properties
{
    char	m_zFamily[FONT_FAMILY_LENGTH];
    char	m_zStyle[FONT_STYLE_LENGTH];
    uint32	m_nFlags;
    float	m_vSize;
    float	m_vShear;
    float	m_vRotation;
};

void Font::_CommonInit()
{
	Message cReq( AR_CREATE_FONT );
	Message cReply;

	assert( Application::GetInstance() != NULL );

	atomic_set( &m_nRefCount, 1 );
	m_sProps.m_vSize = 0.0f;
	m_sProps.m_vShear = 0.0f;
	m_sProps.m_vRotation = 0.0f;
	m_nSpacing = CHAR_SPACING;
	m_nEncoding = 0;
	m_sProps.m_nFlags = 0;
	m_hFontHandle = -1;
	m_hReplyPort = create_port( "font_reply", DEFAULT_PORT_SIZE );

	port_id hPort = Application::GetInstance()->GetAppPort(  );

	Messenger( hPort ).SendMessage( &cReq, &cReply );
	cReply.FindInt( "handle", &m_hFontHandle );
}

Font::Font()
{
	_CommonInit();
}

Font::Font( const Font & cOrig )
{
	_CommonInit();

	// Make sure SetProperties() really takes action
	m_sProps.m_vSize = -1.0f;
	m_sProps.m_vRotation = -2.0f;
	m_sProps.m_vShear = -3.0f;


	SetFamilyAndStyle( cOrig.m_sProps.m_cFamily.c_str(), cOrig.m_sProps.m_cStyle.c_str(  ) );
	_SetProperties(cOrig.m_sProps);
}

Font::Font( const String & cConfigName )
{
	_CommonInit();
	font_properties sProps;

	if( GetDefaultFont( cConfigName, &sProps ) != 0 )
	{
		throw( std::runtime_error( "Configuration not found" ) );
	}

	SetFamilyAndStyle(sProps.m_cFamily.c_str(),sProps.m_cStyle.c_str());

	if( SetProperties( sProps ) != 0 )
	{
		throw( std::runtime_error( "Failed to set properties" ) );
	}
}

Font::Font( const font_properties & sProps )
{
	_CommonInit();
	if( SetProperties( sProps ) != 0 )
	{
		throw( std::runtime_error( "Failed to set properties" ) );
	}
}


Font::~Font()
{
	assert( Application::GetInstance() != NULL );
	assert( atomic_read( &m_nRefCount ) == 0 );

	Message cReq( AR_DELETE_FONT );

	cReq.AddInt32( "handle", m_hFontHandle );

	port_id hPort = Application::GetInstance()->GetAppPort(  );

	Messenger( hPort ).SendMessage( &cReq );
	delete_port( m_hReplyPort );
}

bool Font::operator==( const Font & cOther )
{
	return ( m_sProps.m_cFamily == cOther.m_sProps.m_cFamily && m_sProps.m_cStyle == cOther.m_sProps.m_cStyle && m_sProps.m_vSize == cOther.m_sProps.m_vSize && m_sProps.m_vShear == cOther.m_sProps.m_vShear && m_sProps.m_vRotation == cOther.m_sProps.m_vRotation && m_nSpacing == cOther.m_nSpacing && m_nEncoding == cOther.m_nEncoding && m_sProps.m_nFlags == cOther.m_sProps.m_nFlags );
}

bool Font::operator!=( const Font & cOther )
{
	return ( !(m_sProps.m_cFamily == cOther.m_sProps.m_cFamily) || !(m_sProps.m_cStyle == cOther.m_sProps.m_cStyle) || m_sProps.m_vSize != cOther.m_sProps.m_vSize || m_sProps.m_vShear != cOther.m_sProps.m_vShear || m_sProps.m_vRotation != cOther.m_sProps.m_vRotation || m_nSpacing != cOther.m_nSpacing || m_nEncoding != cOther.m_nEncoding || m_sProps.m_nFlags != cOther.m_sProps.m_nFlags );
}

Font & Font::operator=( const Font & cOther )
{
	SetFamilyAndStyle( cOther.m_sProps.m_cFamily.c_str(), cOther.m_sProps.m_cStyle.c_str(  ) );
	SetProperties( cOther.m_sProps );
	return ( *this );
}

void Font::AddRef()
{
	atomic_inc( &m_nRefCount );
}

void Font::Release()
{
	if( atomic_dec_and_test( &m_nRefCount ) )
	{
		delete this;
	}
}

status_t Font::SetProperties( const String & cConfigName )
{
	status_t nError;
	font_properties sProps;

	nError = GetDefaultFont( cConfigName, &sProps );
	if( nError != 0 )
	{
		return ( nError );;
	}
	return ( SetProperties( sProps ) );
}

status_t Font::SetProperties( const font_properties & sProps )
{
	status_t nError = SetFamilyAndStyle(sProps.m_cFamily.c_str(), sProps.m_cStyle.c_str(  ) );
	if( nError < 0 )
	{
		return ( nError );
	}
	return _SetProperties(sProps);
}

status_t Font::SetFamilyAndStyle( const char *pzFamily, const char *pzStyle )
{
	Message cReq( AR_SET_FONT_FAMILY_AND_STYLE );
	Message cReply;

	cReq.AddInt32( "handle", m_hFontHandle );
	cReq.AddString( "family", pzFamily );
	cReq.AddString( "style", pzStyle );

	port_id hPort = Application::GetInstance()->GetAppPort(  );

	Messenger( hPort ).SendMessage( &cReq, &cReply );

	int nError = -EINVAL;

	cReply.FindInt( "error", &nError );

	if( nError >= 0 )
	{
		int32 nAscender;
		int32 nDescender;
		int32 nLineGap;

		cReply.FindInt( "ascender", &nAscender );
		cReply.FindInt( "descender", &nDescender );
		cReply.FindInt( "line_gap", &nLineGap );

		m_sHeight.ascender = nAscender;
		m_sHeight.descender = -nDescender;
		m_sHeight.line_gap = nLineGap;

		m_sProps.m_cFamily = pzFamily;
		m_sProps.m_cStyle = pzStyle;

		return ( 0 );
	}
	else
	{
		errno = -nError;
		return ( -1 );
	}
}

void Font::SetSize( float vSize )
{
	m_sProps.m_vSize = vSize;

	_SetProperties(m_sProps);
}

void Font::SetShear(float vShear)
{
	m_sProps.m_vShear = vShear;

	_SetProperties(m_sProps);
}

void Font::SetRotation( float vRotation )
{
	m_sProps.m_vRotation = vRotation;
	_SetProperties(m_sProps);
}

void Font::SetFlags( uint32 nFlags )
{
	m_sProps.m_nFlags = nFlags;
	_SetProperties(m_sProps);
}


status_t Font::SetProperties( float vSize, float vShear, float vRotation )
{
	m_sProps.m_vSize = vSize;
	m_sProps.m_vShear = vShear;
	m_sProps.m_vRotation = vRotation;
	return _SetProperties(m_sProps);
}

status_t Font::_SetProperties(const font_properties& sProps)
{
	//if( vSize != m_vSize || vShear != m_vShear || vRotation != m_vRotation || nFlags != m_nFlags )
	//{
		Message cReq( AR_SET_FONT_PROPERTIES );
		Message cReply;

		cReq.AddInt32( "handle", m_hFontHandle );
		cReq.AddFloat( "size", sProps.m_vSize );
		cReq.AddFloat( "rotation", sProps.m_vRotation );
		cReq.AddFloat( "shear", sProps.m_vShear );
		cReq.AddInt32( "flags", sProps.m_nFlags );

		port_id hPort = Application::GetInstance()->GetAppPort(  );

		Messenger( hPort ).SendMessage( &cReq, &cReply );

		int nError = -EINVAL;

		cReply.FindInt( "error", &nError );

		if( nError >= 0 )
		{
			int32 nAscender;
			int32 nDescender;
			int32 nLineGap;

			cReply.FindInt( "ascender", &nAscender );
			cReply.FindInt( "descender", &nDescender );
			cReply.FindInt( "line_gap", &nLineGap );

			m_sHeight.ascender = nAscender;
			m_sHeight.descender = -nDescender;
			m_sHeight.line_gap = nLineGap;

			/*this has to be changed*/
			m_sProps.m_vSize = sProps.m_vSize;
			m_sProps.m_vRotation = sProps.m_vRotation;
			m_sProps.m_vShear = sProps.m_vShear;
			m_sProps.m_nFlags = sProps.m_nFlags;

			return ( 0 );
		}
		else
		{
			errno = -nError;
			return ( -1 );
		}
	//}
	return ( 0 );
}

String Font::GetFamily() const
{
	return ( m_sProps.m_cFamily );
}

String Font::GetStyle() const
{
	return ( m_sProps.m_cStyle );
}

float Font::GetSize() const
{
	return ( m_sProps.m_vSize );
}

float Font::GetShear() const
{
	return ( m_sProps.m_vShear );
}

float Font::GetRotation() const
{
	return ( m_sProps.m_vRotation );
}

int Font::GetSpacing() const
{
	return ( 0 );
}

int Font::GetEncoding() const
{
	return ( m_nEncoding );
}


uint32 Font::GetFlags() const
{
	return ( m_sProps.m_nFlags );
}

font_direction Font::GetDirection() const
{
	return ( FONT_LEFT_TO_RIGHT );
}

float Font::GetStringWidth( const char *pzString, int nLength ) const
{
	const char *apzStrPtr[] = { pzString };
	float vWidth;

	if( nLength == -1 ) nLength = strlen( pzString );

	GetStringWidths( apzStrPtr, &nLength, 1, &vWidth );

	return ( vWidth );
}

float Font::GetStringWidth( const String & cString ) const
{
	return ( GetStringWidth( cString.c_str(), cString.size(  ) ) );
}

void Font::GetStringWidths( const char **apzStringArray, const int *anLengthArray, int nStringCount, float *avWidthArray ) const
{
	int i;

	// The first string size, and one byte of the first string is already included
	int nBufSize = sizeof( AR_GetStringWidths_s ) - sizeof( int ) - 1;

	for( i = 0; i < nStringCount; ++i )
	{
		nBufSize += anLengthArray[i] + sizeof( int );
	}

	AR_GetStringWidths_s *psReq;

	char *pBuffer = new char[nBufSize];

	psReq = ( AR_GetStringWidths_s * ) pBuffer;

	psReq->hReply = m_hReplyPort;
	psReq->hFontToken = m_hFontHandle;
	psReq->nStringCount = nStringCount;

	int *pnLen = &psReq->sFirstHeader.nLength;

	for( i = 0; i < nStringCount; ++i )
	{
		int nLen = anLengthArray[i];

		*pnLen = nLen;
		pnLen++;

		memcpy( pnLen, apzStringArray[i], nLen );
		pnLen = ( int * )( ( ( uint8 * )pnLen ) + nLen );
	}
	if( send_msg( Application::GetInstance()->GetAppPort(  ), AR_GET_STRING_WIDTHS, psReq, nBufSize ) == 0 )
	{
		AR_GetStringWidthsReply_s *psReply = ( AR_GetStringWidthsReply_s * ) pBuffer;

		if( get_msg( m_hReplyPort, NULL, psReply, nBufSize ) == 0 )
		{
			if( 0 == psReply->nError )
			{
				for( i = 0; i < nStringCount; ++i )
				{
					avWidthArray[i] = psReply->anLengths[i];
				}
			}
		}
		else
		{
			dbprintf( "Error: Font::GetStringWidths() failed to get reply from %d (%p) (%p)\n", m_hReplyPort, &m_hReplyPort, this );
		}
	}
	else
	{
		dbprintf( "Error: Font::GetStringWidths() failed to send AR_GET_STRING_WIDTHS to server\n" );
	}
	delete[]pBuffer;
}

Point Font::GetTextExtent( const char *pzString, int nLength, uint32 nFlags, int nTargetWidth ) const
{
	const char *apzStrPtr[] = { pzString };
	Point cExt;

	if( nLength == -1 ) nLength = strlen( pzString );

	GetTextExtents( apzStrPtr, &nLength, 1, &cExt, nFlags, nTargetWidth );

	return ( cExt );
}

Point Font::GetTextExtent( const String & cString, uint32 nFlags, int nTargetWidth ) const
{
	return ( GetTextExtent( cString.c_str(), cString.size(), nFlags, nTargetWidth ) );
}

void Font::GetTextExtents( const char **apzStringArray, const int *anLengthArray, int nStringCount, Point *acExtentArray, uint32 nFlags, int nTargetWidth ) const
{
	int i;

	// The first string size, and one byte of the first string is already included
	int nBufSize = sizeof( AR_GetTextExtents_s ) - sizeof( int ) - 1;

	for( i = 0; i < nStringCount; ++i )
	{
		nBufSize += anLengthArray[i] + sizeof( int );
	}

	AR_GetTextExtents_s *psReq;

	char *pBuffer = new char[nBufSize+50];

	psReq = ( AR_GetTextExtents_s * ) pBuffer;

	psReq->hReply = m_hReplyPort;
	psReq->hFontToken = m_hFontHandle;
	psReq->nStringCount = nStringCount;
	psReq->nFlags = nFlags;
	psReq->nTargetWidth = nTargetWidth;

	int *pnLen = &psReq->sFirstHeader.nLength;

	for( i = 0; i < nStringCount; ++i )
	{
		int nLen = anLengthArray[i];

		*pnLen = nLen;
		pnLen++;

		memcpy( pnLen, apzStringArray[i], nLen );
		pnLen = ( int * )( ( ( uint8 * )pnLen ) + nLen );
	}

	if( send_msg( Application::GetInstance()->GetAppPort(  ), AR_GET_TEXT_EXTENTS, psReq, nBufSize ) == 0 )
	{
		AR_GetTextExtentsReply_s *psReply = ( AR_GetTextExtentsReply_s * ) pBuffer;

		if( get_msg( m_hReplyPort, NULL, psReply, nBufSize ) == 0 )
		{
			if( 0 == psReply->nError )
			{
				for( i = 0; i < nStringCount; ++i )
				{
					acExtentArray[i] = Point(psReply->acExtent[i].x, psReply->acExtent[i].y);
				}
			}
		}
		else
		{
			dbprintf( "Error: Font::GetStringExtents() failed to get reply from %d (%p) (%p)\n", m_hReplyPort, &m_hReplyPort, this );
		}
	}
	else
	{
		dbprintf( "Error: Font::GetStringExtents() failed to send AR_GET_STRING_EXTENTS to server\n" );
	}
	

	delete[]pBuffer;

}

int Font::GetStringLength( const char *pzString, float vWidth, bool bIncludeLast ) const
{
	return ( GetStringLength( pzString, strlen( pzString ), vWidth, bIncludeLast ) );
}

int Font::GetStringLength( const char *pzString, int nLength, float vWidth, bool bIncludeLast ) const
{
	const char *apzStrPtr[] = { pzString };
	int nMaxLength;

	if( nLength == -1 ) nLength = strlen( pzString );

	GetStringLengths( apzStrPtr, &nLength, 1, vWidth, &nMaxLength, bIncludeLast );

	return ( nMaxLength );
}

int Font::GetStringLength( const String & cString, float vWidth, bool bIncludeLast ) const
{
	const char *apzStrPtr[] = { cString.c_str() };
	int nMaxLength;
	int nLength = cString.size();

	GetStringLengths( apzStrPtr, &nLength, 1, vWidth, &nMaxLength, bIncludeLast );

	return ( nMaxLength );
}

void Font::GetStringLengths( const char **apzStringArray, const int *anLengthArray, int nStringCount, float vWidth, int *anMaxLengthArray, bool bIncludeLast ) const
{
	int i;

	// The first string size, and one byte of the first string is already included
	int nBufSize = sizeof( AR_GetStringLengths_s ) - sizeof( int ) - 1;

	for( i = 0; i < nStringCount; ++i )
	{
		nBufSize += anLengthArray[i] + sizeof( int );
	}

	AR_GetStringLengths_s *psReq;

	char *pBuffer = new char[nBufSize];

	psReq = ( AR_GetStringLengths_s * ) pBuffer;

	psReq->hReply = m_hReplyPort;
	psReq->hFontToken = m_hFontHandle;
	psReq->nStringCount = nStringCount;
	psReq->nWidth = ( int )vWidth;
	psReq->bIncludeLast = bIncludeLast;

	int *pnLen = &psReq->sFirstHeader.nLength;

	for( i = 0; i < nStringCount; ++i )
	{
		int nLen = anLengthArray[i];

		*pnLen = nLen;
		pnLen++;

		memcpy( pnLen, apzStringArray[i], nLen );
		pnLen = ( int * )( ( ( uint8 * )pnLen ) + nLen );
	}

	if( send_msg( Application::GetInstance()->GetAppPort(  ), AR_GET_STRING_LENGTHS, psReq, nBufSize ) == 0 )
	{
		AR_GetStringLengthsReply_s *psReply = ( AR_GetStringLengthsReply_s * ) pBuffer;

		if( get_msg( m_hReplyPort, NULL, psReply, nBufSize ) == 0 )
		{
			if( 0 == psReply->nError )
			{
				for( i = 0; i < nStringCount; ++i )
				{
					anMaxLengthArray[i] = psReply->anLengths[i];
				}
			}
		}
		else
		{
			dbprintf( "Error: Font::GetStringLengths() failed to get reply\n" );
		}
	}
	else
	{
		dbprintf( "Error: Font::GetStringLengths() failed to send AR_GET_STRING_LENGTHS request to server\n" );
	}
	delete[]pBuffer;
}

std::vector<uint32> Font::GetSupportedCharacters() const
{
	std::vector<uint32> cCharCodes;
	int32 nCharCode = 0;
	bool bMoreChars;

	do {
		Message cReq( AR_GET_FONT_CHARACTERS );
		Message cReply;

		cReq.AddInt32( "handle", m_hFontHandle );
		cReq.AddInt32( "lastchar", nCharCode );
		cReq.AddInt32( "maxchars", 1000 );

		port_id hPort = Application::GetInstance()->GetAppPort(  );

		Messenger( hPort ).SendMessage( &cReq, &cReply );

		int nError = -EINVAL;

		cReply.FindInt( "error", &nError );

		if( nError >= 0 )
		{
			int i = 0;
	
			while( cReply.FindInt32( "character_code", &nCharCode, i++ ) == 0 ) {
				cCharCodes.push_back( nCharCode );
			}
		}
		else
		{
			errno = -nError;
			dbprintf("Font::GetSupportedCharacters() failed: %d\n", nError);
			break;
		}
		
		bMoreChars = false;
		cReply.FindBool( "more_chars", &bMoreChars );
	} while( bMoreChars );

	return ( cCharCodes );
}


/** Get a list of default font names.
 * \par Description:
 *  This method will give you a list of all the default font names.
 * \param pcTable - A vector pointer of os::String's to return all the config names in.
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Font::GetConfigNames( std::vector < String > *pcTable )
{
	return ( Application::GetInstance()->GetFontConfigNames( pcTable ) );
}

/** Get the properties of a default font.
 * \par Description:
	Gets the properties of the default font.
 * \param  cName   - Name of the default fFont.
 * \param  psProps - font_propties pointer which will contain the properties of the default Font.
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Font::GetDefaultFont( const String & cName, font_properties * psProps )
{
	return ( Application::GetInstance()->GetDefaultFont( cName, psProps ) );
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

status_t Font::SetDefaultFont( const String & cName, const font_properties & sProps )
{
	return ( Application::GetInstance()->SetDefaultFont( cName, sProps ) );
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

status_t Font::AddDefaultFont( const String & cName, const font_properties & sProps )
{
	return ( Application::GetInstance()->AddDefaultFont( cName, sProps ) );
}

/** Get number of installed Font families
 * \par Description:
 *  Returns the number of Font families
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int Font::GetFamilyCount()
{
	return ( Application::GetInstance()->GetFontFamilyCount(  ) );
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

status_t Font::GetFamilyInfo( int nIndex, char *pzFamily )
{
	return ( Application::GetInstance()->GetFontFamily( nIndex, pzFamily ) );
}

/** Get number of styles in a given family
 * \par Description:
 *  Returns the number of styles in the family
 * \param pzFamily - The family.
 * \return The number of styles.
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int Font::GetStyleCount( const char *pzFamily )
{
	return ( Application::GetInstance()->GetFontStyleCount( pzFamily ) );
}

/** Get info about a given font style
 * \par Description: 
 *  Get all the information about the Font. 
 * \param pzFamily - The family of the Font.
 * \param nIndex   - The index of the family.
 * \param pzStyle  - The style of the Font.
 * \param pnFlags  - The flags of the Font.
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Font::GetStyleInfo( const char *pzFamily, int nIndex, char *pzStyle, uint32 *pnFlags )
{
	return ( Application::GetInstance()->GetFontStyle( pzFamily, nIndex, pzStyle, pnFlags ) );
}

status_t Font::GetBitmapSizes( const String & cFamily, const String & cStyle, Font::size_list_t *pcList )
{
	Message cReq( AR_GET_FONT_SIZES );
	Message cReply;

	cReq.AddString( "family", cFamily );
	cReq.AddString( "style", cStyle );

	if( Messenger( Application::GetInstance()->GetAppPort(  ) ).SendMessage( &cReq, &cReply ) >= 0 )
	{
		int nError = -EINVAL;

		cReply.FindInt( "error", &nError );
		if( nError < 0 )
		{
			errno = -nError;
			return ( -1 );
		}
		pcList->clear();
		float vSize;

		for( uint i = 0; cReply.FindFloat( "size_table", &vSize, i ) == 0; ++i )
		{
			pcList->push_back( vSize );
		}
		return ( 0 );
	}
	else
	{
		return ( -1 );
	}
}

bool Font::Rescan()
{
	Message cReq( DR_RESCAN_FONTS );
	Message cReply;

	if( Messenger( Application::GetInstance()->GetServerPort(  ) ).SendMessage( &cReq, &cReply ) >= 0 )
	{
		bool bChanged = true;

		cReply.FindBool( "changed", &bChanged );
		return ( bChanged );
	}
	else
	{
		dbprintf( "Font::Rescan() failed to send message to server: %s\n", strerror( errno ) );
		return ( false );
	}
}

size_t 		Font::GetFlattenedSize( void ) const
{
	return sizeof(flattened_font_properties);
}

status_t Font::Flatten( uint8* pBuffer, size_t nSize ) const 
{
	//sProps = GetFontProperties();
	flattened_font_properties sProps;
	strncpy( sProps.m_zFamily, m_sProps.m_cFamily.c_str(), 63 );
	strncpy( sProps.m_zStyle, m_sProps.m_cStyle.c_str(), 63 );
	sProps.m_nFlags = m_sProps.m_nFlags;
	sProps.m_vSize = m_sProps.m_vSize;
	sProps.m_vShear = m_sProps.m_vShear;
	sProps.m_vRotation = m_sProps.m_vRotation;
	if( nSize >= sizeof( sProps ) ) 
	{
		memcpy( pBuffer, &sProps, sizeof( sProps ) );
		return 0;
    } 
	else 
	{
		return -1;
	}
}
	
status_t	Font::Unflatten( const uint8* pBuffer )
{
	flattened_font_properties* psProps = (flattened_font_properties*)pBuffer;
	m_sProps.m_cFamily = psProps->m_zFamily;
	m_sProps.m_cStyle = psProps->m_zStyle;
	m_sProps.m_nFlags = psProps->m_nFlags;
	m_sProps.m_vSize =  psProps->m_vSize;
	m_sProps.m_vShear =  psProps->m_vShear;
	m_sProps.m_vRotation = psProps->m_vRotation;
	SetProperties(m_sProps);
	return 0;
}

void Font::_InitFontProperties()
{
	m_sProps.m_cFamily = GetFamily();
	m_sProps.m_cStyle = GetStyle();
	m_sProps.m_nFlags = GetFlags();
	m_sProps.m_vSize = GetSize();
	m_sProps.m_vShear = GetShear();
	m_sProps.m_vRotation = GetRotation();
}




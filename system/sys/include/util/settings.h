/*  Settings - Manager for settings files.
 *  Copyright (C)2002 Henrik Isaksson
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

#ifndef	__F_UTIL_SETTINGS_H__
#define	__F_UTIL_SETTINGS_H__

#include <util/message.h>
#include <storage/file.h>
#include <storage/path.h>
#include <storage/seekableio.h>

#define SETTINGS_MAGIC		0x53626C53	// Magic number to recognize files
#define SETTINGS_VERSION	0x00000001	// Version number, changed when compatibility breaks

namespace os
{

/** Utility class for storage of user settings and preferences.
 * \ingroup util
 * \par Description:
 *	The Settings class handles persistent storage of collections of
 *	data associated with names. You can of course also use the class to
 *	store other kinds of data. Settings is based on the very versatile
 *	name/value association mechanisms of os::Message, but it also extends
 *	the API with easy-to-use versions of the methods in os::Message,
 *	which are more suitable for managing settings.
 *	
 * \sa os::Message
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/

class Settings : public Message
{
public:
    Settings();
    Settings( SeekableIO *pcFile );
    ~Settings();

    status_t	Load( void );
    status_t	Save( void ) const;

    Settings&	operator=( const Settings& cSource );
    Settings&	operator=( const Message& cSource );

	Path	GetPath() const;
	File	GetFile() const;
	void	SetPath( Path* pcPath );
	void	SetFile( SeekableIO* pcFile );
	void	SetFile( String cFilename );

    String	GetString( const char* pzName, const char* pzDefault = "", int nIndex = 0 ) const;
    int8	GetInt8( const char* pzName, int8 nDefault, int nIndex = 0 ) const;
    int16	GetInt16( const char* pzName, int16 nDefault, int nIndex = 0 ) const;
    int32	GetInt32( const char* pzName, int32 nDefault, int nIndex = 0 ) const;
    int64	GetInt64( const char* pzName, int64 nDefault, int nIndex = 0 ) const;
    bool	GetBool( const char* pzName, bool bDefault, int nIndex = 0 ) const;
    float	GetFloat( const char* pzName, float vDefault, int nIndex = 0 ) const;
    double	GetDouble( const char* pzName, double vDefault, int nIndex = 0 ) const;
    Rect	GetRect( const char* pzName, const Rect& cDefault, int nIndex = 0 ) const;
    IRect	GetIRect( const char* pzName, const IRect& cDefault, int nIndex = 0 ) const;
    Point	GetPoint( const char* pzName, const Point& cDefault, int nIndex = 0 ) const;
    IPoint	GetIPoint( const char* pzName, const IPoint& cDefault, int nIndex = 0 ) const;
    Color32_s	GetColor32( const char* pzName, const Color32_s& cDefault, int nIndex = 0 ) const;
    Variant	GetVariant( const char* pzName, const Variant& cDefault, int nIndex = 0 ) const;
	// Note: GetMessage() and GetPointer() are deliberately left out.
	// Reason: You can't store pointers in settings files, and it would not
	// be efficent to work with this concept of default values with messages,
	// which can grow pretty big. Use Message::FindMessage() instead.

    status_t	SetData( const char* pzName, int nType, const void* pData, uint32 nSize, int nIndex = 0, bool bFixedSize = true, int nMaxCountHint = 1 );
    status_t	SetString( const char* pzName, const String& cValue, int nIndex = 0 );
    status_t	SetInt8( const char* pzName, int8 cValue, int nIndex = 0 );
    status_t	SetInt16( const char* pzName, int16 cValue, int nIndex = 0 );
    status_t	SetInt32( const char* pzName, int32 cValue, int nIndex = 0 );
    status_t	SetInt64( const char* pzName, int64 cValue, int nIndex = 0 );
    status_t	SetBool( const char* pzName, bool cValue, int nIndex = 0 );
    status_t	SetFloat( const char* pzName, float cValue, int nIndex = 0 );
    status_t	SetDouble( const char* pzName, double cValue, int nIndex = 0 );
    status_t	SetRect( const char* pzName, const Rect& cValue, int nIndex = 0 );
    status_t	SetIRect( const char* pzName, const IRect& cValue, int nIndex = 0 );
    status_t	SetPoint( const char* pzName, const Point& cValue, int nIndex = 0 );
    status_t	SetIPoint( const char* pzName, const IPoint& cValue, int nIndex = 0 );
    status_t	SetColor32( const char* pzName, const Color32_s& cValue, int nIndex = 0 );
    status_t	SetMessage( const char* pzName, const Message& cValue, int nIndex = 0 );
    status_t	SetVariant( const char* pzName, const Variant& cValue, int nIndex = 0 );

private:

    Chunk_s*	_ResizeChunk( DataArray_s* psArray, Chunk_s* psChunk, uint32 nSize );
    
    struct FileHeader {
	uint32	nMagic;
	uint32  nHeaderSize;
	uint32	nVersion;
	uint32	nSize;
    };

    class Private;
    Private* m;
};


} // end of namespace

#endif // __F_UTIL_SETTINGS_H__


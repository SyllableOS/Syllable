/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 2001 Kurt Skauen
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

#ifndef __F_UTIL_VARIANT_H__
#define __F_UTIL_VARIANT_H__

#include <atheos/types.h>
#include <util/string.h>
#include <util/typecodes.h>

#include <gui/rect.h>
#include <gui/point.h>
#include <gui/gfxtypes.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

class Point;
class IPoint;
class Rect;
class IRect;
class Message;

class Variant
{
public:
    Variant();
    Variant( int nValue );
    Variant( int64 nValue );
    Variant( float vValue );
    Variant( double vValue );
    Variant( bool bValue );
    Variant( const String& cString );
    Variant( const Point& cValue );
    Variant( const IPoint& cValue );
    Variant( const Rect& cValue );
    Variant( const IRect& cValue );
    Variant( const Message& cValue );
    Variant( const Color32& cValue );
    Variant( void* pValue );
    Variant( void* pData, int nSize );
    Variant( const Variant& cOther );
    ~Variant();

    void SetInt8( int8 nValue );
    void SetInt16( int16 nValue );
    void SetInt32( int32 nValue );
    void SetInt64( int64 nValue );
    void SetFloat( float vValue );
    void SetDouble( double vValue );
    void SetBool( bool bValue );
    void SetString( const String& cValue );
    void SetPoint( const Point& cValue );
    void SetIPoint( const IPoint& cValue );
    void SetRect( const Rect& cValue );
    void SetIRect( const IRect& cValue );
    void SetColor32( const Color32& cValue );
    void SetPointer( void* pValue  );
    void SetRaw( const void* pData, int nSize );
    
    int8	AsInt8() const;
    int16	AsInt16() const;
    int32	AsInt32() const;
    int64	AsInt64() const;
    float	AsFloat() const;
    double	AsDouble() const;
    bool	AsBool() const;
    String	AsString() const;
    Point	AsPoint() const;
    IPoint	AsIPoint() const;
    Rect	AsRect() const;
    IRect	AsIRect() const;
    Color32	AsColor32() const;
    void*	AsPointer() const;
    void*	AsRaw( size_t* pnSize );

    Variant& operator=( int8 nValue );
    Variant& operator=( int16 nValue );
    Variant& operator=( int32 nValue );
    Variant& operator=( int64 nValue );
    Variant& operator=( float vValue );
    Variant& operator=( double vValue );
    Variant& operator=( bool bValue );
    Variant& operator=( const String& cValue );
    Variant& operator=( const Point& cValue );
    Variant& operator=( const IPoint& cValue );
    Variant& operator=( const Rect& cValue );
    Variant& operator=( const IRect& cValue );
    Variant& operator=( const Color32& cValue );
    Variant& operator=( const Variant& cValue );
    
    operator int() const;
    operator int64() const;
    operator float() const;
    operator double() const;
    operator bool() const;
    operator String() const;
    operator Point() const;
    operator IPoint() const;
    operator Rect() const;
    operator IRect() const;
    operator Color32() const;
    
    bool operator==( const Variant& cValue ) const;

    bool operator!=( const Variant& cValue ) const;

    size_t GetFlattenedSize() const;
    status_t Flatten( void* pBuffer, size_t nBufSize ) const;
    status_t Unflatten( const void* pBuffer, size_t nBufSize );
    
private:
    void _Clear();

    type_code m_eType;
    union {
	int32  nInt32;
	int64  nInt64;
	float  vFloat;
	double vDouble;
	bool   bBool;
	void*  pData;
	char*  pzString;
	struct {
	    size_t nSize;
	    void*  pBuffer;
	} sBuffer;
    } m_uData;
};


} // end of namespace

#endif // __F_UTIL_VARIANT_H__

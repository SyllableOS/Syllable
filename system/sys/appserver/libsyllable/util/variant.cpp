
/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 2001  Kurt Skauen
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

/*

	Type code     Alloc type    Storage
	=========     ==========    =======
    T_ANY_TYPE    none          none
    T_POINTER     aggregate     m_uData.pData
    T_INT8        aggregate     m_uData.nInt32
    T_INT16       aggregate     m_uData.nInt32
    T_INT32       aggregate     m_uData.nInt32
    T_INT64       aggregate     m_uData.nInt64
    T_BOOL        aggregate     m_uData.bBool
    T_FLOAT       aggregate     m_uData.vFloat
    T_DOUBLE      aggregate     m_uData.vDouble
    T_STRING      heap          m_uData.pzString
    T_IRECT       heap          m_uData.sBuffer.pBuffer / m_uData.sBuffer.nSize
    T_IPOINT      heap          m_uData.sBuffer.pBuffer / m_uData.sBuffer.nSize
    T_MESSAGE     heap          m_uData.sBuffer.pBuffer / m_uData.sBuffer.nSize
    T_COLOR32     heap          m_uData.sBuffer.pBuffer / m_uData.sBuffer.nSize
    T_RECT        heap          m_uData.sBuffer.pBuffer / m_uData.sBuffer.nSize
    T_POINT       heap          m_uData.sBuffer.pBuffer / m_uData.sBuffer.nSize
    T_VARIANT     invalid       invalid
    T_RAW         heap          m_uData.sBuffer.pBuffer / m_uData.sBuffer.nSize

*/

#include <stdio.h>
#include <util/variant.h>
#include <util/message.h>
#include <iostream>

#define HEAP_ALLOC(type, var)		m_eType = type;	\
									m_uData.sBuffer.nSize = sizeof( var ); \
									m_uData.sBuffer.pBuffer = new int8[m_uData.sBuffer.nSize]; \
									memcpy( m_uData.sBuffer.pBuffer, &var, m_uData.sBuffer.nSize );

using namespace os;

/**************************************************************************/
/*** CONSTRUCTORS *********************************************************/
/**************************************************************************/

Variant::Variant()
{
	m_eType = T_ANY_TYPE;
}

Variant::Variant( int nValue )
{
	m_eType = T_INT32;
	m_uData.nInt32 = nValue;
}

Variant::Variant( int64 nValue )
{
	m_eType = T_INT64;
	m_uData.nInt64 = nValue;
}

Variant::Variant( float vValue )
{
	m_eType = T_FLOAT;
	m_uData.vFloat = vValue;
}

Variant::Variant( double vValue )
{
	m_eType = T_DOUBLE;
	m_uData.vDouble = vValue;
}

Variant::Variant( bool bValue )
{
	m_eType = T_BOOL;
	m_uData.bBool = bValue;
}

Variant::Variant( const String &cString )
{
	m_eType = T_STRING;
	m_uData.pzString = new char[cString.size() + 1];

	memcpy( m_uData.pzString, cString.c_str(), cString.size(  ) + 1 );
}

Variant::Variant( const Point & cValue )
{
	HEAP_ALLOC( T_POINT, cValue );
}

Variant::Variant( const IPoint & cValue )
{
	HEAP_ALLOC( T_IPOINT, cValue );
}

Variant::Variant( const Rect & cValue )
{
	HEAP_ALLOC( T_RECT, cValue );
}

Variant::Variant( const IRect & cValue )
{
	HEAP_ALLOC( T_IRECT, cValue );
}

Variant::Variant( const Message & cValue )
{
	m_eType = T_MESSAGE;
	m_uData.sBuffer.nSize = cValue.GetFlattenedSize();
	m_uData.sBuffer.pBuffer = new int8[m_uData.sBuffer.nSize];

	cValue.Flatten( ( uint8 * )m_uData.sBuffer.pBuffer, m_uData.sBuffer.nSize );
}

Variant::Variant( const Color32_s& cValue )
{
	HEAP_ALLOC( T_COLOR32, cValue );
}

Variant::Variant( void *pValue )
{
	m_eType = T_POINTER;
	m_uData.pData = pValue;
}

Variant::Variant( void *pData, int nSize )
{
	m_eType = T_RAW;
	m_uData.sBuffer.nSize = nSize;
	m_uData.sBuffer.pBuffer = new int8[nSize];

	memcpy( m_uData.sBuffer.pBuffer, pData, nSize );
}

Variant::Variant( const Variant & cOther )
{
	m_eType = T_ANY_TYPE;
	*this = cOther;
}

Variant::~Variant()
{
	_Clear();
}

/**************************************************************************/
/*** PRIVATE METHODS ******************************************************/
/**************************************************************************/

void Variant::_Clear()
{
	switch ( m_eType )
	{
	case T_STRING:
		delete[]m_uData.pzString;
		break;
	case T_MESSAGE:
	case T_VARIANT:
	case T_RAW:
	case T_RECT:
	case T_IRECT:
	case T_POINT:
	case T_IPOINT:
	case T_COLOR32:
		delete[]reinterpret_cast < int8 *>( m_uData.sBuffer.pBuffer );
		break;
	default:
		break;
	}
	m_eType = T_ANY_TYPE;
}

/**************************************************************************/
/*** SET METHODS **********************************************************/
/**************************************************************************/

void Variant::SetInt8( int8 nValue )
{
	_Clear();
	m_eType = T_INT8;
	m_uData.nInt32 = nValue;
}

void Variant::SetInt16( int16 nValue )
{
	_Clear();
	m_eType = T_INT16;
	m_uData.nInt32 = nValue;
}
void Variant::SetInt32( int32 nValue )
{
	_Clear();
	m_eType = T_INT32;
	m_uData.nInt32 = nValue;
}
void Variant::SetInt64( int64 nValue )
{
	_Clear();
	m_eType = T_INT64;
	m_uData.nInt64 = nValue;
}
void Variant::SetFloat( float vValue )
{
	_Clear();
	m_eType = T_FLOAT;
	m_uData.vFloat = vValue;
}
void Variant::SetDouble( double vValue )
{
	_Clear();
	m_eType = T_DOUBLE;
	m_uData.vDouble = vValue;
}
void Variant::SetBool( bool bValue )
{
	_Clear();
	m_eType = T_BOOL;
	m_uData.bBool = bValue;
}
void Variant::SetString( const String &cValue )
{
	_Clear();
	m_eType = T_STRING;
	m_uData.pzString = new char[cValue.size() + 1];

	memcpy( m_uData.pzString, cValue.c_str(), cValue.size(  ) + 1 );
}

void Variant::SetPoint( const Point & cValue )
{
	_Clear();
	HEAP_ALLOC( T_POINT, cValue );
}
void Variant::SetIPoint( const IPoint & cValue )
{
	_Clear();
	HEAP_ALLOC( T_IPOINT, cValue );
}
void Variant::SetRect( const Rect & cValue )
{
	_Clear();
	HEAP_ALLOC( T_RECT, cValue );
}
void Variant::SetIRect( const IRect & cValue )
{
	_Clear();
	HEAP_ALLOC( T_IRECT, cValue );
}
void Variant::SetColor32( const Color32 & cValue )
{
	_Clear();
	HEAP_ALLOC( T_COLOR32, cValue );
}

void Variant::SetPointer( void *pValue )
{
	_Clear();
	m_eType = T_POINTER;
	m_uData.pData = pValue;
}

void Variant::SetRaw( const void *pData, int nSize )
{
	_Clear();
	m_eType = T_RAW;
	m_uData.sBuffer.nSize = nSize;
	m_uData.sBuffer.pBuffer = new int8[nSize];

	memcpy( m_uData.sBuffer.pBuffer, pData, nSize );
}

/**************************************************************************/
/*** GET METHODS **********************************************************/
/**************************************************************************/

int8 Variant::AsInt8() const
{

	switch ( m_eType )
	{
	case T_POINTER:
		return ( int ( m_uData.pData ) );
	case T_INT8:
	case T_INT16:
	case T_INT32:
		return ( m_uData.nInt32 );
	case T_INT64:
		return ( int ( m_uData.nInt64 ) );
	case T_BOOL:
		return ( int ( m_uData.bBool ) );
	case T_FLOAT:
		return ( int ( m_uData.vFloat ) );
	case T_DOUBLE:
		return ( int ( m_uData.vDouble ) );
	case T_STRING:
		{
			int nValue;

			if( sscanf( m_uData.pzString, "%d", &nValue ) == 1 )
			{
				return ( nValue );
			}
			else
			{
				return ( 0 );
			}
		}
	default:
		return ( 0 );
	}
}
int16 Variant::AsInt16() const
{

	switch ( m_eType )
	{
	case T_POINTER:
		return ( int ( m_uData.pData ) );
	case T_INT8:
	case T_INT16:
	case T_INT32:
		return ( m_uData.nInt32 );
	case T_INT64:
		return ( int ( m_uData.nInt64 ) );
	case T_BOOL:
		return ( int ( m_uData.bBool ) );
	case T_FLOAT:
		return ( int ( m_uData.vFloat ) );
	case T_DOUBLE:
		return ( int ( m_uData.vDouble ) );
	case T_STRING:
		{
			int nValue;

			if( sscanf( m_uData.pzString, "%d", &nValue ) == 1 )
			{
				return ( nValue );
			}
			else
			{
				return ( 0 );
			}
		}
	default:
		return ( 0 );
	}
}
int32 Variant::AsInt32() const
{

	switch ( m_eType )
	{
	case T_POINTER:
		return ( int ( m_uData.pData ) );
	case T_INT8:
	case T_INT16:
	case T_INT32:
		return ( m_uData.nInt32 );
	case T_INT64:
		return ( int32 ( m_uData.nInt64 ) );
	case T_BOOL:
		return ( int32 ( m_uData.bBool ) );
	case T_FLOAT:
		return ( int32 ( m_uData.vFloat ) );
	case T_DOUBLE:
		return ( int32 ( m_uData.vDouble ) );
	case T_STRING:
		{
			int nValue;

			if( sscanf( m_uData.pzString, "%d", &nValue ) == 1 )
			{
				return ( nValue );
			}
			else
			{
				return ( 0 );
			}
		}
    case T_COLOR32:
		return ( uint32 ( *reinterpret_cast < Color32 * >( m_uData.sBuffer.pBuffer ) ) );
	default:
		return ( 0 );
	}
}

int64 Variant::AsInt64() const
{
	switch ( m_eType )
	{
	case T_POINTER:
		return ( int64 ( m_uData.pData ) );
	case T_INT8:
	case T_INT16:
	case T_INT32:
		return ( int64 ( m_uData.nInt32 ) );
	case T_INT64:
		return ( m_uData.nInt64 );
	case T_BOOL:
		return ( int64 ( m_uData.bBool ) );
	case T_FLOAT:
		return ( int64 ( m_uData.vFloat ) );
	case T_DOUBLE:
		return ( int64 ( m_uData.vDouble ) );
	case T_STRING:
		{
			int64 nValue;

			if( sscanf( m_uData.pzString, "%Ld", &nValue ) == 1 )
			{
				return ( nValue );
			}
			else
			{
				return ( 0LL );
			}
		}
    case T_COLOR32:
		return ( uint32 ( *reinterpret_cast < Color32 * >( m_uData.sBuffer.pBuffer ) ) );
	default:
		return ( 0 );
	}
}
float Variant::AsFloat() const
{
	switch ( m_eType )
	{
	case T_POINTER:
		return ( float ( int ( m_uData.pData ) ) );

	case T_INT8:
	case T_INT16:
	case T_INT32:
		return ( float ( m_uData.nInt32 ) );
	case T_INT64:
		return ( float ( m_uData.nInt64 ) );
	case T_BOOL:
		return ( float ( m_uData.bBool ) );
	case T_FLOAT:
		return ( m_uData.vFloat );
	case T_DOUBLE:
		return ( float ( m_uData.vDouble ) );
	case T_STRING:
		{
			float vValue;

			if( sscanf( m_uData.pzString, "%f", &vValue ) == 1 )
			{
				return ( vValue );
			}
			else
			{
				return ( 0.0f );
			}
		}
	default:
		return ( 0 );
	}
}
double Variant::AsDouble() const
{
	switch ( m_eType )
	{
	case T_POINTER:
		return ( double ( int ( m_uData.pData ) ) );

	case T_INT8:
	case T_INT16:
	case T_INT32:
		return ( double ( m_uData.nInt32 ) );
	case T_INT64:
		return ( double ( m_uData.nInt64 ) );
	case T_BOOL:
		return ( double ( m_uData.bBool ) );
	case T_FLOAT:
		return ( double ( m_uData.vFloat ) );
	case T_DOUBLE:
		return ( m_uData.vDouble );
	case T_STRING:
		{
			double vValue;

			if( sscanf( m_uData.pzString, "%lf", &vValue ) == 1 )
			{
				return ( vValue );
			}
			else
			{
				return ( 0.0 );
			}
		}
	default:
		return ( 0 );
	}

}
bool Variant::AsBool() const
{
	switch ( m_eType )
	{
	case T_POINTER:
		return ( bool ( m_uData.pData ) );
	case T_INT8:
	case T_INT16:
	case T_INT32:
		return ( m_uData.nInt32 != 0 );
	case T_INT64:
		return ( m_uData.nInt64 != 0LL );
	case T_BOOL:
		return ( m_uData.bBool );
	case T_FLOAT:
		return ( m_uData.vFloat != 0.0f );
	case T_DOUBLE:
		return ( m_uData.vDouble != 0.0 );
	case T_STRING:
		{
			int nValue;

			if( strcasecmp( "false", m_uData.pzString ) == 0 )
			{
				return ( false );
			}
			else if( strcasecmp( "true", m_uData.pzString ) == 0 )
			{
				return ( true );
			}
			else if( sscanf( m_uData.pzString, "%d", &nValue ) == 1 )
			{
				return ( nValue != 0 );
			}
			else
			{
				return ( false );
			}
		}
	default:
		return ( false );
	}
}
String Variant::AsString() const
{
	String cValue;

	switch ( m_eType )
	{
	case T_POINTER:
		cValue.Format( "%p", m_uData.pData );
		break;
	case T_INT8:
	case T_INT16:
	case T_INT32:
		cValue.Format( "%d", m_uData.nInt32 );
		break;
	case T_INT64:
		cValue.Format( "%Ld", m_uData.nInt64 );
		break;
	case T_BOOL:
		cValue.Format( "%s", ( m_uData.bBool ) ? "true" : "false" );
		break;
	case T_FLOAT:
		cValue.Format( "%f", m_uData.vFloat );
		break;
	case T_DOUBLE:
		cValue.Format( "%f", m_uData.vDouble );
		break;
	case T_STRING:
		cValue = m_uData.pzString;
		break;
	case T_RECT:
		{
			const Rect *r = ( Rect * ) m_uData.sBuffer.pBuffer;

			cValue.Format( "%f, %f, %f, %f", r->left, r->top, r->right, r->bottom );
		}
		break;
	case T_IRECT:
		{
			const IRect *r = ( IRect * ) m_uData.sBuffer.pBuffer;

			cValue.Format( "%d, %d, %d, %d", r->left, r->top, r->right, r->bottom );
		}
		break;
	case T_POINT:
		{
			const Point *p = ( Point * ) m_uData.sBuffer.pBuffer;
			cValue.Format( "X = %f, Y = %f", p->x, p->y );
		}
		break;
	case T_IPOINT:
		{
			const IPoint *p = ( IPoint * ) m_uData.sBuffer.pBuffer;
			cValue.Format( "X = %d, Y = %d", p->x, p->y );
		}
		break;
	case T_COLOR32:
		{
			const Color32_s *c = ( Color32_s * ) m_uData.sBuffer.pBuffer;

			cValue.Format( "R = %d, G = %d, B = %d, A = %d", c->red, c->green, c->blue, c->alpha );
		}
		break;
	default:
		break;
	}
	return ( cValue );
}

Point Variant::AsPoint() const
{
	switch ( m_eType )
	{
	case T_POINT:
		return ( *reinterpret_cast < Point * >( m_uData.sBuffer.pBuffer ) );
	case T_IPOINT:
		return ( static_cast < Point > ( *reinterpret_cast < IPoint * >( m_uData.sBuffer.pBuffer ) ) );
	case T_RECT:
		return ( reinterpret_cast < Rect * >( m_uData.sBuffer.pBuffer )->LeftTop() );
	case T_IRECT:
		return ( static_cast < Point > ( reinterpret_cast < IRect * >( m_uData.sBuffer.pBuffer )->LeftTop() ) );
	default:
		return ( Point( 0.0f, 0.0f ) );
	}
}
IPoint Variant::AsIPoint() const
{
	switch ( m_eType )
	{
	case T_POINT:
		return ( static_cast < IPoint > ( *reinterpret_cast < Point * >( m_uData.sBuffer.pBuffer ) ) );
	case T_IPOINT:
		return ( *reinterpret_cast < IPoint * >( m_uData.sBuffer.pBuffer ) );
	case T_RECT:
		return ( static_cast < IPoint > ( reinterpret_cast < Rect * >( m_uData.sBuffer.pBuffer )->LeftTop() ) );
	case T_IRECT:
		return ( reinterpret_cast < IRect * >( m_uData.sBuffer.pBuffer )->LeftTop() );
	default:
		return ( IPoint( 0, 0 ) );
	}
}
Rect Variant::AsRect() const
{
	switch ( m_eType )
	{
	case T_POINT:
		return ( Rect( *reinterpret_cast < Point * >( m_uData.sBuffer.pBuffer ), *reinterpret_cast < Point * >( m_uData.sBuffer.pBuffer ) ) );
	case T_IPOINT:
		return ( Rect( static_cast < Point > ( *reinterpret_cast < IPoint * >( m_uData.sBuffer.pBuffer ) ), static_cast < Point > ( *reinterpret_cast < IPoint * >( m_uData.sBuffer.pBuffer ) ) ) );
	case T_RECT:
		return ( *reinterpret_cast < Rect * >( m_uData.sBuffer.pBuffer ) );
	case T_IRECT:
		return ( static_cast < Rect > ( *reinterpret_cast < IRect * >( m_uData.sBuffer.pBuffer ) ) );
	default:
		return ( Rect( 0.0f, 0.0f, -1.0f, -1.0f ) );
	}
}

IRect Variant::AsIRect() const
{
	switch ( m_eType )
	{
	case T_POINT:
		return ( IRect( static_cast < IPoint > ( *reinterpret_cast < Point * >( m_uData.sBuffer.pBuffer ) ), static_cast < IPoint > ( *reinterpret_cast < Point * >( m_uData.sBuffer.pBuffer ) ) ) );
	case T_IPOINT:
		return ( IRect( *reinterpret_cast < IPoint * >( m_uData.sBuffer.pBuffer ), *reinterpret_cast < IPoint * >( m_uData.sBuffer.pBuffer ) ) );
	case T_RECT:
		return ( static_cast < IRect > ( *reinterpret_cast < Rect * >( m_uData.sBuffer.pBuffer ) ) );
	case T_IRECT:
		return ( *reinterpret_cast < IRect * >( m_uData.sBuffer.pBuffer ) );
	default:
		return ( IRect( 0, 0, -1, -1 ) );
	}
}

Color32 Variant::AsColor32() const
{
	switch ( m_eType )
	{
	case T_INT32:
		return ( Color32( m_uData.nInt32 ) );
	case T_INT64:
		return ( Color32( uint32( m_uData.nInt64 ) ) );
	case T_COLOR32:
		return ( *reinterpret_cast < Color32 * >( m_uData.sBuffer.pBuffer ) );
	default:
		return ( 0 );
	}
}

void *Variant::AsPointer() const
{
	if( m_eType == T_POINTER )
	{
		return ( m_uData.pData );
	}
	else
	{
		return ( NULL );
	}
}

void *Variant::AsRaw( size_t *pnSize )
{
	if( m_eType == T_RAW )
	{
		*pnSize = m_uData.sBuffer.nSize;
		return ( m_uData.sBuffer.pBuffer );
	}
	else
	{
		return ( NULL );
	}
}

/**************************************************************************/
/*** OPERATORS ************************************************************/
/**************************************************************************/

Variant & Variant::operator=( const Variant & cOther )
{
	_Clear();
	m_eType = cOther.m_eType;
	switch ( m_eType )
	{
	case T_ANY_TYPE:
		break;
	case T_STRING:
		m_uData.pzString = new char[strlen( cOther.m_uData.pzString ) + 1];

		strcpy( m_uData.pzString, cOther.m_uData.pzString );
		break;
	case T_MESSAGE:
	case T_VARIANT:
	case T_RAW:
	case T_POINT:
	case T_IPOINT:
	case T_RECT:
	case T_IRECT:
	case T_COLOR32:
		m_uData.sBuffer.nSize = cOther.m_uData.sBuffer.nSize;
		m_uData.sBuffer.pBuffer = new int8[m_uData.sBuffer.nSize];
		memcpy( m_uData.sBuffer.pBuffer, cOther.m_uData.sBuffer.pBuffer, m_uData.sBuffer.nSize );
		break;
	default:
		m_uData = cOther.m_uData;
		break;
	}
	return ( *this );
}

Variant & Variant::operator=( int8 nValue )
{
	SetInt8( nValue );
	return ( *this );
}

Variant & Variant::operator=( int16 nValue )
{
	SetInt16( nValue );
	return ( *this );
}

Variant & Variant::operator=( int32 nValue )
{
	SetInt32( nValue );
	return ( *this );
}

Variant & Variant::operator=( int64 nValue )
{
	SetInt64( nValue );
	return ( *this );
}

Variant & Variant::operator=( float vValue )
{
	SetFloat( vValue );
	return ( *this );
}

Variant & Variant::operator=( double vValue )
{
	SetDouble( vValue );
	return ( *this );
}

Variant & Variant::operator=( bool bValue )
{
	SetBool( bValue );
	return ( *this );
}

Variant & Variant::operator=( const String &cValue )
{
	SetString( cValue );
	return ( *this );
}

Variant & Variant::operator=( const Point & cValue )
{
	SetPoint( cValue );
	return ( *this );
}

Variant & Variant::operator=( const IPoint & cValue )
{
	SetIPoint( cValue );
	return ( *this );
}

Variant & Variant::operator=( const Rect & cValue )
{
	SetRect( cValue );
	return ( *this );
}

Variant & Variant::operator=( const IRect & cValue )
{
	SetIRect( cValue );
	return ( *this );
}

Variant & Variant::operator=( const Color32 & cValue )
{
	SetColor32( cValue );
	return ( *this );
}

Variant::operator  int ()
    const
    {
	    return ( AsInt32() );
    }
    Variant::operator  int64 () const
{
	return ( AsInt64() );
}
Variant::operator  float ()
    const
    {
	    return ( AsFloat() );
    }
    Variant::operator  double () const
{
	return ( AsDouble() );
}
Variant::operator  bool ()
    const
    {
	    return ( AsBool() );
    }
    Variant::operator  String () const
{
	return ( AsString() );
}

Variant::operator  Point() const
{
	return ( AsPoint() );
}

Variant::operator  IPoint() const
{
	return ( AsIPoint() );
}

Variant::operator  Rect() const
{
	return ( AsRect() );
}

Variant::operator  IRect() const
{
	return ( AsIRect() );
}

Variant::operator  Color32() const
{
	return ( AsColor32() );
}

bool Variant::operator==( const Variant & cOther ) const
{
	switch ( m_eType )
	{
	case T_ANY_TYPE:
		return ( cOther.m_eType == T_ANY_TYPE );
	case T_POINTER:
		return ( cOther.m_eType == T_POINTER && m_uData.pData == cOther.m_uData.pData );
	case T_INT8:
	case T_INT16:
	case T_INT32:
		return ( ( cOther.m_eType == T_INT8 && m_uData.nInt32 == cOther.m_uData.nInt32 ) || ( cOther.m_eType == T_INT16 && m_uData.nInt32 == cOther.m_uData.nInt32 ) || ( cOther.m_eType == T_INT32 && m_uData.nInt32 == cOther.m_uData.nInt32 ) || ( cOther.m_eType == T_INT64 && m_uData.nInt32 == cOther.m_uData.nInt64 ) || ( cOther.m_eType == T_BOOL && bool ( m_uData.nInt32 ) == cOther.m_uData.bBool )||( cOther.m_eType == T_FLOAT && float ( m_uData.nInt32 ) == cOther.m_uData.vFloat )||( cOther.m_eType == T_DOUBLE && double ( m_uData.nInt32 ) == cOther.m_uData.vDouble ) );
	case T_INT64:
		return ( ( cOther.m_eType == T_INT64 && m_uData.nInt64 == cOther.m_uData.nInt64 ) || ( cOther.m_eType == T_INT8 && m_uData.nInt64 == cOther.m_uData.nInt32 ) || ( cOther.m_eType == T_INT16 && m_uData.nInt64 == cOther.m_uData.nInt32 ) || ( cOther.m_eType == T_INT32 && m_uData.nInt64 == cOther.m_uData.nInt32 ) || ( cOther.m_eType == T_BOOL && bool ( m_uData.nInt64 ) == cOther.m_uData.bBool )||( cOther.m_eType == T_FLOAT && float ( m_uData.nInt64 ) == cOther.m_uData.vFloat )||( cOther.m_eType == T_DOUBLE && double ( m_uData.nInt64 ) == cOther.m_uData.vDouble ) );
	case T_BOOL:
		return ( ( cOther.m_eType == T_BOOL && m_uData.bBool == cOther.m_uData.bBool ) || ( cOther.m_eType == T_INT8 && m_uData.bBool == cOther.m_uData.nInt32 ) || ( cOther.m_eType == T_INT16 && m_uData.bBool == cOther.m_uData.nInt32 ) || ( cOther.m_eType == T_INT32 && m_uData.bBool == cOther.m_uData.nInt32 ) || ( cOther.m_eType == T_INT64 && m_uData.bBool == cOther.m_uData.nInt64 ) || ( cOther.m_eType == T_FLOAT && m_uData.bBool == ( cOther.m_uData.vFloat != 0.0f ) ) || ( cOther.m_eType == T_DOUBLE && m_uData.bBool == ( cOther.m_uData.vDouble != 0.0 ) ) );
	case T_FLOAT:
		return ( ( cOther.m_eType == T_FLOAT && m_uData.vFloat == cOther.m_uData.vFloat ) || ( cOther.m_eType == T_DOUBLE && double ( m_uData.vFloat ) == cOther.m_uData.vDouble )||( cOther.m_eType == T_INT8 && m_uData.vFloat == float ( cOther.m_uData.nInt32 ) )||( cOther.m_eType == T_INT16 && m_uData.vFloat == float ( cOther.m_uData.nInt32 ) )||( cOther.m_eType == T_INT32 && m_uData.vFloat == float ( cOther.m_uData.nInt32 ) )||( cOther.m_eType == T_INT64 && m_uData.vFloat == float ( cOther.m_uData.nInt64 ) )||( cOther.m_eType == T_BOOL && ( m_uData.vFloat != 0.0f ) == cOther.m_uData.bBool ) );
	case T_DOUBLE:
		return ( ( cOther.m_eType == T_DOUBLE && m_uData.vDouble == cOther.m_uData.vDouble ) || ( cOther.m_eType == T_FLOAT && m_uData.vDouble == double ( cOther.m_uData.vFloat ) )||( cOther.m_eType == T_INT8 && m_uData.vDouble == double ( cOther.m_uData.nInt32 ) )||( cOther.m_eType == T_INT16 && m_uData.vDouble == double ( cOther.m_uData.nInt32 ) )||( cOther.m_eType == T_INT32 && m_uData.vDouble == double ( cOther.m_uData.nInt32 ) )||( cOther.m_eType == T_INT64 && m_uData.vDouble == double ( cOther.m_uData.nInt64 ) )||( cOther.m_eType == T_BOOL && ( m_uData.vDouble != 0.0 ) == cOther.m_uData.bBool ) );
	case T_STRING:
		return ( cOther.m_eType == T_STRING && strcmp( m_uData.pzString, cOther.m_uData.pzString ) == 0 );
	case T_IRECT:
		return ( ( cOther.m_eType == T_IRECT || cOther.m_eType == T_RECT ) && AsIRect() == cOther.AsIRect(  ) );
	case T_IPOINT:
		return ( ( cOther.m_eType == T_IPOINT || cOther.m_eType == T_POINT ) && AsIPoint() == cOther.AsIPoint(  ) );
	case T_MESSAGE:
		return ( false );
	case T_COLOR32:
		return ( ( cOther.m_eType == T_COLOR32 ) && (int32)(AsColor32()) == (int32)(cOther.AsColor32()) );
	case T_RECT:
		return ( ( cOther.m_eType == T_RECT || cOther.m_eType == T_IRECT ) && AsRect() == cOther.AsRect(  ) );
	case T_POINT:
		return ( ( cOther.m_eType == T_POINT || cOther.m_eType == T_IPOINT ) && AsPoint() == cOther.AsPoint(  ) );
	case T_RAW:
		return ( cOther.m_eType == T_RAW && cOther.m_uData.sBuffer.nSize == m_uData.sBuffer.nSize && memcmp( cOther.m_uData.sBuffer.pBuffer, m_uData.sBuffer.pBuffer, m_uData.sBuffer.nSize ) == 0 );
	default:
		return ( false );
	}
}

bool Variant::operator!=( const Variant & cValue ) const
{
	return ( !( *this == cValue ) );
}

/**************************************************************************/
/*** FLATTEN/UNFLATTEN METHODS ********************************************/
/**************************************************************************/

size_t Variant::GetFlattenedSize() const
{
	switch ( m_eType )
	{
	case T_ANY_TYPE:
		return ( sizeof( int ) );
	case T_POINTER:
		return ( sizeof( int ) + sizeof( m_uData.pData ) );
	case T_INT8:
	case T_INT16:
	case T_INT32:
		return ( sizeof( int ) + sizeof( m_uData.nInt32 ) );
	case T_INT64:
		return ( sizeof( int ) + sizeof( m_uData.nInt64 ) );
	case T_BOOL:
		return ( sizeof( int ) + sizeof( m_uData.bBool ) );
	case T_FLOAT:
		return ( sizeof( int ) + sizeof( m_uData.vFloat ) );
	case T_DOUBLE:
		return ( sizeof( int ) + sizeof( m_uData.vDouble ) );
	case T_STRING:
		return ( sizeof( int ) * 2 + strlen( m_uData.pzString ) + 1 );
	case T_IRECT:
		return ( sizeof( int ) + sizeof( IRect ) );
	case T_IPOINT:
		return ( sizeof( int ) + sizeof( IPoint ) );
	case T_MESSAGE:
		return ( sizeof( int ) + sizeof( int ) + m_uData.sBuffer.nSize );
	case T_COLOR32:
		return ( sizeof( int ) + sizeof( Color32_s ) );
	case T_RECT:
		return ( sizeof( int ) + sizeof( Rect ) );
	case T_POINT:
		return ( sizeof( int ) + sizeof( Point ) );
	case T_VARIANT:
		return ( sizeof( int ) + sizeof( int ) + m_uData.sBuffer.nSize );
	case T_RAW:
		return ( sizeof( int ) + sizeof( int ) + m_uData.sBuffer.nSize );
	default:
		return ( sizeof( int ) );
	}
	return ( sizeof( int ) );
}

status_t Variant::Flatten( void *pBuffer, size_t nBufSize ) const
{
	uint nLen = GetFlattenedSize();

	if( nBufSize < nLen )
	{
		errno = EINVAL;
		return ( -1 );
	}

	*( ( int * )pBuffer ) = m_eType;
	pBuffer = ( ( int8 * )pBuffer ) + sizeof( int );
	nLen -= sizeof( int );

	switch ( m_eType )
	{
	case T_STRING:
		{
			int nLen = strlen( m_uData.pzString ) + 1;

			*( ( int * )pBuffer ) = nLen;
			pBuffer = ( ( int8 * )pBuffer ) + sizeof( int );
			memcpy( pBuffer, m_uData.pzString, nLen );
			break;
		}
	case T_RECT:
	case T_IRECT:
	case T_POINT:
	case T_IPOINT:
	case T_MESSAGE:
	case T_VARIANT:
	case T_COLOR32:
	case T_RAW:
		memcpy( pBuffer, m_uData.sBuffer.pBuffer, m_uData.sBuffer.nSize );
		break;
	default:
		memcpy( pBuffer, &m_uData, nLen );
	}
	return ( sizeof( int ) );

	return ( 0 );
}

status_t Variant::Unflatten( const void *pBuffer, size_t nBufSize )
{
	switch ( m_eType )
	{
	case T_STRING:
		delete[]m_uData.pzString;
		break;
	case T_MESSAGE:
	case T_VARIANT:
	case T_RAW:
		delete[]reinterpret_cast < int8 *>( m_uData.sBuffer.pBuffer );

		break;
	default:
		break;
	}
	m_eType = T_ANY_TYPE;

	if( nBufSize < 4 )
	{
		errno = EINVAL;
		return ( -1 );
	}
	m_eType = ( type_code ) ( *( ( int * )pBuffer ) );
	pBuffer = ( ( int8 * )pBuffer ) + sizeof( int );
	nBufSize -= sizeof( int );

	int nError = 0;

	switch ( m_eType )
	{
	case T_ANY_TYPE:
		break;
	case T_POINTER:
		if( nBufSize < sizeof( m_uData.pData ) )
		{
			errno = EINVAL;
			nError = -1;
			break;
		}
		memcpy( &m_uData, pBuffer, sizeof( m_uData.pData ) );
		break;
	case T_INT8:
	case T_INT16:
	case T_INT32:
		if( nBufSize < sizeof( m_uData.nInt32 ) )
		{
			errno = EINVAL;
			nError = -1;
			break;
		}
		memcpy( &m_uData, pBuffer, sizeof( m_uData.nInt32 ) );
		break;
	case T_INT64:
		if( nBufSize < sizeof( m_uData.nInt64 ) )
		{
			errno = EINVAL;
			nError = -1;
			break;
		}
		memcpy( &m_uData, pBuffer, sizeof( m_uData.nInt64 ) );
		break;
	case T_BOOL:
		if( nBufSize < sizeof( m_uData.bBool ) )
		{
			errno = EINVAL;
			nError = -1;
			break;
		}
		memcpy( &m_uData, pBuffer, sizeof( m_uData.bBool ) );
		break;
	case T_FLOAT:
		if( nBufSize < sizeof( m_uData.vFloat ) )
		{
			errno = EINVAL;
			nError = -1;
			break;
		}
		memcpy( &m_uData, pBuffer, sizeof( m_uData.vFloat ) );
		break;
	case T_DOUBLE:
		if( nBufSize < sizeof( m_uData.vDouble ) )
		{
			errno = EINVAL;
			nError = -1;
			break;
		}
		memcpy( &m_uData, pBuffer, sizeof( m_uData.vDouble ) );
		break;
	case T_STRING:
		{
			if( nBufSize < sizeof( int ) )
			{
				errno = EINVAL;
				nError = -1;
				break;
			}
			int nSize = ( *( ( int * )pBuffer ) );
			pBuffer = ( ( int8 * )pBuffer ) + sizeof( int );
			if( nBufSize < nSize + sizeof( int ) )
			{
				errno = EINVAL;
				nError = -1;
				break;
			}
			m_uData.pzString = new char[nSize];

			memcpy( m_uData.pzString, pBuffer, nSize );
			break;
		}
	case T_IRECT:
		if( nBufSize < sizeof( IRect ) )
		{
			errno = EINVAL;
			nError = -1;
			break;
		}
		m_uData.sBuffer.nSize = sizeof( IRect );
		m_uData.sBuffer.pBuffer = new int8[m_uData.sBuffer.nSize];

		memcpy( m_uData.sBuffer.pBuffer, pBuffer, m_uData.sBuffer.nSize );
		break;
	case T_IPOINT:
		if( nBufSize < sizeof( IPoint ) )
		{
			errno = EINVAL;
			nError = -1;
			break;
		}
		m_uData.sBuffer.nSize = sizeof( IPoint );
		m_uData.sBuffer.pBuffer = new int8[m_uData.sBuffer.nSize];

		memcpy( m_uData.sBuffer.pBuffer, pBuffer, m_uData.sBuffer.nSize );
		break;
	case T_MESSAGE:
		return ( sizeof( int ) + sizeof( int ) + m_uData.sBuffer.nSize );
	case T_COLOR32:
		if( nBufSize < sizeof( Color32_s ) )
		{
			errno = EINVAL;
			nError = -1;
			break;
		}
		m_uData.sBuffer.nSize = sizeof( Color32_s );
		m_uData.sBuffer.pBuffer = new int8[m_uData.sBuffer.nSize];

		memcpy( m_uData.sBuffer.pBuffer, pBuffer, m_uData.sBuffer.nSize );
		break;
	case T_RECT:
		if( nBufSize < sizeof( Rect ) )
		{
			errno = EINVAL;
			nError = -1;
			break;
		}
		m_uData.sBuffer.nSize = sizeof( Rect );
		m_uData.sBuffer.pBuffer = new int8[m_uData.sBuffer.nSize];

		memcpy( m_uData.sBuffer.pBuffer, pBuffer, m_uData.sBuffer.nSize );
		break;
	case T_POINT:
		if( nBufSize < sizeof( Point ) )
		{
			errno = EINVAL;
			nError = -1;
			break;
		}
		m_uData.sBuffer.nSize = sizeof( Point );
		m_uData.sBuffer.pBuffer = new int8[m_uData.sBuffer.nSize];

		memcpy( m_uData.sBuffer.pBuffer, pBuffer, m_uData.sBuffer.nSize );
		break;;
	case T_VARIANT:
		errno = EINVAL;
		nError = -1;
		break;
	case T_RAW:
		if( nBufSize < sizeof( int ) )
		{
			errno = EINVAL;
			nError = -1;
			break;
		}
		m_uData.sBuffer.nSize = ( *( ( int * )pBuffer ) );
		pBuffer = ( ( int8 * )pBuffer ) + sizeof( int );
		if( nBufSize < sizeof( int ) + m_uData.sBuffer.nSize )
		{
			errno = EINVAL;
			nError = -1;
			break;
		}
		m_uData.sBuffer.pBuffer = new int8[m_uData.sBuffer.nSize];

		memcpy( m_uData.sBuffer.pBuffer, pBuffer, m_uData.sBuffer.nSize );
		break;
	default:
		nError = -1;
		errno = EINVAL;
		break;
	}
	if( nError < 0 )
	{
		m_eType = T_ANY_TYPE;
		return ( -1 );
	}
	else
	{
		return ( 0 );
	}
}


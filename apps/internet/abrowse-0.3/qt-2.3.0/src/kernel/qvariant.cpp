/****************************************************************************
** $Id$
**
** Implementation of QVariant class
**
** Created : 990414
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include "qvariant.h"

#ifndef QT_NO_PROPERTIES

#include "qstring.h"
#include "qfont.h"
#include "qpixmap.h"
#include "qimage.h"
#include "qbrush.h"
#include "qpoint.h"
#include "qrect.h"
#include "qsize.h"
#include "qcolor.h"
#include "qpalette.h"
#include "qiconset.h"
#include "qdatastream.h"
#include "qregion.h"
#include "qpointarray.h"
#include "qbitmap.h"
#include "qcursor.h"
#include "qsizepolicy.h"
#include "qshared.h"

// Uncomment to test for memory leaks or to run qt/test/qvariant/main.cpp
// #define QVARIANT_DEBUG

#ifdef QVARIANT_DEBUG
int qv_count = 0;
int get_qv_count() { return qv_count; }
#endif

QVariantPrivate::QVariantPrivate()
{
#ifdef QVARIANT_DEBUG
    qv_count++;
#endif
    typ = QVariant::Invalid;
}

QVariantPrivate::QVariantPrivate( QVariantPrivate* d )
{
#ifdef QVARIANT_DEBUG
    qv_count++;
#endif

    switch( d->typ )
	{
	case QVariant::Invalid:
	    break;
	case QVariant::Bitmap:
	    value.ptr = new QBitmap( *((QBitmap*)d->value.ptr) );
	    break;
	case QVariant::Region:
	    value.ptr = new QRegion( *((QRegion*)d->value.ptr) );
	    // ## Force a detach
	    // ((QRegion*)value.ptr)->translate( 0, 0 );
	    break;
	case QVariant::PointArray:
	    // QPointArray is explicit shared
	    value.ptr = new QPointArray( *((QPointArray*)d->value.ptr) );
	    break;
	case QVariant::String:
	    value.ptr = new QString( *((QString*)d->value.ptr) );
	    break;
	case QVariant::CString:
	    // QCString is explicit shared
	    value.ptr = new QCString( *((QCString*)d->value.ptr) );
	    break;
	case QVariant::StringList:
	    value.ptr = new QStringList( *((QStringList*)d->value.ptr) );
	    break;
	case QVariant::Map:
	    value.ptr = new QMap<QString,QVariant>( *((QMap<QString,QVariant>*)d->value.ptr) );
	    break;
	case QVariant::Font:
	    value.ptr = new QFont( *((QFont*)d->value.ptr) );
	    break;
	case QVariant::Pixmap:
	    value.ptr = new QPixmap( *((QPixmap*)d->value.ptr) );
	    break;
	case QVariant::Image:
	    // QImage is explicit shared
	    value.ptr = new QImage( *((QImage*)d->value.ptr) );
	    break;
	case QVariant::Brush:
	    value.ptr = new QBrush( *((QBrush*)d->value.ptr) );
	    // ## Force a detach
	    // ((QBrush*)value.ptr)->setColor( ((QBrush*)value.ptr)->color() );
	    break;
	case QVariant::Point:
	    value.ptr = new QPoint( *((QPoint*)d->value.ptr) );
	    break;
	case QVariant::Rect:
	    value.ptr = new QRect( *((QRect*)d->value.ptr) );
	    break;
	case QVariant::Size:
	    value.ptr = new QSize( *((QSize*)d->value.ptr) );
	    break;
	case QVariant::Color:
	    value.ptr = new QColor( *((QColor*)d->value.ptr) );
	    break;
	case QVariant::Palette:
	    value.ptr = new QPalette( *((QPalette*)d->value.ptr) );
	    break;
	case QVariant::ColorGroup:
	    value.ptr = new QColorGroup( *((QColorGroup*)d->value.ptr) );
	    break;
	case QVariant::IconSet:
	    value.ptr = new QIconSet( *((QIconSet*)d->value.ptr) );
	    break;
	case QVariant::List:
	    value.ptr = new QValueList<QVariant>( *((QValueList<QVariant>*)d->value.ptr) );
	    break;
	case QVariant::Int:
	    value.i = d->value.i;
	    break;
	case QVariant::UInt:
	    value.u = d->value.u;
	    break;
	case QVariant::Bool:
	    value.b = d->value.b;
	    break;
	case QVariant::Double:
	    value.d = d->value.d;
	    break;
	default:
	    ASSERT( 0 );
	}

    typ = d->typ;
}

QVariantPrivate::~QVariantPrivate()
{
#ifdef QVARIANT_DEBUG
    qv_count--;
#endif
    clear();
}

void QVariantPrivate::clear()
{
    switch( typ )
	{
	case QVariant::Bitmap:
	    delete (QBitmap*)value.ptr;
	    break;
	case QVariant::Cursor:
	    delete (QCursor*)value.ptr;
	    break;
	case QVariant::Region:
	    delete (QRegion*)value.ptr;
	    break;
	case QVariant::PointArray:
	    delete (QPointArray*)value.ptr;
	    break;
	case QVariant::String:
	    delete (QString*)value.ptr;
	    break;
	case QVariant::CString:
	    delete (QCString*)value.ptr;
	    break;
	case QVariant::Map:
	    delete (QMap<QString,QVariant>*)value.ptr;
	    break;
	case QVariant::StringList:
	    delete (QStringList*)value.ptr;
	    break;
	case QVariant::Font:
	    delete (QFont*)value.ptr;
	    break;
	case QVariant::Pixmap:
	    delete (QPixmap*)value.ptr;
	    break;
	case QVariant::Image:
	    delete (QImage*)value.ptr;
	    break;
	case QVariant::Brush:
	    delete (QBrush*)value.ptr;
	    break;
	case QVariant::Point:
	    delete (QPoint*)value.ptr;
	    break;
	case QVariant::Rect:
	    delete (QRect*)value.ptr;
	    break;
	case QVariant::Size:
	    delete (QSize*)value.ptr;
	    break;
	case QVariant::Color:
	    delete (QColor*)value.ptr;
	    break;
	case QVariant::Palette:
	    delete (QPalette*)value.ptr;
	    break;
	case QVariant::ColorGroup:
	    delete (QColorGroup*)value.ptr;
	    break;
	case QVariant::IconSet:
	    delete (QIconSet*)value.ptr;
	    break;
	case QVariant::List:
	    delete (QValueList<QVariant>*)value.ptr;
	    break;
	case QVariant::SizePolicy:
	    delete (QSizePolicy*)value.ptr;
	    break;
	case QVariant::Invalid:
	case QVariant::Int:
	case QVariant::UInt:
	case QVariant::Bool:
	case QVariant::Double:
	    break;
	}

    typ = QVariant::Invalid;
}

// REVISED: arnt
/*!
  \class QVariant qvariant.h
  \brief Acts like a union for the most common Qt data types.

  \ingroup objectmodel
  \ingroup misc

  Since C++ forbids unions from including types that have non-default
  constructors or destructors, most interesting Qt classes cannot be
  used in unions.  This is a problem when using QObject::property(),
  among other things.

  This class provides union functionality for property() and most
  other needs that might be solved by a union including e.g. QWidget.

  A QVariant object can hold any one type() at a time, and you can
  find out what type it holds, convert it to a different type using
  e.g. asSize(), get its value using e.g. toSize(), and check whether
  the type can be converted to e.g. QSize using canCast().

  The methods named toT() (for any supported T, see the Type
  documentation for a list) are const. If you ask for the stored type,
  they return a copy of the stored object.  If you ask for a type
  which can be generated from the stored type, toT() copies and
  converts, and leaves the object itself unchanged.  If you ask for a
  type that cannot be generated from the stored type, the result
  depends on the type, see the function documentation for details.

  Note that three data types supported by QVariant are explicitly
  shared, namely QImage, QPointArray, and QCString, and in these cases
  the toT() methods return a shallow copy.  In almost all cases, you
  must make a deep copy of the returned values before modifying them.

  The methods named asT() are not const. They do conversion like toT()
  methods, set the variant to hold the converted value, and return a
  reference to the new contents of the variant.

  Here is some example code to demonstrate use of QVariant:

  \code
    QDataStream out(...);
    QVariant v(123);          // The variant now contains an int
    int x = v.toInt();        // x = 123
    out << v;                 // Writes a type tag and an int to out
    v = QVariant("hello");    // The variant now contains a QCString
    v = QVariant(tr("hello"));// The variant now contains a QString
    int y = v.toInt();        // y = 0 since v cannot be converted to an int
    QString s = v.toString(); // s = tr("hello")  (see QObject::tr())
    out << v;                 // Writes a type tag and a QString to out
    ...
    QDataStream in(...);      // (opening the previously written stream)
    in >> v;                  // Reads an Int variant
    int z = v.toInt();        // z = 123
    qDebug("Type is %s",      // prints "Type is int"
      v.typeName());
    v.asInt() += 100;	      // The variant now hold the value 223.
    v = QVariant( QStringList() );
    v.asStringList().append( "Hallo" );
  \endcode

  You can even have a QValueList<QVariant> stored in the variant -
  giving arbitrarily complex data values with lists of variants, some
  of which are strings while others are integers and other still are
  lists of lists of lists of variants.  This is very powerful, and you
  can easily shoot yourself in the foot with all this power.  Caveat
  programmor.
*/

/*! \enum QVariant::Type

  This enum type defines the types of variable that a QVariant can
  contain.  The supported enum values and the associated types are: <ul>

  <li> \c Invalid - no type
  <li> \c List - a QValueList<QVariant>
  <li> \c Map - a QMap<QString,QVariant>
  <li> \c String - a QString
  <li> \c StringList - a QStringList
  <li> \c Font - a QFont
  <li> \c Pixmap - a QPixmap
  <li> \c Brush - a QBrush
  <li> \c Rect - a QRect
  <li> \c Size - a QSize
  <li> \c Color - a QColor
  <li> \c Palette - a QPalette
  <li> \c ColorGroup - a QColorGroup
  <li> \c IconSet - a QIconSet
  <li> \c Point - a QPoint
  <li> \c Image - a QImage
  <li> \c Int - an int
  <li> \c UInt - an unsigned int
  <li> \c Bool - a bool
  <li> \c Double - a doublea
  <li> \c CString - a QCString
  <li> \c PointArray - a QPointArray
  <li> \c Region - a QRegion
  <li> \c Bitmap - a QBitmap
  <li> \c Cursor - a QCursor
  <li> \c SizePolicy - a QSizePolicy

  </ul>

  Note that Qt's definition of bool depends on the compiler.
  qglobal.h has the system-dependent definition of bool.
*/

/*!
  Constructs an invalid variant.
*/
QVariant::QVariant()
{
    d = new QVariantPrivate;
}

/*!
  Destructs the QVariant and the contained object.

  Note that subclasses that re-implement clear() should reimplement
  the destructor to call clear().  This destructor calls clear(), but
  since it is the destructor, QVariant::clear() is called rather than
  any subclass.
*/
QVariant::~QVariant()
{
    if ( d->deref() )
	delete d;
}

/*!
  Constructs a copy of the variant passed as argument to this
  constructor. Usually this is a deep copy, but if the stored data
  type is explicit shared then a shallow copy is made.
*/
QVariant::QVariant( const QVariant& p )
{
    d = new QVariantPrivate;
    *this = p;
}

/*!
  Reads the variant from the data stream.
*/
QVariant::QVariant( QDataStream& s )
{
    d = new QVariantPrivate;
    s >> *this;
}

/*!
  Constructs a new variant with a string value.
*/
QVariant::QVariant( const QString& val )
{
    d = new QVariantPrivate;
    d->typ = String;
    d->value.ptr = new QString( val );
}

/*!
  Constructs a new variant with a c-string value.

  If you want to modify the QCString you pass to this constructor
  after this call, we recommend passing a deep copy (see
  QCString::copy()).
*/
QVariant::QVariant( const QCString& val )
{
    d = new QVariantPrivate;
    d->typ = CString;
    d->value.ptr = new QCString( val );
}

/*!
  Constructs a new variant with a c-string value, if \a val is
  non-null.  The variant creates a deep copy of \a val.

  If \a val is null, the resulting variant has type Invalid.
*/
QVariant::QVariant( const char* val )
{
    d = new QVariantPrivate;
    if ( val == 0 )
	return;
    d->typ = CString;
    d->value.ptr = new QCString( val );
}

/*!
  Constructs a new variant with a string list value.
*/
QVariant::QVariant( const QStringList& val )
{
    d = new QVariantPrivate;
    d->typ = StringList;
    d->value.ptr = new QStringList( val );
}

/*!
  Constructs a new variant with a map of QVariants.
*/
QVariant::QVariant( const QMap<QString,QVariant>& val )
{
    d = new QVariantPrivate;
    d->typ = Map;
    d->value.ptr = new QMap<QString,QVariant>( val );
}

/*!
  Constructs a new variant with a font value.
*/
QVariant::QVariant( const QFont& val )
{
    d = new QVariantPrivate;
    d->typ = Font;
    d->value.ptr = new QFont( val );
}

/*!
  Constructs a new variant with a pixmap value.
*/
QVariant::QVariant( const QPixmap& val )
{
    d = new QVariantPrivate;
    d->typ = Pixmap;
    d->value.ptr = new QPixmap( val );
}


/*!
  Constructs a new variant with an image value.

  Since QImage is explicitly shared you may need to pass a deep copy
  to the variant using QImage::copy().
*/
QVariant::QVariant( const QImage& val )
{
    d = new QVariantPrivate;
    d->typ = Image;
    d->value.ptr = new QImage( val );
}

/*!
  Constructs a new variant with a brush value.
*/
QVariant::QVariant( const QBrush& val )
{
    d = new QVariantPrivate;
    d->typ = Brush;
    d->value.ptr = new QBrush( val );
}

/*!
  Constructs a new variant with a point value.
*/
QVariant::QVariant( const QPoint& val )
{
    d = new QVariantPrivate;
    d->typ = Point;
    d->value.ptr = new QPoint( val );
}

/*!
  Constructs a new variant with a rect value.
*/
QVariant::QVariant( const QRect& val )
{
    d = new QVariantPrivate;
    d->typ = Rect;
    d->value.ptr = new QRect( val );
}

/*!
  Constructs a new variant with a size value.
*/
QVariant::QVariant( const QSize& val )
{
    d = new QVariantPrivate;
    d->typ = Size;
    d->value.ptr = new QSize( val );
}

/*!
  Constructs a new variant with a color value.
*/
QVariant::QVariant( const QColor& val )
{
    d = new QVariantPrivate;
    d->typ = Color;
    d->value.ptr = new QColor( val );
}

/*!
  Constructs a new variant with a color palette value.
*/
QVariant::QVariant( const QPalette& val )
{
    d = new QVariantPrivate;
    d->typ = Palette;
    d->value.ptr = new QPalette( val );
}

/*!
  Constructs a new variant with a color group value.
*/
QVariant::QVariant( const QColorGroup& val )
{
    d = new QVariantPrivate;
    d->typ = ColorGroup;
    d->value.ptr = new QColorGroup( val );
}

/*!
  Constructs a new variant with an icon set value.
*/
QVariant::QVariant( const QIconSet& val )
{
    d = new QVariantPrivate;
    d->typ = IconSet;
    d->value.ptr = new QIconSet( val );
}

/*!
  Constructs a new variant with a region.
*/
QVariant::QVariant( const QRegion& val )
{
    d = new QVariantPrivate;
    d->typ = Region;
    // ## Force a detach
    d->value.ptr = new QRegion( val );
    ((QRegion*)d->value.ptr)->translate( 0, 0 );
}

/*!
  Constructs a new variant with a bitmap value.
*/
QVariant::QVariant( const QBitmap& val )
{
    d = new QVariantPrivate;
    d->typ = Bitmap;
    d->value.ptr = new QBitmap( val );
}

/*!
  Constructs a new variant with a cursor value.
*/
QVariant::QVariant( const QCursor& val )
{
    d = new QVariantPrivate;
    d->typ = Cursor;
    d->value.ptr = new QCursor( val );
}

/*!
  Constructs a new variant with an point array value.

  Since QPointArray is explicitly shared you may need to pass a deep copy
  to the variant using QPointArray::copy().
*/
QVariant::QVariant( const QPointArray& val )
{
    d = new QVariantPrivate;
    d->typ = PointArray;
    d->value.ptr = new QPointArray( val );
}

/*!
  Constructs a new variant with an integer value.
*/
QVariant::QVariant( int val )
{
    d = new QVariantPrivate;
    d->typ = Int;
    d->value.i = val;
}

/*!
  Constructs a new variant with an unsigned integer value.
*/
QVariant::QVariant( uint val )
{
    d = new QVariantPrivate;
    d->typ = UInt;
    d->value.u = val;
}

/*!
  Constructs a new variant with a boolean value. The integer argument
  is a dummy, necessary for compatibility with certain compiler that
  even its mother cannot love.
*/
QVariant::QVariant( bool val, int )
{
    d = new QVariantPrivate;
    d->typ = Bool;
    d->value.b = val;
}


/*!
  Constructs a new variant with a floating point value.
*/
QVariant::QVariant( double val )
{
    d = new QVariantPrivate;
    d->typ = Double;
    d->value.d = val;
}

/*!
  Constructs a new variant with a list value.
*/
QVariant::QVariant( const QValueList<QVariant>& val )
{
    d = new QVariantPrivate;
    d->typ = List;
    d->value.ptr = new QValueList<QVariant>( val );
}

/*!
  Constructs a new variant with a size policy value.
*/
QVariant::QVariant( QSizePolicy val )
{
    d = new QVariantPrivate;
    d->typ = SizePolicy;
    d->value.ptr = new QSizePolicy( val );
}

/*!
  Assigns the value of some \a other variant to this variant.

  This is a deep copy of the variant, but note that if the variant
  holds an explicitly shared type such as QImage, it is a shallow copy
  of the (e.g.) QImage.
*/
QVariant& QVariant::operator= ( const QVariant& variant )
{
    QVariant& other = (QVariant&)variant;

    other.d->ref();
    if ( d->deref() )
	delete d;

    d = other.d;

    return *this;
}

/*!
  \internal
*/
void QVariant::detach()
{
    if ( d->count == 1 )
	return;

    d->deref();
    d = new QVariantPrivate( d );
}

/*!
  Returns the name of the type stored in the variant.
  The returned strings describe the C++ datatype used to store the
  data, for example "QFont", "QString" or "QValueList<QVariant>".
  An Invalid variant returns 0.
*/
const char* QVariant::typeName() const
{
    return typeToName( d->typ );
}

/*! Convert this variant to type Invalid and free up any resources
  used.
*/
void QVariant::clear()
{
    if ( d->count > 1 )
    {
	d->deref();
	d = new QVariantPrivate;
	return;
    }

    d->clear();
}

/* Attention!

   For dependency reasons, this table is duplicated in moc.y. If you
   change one, change both.

   (Search for the word 'Attention' in moc.y.)
*/
static const int ntypes = 26;
static const char* const type_map[ntypes] =
{
    0,
    "QMap<QString,QVariant>",
    "QValueList<QVariant>",
    "QString",
    "QStringList",
    "QFont",
    "QPixmap",
    "QBrush",
    "QRect",
    "QSize",
    "QColor",
    "QPalette",
    "QColorGroup",
    "QIconSet",
    "QPoint",
    "QImage",
    "int",
    "uint",
    "bool",
    "double",
    "QCString",
    "QPointArray",
    "QRegion",
    "QBitmap",
    "QCursor",
    "QSizePolicy"
};


/*!
  Converts the enum representation of the storage type to its
  string representation.
*/
const char* QVariant::typeToName( Type typ )
{
    if ( typ >= ntypes )
	return 0;
    return type_map[typ];
}


/*!
  Converts the string representation of the storage type to
  its enum representation.

  If the string representation cannot be converted to any enum
  representation, the variant is set to \c Invalid.
*/
QVariant::Type QVariant::nameToType( const char* name )
{
    for ( int i = 0; i < ntypes; i++ ) {
	if ( !qstrcmp( type_map[i], name ) )
	    return (Type) i;
    }
    return Invalid;
}

/*! Internal function for loading a variant. Use the stream operators
  instead.

  \internal
*/
void QVariant::load( QDataStream& s )
{
    Q_UINT32 u;
    s >> u;
    Type t = (Type)u;

    switch( t ) {
    case Invalid:
	d->typ = t;
	break;
    case Map:
	{
	    QMap<QString,QVariant>* x = new QMap<QString,QVariant>;
	    s >> *x;
	    d->value.ptr = x;
	}
	break;
    case List:
	{
	    QValueList<QVariant>* x = new QValueList<QVariant>;
	    s >> *x;
	    d->value.ptr = x;
	}
	break;
    case Cursor:
	{
#ifndef QT_NO_CURSOR
	    QCursor* x = new QCursor;
	    s >> *x;
	    d->value.ptr = x;
#endif
	}
	break;
    case Bitmap:
	{
	    QBitmap* x = new QBitmap;
	    s >> *x;
	    d->value.ptr = x;
	}
	break;
    case Region:
	{
	    QRegion* x = new QRegion;
	    s >> *x;
	    d->value.ptr = x;
	}
	break;
    case PointArray:
	{
	    QPointArray* x = new QPointArray;
	    s >> *x;
	    d->value.ptr = x;
	}
	break;
    case String:
	{
	    QString* x = new QString;
	    s >> *x;
	    d->value.ptr = x;
	}
	break;
    case CString:
	{
	    QCString* x = new QCString;
	    s >> *x;
	    d->value.ptr = x;
	}
	break;
    case StringList:
	{
	    QStringList* x = new QStringList;
	    s >> *x;
	    d->value.ptr = x;
	}
	break;
    case Font:
	{
	    QFont* x = new QFont;
	    s >> *x;
	    d->value.ptr = x;
	}
	break;
    case Pixmap:
	{
	    QPixmap* x = new QPixmap;
	    s >> *x;
	    d->value.ptr = x;
	}
	break;
    case Image:
	{
	    QImage* x = new QImage;
	    s >> *x;
	    d->value.ptr = x;
	}
	break;
    case Brush:
	{
	    QBrush* x = new QBrush;
	    s >> *x;
	    d->value.ptr = x;
	}
	break;
    case Rect:
	{
	    QRect* x = new QRect;
	    s >> *x;
	    d->value.ptr = x;
	}
	break;
    case Point:
	{
	    QPoint* x = new QPoint;
	    s >> *x;
	    d->value.ptr = x;
	}
	break;
    case Size:
	{
	    QSize* x = new QSize;
	    s >> *x;
	    d->value.ptr = x;
	}
	break;
    case Color:
	{
	    QColor* x = new QColor;
	    s >> *x;
	    d->value.ptr = x;
	}
	break;
    case Palette:
	{
	    QPalette* x = new QPalette;
	    s >> *x;
	    d->value.ptr = x;
	}
	break;
    case ColorGroup:
	{
	    QColorGroup* x = new QColorGroup;
	    s >> *x;
	    d->value.ptr = x;
	}
	break;
    case IconSet:
	{
	    QPixmap* x = new QPixmap;
	    s >> *x;
	    d->value.ptr = x;
	}
	break;
    case Int:
	{
	    int x;
	    s >> x;
	    d->value.i = x;
	}
	break;
    case UInt:
	{
	    uint x;
	    s >> x;
	    d->value.u = x;
	}
	break;
    case Bool:
	{
	    Q_INT8 x;
	    s >> x;
	    d->value.b = x;
	}
	break;
    case Double:
	{
	    double x;
	    s >> x;
	    d->value.d = x;
	}
	break;
    case SizePolicy:
	{
	    int h,v;
	    Q_INT8 hfw;
	    s >> h >> v >> hfw;
	    d->value.ptr = new QSizePolicy( (QSizePolicy::SizeType)h,
					    (QSizePolicy::SizeType)v,
					    (bool) hfw);
	}
	break;
    }

    d->typ = t;
}

/*!
  Internal function for saving a variant. Use the stream operators
  instead.
*/
void QVariant::save( QDataStream& s ) const
{
    s << (Q_UINT32)type();

    switch( d->typ ) {
    case Cursor:
	s << *((QCursor*)d->value.ptr);
	break;
    case Bitmap:
	s << *((QBitmap*)d->value.ptr);
	break;
    case PointArray:
	s << *((QPointArray*)d->value.ptr);
	break;
    case Region:
	s << *((QRegion*)d->value.ptr);
	break;
    case List:
	s << *((QValueList<QVariant>*)d->value.ptr);
	break;
    case Map:
	s << *((QMap<QString,QVariant>*)d->value.ptr);
	break;
    case String:
	s << *((QString*)d->value.ptr);
	break;
    case CString:
	s << *((QCString*)d->value.ptr);
	break;
    case StringList:
	s << *((QStringList*)d->value.ptr);
	break;
    case Font:
	s << *((QFont*)d->value.ptr);
	break;
    case Pixmap:
	s << *((QPixmap*)d->value.ptr);
	break;
    case Image:
	s << *((QImage*)d->value.ptr);
	break;
    case Brush:
	s << *((QBrush*)d->value.ptr);
	break;
    case Point:
	s << *((QPoint*)d->value.ptr);
	break;
    case Rect:
	s << *((QRect*)d->value.ptr);
	break;
    case Size:
	s << *((QSize*)d->value.ptr);
	break;
    case Color:
	s << *((QColor*)d->value.ptr);
	break;
    case Palette:
	s << *((QPalette*)d->value.ptr);
	break;
    case ColorGroup:
	s << *((QColorGroup*)d->value.ptr);
	break;
    case IconSet:
	//### add stream operator to iconset
	s << ((QIconSet*)d->value.ptr)->pixmap();
	break;
    case Int:
	s << d->value.i;
	break;
    case UInt:
	s << d->value.u;
	break;
    case Bool:
	s << (Q_INT8)d->value.b;
	break;
    case Double:
	s << d->value.d;
	break;
    case SizePolicy:
	{
	    QSizePolicy p = toSizePolicy();
	    s << (int) p.horData() << (int) p.verData()
	      << (Q_INT8) p.hasHeightForWidth();
	}
	break;
    case Invalid:
	s << QString(); // ### looks wrong.
	break;
    }
}

/*!
  Reads a variant \a p from the stream \a s.
  \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/
QDataStream& operator>> ( QDataStream& s, QVariant& p )
{
    p.load( s );
    return s;
}

/*!
  Writes a variant \a p to the stream \a s.
  \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/
QDataStream& operator<< ( QDataStream& s, const QVariant& p )
{
    p.save( s );
    return s;
}

/*!
  Reads a variant type \a p in enum representation from the stream \a s.
*/
QDataStream& operator>> ( QDataStream& s, QVariant::Type& p )
{
    Q_UINT32 u;
    s >> u;
    p = (QVariant::Type) u;

    return s;
}

/*!
  Writes a variant type \a p to the stream \a s.
*/
QDataStream& operator<< ( QDataStream& s, const QVariant::Type p )
{
    s << (Q_UINT32)p;

    return s;
}

/*! \fn Type QVariant::type() const

  Returns the storage type of the value stored in the variant. Usually
  it's best to test with canCast() wether the variant can deliver the
  data type you are interested in.
*/

/*! \fn bool QVariant::isValid() const

  Returns TRUE if the storage type of this variant is not QVariant::Invalid.
*/

/*! \fn QValueListConstIterator<QString> QVariant::stringListBegin() const

  Returns an iterator to the first string in the list, if the
  variant's type is StringList, or else a null iterator.
*/

/*! \fn QValueListConstIterator<QString> QVariant::stringListEnd() const

  Returns the end iterator for the list, if the variant's type is
  StringList, or else a null iterator.
*/

/*! \fn QValueListConstIterator<QVariant> QVariant::listBegin() const

  Returns an iterator to the first item in the list, if the
  variant's type is appropriate, or else a null iterator.
*/

/*! \fn QValueListConstIterator<QVariant> QVariant::listEnd() const

  Returns the end iterator for the list, if the variant's type is
  appropriate, or else a null iterator.
*/

/*! \fn QMapConstIterator<QString, QVariant> QVariant::mapBegin() const

  Returns an iterator to the first item in the map, if the
  variant's type is appropriate, or else a null iterator.
*/

/*! \fn QMapConstIterator<QString, QVariant> QVariant::mapEnd() const

  Returns the end iterator for the map, if the variant's type is
  appropriate, or else a null iterator.
*/

/*! \fn QMapConstIterator<QString, QVariant> QVariant::mapFind( const QString& key ) const

  Returns an iterator to the item in the map with \a key as key, if the
  variant's type is appropriate and \a key is a valid key, or else a
  null iterator.
*/

/*!
  Returns the variant as a QString if the variant has type()
  String or CString, or QString::null otherwise.

  \sa asString()
*/
const QString QVariant::toString() const
{
    if ( d->typ == CString )
	return QString::fromLatin1( toCString() );
    if ( d->typ != String )
	return QString::null;

    return *((QString*)d->value.ptr);
}

/*!
  Returns the variant as a QCString if the variant has type()
  CString, or a 0 otherwise.

  \sa asCString()
*/
const QCString QVariant::toCString() const
{
    if ( d->typ == CString )
	return *((QCString*)d->value.ptr);
    if ( d->typ == String )
	return ((QString*)d->value.ptr)->latin1();

    return 0;
}

/*!
  Returns the variant as a QStringList if the variant has type()
  StringList or List of a type that can be converted to QString, or an
  empty list otherwise.

  \sa asStringList()
*/
const QStringList QVariant::toStringList() const
{
    if ( d->typ == StringList )
	return *((QStringList*)d->value.ptr);
    if ( d->typ == List ) {
	QStringList lst;
	QValueList<QVariant>::ConstIterator it = listBegin();
	QValueList<QVariant>::ConstIterator end = listEnd();
	while( it != end ) {
	    QString tmp = (*it).toString();
	    ++it;
	    lst.append( tmp );
	}
	return lst;
    }

    return QStringList();
}

/*!
  Returns the variant as a QMap<QString,QVariant> if the variant has type()
  Map, or an empty map otherwise.

  \sa asMap()
*/
const QMap<QString, QVariant> QVariant::toMap() const
{
    if ( d->typ != Map )
	return QMap<QString,QVariant>();

    return *((QMap<QString,QVariant>*)d->value.ptr);
}

/*!
  Returns the variant as a QFont if the variant has type()
  Font, or the default font otherwise.

  \sa asFont()
*/
const QFont QVariant::toFont() const
{
    if ( d->typ != Font )
	return QFont();

    return *((QFont*)d->value.ptr);
}

/*!
  Returns the variant as a QPixmap if the variant has type()
  Pixmap, or a null pixmap otherwise.

  \sa asPixmap()
*/
const QPixmap QVariant::toPixmap() const
{
    if ( d->typ != Pixmap )
	return QPixmap();

    return *((QPixmap*)d->value.ptr);
}

/*!
  Returns the variant as a QImage if the variant has type()
  Image, or a null image otherwise.

  \sa asImage()
*/
const QImage QVariant::toImage() const
{
    if ( d->typ != Image )
	return QImage();

    return *((QImage*)d->value.ptr);
}

/*!
  Returns the variant as a QBrush if the variant has type()
  Brush, or a default brush (with all black colors) otherwise.

  \sa asBrush()
*/
const QBrush QVariant::toBrush() const
{
    if( d->typ != Brush )
	return QBrush();

    return *((QBrush*)d->value.ptr);
}

/*!
  Returns the variant as a QPoint if the variant has type()
  Point, or a the point (0,0) otherwise.

  \sa asPoint()
*/
const QPoint QVariant::toPoint() const
{
    if ( d->typ != Point )
	return QPoint();

    return *((QPoint*)d->value.ptr);
}

/*!
  Returns the variant as a QRect if the variant has type()
  Rect, or an empty rectangle otherwise.

  \sa asRect()
*/
const QRect QVariant::toRect() const
{
    if ( d->typ != Rect )
	return QRect();

    return *((QRect*)d->value.ptr);
}

/*!
  Returns the variant as a QSize if the variant has type()
  Size, or an invalid size otherwise.

  \sa asSize()
*/
const QSize QVariant::toSize() const
{
    if ( d->typ != Size )
	return QSize();

    return *((QSize*)d->value.ptr);
}

/*!
  Returns the variant as a QColor if the variant has type()
  Color, or an invalid color otherwise.

  \sa asColor()
*/
const QColor QVariant::toColor() const
{
    if ( d->typ != Color )
	return QColor();

    return *((QColor*)d->value.ptr);
}

/*!
  Returns the variant as a QPalette if the variant has type()
  Palette, or a completely black palette otherwise.

  \sa asPalette()
*/
const QPalette QVariant::toPalette() const
{
    if ( d->typ != Palette )
	return QPalette();

    return *((QPalette*)d->value.ptr);
}

/*!
  Returns the variant as a QColorGroup if the variant has type()
  ColorGroup, or a completely black color group otherwise.

  \sa asColorGroup()
*/
const QColorGroup QVariant::toColorGroup() const
{
    if ( d->typ != ColorGroup )
	return QColorGroup();

    return *((QColorGroup*)d->value.ptr);
}

/*!
  Returns the variant as a QIconSet if the variant has type()
  IconSet, or an icon set of null pixmaps otherwise.

  \sa asIconSet()
*/
const QIconSet QVariant::toIconSet() const
{
    if ( d->typ != IconSet )
	return QIconSet();

    return *((QIconSet*)d->value.ptr);
}

/*!
  Returns the variant as a QPointArray if the variant has type()
  PointArray, or an empty QPointArray otherwise.

  \sa asPointArray()
*/
const QPointArray QVariant::toPointArray() const
{
    if ( d->typ != PointArray )
	return QPointArray();

    return *((QPointArray*)d->value.ptr);
}

/*!
  Returns the variant as a QBitmap if the variant has type()
  Bitmap, or a null QBitmap otherwise.

  \sa asBitmap()
*/
const QBitmap QVariant::toBitmap() const
{
    if ( d->typ != Bitmap )
	return QBitmap();

    return *((QBitmap*)d->value.ptr);
}

/*!
  Returns the variant as a QRegion if the variant has type()
  Region, or an empty QRegion otherwise.

  \sa asRegion()
*/
const QRegion QVariant::toRegion() const
{
    if ( d->typ != Region )
	return QRegion();

    return *((QRegion*)d->value.ptr);
}

/*!
  Returns the variant as a QCursor if the variant has type()
  Cursor, or the default arrow cursor otherwise.

  \sa asCursor()
*/
const QCursor QVariant::toCursor() const
{
#ifndef QT_NO_CURSOR
    if ( d->typ != Cursor )
	return QCursor();
#endif

    return *((QCursor*)d->value.ptr);
}

/*!
  Returns the variant as an int if the variant has type()
  Int, UInt, Double or Bool, or 0 otherwise.

  \sa asInt()
*/
int QVariant::toInt() const
{
    if( d->typ == Int )
	return d->value.i;
    if( d->typ == UInt )
	return (int)d->value.u;
    if ( d->typ == Double )
	return (int)d->value.d;
    if ( d->typ == Bool )
	return (int)d->value.b;

    /* if ( d->typ == String )
	return ((QString*)d->value.ptr)->toInt();
    if ( d->typ == CString )
    return ((QCString*)d->value.ptr)->toInt(); */
    return 0;
}

/*!
  Returns the variant as an unsigned int if the variant has type()
  UInt, Int, Double or Bool, or 0 otherwise.

  \sa asUInt()
*/
uint QVariant::toUInt() const
{
    if( d->typ == Int )
	return d->value.i;
    if( d->typ == UInt )
	return (int)d->value.u;
    if ( d->typ == Double )
	return (int)d->value.d;
    if ( d->typ == Bool )
	return (int)d->value.b;

    return 0;
}

/*!
  Returns the variant as a bool if the variant has type()
  Bool, or FALSE otherwise. The only exceptions to this rule are
  the types Int, UInt, Double. In this case TRUE is returned if the numerical
  value is not zero or FALSE otherwise.

  \sa asBool()
*/
bool QVariant::toBool() const
{
    if ( d->typ == Bool )
	return d->value.b;
    if ( d->typ == Double )
	return d->value.d != 0.0;
    if ( d->typ == Int )
	return d->value.i != 0;
    if ( d->typ == UInt )
	return d->value.u != 0;

    return FALSE;
}

/*!
  Returns the variant as a double if the variant has type()
  Double, Int, UInt or Bool, or 0.0 otherwise.

  \sa asDouble()
*/
double QVariant::toDouble() const
{
    if ( d->typ == Double )
	return d->value.d;
    if ( d->typ == Int )
	return (double)d->value.i;
    if ( d->typ == Bool )
	return (double)d->value.b;
    if ( d->typ == UInt )
	return (double)d->value.u;
    return 0.0;
}

/*!
  Returns the variant as a QValueList<QVariant> if the variant has type()
  List or StringList, or an empty list otherwise.

  \sa asList()
*/
const QValueList<QVariant> QVariant::toList() const
{
    if ( d->typ == List )
	return *((QValueList<QVariant>*)d->value.ptr);
    if ( d->typ == StringList ) {
	QValueList<QVariant> lst;
	QStringList::ConstIterator it = stringListBegin();
	QStringList::ConstIterator end = stringListEnd();
	for( ; it != end; ++it )
	    lst.append( QVariant( *it ) );
	return lst;
    }

    return QValueList<QVariant>();
}

/*! Returns the variant as a QSizePolicy if the variant has type()
SizePolicy, or an undefined but legal size policy else.
*/

QSizePolicy QVariant::toSizePolicy() const
{
    if ( d->typ == SizePolicy )
	return *((QSizePolicy*)d->value.ptr);

    return QSizePolicy();
}


#define Q_VARIANT_AS( f ) Q##f& QVariant::as##f() { \
   if ( d->typ != f ) *this = QVariant( to##f() ); else detach(); return *((Q##f*)d->value.ptr);}

Q_VARIANT_AS(String)
Q_VARIANT_AS(CString)
Q_VARIANT_AS(StringList)
Q_VARIANT_AS(Font)
Q_VARIANT_AS(Pixmap)
Q_VARIANT_AS(Image)
Q_VARIANT_AS(Brush)
Q_VARIANT_AS(Point)
Q_VARIANT_AS(Rect)
Q_VARIANT_AS(Size)
Q_VARIANT_AS(Color)
Q_VARIANT_AS(Palette)
Q_VARIANT_AS(ColorGroup)
Q_VARIANT_AS(IconSet)
Q_VARIANT_AS(PointArray)
Q_VARIANT_AS(Bitmap)
Q_VARIANT_AS(Region)
Q_VARIANT_AS(Cursor)
Q_VARIANT_AS(SizePolicy)

/*! \fn QString& QVariant::asString()

  Tries to convert the variant to hold a string value. If that
  is not possible then the variant is set to an empty string.

  Returns a reference to the stored string.

  \sa toString()
*/

/*! \fn QCString& QVariant::asCString()

  Tries to convert the variant to hold a string value. If that
  is not possible then the variant is set to an empty string.

  Returns a reference to the stored string.

  \sa toCString()
*/

/*! \fn QStringList& QVariant::asStringList()

  Tries to convert the variant to hold a QStringList value. If that
  is not possible then the variant is set to an empty string list.

  Returns a reference to the stored string list.

  \sa toStringList()
*/

/*! \fn QFont& QVariant::asFont()

  Tries to convert the variant to hold a QFont. If that
  is not possible then the variant is set to a default font.

  Returns a reference to the stored font.

  \sa toFont()
*/

/*! \fn QPixmap& QVariant::asPixmap()

  Tries to convert the variant to hold a pixmap value. If that
  is not possible then the variant is set to a null pixmap.

  Returns a reference to the stored pixmap.

  \sa toPixmap()
*/

/*! \fn QImage& QVariant::asImage()

  Tries to convert the variant to hold an image value. If that
  is not possible then the variant is set to a null image.

  Returns a reference to the stored image.

  \sa toImage()
*/

/*! \fn QBrush& QVariant::asBrush()

  Tries to convert the variant to hold a brush value. If that
  is not possible then the variant is set to a default black brush.

  Returns a reference to the stored brush.

  \sa toBrush()
*/

/*! \fn QPoint& QVariant::asPoint()

  Tries to convert the variant to hold a point value. If that
  is not possible then the variant is set to a null point.

  Returns a reference to the stored point.

  \sa toPoint()
*/

/*! \fn QRect& QVariant::asRect()

  Tries to convert the variant to hold a rectangle value. If that
  is not possible then the variant is set to an empty rectangle.

  Returns a reference to the stored rectangle.

  \sa toRect()
*/

/*! \fn QSize& QVariant::asSize()

  Tries to convert the variant to hold a QSize value. If that
  is not possible then the variant is set to an invalid size.

  Returns a reference to the stored size.

  \sa toSize() QSize::isValid()
*/

/*!  \fn QSizePolicy& QVariant::asSizePolicy()

  Tries to convert the variant to hold a QSizePolicy value.  If that
  fails, the variant is set to an arbitrary size policy.
*/


/*! \fn QColor& QVariant::asColor()

  Tries to convert the variant to hold a QColor value. If that
  is not possible then the variant is set to an invalid color.

  Returns a reference to the stored color.

  \sa toColor() QColor::isValid()
*/

/*! \fn QPalette& QVariant::asPalette()

  Tries to convert the variant to hold a QPalette value. If that
  is not possible then the variant is set to a palette with black colors only.

  Returns a reference to the stored palette.

  \sa toString()
*/

/*! \fn QColorGroup& QVariant::asColorGroup()

  Tries to convert the variant to hold a QColorGroup value. If that
  is not possible then the variant is set to a color group with all colors
  set to black.

  Returns a reference to the stored color group.

  \sa toColorGroup()
*/

/*! \fn QIconSet& QVariant::asIconSet()

  Tries to convert the variant to hold a QIconSet value. If that
  is not possible then the variant is set to an empty iconset.

  Returns a reference to the stored iconset.

  \sa toIconSet()
*/

/*! \fn QPointArray& QVariant::asPointArray()

  Tries to convert the variant to hold a QPointArray value. If that
  is not possible then the variant is set to an empty point array.

  Returns a reference to the stored point array.

  \sa toPointArray()
*/

/*! \fn QBitmap& QVariant::asBitmap()

  Tries to convert the variant to hold a bitmap value. If that
  is not possible then the variant is set to a null bitmap.

  Returns a reference to the stored bitmap.

  \sa toBitmap()
*/

/*! \fn QRegion& QVariant::asRegion()

  Tries to convert the variant to hold a QRegion value. If that
  is not possible then the variant is set to a null region.

  Returns a reference to the stored region.

  \sa toRegion()
*/

/*! \fn QCursor& QVariant::asCursor()

  Tries to convert the variant to hold a QCursor value. If that
  is not possible then the variant is set to a default arrow cursor.

  Returns a reference to the stored cursor.

  \sa toCursor()
*/

/*!
  Returns the variant's value as int reference.
*/
int& QVariant::asInt()
{
    detach();
    if ( d->typ != Int ) {
	d->value.i = toInt();
	d->typ = Int;
    }
    return d->value.i;
}

/*!
  Returns the variant's value as unsigned int reference.
*/
uint& QVariant::asUInt()
{
    detach();
    if ( d->typ != UInt ) {
	d->value.u = toUInt();
	d->typ = UInt;
    }
    return d->value.u;
}

/*!
  Returns the variant's value as bool reference.
*/
bool& QVariant::asBool()
{
    detach();
    if ( d->typ != Bool ) {
	d->value.b = toBool();
	d->typ = Bool;
    }
    return d->value.b;
}

/*!
  Returns the variant's value as double reference.
*/
double& QVariant::asDouble()
{
    if ( d->typ != Double ) {
	d->value.d = toDouble();
	d->typ = Double;
    }
    return d->value.d;
}

/*!
  Returns the variant's value as variant list reference.
*/
QValueList<QVariant>& QVariant::asList()
{
    if ( d->typ != List )
	*this = QVariant( toList() );
    return *((QValueList<QVariant>*)d->value.ptr);
}

/*!
  Returns the variant's value as variant map reference.
*/
QMap<QString, QVariant>& QVariant::asMap()
{
    if ( d->typ != Map )
	*this = QVariant( toMap() );
    return *((QMap<QString,QVariant>*)d->value.ptr);
}


/*!
  Returns TRUE if the current type of the variant can be cast to
  the requested type. Such casting is done automatically when calling
  the toInt(), toBool(), ... or asInt(), asBool(), ... methods.

  The following casts are done automatically:
  <ul>
  <li> Bool -> Double, Int, UInt
  <li> Double -> Int, Bool, UInt
  <li> Int -> Double, Bool, UInt
  <li> UInt -> Double, Bool, Int
  <li> String -> CString
  <li> CString -> String
  <li> List -> StringList (if the list contains strings or something
       that can be cast to a string).
  <li> StringList -> List
  </ul>
*/
bool QVariant::canCast( Type t ) const
{
    if ( d->typ == t )
	return TRUE;
    if ( t == Bool && ( d->typ == Double || d->typ == Int || d->typ == UInt ) )
	 return TRUE;
    if ( t == Int && ( d->typ == Double || d->typ == Bool || d->typ == UInt ) )
	return TRUE;
    if ( t == UInt && ( d->typ == Double || d->typ == Bool || d->typ == Int ) )
	return TRUE;
    if ( t == Double && ( d->typ == Int || d->typ == Bool || d->typ == UInt ) )
	return TRUE;
    if ( t == CString && d->typ == String )
	return TRUE;
    if ( t == String && d->typ == CString )
	return TRUE;
    if ( t == List && d->typ == StringList )
	return TRUE;
    if ( t == StringList && d->typ == List )   {
	QValueList<QVariant> vl = toList();
	QValueList<QVariant>::ConstIterator it = listBegin();
	QValueList<QVariant>::ConstIterator end = listEnd();
	for( ; it != end; ++it ) {
	    if ( !(*it).canCast( String ) )
		return FALSE;
	}
	return TRUE;
    }

    return FALSE;
}

/*!  Compares this QVariant with \a v and returns TRUE if they are
  equal, FALSE otherwise.
*/

bool QVariant::operator==( const QVariant &v ) const
{
    switch( d->typ ) {
    case Cursor:
#ifndef QT_NO_CURSOR
	return v.toCursor().shape() == toCursor().shape();
#endif
    case Bitmap:
	return v.toBitmap().serialNumber() == toBitmap().serialNumber();
    case PointArray:
	return v.toPointArray() == toPointArray();
    case Region:
	return v.toRegion() == toRegion();
    case List:
	return v.toList() == toList();
    case Map: {
	if ( v.toMap().count() != toMap().count() )
	    return FALSE;
	QMap<QString, QVariant>::ConstIterator it = v.toMap().begin();
	QMap<QString, QVariant>::ConstIterator it2 = toMap().begin();
	for ( ; it != v.toMap().end(); ++it ) {
	    if ( *it != *it2 )
		return FALSE;
	}
	return TRUE;
    }
    case String:
	return v.toString() == toString();
    case CString:
	return v.toCString() == toCString();
    case StringList:
	return v.toStringList() == toStringList();
    case Font:
	return v.toFont() == toFont();
    case Pixmap:
	return v.toPixmap().serialNumber() == toPixmap().serialNumber();
    case Image:
	return v.toImage() == toImage();
    case Brush:
	return v.toBrush() == toBrush();
    case Point:
	return v.toPoint() == toPoint();
    case Rect:
	return v.toRect() == toRect();
    case Size:
	return v.toSize() == toSize();
    case Color:
	return v.toColor() == toColor();
    case Palette:
	return v.toPalette() == toPalette();
    case ColorGroup:
	return v.toColorGroup() == toColorGroup();
    case IconSet:
	return v.toIconSet().pixmap().serialNumber()
	    == toIconSet().pixmap().serialNumber();
    case Int:
	return v.toInt() == toInt();
    case UInt:
	return v.toUInt() == toUInt();
    case Bool:
	return v.toBool() == toBool();
    case Double:
	return v.toDouble() == toDouble();
    case SizePolicy:
	return v.toSizePolicy() == toSizePolicy();
    case Invalid:
	break;
    }
    return FALSE;
}

/*!  Compares this QVariant with \a v and returns TRUE if they are
  not equal, FALSE otherwise.
*/

bool QVariant::operator!=( const QVariant &v ) const
{
    return !( v == *this );
}

#endif // QT_NO_PROPERTIES

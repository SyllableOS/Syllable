/****************************************************************************
** $Id$
**
** Definition of QMetaObject class
**
** Created : 930419
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

#ifndef QMETAOBJECT_H
#define QMETAOBJECT_H

#ifndef QT_H
#include "qconnection.h"
#include "qstrlist.h"
#endif // QT_H

class QObject;

struct QMetaData				// - member function meta data
{						//   for signal and slots
    const char *name;				// - member name
    QMember ptr;				// - member pointer
    enum Access { Private, Protected, Public };
    /* ### add this in 3.0
       Access access;				// - access permission
    */
};


struct QMetaEnum 				// enumerator meta data
{						//  for properties
    QMetaEnum() { }
    ~QMetaEnum() { delete [] items; }
    const char *name;				// - enumerator name
    uint count;					// - number of values
    struct Item 				// - a name/value pair
    {
	const char *key;
	int value;
    };
    Item *items;				// - the name/value pairs
    bool set;					// whether enum has to be treated as a set
};

#ifndef QT_NO_PROPERTIES
class Q_EXPORT QMetaProperty 			// property meta data
{
public:
    QMetaProperty();
    ~QMetaProperty();

    const char 	*type() const { return t; }	// type of the property
    const char*	name() const { return n; }	// name of the property

    bool writable() const;
    bool writeable() const;			// ### remove in 3.0
    bool isValid() const;

    bool isSetType() const;
    bool isEnumType() const;
    QStrList enumKeys() const;			// enumeration names

    int keyToValue( const char* key ) const; 	// enum and set conversion functions
    const char* valueToKey( int value ) const;
    int keysToValue( const QStrList& keys ) const;
    QStrList valueToKeys( int value ) const;

    bool stored( QObject* ) const;
    bool designable() const;

    const char* t;
    const char* n;
    QMember 	get;				// get-function or 0 ( 0 indicates an error )
    QMember 	set;				// set-function or 0
    QMember 	store;				// store-function or 0
    QMember     reset;                          // reset-function or 0
    QMetaEnum	*enumData; 			// a pointer to the enum specification or 0

    enum Specification  { Unspecified, Class, Reference, Pointer, ConstCharStar };
    Specification gspec;			// specification of the get-function
    Specification sspec;			// specification of the set-function

    enum Flags  {
	UnresolvedEnum       = 0x00000001,
	UnresolvedSet        = 0x00000002,
	UnresolvedEnumOrSet  = 0x00000004,
	UnresolvedStored     = 0x00000008,
	UnresolvedDesignable = 0x00000010,
	NotDesignable        = 0x00000020,
	NotStored            = 0x00000040,
	StdSet 	             = 0x00000080 
    };

    bool testFlags( uint f ) const;
    void setFlags( uint f );
    void clearFlags( uint f );

private:
    uint flags;
};
#endif // QT_NO_PROPERTIES

struct QClassInfo 				// class info meta data
{
    const char* name;				// - name of the info
    const char* value;				// - value of the info
};

class QMetaObjectPrivate;

class Q_EXPORT QMetaObject			// meta object class
{
public:
    QMetaObject( const char *class_name, const char *superclass_name,
		 QMetaData *slot_data,	int n_slots,
		 QMetaData *signal_data, int n_signals );
    QMetaObject( const char *class_name, const char *superclass_name,
		 QMetaData *slot_data,	int n_slots,
		 QMetaData *signal_data, int n_signals,
#ifndef QT_NO_PROPERTIES
		 QMetaProperty *prop_data, int n_props,
		 QMetaEnum *enum_data, int n_enums,
#endif
		 QClassInfo *class_info, int n_info );


    virtual ~QMetaObject();

    const char	*className()		const { return classname; }
    const char	*superClassName()	const { return superclassname; }

    QMetaObject *superClass()		const { return superclass; }

    bool 	inherits( const char* clname ) const;

    int  	numSlots( bool super = FALSE ) const;
    int		numSignals( bool super = FALSE ) const;

    QMetaData	*slot( int index, bool super = FALSE ) const;
    QMetaData	*signal( int index, bool super = FALSE ) const;

    QMetaData	*slot( const char *, bool super = FALSE ) const;
    QMetaData	*signal( const char *, bool super = FALSE ) const;

    QStrList	slotNames( bool super = FALSE ) const;
    QStrList	signalNames( bool super = FALSE ) const;

    int		numClassInfo( bool super = FALSE ) const;
    QClassInfo 	*classInfo( int index, bool super = FALSE ) const;
    const char 	*classInfo( const char* name, bool super = FALSE ) const;

#ifndef QT_NO_PROPERTIES
    const QMetaProperty	*property( const char* name, bool super = FALSE ) const;
    QStrList		propertyNames( bool super = FALSE ) const;
    void		resolveProperty( QMetaProperty* prop );
#endif

    // static wrappers around constructors, necessary to work around a
    // Windows-DLL limitation: objects can only be deleted within a
    // DLL if they were actually created within that DLL.
    static QMetaObject	*new_metaobject( const char *, const char *,
					QMetaData *, int,
					QMetaData *, int,
#ifndef QT_NO_PROPERTIES
					QMetaProperty *prop_data, int n_props,
					QMetaEnum *enum_data, int n_enums,
#endif
					QClassInfo * class_info, int n_info );
    static QMetaObject	*new_metaobject( const char *, const char *,
					QMetaData *, int,
					QMetaData *, int );
    static QMetaData		*new_metadata( int );
    static QMetaData::Access		*new_metaaccess( int ); // ### remove in 3.0
    void set_slot_access( QMetaData::Access* ); 		// ### remove in 3.0
    QMetaData::Access slot_access(int index, bool super = FALSE ); // ### remove in 3.0
    static QMetaEnum 		*new_metaenum( int );
    static QMetaEnum::Item 	*new_metaenum_item( int );
#ifndef QT_NO_PROPERTIES
    static QMetaProperty 	*new_metaproperty( int );
#endif
    static QClassInfo 		*new_classinfo( int );

private:
    QMemberDict		*init( QMetaData *, int );
    QMetaData		*mdata( int code, const char *, bool ) const;
    QMetaData		*mdata( int code, int index, bool super ) const;

    const char		*classname;			// class name
    const char		*superclassname;		// super class name
    QMetaObject 	*superclass;			// super class meta object
    QMetaObjectPrivate	*d;				// private data for...
    void        	*reserved;			// ...binary compatibility
    QMetaData		*slotData;			// slot meta data
    QMemberDict 	*slotDict;			// slot dictionary
    QMetaData		*signalData;			// signal meta data
    QMemberDict 	*signalDict;			// signal dictionary
    QMetaEnum		*enumerator( const char* name, bool super = FALSE ) const;

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QMetaObject( const QMetaObject & );
    QMetaObject &operator=( const QMetaObject & );
#endif
};

#ifndef QT_NO_PROPERTIES
inline bool QMetaProperty::writable() const
{ return set != 0; }
inline bool QMetaProperty::writeable() const
{ return set != 0; }
inline bool QMetaProperty::testFlags( uint f ) const
{ return (flags & (uint)f) != (uint)0; }
inline bool QMetaProperty::isValid() const
{ return get != 0 && !testFlags( UnresolvedEnum | UnresolvedDesignable | UnresolvedStored ) ; }
inline bool QMetaProperty::isSetType() const
{ return ( enumData != 0 && enumData->set ); }
inline bool QMetaProperty::isEnumType() const
{ return ( enumData != 0 ); }
inline bool QMetaProperty::designable() const
{ return ( isValid() && set != 0 && !testFlags( NotDesignable | UnresolvedDesignable ) ); }
inline void QMetaProperty::setFlags( uint f )
{ flags |= (uint)f; }
inline void QMetaProperty::clearFlags( uint f )
{ flags &= ~(uint)f; }
#endif

// ### remove 3.0 (binary compatibility with Qt-2.0.2)
class Q_EXPORT QMetaObjectInit {
public:
    QMetaObjectInit(void(*)());
    static int init();
};

#endif // QMETAOBJECT_H

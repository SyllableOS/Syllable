/****************************************************************************
** $Id$
**
** Implementation of QSignal class
**
** Created : 941201
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

#include "qsignal.h"
#include "qmetaobject.h"
#include "qptrdict.h"
#include <ctype.h>

// NOT REVISED
/*!
  \class QSignal qsignal.h
  \brief The QSignal class can be used to send signals without parameters.

  \ingroup misc

  QSignal is a simple extension of QObject that can send plain signals
  without parameters.  If you want to send signals from a class that does
  not inherit QObject, you can create an internal QSignal object to emit
  the signal. You must also provide a function that connects the signal to
  an outside object slot.  This is how we have implemented signals in the
  QMenuData class, which is not a QObject.

  In general, we recommend inheriting QObject instead.	QObject provides
  much more functionality.

  Note that QObject is a \e private base class of QSignal, i.e. you cannot
  call any QObject member functions from a QSignal object.

  Example:
  \code
    #include <qsignal.h>

    class MyClass
    {
    public:
	MyClass();
       ~MyClass();

	void doSomething();

	void connect( QObject *receiver, const char *member );

    private:
	QSignal *sig;
    };

    MyClass::MyClass()
    {
	sig = new QSignal;
    }

    MyClass::~MyClass()
    {
	delete sig;
    }

    void MyClass::doSomething()
    {
	// ... does something
	sig->activate();	// activates the signal
    }

    void MyClass::connect( QObject *receiver, const char *member )
    {
	sig->connect( receiver, member );
    }
  \endcode
*/


QMetaObject *QSignal::metaObj = 0;



#if QT_VERSION >= 300
#error "Move to QSignal body"
#endif

class QSignalPrivate {
public:
    QSignalPrivate() :
	val(0)
    {
    }
    int val;
};

static QPtrDict<QSignalPrivate>* d_ptr = 0;
static void cleanup_d_ptr()
{
    delete d_ptr;
    d_ptr = 0;
}
static QSignalPrivate* d( const QSignal* that )
{
    if ( !d_ptr ) {
	d_ptr = new QPtrDict<QSignalPrivate>;
	d_ptr->setAutoDelete( TRUE );
	qAddPostRoutine( cleanup_d_ptr );
    }
    QSignalPrivate* ret = d_ptr->find( (void*) that );
    if ( ! ret ) {
	ret = new QSignalPrivate;
	d_ptr->replace( (void*) that, ret );
    }
    return ret;
}
static void delete_d( const QSignal* that )
{
    if ( d_ptr )
	d_ptr->remove( (void*) that );
}
static bool has_d( const QSignal* that )
{
    return d_ptr && d_ptr->find( (void*) that);
}




/*!
  Constructs a signal object with the parent object \e parent and a \e name.
  These arguments are passed directly to QObject.
*/

QSignal::QSignal( QObject *parent, const char *name )
    : QObject( parent, name )
{
    if ( !metaObj )				// will create object dict
	initMetaObject();
    isSignal = TRUE;
}

/*!
  Destructs the signal.  All connections are removed, as is the case
  with all QObjects.
*/
QSignal::~QSignal()
{
    delete_d(this);
}


/*!
  \fn const char *QSignal::name() const
  Returns the name of this signal object.

  Since QObject is a private base class, we have added this function, which
  calls QObject::name().
*/

/*!
  \fn void QSignal::setName( const char *name )
  Sets the name of this signal object to \e name.

  Since QObject is a private base class, we have added this function, which
  calls QObject::setName().
*/

/* NOTE: should not be documented */

const char *QSignal::className() const
{
    return "QSignal";
}


/*!
  Connects the signal to \e member in object \e receiver.
  \sa disconnect(), QObject::connect()
*/

bool QSignal::connect( const QObject *receiver, const char *member )
{
    return QObject::connect( (QObject *)this, SIGNAL(x(int)),
			     receiver, member );
}

/*!
  Disonnects the signal from \e member in object \e receiver.
  \sa connect(), QObject::disconnect()
*/

bool QSignal::disconnect( const QObject *receiver, const char *member )
{
    return QObject::disconnect( (QObject *)this, SIGNAL(x(int)),
				receiver, member );
}


/*!
  \fn bool QSignal::isBlocked() const
  Returns TRUE if the signal is blocked, or FALSE if it is not blocked.

  The signal is not blocked by default.

  \sa block(), QObject::signalsBlocked()
*/

/*!
  \fn void QSignal::block( bool b )
  Blocks the signal if \e b is TRUE, or unblocks the signal if \e b is FALSE.

  An activated signal disappears into hyperspace if it is blocked.

  \sa isBlocked(), activate(), QObject::blockSignals()
*/


/*!
  \fn void QSignal::activate()
  Emits the signal.

  \sa isBlocked()
*/
void  QSignal::activate()
{
    if ( has_d(this) )
	activate_signal("x(int)", d(this)->val );
    else
	activate_signal("x(int)", 0 );
}


/*!
  Sets the signal's parameter to \a value
 */
void QSignal::setParameter( int value )
{
    d(this)->val = value;
}

/*!
  Returns the signal's parameter.
 */
int QSignal::parameter() const
{
    if ( has_d(this) )
	return d(this)->val;
    else
	return 0;
}

void QSignal::dummy(int)				// just for the meta object
{						//   should never be called
#if defined(CHECK_STATE)
    qWarning( "QSignal: Internal error" );
#endif
}

void QSignal::initMetaObject()			// initialize meta object
{
    if ( metaObj )
	return;
    if ( qstrcmp(QObject::className(), "QObject") != 0 )
	badSuperclassWarning("QWidget","QObject");
    (void)staticMetaObject();
}

QMetaObject* QSignal::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    (void)QObject::staticMetaObject();

    typedef void(QSignal::*m2_t0)(int);
    m2_t0 v2_0 =  &QSignal::dummy;
    QMetaData *signal_tbl = QMetaObject::new_metadata(1);
    signal_tbl[0].name = "x(int)";
    signal_tbl[0].ptr = *((QMember*)&v2_0);
    metaObj = QMetaObject::new_metaobject(
	"QSignal", "QObject",
	0, 0,
	signal_tbl, 1 );
    return metaObj;
}

/****************************************************************************
** $Id$
**
** Definition of validator classes.
**
** Created : 970610
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the widgets module of the Qt GUI Toolkit.
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

#ifndef QVALIDATOR_H
#define QVALIDATOR_H

#ifndef QT_H
#include "qobject.h"
#include "qstring.h"
#endif // QT_H

#ifndef QT_NO_VALIDATOR


class Q_EXPORT QValidator: public QObject
{
    Q_OBJECT
public:
    QValidator( QWidget * parent, const char *name = 0 );
    ~QValidator();

    enum State { Invalid, Intermediate, Valid=Intermediate, Acceptable };

    virtual State validate( QString &, int & ) const = 0;
    virtual void fixup( QString & ) const;

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QValidator( const QValidator & );
    QValidator& operator=( const QValidator & );
#endif
};


class Q_EXPORT QIntValidator: public QValidator
{
    Q_OBJECT
    Q_PROPERTY( int bottom READ bottom WRITE setBottom )
    Q_PROPERTY( int top READ top WRITE setTop )

public:
    QIntValidator( QWidget * parent, const char *name = 0 );
    QIntValidator( int bottom, int top,
		   QWidget * parent, const char *name = 0 );
    ~QIntValidator();

    QValidator::State validate( QString &, int & ) const;

    void setBottom( int );
    void setTop( int );
    virtual void setRange( int bottom, int top );

    int bottom() const { return b; }
    int top() const { return t; }

private:
    int b, t;

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QIntValidator( const QIntValidator & );
    QIntValidator& operator=( const QIntValidator & );
#endif
};


class Q_EXPORT QDoubleValidator: public QValidator
{
    Q_OBJECT
    Q_PROPERTY( double bottom READ bottom WRITE setBottom )
    Q_PROPERTY( double top READ top WRITE setTop )
    Q_PROPERTY( int decimals READ decimals WRITE setDecimals )

public:
    QDoubleValidator( QWidget * parent, const char *name = 0 );
    QDoubleValidator( double bottom, double top, int decimals,
		      QWidget * parent, const char *name = 0 );
    ~QDoubleValidator();

    QValidator::State validate( QString &, int & ) const;

    virtual void setRange( double bottom, double top, int decimals = 0 );
    void setBottom( double );
    void setTop( double );
    void setDecimals( int );

    double bottom() const { return b; }
    double top() const { return t; }
    int decimals() const { return d; }

private:
    double b, t;
    int d;

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QDoubleValidator( const QDoubleValidator & );
    QDoubleValidator& operator=( const QDoubleValidator & );
#endif
};


#endif // QT_NO_VALIDATOR

#endif // QVALIDATOR_H

/****************************************************************************
** $Id$
**
** Definition of QRadioButton class
**
** Created : 940222
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

#ifndef QRADIOBUTTON_H
#define QRADIOBUTTON_H

#ifdef __ATHEOS__
#include "../../../konq-embed/dropin/atheos/aradiobutton.h"
#else

#ifndef QT_H
#include "qbutton.h"
#endif // QT_H

#ifndef QT_NO_RADIOBUTTON

class Q_EXPORT QRadioButton : public QButton
{
    Q_OBJECT
    Q_PROPERTY( bool checked READ isChecked WRITE setChecked )

public:
    QRadioButton( QWidget *parent, const char *name=0 );
    QRadioButton( const QString &text, QWidget *parent, const char* name=0 );

    bool    isChecked() const;
    virtual void    setChecked( bool check );

    QSize    sizeHint() const;
    QSizePolicy sizePolicy() const;

protected:
    bool    hitButton( const QPoint & ) const;
    void    drawButton( QPainter * );
    void    drawButtonLabel( QPainter * );

    void    resizeEvent( QResizeEvent* );
    void    focusInEvent( QFocusEvent * );

    void    updateMask();

private:
    void    init();

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QRadioButton( const QRadioButton & );
    QRadioButton &operator=( const QRadioButton & );
#endif
};


inline bool QRadioButton::isChecked() const
{ return isOn(); }

#endif // QT_NO_RADIOBUTTON

#endif // __ATHEOS__

#endif // QRADIOBUTTON_H

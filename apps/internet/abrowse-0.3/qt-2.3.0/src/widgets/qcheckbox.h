/****************************************************************************
** $Id$
**
** Definition of QCheckBox class
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

#ifndef QCHECKBOX_H
#define QCHECKBOX_H

#ifdef __ATHEOS__
#include "../../../konq-embed/dropin/atheos/acheckbox.h"
#else

#ifndef QT_H
#include "qbutton.h"
#endif // QT_H

#ifndef QT_NO_CHECKBOX

class Q_EXPORT QCheckBox : public QButton
{
    Q_OBJECT
    Q_PROPERTY( bool checked READ isChecked WRITE setChecked )
    Q_PROPERTY( bool tristate READ isTristate WRITE setTristate )

public:
    QCheckBox( QWidget *parent, const char *name=0 );
    QCheckBox( const QString &text, QWidget *parent, const char* name=0 );

    bool    isChecked() const;
    void    setChecked( bool check );

    void    setNoChange();

    void    setTristate(bool y=TRUE);
    bool    isTristate() const;

    QSize sizeHint() const;
    QSizePolicy sizePolicy() const;

protected:
    void    resizeEvent( QResizeEvent* );
    void    drawButton( QPainter * );
    void    drawButtonLabel( QPainter * );
    void    updateMask();

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QCheckBox( const QCheckBox & );
    QCheckBox &operator=( const QCheckBox & );
#endif
};


inline bool QCheckBox::isChecked() const
{ return isOn(); }

inline void QCheckBox::setChecked( bool check )
{ setOn( check ); }


#endif // QT_NO_CHECKBOX

#endif // __ATHEOS__

#endif // QCHECKBOX_H

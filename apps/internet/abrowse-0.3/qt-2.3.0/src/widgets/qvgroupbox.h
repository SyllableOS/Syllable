/**********************************************************************
** $Id$
**
** Definition of QVGroupBox widget class
**
** Created : 990602
**
** Copyright (C) 1999-2000 Trolltech AS.  All rights reserved.
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

#ifndef QVGROUPBOX_H
#define QVGROUPBOX_H

#ifndef QT_H
#include "qgroupbox.h"
#endif // QT_H

#ifndef QT_NO_VGROUPBOX

class Q_EXPORT QVGroupBox : public QGroupBox
{
    Q_OBJECT
public:
    QVGroupBox( QWidget *parent=0, const char *name=0 );
    QVGroupBox( const QString &title, QWidget *parent=0, const char* name=0 );
   ~QVGroupBox();

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QVGroupBox( const QVGroupBox & );
    QVGroupBox &operator=( const QVGroupBox & );
#endif
};

#endif // QT_NO_VGROUPBOX

#endif // QVGROUPBOX_H

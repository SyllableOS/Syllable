/****************************************************************************
** $Id$
**
** Definition of QWidgetIntDict
**
** Created : 950116
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

#ifndef QWIDINTDICT_H
#define QWIDINTDICT_H

#ifndef QT_H
#include "qwidget.h"
#include "qintdict.h"
#endif // QT_H


#if defined(Q_TEMPLATEDLL)
template class Q_EXPORT QIntDict<QWidget>;
template class Q_EXPORT QIntDictIterator<QWidget>;
#endif


class Q_EXPORT QWidgetIntDict : public QIntDict<QWidget>
{
public:
    QWidgetIntDict(int size=17) : QIntDict<QWidget>(size) {}
    QWidgetIntDict( const QWidgetIntDict &dict ) : QIntDict<QWidget>(dict) {}
   ~QWidgetIntDict() { clear(); }
    QWidgetIntDict &operator=(const QWidgetIntDict &dict)
	{ return (QWidgetIntDict&)QIntDict<QWidget>::operator=(dict); }
};

class Q_EXPORT QWidgetIntDictIt : public QIntDictIterator<QWidget>
{
public:
    QWidgetIntDictIt( const QWidgetIntDict &d ) : QIntDictIterator<QWidget>(d) {}
    QWidgetIntDictIt &operator=(const QWidgetIntDictIt &i)
	{ return (QWidgetIntDictIt&)QIntDictIterator<QWidget>::operator=(i); }
};


#endif // QWIDINTDICT_H

/****************************************************************************
** $Id$
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


#ifndef QHBOX_H
#define QHBOX_H

#ifndef QT_H
#include "qwidget.h"
#endif // QT_H

#ifndef QT_NO_HBOX

#include "qframe.h"

class QBoxLayout;

class Q_EXPORT QHBox : public QFrame
{
    Q_OBJECT
public:
    QHBox( QWidget *parent=0, const char *name=0, WFlags f=0,  bool allowLines=TRUE  );
    void setSpacing( int );
    bool setStretchFactor( QWidget*, int stretch );
    QSize sizeHint() const;

protected:
    QHBox( bool horizontal, QWidget *parent=0, const char *name=0, WFlags f=0,  bool allowLines=TRUE  );
    void frameChanged();

private:
    QBoxLayout *lay;
};

#endif // QT_NO_HBOX

#endif // QHBOX_H

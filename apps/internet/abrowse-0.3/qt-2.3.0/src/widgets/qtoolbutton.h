/****************************************************************************
** $Id$
**
** Definition of a buttom customized for tool bar use
**
** Created : 979899
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

#ifndef QTOOLBUTTON_H
#define QTOOLBUTTON_H

#ifndef QT_H
#include "qbutton.h"
#include "qstring.h"
#include "qpixmap.h"
#include "qiconset.h"
#endif // QT_H

#ifndef QT_NO_TOOLBUTTON


class QToolButtonPrivate;
class QToolBar;
class QStyle;

class Q_EXPORT QToolButton: public QButton
{
    Q_OBJECT

    Q_PROPERTY( QIconSet onIconSet READ onIconSet WRITE setOnIconSet )
    Q_PROPERTY( QIconSet offIconSet READ offIconSet WRITE setOffIconSet )
    Q_PROPERTY( bool usesBigPixmap READ usesBigPixmap WRITE setUsesBigPixmap )
    Q_PROPERTY( bool usesTextLabel READ usesTextLabel WRITE setUsesTextLabel )
    Q_PROPERTY( QString textLabel READ textLabel WRITE setTextLabel )
    Q_PROPERTY( int popupDelay READ popupDelay WRITE setPopupDelay )
    Q_PROPERTY( bool autoRaise READ autoRaise WRITE setAutoRaise )

    Q_OVERRIDE( bool toggleButton WRITE setToggleButton )
    Q_OVERRIDE( bool on WRITE setOn )

public:
    QToolButton( QWidget * parent, const char *name = 0 );
    QToolButton( const QPixmap & pm, const QString &textLabel, //### fjern 3.0
		 const QString& grouptext,
		 QObject * receiver, const char* slot,
		 QToolBar * parent, const char* name = 0 );
    QToolButton( const QIconSet& s, const QString &textLabel,
		 const QString& grouptext,
		 QObject * receiver, const char* slot,
		 QToolBar * parent, const char* name = 0 );
    QToolButton( ArrowType type, QWidget *parent, const char *name = 0 );
    ~QToolButton();

    QSize sizeHint() const;
    QSizePolicy sizePolicy() const;

    void setOnIconSet( const QIconSet& );
    void setOffIconSet( const QIconSet& );
    QIconSet onIconSet() const;
    QIconSet offIconSet( ) const;
    virtual void setIconSet( const QIconSet &, bool on = FALSE );
    QIconSet iconSet( bool on = FALSE) const;

    bool usesBigPixmap() const { return ubp; }
    bool usesTextLabel() const { return utl; }
    QString textLabel() const { return tl; }

    void setPopup( QPopupMenu* popup );
    QPopupMenu* popup() const;

    void setPopupDelay( int delay );
    int popupDelay() const;

    void setAutoRaise( bool enable );
    bool autoRaise() const;

public slots:
    virtual void setUsesBigPixmap( bool enable );
    virtual void setUsesTextLabel( bool enable );
    virtual void setTextLabel( const QString &, bool );

    virtual void setToggleButton( bool enable ); //### fjern virtual 3.0

    virtual void setOn( bool enable ); //### fjern virtual 3.0
    void toggle();
    void setTextLabel( const QString & );

protected:
    void drawButton( QPainter * );
    void drawButtonLabel( QPainter * );

    void enterEvent( QEvent * );
    void leaveEvent( QEvent * );
    void moveEvent( QMoveEvent * );

    bool uses3D() const;

private slots:
    void popupTimerDone();
    void popupPressed();

private:
    void init();

    QPixmap bp;
    int bpID;
    QPixmap sp;
    int spID;

    QString tl;

    QToolButtonPrivate * d;
    QIconSet * s, *son;

    uint utl: 1;
    uint ubp: 1;
    uint hasArrow : 1;

    friend class QStyle;

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QToolButton( const QToolButton & );
    QToolButton& operator=( const QToolButton & );
#endif
};

#endif // QT_NO_TOOLBUTTON

#endif // QTOOLBUTTON_H

/**********************************************************************
** $Id$
**
** Definition of QComboBox class
**
** Created : 950426
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

#ifndef QCOMBOBOX_H
#define QCOMBOBOX_H

#ifdef __ATHEOS__
#include "../../../konq-embed/dropin/atheos/acombobox.h"
#else

#ifndef QT_H
#include "qwidget.h"
#endif // QT_H

#ifndef QT_NO_COMBOBOX


struct QComboData;
class QStrList;
class QStringList;
class QLineEdit;
class QValidator;
class QListBox;


class Q_EXPORT QComboBox : public QWidget
{
    Q_OBJECT
    Q_ENUMS( Policy )
    Q_PROPERTY( bool editable READ editable WRITE setEditable )
    Q_PROPERTY( int count READ count )
    Q_PROPERTY( QString currentText READ currentText )
    Q_PROPERTY( int currentItem READ currentItem WRITE setCurrentItem )
    Q_PROPERTY( bool autoResize READ autoResize WRITE setAutoResize )
    Q_PROPERTY( int sizeLimit READ sizeLimit WRITE setSizeLimit )
    Q_PROPERTY( int maxCount READ maxCount WRITE setMaxCount )
    Q_PROPERTY( Policy insertionPolicy READ insertionPolicy WRITE setInsertionPolicy )
    Q_PROPERTY( bool autoCompletion READ autoCompletion WRITE setAutoCompletion )
    Q_PROPERTY( bool duplicatesEnabled READ duplicatesEnabled WRITE setDuplicatesEnabled )

public:
    QComboBox( QWidget *parent=0, const char *name=0 );
    QComboBox( bool rw, QWidget *parent=0, const char *name=0 );
   ~QComboBox();

    int		count() const;

    void	insertStringList( const QStringList &, int index=-1 );
    void	insertStrList( const QStrList &, int index=-1 );
    void	insertStrList( const QStrList *, int index=-1 );
    void	insertStrList( const char **, int numStrings=-1, int index=-1);

    void	insertItem( const QString &text, int index=-1 );
    void	insertItem( const QPixmap &pixmap, int index=-1 );
    void	insertItem( const QPixmap &pixmap, const QString &text, int index=-1 );

    void	removeItem( int index );
    void	clear();

    QString currentText() const;
    QString text( int index ) const;
    const QPixmap *pixmap( int index ) const;

    void	changeItem( const QString &text, int index );
    void	changeItem( const QPixmap &pixmap, int index );
    void	changeItem( const QPixmap &pixmap, const QString &text, int index );

    int		currentItem() const;
    virtual void	setCurrentItem( int index );

    bool	autoResize()	const;
    virtual void	setAutoResize( bool );
    QSize	sizeHint() const;
    virtual QSizePolicy sizePolicy() const;

    virtual void	setBackgroundColor( const QColor & );
    virtual void	setPalette( const QPalette & );
    virtual void	setFont( const QFont & );
    virtual void	setEnabled( bool );

    virtual void	setSizeLimit( int );
    int		sizeLimit() const;

    virtual void	setMaxCount( int );
    int		maxCount() const;

    enum Policy { NoInsertion, AtTop, AtCurrent, AtBottom,
		  AfterCurrent, BeforeCurrent };

    virtual void	setInsertionPolicy( Policy policy );
    Policy 	insertionPolicy() const;

    virtual void	setValidator( const QValidator * );
    const QValidator * validator() const;

    virtual void	setListBox( QListBox * );
    QListBox * 	listBox() const;

    QLineEdit*	lineEdit() const;

    virtual void	setAutoCompletion( bool );
    bool	autoCompletion() const;

    bool	eventFilter( QObject *object, QEvent *event );

    void 	setDuplicatesEnabled( bool enable );
    bool 	duplicatesEnabled() const;
    
    bool 	editable() const;
    void 	setEditable( bool );

public slots:
    void	clearValidator();
    void	clearEdit();
    virtual void	setEditText( const QString &);

signals:
    void	activated( int index );
    void	highlighted( int index );
    void	activated( const QString &);
    void	highlighted( const QString &);
    void	textChanged( const QString &);

private slots:
    void	internalActivate( int );
    void	internalHighlight( int );
    void	internalClickTimeout();
    void	returnPressed();

protected:
    void	paintEvent( QPaintEvent * );
    void	resizeEvent( QResizeEvent * );
    void	mousePressEvent( QMouseEvent * );
    void	mouseMoveEvent( QMouseEvent * );
    void	mouseReleaseEvent( QMouseEvent * );
    void	mouseDoubleClickEvent( QMouseEvent * );
    void	keyPressEvent( QKeyEvent *e );
    void	focusInEvent( QFocusEvent *e );
    void	styleChange( QStyle& );

    void	popup(); //#### virtual in 3.0
    void	updateMask();

private:
    void	setUpListBox();
    void	setUpLineEdit();
    void	popDownListBox();
    void	reIndex();
    void	currentChanged();
    QRect	arrowRect() const;
    bool	getMetrics( int *dist, int *buttonW, int *buttonH ) const;

    QComboData	*d;

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QComboBox( const QComboBox & );
    QComboBox &operator=( const QComboBox & );
#endif
};


#endif // QT_NO_COMBOBOX

#endif // __ATHEOS__

#endif // QCOMBOBOX_H

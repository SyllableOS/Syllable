/****************************************************************************
** $Id$
**
** Definition of the QTextView class
**
** Created : 990101
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

#ifndef QTEXTVIEW_H
#define QTEXTVIEW_H

#ifndef QT_H
#include "qlist.h"
#include "qpixmap.h"
#include "qscrollview.h"
#include "qcolor.h"
#endif // QT_H

#ifndef QT_NO_TEXTVIEW

class QRichText;
class QTextViewData;
class QStyleSheet;
class QMimeSourceFactory;

class Q_EXPORT QTextView : public QScrollView
{
    Q_OBJECT
    Q_PROPERTY( QString text READ text WRITE setText )
    Q_PROPERTY( TextFormat textFormat READ textFormat WRITE setTextFormat )
    Q_PROPERTY( QBrush paper READ paper WRITE setPaper )
    Q_PROPERTY( QColorGroup paperColorGroup READ paperColorGroup WRITE setPaperColorGroup )
    Q_PROPERTY( QColor linkColor READ linkColor WRITE setLinkColor )
    Q_PROPERTY( bool linkUnderline READ linkUnderline WRITE setLinkUnderline )
    Q_PROPERTY( QString documentTitle READ documentTitle )

public:
    QTextView(QWidget *parent=0, const char *name=0);
    QTextView( const QString& text, const QString& context = QString::null,
	       QWidget *parent=0, const char *name=0);
    ~QTextView();

    virtual void setText( const QString& text, const QString& context  );
    void setText( const QString& text ); // write function for 'text' property
    virtual QString text() const;
    virtual QString context() const;

    TextFormat textFormat() const;
    void setTextFormat( TextFormat );


    QStyleSheet* styleSheet() const;
    void setStyleSheet( QStyleSheet* styleSheet );


    // convenience functions
    void setPaper( const QBrush& pap);
    // ##### This non const thing is obsolete
    const QBrush& paper();
    const QBrush& paper() const;

    void setPaperColorGroup( const QColorGroup& colgrp);
    const QColorGroup &paperColorGroup() const;

    void setLinkColor( const QColor& );
    const QColor& linkColor() const;

    void setLinkUnderline( bool );
    bool linkUnderline() const;

    void setMimeSourceFactory( QMimeSourceFactory* factory );
    QMimeSourceFactory* mimeSourceFactory() const;

    QString documentTitle() const;

    int heightForWidth( int w ) const;

    void append( const QString& text );

    bool hasSelectedText() const;
    QString selectedText() const;

public slots:
#ifndef QT_NO_CLIPBOARD
   void copy();
#endif
   void selectAll();


protected:
    void drawContentsOffset(QPainter*, int ox, int oy,
			    int cx, int cy, int cw, int ch);
    void resizeEvent(QResizeEvent*);
    void viewportResizeEvent(QResizeEvent*);
    void viewportMousePressEvent( QMouseEvent* );
    void viewportMouseReleaseEvent( QMouseEvent* );
    void viewportMouseMoveEvent( QMouseEvent* );
    void keyPressEvent( QKeyEvent * );
    void showEvent( QShowEvent* );
    void focusInEvent( QFocusEvent * );
    void focusOutEvent( QFocusEvent * );

protected:

    QRichText& richText() const;
    void paletteChange( const QPalette & );

private slots:
    void doResize();
    void clipboardChanged();
#ifndef QT_NO_DRAGANDDROP
    void doStartDrag();
#endif
    void doAutoScroll();

private:
    void init();
    void createRichText();
    friend class QRichText;
    QTextViewData* d;
    void updateLayout();
    void clearSelection();
    void doSelection( const QPoint& );

    // something like this will be public in a later version
    QString selectedRichTextInternal() const;

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QTextView( const QTextView & );
    QTextView& operator=( const QTextView & );
#endif
};

#endif // QT_NO_TEXTVIEW

#endif // QTEXTVIEW_H

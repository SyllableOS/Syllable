/****************************************************************************
** $Id$
**
** Definition of QProgressBar class
**
** Created : 970520
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

#ifndef QPROGRESSBAR_H
#define QPROGRESSBAR_H

#ifndef QT_H
#include "qframe.h"
#endif // QT_H

#ifndef QT_NO_PROGRESSBAR


class QProgressBarPrivate;


class Q_EXPORT QProgressBar : public QFrame
{
    Q_OBJECT
    Q_PROPERTY( int totalSteps READ totalSteps WRITE setTotalSteps )
    Q_PROPERTY( int progress READ progress WRITE setProgress )
    Q_PROPERTY( bool centerIndicator READ centerIndicator WRITE setCenterIndicator )
    Q_PROPERTY( bool indicatorFollowsStyle READ indicatorFollowsStyle WRITE setIndicatorFollowsStyle )
	
public:
    QProgressBar( QWidget *parent=0, const char *name=0, WFlags f=0 );
    QProgressBar( int totalSteps, QWidget *parent=0, const char *name=0,
		  WFlags f=0 );

    int		totalSteps() const;
    int		progress()   const;

    QSize	sizeHint() const;
    QSizePolicy sizePolicy() const;
    QSize	minimumSizeHint() const;

    void	setCenterIndicator( bool on );
    bool	centerIndicator() const;

    void        setIndicatorFollowsStyle( bool );
    bool	indicatorFollowsStyle() const;

    void	show();

public slots:
    void	reset();
    virtual void setTotalSteps( int totalSteps );
    virtual void setProgress( int progress );

protected:
    void	drawContents( QPainter * );
    void	drawContentsMask( QPainter * );
    virtual bool setIndicator( QString & progress_str, int progress,
			       int totalSteps );
    void styleChange( QStyle& );

private:
    int		total_steps;
    int		progress_val;
    int		percentage;
    QString	progress_str;
    bool        center_indicator;
    bool        auto_indicator;
    QProgressBarPrivate * d;
    void         initFrame();

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QProgressBar( const QProgressBar & );
    QProgressBar &operator=( const QProgressBar & );
#endif
};


inline int QProgressBar::totalSteps() const
{
    return total_steps;
}

inline int QProgressBar::progress() const
{
    return progress_val;
}

inline bool QProgressBar::centerIndicator() const
{
    return center_indicator;
}

inline bool QProgressBar::indicatorFollowsStyle() const
{
    return auto_indicator;
}


#endif // QT_NO_PROGRESSBAR

#endif // QPROGRESSBAR_H

/****************************************************************************
** $Id$
**
** Definition of QFontInfo class
**
** Created : 950131
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

#ifndef QFONTINFO_H
#define QFONTINFO_H

#ifdef __ATHEOS__
#include "../../konq-embed/dropin/atheos/afontinfo.h"
#else

#ifndef QT_H
#include "qfont.h"
#endif // QT_H


class Q_EXPORT QFontInfo
{
public:
    QFontInfo( const QFont & );
    QFontInfo( const QFontInfo & );
   ~QFontInfo();

    QFontInfo	       &operator=( const QFontInfo & );

    QString   	        family()	const;
    int			pointSize()	const;
    bool		italic()	const;
    int			weight()	const;
    bool		bold()		const;
    bool		underline()	const;
    bool		strikeOut()	const;
    bool		fixedPitch()	const;
    QFont::StyleHint	styleHint()	const;
    QFont::CharSet	charSet()	const;
    bool		rawMode()	const;

    bool		exactMatch()	const;

#if 1	/* OBSOLETE */
    const QFont &font() const;
#endif

private:
    QFontInfo( const QPainter * );
    static void reset( const QPainter * );
    const QFontDef *spec() const;

    QFontInternal *fin;
    QPainter      *painter;
    int		   flags;

    bool    underlineFlag()  const { return (flags & 0x1) != 0; }
    bool    strikeOutFlag()  const { return (flags & 0x2) != 0; }
    bool    exactMatchFlag() const { return (flags & 0x4) != 0; }
    void    setUnderlineFlag()	   { flags |= 0x1; }
    void    setStrikeOutFlag()	   { flags |= 0x2; }
    void    setExactMatchFlag()	   { flags |= 0x4; }

    friend class QWidget;
    friend class QPainter;
};


inline bool QFontInfo::bold() const
{ return weight() > QFont::Normal; }

#endif // __ATHEOS__

#endif // QFONTINFO_H

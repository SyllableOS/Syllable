/****************************************************************************
** $Id$
**
** Definition of internal QFontData struct
**
** Created : 941229
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

#ifndef QFONTDATA_P_H
#define QFONTDATA_P_H


//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qmenudata.cpp, qmenubar.cpp, qmenubar.cpp, qpopupmenu.cpp,
// qmotifstyle.cpp and qwindowssstyle.cpp.  This header file may change
// from version to version without notice, or even be removed.
//
// We mean it.
//
//

#ifndef QT_H
#endif // QT_H

struct QFontDef {				// font definition
    QString	family;
    short	pointSize;
    uint	styleHint	: 8;
    uint	styleStrategy	: 8;
    uint	charSet		: 8;
    uint	weight		: 8;
    uint	italic		: 1;
    uint	underline	: 1;
    uint	strikeOut	: 1;
    uint	fixedPitch	: 1;
    uint	hintSetByUser	: 1;
    uint	rawMode		: 1;
    uint	dirty		: 1;
    short	lbearing;
    short	rbearing;
};


class QFontInternal;
class QTextCodec;

struct QFontData : public QShared {
    QFontData()
	: exactMatch(FALSE), fin(0), printerHackFont( 0 )
	{}
    QFontData( const QFontData &d )
	: QShared(d), req(d.req), exactMatch(d.exactMatch), fin(d.fin),
	  printerHackFont(0)
	// Copy the QShared count as well. The count may need to be
	// reset when using the QFontData class, see QFont::QFont(QFontData*)
	{}
   ~QFontData()
	{}
    QFontData &operator=( const QFontData &d )
	{
	    req = d.req;
	    exactMatch = d.exactMatch;
	    fin = d.fin;
	    printerHackFont=d.printerHackFont;
	    return *this;
	}
    QFontDef	      req;			// requested font
    bool	      exactMatch;
    QFontInternal    *fin;
    QFont            *printerHackFont;
    const QTextCodec *mapper()  const;
    void	     *fontSet() const;
};


#endif // QFONTDATA_P_H

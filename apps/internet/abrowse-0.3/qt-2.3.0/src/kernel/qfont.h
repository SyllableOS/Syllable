/****************************************************************************
** $Id$
**
** Definition of QFont class
**
** Created : 940514
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

#ifndef QFONT_H
#define QFONT_H

#ifdef __ATHEOS__
#include "../../konq-embed/dropin/atheos/afont.h"
#else

#ifndef QT_H
#include "qwindowdefs.h"
#include "qstring.h"
#endif // QT_H


class QStringList;
struct QFontDef;
struct QFontData;
class QFontInternal;
class QRenderedFont;

class Q_EXPORT QFont					// font class
{
public:
    enum CharSet   { ISO_8859_1,  Latin1 = ISO_8859_1, AnyCharSet,
		     ISO_8859_2,  Latin2 = ISO_8859_2,
		     ISO_8859_3,  Latin3 = ISO_8859_3,
		     ISO_8859_4,  Latin4 = ISO_8859_4,
		     ISO_8859_5,
		     ISO_8859_6,
		     ISO_8859_7,
		     ISO_8859_8,
		     ISO_8859_9,  Latin5 = ISO_8859_9,
		     ISO_8859_10, Latin6 = ISO_8859_10,
		     ISO_8859_11, TIS620 = ISO_8859_11,
		     ISO_8859_12,
		     ISO_8859_13, Latin7 = ISO_8859_13,
		     ISO_8859_14, Latin8 = ISO_8859_14,
		     ISO_8859_15, Latin9 = ISO_8859_15,
		     KOI8R,
		     Set_Ja, Set_1 = Set_Ja,
		     Set_Ko,
		     Set_Th_TH,
		     Set_Zh,
		     Set_Zh_TW,
		     Set_N = Set_Zh_TW,
		     Unicode,
		     /* The following will need to be re-ordered later,
			since we accidentally left no room below "Unicode".
		        (For binary-compatibility that cannot change yet).
			The above will be obsoleted and a same-named list
			added below.
		     */
		     Set_GBK,
		     Set_Big5,

		     TSCII,
		     KOI8U,
		     CP1251,
		     PT154,
		     /* The following are font-specific encodings that
			we shouldn't need in a perfect world.
		     */
		     // 8-bit fonts
		     JIS_X_0201 = 0xa0,
		     // 16-bit fonts
		     JIS_X_0208 = 0xc0, Enc16 = JIS_X_0208,
		     KSC_5601,
		     GB_2312,
		     Big5
    };
    enum StyleHint { Helvetica, Times, Courier, OldEnglish,  System, AnyStyle,
		     SansSerif	= Helvetica,
		     Serif	= Times,
		     TypeWriter = Courier,
		     Decorative = OldEnglish};
    enum StyleStrategy { PreferDefault = 0x0001,
			  PreferBitmap = 0x0002,
			  PreferDevice = 0x0004,
			  PreferOutline = 0x0008,
			  ForceOutline = 0x0010,
			  PreferMatch = 0x0020,
			  PreferQuality = 0x0040 };
    enum Weight	   { Light = 25, Normal = 50, DemiBold = 63,
		     Bold  = 75, Black	= 87 };
    QFont();					// default font
    QFont( const QString &family, int pointSize = 12,
	   int weight = Normal, bool italic = FALSE );
    QFont( const QString &family, int pointSize,
	   int weight, bool italic, CharSet charSet );
    QFont( const QFont & );
    ~QFont();
    QFont      &operator=( const QFont & );

    QString	family()	const;
    void	setFamily( const QString &);
    int		pointSize()	const;
    float	pointSizeFloat()	const;
    void	setPointSize( int );
    void	setPointSizeFloat( float );
    int		pixelSize() const;
    void	setPixelSize( int );
    void	setPixelSizeFloat( float );
    int		weight()	const;
    void	setWeight( int );
    bool	bold()		const;
    void	setBold( bool );
    bool	italic()	const;
    void	setItalic( bool );
    bool	underline()	const;
    void	setUnderline( bool );
    bool	strikeOut()	const;
    void	setStrikeOut( bool );
    bool	fixedPitch()	const;
    void	setFixedPitch( bool );
    StyleHint	styleHint()	const;
    void	setStyleHint( StyleHint );
    StyleStrategy styleStrategy() const;
    void	setStyleHint( StyleHint, StyleStrategy );
    CharSet	charSet()	const;
    void	setCharSet( CharSet );

    static CharSet charSetForLocale();

    bool	rawMode()      const;
    void	setRawMode( bool );

    bool	exactMatch()	const;

    bool	operator==( const QFont & ) const;
    bool	operator!=( const QFont & ) const;
    bool	isCopyOf( const QFont & ) const;

#if defined(_WS_WIN_)
    HFONT	handle() const;
#elif defined(_WS_MAC_)
    HANDLE      handle() const;
#elif defined(_WS_X11_)
    HANDLE	handle() const;
#elif defined(_WS_QWS_)
    HANDLE	handle() const;
#endif

    void	setRawName( const QString & );
    QString	rawName() const;

    QString	key() const;

    static QString encodingName( CharSet );

    static QFont defaultFont();
    static void setDefaultFont( const QFont & );

    static QString substitute( const QString &familyName );
    static void insertSubstitution( const QString&, const QString &);
    static void removeSubstitution( const QString &);
    static QStringList substitutions();

    static void initialize();
    static void locale_init();
    static void cleanup();
    static void cacheStatistics();

#if defined(_WS_QWS_)
    void qwsRenderToDisk(bool all=TRUE);
#endif

protected:
    bool	dirty()			const;

    QString	defaultFamily()		const;
    QString	lastResortFamily()	const;
    QString	lastResortFont()	const;
    int		deciPointSize()		const;

private:
    QFont( QFontData * );
    void	init();
    void	detach();
    void	initFontInfo() const;
    void	load() const;
#if defined(_WS_MAC_)
    void        macSetFont(void *);
#endif
#if defined(_WS_WIN_)
    HFONT	create( bool *, HDC=0, bool=FALSE ) const;
    void       *textMetric() const;
#endif

    friend class QFontInternal;
    friend class QFontMetrics;
    friend class QFontInfo;
    friend class QPainter;
#if defined(_WS_X11_) && defined(QT_XFT)
    friend void * qt_ft_font (const QFont *f);
#endif

#ifndef QT_NO_DATASTREAM
    friend Q_EXPORT QDataStream &operator<<( QDataStream &, const QFont & );
    friend Q_EXPORT QDataStream &operator>>( QDataStream &, QFont & );
#endif
    QFontData	 *d;				// internal font data
    static CharSet defaultCharSet;
};

inline bool QFont::bold() const
{ return weight() > Normal; }

inline void QFont::setBold( bool enable )
{ setWeight( enable ? Bold : Normal ); }


/*****************************************************************************
  QFont stream functions
 *****************************************************************************/

#ifndef QT_NO_DATASTREAM
Q_EXPORT QDataStream &operator<<( QDataStream &, const QFont & );
Q_EXPORT QDataStream &operator>>( QDataStream &, QFont & );
#endif

#endif // __ATHEOS__

#endif // QFONT_H

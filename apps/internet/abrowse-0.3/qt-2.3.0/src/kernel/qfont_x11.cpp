/****************************************************************************
** $Id$
**
** Implementation of QFont, QFontMetrics and QFontInfo classes for X11
**
** Created : 940515
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
** licenses for Unix/X11 may use this file in accordance with the Qt Commercial
** License Agreement provided with the Software.
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
#define DEBUG
// REVISED: arnt

#include "qwidget.h"
#include "qpainter.h"
#include "qfontdata_p.h"
#include "qfontdatabase.h"
#include "qstrlist.h"
#include "qcache.h"
#include "qdict.h"
#include "qtextcodec.h"
#include "qapplication.h"
#include "qfile.h"
#include "qtextstream.h"
#include "qdir.h" // Font Guessing
#include <ctype.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "qt_x11.h"
#include "qmap.h"

#include <netinet/in.h>

// NOT REVISED

/* UNICODE:
     XLFD names are defined to be Latin1.
*/

// Font Guessing
struct FontGuessingPair {
  QStringList charset;
  QStringList family;
};

#define FONT_GUESSING_FILE "/etc/qt.fontguess"

QList<FontGuessingPair> *fontGuessingList=0;
//

static const int fontFields = 14;

enum FontFieldNames {				// X LFD fields
    Foundry,
    Family,
    Weight_,
    Slant,
    Width,
    AddStyle,
    PixelSize,
    PointSize,
    ResolutionX,
    ResolutionY,
    Spacing,
    AverageWidth,
    CharsetRegistry,
    CharsetEncoding };

// Internal functions

bool	qParseXFontName( QCString &fontName, char **tokens );
static char   **getXFontNames( const char *pattern, int *count );
static bool	fontExists( const QString &fontName );
int	qFontGetWeight( const QCString &weightString, bool adjustScore=FALSE );


#undef	IS_ZERO
#define IS_ZERO(X) (X[0] == '0' && X[1] == 0)

static
inline bool isScalable( char **tokens )
{
    return ( IS_ZERO(tokens[PixelSize]) &&
	     IS_ZERO(tokens[PointSize]) &&
	     IS_ZERO(tokens[AverageWidth]) );
}

static
inline bool isSmoothlyScalable( char **tokens )
{
    return ( IS_ZERO( tokens[ResolutionX] ) &&
	     IS_ZERO( tokens[ResolutionY] ) );
}

// QFont_Private accesses QFont protected functions

class QFont_Private : public QFont
{
public:
    int	    fontMatchScore( char *fontName, QCString &buffer,
			    float *pointSizeDiff, int *weightDiff,
			    bool *scalable, bool *smoothScalable );
	// Font Guessing
    bool fontmapping(const QString& filename);
    QCString bestMatchFontSetMember( const QString& family,
		const char *wt, const char *slant, int size, int xdpi, int ydpi );
    //
    QCString bestMatch( const char *pattern, int *score );
    QCString bestFamilyMember( const QString& foundry,
			       const QString& family, int *score );
    QCString findFont( bool *exact );
    bool needsSet() const { return (charSet() >= Set_1 && charSet() <= Set_N)
	    || charSet() == Set_GBK || charSet() == Set_Big5; }

};

#undef  PRIV
#define PRIV ((QFont_Private*)this)


/*****************************************************************************
  QFontInternal contains X11 font data: an XLFD font name ("-*-*-..") and
  an X font struct.

  Two global dictionaries and a cache hold QFontInternal objects, which
  are shared between all QFonts.
  This mechanism makes font loading much faster than using XLoadQueryFont.
 *****************************************************************************/

class QFontInternal
{
public:
   ~QFontInternal();
    bool	    dirty() const;
    const char	   *name()  const;
    int		    xResolution() const;
#ifdef QT_XFT
    XftFont	    *ftfont() const;
#endif
    XFontStruct	   *fontStruct() const;
    XFontSet	    fontSet() const;
    const QFontDef *spec()  const;
    int		    lineWidth() const;
    const QTextCodec *mapper() const;
    void	    reset();
private:
    QFontInternal( const QString & );
    void computeLineWidth();

    QCString	    n;
    XFontStruct	   *f;
    XFontSet	    set;
#ifdef QT_XFT
    XftFont	    *ft;
#endif
    QFontDef	    s;
    int		    lw;
    int		    xres;
    QTextCodec	   *cmapper;
    friend void QFont::load() const;
    friend void QFont::initFontInfo() const;
};

inline QFontInternal::QFontInternal( const QString &name )
    : n(name.ascii()), f(0), set(0), cmapper(0)
{
    f = 0;
#ifdef QT_XFT
    ft = 0;
#endif
    s.dirty = TRUE;
}

inline bool QFontInternal::dirty() const
{
    return f == 0 && set == 0;
}

inline const char *QFontInternal::name() const
{
    return n;
}

#ifdef QT_XFT
inline XftFont *QFontInternal::ftfont() const
{
    return ft;
}
#endif

inline XFontStruct *QFontInternal::fontStruct() const
{
    return f;
}

inline XFontSet QFontInternal::fontSet() const
{
    return set;
}

inline const QFontDef *QFontInternal::spec() const
{
    return &s;
}

inline int QFontInternal::lineWidth() const
{
    return lw;
}

inline int QFontInternal::xResolution() const
{
    return xres;
}

inline const QTextCodec *QFontInternal::mapper() const
{
    return cmapper;
}

inline void QFontInternal::reset()
{
    if ( f ) {
	XFreeFont( QPaintDevice::x11AppDisplay(), f );
	f = 0;
    }
    if ( set ) {
	XFreeFontSet( QPaintDevice::x11AppDisplay(), set );
	set = 0;
    }
}

inline QFontInternal::~QFontInternal()
{
    reset();
}


static const int fontCacheSize = 1024;


typedef QCacheIterator<QFontInternal> QFontCacheIt;
typedef QDict<QFontInternal>	      QFontDict;
typedef QDictIterator<QFontInternal>  QFontDictIt;


class QFontCache : public QCache<QFontInternal>
{
public:
    QFontCache( int maxCost, int size=17 )
	: QCache<QFontInternal>(maxCost,size) {}
    ~QFontCache() { clear(); }
    void deleteItem( Item );
};

void QFontCache::deleteItem( Item d )
{
    QFontInternal *fin = (QFontInternal *)d;
    fin->reset();
}


struct QXFontName
{
    QXFontName( const QCString &n, bool e ) : name(n), exactMatch(e) {}
    QCString name;
    bool    exactMatch;
};

typedef QDict<QXFontName> QFontNameDict;

static QFontCache    *fontCache	     = 0;	// cache of loaded fonts
static QFontDict     *fontDict	     = 0;	// dict of all loaded fonts
static QFontNameDict *fontNameDict   = 0;	// dict of matched font names
						// default character set:
QFont::CharSet QFont::defaultCharSet = QFont::AnyCharSet;

//
// This function returns the X font struct for a QFontData.
// It is called from QPainter::drawText().
//

XFontStruct *qt_get_xfontstruct( QFontData *d )
{
    return d->fin->fontStruct();
}


/*****************************************************************************
  QFont member functions
 *****************************************************************************/

/*****************************************************************************
  set_local_font() - tries to set a sensible default font char set
 *****************************************************************************/

/* This will go away - we'll just use codecForLocale() */
static struct {
    const char * name;
    QFont::CharSet cs;
} encoding_names[] = {
    { "ISO 8859-1", QFont::ISO_8859_1 },
    { "ISO 8859-2", QFont::ISO_8859_2 },
    { "ISO 8859-3", QFont::ISO_8859_3 },
    { "ISO 8859-4", QFont::ISO_8859_4 },
    { "ISO 8859-5", QFont::ISO_8859_5 },
    { "ISO 8859-6", QFont::ISO_8859_6 },
    { "ISO 8859-7", QFont::ISO_8859_7 },
    { "ISO 8859-8-I", QFont::ISO_8859_8 },
    { "ISO 8859-9", QFont::ISO_8859_9 },
    { "ISO 8859-10", QFont::ISO_8859_10 },
    { "ISO 8859-11", QFont::ISO_8859_11 },
    { "ISO 8859-12", QFont::ISO_8859_12 },
    { "ISO 8859-13", QFont::ISO_8859_13 },
    { "ISO 8859-14", QFont::ISO_8859_14 },
    { "ISO 8859-15", QFont::ISO_8859_15 },
    { "KOI8-R", QFont::KOI8R },
    { "eucJP", QFont::Set_Ja },
    { "SJIS", QFont::Set_Ja },
    { "JIS7", QFont::Set_Ja },
    { "eucKR", QFont::Set_Ko },
    { "TACTIS", QFont::Set_Th_TH },
    { "GBK", QFont::Set_GBK },
    { "zh_CN.GBK", QFont::Set_GBK },
    { "eucCN", QFont::Set_Zh },
    { "eucTW", QFont::Set_Zh_TW },
    { "zh_TW.Big5", QFont::Set_Big5 },
    { "Big5", QFont::Set_Big5 },
    { "ta_TA.TSCII", QFont::TSCII },
    { "TSCII", QFont::TSCII },
    { "KOI8-U", QFont::KOI8U },
    { "CP 1251", QFont::CP1251 },
    { "PT 154", QFont::PT154 },
    { 0, /* anything */ QFont::ISO_8859_1 }
};


/*!
  Internal function that uses locale information to find the preferred
  character set of loaded fonts.

  \internal
  Uses QTextCodec::codecForLocale() to find the character set name.
*/
void QFont::locale_init()
{
    QTextCodec * t = QTextCodec::codecForLocale();
    const char * p = t ? t->name() : 0;
    if ( p && *p ) {
	int i=0;
	while( encoding_names[i].name &&
	       qstricmp( p, encoding_names[i].name ) )
	    i++;
	if ( encoding_names[i].name ) {
	    defaultCharSet = encoding_names[i].cs;
	    return;
	}
    }
    defaultCharSet = QFont::Latin1;
}

/*!
  Internal function that initializes the font system.

  \internal
  The font cache and font dict do not alloc the keys. The key is a QString
  which is shared between QFontInternal and QXFontName.
*/

void QFont::initialize()
{
    if ( fontCache )
	return;
    fontCache = new QFontCache( fontCacheSize, 29 );
    CHECK_PTR( fontCache );
    fontDict  = new QFontDict( 29 );
    CHECK_PTR( fontDict );
    fontNameDict = new QFontNameDict( 29 );
    CHECK_PTR( fontNameDict );
    fontNameDict->setAutoDelete( TRUE );

    QString vendorstring(ServerVendor(QPaintDevice::x11AppDisplay()));
    int vendorrelease = VendorRelease(QPaintDevice::x11AppDisplay());

    if (vendorstring.lower().contains("xfree86") &&
	vendorrelease < 4000) {
    }
}

/*!
  Internal function that cleans up the font system.
*/

void QFont::cleanup()
{
    delete fontCache;
    fontCache = 0;
    if ( fontDict )
	fontDict->setAutoDelete( TRUE );
    delete fontDict;
    fontDict = 0;
    delete fontNameDict;
    fontNameDict = 0;
}

/*!
  Internal function that dumps font cache statistics.
*/

void QFont::cacheStatistics()
{
#if defined(DEBUG)
    fontCache->statistics();
    QFontCacheIt it(*fontCache);
    QFontInternal *fin;
    qDebug( "{" );
    while ( (fin = it.current()) ) {
	++it;
	qDebug( "   [%s]", fin->name() );
    }
    qDebug( "}" );
#endif
}

/* Clears the internal cache of mappings from QFont instances to X11
    (XLFD) font names. Called from QPaintDevice::x11App
*/
void qX11ClearFontNameCache()
{
    if ( fontNameDict )
	fontNameDict->clear();
}

// If d->req.dirty is not TRUE the font must have been loaded
// and we can safely assume that d->fin is a valid pointer:

#define DIRTY_FONT (d->req.dirty || d->fin->dirty())


/*!
  Returns the window system handle to the font, for low-level
  access.  Using this function is \e not portable.
*/

HANDLE QFont::handle() const
{
    static Font last = 0;
    if ( DIRTY_FONT ) {
	load();
	if ( d->fin->fontSet() )
	    return 0;
    } else {
	if ( d->fin->fontSet() )
	    return 0;
	if ( d->fin->fontStruct () && d->fin->fontStruct()->fid != last )
	    fontCache->find( d->fin->name() );
    }

    if (d->fin->fontStruct())
    last = d->fin->fontStruct()->fid;
    else
	last = 1;
    return last;
}

#ifdef QT_XFT
void *qt_ft_font (const QFont *f)
{
    if (!f->handle())
	return 0;

    return f->d->fin->ftfont();
}
#endif

// ### maybe reorder the CharSet enum and get rid of this crap?
static QFont::CharSet c8859[] = { QFont::ISO_8859_1,  QFont::ISO_8859_2,
				  QFont::ISO_8859_3,  QFont::ISO_8859_4,
				  QFont::ISO_8859_5,  QFont::ISO_8859_6,
				  QFont::ISO_8859_7,  QFont::ISO_8859_8,
				  QFont::ISO_8859_9,  QFont::ISO_8859_10,
				  QFont::ISO_8859_11,  QFont::ISO_8859_12,
				  QFont::ISO_8859_13, QFont::ISO_8859_14,
				  QFont::ISO_8859_15 };

/*
  Fills in a font definition (QFontDef) from an XLFD (X Logical Font
  Description). Returns TRUE if the the given xlfd is valid. If the xlfd
  is valid the encoding name (charset registry + "-" + charset encoding)
  is returned in /e encodingName if /e encodingName is non-zero. The
  fileds lbearing and rbearing are not given any values.
 */

static bool fillFontDef( const QCString &xlfd, QFontDef *fd,
			 QCString *encodingName )
{

    char *tokens[fontFields];
    QCString buffer = xlfd;
    if ( !qParseXFontName(buffer, tokens) )
	return FALSE;

    if ( encodingName ) {
	*encodingName = tokens[CharsetRegistry];
	*encodingName += '-';
	*encodingName += tokens[CharsetEncoding];
    }

    fd->family = QString::fromLatin1(tokens[Family]);
    fd->pointSize = atoi(tokens[PointSize]);
    fd->styleHint = QFont::AnyStyle;	// ### any until we match families

    if ( qstrcmp( tokens[CharsetRegistry], "iso8859" ) == 0 ) {
	int tmp = 999;
	if ( sscanf( tokens[CharsetEncoding], "%u", &tmp ) == 1 &&
	     tmp >= 1 && tmp <= 15 ) {
	    fd->charSet = c8859[tmp-1];
	    if ( encodingName && ( tmp == 8 || tmp == 6 ) ) { // hebrew and arabic
		*encodingName += "-I";
	    }
	}
    } else if( qstrcmp( tokens[CharsetRegistry], "koi8" ) == 0 &&
	       (qstrcmp( tokens[CharsetEncoding], "r" ) == 0 ||
		qstrcmp( tokens[CharsetEncoding], "1" ) == 0) ) {
	fd->charSet = QFont::KOI8R;
    } else if( strcmp( tokens[CharsetRegistry], "koi8" ) == 0 &&
	       strcmp( tokens[CharsetEncoding], "u" ) == 0) {
	fd->charSet = QFont::KOI8U;
    } else if( qstrcmp( tokens[CharsetRegistry], "tscii" ) == 0 &&
	       qstrcmp( tokens[CharsetEncoding], "0" ) == 0 ) {
	fd->charSet = QFont::TSCII;
    } else if( qstrcmp( tokens[CharsetRegistry], "tis620" ) == 0 ) {
        // tis620 and latin11 are the same
        fd->charSet = QFont::ISO_8859_11;
    } else if( qstrcmp( tokens[CharsetRegistry], "iso10646" ) == 0 ) {
	fd->charSet = QFont::Unicode;
    } else if( qstrncmp( tokens[CharsetRegistry], "jisx0201.", 9 ) == 0 ) {
	fd->charSet = QFont::JIS_X_0201;
    } else if( qstrncmp( tokens[CharsetRegistry], "jisx0208.", 9 ) == 0 ) {
	fd->charSet = QFont::JIS_X_0208;
    } else if( qstrncmp( tokens[CharsetRegistry], "ksc5601.", 8 ) == 0 ) {
	fd->charSet = QFont::KSC_5601;
    } else if( qstrncmp( tokens[CharsetRegistry], "gb2312.", 7 ) == 0 ) {
	fd->charSet = QFont::GB_2312;
    } else if( qstrncmp( tokens[CharsetRegistry], "big5", 4 ) == 0 ) {
	fd->charSet = QFont::Big5;
    } else if( qstrcmp( tokens[CharsetEncoding], "cp1251" ) == 0 ||
  	       (qstrcmp( tokens[CharsetEncoding], "1251" ) == 0 ) ) {
  	fd->charSet = QFont::CP1251;
    } else if( qstrcmp( tokens[CharsetEncoding], "cp154" ) == 0 ||
  	       (qstrcmp( tokens[CharsetEncoding], "154" ) == 0 ) ) {
  	fd->charSet = QFont::PT154;
    } else {
	fd->charSet = QFont::AnyCharSet;
    }

    char slant	= tolower( tokens[Slant][0] );
    fd->italic = (slant == 'o' || slant == 'i');
    char fixed	= tolower( tokens[Spacing][0] );
    fd->fixedPitch = (fixed == 'm' || fixed == 'c');
    fd->weight = qFontGetWeight( tokens[Weight_] );

#if 1
    int r = atoi(tokens[ResolutionY]);
    if ( r && QPaintDevice::x11AppDpiY() && r != QPaintDevice::x11AppDpiY() ) { // not "0" or "*", or required DPI
	// calculate actual pointsize for display DPI
	fd->pointSize = ( 2*fd->pointSize*atoi(tokens[ResolutionY])
			  + QPaintDevice::x11AppDpiY()
			  ) / (QPaintDevice::x11AppDpiY() * 2);
    }
#endif

    fd->underline     = FALSE;
    fd->strikeOut     = FALSE;
    fd->hintSetByUser = FALSE;
    fd->rawMode       = FALSE;
    fd->dirty         = FALSE;
    return TRUE;
}

/*!
  Returns the name of the font within the underlying window system.
  On Windows, this is usually just the family name of a true type
  font. Under X, it is a rather complex XLFD (X Logical Font
  Description). Using the return value of this function is usually \e
  not \e portable.

  \sa setRawName()
*/
QString QFont::rawName() const
{
    if ( DIRTY_FONT )
	load();
    return QString::fromLatin1(d->fin->name());
}

/*!
  Sets a font by its system specific name. The function is in
  particular useful under X, where system font settings ( for example
  X resources) are usually available as XLFD (X Logical Font
  Description) only. You can pass an XLFD as \a name to this function.

  In Qt 2.0 and later, a font set with setRawName() is still a
  full-featured QFont. It can be queried (for example with italic())
  or modified (for example with setItalic() ) and is therefore also
  suitable as a basis font for rendering rich text.

  If Qt's internal font database cannot resolve the raw name, the font
  becomes a raw font with \a name as family.

  Note that the present implementation does not handle handle
  wildcards in XLFDs well, and that font aliases (file \c fonts.alias
  in the font directory on X11) are not supported.

  \sa rawName(), setRawMode(), setFamily()
*/
void QFont::setRawName( const QString &name )
{
    detach();
    bool validXLFD = fillFontDef( name.latin1(), &d->req, 0 );
    d->req.dirty = TRUE;
    if ( !validXLFD ) {
#if defined(CHECK_STATE)
	qWarning( "QFont::setRawMode(): Invalid XLFD: \"%s\"", name.latin1() );
#endif
	setFamily( name );
	setRawMode( TRUE );
    }
}



/*!
  Returns TRUE if the font attributes have been changed and the font has to
  be (re)loaded, or FALSE if no changes have been made.
*/

bool QFont::dirty() const
{
    return DIRTY_FONT;
}


/*!
  Returns the family name that corresponds to the current style hint.
*/

QString QFont::defaultFamily() const
{
    switch( d->req.styleHint ) {
	case Times:
	    return QString::fromLatin1("times");
	case Courier:
	    return QString::fromLatin1("courier");
	case Decorative:
	    return QString::fromLatin1("old english");
	case Helvetica:
	case System:
	default:
	    return QString::fromLatin1("helvetica");
    }
}


/*!
  Returns a last resort family name for the font matching algorithm.

  \sa lastResortFont()
*/

QString QFont::lastResortFamily() const
{
    return QString::fromLatin1("helvetica");
}


static const char * const tryFonts[] = {
    "6x13",
    "7x13",
    "8x13",
    "9x15",
    "fixed",
    "-*-helvetica-medium-r-*-*-*-120-*-*-*-*-*-*",
    "-*-courier-medium-r-*-*-*-120-*-*-*-*-*-*",
    "-*-times-medium-r-*-*-*-120-*-*-*-*-*-*",
    "-*-lucida-medium-r-*-*-*-120-*-*-*-*-*-*",
    "-*-helvetica-*-*-*-*-*-120-*-*-*-*-*-*",
    "-*-courier-*-*-*-*-*-120-*-*-*-*-*-*",
    "-*-times-*-*-*-*-*-120-*-*-*-*-*-*",
    "-*-lucida-*-*-*-*-*-120-*-*-*-*-*-*",
    "-*-helvetica-*-*-*-*-*-*-*-*-*-*-*-*",
    "-*-courier-*-*-*-*-*-*-*-*-*-*-*-*",
    "-*-times-*-*-*-*-*-*-*-*-*-*-*-*",
    "-*-lucida-*-*-*-*-*-*-*-*-*-*-*-*",
    0 };

/*!
  Returns a last resort raw font name for the font matching algorithm.
  This is used if even the last resort family is not available.  It
  returns \e something, almost no matter what.

  The current implementation tries a wide variety of common fonts,
  returning the first one it finds.  The implementation may change at
  any time.

  \sa lastResortFamily()
*/

QString QFont::lastResortFont() const
{
    static QString last;
    if ( !last.isNull() )			// already found
	return last;
    int i = 0;
    const char* f;
    while ( (f = tryFonts[i]) ) {
	last = QString::fromLatin1(f);
	if ( fontExists(last) ) {
	    return last;
	}
	i++;
    }
#if defined(CHECK_NULL)
    qFatal( "QFont::lastResortFont: Cannot find any reasonable font" );
#endif
    return last;
}


static void resetFontDef( QFontDef *def )	// used by initFontInfo()
{
    def->pointSize     = 0;
    def->styleHint     = QFont::AnyStyle;
    def->weight	       = QFont::Normal;
    def->italic	       = FALSE;
    def->charSet       = QFont::Latin1;
    def->underline     = FALSE;
    def->strikeOut     = FALSE;
    def->fixedPitch    = FALSE;
    def->hintSetByUser = FALSE;
    def->lbearing      = SHRT_MIN;
    def->rbearing      = SHRT_MIN;
}

#ifndef QT_NO_CODECS
#include <qjpunicode.h>

class QFontJis0208Codec : public QTextCodec
{
public:
    QFontJis0208Codec();

    const char* name() const ;
    //       Return the official name for the encoding.
    int mibEnum() const ;
    // Return the MIB enum for the encoding if it is listed in the
    // IANA character-sets encoding file.

    QString toUnicode(const char* chars, int len) const ;
    // Converts len characters from chars to Unicode.
    QCString fromUnicode(const QString& uc, int& lenInOut ) const;
    // Converts lenInOut characters (of type QChar) from the start of
    // the string uc, returning a QCString result, and also returning
    // the length of the result in lenInOut.

    int heuristicContentMatch(const char *, int) const;
private:
    static const QJpUnicodeConv * convJP;
};


int QFontJis0208Codec::heuristicContentMatch(const char *, int) const
{
    return 0;
}

const QJpUnicodeConv * QFontJis0208Codec::convJP;

QFontJis0208Codec::QFontJis0208Codec()
{
    if ( !convJP )
	convJP = QJpUnicodeConv::newConverter(JU_Default);
}

const char* QFontJis0208Codec::name() const
{
    return "JIS_X0208";
}

int QFontJis0208Codec::mibEnum() const
{
    return 63;
}

QString QFontJis0208Codec::toUnicode(const char* /*chars*/, int /*len*/) const
{
    return QString(); //###
}

QCString QFontJis0208Codec::fromUnicode(const QString& uc, int& lenInOut ) const
{
    QCString result;
    for ( int i = 0; i < lenInOut; i++ ) {
	QChar ch = uc[i];
	if ( ch.row() == 0) {
	    if ( ch.cell() == ' ' )
		ch = QChar( 0x3000 );
	    else if ( ch.cell() == '"' )
		ch = QChar( 0x2033 );
	    else if ( ch.cell() == '\'' )
		ch = QChar( 0x2032 );
	    else if ( ch.cell() == '-' )
		ch = QChar( 0x2212 );
	    else if ( ch.cell() == '~' )
		ch = QChar( 0x301c );
	    else if ( ch.cell() > ' ' && ch.cell() < 127 )
		ch = QChar( ch.cell()-' ', 255 );
	}
	ch = convJP->UnicodeToJisx0208( ch.unicode());
	if ( !ch.isNull() ) {
	    result += ch.row();
	    result += ch.cell();
	} else {
	    //black square
	    result += 0x22;
	    result += 0x23;
	}
    }
    lenInOut *=2;
    return result;
}



extern unsigned int qt_UnicodeToKsc5601(unsigned int unicode);


class QFontKsc5601Codec : public QTextCodec
{
public:
    QFontKsc5601Codec();

    const char* name() const ;
    //       Return the official name for the encoding.
    int mibEnum() const ;
    // Return the MIB enum for the encoding if it is listed in the
    // IANA character-sets encoding file.

    QString toUnicode(const char* chars, int len) const ;
    // Converts len characters from chars to Unicode.
    QCString fromUnicode(const QString& uc, int& lenInOut ) const;
    // Converts lenInOut characters (of type QChar) from the start of
    // the string uc, returning a QCString result, and also returning
    // the length of the result in lenInOut.

    int heuristicContentMatch(const char *, int) const;
};


int QFontKsc5601Codec::heuristicContentMatch(const char *, int) const
{
    return 0;
}

QFontKsc5601Codec::QFontKsc5601Codec()
{
}

const char* QFontKsc5601Codec::name() const
{
    return "KSC_5601";
}

int QFontKsc5601Codec::mibEnum() const
{
    return 63;
}

QString QFontKsc5601Codec::toUnicode(const char* /*chars*/, int /*len*/) const
{
    return QString(); //###
}

QCString QFontKsc5601Codec::fromUnicode(const QString& uc, int& lenInOut ) const
{
    QCString result;
    for ( int i = 0; i < lenInOut; i++ ) {
	QChar ch = uc[i];
	if ( ch.row() == 0) {
	    if ( ch.cell() == ' ' )
		ch = QChar( 0x3000 );
	    else if ( ch.cell() > ' ' && ch.cell() < 127 )
		ch = QChar( ch.cell()-' ', 255 );
	}
	ch = QChar( qt_UnicodeToKsc5601(ch.unicode()) );

	if ( ch.row() > 0 && ch.cell() > 0  ) {
	    result += ch.row() & 0x7f ;
	    result += ch.cell() & 0x7f;
	} else {
	    //black square
	    result += 0x21;
	    result += 0x61;
	}
    }
    lenInOut *=2;
    return result;
}


/********


Name: GB_2312-80                                        [RFC1345,KXS2]
MIBenum: 57
Source: ECMA registry
Alias: iso-ir-58
Alias: chinese
Alias: csISO58GB231280

*/


extern unsigned int qt_UnicodeToGBK(unsigned int code);



class QFontGB2312Codec : public QTextCodec
{
public:
    QFontGB2312Codec();

    const char* name() const ;
    //       Return the official name for the encoding.
    int mibEnum() const ;
    // Return the MIB enum for the encoding if it is listed in the
    // IANA character-sets encoding file.

    QString toUnicode(const char* chars, int len) const ;
    // Converts len characters from chars to Unicode.
    QCString fromUnicode(const QString& uc, int& lenInOut ) const;
    // Converts lenInOut characters (of type QChar) from the start of
    // the string uc, returning a QCString result, and also returning
    // the length of the result in lenInOut.

    int heuristicContentMatch(const char *, int) const;
};


int QFontGB2312Codec::heuristicContentMatch(const char *, int) const
{
    return 0;
}

QFontGB2312Codec::QFontGB2312Codec()
{
}

const char* QFontGB2312Codec::name() const
{
    return "GB_2312";
}

int QFontGB2312Codec::mibEnum() const
{
    return 57;
}

QString QFontGB2312Codec::toUnicode(const char* /*chars*/, int /*len*/) const
{
    return QString(); //###
}

QCString QFontGB2312Codec::fromUnicode(const QString& uc, int& lenInOut ) const
{
    QCString result;
    for ( int i = 0; i < lenInOut; i++ ) {
	QChar ch = uc[i];
	if ( ch.row() == 0) {
	    if ( ch.cell() == ' ' )
		ch = QChar( 0x3000 );
	    else if ( ch.cell() > ' ' && ch.cell() < 127 )
		ch = QChar( ch.cell()-' ', 255 );
	}
	ch = QChar( qt_UnicodeToGBK(ch.unicode()) );

	if ( ch.row() > 0xa0 && ch.cell() > 0xa0  ) {
	    result += ch.row() & 0x7f ;
	    result += ch.cell() & 0x7f;
	} else {
	    //black square
	    result += 0x21;
	    result += 0x76;
	}
    }
    lenInOut *=2;
    return result;
}



extern unsigned int qt_UnicodeToBig5(unsigned int unicode);

class QFontBig5Codec : public QTextCodec
{
public:
    QFontBig5Codec();

    const char* name() const ;
    //       Return the official name for the encoding.
    int mibEnum() const ;
    // Return the MIB enum for the encoding if it is listed in the
    // IANA character-sets encoding file.

    QString toUnicode(const char* chars, int len) const ;
    // Converts len characters from chars to Unicode.
    QCString fromUnicode(const QString& uc, int& lenInOut ) const;
    // Converts lenInOut characters (of type QChar) from the start of
    // the string uc, returning a QCString result, and also returning
    // the length of the result in lenInOut.

    int heuristicContentMatch(const char *, int) const;
};


int QFontBig5Codec::heuristicContentMatch(const char *, int) const
{
    return 0;
}

QFontBig5Codec::QFontBig5Codec()
{
}

const char* QFontBig5Codec::name() const
{
    return "Big5";
}

int QFontBig5Codec::mibEnum() const
{
    return -1;
}

QString QFontBig5Codec::toUnicode(const char* /*chars*/, int /*len*/) const
{
    return QString(); //###
}

QCString QFontBig5Codec::fromUnicode(const QString& uc, int& lenInOut ) const
{
    QCString result;
    for ( int i = 0; i < lenInOut; i++ ) {
	QChar ch = uc[i];
	if ( ch.row() == 0) {
	    if ( ch.cell() == ' ' )
		ch = QChar( 0x3000 );
	    else if ( ch.cell() > ' ' && ch.cell() < 127 )
		ch = QChar( ch.cell()-' ', 255 );
	}
	ch = QChar( qt_UnicodeToBig5(ch.unicode()) );

	if ( ch.row() > 0xa0 && ch.cell() >= 0x40  ) {
	    result += ch.row();
	    result += ch.cell();
	} else {
	    //black square
	    result += 0xa1;
	    result += 0xbd;
	}
    }
    lenInOut *=2;
    return result;
}


#endif //QT_NO_CODECS


/*!
  Initializes the font information in the font's QFontInternal data.
  This function is called from load() for a new font.
*/

void QFont::initFontInfo() const
{
    QFontInternal *f = d->fin;
    if ( !f->s.dirty )				// already initialized
	return;

    f->s.lbearing = SHRT_MIN;
    f->s.rbearing = SHRT_MIN;
    f->computeLineWidth();


#ifdef QT_XFT
    XftFont *ft = d->fin->ft;
    if ( ft ) {
	resetFontDef( &f->s );
	XftPattern *pattern = ft->pattern;
	char *family;
	int slant_value;
	int weight_value;
	int fixed_pitch;
	XftPatternGetString (pattern, XFT_FAMILY, 0, &family);
	XftPatternGetInteger (pattern, XFT_SLANT, 0, &slant_value);
	XftPatternGetInteger (pattern, XFT_WEIGHT, 0, &weight_value);
	XftPatternGetInteger (pattern, XFT_SPACING, 0, &fixed_pitch);
	f->s.family = family;
	f->s.pointSize = d->req.pointSize; //will fit because Xft fonts are scalable
	f->s.charSet = QFont::Unicode;

	if (weight_value == XFT_WEIGHT_LIGHT)
	    weight_value = Light;
	else if (weight_value == XFT_WEIGHT_DEMIBOLD)
	    weight_value = DemiBold;
	else if (weight_value < XFT_WEIGHT_BOLD)
	    weight_value = Bold;
	else if ( weight_value == XFT_WEIGHT_BLACK)
	    weight_value = Black;
	else
	    weight_value = Normal;
	f->s.weight = weight_value;
	f->s.italic = slant_value != XFT_SLANT_ROMAN ? TRUE : FALSE;
	f->s.fixedPitch = fixed_pitch != XFT_PROPORTIONAL ? TRUE : FALSE;
	f->s.underline = d->req.underline;
	f->s.strikeOut = d->req.strikeOut;
	f->s.rawMode = FALSE;
	f->s.dirty = FALSE;
	return;
    }
#endif

    if (  d->exactMatch ) {
	if ( PRIV->needsSet() ) {
	    QCString locale = setlocale(LC_CTYPE, 0);
	    bool useLocale = FALSE;

	    switch (charSet()) {
	    case Set_Ja:
		// In case the user has set a locale, we want to use codecForLocale(),
		// and only use codecForName if the locale is something different (as eg "de").
		if (locale.left(2) != "ja")
		    f->cmapper = QTextCodec::codecForName("eucJP");
		break;
	    case Set_Ko:
		if (locale.left(2) != "ko")
		    f->cmapper = QTextCodec::codecForName("eucKR");
		break;
	    case Set_Zh:
		if (locale.left(2) != "zh")
		    f->cmapper = QTextCodec::codecForName("GBK");
		break;
	    case Set_Zh_TW:
		if (locale.left(5) != "zh_TW")
		    f->cmapper = QTextCodec::codecForName("Big5");
		break;
	    case Set_Th_TH: // ### need a codec?
	    default:
		useLocale = TRUE;
	    }

	    if ( useLocale || !f->cmapper )
		f->cmapper = QTextCodec::codecForLocale();
#ifndef QT_NO_CODECS
	} else if ( charSet() == JIS_X_0208 ) {
	    f->cmapper = QTextCodec::codecForName( "JIS_X_0208" );
	} else if ( charSet() == KSC_5601 ) {
	    f->cmapper = QTextCodec::codecForName( "KSC_5601" );
	} else if ( charSet() == GB_2312 ) {
	    f->cmapper = QTextCodec::codecForName( "GB_2312" );
	} else if ( charSet() == Big5 ) {
	    f->cmapper = QTextCodec::codecForName( "Big5" );
#endif //QT_NO_CODECS
	} else {
	    QCString encoding;
	    encoding = encodingName( charSet() );
	    if( charSet() == ISO_8859_8 || charSet() == ISO_8859_6 )
		encoding += "-I";
	    f->cmapper = QTextCodec::codecForName( encoding );
	}
	f->s = d->req;
	f->s.dirty = FALSE;
	return;
    }

    ASSERT(!PRIV->needsSet()); // They are always exact
    QCString encoding;

    if ( fillFontDef( f->name(), &f->s, &encoding ) ) { // valid XLFD?
#ifndef QT_NO_CODECS
	if ( encoding.left(9) == "jisx0208." ) {
	    f->cmapper = QTextCodec::codecForName( "JIS_X_0208" );
	} else if ( encoding.left(8) == "ksc5601." ) {
	    f->cmapper  = QTextCodec::codecForName( "KSC_5601" );
	} else if ( encoding.left(7) == "gb2312." ) {
	    f->cmapper = QTextCodec::codecForName( "GB_2312" );
	} else if ( encoding.left(4) == "big5" ) {
	    f->cmapper = QTextCodec::codecForName( "Big5" );
	} else
#endif //QT_NO_CODECS
	    f->cmapper = QTextCodec::codecForName( encoding );
    } else {
	f->cmapper = 0;
	resetFontDef( &f->s );
	f->s.family   = QString::fromLatin1(f->name());
	f->s.rawMode  = TRUE;
	d->exactMatch = FALSE;
	return;
    }
    f->s.underline = d->req.underline;
    f->s.strikeOut = d->req.strikeOut;
}

static
inline int maxIndex(XFontStruct *f)
{
    return
	( ( (f->max_byte1 - f->min_byte1) *
	    (f->max_char_or_byte2 - f->min_char_or_byte2 + 1) ) +
	  f->max_char_or_byte2 - f->min_char_or_byte2 );
}


static QCString setLocaleForCharSet(QFont::CharSet charset)
{
    QCString oldlocale;
    int i = 0;

    switch (charset) {
    case QFont::Set_Ja:
	{
	    oldlocale = setlocale(LC_CTYPE, NULL);
	    if (oldlocale.left(2) == "ja") return (const char *) 0;

	    const char *locales[] =
	    { "ja", "ja_JP", "ja_JP.EUC", "ja_JP.sjis", "ja_JP.ujis", "ja_JP.PCK",
		  "ja_JP.UTF-8", "ja_JP.SJIS", "ja_JP.Shift_JIS", 0 };

	    while (setlocale(LC_CTYPE, locales[i]) == NULL) i++;

	    break;
	}

    case QFont::Set_Ko:
	{
	    oldlocale = setlocale(LC_CTYPE, NULL);
	    if (oldlocale.left(2) == "ko") return (const char *) 0;

	    const char *locales[] =
	    { "ko", "ko_KR", "ko_KR.EUC", 0 };

	    while (setlocale(LC_CTYPE, locales[i]) == NULL) i++;

	    break;
	}

    case QFont::Set_Zh: // assume GBK and/or EUC
    case QFont::Set_GBK: // assume GBK and/or EUC
	{
	    oldlocale = setlocale(LC_CTYPE, NULL);
	    if (oldlocale.left(2) == "zh") return (const char *) 0;

	    const char *locales[] =
	    { "zh", "zh_CN", "zh.GBK", "zh_CN.GBK", "zh_CN.GB2312", "zh_CN.EUC", 0 };

	    while (setlocale(LC_CTYPE, locales[i]) == NULL) i++;

	    break;
	}

    case QFont::Set_Zh_TW: // assume Big5 and/or EUC
    case QFont::Set_Big5: // assume Big5 and/or EUC
	{
	    oldlocale = setlocale(LC_CTYPE, NULL);
	    if (oldlocale.left(5) == "zh_TW") return (const char *) 0;

	    const char *locales[] =
	    { "zh_TW.Big5", "zh_TW.BIG5", "zh_TW", "zh_TW.EUC", 0 };

	    while (setlocale(LC_CTYPE, locales[i]) == NULL) i++;

	    break;
	}

    case QFont::Set_Th_TH:
    default:
	;
    }

    return oldlocale;
}


/*!
  Loads the font.
*/
void QFont::load() const
{
    if ( !fontCache ) {				// not initialized
#if defined(CHECK_STATE)
	qFatal( "QFont: Must construct a QApplication before a QFont" );
#endif

	return;
    }

    QString k = key();

#ifdef QT_XFT
    if (qt_use_xft())
    {
	d->fin = fontCache->find (k);
	if (!d->fin) {
	    d->fin = fontDict->find (k);
	    if (!d->fin) {
		d->fin = new QFontInternal (k);
		CHECK_PTR(d->fin);
		fontDict->insert (k, d->fin);
	    }
	}
	
	XftFont	    *ft = d->fin->ft;
	
	if (!ft)
	{
	    const char	*family_value;
	    const char	*generic_value;
	    int	weight_value;
	    int	slant_value;
	    double	size_value;
	    int	mono_value;

	    family_value = family();
	    weight_value = weight ();
	    if (weight_value == 0)
		weight_value = XFT_WEIGHT_MEDIUM;
	    else if (weight_value < (Light + Normal) / 2)
		weight_value = XFT_WEIGHT_LIGHT;
	    else if (weight_value < (Normal + DemiBold) / 2)
		weight_value = XFT_WEIGHT_MEDIUM;
	    else if (weight_value < (DemiBold + Bold) / 2)
		weight_value = XFT_WEIGHT_DEMIBOLD;
	    else if (weight_value < (Bold + Black) / 2)
		weight_value = XFT_WEIGHT_BOLD;
	    else
		weight_value = XFT_WEIGHT_BLACK;
	    if (italic())
		slant_value = XFT_SLANT_ITALIC;
	    else
		slant_value = XFT_SLANT_ROMAN;
	    size_value = pointSize();
	    mono_value = fixedPitch() ? XFT_MONO : XFT_PROPORTIONAL;
	    
	    int	family_width, family_height;
	    if (sscanf (family_value, "%dx%d", 
			&family_width, &family_height))
	    {
		family_value = "mono";
		size_value = family_height;
		mono_value = 1;
	    }
	    switch (styleHint()) {
	    case SansSerif:
	    default:
		generic_value = "sans";
		break;
	    case Serif:
		generic_value = "serif";
		break;
	    case TypeWriter:
		generic_value = "mono";
		mono_value = 1;
		break;
	    }
#if 0
	    printf ("%s,%s-%g:weight-%d:slant-%d\n",
		    family_value, generic_value, size_value,
		    weight_value, slant_value);
#endif
	    ft = XftFontOpen (QPaintDevice::x11AppDisplay(),
			      QPaintDevice::x11AppScreen(),
			      XFT_ENCODING, XftTypeString, "iso10646-1",
			      XFT_FAMILY, XftTypeString, family_value,
			      XFT_FAMILY, XftTypeString, generic_value,
			      XFT_WEIGHT, XftTypeInteger, weight_value,
			      XFT_SLANT, XftTypeInteger, slant_value,
			      XFT_SIZE, XftTypeDouble, size_value,
			      XFT_SPACING, XftTypeInteger, mono_value,
			      0);
#if 0
	    if (ft)
		XftPatternPrint (ft->pattern);
#endif
	    fontCache->insert (k, d->fin, 1);
	    d->fin->ft = ft;
	    d->exactMatch = TRUE;
	    initFontInfo ();
	}
	return;
    }
#endif
    
    QXFontName *fn = fontNameDict->find( k );

    if ( !fn ) {
	QString name;
	bool match;
	if ( d->req.rawMode ) {
	    name = substitute( family() );
	    match = fontExists( name );
	    if ( !match )
		name = lastResortFont();
	} else {
	    name = PRIV->findFont( &match );
	}
	fn = new QXFontName( name.ascii(), match );
	CHECK_PTR( fn );
	fontNameDict->insert( k, fn );
    }
    d->exactMatch = fn->exactMatch;

    QCString n = fn->name;
    d->fin = fontCache->find( n.data() );
    if ( !d->fin ) {				// font not loaded
	d->fin = fontDict->find( n.data() );
	if ( !d->fin ) {			// font was never loaded
	    d->fin = new QFontInternal( n );
	    CHECK_PTR( d->fin );
	    fontDict->insert( d->fin->name(), d->fin );
	}
    }
    if ( PRIV->needsSet() )  {
	XFontSet s = d->fin->set;
	if ( !s ) {
	    char** missing=0;
	    int nmissing;
	    //qDebug("fontSet : %s\n",n.data());

	    // set the locale for the charset of the application, this will
	    // allow applications in (for example) Latin1 environment display
	    // Korean or Japanese characters

	    QCString oldlocale = setLocaleForCharSet(charSet());
	    s = XCreateFontSet( QPaintDevice::x11AppDisplay(), n,
				&missing, &nmissing, 0 );
	    if ( missing ) {
#if defined(DEBUG)
		for(int i=0; i<nmissing; i++)
		    qDebug("Qt: missing charset %s",missing[i]);
#endif
		XFreeStringList(missing);
	    }

	    // restore locale for application
	    if (! oldlocale.isNull() && ! oldlocale.isEmpty()) {
		// qDebug("restoring locale '%s'", (const char *) oldlocale);
		setlocale(LC_CTYPE, (const char *) oldlocale);
	    }
	    if ( !s ) {
		d->fin->f = XLoadQueryFont( QPaintDevice::x11AppDisplay(),
				    lastResortFont().ascii() );
		fn->exactMatch = FALSE;
#if defined(CHECK_NULL)
		if ( !d->fin->f )
		    qFatal( "QFont::load: Internal error" );
#endif
	    }

	    d->fin->set = s;
	    // [not cached]
	    initFontInfo();
	}
    } else {
	XFontStruct *f = d->fin->f;
	if ( !f ) {					// font not loaded
	    //qDebug("font : %s\n",n.data());
	    f = XLoadQueryFont( QPaintDevice::x11AppDisplay(), n );
	    if ( !f ) {
		f = XLoadQueryFont( QPaintDevice::x11AppDisplay(),
				    lastResortFont().ascii() );
		fn->exactMatch = FALSE;
#if defined(CHECK_NULL)
		if ( !f )
		    qFatal( "QFont::load: Internal error" );
#endif
	    }
	    int chars = maxIndex(f);
	    if ( chars > 2000 ) {
		// If the user is using large fonts, we assume they have
		// turned on the Xserver option deferGlyphs, and that they
		// have more memory available to the server.
		chars = 2000;
	    }
	    fontCache->insert(d->fin->name(), d->fin, 1);
	    d->fin->f = f;
	    initFontInfo();
	}
    }
    d->req.dirty = FALSE;
    delete d->printerHackFont;
    d->printerHackFont = 0;
}


/*****************************************************************************
  QFont_Private member functions
 *****************************************************************************/

#define exactScore	   0xfffe
#define exactNonUnicodeScore  0xffff

#define CharSetScore	 0x80
#define PitchScore	 0x40
#define SizeScore	 0x20
#define ResolutionScore	 0x10
#define WeightScore	 0x08
#define SlantScore	 0x04
#define WidthScore	 0x02
#define NonUnicodeScore	 0x01

//
// Returns a score describing how well a font name matches the contents
// of a font.
//

int QFont_Private::fontMatchScore( char	 *fontName,	 QCString &buffer,
				   float *pointSizeDiff, int  *weightDiff,
				   bool	 *scalable     , bool *smoothScalable )
{
    char *tokens[fontFields];
    bool   exactMatch = TRUE;
    int	   score      = NonUnicodeScore;
    *scalable	      = FALSE;
    *smoothScalable   = FALSE;
    *weightDiff	      = 0;
    *pointSizeDiff    = 0;

    qstrcpy( buffer.data(), fontName );	// NOTE: buffer must be large enough
    if ( !qParseXFontName( buffer, tokens ) )
	return 0;	// Name did not conform to X Logical Font Description

#undef	IS_ZERO
#define IS_ZERO(X) (X[0] == '0' && X[1] == 0)

    if ( isScalable( tokens ) ) {
	*scalable = TRUE;			// scalable font
	if ( isSmoothlyScalable( tokens ) )
	    *smoothScalable = TRUE;
    }

    // the next bit isn't comprehensive, but it does at least cover
    // the fonts I've seen so far. Thai and traditional Chinese fonts
    // aren't on the list, guess why. --Arnt
    if ( charSet() == AnyCharSet ) {
	// this can happen at least two cases which do not deserve warnings:
	// 1. if the program is being used in the yoo-nited states
	//    and without $LANG
	// 2. if the program explicitly asks for AnyCharSet
	score |= CharSetScore;
    } else if ( charSet() == KOI8R ) {
	if ( qstrcmp( tokens[CharsetRegistry], "koi8" ) == 0 &&
	     ( qstrcmp( tokens[CharsetEncoding], "r" ) == 0 ||
	       qstrcmp( tokens[CharsetEncoding], "1" ) == 0) )
	       score |= CharSetScore;
       else
	       exactMatch = FALSE;
    } else if ( charSet() == KOI8U ) {
       if ( qstrcmp( tokens[CharsetRegistry], "koi8" ) == 0 &&
	    qstrcmp( tokens[CharsetEncoding], "u" ) == 0 )
	    score |= CharSetScore;
	else
	    exactMatch = FALSE;
    } else if ( qstrcmp( tokens[CharsetRegistry], "iso8859" ) == 0 &&
		charSet() >= ISO_8859_1 && charSet() <= ISO_8859_15 ) {
	int i = 0;
	while( c8859[i] != ISO_8859_15 && c8859[i] != charSet() )
	    i++;
	QString s = QString::number( i+1 );
	if ( s == tokens[CharsetEncoding] )
	    score |= CharSetScore;
	else
	    exactMatch = FALSE;
    } else if ( charSet() == QFont::JIS_X_0201 ) {
	if( qstrncmp( tokens[CharsetRegistry], "jisx0201.", 9 ) == 0 )
	    score |= CharSetScore;
	else
	    exactMatch = FALSE;
    } else if ( charSet() == QFont::JIS_X_0208 ) {
	if( qstrncmp( tokens[CharsetRegistry], "jisx0208.", 9 ) == 0 )
	    score |= CharSetScore;
	else
	    exactMatch = FALSE;
    } else if ( charSet() == QFont::KSC_5601 ) {
	if( qstrncmp( tokens[CharsetRegistry], "ksc5601.", 8 ) == 0 )
	    score |= CharSetScore;
	else
	    exactMatch = FALSE;
    } else if ( charSet() == QFont::GB_2312 ) {
	if( qstrncmp( tokens[CharsetRegistry], "gb2312.", 7 ) == 0 )
	    score |= CharSetScore;
	else
	    exactMatch = FALSE;
    } else if ( charSet() == QFont::Big5 ) {
	if( qstrncmp( tokens[CharsetRegistry], "big5", 4 ) == 0 )
	    score |= CharSetScore;
	else
	    exactMatch = FALSE;
    } else if ( charSet() == TSCII ) {
	if ( qstrcmp( tokens[CharsetRegistry], "tscii" ) == 0 &&
	     qstrcmp( tokens[CharsetEncoding], "0" ) == 0 )
	    score |= CharSetScore;
	else
	    exactMatch = FALSE;
    } else if ( charSet() == TIS620 ) {
	if ( qstrcmp( tokens[CharsetRegistry], "tis620" ) == 0 )
	    score |= CharSetScore;
	else
	    exactMatch = FALSE;
    } else if ( charSet() == CP1251 ) {
        if  (qstrcmp( tokens[CharsetEncoding], "cp1251" ) == 0 ||
                qstrcmp( tokens[CharsetEncoding], "1251" ) == 0)
                  score |= CharSetScore;
          else
                  exactMatch = FALSE;
    } else if ( charSet() == PT154 ) {
        if  (qstrcmp( tokens[CharsetEncoding], "cp154" ) == 0 ||
                qstrcmp( tokens[CharsetEncoding], "154" ) == 0)
                  score |= CharSetScore;
          else
                  exactMatch = FALSE;
    } else if ( qstrcmp( tokens[CharsetRegistry], "iso10646" ) == 0 ) {
	// Yes...
	score |= CharSetScore;
	// But it's big...
	score &= ~NonUnicodeScore;
    } else {
	exactMatch = FALSE;
    }

    char pitch = tolower( tokens[Spacing][0] );
    if ( fixedPitch() ) {
	if ( pitch == 'm' || pitch == 'c' )
	    score |= PitchScore;
	else
	    exactMatch = FALSE;
    } else {
	if ( pitch == 'p' )
	    score |= PitchScore;
	else
	    exactMatch = FALSE;
    }

    // ### fix scaled bitmap fonts
    float diff;
    if ( *scalable ) {
	diff = 9.0;	// choose scalable font over >= 0.9 point difference
	score |= SizeScore;
	exactMatch = FALSE;
    } else {
	int pSize;
	float percentDiff;

	pSize = ( 2*atoi( tokens[PointSize] )*atoi(tokens[ResolutionY]) +
		      QPaintDevice::x11AppDpiY())
		    / (QPaintDevice::x11AppDpiY() * 2); // adjust actual pointsize

	if ( deciPointSize() != 0 ) {
	    diff = (float)QABS(pSize - deciPointSize());
	    percentDiff = diff/deciPointSize()*100.0F;
	} else {
	    diff = (float)pSize;
	    percentDiff = 100;
	}

	if ( percentDiff < 10 ) {
	    score |= SizeScore;
	    if ( pSize != deciPointSize() ) {
		exactMatch = FALSE;
	    }
	} else {
	    exactMatch = FALSE;
	}
    }
    if ( pointSizeDiff )
	*pointSizeDiff = diff;
    int weightVal = qFontGetWeight( tokens[Weight_], TRUE );

    if ( weightVal == weight() )
	score |= WeightScore;
    else
	exactMatch = FALSE;

    *weightDiff = QABS( weightVal - weight() );
    char slant = tolower( tokens[Slant][0] );
    if ( italic() ) {
	if ( slant == 'o' || slant == 'i' )
	    score |= SlantScore;
	else
	    exactMatch = FALSE;
    } else {
	if ( slant == 'r' )
	    score |= SlantScore;
	else
	    exactMatch = FALSE;
    }
    if ( qstricmp( tokens[Width], "normal" ) == 0 )
	score |= WidthScore;
    else
	exactMatch = FALSE;
    return exactMatch ? (exactScore | (score&NonUnicodeScore)) : score;
}


struct QFontMatchData {			// internal for bestMatch
    QFontMatchData()
	{ score=0; name=0; pointDiff=99; weightDiff=99; smooth=FALSE; }
    int	    score;
    char   *name;
    float   pointDiff;
    int	    weightDiff;
    bool    smooth;
};

QCString QFont_Private::bestMatch( const char *pattern, int *score )
{
    QFontMatchData	best;
    QFontMatchData	bestScalable;

    QCString	matchBuffer( 256 );	// X font name always <= 255 chars
    char **	xFontNames;
    int		count;
    int		sc;
    float	pointDiff;	// difference in % from requested point size
    int		weightDiff;	// difference from requested weight
    bool	scalable       = FALSE;
    bool	smoothScalable = FALSE;
    int		i;

    xFontNames = getXFontNames( pattern, &count );

    for( i = 0; i < count; i++ ) {
	sc = fontMatchScore( xFontNames[i], matchBuffer,
			     &pointDiff, &weightDiff,
			     &scalable, &smoothScalable );
	if ( scalable ) {
	    if ( sc > bestScalable.score ||
		 sc == bestScalable.score &&
		 weightDiff < bestScalable.weightDiff ||
		 sc == bestScalable.score &&
		 weightDiff == bestScalable.weightDiff &&
		 smoothScalable && !bestScalable.smooth ) {
		bestScalable.score	= sc;
		bestScalable.name	= xFontNames[i];
		bestScalable.pointDiff  = pointDiff;
		bestScalable.weightDiff = weightDiff;
		bestScalable.smooth	= smoothScalable;
	    }
	} else {
	    if ( sc > best.score ||
		 sc == best.score && pointDiff < best.pointDiff ||
		 sc == best.score && pointDiff == best.pointDiff &&
		 weightDiff < best.weightDiff ) {
		best.score	= sc;
		best.name	= xFontNames[i];
		best.pointDiff	= pointDiff;
		best.weightDiff = weightDiff;
	    }
	}
    }
    QCString bestName;
    char *tokens[fontFields];

    //qDebug("requesting font '%s' pixSize=%d pointSize=%d", pattern, pixelSize(), pointSize() );
    //qDebug("best font: %d '%s'", best.score, best.name);
    //qDebug("best scalable font: %d '%s'", bestScalable.score, bestScalable.name);

    if ( bestScalable.score > best.score ||
	 bestScalable.score == best.score &&
	 bestScalable.pointDiff < best.pointDiff ||
	 bestScalable.score == best.score &&
	 bestScalable.pointDiff == best.pointDiff &&
	 bestScalable.weightDiff < best.weightDiff ) {
	//qDebug("using scalable font");
	qstrcpy( matchBuffer.data(), bestScalable.name );
	if ( qParseXFontName( matchBuffer, tokens ) ) {
	    int resx;
	    int resy;
	    int pSize;
	    if ( bestScalable.smooth ) {
		// X will scale the font accordingly
		resx  = QPaintDevice::x11AppDpiX();
		resy  = QPaintDevice::x11AppDpiY();
		pSize = deciPointSize();
	    } else {
		resx = atoi(tokens[ResolutionX]);
		resy = atoi(tokens[ResolutionY]);
		pSize = ( 2*deciPointSize()*QPaintDevice::x11AppDpiY() + resy )
			/ (resy * 2);
	    }
	    bestName.sprintf( "-%s-%s-%s-%s-%s-%s-*-%i-%i-%i-%s-*-%s-%s",
			      tokens[Foundry],
			      tokens[Family],
			      tokens[Weight_],
			      tokens[Slant],
			      tokens[Width],
			      tokens[AddStyle],
			      pSize,
			      resx, resy,
			      tokens[Spacing],
			      tokens[CharsetRegistry],
			      tokens[CharsetEncoding] );
	    best.name  = bestName.data();
	    best.score = bestScalable.score;
	}
    }
    *score   = best.score;
    bestName = best.name;

    XFreeFontNames( xFontNames );
    //qDebug("best font found is '%s'", (const char *)bestName);
    return bestName;
}


QCString QFont_Private::bestFamilyMember( const QString& foundry,
					  const QString& family, int *score )
{
    const int prettyGoodScore = CharSetScore | SizeScore |
				WeightScore | SlantScore | WidthScore;

    int testScore = 0;
    QCString testResult;
    int bestScore = 0;
    QCString result;

    if ( !foundry.isEmpty() ) {
	QString pattern
	    = "-" + foundry + "-" + family + "-*-*-*-*-*-*-*-*-*-*-*-*";
	result = bestMatch( pattern.latin1(), &bestScore );
    }

    if ( bestScore < prettyGoodScore ) {
	QRegExp alt("[,;]");
	int alternator = 0;
	int next;
	int bias = 0;
	while ( alternator < (int)family.length() ) {
	    next = family.find(alt,alternator);
	    if ( next < alternator ) next = family.length();
	    QString fam = family.mid(alternator,next-alternator);
	    QString pattern = "-*-" + fam + "-*-*-*-*-*-*-*-*-*-*-*-*";
	    testResult = bestMatch( pattern.latin1(), &testScore );
	    bestScore -= bias;
	    if ( testScore > bestScore ) {
		bestScore = testScore;
		result = testResult;
	    }
	    if ( family[next] == ';' )
		bias += 1;
	    alternator = next + 1;
	}
    }

    if ( score )
	*score = bestScore;
    return result;
}

// Font Guessing
bool QFont_Private::fontmapping(const QString& filename)
{
    QStringList chsetlst;

    QFile f( filename );
    if ( f.open( IO_ReadOnly ) ) {
	QTextStream t( &f );
	QString s;
	while ( !t.eof() ) {
  	    s = t.readLine();
	    if ( s.isEmpty() || s[0] == '#' )
	        continue;
	    if ( s.contains('[') ) {
		QRegExp sep( "[][]" );
		chsetlst = QStringList::split( sep, s );
	    } else {
		QStringList familylst;
		QString ss = s.stripWhiteSpace();
		int sslen = ss.length();
		QString fontName;
		bool quote = FALSE;
		for( int i = 0; i <= sslen; ++i ) {
		    if( i < sslen ) {
			QChar c = ss[i];
			if( c == '\"' || c == '\'' ) {
			    quote = !quote;
			    continue;
			} else if( quote || (c != ' ' && c != '\t' && c != '=' ) ) {
			    fontName += c;
			    continue;
			}
		    }
		    if ( !fontName.isEmpty() ) {
			familylst << fontName;
			fontName = "";
		    }
		}
		if ( !chsetlst.isEmpty() && !familylst.isEmpty() ) {
		    if ( !fontGuessingList )
			fontGuessingList = new QList<FontGuessingPair>;
		    FontGuessingPair *fontGuessingPair;
		    fontGuessingPair = new FontGuessingPair;
		    fontGuessingPair->charset = chsetlst;
		    fontGuessingPair->family = familylst;
		    fontGuessingList->append(fontGuessingPair);
#if DBG // to debug
		    printf( "charset = [%s], from = [%s], to = [%s]\n",
		  		fontGuessingPair->charset[0].ascii(),
		  		fontGuessingPair->family[1].ascii() );
#endif
		}
	    }
	} /* while */
	f.close();
	return TRUE;
    } else
	return FALSE;
}

QCString QFont_Private::bestMatchFontSetMember( const QString& family,
		const char *wt, const char *slant, int size, int xdpi, int ydpi )
{
    static int read = 0;

    if ( !read ) {
        QString filename;
        read = 1;
        QString home = QDir::homeDirPath();
        filename = qstrdup(home + "/.fontguess");
        fontmapping(filename);
        filename = qstrdup(FONT_GUESSING_FILE);
        fontmapping(filename);
    }

    QCString bestName;

    if ( fontGuessingList ) {
	FontGuessingPair *fontGuessingPair = fontGuessingList->first();
	while ( fontGuessingPair ) {
	    if ( qstricmp(family, fontGuessingPair->family[0]) == 0 ) {
		for ( int i = 0; i < (int)(fontGuessingPair->family.count())-1; ++i ) {
		    char s[1024];
		    sprintf( s, "-*-%s-%s-%s-*-*-*-%d-%d-%d-*-*-%s,",
				fontGuessingPair->family[i+1].latin1(),
			     wt, slant, size, xdpi, ydpi,
				fontGuessingPair->charset[i].latin1() );
				bestName.append(s);
		}
		return bestName;
	    }
	    fontGuessingPair = fontGuessingList->next();
	}
    }

    // default font
    bestName.sprintf("-*-helvetica-%s-%s-*-*-*-%d-%d-%d-*-*-*-*,",
		wt, slant, size, xdpi, ydpi);
    return bestName;
}

// Font Guessing end


QCString QFont_Private::findFont( bool *exact )
{
    QString familyName = family();
    *exact = TRUE;				// assume exact match
    if ( familyName.isEmpty() ) {
	familyName = defaultFamily();
	*exact = FALSE;
    }

    QString foundry;

    if ( familyName.contains('-') ) {
	int i = familyName.find('-');
	foundry = familyName.left( i );
	familyName = familyName.right( familyName.length() - i - 1 );
    }

    if ( needsSet() ) {
	// Font sets do not use scoring.
	*exact = TRUE;

	const char* wt = weight() < 37 ? "light" :
		 ( weight() < 57 ? "medium" :
		   ( weight() < 69 ? "demibold" :
		     ( weight() < 81 ? "bold" : "black" ) ) );
	const char* slant = italic() ? "i" : "r";
	const char* slant2 = italic() ? "o" : "r";
	int size = pointSize()*10;
	QCString s( 512 + 3*familyName.length() );
	int xdpi = QPaintDevice::x11AppDpiX();
	int ydpi = QPaintDevice::x11AppDpiY();
	if ( foundry.isEmpty() ) {
	    s.sprintf( "-*-%s-%s-%s-normal-*-*-%d-%d-%d-*-*-*-*,"
		       "-*-%s-*-%s-*-*-*-%d-%d-%d-*-*-*-*,"
		       "-*-%s-*-%s-*-*-*-%d-%d-%d-*-*-*-*,"
				// Font Guessing
		       "%s"
		       "-*-*-*-%s-*-*-*-%d-%d-%d-*-*-*-*,"
		       "-*-*-*-*-*-*-*-%d-%d-%d-*-*-*-*",
		       familyName.ascii(), wt, slant, size, xdpi, ydpi,
		       familyName.ascii(), slant, size, xdpi, ydpi,
		       familyName.ascii(), slant2, size, xdpi, ydpi,
				// Font Guessing
		       bestMatchFontSetMember(familyName, wt, slant, size, xdpi, ydpi).data(),
		       slant, size, xdpi, ydpi,
		       size, xdpi, ydpi );
	} else {
	    s.sprintf( "-%s-%s-%s-%s-normal-*-*-%d-%d-%d-*-*-*-*,"
		       "-%s-%s-*-%s-*-*-*-%d-%d-%d-*-*-*-*,"
		       "-%s-%s-*-%s-*-*-*-%d-%d-%d-*-*-*-*,"
		       "-*-%s-%s-%s-normal-*-*-%d-%d-%d-*-*-*-*,"
		       "-*-%s-*-%s-*-*-*-%d-%d-%d-*-*-*-*,"
		       "-*-%s-*-%s-*-*-*-%d-%d-%d-*-*-*-*,"
				// Font Guessing
		       "%s"
		       "-*-*-*-%s-*-*-*-%d-%d-%d-*-*-*-*,"
		       "-*-*-*-*-*-*-*-%d-%d-%d-*-*-*-*",
		       foundry.ascii(), familyName.ascii(), wt, slant, size, xdpi, ydpi,
		       foundry.ascii(), familyName.ascii(), slant, size, xdpi, ydpi,
		       foundry.ascii(), familyName.ascii(), slant2, size, xdpi, ydpi,
		       familyName.ascii(), wt, slant, size, xdpi, ydpi,
		       familyName.ascii(), slant, size, xdpi, ydpi,
		       familyName.ascii(), slant2, size, xdpi, ydpi,
				// Font Guessing
		       bestMatchFontSetMember(familyName, wt, slant, size, xdpi, ydpi).data(),
		       slant, size, xdpi, ydpi,
		       size, xdpi, ydpi );
	}
	return s;
    } else {
	int score;
	QCString bestName = bestFamilyMember( foundry, familyName, &score );
	if ( score < exactScore )
	    *exact = FALSE;

	if ( !(score & NonUnicodeScore) )
	    setCharSet( Unicode );

	if ( score < CharSetScore ) {
	    QString f = substitute( family() );
	    if( familyName != f ) {
		familyName = f;                     // try substitution
		bestName = bestFamilyMember( foundry, familyName, &score );
	    }
	}
	if ( score < CharSetScore ) {
	    QString f = defaultFamily();
	    if ( familyName != f ) {
		familyName = f;			// try default family for style
		bestName = bestFamilyMember( foundry, familyName, &score );
	    }
	    if ( score < CharSetScore ) {
		f = lastResortFamily();
		if ( familyName != f ) {
		    familyName = f;		// try system default family
		    bestName = bestFamilyMember( foundry, familyName, &score );
		}
	    }
	}
	if ( bestName.isNull() )		// no matching fonts found
	    bestName = lastResortFont().ascii();
	return bestName;
    }
}


/*****************************************************************************
  QFontMetrics member functions
 *****************************************************************************/

const QFontDef *QFontMetrics::spec() const
{
    if ( painter ) {
	painter->cfont.handle();
	return painter->cfont.d->fin->spec();
    } else {
	return fin->spec();
    }
}

void *QFontMetrics::fontStruct() const
{
    if ( painter ) {
	painter->cfont.handle();
	// ### printer font metrics hack
	if ( painter->device() && 0 &&
	     painter->device()->devType() == QInternal::Printer &&
	     painter->cfont.pointSize() < 48 ) {
	    if ( painter->cfont.d->printerHackFont == 0 ) {
		painter->cfont.d->printerHackFont
		    = new QFont( painter->cfont );
		painter->cfont.d->printerHackFont->setPointSize( 64 );
	    }
	    painter->cfont.d->printerHackFont->handle();
	    return painter->cfont.d->printerHackFont->d->fin->fontStruct();
	}
	return painter->cfont.d->fin->fontStruct();
    } else {
	return fin->fontStruct();
    }
}

void *QFontMetrics::fontSet() const
{
    if ( painter ) {
	painter->cfont.handle();
	return painter->cfont.d->fin->fontSet();
    } else {
	return fin->fontSet();
    }
}

const QTextCodec *QFontMetrics::mapper() const
{
    if ( painter ) {
	painter->cfont.handle();
	return painter->cfont.d->fin->mapper();
    } else {
	return fin->mapper();
    }
}

#undef  FS
#define FS (painter ? (XFontStruct*)fontStruct() : fin->fontStruct())

#undef FT
#undef FT_TYPE
#ifdef QT_XFT
#define FT_TYPE	XftFont
#define FT (painter ? painter->cfont.handle(), (XftFont *)painter->cfont.d->fin->ftfont() : fin->ftfont())
#else
#define FT_TYPE void
#define FT ((FT_TYPE *) 0)
#endif

#undef  SET
#define SET ((XFontSet)fontSet())

// How to calculate metrics from ink and logical rectangles.
#define LBEARING(i,l) (i.x+l.x)
#define RBEARING(i,l) (i.width-l.width)
#define ASCENT(i,l) (-i.y)
#define DESCENT(i,l) (i.height+i.y-1)


int QFontMetrics::printerAdjusted(int val) const
{
    if ( painter && painter->device() &&
	 painter->device()->devType() == QInternal::Printer) {
	painter->cfont.handle();
	// ### printer font metrics hack
// 	if ( painter->device() &&
// 	     painter->device()->devType() == QInternal::Printer &&
// 	     painter->cfont.d->printerHackFont ) {
// 	    painter->cfont.d->printerHackFont->handle();
// 	    val = val * painter->cfont.pointSize() / 64;
// 	}
	// this was wrong.
	//return val;
	int res = QPaintDevice::x11AppDpiY();
	return ( val * 75 + 36 ) / res; // PostScript is 72dpi, but 75 gives a little more spacing as some
	// fonts would be too close together otherwise.
    } else {
	return val;
    }
}

/*!
  Returns the maximum ascent of the font.

  The ascent is the distance from the base line to the uppermost line
  where pixels may be drawn.

  \sa descent()
*/

int QFontMetrics::ascent() const
{
#ifdef QT_XFT
    XftFont	*ft = FT;

    if (ft)
	return printerAdjusted(ft->ascent);
#endif
    XFontStruct *f = FS;
    if ( f )
	return printerAdjusted(f->max_bounds.ascent);
    XFontSetExtents *ext = XExtentsOfFontSet(SET);
    return printerAdjusted(ASCENT(ext->max_ink_extent,
				  ext->max_logical_extent));
}


/*!
  Returns the maximum descent of the font.

  The descent is the distance from the base line to the lowermost line
  where pixels may be drawn. (Note that this is different from X, which
  adds 1 pixel.)

  \sa ascent()
*/

int QFontMetrics::descent() const
{
#ifdef QT_XFT
    XftFont	*ft = FT;

    if (ft)
	return printerAdjusted(ft->descent);
#endif
    XFontStruct *f = FS;
    if ( f )
	return printerAdjusted(f->max_bounds.descent - 1);
    XFontSetExtents *ext = XExtentsOfFontSet(SET);
    return printerAdjusted(DESCENT(ext->max_ink_extent,ext->max_logical_extent));
}

inline bool inFont(const QTextCodec *mapper, XFontStruct *f, QChar ch)
{
    XCharStruct *xcs = 0;

    if ( mapper ) {
	int l = 1;
	QCString c = mapper->fromUnicode(ch,l);
	if ( c.length() == 1 )
	    ch = c[0];
	else
	    ch = QChar( *((ushort *)c.data()) ); // this *is* ugly
    }

    if ( f->max_byte1 ) {
	if ( ch.cell() >= f->min_char_or_byte2
	     && ch.cell() <= f->max_char_or_byte2
	     && ch.row() >= f->min_byte1
	     && ch.row() <= f->max_byte1 )
	    if( f->per_char )
		xcs = f->per_char + ((ch.row() - f->min_byte1)
				     * (f->max_char_or_byte2 - f->min_char_or_byte2 + 1)
				     + ch.cell() - f->min_char_or_byte2);
	    else
		xcs = &f->max_bounds;
    } else if ( ch.row() ) {
	uint ch16 = ch.unicode();
	if ( ch16 >= f->min_char_or_byte2
	     && ch16 <= f->max_char_or_byte2 )
	    if( f->per_char )
		xcs = f->per_char + ch16 - f->min_char_or_byte2;
	    else
		xcs = &f->max_bounds;
    } else {
	if ( ch.cell() >= f->min_char_or_byte2
	     && ch.cell() <= f->max_char_or_byte2 )
	    if( f->per_char )
		xcs = f->per_char + ch.cell() - f->min_char_or_byte2;
	    else
		xcs = &f->max_bounds;
    }
    if ( !xcs || (xcs->width == 0 && xcs->ascent + xcs->descent == 0) )
	return FALSE;
    return TRUE;
}

/*!
  Returns TRUE if \a ch is a valid character in the font.
*/
 bool QFontMetrics::inFont(QChar ch) const
 {
#ifdef QT_XFT
     XftFont *ft = FT;
    if (ft)
	return XftGlyphExists (QPaintDevice::x11AppDisplay(), ft,
			       (unsigned int) ch.unicode());
#endif
    XFontStruct *f = FS;
    return ::inFont( mapper(), f, ch );
}

static
XCharStruct* charStr(const QTextCodec* mapper, XFontStruct *f, QChar ch)
{
    // Optimized - inFont() is merged in here.

    if ( !f->per_char )
	return &f->max_bounds;

    if ( mapper ) {
	int l = 1;
	QCString c = mapper->fromUnicode(ch,l);
	if ( c.length() == 1 )
	    ch = c[0];
	else
	    ch = QChar( *((ushort *)c.data()) ); // this *is* ugly
    }

    if ( f->max_byte1 ) {
	if ( !(ch.cell() >= f->min_char_or_byte2
	       && ch.cell() <= f->max_char_or_byte2
	       && ch.row() >= f->min_byte1
	       && ch.row() <= f->max_byte1) ) {
	    ch = QChar((ushort)f->default_char);
	    if ( !(ch.cell() >= f->min_char_or_byte2
		   && ch.cell() <= f->max_char_or_byte2
		   && ch.row() >= f->min_byte1
		   && ch.row() <= f->max_byte1) )
		return f->per_char;
	}
	return f->per_char +
	    ((ch.row() - f->min_byte1)
	     * (f->max_char_or_byte2 - f->min_char_or_byte2 + 1)
	     + ch.cell() - f->min_char_or_byte2);
    } else if ( ch.row() ) {
	uint ch16 = ch.unicode();
	if ( !(ch16 >= f->min_char_or_byte2
	       && ch16 <= f->max_char_or_byte2) ) {
	    ch16 = f->default_char;
	    if ( !(ch16 >= f->min_char_or_byte2
		   && ch16 <= f->max_char_or_byte2) )
		return f->per_char;
	}
	return f->per_char + ch16 - f->min_char_or_byte2;
    } else {
	if ( !( ch.cell() >= f->min_char_or_byte2
		&& ch.cell() <= f->max_char_or_byte2) ) {
	    ch = QChar((uchar)f->default_char);
	    if ( !( ch.cell() >= f->min_char_or_byte2
		    && ch.cell() <= f->max_char_or_byte2) )
		return f->per_char;
	}
	return f->per_char + ch.cell() - f->min_char_or_byte2;
    }
}

#ifdef QT_XFT
static 
XGlyphInfo *glyphStr(const QTextCodec* /*mapper*/, XftFont *ft, QChar ch)
{
    unsigned int	ich;
    static XGlyphInfo	info;

    ich = ch.unicode();
    XftTextExtents32 (QPaintDevice::x11AppDisplay(),
		      ft, &ich, 1, &info);
#if 0
    if (ich == 32)
    printf ("glyphStr '%c' x: %d y: %d width: %d height: %d: xoff: %d yoff: %d\n",
	    (char) ich, info.x, info.y, info.width, info.height,
	    info.xOff, info.yOff);
#endif
    return &info;
}
#endif

static void getExt( QString str, int len, XRectangle& ink,
		    XRectangle& logical, XFontSet set, FT_TYPE *ft,
		   const QTextCodec* m )
{
    // Callers to this / this needs to be optimized.
    // Trouble is, too much caching in multiple clients will make the
    //  overall performance suffer.

#ifdef QT_XFT
    if (ft)
    {
	XGlyphInfo	info;
	unsigned short	*tmp;
	unsigned short	*u, c;
	int i;

	tmp = new unsigned short[len];
#if 0
	printf ("getExt \"");
#endif
	// #####
	// byteswap
	u = (unsigned short *) str.unicode ();
	for (i = 0; i < len; i++)
	{
	    c = u[i];
#if 0
	    printf ("%c", (char) (c >>8 ));
#endif
	    tmp[i] = htons( c );
	}
#if 0
	printf ("\"\n");
#endif
	XftTextExtents16 (QPaintDevice::x11AppDisplay(),
			  ft, tmp, len, &info);
#if 0
	if (len == 1 && tmp[0] == ' ')
	{
	    printf ("x: %d y: %d width: %d height: %d xoff: %d yoff: %d\n",
		    info.x, info.y, info.width, info.height, info.xOff,
		    info.yOff);
	    printf ("font ascent: %d descent: %d max_advance_width: %d\n",
		    ft->ascent, ft->descent, ft->max_advance_width);
	}
#endif
	ink.x = -info.x;
	ink.y = info.height - info.y;
	ink.width = info.width;
	ink.height = info.height;
	logical.x = 0;
	logical.y = -ft->ascent;
	logical.width = info.xOff;
	logical.height = ft->ascent + ft->descent;
#if 0
	if (len == 1 && tmp[0] == ' ')
	{
	    printf ("ink x: %d y: %d width: %d height: %d\n",
		    ink.x, ink.y, ink.width, ink.height);
	    printf ("log x: %d y: %d width: %d height: %d\n",
		    logical.x, logical.y, logical.width, logical.height);
	}
#endif
	delete [] tmp;
	return;
    }
#endif
    QCString x = m->fromUnicode(str,len);
    XmbTextExtents( set, x, len, &ink, &logical );
}


/*!
  Returns the left bearing of character \a ch in the font.

  The left bearing is the rightward distance of the left-most pixel
  of the character from the logical origin of the character.
  This value is negative if the pixels of the character extend
  to the left of the logical origin.

  See width(QChar) for a graphical description of this metric.

  \sa rightBearing(QChar), minLeftBearing(), width()
*/
int QFontMetrics::leftBearing(QChar ch) const
{
#ifdef QT_XFT
    XftFont	*ft = FT;
    if (ft)
	return printerAdjusted(-glyphStr(mapper(),ft,ch)->x);
#endif
    XFontStruct *f = FS;
    if ( f )
	return printerAdjusted(charStr(mapper(),f,ch)->lbearing);

    XRectangle ink, log;
    getExt(ch,1,ink,log,SET,FT,mapper());
    return printerAdjusted(LBEARING(ink,log));
}

/*!
  Returns the right bearing of character \a ch in the font.

  The right bearing is the leftward distance of the right-most pixel
  of the character from the logical origin of a subsequent character.
  This value is negative if the pixels of the character extend
  to the right of the width() of the character.

  See width() for a graphical description of this metric.

  \sa leftBearing(char), minRightBearing(), width()
*/
int QFontMetrics::rightBearing(QChar ch) const
{
#ifdef QT_XFT
    XftFont	*ft = FT;
    if (ft)
    {
	XGlyphInfo  *info = glyphStr(mapper(),ft,ch);
	int	    bearing;

	bearing = info->xOff - (-info->x + info->width);
#if 0	
	printf ("xoff %d x %d width %d bearing %d\n", 
		info->xOff, info->x, info->width, bearing);
#endif
	return printerAdjusted(bearing);
    }
#endif
    XFontStruct *f = FS;
    if ( f ) {
	XCharStruct* cs = charStr(mapper(),f,ch);
	return printerAdjusted(cs->width - cs->rbearing);
    }
    XRectangle ink, log;
    getExt(ch,1,ink,log,SET,FT,mapper());
    return printerAdjusted(RBEARING(ink,log));
}

/*!
  Returns the minimum left bearing of the font.

  This is the smallest leftBearing(char) of all characters in the font.

  Note that this function can be very slow if the font is big.

  \sa minRightBearing(), leftBearing(char)
*/
int QFontMetrics::minLeftBearing() const
{
#ifdef QT_XFT
    XftFont	*ft = FT;
    if (ft)
    {
	// ####
	return 0;
    }
#endif
    // Don't need def->lbearing, the FS stores it.
    XFontStruct *f = FS;
    if ( f )
	return printerAdjusted(f->min_bounds.lbearing);
    XFontSetExtents *ext = XExtentsOfFontSet(SET);
    return printerAdjusted(ext->max_logical_extent.x+ext->max_ink_extent.x);
}

/*!
  Returns the minimum right bearing of the font.

  This is the smallest rightBearing(char) of all characters in the
  font.

  Note that this function can be very slow if the font is big.

  \sa minLeftBearing(), rightBearing(char)
*/
int QFontMetrics::minRightBearing() const
{
#ifdef QT_XFT
    XftFont	*ft = FT;
    if (ft)
    {
	// ####
	return 0;
    }
#endif
    // Safely cast away const, as we cache rbearing there.
    QFontDef* def = (QFontDef*)spec();

    if ( def->rbearing == SHRT_MIN ) {
	XFontStruct *f = FS;
	if ( f ) {
	    if ( f->per_char ) {
		XCharStruct *c = f->per_char;
		int nc = maxIndex(f)+1;
		int mx = c->width - c->rbearing;
		for ( int i=1; i < nc; i++ ) {
		    int nmx = c[i].width - c[i].rbearing;
		    if ( nmx < mx )
			mx = nmx;
		}
		def->rbearing = mx;
	    } else {
		def->rbearing = f->max_bounds.width - f->max_bounds.rbearing;
	    }
	} else {
	    XFontSetExtents *ext = XExtentsOfFontSet(SET);
	    def->rbearing = ext->max_ink_extent.width
			-ext->max_logical_extent.width;
	}
    }

    return printerAdjusted(def->rbearing);
}


/*!
  Returns the height of the font.

  This is always equal to ascent()+descent()+1 (the 1 is for the base line).

  \sa leading(), lineSpacing()
*/

int QFontMetrics::height() const
{
#ifdef QT_XFT
    XftFont	*ft = FT;
    if (ft)
	return printerAdjusted(ft->ascent + ft->descent);
#endif
    XFontStruct *f = FS;
    if ( f )
	return printerAdjusted(f->max_bounds.ascent + f->max_bounds.descent);
    XFontSetExtents *ext = XExtentsOfFontSet(SET);
    return printerAdjusted(ext->max_ink_extent.height);
}

/*!
  Returns the leading of the font.

  This is the natural inter-line spacing.

  \sa height(), lineSpacing()
*/

int QFontMetrics::leading() const
{
#ifdef QT_XFT
    XftFont	*ft = FT;
    if (ft) {
	int l = ft->height - (ft->ascent + ft->descent);
	if (l > 0)
	    return printerAdjusted(l);
	else
	    return 0;
    }
#endif
    XFontStruct *f = FS;
    if ( f ) {
	int l = f->ascent		 + f->descent -
		f->max_bounds.ascent - f->max_bounds.descent;
	if ( l > 0 ) {
	    return printerAdjusted(l);
	} else {
	    return 0;
	}
    }
    XFontSetExtents *ext = XExtentsOfFontSet(SET);
    return printerAdjusted(ext->max_logical_extent.height
		-ext->max_ink_extent.height);
}


/*!
  Returns the distance from one base line to the next.

  This value is always equal to leading()+height().

  \sa height(), leading()
*/

int QFontMetrics::lineSpacing() const
{
#ifdef QT_XFT
    XftFont	*ft = FT;
    if (ft)
	return ft->height;
#endif
    return leading() + height();
}

/*! \overload int QFontMetrics::width( char c ) const

  \obsolete

  Provided to aid porting from Qt 1.x.
*/

/*!
  <img src="bearings.png" align=right> Returns the logical width of a
  \e ch in pixels.  This is a distance appropriate for drawing a
  subsequent character after \e ch.

  Some of the metrics are described in the image to the right.  The
  tall dark rectangle covers the logical width() of a character.  The
  shorter pale rectangles cover leftBearing() and rightBearing() of
  the characters.  Notice that the bearings of "f" in this particular
  font are both negative, while the bearings of "o" are both positive.

  \sa boundingRect()
*/

int QFontMetrics::width( QChar ch ) const
{
#ifdef QT_XFT
    XftFont	*ft = FT;
    if (ft) {
	return printerAdjusted(glyphStr(mapper(),ft,ch)->xOff);
    }
#endif
    XFontStruct *f = FS;
    if ( f ) {
	return printerAdjusted(charStr(mapper(),f,ch)->width);
    } else {
	XRectangle ink, log;
	getExt(ch,1,ink,log,SET,FT,mapper());
	return printerAdjusted(log.width);
    }
}

/*!
  Returns the width in pixels of the first \e len characters of \e str.

  If \e len is negative (the default value is), the whole string is used.

  Note that this value is \e not equal to boundingRect().width();
  boundingRect() returns a rectangle describing the pixels this string
  will cover whereas width() returns the distance to where the next string
  should be drawn.

  \sa boundingRect()
*/

int QFontMetrics::width( const QString &str, int len ) const
{
    if ( len < 0 )
	len = str.length();
#ifdef QT_XFT
    XftFont	*ft = FT;
    if (ft) {
	XGlyphInfo  info;
	{
	    unsigned short	tmp[1024];
	    unsigned short	*u, c;
	    int			i;

	    // ### why byteswap
	    u = (unsigned short *) str.unicode ();
	    for (i = 0; i < len; i++)
	    {
		c = u[i];
		tmp[i] = htons( c );
	    }
	    XftTextExtents16 (QPaintDevice::x11AppDisplay(), ft,
			      tmp, len, &info);
	}
	return printerAdjusted(info.xOff);
    }
#endif
    XFontStruct *f = FS;
    if ( f ) {
	const QTextCodec* m = mapper();
	if ( m )
	    return printerAdjusted(XTextWidth( f, m->fromUnicode(str,len),
					       len ));
	else
	    return printerAdjusted(XTextWidth16( f, (XChar2b*)str.unicode(),
						 len ));
    }
    XRectangle ink, log;
    getExt(str,len,ink,log,SET,FT,mapper());
    return printerAdjusted(log.width);
}


/*!
  Returns the bounding rectangle of the first \e len characters of \e str,
  which is the set of pixels the text would cover if drawn at (0,0).

  If \e len is negative (default value), the whole string is used.

  Note that the bounding rectangle may extend to the left of (0,0),
  e.g. for italicized fonts, and that the text output may cover \e all
  pixels in the bounding rectangle.

  Newline characters are processed as regular characters, \e not as
  linebreaks.

  Due to the different actual character heights, the height of the
  bounding rectangle of e.g. "Yes" and "yes" may be different.

  \sa width(), QPainter::boundingRect()
*/

QRect QFontMetrics::boundingRect( const QString &str, int len ) const
{
    // Values are printerAdjusted during calculations.

    if ( len < 0 )
	len = str.length();
    XFontStruct *f = FS;
    int direction;
    int ascent;
    int descent;
    XCharStruct overall;

    bool underline;
    bool strikeOut;
    if ( painter ) {
	underline = painter->cfont.underline();
	strikeOut = painter->cfont.strikeOut();
    } else {
	underline = underlineFlag();
	strikeOut = strikeOutFlag();
    }

#ifdef QT_XFT
    XftFont	*ft = FT;
    if (ft) {
	XGlyphInfo  info;
	{
	    unsigned short	tmp[1024];
	    unsigned short	*u, c;
	    int			i;

	    // ### why byteswap
	    u = (unsigned short *) str.unicode ();
	    for (i = 0; i < len; i++)
	    {
		c = u[i];
		tmp[i] = htons( c );
	    }
	    XftTextExtents16 (QPaintDevice::x11AppDisplay(), ft,
			      tmp, len, 
			      &info);
	}
	overall.lbearing = -info.x;
	overall.rbearing = info.width - info.x;
	overall.ascent = info.height - info.y;
	overall.descent = info.y;
	overall.width = info.xOff;
#if 0
	qDebug ("lbearing %d rbearing %d ascent %d descent %d width %d",
		overall.lbearing,
		overall.rbearing,
		overall.ascent,
		overall.descent,
		overall.width);
#endif
    } else
#endif

    if ( f ) {
	const QTextCodec *m = mapper();
	if ( m ) {
	    XTextExtents( f, m->fromUnicode(str,len), len, &direction, &ascent, &descent, &overall );
	} else {
	    XTextExtents16( f, (XChar2b*)str.unicode(), len, &direction, &ascent, &descent, &overall );
	}
    } else {
	XRectangle ink, log;
	getExt(str,len,ink,log,SET,FT,mapper());
	overall.lbearing = LBEARING(ink,log);
	overall.rbearing = ink.width+ink.x; // RBEARING(ink,log);
	overall.ascent = ASCENT(ink,log);
	overall.descent = DESCENT(ink,log);
	overall.width = log.width;
    }

    overall.lbearing = printerAdjusted(overall.lbearing);
    overall.rbearing = printerAdjusted(overall.rbearing);
    overall.ascent = printerAdjusted(overall.ascent);
    overall.descent = printerAdjusted(overall.descent);
    overall.width = printerAdjusted(overall.width);

    int startX = overall.lbearing;
    int width  = overall.rbearing - startX;
    ascent     = overall.ascent;
    descent    = overall.descent;
    if ( !underline && !strikeOut ) {
	width  = overall.rbearing - startX;
    } else {
	if ( startX > 0 )
	    startX = 0;
	if ( overall.rbearing < overall.width )
	   width =  overall.width - startX;
	else
	   width =  overall.rbearing - startX;
	if ( underline && len != 0 ) {
	    int ulTop = underlinePos();
	    int ulBot = ulTop + lineWidth(); // X descent is 1
	    if ( descent < ulBot )	// more than logical descent, so don't
		descent = ulBot;	// subtract 1 here!
	    if ( ascent < -ulTop )
		ascent = -ulTop;
	}
	if ( strikeOut && len != 0 ) {
	    int soTop = strikeOutPos();
	    int soBot = soTop - lineWidth(); // same as above
	    if ( descent < -soBot )
		descent = -soBot;
	    if ( ascent < soTop )
		ascent = soTop;
	}
    }
    return QRect( startX, -ascent, width, descent + ascent );
}


/*!
  Returns the width of the widest character in the font.
*/

int QFontMetrics::maxWidth() const
{
#ifdef QT_XFT
    XftFont	*ft = FT;
    if (ft)
	return printerAdjusted(ft->max_advance_width);
#endif
    XFontStruct *f = FS;
    if ( f )
	return printerAdjusted(f->max_bounds.width);
    XFontSetExtents *ext = XExtentsOfFontSet(SET);
    return printerAdjusted(ext->max_logical_extent.width);
}


/*!
  Returns the distance from the base line to where an underscore should be
  drawn.
  \sa strikeOutPos(), lineWidth()
*/

int QFontMetrics::underlinePos() const
{
    int pos = (lineWidth()*2 + 3)/6;
    if ( pos ) {
	return pos;
    } else {
	return 1;
    }
}


/*!
  Returns the distance from the base line to where the strike-out line
  should be drawn.
  \sa underlinePos(), lineWidth()
*/

int QFontMetrics::strikeOutPos() const
{
#ifdef QT_XFT
    XftFont	*ft = FT;
    if (ft) {
	int pos = ft->ascent / 3;
	if (pos)
	    return printerAdjusted(pos);
	else
	    return 1;
    }
#endif
    XFontStruct *f = FS;
    if ( f ) {
	int pos = f->max_bounds.ascent/3;
	if ( pos ) {
	    return printerAdjusted(pos);
	} else {
	    return 1;
	}
    }
    return ascent()/3;
}


/*!
  Returns the width of the underline and strike-out lines, adjusted for
  the point size of the font.
  \sa underlinePos(), strikeOutPos()
*/

int QFontMetrics::lineWidth() const
{
    if ( painter ) {
	painter->cfont.handle();
	return printerAdjusted(painter->cfont.d->fin->lineWidth());
    } else {
	return fin->lineWidth();
    }
}


/*****************************************************************************
  QFontInfo member functions
 *****************************************************************************/

const QFontDef *QFontInfo::spec() const
{
    if ( painter ) {
	painter->cfont.handle();
	return painter->cfont.d->fin->spec();
    } else {
	return fin->spec();
    }
}


/*****************************************************************************
  QFontData member functions
 *****************************************************************************/

const QTextCodec* QFontData::mapper() const
{
    return fin ? fin->mapper() : 0;
}

void* QFontData::fontSet() const
{
    return fin ? fin->fontSet() : 0;
}


/*****************************************************************************
  Internal X font functions
 *****************************************************************************/

//
// Splits an X font name into fields separated by '-'
//

bool qParseXFontName( QCString &fontName, char **tokens )
{
    if ( fontName.isEmpty() || fontName[0] != '-' ) {
	tokens[0] = 0;
	return FALSE;
    }
    int	  i;
    char *f = fontName.data() + 1;
    for ( i=0; i<fontFields && f && f[0]; i++ ) {
	tokens[i] = f;
	f = strchr( f, '-' );
	if( f )
	    *f++ = '\0';
    }
    if ( i < fontFields ) {
	for( int j=i ; j<fontFields; j++ )
	    tokens[j] = 0;
	return FALSE;
    }
    return TRUE;
}


//
// Get an array of X font names that matches a pattern
//

static char **getXFontNames( const char *pattern, int *count )
{
    static int maxFonts = 256;
    char **list;
    while( 1 ) {
	list = XListFonts( QPaintDevice::x11AppDisplay(), (char*)pattern,
			   maxFonts, count );
	// I know precisely why 32768 is 32768.
	if ( *count != maxFonts || maxFonts >= 32768 )
	    return list;
	XFreeFontNames( list );
	maxFonts *= 2;
    }
}


//
// Returns TRUE if the font exists, FALSE otherwise
//

static bool fontExists( const QString &fontName )
{
    char **fontNames;
    int	   count;
    fontNames = getXFontNames( fontName.ascii(), &count );
    XFreeFontNames( fontNames );
    return count != 0;
}


//
// Computes the line width (underline,strikeout) for the X font
// and fills in the X resolution of the font.
//

void QFontInternal::computeLineWidth()
{
    char *tokens[fontFields];
    QCString buffer( name(), 255 );		// X font name always <= 255 chars
    if ( !qParseXFontName(buffer, tokens) ) {
	lw   = 1;                   // name did not conform to X LFD
	xres = QPaintDevice::x11AppDpiX();
	return;
    }
    int weight = qFontGetWeight( tokens[Weight_] );
    int pSize  = atoi( tokens[PointSize] ) / 10;
    int ry = atoi( tokens[ResolutionY] );
    if ( ry != QPaintDevice::x11AppDpiY() )
	pSize = ( 2*pSize*ry + QPaintDevice::x11AppDpiY() )
	    / ( QPaintDevice::x11AppDpiY() * 2 );
    QCString tmp = tokens[ResolutionX];
    bool ok;
    xres = tmp.toInt( &ok );
    if ( !ok || xres == 0 )
	xres = QPaintDevice::x11AppDpiX();
    int score = pSize*weight;		// ad hoc algorithm
    lw = ( score ) / 700;
    if ( lw < 2 && score >= 1050 )	// looks better with thicker line
	lw = 2;				//   for small pointsizes
    if ( lw == 0 )
	lw = 1;
}


//
// Converts a weight string to a value
//

int qFontGetWeight( const QCString &weightString, bool adjustScore )
{
    // Test in decreasing order of commonness
    //
    if ( weightString == "medium" )       return QFont::Normal;
    else if ( weightString == "bold" )    return QFont::Bold;
    else if ( weightString == "demibold") return QFont::DemiBold;
    else if ( weightString == "black" )   return QFont::Black;
    else if ( weightString == "light" )   return QFont::Light;

    QCString s = weightString;
    s = s.lower();
    if ( s.contains("bold") ) {
	if ( adjustScore )
	    return (int) QFont::Bold - 1;  // - 1, not sure that this IS bold
	else
	    return (int) QFont::Bold;
    }
    if ( s.contains("light") ) {
	if ( adjustScore )
	    return (int) QFont::Light - 1; // - 1, not sure that this IS light
       else
	    return (int) QFont::Light;
    }
    if ( s.contains("black") ) {
	if ( adjustScore )
	    return (int) QFont::Black - 1; // - 1, not sure this IS black
	else
	    return (int) QFont::Black;
    }
    if ( adjustScore )
	return (int) QFont::Normal - 2;	   // - 2, we hope it's close to normal

    return (int) QFont::Normal;
}

/*!
  Returns the logical pixel height of characters in the font if shown on
  the screen.
*/
int QFont::pixelSize() const
{
    return ( d->req.pointSize*QPaintDevice::x11AppDpiY() + 360 ) / 720;
}

/*!
  Sets the logical pixel height of characters in the font if shown on
  the screen.
*/
void QFont::setPixelSizeFloat( float pixelSize )
{
    setPointSizeFloat( pixelSize * 72.0 / QPaintDevice::x11AppDpiY() );
}


/****************************************************************************
** $Id$
**
** Definition of internal rich text classes
**
** Created : 990124
**
** Copyright (C) 1999-2000 Trolltech AS.  All rights reserved.
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

#ifndef QRICHTEXT_P_H
#define QRICHTEXT_P_H


//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qrichtext.cpp.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//
//

#ifndef QT_H
#include "qstylesheet.h"
#include "qstring.h"
#include "qpixmap.h"
#include "qmap.h"
#include "qapplication.h"
#include "qmime.h"
#include "qdict.h"
#include "qcolor.h"
#include "qfont.h"
#include "qlist.h"
#include "qlayout.h"
#endif // QT_H

#ifndef QT_NO_RICHTEXT

class QTextCustomItem;
class QTextFormatCollection;
class QRichText;
class QTextView;
class QTextFlow;
class QTextTable;


class QtTriple
{
public:
    QtTriple(int a_=0, int b_=0, int c_=0):a(a_),b(b_),c(c_){};
    int a,b,c;
};
bool operator!=( const QtTriple &t1, const QtTriple &t2 );
bool operator<( const QtTriple &t1, const QtTriple &t2 );
bool operator>=( const QtTriple &t1, const QtTriple &t2 );


class QTextCharFormat : public QShared
{
    friend class QTextFormatCollection;

public:
    QTextCharFormat();
    QTextCharFormat( const QTextCharFormat &format );
    QTextCharFormat( const QFont &f, const QColor &c );
    QTextCharFormat &operator=( const QTextCharFormat &fmt );
    bool operator==( const QTextCharFormat &format );
    virtual ~QTextCharFormat();

    QTextCharFormat makeTextFormat( const QStyleSheetItem *style, const QMap<QString,QString>& attr,
				     QTextCustomItem* item = 0) const;

    QColor color() const;
    QFont font() const;
    QString anchorHref() const;
    QString anchorName() const;
    bool useLinkColor() const { return linkColor; }
    
    bool isAnchor() const;

    QTextCharFormat formatWithoutCustom();

    QTextCustomItem *customItem() const;

private:
    QFont font_;
    QColor color_;
    QString key;
    int logicalFontSize;
    int stdPointSize;
    QString anchor_href;
    QString anchor_name;
    void createKey();
    QTextFormatCollection* parent;
    QTextCustomItem* custom;
    uint linkColor : 1;
};


class QTextFormatCollection
{
    friend class QTextCharFormat;

public:
    QTextFormatCollection();

    QTextCharFormat*  registerFormat( const QTextCharFormat &format );
    void unregisterFormat( const QTextCharFormat &format  );

protected:
    QDict<QTextCharFormat > cKey;
    QTextCharFormat* lastRegisterFormat;
};


class QTextOptions {
public:
    QTextOptions( const QBrush* p = 0, QColor lc = Qt::blue, bool lu = TRUE )
	:paper( p ), linkColor( lc ), linkUnderline( lu ), offsetx( 0 ), offsety( 0 )
    {
    };
    const QBrush* paper;
    QColor linkColor;
    bool linkUnderline;
    int offsetx;
    int offsety;
    void erase( QPainter* p, const QRect& r ) const;
    bool inSelection( const QtTriple& ) const;
    QtTriple selstart;
    QtTriple selend;
};


class QTextCustomItem : public QShared
{
public:
    QTextCustomItem()
	: xpos(0), ypos(0), width(-1), height(0)
    {}
    virtual ~QTextCustomItem() {}
    virtual void draw(QPainter* p, int x, int y,
		      int ox, int oy, int cx, int cy, int cw, int ch,
		      QRegion& backgroundRegion, const QColorGroup& cg,
		      const QTextOptions& to ) = 0;

    virtual void realize( QPainter* ) { width = 0; }

    enum Placement { PlaceInline = 0, PlaceLeft, PlaceRight };
    virtual Placement placement() const { return PlaceInline; }
    bool placeInline() { return placement() == PlaceInline; }

    virtual bool breakLine() const { return ownLine(); }
    enum Clear { ClearNone, ClearLeft, ClearRight, ClearBoth }; //  move to QStyleSheetItem?
    virtual Clear clearBehind() const { return ClearNone; }

    virtual bool noErase() const { return FALSE; };
    virtual bool expandsHorizontally() const { return FALSE; }
    virtual bool ownLine() const { return expandsHorizontally(); };
    virtual void resize( QPainter*, int nwidth ){ width = nwidth; };

    virtual bool isTable() const { return FALSE; }

    int xpos; // used for floating items
    int ypos; // used for floating items
    int width;
    int height;
};


class QTextHorizontalLine : public QTextCustomItem
{
public:
    QTextHorizontalLine();
    ~QTextHorizontalLine();
    void realize( QPainter* );
    void draw(QPainter* p, int x, int y,
	      int ox, int oy, int cx, int cy, int cw, int ch,
	      QRegion& backgroundRegion, const QColorGroup& cg, const QTextOptions& to );

    bool expandsHorizontally() const { return TRUE; }
private:
};

class QTextLineBreak : public QTextCustomItem
{
public:
    QTextLineBreak(const QMap<QString, QString> &attr );
    ~QTextLineBreak();

    bool breakLine() const { return TRUE; }
    Clear clearBehind() const { return clr; }

    void draw(QPainter* , int , int ,
	      int, int, int, int, int, int,
	      QRegion& , const QColorGroup& , const QTextOptions&  ){}
private:
    Clear clr;
};


class QTextRichString
{
    friend class QRichTextFormatter;
    struct Item {
	Item()
	: width(-1),newline(0),format(0)
	{
	};
	~Item() {
	};
 	int base;
	signed int width : 30;
	uint newline : 1;
	QTextCharFormat* format;
	QString c;
    };
    Item* items;
    int store;
    int len;

public:
    QTextRichString( QTextFormatCollection* fmt );
    QTextRichString( const QTextRichString &other );
    QTextRichString& operator=( const QTextRichString &other );
    ~QTextRichString();

    int length() const;
    bool isEmpty() const;
    void remove( int index, int len );
    void insert( int index, const QString& c, const QTextCharFormat& fmt );
    void append( const QString& c,const  QTextCharFormat& fmt );
    void clear();

    QString charAt( int index ) const;
    QString& getCharAt( int index );
    QTextCharFormat *formatAt( int index ) const;
    bool haveSameFormat( int index1, int index2 ) const;

    bool isCustomItem( int index ) const;
    QTextCustomItem* customItemAt( int index ) const;

    QTextFormatCollection* formats; // make private

private:
    void setLength( int l );
};


class QTextParagraph
{
public:
    QTextParagraph( QTextParagraph* p, QTextFormatCollection* formatCol, const QTextCharFormat& fmt,
	   const QStyleSheetItem *stl, const QMap<QString, QString> &attr );

    QTextParagraph( QTextParagraph* p, QTextFormatCollection* formatCol, const QTextCharFormat& fmt,
	   const QStyleSheetItem *stl );

    ~QTextParagraph();

    QTextParagraph* parent;
    QTextFormatCollection* formats;
    QTextCharFormat format;
    QTextRichString text;
    const QStyleSheetItem* style;
    QMap<QString, QString> attributes_;
    QTextParagraph* child;
    QTextParagraph* prev;
    QTextParagraph* next;

    QTextParagraph* nextInDocument() const;
    QTextParagraph* prevInDocument() const;

    QTextParagraph* lastChild() const;

    inline QMap<QString, QString> attributes()  const
    {
	return attributes_;
    }

    int ypos;
    int height;
    bool dirty;
    bool selected;
    int id;

    QTextCustomItem::Clear clear;

    QTextFlow* flow() const;

    inline int margin(QStyleSheetItem::Margin m) const
    {
	if (style->margin(m) != QStyleSheetItem::Undefined)
	    return style->margin(m);
	return 0;
    }

    inline int topMargin() const
    {
	int m = margin( QStyleSheetItem::MarginTop );
	if ( !prev && parent )
	    m += parent->topMargin();
	return m;
    }

    inline int bottomMargin() const
    {
	int m = margin( QStyleSheetItem::MarginBottom );
	if ( !next && parent )
	    m += parent->bottomMargin();
	return m;
    }

    int labelMargin() const;

    inline int totalMargin(QStyleSheetItem::Margin m) const
    {
	int tm = parent? parent->totalMargin( m ) : 0;
	if (style->margin(m) != QStyleSheetItem::Undefined)
	    tm += style->margin(m);
	return tm;
    }

    inline int totalLabelMargin() const
    {
	int tlm = parent? parent->totalLabelMargin() : 0;
	tlm += labelMargin();
	return tlm;
    }

    int numberOfSubParagraph( QTextParagraph* subparagraph, bool onlyListItems);
    QStyleSheetItem::ListStyle listStyle();
    inline int alignment() const
    {
	if ( align != QStyleSheetItem::Undefined )
	    return align;
	if ( style->alignment() != QStyleSheetItem::Undefined )
	    return style->alignment();
	return parent?parent->alignment():QStyleSheetItem::AlignLeft;
    }

    void invalidateLayout();

private:
    void init();
    int align;

protected:
    QTextFlow* flow_;
};


class QTextImage : public QTextCustomItem
{
public:
    QTextImage(const QMap<QString, QString> &attr, const QString& context,
		       const QMimeSourceFactory &factory);
    ~QTextImage();

    Placement placement() const { return place; }
    void realize( QPainter* );

    void draw(QPainter* p, int x, int y,
	      int ox, int oy, int cx, int cy, int cw, int ch,
	      QRegion& backgroundRegion, const QColorGroup& cg, const QTextOptions& to );

private:
    QRegion* reg;
    QPixmap pm;
    Placement place;
    int tmpwidth;
    QString imgId;
};


class QRichTextFormatter
{
public:
    QRichTextFormatter(QRichText& document );
    ~QRichTextFormatter();


    QTextParagraph* paragraph;
    QTextFlow* flow;
    void update( QPainter* p = 0);
//     void updateParagraph( QPainter* );
    int first;
    int last;
    int current;
    int currentx;
    int currentasc;
    int currentdesc;
    int currentoffset;
    int currentoffsetx;
    bool atEnd() const;
    bool pastEnd() const;
    bool atEndOfLine() const;
    bool pastEndOfLine() const;
    void gotoParagraph( QPainter* p, QTextParagraph* b );

    void initParagraph( QPainter* p, QTextParagraph* b );
    bool updateLayout( QPainter* p = 0, int ymax = -1 );


    void makeLineLayout( QPainter* p = 0 );
    bool gotoNextLine( QPainter* p = 0 );
    void gotoLineStart( QPainter* p = 0 );
    void drawLine( QPainter* p, int ox, int oy,
		   int cx, int cy, int cw, int ch,
		   QRegion& backgroundRegion,
		   const QColorGroup& cg, const QTextOptions& to );
    void drawLabel( QPainter* p, QTextParagraph* par, int x, int y, int w, int h, int ox, int oy,
		   QRegion& backgroundRegion,
		   const QColorGroup& cg, const QTextOptions& to );
    void gotoNextItem( QPainter* p = 0 );

    void updateCharFormat( QPainter* p = 0 );

    int y() const { return y_; }
    int x() const { return currentx + currentoffsetx; }
    QTextCharFormat* format() const;
    int width;
    int widthUsed;
    int height;
    int base;
    int fill;
    int lmargin;
    int rmargin;

    int static_lmargin;
    int static_rmargin;
    int static_labelmargin;

    QtTriple position() const;
    QRect lineGeometry() const;
    void right( QPainter* p = 0 );
    void left( QPainter* p = 0 );
    bool goTo( QPainter* p, int xpos, int ypos );

    bool rightOneItem( QPainter* p = 0 );
    bool lazyRightOneItem();
    QRichText* doc;

private:
    int y_;
    QTextCharFormat* formatinuse;
    int alignment;
    double xscale, yscale;
    int adjustHorizontalMargins( QTextCustomItem::Clear );
};


// moved from qrichtext.cpp for GCC 2.7.* compatibility
class QTextTableCell : public QLayoutItem
{
public:
    QTextTableCell(QTextTable* table,
      int row, int column,
      const QMap<QString, QString> &attr,
      const QStyleSheetItem* style,
      const QTextCharFormat& fmt, const QString& context,
      const QMimeSourceFactory &factory, const QStyleSheet *sheet, const QString& doc, int& pos );
    ~QTextTableCell();
    QSize sizeHint() const ;
    QSize minimumSize() const ;
    QSize maximumSize() const ;
    QSizePolicy::ExpandData expanding() const;
    bool isEmpty() const;
    void setGeometry( const QRect& ) ;
    QRect geometry() const;

    bool hasHeightForWidth() const;
    int heightForWidth( int ) const;

    void realize();

    int row() const { return row_; }
    int column() const { return col_; }
    int rowspan() const { return rowspan_; }
    int colspan() const { return colspan_; }
    int stretch() const { return stretch_; }

    QRichText* richText()  const { return richtext; }
    QTextTable* table() const { return parent; }

    void draw( int x, int y,
	       int ox, int oy, int cx, int cy, int cw, int ch,
	       QRegion& backgroundRegion, const QColorGroup& cg, const QTextOptions& to );

private:
    QPainter* painter() const;
    QRect geom;
    QTextTable* parent;
    QRichText* richtext;
    QBrush* background;
    int row_;
    int col_;
    int rowspan_;
    int colspan_;
    int stretch_;
    int maxw;
    int minw;
    bool hasFixedWidth;
};


class QRichTextIterator
{
public:
    QRichTextIterator( QRichText& );

    QtTriple position() const;
    QString text() const;
    QRect lineGeometry() const;
    QTextCharFormat* format() const;
    QTextParagraph* outmostParagraph() const { return fc.paragraph; }

    bool goTo( const QPoint& pos );
    void goTo( const QtTriple& position );
    bool right( bool doFormat = TRUE );

private:
    struct Item {
	Item( const QRichTextFormatter& cur, QList<QTextTableCell> &cells )
	    : fc( cur ), it( cells )
    {}
	QRichTextFormatter fc;
	QListIterator<QTextTableCell> it;
    };
    QRichText& doc;
    QRichTextFormatter fc;
    QList<Item> stack;
    bool dirty;
    void update();
};


bool operator>( const QRichTextIterator &i1, const QRichTextIterator &i2 );

class QTextFlow
{
public:
    QTextFlow();
    ~QTextFlow();

    void initialize( int w );

    int adjustLMargin( int yp, int margin, int space );
    int adjustRMargin( int yp, int margin, int space );

    void registerFloatingItem( QTextCustomItem* item, bool right = FALSE );
    void drawFloatingItems(QPainter* p,
			   int ox, int oy, int cx, int cy, int cw, int ch,
			   QRegion& backgroundRegion, const QColorGroup& cg, const QTextOptions& to );
    void adjustFlow( int  &yp, int w, int h, bool pages = TRUE );

    int width;
    int widthUsed;
    int height;

    int pagesize;

private:
    QList<QTextCustomItem> leftItems;
    QList<QTextCustomItem> rightItems;

};


class QRichText : public QTextParagraph
{
public:
    QRichText( const QString &doc, const QFont& fnt,
	       const QString& context = QString::null, int margin = 8,
	       const QMimeSourceFactory* factory = 0,
	       const QStyleSheet* sheet = 0 );
    QRichText( const QMap<QString, QString> &attr, const QString &doc, int& pos,
	       const QStyleSheetItem* style, const QTextCharFormat& fmt,
	       const QString& context = QString::null,
	       int margin = 8, const QMimeSourceFactory* factory = 0, const QStyleSheet* sheet = 0 );
    ~QRichText();

    bool isValid() const;

    QString context() const;
    void dump();

    void draw(QPainter* p, int x, int y,
	      int ox, int oy, int cx, int cy, int cw, int ch,
	      QRegion& backgroundRegion, const QColorGroup& cg, const QTextOptions& to );

    void doLayout( QPainter* p, int nwidth );
    QString anchorAt( const QPoint& pos ) const;

    void append( const QString& txt, const QMimeSourceFactory* factory = 0, const QStyleSheet* sheet = 0 );

    QTextParagraph* getParBefore( int y ) const;

private:
    void init( const QString& doc, int& pos );

    bool parse (QTextParagraph* current, const QStyleSheetItem* cursty, QTextParagraph* dummy,
		QTextCharFormat fmt, const QString& doc, int& pos,
		QStyleSheetItem::WhiteSpaceMode = QStyleSheetItem::WhiteSpaceNormal );

    bool eatSpace(const QString& doc, int& pos, bool includeNbsp = FALSE );
    bool eat(const QString& doc, int& pos, QChar c);
    bool lookAhead(const QString& doc, int& pos, QChar c);
    QString parseOpenTag(const QString& doc, int& pos, QMap<QString, QString> &attr, bool& emptyTag);
    QString parseCloseTag( const QString& doc, int& pos );
    bool eatCloseTag(const QString& doc, int& pos, const QString& open);
    QChar parseHTMLSpecialChar(const QString& doc, int& pos);
    QString parseWord(const QString& doc, int& pos, bool insideTag = FALSE, bool lower = FALSE);
    QString parsePlainText(const QString& doc, int& pos, QStyleSheetItem::WhiteSpaceMode wsm, bool justOneWord);
    bool hasPrefix(const QString& doc, int pos, QChar c);
    bool hasPrefix(const QString& doc, int pos, const QString& s);
    bool valid;
    QString contxt;
    QStyleSheetItem* base;
    const QStyleSheet* sheet_;
    const QMimeSourceFactory* factory_;
    static bool space_;
    const QStyleSheetItem* nullstyle;

    QTextCustomItem* parseTable( const QMap<QString, QString> &attr, const QTextCharFormat &fmt, const QString &doc, int& pos );

    bool keep_going;
    QTextParagraph* b_cache;
};


inline QColor QTextCharFormat::color() const
{
    return color_;
}

inline QFont QTextCharFormat::font() const
{
    return font_;
}

inline QString QTextCharFormat::anchorHref() const
{
    return anchor_href;
}

inline QString QTextCharFormat::anchorName() const
{
    return anchor_name;
}

inline QTextCustomItem * QTextCharFormat::customItem() const
{
    return custom;
}

inline bool QTextCharFormat::isAnchor() const
{
    return !anchor_href.isEmpty()  || !anchor_href.isEmpty();
}

inline int QTextRichString::length() const
{
    return len;
}

inline void QTextRichString::append( const QString& c,const  QTextCharFormat& fmt )
{
	insert( length(), c, fmt);
}

inline bool QTextRichString::isEmpty() const
{
    return len == 0;
}

inline QString QTextRichString::charAt( int index ) const
{
    return items[index].c;
}

inline QTextCharFormat *QTextRichString::formatAt( int index ) const
{
    return items[index].format;
}

inline QTextCustomItem* QTextRichString::customItemAt( int index ) const
{
    return items[index].format->customItem();
}

inline bool QTextRichString::isCustomItem( int index ) const
{
    return customItemAt( index ) != 0;
}

inline bool QTextRichString::haveSameFormat( int index1, int index2 ) const
{
    return items[index1].format == items[index2].format;
}

inline QTextCharFormat* QRichTextFormatter::format() const
{
    return paragraph->text.formatAt( current );
}

#endif // QT_FEATURE_RICHTEXT

#endif // QRICHTEXT_P_H

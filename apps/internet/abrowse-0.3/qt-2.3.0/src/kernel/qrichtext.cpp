/****************************************************************************
** $Id$
**
** Implementation of the internal Qt classes dealing with rich text
**
** Created : 990101
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

#include "qrichtext_p.h"

#ifndef QT_NO_RICHTEXT

#include "qpainter.h"
#include "qfile.h"
#include "qtextstream.h"
#include "qtimer.h"
#include "qimage.h"
#include "qdragobject.h"
#include "qdrawutil.h"
#include "qpaintdevicemetrics.h"

#include <stdio.h>
#include <limits.h>


static int qt_text_paragraph_id = 0;

bool isBreakable( const QChar *c, bool checkNext = FALSE )
{
    char ch = c->latin1();
    if ( ! ch ) {
	// not latin1, need to do more sophisticated checks for asian fonts
	uchar row = c->row();
	if ( row < 0x11 ) // no asian font
	    return FALSE;
	if ( row > 0x2d && row < 0xfb || row == 0x11 ) {
	    if ( !checkNext )
		return TRUE;
	    else
		return !(c+1)->isPunct();
	}
    } else {
	if ( ch == ' ' || ch == '\n' )
	    return TRUE;
    }
    return FALSE;
}

class QTextTable: public QTextCustomItem
{
public:
    QTextTable(const QMap<QString, QString> &attr);
    ~QTextTable();
    void realize( QPainter* );
    void verticalBreak( int  y, QTextFlow* flow );
    void draw(QPainter* p, int x, int y,
	      int ox, int oy, int cx, int cy, int cw, int ch,
	      QRegion& backgroundRegion, const QColorGroup& cg, const QTextOptions& to );

    bool noErase() const { return TRUE; };
    bool expandsHorizontally() const { return TRUE; }
    Placement placement() const { return place; }
    bool isTable() const { return TRUE; }
    void resize( QPainter*, int nwidth );
    QString anchorAt( QPainter* p, int x, int y );

private:
    QGridLayout* layout;
    QList<QTextTableCell> cells;

    friend class QTextTableCell;
    friend class QRichTextIterator;
    void addCell( QTextTableCell* cell );
    QPainter* painter;
    int cachewidth;
    int fixwidth;
    int cellpadding;
    int cellspacing;
    int border;
    int outerborder;
    int stretch;
    int innerborder;

    Placement place;
};


static double scale_factor( double v )
{
    return v/96;
}


void QTextOptions::erase( QPainter* p, const QRect& r ) const
{
    if ( !paper )
	return;
    if ( p->device()->devType() == QInternal::Printer )
	return;
    if ( paper->pixmap() )
	p->drawTiledPixmap( r, *paper->pixmap(),
			    QPoint(r.x()+offsetx, r.y()+offsety) );
    else
	p->fillRect(r, *paper );
}

bool QTextOptions::inSelection( const QtTriple& pos ) const
{
    QtTriple t = selstart; t.c = 0;
    return (pos >= t )  && (pos < selend);
}

struct QPixmapInt
{
    QPixmapInt() : ref( 0 ) {}
    QPixmap pm;
    int	    ref;
};

static QMap<QString, QPixmapInt> *pixmap_map = 0;

QTextImage::QTextImage(const QMap<QString, QString> &attr, const QString& context,
		       const QMimeSourceFactory &factory)
{
    width = height = 0;
    if ( attr.contains("width") )
	width = attr["width"].toInt();
    if ( attr.contains("height") )
	height = attr["height"].toInt();

    reg = 0;
    QString imageName = attr["src"];

    if (!imageName)
	imageName = attr["source"];

    if ( !imageName.isEmpty() ) {
	imgId = QString( "%1,%2,%3" ).arg( imageName ).arg( width ).arg( height );
	if ( !pixmap_map )
	    pixmap_map = new QMap<QString, QPixmapInt>;
	if ( pixmap_map->contains( imgId ) ) {
	    QPixmapInt& pmi = pixmap_map->operator[](imgId);
	    pm = pmi.pm;
	    pmi.ref++;
	    width = pm.width();
	    height = pm.height();
	} else {
	    QImage img;
	    const QMimeSource* m =
		factory.data( imageName, context );
	    if ( !m ) {
		qWarning("QTextImage: no mimesource for %s", imageName.latin1() );
	    }
	    else {
		if ( !QImageDrag::decode( m, img ) ) {
		    qWarning("QTextImage: cannot decode %s", imageName.latin1() );
		}
	    }
	
	    if ( !img.isNull() ) {
		if ( width == 0 ) {
		    width = img.width();
		    if ( height != 0 ) {
			width = img.width() * height / img.height();
		    }
		}
		if ( height == 0 ) {
		    height = img.height();
		    if ( width != img.width() ) {
			height = img.height() * width / img.width();
		    }
		}
		
		if ( img.width() != width || img.height() != height ){
		    img = img.smoothScale(width, height);
		    width = img.width();
		    height = img.height();
		}
		pm.convertFromImage( img );
	    }
	    if ( !pm.isNull() ) {
		QPixmapInt& pmi = pixmap_map->operator[](imgId);
		pmi.pm = pm;
		pmi.ref++;
	    }
	}
	if ( pm.mask() ) {
	    QRegion mask( *pm.mask() );
	    QRegion all( 0, 0, pm.width(), pm.height() );
	    reg = new QRegion( all.subtract( mask ) );
	}
    }

    if ( pm.isNull() && (width*height)==0 )
	width = height = 50;

    place = PlaceInline;
    if ( attr["align"] == "left" )
	place = PlaceLeft;
    else if ( attr["align"] == "right" )
	place = PlaceRight;

    tmpwidth = width;
    width = -1; // force realize
}

QTextImage::~QTextImage()
{
    if ( pixmap_map && pixmap_map->contains( imgId ) ) {
	QPixmapInt& pmi = pixmap_map->operator[](imgId);
	pmi.ref--;
	if ( !pmi.ref ) {
	    pixmap_map->remove( imgId );
	    if ( pixmap_map->isEmpty() ) {
		delete pixmap_map;
		pixmap_map = 0;
	    }
	}
    }
}

void QTextImage::realize( QPainter* p )
{
    width = tmpwidth;
    if ( !p || p->device()->devType() != QInternal::Printer )
	return;
    QPaintDeviceMetrics metrics(p->device());
    width = int( width * scale_factor( metrics.logicalDpiX() ) );
    height = int( height * scale_factor( metrics.logicalDpiY() ) );
}

void QTextImage::draw(QPainter* p, int x, int y,
		    int ox, int oy, int , int , int , int ,
		       QRegion& backgroundRegion, const QColorGroup& cg, const QTextOptions& /*to*/)
{
    if ( pm.isNull() ) {
	p->fillRect( x-ox , y-oy, width, height,  cg.dark() );
	return;
    }

    if ( p->device()->devType() == QInternal::Printer ) {
#ifndef QT_NO_TRANSFORMATIONS
	p->saveWorldMatrix();
	QPaintDeviceMetrics metrics(p->device());
	p->translate( x-ox, y-oy );
	p->scale( scale_factor( metrics.logicalDpiY() ),
		  scale_factor( metrics.logicalDpiY() ) );
	p->drawPixmap( 0, 0, pm );
	p->restoreWorldMatrix();
#else
	p->drawPixmap( x-ox, y-oy, pm );
#endif
	return;
    }

    QRect r( x-ox, y-oy, pm.width(), pm.height() );
    backgroundRegion = backgroundRegion.subtract( r );

    if ( reg ){
	QRegion tmp( *reg );
	tmp.translate( x-ox, y-oy );
	backgroundRegion = backgroundRegion.unite( tmp );
    }
    p->drawPixmap( x-ox , y-oy, pm );
}

void QTextHorizontalLine::realize( QPainter* p )
{
    if ( !p || p->device()->devType() != QInternal::Printer )
	return;
    QPaintDeviceMetrics metrics(p->device());
    height = int( height * scale_factor( metrics.logicalDpiY() ) );
}


QTextHorizontalLine::QTextHorizontalLine()
{
    height = 8;
}


QTextHorizontalLine::~QTextHorizontalLine()
{
}


void QTextHorizontalLine::draw(QPainter* p, int x, int y,
				//int ox, int oy, int cx, int cy, int cw, int ch,
				int ox, int oy, int , int , int , int ,
				QRegion&, const QColorGroup& cg, const QTextOptions& to )
{
    QRect r( x-ox, y-oy, width, height);
    to.erase( p, r );
    if ( p->device()->devType() == QInternal::Printer ) {
	QPen oldPen = p->pen();
	p->setPen( QPen( cg.text(), height/8 ) );
	p->drawLine( r.left()-1, y-oy+height/2, r.right()+1, y-oy+height/2 );
	p->setPen( oldPen );
    } else {
	qDrawShadeLine( p, r.left()-1, y-oy+height/2, r.right()+1, y-oy+height/2, cg, TRUE, height/8 );
    }
}

QTextLineBreak::QTextLineBreak(const QMap<QString, QString> &attr )
{
    clr = QTextCustomItem::ClearNone;
    if ( attr.contains("clear") ) {
 	QString s = attr["clear"].lower();
 	if ( s  == "left" )
 	    clr = QTextCustomItem::ClearLeft;
 	else if ( s == "right" )
 	    clr = QTextCustomItem::ClearRight;
	else if ( s == "both" || s == "all")
	    clr = QTextCustomItem::ClearBoth;
    }
}

QTextLineBreak::~QTextLineBreak()
{
}

//************************************************************************

bool QRichText::space_ = FALSE;


QRichText::QRichText( const QString &doc, const QFont& font,
		      const QString& context,
		      int margin, const QMimeSourceFactory* factory,
		      const QStyleSheet* sheet )
    : QTextParagraph( 0, new QTextFormatCollection(),
		      QTextCharFormat( font, QColor() ), new QStyleSheetItem(0,"") )
{
    base = (QStyleSheetItem*)style;
    contxt = context;

    // for access during parsing only
    factory_ = factory? factory : QMimeSourceFactory::defaultFactory();
    // for access during parsing only
    sheet_ = sheet? sheet : (QStyleSheet*)QStyleSheet::defaultSheet();

    int pos = 0;

    //set up base style

    base->setDisplayMode(QStyleSheetItem::DisplayBlock);
    base->setFontFamily( font.family() );
    base->setFontItalic( font.italic() );
    base->setFontUnderline( font.underline() );
    base->setFontWeight( font.weight() );
    base->setFontSize( font.pointSize() );
    base->setLogicalFontSize( 3 );
    base->setMargin( QStyleSheetItem::MarginAll, margin );

    keep_going = TRUE;
    init( doc, pos );

    // clear references that are no longer needed
    factory_ = 0;
    sheet_ = 0;
}

// constructor for nested text in text (tables, etc.)
QRichText::QRichText( const QMap<QString, QString> &attr, const QString &doc, int& pos,
			const QStyleSheetItem* basestyle, const QTextCharFormat& fmt,
			const QString& context,
			int margin,  const QMimeSourceFactory* factory, const QStyleSheet* sheet  )
    :QTextParagraph( 0, new QTextFormatCollection(),
		     QTextCharFormat( fmt ), new QStyleSheetItem(*basestyle), attr )
{
    base = (QStyleSheetItem*)style;
    contxt = context;

    // for access during parsing only
    factory_ = factory? factory : QMimeSourceFactory::defaultFactory();
    // for access during parsing only
    sheet_ = sheet? sheet : (QStyleSheet*)QStyleSheet::defaultSheet();

    base->setDisplayMode(QStyleSheetItem::DisplayBlock);
    base->setMargin( QStyleSheetItem::MarginAll, margin );

    keep_going = FALSE;
    init( doc, pos );

     // clear references that are no longer needed
    factory_ = 0;
    sheet_ = 0;
}


void QRichText::init( const QString& doc, int& pos )
{
    if ( !flow_ )
	flow_ = new QTextFlow();

    nullstyle = sheet_->item("");

    valid = TRUE;
    // for access during parsing only
    space_ = FALSE;
    if ( !keep_going )
	parse(this, style, 0, format, doc, pos);
    else  do {
	parse(this, style, 0, format, doc, pos);
	// missplaced close tags may kick us out of the parser (auto
	// recover failure), simply jump over the tag and continue
	// until the very end
	int oldpos = pos;
	if ( pos < (int) doc.length()-1
	     && (hasPrefix(doc, pos, QChar('<')) ) ) {
	    if ( hasPrefix(doc, pos+1, QChar('/')) )
		(void) parseCloseTag( doc, pos );
	    else {
		QMap<QString, QString> attr;
		bool emptyTag = FALSE;
		(void) parseOpenTag( doc, pos, attr, emptyTag );
	    }
	}
	if ( pos == oldpos )
	    pos++;
    } while ( pos < (int) doc.length()-1 );
    b_cache = 0;
}

QRichText::~QRichText()
{
    delete base;
    delete flow_;
}

void QRichText::dump()
{
}

bool QRichText::isValid() const
{
    return valid;
}

/*!
  Returns the context of the rich text document. If no context has been specified
  in the constructor, a null string is returned.
*/
QString QRichText::context() const
{
    return contxt;
}

#define ENSURE_ENDTOKEN     if ( curstyle->displayMode() == QStyleSheetItem::DisplayBlock \
		     || curstyle->displayMode() == QStyleSheetItem::DisplayListItem ){ \
		    (dummy?dummy:current)->text.append( "\n", fmt ); \
		}

#define CLOSE_TAG (void) eatSpace(doc, pos); \
		int recoverPos = pos; \
		valid = (hasPrefix(doc, pos, QChar('<')) \
			 && hasPrefix(doc, pos+1, QChar('/')) \
			 && eatCloseTag(doc, pos, tagname) ); \
		if (!valid) { \
		    pos = recoverPos; \
		    valid = TRUE; \
		    return TRUE; \
		}

#define PROVIDE_DUMMY	if ( current->child && !dummy ) { \
		    dummy = new QTextParagraph( current, formats, fmt,  nullstyle ); \
		    QTextParagraph* it = current->child; \
		    while ( it->next ) \
			it = it->next; \
		    it->next = dummy; \
		    dummy->prev = it; \
		}

void QRichText::append( const QString& txt, const QMimeSourceFactory* factory, const QStyleSheet* sheet  )
{
    // for access during parsing only
    factory_ = factory? factory : QMimeSourceFactory::defaultFactory();
    // for access during parsing only
    sheet_ = sheet? sheet : (QStyleSheet*)QStyleSheet::defaultSheet();
    // for access during parsing only
    space_ = FALSE;
    int pos = 0;
    lastChild()->invalidateLayout(); // fix bottom border
    parse( this, style, 0, format, txt, pos );
    // clear references that are no longer needed
    factory_ = 0;
    sheet_ = 0;
}


bool QRichText::parse (QTextParagraph* current, const QStyleSheetItem* curstyle, QTextParagraph* dummy,
		       QTextCharFormat fmt, const QString &doc, int& pos, QStyleSheetItem::WhiteSpaceMode wsm )
{
    if ( wsm != QStyleSheetItem::WhiteSpacePre && !space_)
	space_ = !eatSpace(doc, pos);
    while ( valid && pos < int(doc.length() )) {
	int beforePos = pos;
	if (hasPrefix(doc, pos, QChar('<')) ){
	    if (hasPrefix(doc, pos+1, QChar('/'))) {
		ENSURE_ENDTOKEN
		if ( curstyle->isAnchor() ) {
		    PROVIDE_DUMMY
		    (dummy?dummy:current)->text.append( "", fmt );
		}
		return TRUE;
	    }
	    QMap<QString, QString> attr;
	    bool emptyTag = FALSE;
	    QString tagname = parseOpenTag(doc, pos, attr, emptyTag);

	    const QStyleSheetItem* nstyle = sheet_->item(tagname);
 	    if ( nstyle && !nstyle->selfNesting() && ( tagname == curstyle->name() ) ) {
 		pos = beforePos;
		ENSURE_ENDTOKEN
 		return FALSE;
 	    }
	    if ( nstyle && !nstyle->allowedInContext( curstyle ) ) {
		QString msg;
		msg.sprintf( "QText Warning: Document not valid ( '%s' not allowed in '%s' #%d)",
			 tagname.ascii(), current->style->name().ascii(), pos);
		sheet_->error( msg );
		pos = beforePos;
		ENSURE_ENDTOKEN
		return FALSE;
	    }
	    if ( !nstyle ) {
		if ( emptyTag )
		    continue;
		nstyle = nullstyle;
	    }

	    QTextCustomItem* custom = sheet_->tag( tagname, attr, contxt, *factory_ , emptyTag );
	    if ( custom || tagname == "br") {
		PROVIDE_DUMMY
		if ( !custom && !attr.isEmpty() )
		    custom = new QTextLineBreak( attr );
		if ( custom ) {
		    (dummy?dummy:current)->text.append( "", fmt.makeTextFormat( nstyle, attr, custom ) );
		    if ( custom->ownLine() && custom->placeInline() ) {
			(dummy?dummy:current)->text.append( "\n", fmt ) ;
			if ( wsm != QStyleSheetItem::WhiteSpacePre )
			    eatSpace(doc, pos);
			space_ = FALSE;
		    }
		} else {// br
		    (dummy?dummy:current)->text.append( "\n", fmt ) ;
		    if ( wsm != QStyleSheetItem::WhiteSpacePre )
			eatSpace(doc, pos);
		    space_ = FALSE;
		}
	    }
	    else if ( tagname == "table" ) {
		PROVIDE_DUMMY
		int reserved_id = qt_text_paragraph_id++;
		QTextCharFormat nfmt( fmt.makeTextFormat( nstyle, attr ) );
		custom = parseTable( attr, nfmt, doc, pos );
		(dummy?dummy:current)->text.append( "", fmt.makeTextFormat( nstyle, attr, custom ) );
  		(dummy?dummy:current)->text.append( "\n", fmt ) ;
		if ( custom->placeInline() ) {
		    // inline tables need to be at the end of a paragraph for proper selection handling
		    if ( !current->text.isEmpty() ){
			dummy = new QTextParagraph( current, formats, fmt, nullstyle );
			dummy->id = reserved_id;
			dummy->text = current->text;
			current->text.clear();
			current->child = dummy;
		    }
		    dummy = 0;
		}
		CLOSE_TAG
		(void ) eatSpace( doc, pos );
		space_ = FALSE;
	    }
	    else if (nstyle->displayMode() == QStyleSheetItem::DisplayBlock
		|| nstyle->displayMode() == QStyleSheetItem::DisplayListItem
		|| nstyle->displayMode() == QStyleSheetItem::DisplayNone
		) {

		if ( !current->text.isEmpty() ){
		    dummy = new QTextParagraph( current, formats, fmt, nullstyle );
		    dummy->text = current->text;
		    dummy->text.append( "\n", fmt );
		    current->text.clear();
		    current->child = dummy;
		}

		QTextParagraph* subparagraph = new QTextParagraph( current, formats, fmt.makeTextFormat(nstyle,attr), nstyle, attr );

		if ( current == this && !child && text.isEmpty() )
		    attributes_ = attr; // propagate attributes

		bool recover = FALSE;
		space_ = FALSE;
		if (parse( subparagraph, nstyle, 0, subparagraph->format, doc, pos,
			   nstyle->whiteSpaceMode() != QStyleSheetItem::WhiteSpaceNormal?
			   nstyle->whiteSpaceMode() : wsm) ) {
		    (void) eatSpace(doc, pos);
		    int recoverPos = pos;
		    valid = (hasPrefix(doc, pos, QChar('<'))
			     && hasPrefix(doc, pos+1, QChar('/'))
			     && eatCloseTag(doc, pos, tagname) );
		    // sloppy mode, warning was done in eatCloseTag
		    if (!valid) {
			pos = recoverPos;
			valid = TRUE;
			recover = TRUE;
		    }
		}
		if ( subparagraph->style->displayMode() == QStyleSheetItem::DisplayNone ) {
		    // delete invisible paragraphs
		    delete subparagraph;
		} else {
		    // connect visible paragraphs
		    if ( !current->child )
			current->child = subparagraph;
		    else {
			QTextParagraph* it = current->child;
			while ( it->next )
			    it = it->next;
			it->next = subparagraph;
			subparagraph->prev = it;
		    }
		    dummy = 0;
		}

		if ( recover ) {
		    ENSURE_ENDTOKEN
		    return TRUE; // sloppy, we could return FALSE to abort
		}
		(void) eatSpace(doc, pos);
		space_ = FALSE;
	    }
	    else { // containers and empty tags
		if ( parse( current, nstyle, dummy, fmt.makeTextFormat(nstyle, attr), doc, pos,
			    nstyle->whiteSpaceMode() != QStyleSheetItem::WhiteSpaceNormal?
			    nstyle->whiteSpaceMode() : wsm) ) {
		    CLOSE_TAG
		    if ( current->child && current->lastChild()->style == nullstyle )
			dummy = current->lastChild();
		    else
			dummy = 0;
		    if ( wsm != QStyleSheetItem::WhiteSpacePre && !space_ )
			space_ = !eatSpace( doc, pos ); // start of line, eat space
		}
	    }
	}
	else { // plain text
	    if ( current->child && !dummy ) {
		dummy = new QTextParagraph( current, formats, fmt,  nullstyle );
		QTextParagraph* it = current->child;
		while ( it->next )
		    it = it->next;
		it->next = dummy;
		dummy->prev = it;
	    }
 	    QString word = parsePlainText( doc, pos, wsm, TRUE );
 	    if (valid){
		(dummy?dummy:current)->text.append( word, fmt );
		space_ = word[(int)word.length()-1] != ' ';
	    }
	}
    }
    ENSURE_ENDTOKEN
    return TRUE;
}

static bool qt_is_cell_in_use( QList<QTextTableCell>& cells, int row, int col )
{
    for ( QTextTableCell* c = cells.first(); c; c = cells.next() ) {
	if ( row >= c->row() && row < c->row() + c->rowspan()
	     && col >= c->column() && col < c->column() + c->colspan() )
	    return TRUE;
    }
    return FALSE;
}

QTextCustomItem* QRichText::parseTable( const QMap<QString, QString> &attr, const QTextCharFormat &fmt, const QString &doc, int& pos )
{

    QTextTable* table = new QTextTable( attr );
    int row = -1;
    int col = -1;

    QString rowbgcolor;
    QString rowalign;
    QString tablebgcolor = attr["bgcolor"];

    QList<QTextTableCell> multicells;

    QString tagname;
    (void) eatSpace(doc, pos);
    while ( valid && pos < int(doc.length() )) {
	int beforePos = pos;
	if (hasPrefix(doc, pos, QChar('<')) ){
	    if (hasPrefix(doc, pos+1, QChar('/'))) {
		tagname = parseCloseTag( doc, pos );
		if ( tagname == "table" ) {
		    pos = beforePos;
		    return table;
		}
	    } else {
		QMap<QString, QString> attr2;
		bool emptyTag = FALSE;
		tagname = parseOpenTag( doc, pos, attr2, emptyTag );
		if ( tagname == "tr" ) {
		    rowbgcolor = attr2["bgcolor"];
		    rowalign = attr2["align"];
		    row++;
		    col = -1;
		}
		else if ( tagname == "td" || tagname == "th" ) {
		    col++;
		    while ( qt_is_cell_in_use( multicells, row, col ) ) {
			col++;
		    }

		    if ( row >= 0 && col >= 0 ) {
			const QStyleSheetItem* s = sheet_->item(tagname);
			if ( !attr2.contains("bgcolor") ) {
			    if (!rowbgcolor.isEmpty() )
				attr2["bgcolor"] = rowbgcolor;
			    else if (!tablebgcolor.isEmpty() )
				attr2["bgcolor"] = tablebgcolor;
			}
			if ( !attr2.contains("align") ) {
			    if (!rowalign.isEmpty() )
				attr2["align"] = rowalign;
			}
			QTextTableCell* cell  = new QTextTableCell( table, row, col,
			      attr2, s,
			      fmt.makeTextFormat( s, attr2, 0 ),
			      contxt, *factory_, sheet_, doc, pos );
			if ( cell->colspan() > 1 || cell->rowspan() > 1 )
			    multicells.append( cell );
			col += cell->colspan()-1;
		    }
		}
	    }

	} else {
	    ++pos;
	}
    }
    return table;
}

bool QRichText::eatSpace(const QString& doc, int& pos, bool includeNbsp )
{
    int old_pos = pos;
    while (pos < int(doc.length()) && (doc.unicode())[pos].isSpace() && ( includeNbsp || (doc.unicode())[pos] != QChar(0x00a0U) ) )
	pos++;
    return old_pos < pos;
}

bool QRichText::eat(const QString& doc, int& pos, QChar c)
{
    valid = valid && (bool) ((doc.unicode())[pos] == c);
    if (valid)
	pos++;
    return valid;
}

bool QRichText::lookAhead(const QString& doc, int& pos, QChar c)
{
    return ((doc.unicode())[pos] == c);
}

static QMap<QCString, QChar> *html_map = 0;
static void qt_cleanup_html_map()
{
    delete html_map;
    html_map = 0;
}

// Avoid bloat from excessive inlining.
static void map_insert(QMap<QCString, QChar>* map, const char* str, const QChar& ch)
{
    map->insert(str,ch);
}

// static ####################
QMap<QCString, QChar> *htmlMap()
{
    if ( !html_map ){
	html_map = new QMap<QCString, QChar>;
	qAddPostRoutine( qt_cleanup_html_map );
  	map_insert(html_map, "lt", '<');
  	map_insert(html_map, "gt", '>');
  	map_insert(html_map, "amp", '&');
  	map_insert(html_map, "nbsp", 0x00a0U);
  	map_insert(html_map, "bull", 0x2022U);
  	map_insert(html_map, "aring", '\xe5');
  	map_insert(html_map, "oslash", '\xf8');
  	map_insert(html_map, "ouml", '\xf6');
  	map_insert(html_map, "auml", '\xe4');
  	map_insert(html_map, "uuml", '\xfc');
  	map_insert(html_map, "Ouml", '\xd6');
  	map_insert(html_map, "Auml", '\xc4');
  	map_insert(html_map, "Uuml", '\xdc');
  	map_insert(html_map, "szlig", '\xdf');
  	map_insert(html_map, "copy", '\xa9');
  	map_insert(html_map, "deg", '\xb0');
  	map_insert(html_map, "micro", '\xb5');
  	map_insert(html_map, "plusmn", '\xb1');
  	map_insert(html_map, "middot", '*');
  	map_insert(html_map, "quot", '\"');
  	map_insert(html_map, "commat", '@');
  	map_insert(html_map, "num", '#');
  	map_insert(html_map, "dollar", '$');
  	map_insert(html_map, "ldquo", '`');
  	map_insert(html_map, "rdquo", '\'');
  	map_insert(html_map, "sol", '/' );
  	map_insert(html_map, "bsol", '\\');
  	map_insert(html_map, "lowbar", '_');
    }
    return html_map;
}

QChar QRichText::parseHTMLSpecialChar(const QString& doc, int& pos)
{
    QCString s;
    pos++;
    int recoverpos = pos;
    while ( pos < int(doc.length()) && (doc.unicode())[pos] != ';' && !(doc.unicode())[pos].isSpace() && pos < recoverpos + 6) {
	s += (doc.unicode())[pos];
	pos++;
    }
    if ((doc.unicode())[pos] != ';' && !(doc.unicode())[pos].isSpace() ) {
	pos = recoverpos;
	return '&';
    }
    pos++;

    if ( s.length() > 1 && s[0] == '#') {
	int num = s.mid(1).toInt();
	if ( num == 151 ) // ### hack for designer manual
	    return '-';
	return num;
    }

    QMap<QCString, QChar>::Iterator it = htmlMap()->find(s);
    if ( it != htmlMap()->end() ) {
	return *it;
    }

    pos = recoverpos;
    return '&';
}

QString QRichText::parseWord(const QString& doc, int& pos, bool insideTag, bool lower)
{
    QString s;

    if ((doc.unicode())[pos] == '"') {
	pos++;
	while ( pos < int(doc.length()) && (doc.unicode())[pos] != '"' ) {
	    s += (doc.unicode())[pos];
	    pos++;
	}
	eat(doc, pos, '"');
    }
    else {
	static QString term = QString::fromLatin1("/>");
	while( pos < int(doc.length()) &&
	       ( !insideTag || ((doc.unicode())[pos] != '>' && !hasPrefix( doc, pos, term)) )
	       && (doc.unicode())[pos] != '<'
	       && (doc.unicode())[pos] != '='
	       && !(doc.unicode())[pos].isSpace())
	{
	    if ( (doc.unicode())[pos] == '&')
		s += parseHTMLSpecialChar( doc, pos );
	    else {
		s += (doc.unicode())[pos];
		pos++;
	    }
	}
	if (lower)
	    s = s.lower();
    }
    valid = valid && pos <= int(doc.length());

    return s;
}

QString QRichText::parsePlainText(const QString& doc, int& pos, QStyleSheetItem::WhiteSpaceMode wsm, bool justOneWord)
{
    QString s;
    while( pos < int(doc.length()) &&
	   (doc.unicode())[pos] != '<' ) {
	if (isBreakable( doc.unicode() + pos, pos < int(doc.length()-1) ) && (doc.unicode())[pos] != QChar(0x00a0U) ){

	    if ( wsm == QStyleSheetItem::WhiteSpacePre ) {
		if ( (doc.unicode())[pos] == ' ' )
		    s += QChar(0x00a0U);
		else
		    s += (doc.unicode())[pos];
	    }
	    else { // non-pre mode: collapse whitespace except nbsp
		if( (doc.unicode())[pos].isSpace() ) {
		    while ( pos+1 < int(doc.length() ) &&
			    (doc.unicode())[pos+1].isSpace()  && (doc.unicode())[pos+1] != QChar(0x00a0U) )
			pos++;
		    if ( wsm == QStyleSheetItem::WhiteSpaceNoWrap )
			s += QChar(0x00a0U);
		    else
			s += ' ';
		} else {
		    s += (doc.unicode())[pos];
		}
	    }
	    pos++;
	    if ( justOneWord )
		return s;
	}
	else if ( (doc.unicode())[pos] == '&')
		s += parseHTMLSpecialChar( doc, pos );
	else {
	    s += (doc.unicode())[pos];
	    pos++;
	}
    }
    valid = valid && pos <= int(doc.length());
    return s;
}

bool QRichText::hasPrefix(const QString& doc, int pos, QChar c)
{
    if ( pos >= (int)doc.length() )
	return FALSE;
    return valid && (doc.unicode())[pos] ==c;
}

bool QRichText::hasPrefix(const QString& doc, int pos, const QString& s)
{
    if ( pos + s.length() >= doc.length() )
	return FALSE;
    for (int i = 0; i < int(s.length()); i++) {
	if ((doc.unicode())[pos+i] != s[i])
	    return FALSE;
    }
    return TRUE;
}

QString QRichText::parseOpenTag(const QString& doc, int& pos,
				  QMap<QString, QString> &attr, bool& emptyTag)
{
    emptyTag = FALSE;
    pos++;
    if ( hasPrefix(doc, pos, '!') ) {
	if ( hasPrefix( doc, pos+1, "--")) {
	    pos += 3;
	    // eat comments
	    QString pref = QString::fromLatin1("-->");
	    while ( valid && !hasPrefix(doc, pos, pref )
			&& pos < int(doc.length()) )
		pos++;
	    if ( valid && hasPrefix(doc, pos, pref ) ) {
		pos += 3;
		eatSpace(doc, pos, TRUE);
	    }
	    else
		valid = FALSE;
	    emptyTag = TRUE;
	    return QString::null;
	}
	else {
	    // eat strange internal tags
	    while ( valid && !hasPrefix(doc, pos, QChar('>')) && pos < int(doc.length()) )
		pos++;
	    if ( valid && hasPrefix(doc, pos, QChar('>')) ) {
		pos++;
		eatSpace(doc, pos, TRUE);
	    }
	    else
		valid = FALSE;
	    return QString::null;
	}
    }

    QString tag = parseWord(doc, pos, TRUE, TRUE);
    eatSpace(doc, pos, TRUE);
    static QString term = QString::fromLatin1("/>");
    static QString s_true = QString::fromLatin1("true");

    while (valid && !lookAhead(doc, pos, '>')
	    && ! (emptyTag = hasPrefix(doc, pos, term) ))
    {
	QString key = parseWord(doc, pos, TRUE, TRUE);
	eatSpace(doc, pos, TRUE);
	if ( key.isEmpty()) {
	    // error recovery
	    while ( pos < int(doc.length()) && !lookAhead(doc, pos, '>'))
		pos++;
	    break;
	}
	QString value;
	if (hasPrefix(doc, pos, QChar('=')) ){
	    pos++;
	    eatSpace(doc, pos);
	    value = parseWord(doc, pos, TRUE, FALSE);
	}
	else
	    value = s_true;
	attr.insert(key, value );
	eatSpace(doc, pos, TRUE);
    }

    if (emptyTag) {
	eat(doc, pos, '/');
	eat(doc, pos, '>');
    }
    else
	eat(doc, pos, '>');

    return tag;
}

QString QRichText::parseCloseTag( const QString& doc, int& pos )
{
    pos++;
    pos++;
    QString tag = parseWord(doc, pos, TRUE, TRUE);
    eatSpace(doc, pos, TRUE);
    eat(doc, pos, '>');
    return tag;
}

bool QRichText::eatCloseTag(const QString& doc, int& pos, const QString& open)
{
    QString tag = parseCloseTag( doc, pos );
    if (!valid) {
	QString msg;
	msg.sprintf( "QText Warning: Document not valid ( '%s' not closing #%d)", open.ascii(), pos);
	sheet_->error( msg );
	valid = TRUE;
    }
    valid = valid && tag == open;
    if (!valid) {
	QString msg;
	msg.sprintf( "QText Warning: Document not valid ( '%s' not closed before '%s' #%d)",
		     open.ascii(), tag.ascii(), pos);
	sheet_->error( msg );
    }
    return valid;
}

void QRichText::draw(QPainter* p, int x, int y,
		      int ox, int oy, int cx, int cy, int cw, int ch,
		      QRegion& backgroundRegion, const QColorGroup& cg, const QTextOptions& to )
{
    QRichTextFormatter tc( *this );
    QTextParagraph* b = this;
    while ( b ) {
	tc.gotoParagraph( p, b );
	do {
	    tc.makeLineLayout( p );
	    QRect geom( tc.lineGeometry() );
	    if ( geom.bottom()+y > cy && geom.top()+y < cy+ch )
		tc.drawLine( p, ox-x, oy-y, cx-x, cy-y, cw, ch, backgroundRegion, cg, to );
	}
	while ( tc.gotoNextLine( p ) );
	b = tc.paragraph->nextInDocument();
    }
    flow()->drawFloatingItems( p, ox-x, oy-y, cx-x, cy-y, cw, ch, backgroundRegion, cg, to );
}


void QRichText::doLayout( QPainter* p, int nwidth ) {
    QRichTextFormatter fc( *this );
    invalidateLayout();
    flow()->initialize( nwidth );
    fc.initParagraph( p, this );
    fc.updateLayout( p );
}

QString QRichText::anchorAt( const QPoint& pos ) const
{
    QRichTextIterator it( *((QRichText*)this) );
    it.goTo( pos );
    return it.format()->anchorHref();
}


/*
  Returns the paragraph right before \a y. Fast through caching.
 */
QTextParagraph* QRichText::getParBefore( int y ) const
{
    QTextParagraph* b = dirty?0:b_cache;
    if ( !b ) {
	b = (QTextParagraph*)this;
	while ( b->child )
	    b = b->child;
    }
    while ( b->ypos > y  && b->prevInDocument() )
	b = b->prevInDocument();
    while ( b->ypos + b->height < y && b->nextInDocument() )
	b = b->nextInDocument();
    QRichText* that = (QRichText*) this;
    that->b_cache = b;
    return b;
}

bool operator!=( const QtTriple &t1, const QtTriple &t2 )
{
    return (t1.a != t2.a) || (t1.b != t2.b) || (t1.c != t2.c);
}
bool operator<( const QtTriple &t1, const QtTriple &t2 )
{
    if ( t1.a != t2.a )
	return t1.a < t2.a;
    else if ( t1.b != t2.b )
	return t1.b < t2.b;
    else if ( t1.c != t2.c )
	return t1.c < t2.c;
    return FALSE;
}

bool operator>=( const QtTriple &t1, const QtTriple &t2 )
{
    if ( t1.a != t2.a )
	return t1.a > t2.a;
    else if ( t1.b != t2.b )
	return t1.b > t2.b;
    else if ( t1.c != t2.c )
	return t1.c > t2.c;
    return TRUE;
}

QRichTextIterator::QRichTextIterator(QRichText& document )
    : doc( document ), fc( document )
{
    stack.setAutoDelete( TRUE );
    fc.gotoParagraph( 0, &document );
    dirty = TRUE;
}

QtTriple QRichTextIterator::position() const
{
    return (stack.getLast()?stack.getLast()->fc: fc).position();
}

bool QRichTextIterator::right( bool doFormat )
{
    if ( !doFormat )
	dirty = TRUE;
    else if ( dirty )
	update();
    QRichTextFormatter* f = stack.getLast()?&stack.getLast()->fc: &fc;

    QTextCustomItem* c = f->format()->customItem();
    if ( c && c->isTable() && c->placeInline() ) {
	QTextTable* table = (QTextTable*) c;
	if ( !table->cells.isEmpty() ) {
	    Item* item = new Item( *f, table->cells );
	    stack.append( item );
	    QTextTableCell* cell = item->it.current();
	    item->fc = QRichTextFormatter( *cell->richText() );
	    item->fc.gotoParagraph( 0, cell->richText() );
	    return TRUE;
	}
    }
    bool ok = doFormat?f->rightOneItem():f->lazyRightOneItem();
    while ( !ok && !stack.isEmpty() ) {
	Item* item = stack.getLast();
	++item->it;
	if ( item->it.current() ) {
	    QTextTableCell* cell = item->it.current();
	    item->fc = QRichTextFormatter( *cell->richText() );
	    item->fc.gotoParagraph( 0, cell->richText() );
	    ok = TRUE;
	    break;
	}
	stack.removeLast();
	f = stack.getLast()?&stack.getLast()->fc: &fc;
	ok = doFormat?f->rightOneItem():f->lazyRightOneItem();
    }
    return ok;
}

QString QRichTextIterator::text() const
{
    const QRichTextFormatter& f = stack.getLast()?stack.getLast()->fc:fc;
    if ( f.current < f.paragraph->text.length() )
	return f.paragraph->text.charAt( f.current );
    return QString::null;
}

QTextCharFormat* QRichTextIterator::format() const
{
    const QRichTextFormatter& f = stack.getLast()?stack.getLast()->fc:fc;
    return f.format();
}

QRect QRichTextIterator::lineGeometry() const
{
    if ( dirty ) {
	QRichTextIterator* that = (QRichTextIterator*) this;
	that->update();
    }
    QRect geom( fc.lineGeometry() );
    const QRichTextFormatter* f = &fc;
    QListIterator<Item> it( stack );
    while ( it.current() ) {
	Item* item = it.current();
	++it;
	QTextTable* table = (QTextTable*)f->format()->customItem();
	QTextTableCell* cell = item->it.current();
	QRect r( item->fc.lineGeometry() );
	geom.setRect( geom.x() + f->currentx + table->outerborder + r.x() + cell->geometry().x(),
		      geom.y() + f->base - table->height + table->outerborder + r.y() + cell->geometry().y(),
		      r.width(), r.height() );
	f = &item->fc;
    }
    return geom;
}


bool QRichTextIterator::goTo( const QPoint& pos )
{
    dirty = FALSE;
    stack.clear();
    int x = pos.x();
    int y = pos.y();
    bool within = fc.goTo( 0, x, y );
    QRichTextFormatter* f = &fc;
    QTextCustomItem* c = f->format()->customItem();
    while ( c && c->isTable() && c->placeInline() ) {
	QTextTable* table = (QTextTable*) c;
	if ( table->cells.isEmpty() )
	    return FALSE;
	QRect geom( f->lineGeometry() );
	x -= geom.x() + f->currentx;
	y -= geom.y() + f->base - table->height;
	Item* item = new Item( *f, table->cells );
	stack.append( item );
	while ( !item->it.atLast() ) {
	    QPoint p( x - table->outerborder, y - table->outerborder );
	    QRect r( item->it.current()->geometry() );
	    if ( r.contains( p ) || ( p.x() <= r.right() && p.y() <= r.bottom() ) )
		break;
	    ++item->it;
	}
	QTextTableCell* cell = item->it.current();
	item->fc = QRichTextFormatter( *cell->richText() );
	x -= table->outerborder + cell->geometry().x();
	y -= table->outerborder + cell->geometry().y();
	within = item->fc.goTo( 0, x, y );
	f = &item->fc;
	c = f->format()->customItem();
    }
    return within;
}

void QRichTextIterator::goTo( const QtTriple& pos )
{
    QtTriple rawpos = pos; rawpos.c = 0;
    stack.clear();
    fc.gotoParagraph( 0, &doc );
    while ( position() < rawpos && right( FALSE ) ) { }
    QRichTextFormatter& f = stack.getLast()?stack.getLast()->fc: fc;
    f.currentoffset = pos.c;
}

void QRichTextIterator::update()
{
    fc.update();
    for ( Item* i = stack.first(); i; i = stack.next() )
	i->fc.update();
    dirty = FALSE;
}

void QTextParagraph::invalidateLayout()
{
    dirty = TRUE;
    QTextParagraph* b = child;
    while ( b ) {
	b->dirty = TRUE;
	b->invalidateLayout();
	b = b->next;
    }
}

QStyleSheetItem::ListStyle QTextParagraph::listStyle()
{
    QString s =  attributes()["type"];

    if ( !s )
	return style->listStyle();
    else {
	QCString sl = s.latin1();
	if ( sl == "1" )
	    return QStyleSheetItem::ListDecimal;
	else if ( sl == "a" )
	    return QStyleSheetItem::ListLowerAlpha;
	else if ( sl == "A" )
	    return QStyleSheetItem::ListUpperAlpha;
	sl = sl.lower();
	if ( sl == "square" )
	    return QStyleSheetItem::ListSquare;
	else if ( sl == "disc" )
		return QStyleSheetItem::ListDisc;
	else if ( sl == "circle" )
	    return QStyleSheetItem::ListCircle;
    }
    return style->listStyle();
}


int QTextParagraph::numberOfSubParagraph( QTextParagraph* subparagraph, bool onlyListItems)
{
    QTextParagraph* it = child;
    int i = 1;
    if ( attributes_.contains( "start" ) )
	i = attributes_["start"].toInt();

    while ( it && it != subparagraph ) {
	if ( !onlyListItems || it->style->displayMode() == QStyleSheetItem::DisplayListItem )
	    ++i;
	it = it->next;
    }
    return i;
}

QTextParagraph::QTextParagraph( QTextParagraph* p, QTextFormatCollection* formatCol, const QTextCharFormat& fmt,
	      const QStyleSheetItem *stl, const QMap<QString, QString> &attr )
    : parent( p ), formats( formatCol ), format( fmt ), text( formatCol ), style ( stl ), attributes_( attr )
{
    init();
};


QTextParagraph::QTextParagraph( QTextParagraph* p, QTextFormatCollection* formatCol, const QTextCharFormat& fmt,
	      const QStyleSheetItem *stl )
    : parent( p ), formats( formatCol ), format( fmt ), text( formats ), style ( stl )
{
    init();
};


void QTextParagraph::init()
{
    id = qt_text_paragraph_id++;

    child = next = prev = 0;
    height = ypos = 0;
    dirty = TRUE;
    selected = FALSE;
    flow_ = 0;
    if ( parent )
	flow_ = parent->flow();

    align = QStyleSheetItem::Undefined;

    if ( attributes_.contains("align") ) {
 	QString s = attributes_["align"].lower();
 	if ( s  == "center" )
 	    align = Qt::AlignCenter;
 	else if ( s == "right" )
 	    align = Qt::AlignRight;
	else
	    align = Qt::AlignLeft;
    }

    clear = QTextCustomItem::ClearNone;
    if ( attributes_.contains("clear") ) {
 	QString s = attributes_["clear"].lower();
 	if ( s  == "left" )
 	    clear = QTextCustomItem::ClearLeft;
 	else if ( s == "right" )
 	    clear = QTextCustomItem::ClearRight;
	else if ( s == "both" || s == "all")
	    clear = QTextCustomItem::ClearBoth;
    }
}

QTextParagraph::~QTextParagraph()
{
    while ( child ) {
	QTextParagraph* tmp = child;
	child = child->next;
	delete tmp;
    }
}

int QTextParagraph::labelMargin() const
{
    if ( style->displayMode() != QStyleSheetItem::DisplayListItem )
	return 0;
    QFontMetrics fm( format.font() );
    return fm.width( QString::fromLatin1("123. ") );
}


QTextParagraph* QTextParagraph::nextInDocument() const
{
    if ( next  ) {
	QTextParagraph* b = next;
 	while ( b->child )
 	    b = b->child;
	return b;
    }
    if ( parent ) {
	return parent->nextInDocument();
    }
    return 0;
}

QTextParagraph* QTextParagraph::prevInDocument() const
{
    if ( prev ){
	QTextParagraph* b = prev;
	while ( b->child ) {
	    b = b->child;
	    while ( b->next )
		b = b->next;
	}
	return b;
    }
    if ( parent ) {
	return parent->prevInDocument();
    }
    return 0;
}

QTextParagraph* QTextParagraph::lastChild() const
{
    if ( !child )
	return (QTextParagraph*) this;
    QTextParagraph* b = child;
    while ( b->next )
	b = b->next;
    return b->lastChild();
}

QTextFlow* QTextParagraph::flow() const
{
    return flow_;
}

QTextRichString::QTextRichString( QTextFormatCollection* fmt )
    : formats( fmt )
{
    items = 0;
    len = 0;
    store = 0;
}

QTextRichString::~QTextRichString()
{
    clear();
}

void QTextRichString::clear()
{
    for (int i = 0; i < len; ++i )
	formats->unregisterFormat( *items[i].format );
    if ( items )
	delete [] items;
    items = 0;
    len = 0;
    store = 0;
}


void QTextRichString::remove( int index, int lgth )
{
    for (int i = 0; i < lgth; ++i )
	formats->unregisterFormat( *formatAt( index + i ) );

    int olen = length();
    if ( index + lgth >= olen ) {		// range problems
	if ( index < olen ) {			// index ok
	    setLength( index );
	}
    } else if ( lgth != 0 ) {
// 	memmove( items+index, items+index+lgth,
// 		 sizeof(Item)*(olen-index-lgth) );
	for (int i = index; i < olen-lgth; ++i)
	    items[i] = items[i+lgth];
	setLength( olen-lgth );
    }
}

void QTextRichString::insert( int index, const QString& c, const QTextCharFormat& form )
{
    QTextCharFormat* f = formats->registerFormat( form );

    if ( index >= len ) {			// insert after end
	setLength( index+1 );
    } else {					// normal insert
//	int olen = len;
	int nlen = len + 1;
	setLength( nlen );
// 	memmove( items+index+1, items+index,
// 		 sizeof(Item)*(olen-index) );
	for (int i = len-1; i > index; --i)
	    items[i] = items[i-1];
    }

    items[index].c = c;
    items[index].format = f;
    items[index].width = -1;
}


QString& QTextRichString::getCharAt( int index )
{
    items[index].width = -1;
    return items[index].c;
}

void QTextRichString::setLength( int l )
{
    if ( l <= store ) {
	len = l; // TODO shrinking
	return;
    } else {
	store = QMAX( l*2, 40 );
	Item* newitems = new Item[store]; // TODO speedup using new char(....)
// 	if ( items )
// 	    memcpy( newitems, items, sizeof(Item)*len );
	for (int i = 0; i < len; ++i )
	    newitems[i] = items[i];
	if ( items )
	    delete [] items;
	items = newitems;
	len = l;
    }
}

QTextRichString::QTextRichString( const QTextRichString &other )
{
    formats = other.formats;
    len = other.len;
    items = 0;
    store = 0;
    if ( len ) {
	store = QMAX( len, 40 );
	items = new Item[ store ];
// 	memcpy( items, other.items, sizeof(Item)*len );
	for (int i = 0; i < len; ++i ) {
	    items[i] = other.items[i];
	    items[i].format->ref();
	}
    }
}

QTextRichString& QTextRichString::operator=( const QTextRichString &other )
{
    clear();
    formats = other.formats;
    len = other.len;
    items = 0;
    store = 0;
    if ( len ) {
	items = new Item[ len ];
	store = len;
// 	memcpy( items, other.items, sizeof(Item)*len );
	for (int i = 0; i < len; ++i ) {
	    items[i] = other.items[i];
	    items[i].format->ref();
	}
    }
    return *this;
}


QRichTextFormatter::QRichTextFormatter(QRichText& document )
{
    doc = &document;
    paragraph = doc;
    first = y_ = width = widthUsed = height = base = fill = 0;
    last = first - 1;
    current = currentx = currentoffset = currentoffsetx = 0;
    lmargin = rmargin = 0;
    static_lmargin = static_rmargin = 0;
    currentasc  = currentdesc = 0;
    formatinuse = 0;
    xscale = yscale = 1;
}
QRichTextFormatter::~QRichTextFormatter()
{
}

/*
  Like gotoParagraph() but also initializes the paragraph
  (i.e. setting the cached values in the paragraph to the cursor's
  settings and not vice versa.

 */
void QRichTextFormatter::initParagraph( QPainter* p, QTextParagraph* b )
{
	b->ypos = y_;
	b->height = 0;
	while ( b->child  ) {
	    b->child->ypos = b->ypos;
	    b = b->child;
	}
	gotoParagraph( p, b );
}

void QRichTextFormatter::gotoParagraph( QPainter* p, QTextParagraph* b )
{
    if ( !formatinuse ) { // ### a bit hacky
	if ( p && p->device()->devType() == QInternal::Printer ) {
	    QPaintDeviceMetrics metrics(p->device());
	    xscale = scale_factor( metrics.logicalDpiX() );
	    yscale = scale_factor( metrics.logicalDpiY() );
	}
    }

    if ( !b )
	return;
    while ( b->child ) {
	b = b->child;
    }

    paragraph = b;
    flow = paragraph->flow();
    if ( paragraph->text.isEmpty() )
	paragraph->text.append( " ", paragraph->format );

    first = y_ = width = widthUsed = height = base = fill = 0;
    last = first - 1;


    y_ =  b->ypos;
    int m =  int(b->topMargin() * yscale);
    flow->adjustFlow( y_, widthUsed, m ) ;

    y_ += m;

    static_lmargin = int(paragraph->totalMargin( QStyleSheetItem::MarginLeft ) * xscale);
    static_rmargin = int(paragraph->totalMargin( QStyleSheetItem::MarginRight ) * xscale);
    static_labelmargin = int(paragraph->totalLabelMargin() * xscale);

    alignment = paragraph->alignment();

    width = flow->width;

    int mm = adjustHorizontalMargins( paragraph->clear );
    if ( mm ) {
	flow->adjustFlow( y_, widthUsed, mm + m ) ;
	y_ += mm + m;
    }

    current = 0;

    currentx = lmargin;
    currentoffset = 0;
    currentoffsetx = 0;
    updateCharFormat( p );

}


int QRichTextFormatter::adjustHorizontalMargins( QTextCustomItem::Clear clear )
{
    int m = 0;
    lmargin = flow->adjustLMargin( y_ + m, static_lmargin, int(4*xscale) );
    if ( clear == QTextCustomItem::ClearLeft ||
	 clear == QTextCustomItem::ClearBoth ) {
	while ( lmargin > static_lmargin ) {
	    m += int( QMAX( xscale, 1 ) );
	    lmargin = flow->adjustLMargin( y_ + m, static_lmargin, int(4*xscale) );
	}
    }
    lmargin += static_labelmargin;
    rmargin = flow->adjustRMargin( y_ + m, static_rmargin, int(4*xscale) );
    if ( clear == QTextCustomItem::ClearRight ||
	 clear == QTextCustomItem::ClearBoth ) {
	while ( rmargin > static_rmargin ) {
	    m += int( QMAX( xscale, 1 ) );
	    rmargin = flow->adjustRMargin( y_ + m, static_rmargin, int(4*xscale) );
	}
    }
    return m;
}

void QRichTextFormatter::update( QPainter* p )
{
    int i = current;
    int io = currentoffset;
    gotoParagraph( p, paragraph );
    makeLineLayout( p );
    gotoLineStart( p );
    while ( current < i  && rightOneItem( p ) ) { }
    while ( currentoffset < io )
	right( p );
}


bool QRichTextFormatter::gotoNextLine( QPainter* p )
{
    current = last;
    if ( atEnd() ) {
	current++;
	y_ += height + 1; // first pixel below us
	int m = paragraph->bottomMargin();
	QTextParagraph* nid = paragraph->nextInDocument();
	if ( nid )
	    m -= nid->topMargin();
	if ( m > 0 ) {
	    flow->adjustFlow( y_, widthUsed, m ) ;
	    y_ += m;
	}
	width = flow->width;
	lmargin = flow->adjustLMargin( y_, static_lmargin, int(4*xscale) );
	lmargin += static_labelmargin;
	rmargin = flow->adjustRMargin( y_, static_rmargin, int(4*xscale) );
	paragraph->height = y() - paragraph->ypos; //####
	paragraph->dirty = FALSE;
	return FALSE;
    }
    y_ += height + 1;
    width = flow->width;
    lmargin = flow->adjustLMargin( y_, static_lmargin, int(4*xscale) );
    lmargin += static_labelmargin;
    rmargin = flow->adjustRMargin( y_, static_rmargin, int(4*xscale) );
    current++;
    currentx = lmargin;

    height = 0;
    updateCharFormat( p );
    return TRUE;
}

void QRichTextFormatter::gotoLineStart( QPainter* p )
{
    current = first;
    currentoffset = currentoffsetx = 0;
    currentx = lmargin + fill;
    updateCharFormat( p );
}


void QRichTextFormatter::updateCharFormat( QPainter* p )
{
    if ( pastEnd() )
	return;
    QTextCharFormat* fmt = format();
    QFontMetrics fm( fmt->font() );
    if ( p ) {
	p->setFont( fmt->font() );
	fm = p->fontMetrics();
    }
    currentasc = fm.ascent();
    currentdesc = fm.descent();
    QTextCustomItem* custom = fmt->customItem();
    if ( custom ) {
	if ( custom->width < 0 ) {
	    custom->realize( p );
	}
	if ( width >= 0 && custom->expandsHorizontally() ) {
	    custom->resize( p, width - lmargin - rmargin - fm.width(' ' ) );
	}
	if ( custom->placeInline() )
	    currentasc = custom->height;
    }
    formatinuse = fmt;
}


void QRichTextFormatter::drawLabel( QPainter* p, QTextParagraph* par, int x, int y,
				    int w, int h, int ox, int oy,
				    QRegion& backgroundRegion,
				    const QColorGroup& cg, const QTextOptions& to )

{
    if ( !par->parent )
	return;
    if ( par->style->displayMode() != QStyleSheetItem::DisplayListItem )
	return;

    QRect r (x - ox, y-oy, w, h ); ///#### label width?
    to.erase( p, r );
    backgroundRegion = backgroundRegion.subtract( r );
    QStyleSheetItem::ListStyle s = par->parent->listStyle();

    QFont font = p->font();
    p->setFont( par->parent->format.font() );
    QFontMetrics fm( p->fontMetrics() );
    int size = fm.lineSpacing() / 3;
    if ( size > 12 * xscale )
	size = int(12 * xscale);

    switch ( s ) {
    case QStyleSheetItem::ListDecimal:
    case QStyleSheetItem::ListLowerAlpha:
    case QStyleSheetItem::ListUpperAlpha:
	{
	    int n = par->parent->numberOfSubParagraph( par, TRUE );
	    QString l;
	    switch ( s ) {
	    case QStyleSheetItem::ListLowerAlpha:
		if ( n < 27 ) {
		    l = QChar( ('a' + (char) (n-1)));
		    break;
		}
	    case QStyleSheetItem::ListUpperAlpha:
		if ( n < 27 ) {
		    l = QChar( ('A' + (char) (n-1)));
		    break;
		}
		break;
	    default:  //QStyleSheetItem::ListDecimal:
		l.setNum( n );
		break;
	    }
	    l += QString::fromLatin1(". ");
	    p->drawText( r.right() - fm.width( l ), r.top() + base, l );
	}
	break;
    case QStyleSheetItem::ListSquare:
	{
 	    QRect er( r.right()-size*2, r.top() + base - fm.boundingRect('A').height()/2 - size/2 - 1, size, size);
	    p->fillRect( er , cg.brush( QColorGroup::Foreground ) );
	}
	break;
    case QStyleSheetItem::ListCircle:
	{
	    QRect er( r.right()-size*2, r.top() + base - fm.boundingRect('A').height()/2 - size/2 - 1, size, size);
	    p->drawEllipse( er );
	}
	break;
    case QStyleSheetItem::ListDisc:
    default:
	{
	    p->setBrush( cg.brush( QColorGroup::Foreground ));
	    QRect er( r.right()-size*2, r.top() + base - fm.boundingRect('A').height()/2 - size/2 - 1, size, size);
	    p->drawEllipse( er );
	    p->setBrush( Qt::NoBrush );
	}
	break;
    }

    p->setFont( font );
}

void QRichTextFormatter::drawLine( QPainter* p, int ox, int oy,
			     int cx, int cy, int cw, int ch,
			     QRegion& backgroundRegion,
			     const QColorGroup& cg, const QTextOptions& to )

{
    gotoLineStart( p );
    if ( pastEndOfLine() )
	return;

    int gx = 0;
    int gy = y();
    int realWidth = QMAX( width, widthUsed );
    QRect r(gx-ox+lmargin, gy-oy, realWidth-lmargin-rmargin, height);
    bool clipMode = format()->customItem() && format()->customItem()->noErase();

    bool isPrint = p->device()->devType() == QInternal::Printer;

    if (!clipMode )
	to.erase( p, r );



    if ( first == 0 ) {
	//#### TODO cache existence of label
	QTextParagraph*it = paragraph;
	int m = 0;
	while ( it &&
		( it->style->displayMode() == QStyleSheetItem::DisplayListItem
		  || it->prev == 0 ) ) {
	    int itm = it->labelMargin();
	    m += itm;
	    drawLabel( p, it, gx+lmargin-m, gy, itm, height, ox, oy, backgroundRegion, cg, to );
	    m += it->margin( QStyleSheetItem::MarginLeft );
	    if ( it->style->displayMode() == QStyleSheetItem::DisplayListItem && it->prev )
		it = 0;
	    else
		it = it->parent;
	}
    }

    QString c;
    QString printString;
    int printX = 0;
    int printY = 0;
    while ( current <= last ) {
	QTextCharFormat *fmt  = format();
	if ( !fmt->anchorHref().isEmpty() && fmt->useLinkColor() ) {
	    if ( QPaintDeviceMetrics(p->device()).depth() > 1 ||
		 p->device()->devType() == QInternal::Printer)
		p->setPen( to.linkColor );
	    else
		p->setPen( cg.text() );
	    if ( to.linkUnderline ) {
		QFont f = fmt->font();
		f.setUnderline( TRUE );
		p->setFont( f );
	    }
	} else {
	    if ( fmt->color().isValid() && ( QPaintDeviceMetrics(p->device()).depth() > 1 ||
		p->device()->devType() == QInternal::Printer ) )
		p->setPen( fmt->color() );
	    else
		p->setPen( cg.text() );
	}
	bool selected = !clipMode && to.inSelection( position() );
	QRect highlight;
	bool only_partially_highlighted = FALSE;
	if ( selected ) {
	    QtTriple pos = position();
	    int offsetx = 0;
	    int w = paragraph->text.items[current].width;
	    if ( to.selstart.c  && to.selstart.a == pos.a && to.selstart.b == pos.b ) {
		QFontMetrics fm( p->fontMetrics() );
		c = paragraph->text.charAt( current );
		offsetx = fm.width( c.left( to.selstart.c ) );
		only_partially_highlighted = TRUE;
	    }
	    if ( to.selend.c && to.selend.a == pos.a && to.selend.b == pos.b ) {
		QFontMetrics fm( p->fontMetrics() );
		c = paragraph->text.charAt( current );
		w -= fm.width( c.mid( to.selend.c ) );
		only_partially_highlighted = TRUE;
	    }
	    highlight.setRect( gx-ox+currentx+offsetx, gy-oy+1, w- offsetx, height );
	    p->fillRect( highlight, cg.highlight() );
	    if ( !only_partially_highlighted )
		p->setPen( cg.highlightedText() );
	}

	QTextCustomItem* custom = fmt->customItem();
	if ( custom ) {
	    int h = custom->height;
	    if ( custom->placeInline() ) {
		custom->xpos = gx + currentx;
		custom->ypos = gy + base - h;
		custom->draw(p, custom->xpos, custom->ypos, ox, oy,
			     cx, cy, cw, ch, backgroundRegion, cg, to );
	    }
	} else {
	    c = paragraph->text.charAt( current );
	    int l = c.length();
	    while ( l>0 && ( c[l-1]=='\n' || c[l-1]=='\r' ) )
		--l;

#ifdef _WS_X11_
	    //### workaround for broken courier fonts on X11
 	    for ( int i = 0; i < l; i++ ) {
 		if ( c[i] == QChar(0x00a0U) )
 		    c[i] = ' ';
 	    }
#endif
#ifdef _WS_WIN32_
	    //### workaround for black squares
 	    for ( int i = 0; i < l; i++ ) {
 		if ( c[i] == '\n' || c[i] == '\r' )
 		    c[i] = ' ';
 	    }
#endif
	    if ( l ) {
		if ( isPrint ) {
		    if ( printString.isEmpty() ) {
			printString = c;
			printX = gx + currentx - ox;
			printY = gy-oy+base;
		    }  else {
			printString += c.left(l);
		    }
		} else {
		    p->drawText(gx+currentx-ox, gy-oy+base, c, l );
		}
	    }
	    if ( only_partially_highlighted ) {
		p->setClipRect( highlight );
		p->setPen( cg.highlightedText() );
		if ( l )
		    p->drawText(gx+currentx-ox, gy-oy+base, c, l );
		p->setClipping( FALSE );
	    }
	
	}
	if ( selected )
	    p->setPen( ( isPrint || QPaintDeviceMetrics(p->device()).depth() > 1) ? fmt->color() : cg.text() );
	
	if ( isPrint && !printString.isEmpty() && !paragraph->text.haveSameFormat( current, current + 1) ) {
	    p->drawText( printX, printY, printString );
	    printString = QString::null;
	}
	gotoNextItem( p );
	gy = y();
    }
    if ( isPrint && !printString.isEmpty() ) {
	p->drawText( printX, printY, printString );
	printString = QString::null;
    }

    if (clipMode && !isPrint ) {
	p->setClipRegion( backgroundRegion );
	to.erase( p, r);
	p->setClipping( FALSE );
    }
    backgroundRegion = backgroundRegion.subtract(r);
}

bool QRichTextFormatter::atEnd() const
{
    return current > paragraph->text.length()-2;
}


bool QRichTextFormatter::pastEnd() const
{
    return current > paragraph->text.length() - 1;
}

bool QRichTextFormatter::pastEndOfLine() const
{
    return current > last;
}

bool QRichTextFormatter::atEndOfLine() const
{
    return current > last || (current == last && currentoffset >= int(paragraph->text.charAt( current ).length())-1);
}

bool QRichTextFormatter::rightOneItem( QPainter* p )
{
    if ( atEnd() ) {
	QTextParagraph* next = paragraph->nextInDocument();
	if ( next ) {
	    if ( next->dirty )
		updateLayout( p );
	    gotoParagraph( p, next );
	    makeLineLayout( p );
	    gotoLineStart( p );
	} else
	    return FALSE;
    }
    else if ( current >= last ) {
	(void) gotoNextLine( p );
	makeLineLayout( p );
	gotoLineStart( p );
    }
    else
	gotoNextItem( p );
    return TRUE;
}

bool QRichTextFormatter::lazyRightOneItem()
{
    if ( atEnd() ) {
	QTextParagraph* next = paragraph->nextInDocument();
	if ( next )
	    gotoParagraph( 0, next );
	else
	    return FALSE;
    }
    else if ( !pastEnd() ) {
	current++;
	currentoffset = currentoffsetx = 0;
    }
    return TRUE;
}

void QRichTextFormatter::right( QPainter* p )
{
    if ( !pastEnd() && !pastEndOfLine() ) {
	QString c =  paragraph->text.charAt( current );
#ifdef _WS_X11_
	    //### workaround for broken courier fonts on X11
	int l = c.length();
	for ( int i = 0; i < l; i++ ) {
	    if ( c[i] == QChar(0x00a0U) )
		c[i] = ' ';
	}
#endif
	if ( currentoffset  < int(c.length()) - 1 ) {
	    QTextCharFormat* fmt = format();
	    QFontMetrics fm( fmt->font() );
	    if ( p ) {
		p->setFont( fmt->font() );
		fm = p->fontMetrics();
	    }
	    currentoffset++;
	    currentoffsetx = fm.width( c, currentoffset );
	    return;
	}
    }
    (void) rightOneItem( p );
}

// TODO remove this function and its usage
void QRichTextFormatter::left( QPainter* p )
{
    if ( currentoffset > 0 ) {
	QString c =  paragraph->text.charAt( current );
#ifdef _WS_X11_
	    //### workaround for broken courier fonts on X11
	int l = c.length();
	for ( int i = 0; i < l; i++ ) {
	    if ( c[i] == QChar(0x00a0U) )
		c[i] = ' ';
	}
#endif
	QTextCharFormat* fmt = format();
	QFontMetrics fm( fmt->font() );
	if ( p ) {
	    p->setFont( fmt->font() );
	    fm = p->fontMetrics();
	}
	currentoffset--;
	currentoffsetx = fm.width( c, currentoffset );
    }
    else if ( current == 0 ) {
	if ( paragraph->prevInDocument() ) {
	    gotoParagraph( p, paragraph->prevInDocument() );
	    makeLineLayout( p );
	    gotoLineStart( p );
	    while ( !atEnd() )
		(void) rightOneItem( p );
	}
    }
    else {
	int i = current;
	if ( current == first ) {
	    gotoParagraph( p, paragraph );
	    makeLineLayout( p );
	}
	gotoLineStart( p );
	while ( current <  i - 1 )
	    (void) rightOneItem( p );
	QString c =  paragraph->text.charAt( current );
#ifdef _WS_X11_
	    //### workaround for broken courier fonts on X11
	int l = c.length();
	for ( int i = 0; i < l; i++ ) {
	    if ( c[i] == QChar(0x00a0U) )
		c[i] = ' ';
	}
#endif
	if ( c.length() > 1 ) {
	    currentoffset = c.length() - 1;
	    QTextCharFormat* fmt = format();
	    QFontMetrics fm( fmt->font() );
	    if ( p ) {
		p->setFont( fmt->font() );
		fm = p->fontMetrics();
	    }
	    currentoffsetx = fm.width( c, currentoffset );
	}
    }
}


bool QRichTextFormatter::goTo( QPainter* p, int xpos, int ypos )
{

    QTextParagraph* b = doc->getParBefore( ypos );
    while ( b ) {
	gotoParagraph( p, b );
	b = paragraph;
	b = b->nextInDocument();
	//??? update dirty stuff here?
	if ( !b || y() + paragraph->height  > ypos ) {
	    do {
		makeLineLayout( p );
		QRect geom( lineGeometry() );
		if ( ypos <= geom.bottom()  ) {
		    gotoLineStart( p );
		    bool within = geom.contains(QPoint(xpos,ypos));
		    if ( ypos >= geom.top() ) {
			while ( !atEndOfLine() && geom.left() + x() < xpos )
			    right( p );
			if ( geom.left() + x() > xpos )
			    left( p ); //####necesssary? TODO
			else
			    within = FALSE;
		    }
		    return within;
		}
	    }
	    while ( gotoNextLine( p ) );
	}
    }
    left( p );
    return FALSE;
}

void QRichTextFormatter::gotoNextItem( QPainter* p )
{
    if ( pastEnd() )
	return;
    // tabulators belong here
    QTextRichString::Item* item = &paragraph->text.items[current];
    QTextCustomItem* custom = item->format->customItem();
    if ( format() != formatinuse ) // the document may have changed
	updateCharFormat( p );
    if ( custom ) {
	if ( custom->placeInline() )
	    currentx += custom->width;
    }
    else {
	QString c = item->c;
#ifdef _WS_X11_
	    //### workaround for broken courier fonts on X11
	int l = c.length();
	for ( int i = 0; i < l; i++ ) {
	    if ( c[i] == QChar(0x00a0U) )
		c[i] = ' ';
	}
#endif
	QFontMetrics fm( formatinuse->font() );
	if ( p )
	    fm = p->fontMetrics();
	if ( item->width < 0 )
	    item->width = fm.width( c );
	currentx += item->width;
    }
    current++;
    currentoffset = currentoffsetx = 0;

    if ( current < paragraph->text.length() && !paragraph->text.haveSameFormat( current-1, current ) )
 	updateCharFormat( p );
}

void QRichTextFormatter::makeLineLayout( QPainter* p )
{
    first = current;

    if ( pastEnd() )
	return;

    last = first;
    int rh = 0;
    int rasc = 0;
    int rdesc = 0;

    // do word wrap
    int lastSpace =current;
    int lastHeight = rh;
    int lastWidth = currentx;
    int lastAsc = rasc;
    int lastDesc = rdesc;
    bool noSpaceFound = TRUE;

    QTextCharFormat* fmt = format();
    int fmt_current = current;
    QFontMetrics fm( fmt->font() );
    if ( p ) {
	p->setFont( fmt->font() );
	fm = p->fontMetrics();
    }
    int leading = fm.leading();

    widthUsed = 0;

    QList<QTextCustomItem> floatingItems;

    bool isTableRow = FALSE;
    QTextCustomItem::Clear clear = QTextCustomItem::ClearNone;
    while ( !pastEnd() ) {

	if ( !paragraph->text.haveSameFormat( fmt_current, current ) ) {
	    fmt = format();
	    fmt_current = current;
	    if ( p )
		p->setFont( fmt->font() );
	    else
		fm = QFontMetrics( fmt->font() );
	    if ( fm.leading() > leading )
		leading = fm.leading();
	}

	QChar lastc;
	QTextRichString::Item* item = &paragraph->text.items[current];
	QTextCustomItem* custom = item->format->customItem();

	if ( custom ) {
	    if (!custom->placeInline() )
		floatingItems.append( custom );
	    else if ( current == first && custom->isTable() ) {
		isTableRow = TRUE;
		( (QTextTable*)custom)->verticalBreak( y_, flow );
		currentasc = custom->height;
	    }
	} else if ( !item->c.isEmpty() ) {
	    lastc = item->c[ (int) item->c.length()-1];
	}

	bool custombreak = custom && custom->ownLine() && custom->placeInline();

	if ( custombreak && current > first ) {
	    // break _before_ a custom expander
 	    noSpaceFound = TRUE;
 	    lastc = '\n'; // fake newline
	} else {
	    if ( currentasc + currentdesc > rh )
		rh = currentasc + currentdesc;
	    if ( currentasc > rasc )
		rasc = currentasc;
	    if ( currentdesc > rdesc )
		rdesc = currentdesc;

	    gotoNextItem( p );
   	    if ( custombreak || (custom && custom->breakLine() ) ) {
   		// also break _behind_ a custom expander
   		++current;
  		lastc = '\n';
		clear = custom->clearBehind();
   	    }
	}
	// if a wordbreak is possible and required, do it. Unless we
	// have a newline, of course. In that case we break after the
	// newline to avoid empty lines.
	if ( currentx > width - rmargin && !noSpaceFound && lastc != '\n' ) {
	    break;
	}

	// word break is possible (a) after a space, (b) after a
	// newline, (c) before expandable item  or (d) at the end of the paragraph.
	if ( noSpaceFound || isBreakable(&lastc) || lastc == '\n' || current == paragraph->text.length() ){
	    lastSpace = current - 1;
	    lastHeight = rh;
	    lastAsc = rasc;
	    lastDesc = rdesc;
	    lastWidth = currentx;
 	    if ( isBreakable(&lastc) )
 		noSpaceFound = FALSE;
	}
	if ( lastc == '\n' )
	    break;
    }

    last = lastSpace;
    rh = lastHeight;
    rasc = lastAsc + leading/2;
    rdesc = lastDesc + leading/2;

    height = QMAX(rh, rasc+rdesc+1);
    base = rasc;

    fill = 0;
    switch ( alignment ) {
    case Qt::AlignLeft:
	fill = 0;
	break;
    case Qt::AlignCenter:
	fill = (width - rmargin - lastWidth) / 2;
	break;
    case Qt::AlignRight:
	fill = width - rmargin - lastWidth;
	break;
    }

    current = lastSpace;
    int min = lastWidth ;
    if ( min + rmargin > widthUsed )
	widthUsed = min + rmargin;
    if ( widthUsed > width )
	fill = 0; // fall back to left alignment if there isn't sufficient space

    if ( clear != QTextCustomItem::ClearNone ) {
	int lm = lmargin;
	int rm = rmargin;
	height = QMAX( height, adjustHorizontalMargins( clear ) );
	lmargin = lm;
	rmargin = rm;
    }

    flow->adjustFlow( y_, widthUsed, height, !isTableRow ) ;

    int fl = lmargin;
    int fr = width - rmargin;
    for ( QTextCustomItem* item = floatingItems.first(); item; item = floatingItems.next() ) {
	item->ypos = y_ + height + 1;
	if ( item->isTable() )
	    ( (QTextTable*)item)->verticalBreak( item->ypos, flow );
	flow->adjustFlow( item->ypos, item->width, item->height,
			  !item->isTable() );
	if ( item->placement() == QTextCustomItem::PlaceRight ) {
	    fr -= item->width;
	    item->xpos = fr;
	} else {
	    item->xpos = fl;
	    fl += item->width;
	}
	flow->registerFloatingItem( item, item->placement() == QTextCustomItem::PlaceRight );
    }
}

// Note: The cursor's paragraph needs to be initialized
bool QRichTextFormatter::updateLayout( QPainter* p, int ymax )
{
    QTextParagraph* b = paragraph;
    gotoParagraph( p, b );
    while ( b && ( ymax < 0 || y_ <= ymax ) ) {

	if ( !b->dirty )
	    y_ = b->ypos + b->height;
	else do {
		makeLineLayout( p );
	} while ( gotoNextLine( p ) );

	b = b->nextInDocument();
	if ( b ) {
	    if ( b->dirty ) {
		initParagraph( p, b );
	    } else {
		gotoParagraph( p, b );
	    }
	}
    }
    return b == 0;
}

QRect QRichTextFormatter::lineGeometry() const
{
    int realWidth = QMAX( width, widthUsed );
    return QRect( 0, y(), realWidth, height );
}

QtTriple QRichTextFormatter::position() const
{
    return QtTriple( paragraph->id, current, currentoffset );
}

QTextFlow::QTextFlow()
{
    width = widthUsed = height = pagesize = 0;
}

QTextFlow::~QTextFlow()
{
}

void QTextFlow::initialize( int w)
{
    height = 0;
    width = w;
    widthUsed = 0;

    leftItems.clear();
    rightItems.clear();
}

int QTextFlow::adjustLMargin( int yp, int margin, int space )
{
    for ( QTextCustomItem* item = leftItems.first(); item; item = leftItems.next() ) {
	if ( yp >= item->ypos && yp < item->ypos + item->height )
	    margin = QMAX( margin, item->xpos + item->width + space );
    }
    return margin;
}

int QTextFlow::adjustRMargin( int yp, int margin, int space )
{
    for ( QTextCustomItem* item = rightItems.first(); item; item = rightItems.next() ) {
	if ( yp >= item->ypos && yp < item->ypos + item->height )
	    margin = QMAX( margin, width - item->xpos - space );
    }
    return margin;
}

void QTextFlow::adjustFlow( int  &yp, int w, int h, bool pages )
{
    if ( w > widthUsed )
	widthUsed = w;


    if ( pages && pagesize > 0 ) { // check pages
	int ty = yp;
	int yinpage = ty % pagesize;
 	if ( yinpage < 2 )
 	    yp += 2 - yinpage;
 	else
	    if ( yinpage + h > pagesize - 2 )
	    yp += ( pagesize - yinpage ) + 2;
    }

    if ( yp + h > height )
	height = yp + h;

}

void QTextFlow::registerFloatingItem( QTextCustomItem* item, bool right   )
{
    if ( right ) {
	if ( !rightItems.contains( item ) )
	    rightItems.append( item );
    }
    else if ( !leftItems.contains( item ) )
	leftItems.append( item );
}

void QTextFlow::drawFloatingItems(QPainter* p,
				   int ox, int oy, int cx, int cy, int cw, int ch,
				   QRegion& backgroundRegion, const QColorGroup& cg, const QTextOptions& to )
{
    QTextCustomItem *item;
    for ( item = leftItems.first(); item; item = leftItems.next() ) {
	item->draw( p, item->xpos, item->ypos, ox, oy, cx, cy, cw, ch, backgroundRegion, cg, to );
    }

    for ( item = rightItems.first(); item; item = rightItems.next() ) {
	item->draw( p, item->xpos, item->ypos, ox, oy, cx, cy, cw, ch, backgroundRegion, cg, to );
    }
}

QTextTable::QTextTable(const QMap<QString, QString> & attr  )
{
    cellspacing = 2;
    if ( attr.contains("cellspacing") )
	cellspacing = attr["cellspacing"].toInt();
    cellpadding = 1;
    if ( attr.contains("cellpadding") )
	cellpadding = attr["cellpadding"].toInt();
    border = 0;
    innerborder = 1;
    if ( attr.contains("border" ) ) {
	QString s( attr["border"] );
	if ( s == "true" )
	    border = 1;
	else
	    border = attr["border"].toInt();
    }

    if ( border )
	cellspacing += 2;
    outerborder = cellspacing + border;
    layout = new QGridLayout( 1, 1, cellspacing );

    fixwidth = 0;
    stretch = 0;
    if ( attr.contains("width") ) {
	bool b;
	QString s( attr["width"] );
	int w = s.toInt( &b );
	if ( b ) {
	    fixwidth = w;
	} else {
 	    s = s.stripWhiteSpace();
 	    if ( s.length() > 1 && s[ (int)s.length()-1 ] == '%' )
		stretch = s.left( s.length()-1).toInt();
	}
    }

    place = PlaceInline;
    if ( attr["align"] == "left" )
	place = PlaceLeft;
    else if ( attr["align"] == "right" )
	place = PlaceRight;
    cachewidth = 0;
}

QTextTable::~QTextTable()
{
    delete layout;
}

void QTextTable::realize( QPainter* p)
{
    painter = p;
    if ( p && p->device()->devType() != QInternal::Printer ) {
	QPaintDeviceMetrics metrics(p->device());
	double xscale = scale_factor( metrics.logicalDpiX() );
	cellspacing = int(cellspacing * xscale);
	border = int(border * xscale);
	innerborder = int(innerborder * xscale);
	outerborder = int(outerborder * xscale);
    }
    for (QTextTableCell* cell = cells.first(); cell; cell = cells.next() )
	cell->realize();

    width = 0;
}

void QTextTable::verticalBreak( int  yt, QTextFlow* flow )
{
    if ( flow->pagesize <= 0 )
	return;
    int shift = 0;
    for (QTextTableCell* cell = cells.first(); cell; cell = cells.next() ) {
	QRect r = cell->geometry();
 	r.moveBy(0, shift );
	cell->setGeometry( r );
	if ( cell->column() == 0 ) {
	    int y = yt + outerborder + cell->geometry().y();
	    int oldy = y;
	    flow->adjustFlow( y, width, cell->geometry().height() + 2*cellspacing );
	    shift += y - oldy;
	    r = cell->geometry();
 	    r.moveBy(0, y - oldy );
	    cell->setGeometry( r );
	}
    }
    height += shift;
}

void QTextTable::draw(QPainter* p, int x, int y,
		       int ox, int oy, int cx, int cy, int cw, int ch,
		       QRegion& backgroundRegion, const QColorGroup& cg, const QTextOptions& to )
{
    painter = p;
    for (QTextTableCell* cell = cells.first(); cell; cell = cells.next() ) {
	if ( y + outerborder + cell->geometry().top() - cellspacing < cy+ch
	     && y + outerborder + 2*cellspacing + cell->geometry().bottom() >= cy ) {
	    cell->draw( x+outerborder, y+outerborder, ox, oy, cx, cy, cw, ch, backgroundRegion, cg, to );
	    if ( border ) {
		QRect r( x+outerborder+cell->geometry().x()-innerborder-ox,
			 y+outerborder+cell->geometry().y()-innerborder-oy,
			 cell->geometry().width()+2*innerborder,
			 cell->geometry().height()+2*innerborder);
		int s = cellspacing;
		if ( p->device()->devType() == QInternal::Printer ) {
		    qDrawPlainRect( p, r, cg.text(), innerborder );
		} else {
		    p->fillRect( r.left()-s, r.top(), s, r.height(), cg.button() );
		    p->fillRect( r.right(), r.top(), s, r.height(), cg.button() );
		    p->fillRect( r.left()-s, r.top()-s, r.width()+2*s, s, cg.button() );
		    p->fillRect( r.left()-s, r.bottom(), r.width()+2*s, s, cg.button() );
		    qDrawShadePanel( p, r, cg, TRUE, innerborder );
		}
	    }
	}
    }
    if ( border ) {
	QRect r ( x-ox, y-oy, width, height );
	if ( p->device()->devType() == QInternal::Printer ) {
	    qDrawPlainRect( p, QRect(r.x()+1, r.y()+1, r.width()-2, r.height()-2), cg.text(), border );
	} else {
	    int s = border;
	    p->fillRect( r.left(), r.top(), s, r.height(), cg.button() );
	    p->fillRect( r.right()-s, r.top(), s, r.height(), cg.button() );
	    p->fillRect( r.left(), r.top(), r.width(), s, cg.button() );
	    p->fillRect( r.left(), r.bottom()-s, r.width(), s, cg.button() );
	    qDrawShadePanel( p, r, cg, FALSE, border );
	}
	backgroundRegion = backgroundRegion.subtract( r );
    }
}

void QTextTable::resize( QPainter* p, int nwidth )
{
    if ( nwidth == cachewidth )
	return;
    cachewidth = nwidth;
    painter = p;

    if ( stretch )
	nwidth = nwidth * stretch / 100;

    width = nwidth + 2*outerborder;
    layout->invalidate();
    int shw = layout->sizeHint().width() + 2*outerborder;
    int mw = layout->minimumSize().width() + 2*outerborder;
    if ( stretch )
	width = QMAX( mw, nwidth );
    else
	width = QMAX( mw, QMIN( nwidth, shw ) );

    if ( fixwidth )
	width = fixwidth;

    // play it again, Sam, to fix the stretches
    layout->invalidate();
    mw = layout->minimumSize().width() + 2*outerborder;
    width = QMAX( width, mw );

    int h = layout->heightForWidth( width-2*outerborder );
    layout->setGeometry( QRect(0, 0, width-2*outerborder, h)  );
    height = layout->geometry().height()+2*outerborder;
};

void QTextTable::addCell( QTextTableCell* cell )
{
    cells.append( cell );
    layout->addMultiCell( cell, cell->row(), cell->row() + cell->rowspan()-1,
			  cell->column(), cell->column() + cell->colspan()-1 );
}

QTextTableCell::QTextTableCell(QTextTable* table,
       int row, int column,
       const QMap<QString, QString> &attr,
       const QStyleSheetItem* style,
       const QTextCharFormat& fmt, const QString& context,
       const QMimeSourceFactory &factory, const QStyleSheet *sheet, const QString& doc, int& pos )
{
    maxw = QWIDGETSIZE_MAX;
    minw = 0;

    parent = table;
    row_ = row;
    col_ = column;
    stretch_ = 0;
    richtext = new QRichText( attr, doc, pos, style,
			      fmt, context, parent->cellpadding, &factory, sheet );
    rowspan_ = 1;
    colspan_ = 1;
    if ( attr.contains("colspan") )
	colspan_ = attr["colspan"].toInt();
    if ( attr.contains("rowspan") )
	rowspan_ = attr["rowspan"].toInt();

    background = 0;
    if ( attr.contains("bgcolor") ) {
	background = new QBrush(QColor( attr["bgcolor"] ));
    }

    hasFixedWidth = FALSE;
    if ( attr.contains("width") ) {
	bool b;
	QString s( attr["width"] );
	int w = s.toInt( &b );
	if ( b ) {
	    maxw = w;
	    minw = maxw;
	    hasFixedWidth = TRUE;
	} else {
 	    s = s.stripWhiteSpace();
 	    if ( s.length() > 1 && s[ (int)s.length()-1 ] == '%' )
		stretch_ = s.left( s.length()-1).toInt();
	}
    }

    parent->addCell( this );
}

QTextTableCell::~QTextTableCell()
{
    delete background;
    background = 0;
    QTextFormatCollection* formats = richtext?richtext->formats:0;
    delete richtext;
    delete formats; //#### fix inheritance structure in rich text
}

QSize QTextTableCell::sizeHint() const
{
    //### see QLabel::sizeHint()
    return QSize(maxw,0).expandedTo( minimumSize() );
}

QSize QTextTableCell::minimumSize() const
{
    if ( stretch_ )
	return QSize( QMAX( minw, parent->width * stretch_ / 100 - 2*parent->cellspacing), 0);
    return QSize(minw,0);
}

QSize QTextTableCell::maximumSize() const
{
    return QSize( QWIDGETSIZE_MAX, QWIDGETSIZE_MAX );
}

QSizePolicy::ExpandData QTextTableCell::expanding() const
{
    return QSizePolicy::BothDirections;
}

bool QTextTableCell::isEmpty() const
{
    return FALSE;
}
void QTextTableCell::setGeometry( const QRect& r)
{
    if ( r.width() != richtext->flow()->width ) {
	richtext->doLayout( painter(), r.width() );
    }
    geom = r;
}
QRect QTextTableCell::geometry() const
{
    return geom;
}

bool QTextTableCell::hasHeightForWidth() const
{
    return TRUE;
}

int QTextTableCell::heightForWidth( int w ) const
{
    w = QMAX( minw, w ); //####PAUL SHOULD DO THAT

    if ( richtext->flow()->width != w ) {
	QTextTableCell* that = (QTextTableCell*) this;
	that->richtext->doLayout(painter(), w );
    }
    return richtext->flow()->height;
}

void QTextTableCell::realize()
{

    if ( hasFixedWidth )
	return;

    richtext->doLayout(painter(), QWIDGETSIZE_MAX );
    maxw = richtext->flow()->widthUsed + 6;
    richtext->doLayout(painter(), 0 );
    minw = richtext->flow()->widthUsed;
}

QPainter* QTextTableCell::painter() const
{
    return parent->painter;
}

void QTextTableCell::draw(int x, int y,
			int ox, int oy, int cx, int cy, int cw, int ch,
			QRegion& backgroundRegion, const QColorGroup& cg, const QTextOptions& to )
{
    if ( richtext->flow()->width != geom.width() )
	richtext->doLayout(painter(), geom.width() );

    QTextOptions o(to);
    if ( background )
	o.paper = background;

    QRect r(x-ox+geom.x(), y-oy+geom.y(), geom.width(), geom.height() );
    richtext->draw(painter(), x+geom.x(), y+geom.y(), ox, oy, cx, cy, cw, ch, backgroundRegion, cg, o );

     if ( painter()->device()->devType() != QInternal::Printer ) {
	painter()->setClipRegion( backgroundRegion );
	o.erase( painter(), r );
    }
    backgroundRegion = backgroundRegion.subtract( r );
}

QTextCharFormat::QTextCharFormat()
    : logicalFontSize( 3 ), stdPointSize( 12 ),
      custom( 0 ), linkColor( TRUE )
{
}

QTextCharFormat::QTextCharFormat( const QTextCharFormat &format )
    : QShared(), font_( format.font_ ), color_( format.color_ ),
      key( format.key ),
      logicalFontSize( format.logicalFontSize ),
      stdPointSize( format.stdPointSize ),
      anchor_href( format.anchor_href ),
      anchor_name( format.anchor_name ),
      parent(0), custom( format.custom ),
      linkColor( format.linkColor )
{
    if ( custom )
	custom->ref();
}

QTextCharFormat::QTextCharFormat( const QFont &f, const QColor &c )
    : font_( f ), color_( c ), logicalFontSize( 3 ), stdPointSize( f.pointSize() ),
      parent(0), custom( 0 ), linkColor( TRUE )
{
    createKey();
}

QTextCharFormat::~QTextCharFormat()
{
    if ( custom && custom->deref() ) {
	delete custom;
	custom = 0; // helps debugging
    }
}

void QTextCharFormat::createKey()
{
    QTextOStream ts( &key );
    ts
	<< font_.pointSize() << "_"
	<< font_.weight() << "_"
	<< (int)font_.underline()
	<< (int) font_.italic()
	<< anchor_href << "_"
	<< anchor_name << "_"
	<< color_.pixel()
	<< font_.family() << "_"
	<<(ulong) custom;
}

QTextCharFormat &QTextCharFormat::operator=( const QTextCharFormat &fmt )
{
    font_ = fmt.font_;
    color_ = fmt.color_;
    key = fmt.key;
    logicalFontSize = fmt.logicalFontSize;
    stdPointSize = fmt.stdPointSize;
    anchor_href = fmt.anchor_href;
    anchor_name = fmt.anchor_name;
    custom = fmt.custom;
    linkColor = fmt.linkColor;
    if ( custom )
	custom->ref();
    return *this;
}

bool QTextCharFormat::operator==( const QTextCharFormat &format )
{
    return format.key == key;
}

QTextCharFormat QTextCharFormat::makeTextFormat( const QStyleSheetItem *style,
						   const QMap<QString,QString>& attr,
						   QTextCustomItem*  item ) const
{
    QTextCharFormat format(*this);
    format.custom = item;
    bool changed = FALSE;
    if ( style ) {
	if ( style->name() == "font") {

	    if ( attr.contains("color") ) {
		format.color_.setNamedColor( attr["color"] );
		format.linkColor = FALSE;
	    }
	    if ( attr.contains("size") ) {
		QString a = attr["size"];
		int n = a.toInt();
		if ( a[0] == '+' || a[0] == '-' )
		    n += format.logicalFontSize;
		format.logicalFontSize = n;
		format.font_.setPointSize( format.stdPointSize );
		style->styleSheet()->scaleFont( format.font_, format.logicalFontSize );
	    }
	    if ( attr.contains("face") ) {
		QString a = attr["face"];
		if ( a.contains(',') )
		    a = a.left( a.find(',') );
		format.font_.setFamily( a );
	    }
	} else {

	    if ( style->isAnchor() ) {
		format.anchor_href = attr["href"];
		format.anchor_name = attr["name"];
		changed = TRUE;
	    }

	    if ( style->fontWeight() != QStyleSheetItem::Undefined )
		format.font_.setWeight( style->fontWeight() );
	    if ( style->fontSize() != QStyleSheetItem::Undefined )
		format.font_.setPointSize( style->fontSize() );
	    else if ( style->logicalFontSize() != QStyleSheetItem::Undefined ) {
		format.logicalFontSize = style->logicalFontSize();
		format.font_.setPointSize( format.stdPointSize );
		style->styleSheet()->scaleFont( format.font_, format.logicalFontSize );
	    }
	    else if ( style->logicalFontSizeStep() ) {
		format.logicalFontSize += style->logicalFontSizeStep();
		format.font_.setPointSize( format.stdPointSize );
		style->styleSheet()->scaleFont( format.font_, format.logicalFontSize );
	    }
	    if ( !style->fontFamily().isEmpty() )
		format.font_.setFamily( style->fontFamily() );
	    if ( style->color().isValid() )
		format.color_ = style->color();
	    if ( style->definesFontItalic() )
		format.font_.setItalic( style->fontItalic() );
	    if ( style->definesFontUnderline() )
		format.font_.setUnderline( style->fontUnderline() );
	}
    }

    if ( item || font_ != format.font_ || changed || color_ != format.color_) // slight performance improvement
	format.createKey();
    return format;
}


QTextCharFormat QTextCharFormat::formatWithoutCustom()
{
    QTextCharFormat fm( *this );
    fm.custom = 0;
    fm.createKey();
    return fm;
}

QTextFormatCollection::QTextFormatCollection()
    : cKey(199),lastRegisterFormat( 0 )
{
}

QTextCharFormat* QTextFormatCollection::registerFormat( const QTextCharFormat &format )
{
    if ( format.parent == this ) {
	QTextCharFormat* f = ( QTextCharFormat*) &format;
	f->ref();
	lastRegisterFormat = f;
	return f;
    }

    if ( lastRegisterFormat ) {
        if ( format.key == lastRegisterFormat->key ) {
	    lastRegisterFormat->ref();
	    return lastRegisterFormat;
        }
    }

    if ( format.isAnchor() ) {
	// fancy speed optimization: do _not_ share any anchors to keep the map smaller
	// see unregisterFormat()
	lastRegisterFormat =  new QTextCharFormat( format );
	return lastRegisterFormat;
    }

    QTextCharFormat *fc = cKey[ format.key ];
    if ( fc ) {
	fc->ref();
	lastRegisterFormat = fc;
	return fc;
    } else {
	QTextCharFormat *f = new QTextCharFormat( format );
	f->parent = this;
	cKey.insert( f->key, f );
	lastRegisterFormat = f;
	return f;
    }
}

void QTextFormatCollection::unregisterFormat( const QTextCharFormat &format )
{
    QTextCharFormat* f;

    if ( format.isAnchor() ) {
	// fancy speed optimization: do _not_ share any anchors to keep the map smaller
	// see registerFormat()
	f = (QTextCharFormat*)&format;
	if ( f->deref() ) {
	    if ( f == lastRegisterFormat )
		lastRegisterFormat = 0;
	    delete f;
	}
	return;
    }

    if ( format.parent == this )
	f = ( QTextCharFormat*)&format;
    else
	f = cKey[ format.key ];

    if ( f ) {
	if ( f->deref() ) {
	    if ( f == lastRegisterFormat )
		lastRegisterFormat = 0;
	    cKey.remove( format.key );
	    delete f;
	}
    }
}

#endif // QT_NO_RICHTEXT

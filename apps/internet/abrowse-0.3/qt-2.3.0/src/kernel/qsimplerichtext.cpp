/****************************************************************************
** $Id$
**
** Implementation of the QSimpleRichText class
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

#include "qsimplerichtext.h"
#include "qpainter.h"


// NOT REVISED
/*!
  \class QSimpleRichText qsimplerichtext.h
  \brief A small displayable piece of rich text.

  \ingroup drawing

  This class encapsulates simple richt text usage where a string is
  interpreted as richt text and can be drawn.  This is in particular
  useful if you want to display some rich text in a custom widget.
  A QStyleSheet is needed to actually understand and format rich text.
  Qt provides a default HTML-like style sheet but you may define custom
  style sheets.

  Once created, the rich text object can be queried for its width(),
  height() and the actual width used (see widthUsed()).  Most
  importantly, it can be drawn on any given QPainter with draw().
  QSimpleRichText can also be used to implement hypertext or active
  text facilities by using anchorAt().  A hit test through inText()
  makes it possible to use simple rich text for text objects in
  editable drawing canvases.

  Once constructed from a string, the contents cannot be changed, only
  resized.  If the contents should change, just throw the rich text
  object away and make a new one with the new contents.

  For large documents, see QTextView or QTextBrowser.
*/

class QSimpleRichTextData
{
public:
    QRichText* doc;
    QColor linkColor;
    bool linkUnderline;
    QFont font;
};

/*!
  Constructs a QSimpleRichText from the rich text string \a text
  and the font \a fnt.

  The font is used as basis for the text rendering. When using rich
  text rendering on a certain widget \e w, you would regularly
  specify the widget's font as shown in the following code example:

  \code
  QSimpleRichText myrichtext( contents, mywidget->font() );
  \endcode

  \a context is the optional context of the document. This becomes
  important if \a text contains relative references, for example
  within image tags. QSimpleRichText always uses the default mime
  source factory (see QMimeSourceFactory::defaultFactory() ) to
  resolve those references. The context will then be used to calculate
  the absolute path. See QMimeSourceFactory::makeAbsolute() for
  details.

  Finally \a s is an optional style sheet. If it is 0, the default
  style sheet will be used (see QStyleSheet::defaultSheet() ).

*/
QSimpleRichText::QSimpleRichText( const QString& text, const QFont& fnt,
				  const QString& context, const QStyleSheet* sheet)
{
    d  = new QSimpleRichTextData;
    d->linkColor = Qt::blue;
    d->linkUnderline = TRUE;
    d->font = fnt;
    d->doc = new QRichText( text, fnt, context, 0, 0, sheet );
}


/*!  Another, more complex constructor for QSimpleRichText that takes
  an additional mime source factory \a factory, a vertical break
  parameter \a verticalBreak, a link color \a linkColor and a bool \a
  linkUnderline.

  The constructor is useful to create a QSimpleRichText object
  suitable for printing. Set \a verticalBreak to be the height of the
  contents area of the pages.
 */
QSimpleRichText::QSimpleRichText( const QString& text, const QFont& fnt,
		 const QString& context,
				  const QStyleSheet* sheet, const QMimeSourceFactory* factory, int verticalBreak,
				  const QColor& linkColor, bool linkUnderline )
{
    d  = new QSimpleRichTextData;
    d->linkColor = linkColor;
    d->linkUnderline = linkUnderline;
    d->doc = new QRichText( text, fnt, context, 0, factory, sheet );
    d->doc->flow()->pagesize = verticalBreak;
    d->font = fnt;
}


/*!
  Destructs the document, freeing memory.
*/
QSimpleRichText::~QSimpleRichText()
{
    QTextFormatCollection* formats = d->doc?d->doc->formats:0;
    delete d->doc;
    delete formats; //#### fix inheritance structure in rich text
    delete d;
}

/*!
  Sets the width of the document to \a w pixels.

  \sa height(), adjustSize()
*/
void QSimpleRichText::setWidth( int w)
{
    d->doc->doLayout( 0, w );
}

/*!
  Sets the width of the document to \a w pixels, recalculating the layout
  as if it were to be drawn with \a p.

  Passing a painter is useful when you intend to draw on devices other
  than the screen, like for example a QPrinter.

  \sa height(), adjustSize()
*/
void QSimpleRichText::setWidth( QPainter* p, int w)
{
    d->doc->doLayout( p, w );
}

/*!
  Returns the set width of the document, in pixels.

  \sa widthUsed()
*/
int QSimpleRichText::width() const
{
    return d->doc->flow()->width;
}


/*!
  Returns the width in pixels that is actually used by the document.
  This can be smaller or wider than the set width.

  It may be wider, for example, if the text contains images or
  non-breakable words that are already wider than the available
  space. It's smaller when the document only consists of lines that do
  not fill the width completely.

  \sa width()
*/
int QSimpleRichText::widthUsed() const
{
    return d->doc->flow()->widthUsed;
}


/*!
  Returns the height of the document, in pixels.
  \sa setWidth()
*/
int QSimpleRichText::height() const
{
    return d->doc->flow()->height;
}


/*!
  Draws the formatted text with \a p, at position (\a x, \a y), clipped
  to \a clipRegion.  Colors from the palette \a pal are used as
  needed, and if not 0, *\a paper is used as the background brush.

  Note that the display code is highly optimized to reduce flicker, so
  passing a brush for \a paper is preferable to simply clearing the area
  to be painted and then calling this without a brush.
*/
void QSimpleRichText::draw( QPainter* p,  int x, int y, const QRegion& clipRegion,
	   const QPalette& pal, const QBrush* paper ) const
{
    draw( p, x, y, clipRegion, pal.normal(), paper );
}

/*!\overload
  Draws the formatted text with \a p, at position (\a x, \a y), clipped
  to \a clipRegion.  Colors from the \a cg are used as
  needed, and if not 0, *\a paper is used as the background brush.

  Note that the display code is highly optimized to reduce flicker, so
  passing a brush for \a paper is preferable to simply clearing the area
  to be painted and then calling this without a brush.

  This is a convenience function if there's no palette but just a
  color group available. If you have a palette, pass this instead of \a cg.
*/
void QSimpleRichText::draw( QPainter* p,  int x, int y, const QRegion& clipRegion,
			      const QColorGroup& cg, const QBrush* paper) const
{
    QRect r = clipRegion.boundingRect();
    QRegion bg = clipRegion;

    d->doc->draw(p, x, y, 0, 0, r.x(), r.y(), r.width(), r.height(), bg, cg,
		 QTextOptions( paper, d->linkColor, d->linkUnderline ) );
    if (paper) {
	p->setClipRegion(bg);
	if ( paper->pixmap() )
	    p->drawTiledPixmap( r, *paper->pixmap());
	else
	    p->fillRect(r, *paper);
	p->setClipping( FALSE );
    }
}


/*!
  Returns the context of the rich text document. If no context has been specified
  in the constructor, a null string is returned.
*/
QString QSimpleRichText::context() const
{
    return d->doc->context();
}

/*!\obsolete
  Returns the anchor at the requested position. The QPainter is needed for font size
  calculations. An empty string is returned if no anchor is specified for this certain
  position.

  This function will be removed in 3.0
*/
QString QSimpleRichText::anchor( QPainter*, const QPoint& pos )
{
    return anchorAt( pos );
}

/*! Returns the anchor at the requested position. An empty string is
  returned if no anchor is specified for this certain position.
*/
QString QSimpleRichText::anchorAt( const QPoint& pos ) const
{
    return d->doc->anchorAt( pos );
}


// Dijkstra's bisection algorithm to find the square root as an integer.
// The argument n must be in the range 0...1'073'741'823 [2^31-1].

static uint int_sqrt(uint n)
{
    uint h, p= 0, q= 1, r= n;
    ASSERT( n < 1073741824U );  // UINT_MAX>>2 on 32-bits architecture
    while ( q <= n )
	q <<= 2;
    while ( q != 1 ) {
	q >>= 2;
	h= p + q;
	p >>= 1;
	if ( r >= h ) {
	    p += q;
	    r -= h;
	}
    }
    return p;
}


/*!
  Adjusts the richt text document to a reasonable size.

  \sa setWidth()
*/
void QSimpleRichText::adjustSize()
{
    QFontMetrics fm( d->font );
    int mw =  fm.width( 'x' ) * 80;
    int w = mw;
    d->doc->doLayout( 0,w );
    if ( d->doc->flow()->widthUsed != 0 ) {
	w = int_sqrt( (5*d->doc->height) / (3*d->doc->flow()->widthUsed ) );
	d->doc->doLayout( 0, QMIN( w, mw) );

	if ( w*3 < 5*d->doc->flow()->height ) {
	    w = int_sqrt(6*d->doc->flow()->height/3*d->doc->flow()->widthUsed);
	    d->doc->doLayout( 0,QMIN(w, mw ) );
	}
    }
}


/*!
  Returns whether \a pos is within a text line of the document or not.
 */
bool QSimpleRichText::inText( const QPoint& pos ) const
{
    QRichTextIterator it( *d->doc );
    return it.goTo( pos );
}

#endif // QT_NO_RICHTEXT

#include <stdio.h>

#include <qbrush.h>
#include <qpixmap.h>


/*****************************************************************************
  QBrush member functions
 *****************************************************************************/

/*!
  \class QBrush qbrush.h

  \brief The QBrush class defines the fill pattern of shapes drawn by a QPainter.

  \ingroup drawing
  \ingroup shared

  A brush has a style and a color.  One of the brush styles is a custom
  pattern, which is defined by a QPixmap.

  The brush style defines the fill pattern. The default brush style is \c
  NoBrush (depends on how you construct a brush).  This style tells the
  painter to not fill shapes. The standard style for filling is called \c
  SolidPattern.

  The brush color defines the color of the fill pattern.
  The QColor documentation lists the predefined colors.

  Use the QPen class for specifying line/outline styles.

  Example:
  \code
    QPainter painter;
    QBrush   brush( yellow );		// yellow solid pattern
    painter.begin( &anyPaintDevice );	// paint something
    painter.setBrush( brush );		// set the yellow brush
    painter.setPen( NoPen );		// do not draw outline
    painter.drawRect( 40,30, 200,100 ); // draw filled rectangle
    painter.setBrush( NoBrush );	// do not fill
    painter.setPen( black );		// set black pen, 0 pixel width
    painter.drawRect( 10,10, 30,20 );	// draw rectangle outline
    painter.end();			// painting done
  \endcode

  See the setStyle() function for a complete list of brush styles.

  \sa QPainter, QPainter::setBrush(), QPainter::setBrushOrigin()
*/


/*!
  \internal
  Initializes the brush.
*/

void QBrush::init( const QColor &color, BrushStyle style )
{
    data = new QBrushData;
    CHECK_PTR( data );
    data->style	 = style;
    data->color	 = color;
    data->pixmap = 0;
}

/*!
  Constructs a default black brush with the style \c NoBrush (will not fill
  shapes).
*/

QBrush::QBrush()
{
    init( Qt::black, NoBrush );
}

/*!
  Constructs a black brush with the specified style.
  \sa setStyle()
*/

QBrush::QBrush( BrushStyle style )
{
    init( Qt::black, style );
}

/*!
  Constructs a brush with a specified color and style.
  \sa setColor(), setStyle()
*/

QBrush::QBrush( const QColor &color, BrushStyle style )
{
    init( color, style );
}

/*!
  Constructs a brush with a specified color and a custom pattern.

  The color will only have an effect for monochrome pixmaps, i.e.
  QPixmap::depth() == 1.

  \sa setColor(), setPixmap()
*/

QBrush::QBrush( const QColor &color, const QPixmap &pixmap )
{
    init( color, CustomPattern );
    setPixmap( pixmap );
}

/*!
  Constructs a brush which is a
  \link shclass.html shallow copy\endlink of \a b.
*/

QBrush::QBrush( const QBrush &b )
{
    data = b.data;
    data->ref();
}

/*!
  Destructs the brush.
*/

QBrush::~QBrush()
{
    if ( data->deref() ) {
	delete data->pixmap;
	delete data;
    }
}


/*!
  Detaches from shared brush data to makes sure that this brush is the only
  one referring the data.

  If multiple brushes share common data, this pen dereferences the data
  and gets a copy of the data. Nothing is done if there is just a single
  reference.
*/

void QBrush::detach()
{
    if ( data->count != 1 )
	*this = copy();
}


/*!
  Assigns \a b to this brush and returns a reference to this brush.
*/

QBrush &QBrush::operator=( const QBrush &b )
{
    b.data->ref();				// beware of b = b
    if ( data->deref() ) {
	delete data->pixmap;
	delete data;
    }
    data = b.data;
    return *this;
}


/*!
  Returns a
  \link shclass.html deep copy\endlink of the brush.
*/

QBrush QBrush::copy() const
{
    if ( data->style == CustomPattern ) {     // brush has pixmap
	QBrush b( data->color, *data->pixmap );
	return b;
    } else {				      // brush has std pattern
	QBrush b( data->color, data->style );
	return b;
    }
}


/*!
  \fn BrushStyle QBrush::style() const
  Returns the brush style.
  \sa setStyle()
*/

/*!
  Sets the brush style to \a s.

  The brush styles are:
  <ul>
  <li> \c NoBrush  will not fill shapes (default).
  <li> \c SolidPattern  solid (100%) fill pattern.
  <li> \c Dense1Pattern  94% fill pattern.
  <li> \c Dense2Pattern  88% fill pattern.
  <li> \c Dense3Pattern  63% fill pattern.
  <li> \c Dense4Pattern  50% fill pattern.
  <li> \c Dense5Pattern  37% fill pattern.
  <li> \c Dense6Pattern  12% fill pattern.
  <li> \c Dense7Pattern  6% fill pattern.
  <li> \c HorPattern  horizontal lines pattern.
  <li> \c VerPattern  vertical lines pattern.
  <li> \c CrossPattern  crossing lines pattern.
  <li> \c BDiagPattern  diagonal lines (directed / ) pattern.
  <li> \c FDiagPattern  diagonal lines (directed \ ) pattern.
  <li> \c DiagCrossPattern  diagonal crossing lines pattern.
  <li> \c CustomPattern  set when a pixmap pattern is being used.
  </ul>

  \sa style()
*/

void QBrush::setStyle( BrushStyle s )		// set brush style
{
    if ( data->style == s )
	return;
#if defined(CHECK_RANGE)
    if ( s == CustomPattern )
	qWarning( "QBrush::setStyle: CustomPattern is for internal use" );
#endif
    detach();
    data->style = s;
}


/*!
  \fn const QColor &QBrush::color() const
  Returns the brush color.
  \sa setColor()
*/

/*!
  Sets the brush color to \a c.
  \sa color(), setStyle()
*/

void QBrush::setColor( const QColor &c )
{
    detach();
    data->color = c;
}


/*!
  \fn QPixmap *QBrush::pixmap() const
  Returns a pointer to the custom brush pattern.

  A null pointer is returned if no custom brush pattern has been set.

  \sa setPixmap()
*/

/*!
  Sets the brush pixmap.  The style is set to \c CustomPattern.

  The current brush color will only have an effect for monochrome pixmaps,
  i.e.	QPixmap::depth() == 1.

  \sa pixmap(), color()
*/

void QBrush::setPixmap( const QPixmap &pixmap )
{
    detach();
    if ( data->pixmap )
	delete data->pixmap;
    if ( pixmap.isNull() ) {
	data->style  = NoBrush;
	data->pixmap = 0;
    } else {
	data->style = CustomPattern;
	data->pixmap = new QPixmap( pixmap );
#ifndef __ATHEOS__	
	if ( data->pixmap->optimization() == QPixmap::MemoryOptim )
	    data->pixmap->setOptimization( QPixmap::NormalOptim );
#endif	
    }
}


/*!
  \fn bool QBrush::operator!=( const QBrush &b ) const
  Returns TRUE if the brush is different from \a b, or FALSE if the brushes are
  equal.

  Two brushes are different if they have different styles, colors or pixmaps.

  \sa operator==()
*/

/*!
  Returns TRUE if the brush is equal to \a b, or FALSE if the brushes are
  different.

  Two brushes are equal if they have equal styles, colors and pixmaps.

  \sa operator!=()
*/

bool QBrush::operator==( const QBrush &b ) const
{
    return (b.data == data) || (b.data->style == data->style &&
	    b.data->color  == data->color &&
	    b.data->pixmap == data->pixmap);
}








/*

QBrush::QBrush()
{
//    printf( "Warning: QBrush::QBrush:1() not implemented\n" );
}

QBrush::QBrush( const QColor& cColor, BrushStyle=SolidPattern )
{
    m_color = cColor;
//    printf( "Warning: QBrush::QBrush:2() not implemented\n" );
}

const QColor& QBrush::color() const
{
//    printf( "Warning: QBrush::color() not implemented\n" );
    return m_color;
}

*/

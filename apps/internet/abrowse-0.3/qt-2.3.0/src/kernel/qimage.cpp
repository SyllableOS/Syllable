/****************************************************************************
** $Id$
**
** Implementation of QImage and QImageIO classes
**
** Created : 950207
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

#include "qimage.h"
#include "qregexp.h"
#include "qfile.h"
#include "qdatastream.h"
#include "qtextstream.h"
#include "qbuffer.h"
#include "qlist.h"
#include "qasyncimageio.h"
#include "qpngio.h"
#include "qmngio.h"
#include "qjpegio.h"
#include "qmap.h"
#include <stdlib.h>
#include <ctype.h>

#ifdef _WS_QWS_
#include "qgfx_qws.h"
#endif

// 16bpp images on supported on Qt/Embedded
#if !defined( _WS_QWS_ ) && !defined(QT_NO_IMAGE_16_BIT)
#define QT_NO_IMAGE_16_BIT
#endif


// BEING REVISED: jo (QImageIO)
/*!
  \class QImage qimage.h
  \brief The QImage class provides a hardware-independent pixmap
  representation with direct access to the pixel data.

  \ingroup images
  \ingroup shared

  It is one of the two classes Qt provides for dealing with images,
  the other being QPixmap.  QImage is designed and optimized for I/O
  and for direct pixel access/manipulation, QPixmap is designed and
  optimized for drawing.  There are (slow) functions to convert
  between QImage and QPixmap; QPixmap::convertToImage() and
  QPixmap::convertFromImage().

  An image has the parameters \link width() width\endlink, \link height()
  height\endlink and \link depth() depth\endlink (bits per pixel, bpp), a
  color table and the actual \link bits() pixels\endlink.  QImage supports
  1-bpp, 8-bpp and 32-bpp image data.  1-bpp and 8-bpp images use a color
  lookup table; the pixel value is a color table index.

  32-bpp images encode an RGB value in 24 bits and ignore the color table.
  The most significant byte is used for the \link setAlphaBuffer alpha
  buffer\endlink.

  An entry in the color table is an RGB triplet encoded as \c uint.  Use
  the qRed, qGreen and qBlue functions (qcolor.h) to access the
  components, and qRgb to make an RGB triplet (see the QColor class
  documentation).

  1-bpp (monochrome) images have a color table with maximum 2 colors.
  There are two different formats; big endian (MSB first) or little endian
  (LSB first) bit order. To access a single bit, you will have to do some
  bitshifts:

  \code
    QImage image;
      // sets bit at (x,y) to 1
    if ( image.bitOrder() == QImage::LittleEndian )
	*(image.scanLine(y) + (x >> 3)) |= 1 << (x & 7);
    else
	*(image.scanLine(y) + (x >> 3)) |= 1 << (7 -(x & 7));
  \endcode

  If this looks complicated, it might be a good idea to convert the 1-bpp
  image to an 8-bpp image using convertDepth().

  8-bpp images are much easier to work with than 1-bpp images because they
  have a single byte per pixel:

  \code
    QImage image;
      // set entry 19 in the color table to yellow
    image.setColor( 19, qRgb(255,255,0) );
      // set 8 bit pixel at (x,y) to value yellow (in color table)
    *(image.scanLine(y) + x) = 19;
  \endcode

  32-bpp images ignore the color table, instead each pixel contains the
  RGB triplet. 24 bits contain the RGB value and the most significant
  byte is reserved for the alpha buffer.

  \code
    QImage image;
      // sets 32 bit pixel at (x,y) to yellow.
    uint *p = (uint *)image.scanLine(y) + x;
    *p = qRgb(255,255,0);
  \endcode

  The scanlines are 32-bit aligned for all depths.

  The QImage class uses explicit \link shclass.html sharing\endlink,
  similar to that of QArray.

  \sa QImageIO, QPixmap, \link shclass.html Shared Classes\endlink
*/


/*! \enum QImage::Endian

  This enum type is used to describe the endianness of the CPU and
  graphics hardware.

  The current values are: <ul>

  <li> \c IgnoreEndian - does not matter.  Useful for some operations
  that are independent of endianness

  <li> \c BigEndian - network byte order, like on SPARC and Motorola CPUs.

  <li> \c LittleEndian - PC/Alpha byte order.

  </ul>

*/
#if defined(_CC_DEC_) && defined(__alpha) && (__DECCXX_VER >= 50190001)
#pragma message disable narrowptr
#endif

#ifndef QT_NO_IMAGE_TEXT
class QImageDataMisc {
public:
    QImageDataMisc() { }
    QImageDataMisc( const QImageDataMisc& o ) :
	text_lang(o.text_lang) { }

    QImageDataMisc& operator=(const QImageDataMisc& o)
    {
	text_lang = o.text_lang;
	return *this;
    }
    QValueList<QImageTextKeyLang> list()
    {
	QValueList<QImageTextKeyLang> r;
	QMap<QImageTextKeyLang,QString>::Iterator it = text_lang.begin();
	for ( ; it != text_lang.end(); ++it )
	    r.append( it.key() );
	return r;
    }

    QStringList languages()
    {
	QStringList r;
	QMap<QImageTextKeyLang,QString>::Iterator it = text_lang.begin();
	for ( ; it != text_lang.end(); ++it ) {
	    r.remove( it.key().lang );
	    r.append( it.key().lang );
	}
	return r;
    }
    QStringList keys()
    {
	QStringList r;
	QMap<QImageTextKeyLang,QString>::Iterator it = text_lang.begin();
	for ( ; it != text_lang.end(); ++it ) {
	    r.remove( it.key().key );
	    r.append( it.key().key );
	}
	return r;
    }

    QMap<QImageTextKeyLang,QString> text_lang;
};
#endif // QT_NO_IMAGE_TEXT



/*****************************************************************************
  QImage member functions
 *****************************************************************************/

// table to flip bits
static const uchar bitflip[256] = {
    /*
	open OUT, "| fmt";
	for $i (0..255) {
	    print OUT (($i >> 7) & 0x01) | (($i >> 5) & 0x02) |
		      (($i >> 3) & 0x04) | (($i >> 1) & 0x08) |
		      (($i << 7) & 0x80) | (($i << 5) & 0x40) |
		      (($i << 3) & 0x20) | (($i << 1) & 0x10), ", ";
	}
	close OUT;
    */
    0, 128, 64, 192, 32, 160, 96, 224, 16, 144, 80, 208, 48, 176, 112, 240,
    8, 136, 72, 200, 40, 168, 104, 232, 24, 152, 88, 216, 56, 184, 120, 248,
    4, 132, 68, 196, 36, 164, 100, 228, 20, 148, 84, 212, 52, 180, 116, 244,
    12, 140, 76, 204, 44, 172, 108, 236, 28, 156, 92, 220, 60, 188, 124, 252,
    2, 130, 66, 194, 34, 162, 98, 226, 18, 146, 82, 210, 50, 178, 114, 242,
    10, 138, 74, 202, 42, 170, 106, 234, 26, 154, 90, 218, 58, 186, 122, 250,
    6, 134, 70, 198, 38, 166, 102, 230, 22, 150, 86, 214, 54, 182, 118, 246,
    14, 142, 78, 206, 46, 174, 110, 238, 30, 158, 94, 222, 62, 190, 126, 254,
    1, 129, 65, 193, 33, 161, 97, 225, 17, 145, 81, 209, 49, 177, 113, 241,
    9, 137, 73, 201, 41, 169, 105, 233, 25, 153, 89, 217, 57, 185, 121, 249,
    5, 133, 69, 197, 37, 165, 101, 229, 21, 149, 85, 213, 53, 181, 117, 245,
    13, 141, 77, 205, 45, 173, 109, 237, 29, 157, 93, 221, 61, 189, 125, 253,
    3, 131, 67, 195, 35, 163, 99, 227, 19, 147, 83, 211, 51, 179, 115, 243,
    11, 139, 75, 203, 43, 171, 107, 235, 27, 155, 91, 219, 59, 187, 123, 251,
    7, 135, 71, 199, 39, 167, 103, 231, 23, 151, 87, 215, 55, 183, 119, 247,
    15, 143, 79, 207, 47, 175, 111, 239, 31, 159, 95, 223, 63, 191, 127, 255
};

const uchar *qt_get_bitflip_array()			// called from QPixmap code
{
    return bitflip;
}


/*!
  Constructs a null image.
  \sa isNull()
*/

QImage::QImage()
{
    init();
}

/*!
  Constructs an image with \a w width, \a h height, \a depth bits per
  pixel, \a numColors colors and bit order \a bitOrder.

  Using this constructor is the same as first constructing a null image and
  then calling the create() function.

  \sa create()
*/

QImage::QImage( int w, int h, int depth, int numColors, Endian bitOrder )
{
    init();
    create( w, h, depth, numColors, bitOrder );
}

/*!  Constructs an image with size \a size pixels, depth \a depth
  bits, \a numColors and \a bitOrder endianness.
*/
QImage::QImage( const QSize& size, int depth, int numColors, Endian bitOrder )
{
    init();
    create( size, depth, numColors, bitOrder );
}


/*!
  Constructs an image from loading \a fileName and an optional
  \a format.

  \sa load()
*/

QImage::QImage( const QString &fileName, const char* format )
{
    init();
    load( fileName, format );
}

#ifndef QT_NO_IMAGEIO_XPM
// helper
static void read_xpm_image_or_array( QImageIO *, const char **, QImage & );
#endif
/*!
  Constructs an image from \a xpm, which must be a valid XPM image.

  Errors are silently ignored.

  Note that it's possible to squeeze the XPM variable a little bit by
  using an unusual declaration:

  \code
    static const char * const start_xpm[]={
        "16 15 8 1",
        "a c #cec6bd",
    ....
  \endcode

  The extra \c const makes the entire definition read-only, which is
  slightly more efficient e.g. when the code is in a shared library,
  and ROMable when the application is to be stored in ROM.

  In order to use that sort of declaration, you must cast the variable
  back to <nobr><code>const char **</code></nobr> when you create the
  QImage.
*/

QImage::QImage( const char *xpm[] )
{
    init();
#ifndef QT_NO_IMAGEIO_XPM
    read_xpm_image_or_array( 0, xpm, *this );
#else
    // We use a qFatal rather than disabling the whole function, as this
    // constructor may be ambiguous.
    qFatal("XPM not supported");
#endif
}

/*!
  Constructs an image from \a array, which must be in a supported
  image format image.

  \sa loadFromData()
*/
QImage::QImage( const QByteArray &array )
{
    init();
    loadFromData(array);
}

/*!
  Constructs a
  \link shclass.html shallow copy\endlink of \e image.
*/

QImage::QImage( const QImage &image )
{
    data = image.data;
    data->ref();
}

/*!
  Constructs an image that uses an existing memory buffer.
  The buffer must remain valid for the life of the QImage.  The image
  will not delete the buffer at destruction.

  If colortable is 0, a color table sufficient for \a numColors will be
  allocated (and destructed later).
*/
QImage::QImage( uchar* yourdata, int w, int h, int depth,
		QRgb* colortable, int numColors,
		Endian bitOrder )
{
    init();
    data->w = w;
    data->h = h;
    data->d = depth;
    data->ncols = numColors;
    switch ( depth ) {
    case 1:
	data->nbytes = (w+7)/8*h;
	break;
    case 8:
	data->nbytes = w*h;
	break;
#ifndef QT_NO_IMAGE_16_BIT
    case 16:
	data->nbytes = w*h*2;
	break;
#endif
#ifndef QT_NO_IMAGE_TRUECOLOR
    case 32:
	data->nbytes = w*h*4;
	break;
#endif
    }
    if ( colortable || !numColors ) {
	data->ctbl = colortable;
	data->ctbl_mine = FALSE;
    } else {
	// calloc since we realloc, etc. later (ick)
        data->ctbl = (QRgb*)calloc( numColors*sizeof(QRgb), numColors );
	data->ctbl_mine = TRUE;
    }
    uchar** jt = (uchar**)malloc(h*sizeof(uchar*));
    int bpl = data->nbytes/h;
    for (int j=0; j<h; j++) {
	jt[j] = yourdata+j*bpl;
    }
    data->bits = jt;
    data->bitordr = bitOrder;
}

#ifdef _WS_QWS_

/*!
  Constructs an image that uses an existing memory buffer.
  The buffer must remain valid for the life of the QImage.  The image
  will not delete the buffer at destruction.

  \a bpl specifies the number of bytes per line.

  If colortable is 0, a color table sufficient for \a numColors will be
  allocated (and destructed later).
*/
QImage::QImage( uchar* yourdata, int w, int h, int depth,
		int bpl, QRgb* colortable, int numColors,
		Endian bitOrder )
{
    init();
    data->w = w;
    data->h = h;
    data->d = depth;
    data->ncols = numColors;
    data->nbytes = bpl * h;
    if ( colortable || !numColors ) {
	data->ctbl = colortable;
	data->ctbl_mine = FALSE;
    } else {
	// calloc since we realloc, etc. later (ick)
        data->ctbl = (QRgb*)calloc( numColors*sizeof(QRgb), numColors );
	data->ctbl_mine = TRUE;
    }
    uchar** jt = (uchar**)malloc(h*sizeof(uchar*));
    for (int j=0; j<h; j++) {
	jt[j] = yourdata+j*bpl;
    }
    data->bits = jt;
    data->bitordr = bitOrder;
}
#endif //_WS_QWS_

/*!
  Destructs the image and cleans up.
*/

QImage::~QImage()
{
    if ( data && data->deref() ) {
	reset();
	delete data;
    }
}


/*!
  Assigns a
  \link shclass.html shallow copy\endlink
  of \e image to this image and returns a reference to this image.

  \sa copy()
*/

QImage &QImage::operator=( const QImage &image )
{
    image.data->ref();				// avoid 'x = x'
    if ( data->deref() ) {
	reset();
	delete data;
    }
    data = image.data;
    return *this;
}

/*!
  Sets the image bits to the \e pixmap contents and returns a reference to
  the image.

  If the image shares data with other images, it will first dereference
  the shared data.

  Makes a call to QPixmap::convertToImage().
*/

QImage &QImage::operator=( const QPixmap &pixmap )
{
    *this = pixmap.convertToImage();
    return *this;
}

/*!
  Detaches from shared image data and makes sure that this image is the
  only one referring the data.

  If multiple images share common data, this image makes a copy of the
  data and detaches itself from the sharing mechanism.	Nothing is
  done if there is just a single reference.
*/

void QImage::detach()
{
    if ( data->count != 1 )
	*this = copy();
}

/*!
  Returns a
  \link shclass.html deep copy\endlink of the image.
*/

QImage QImage::copy() const
{
    QImage image;
    if ( !isNull() ) {
	image.create( width(), height(), depth(), numColors(), bitOrder() );
	memcpy( image.bits(), bits(), numBytes() );
	memcpy( image.colorTable(), colorTable(), numColors()*sizeof(QRgb) );
	image.setAlphaBuffer(hasAlphaBuffer());
	image.data->dpmx = dotsPerMeterX();
	image.data->dpmy = dotsPerMeterY();
	image.data->offset = offset();
#ifndef QT_NO_IMAGE_TEXT
	if ( data->misc ) {
	    image.data->misc = new QImageDataMisc;
	    *image.data->misc = misc();
	}
#endif	
    }
    return image;
}

/*!
  Returns a
  \link shclass.html deep copy\endlink of a sub-area of the image.

  The returned image is always \a w by \a h pixels is size. If the
  area is beyond this image, the pixels are filled with pixel 0.

  \sa bitBlt()
*/

QImage QImage::copy(int x, int y, int w, int h, int conversion_flags) const
{
    if ( w <= 0 || h <= 0 ) return QImage(); // Nothing to copy

    QImage image( w, h, depth(), numColors(), bitOrder() );

    if ( x < 0 || y < 0 || x + w > width() || y + h > height() ) {
	// bitBlt will not cover entire image - clear it.
	// ### should deal with each side separately for efficiency
	image.fill(0);
    }

    memcpy( image.colorTable(), colorTable(), numColors()*sizeof(QRgb) );
    image.setAlphaBuffer(hasAlphaBuffer());
    bitBlt( &image, 0, 0, this, x, y, -1, -1, conversion_flags );
    return image;
}

/*!
  \overload QImage QImage::copy(const QRect& r) const
*/

/*!
  \obsolete
  \fn QImage QImage::copy(QRect& r) const
  Use the const version.
*/

/*!
  \fn bool QImage::isNull() const
  Returns TRUE if it is a null image.

  A null image has all parameters set to zero and no allocated data.
*/


/*!
  \fn int QImage::width() const
  Returns the width of the image.
  \sa height(), size(), rect()
*/

/*!
  \fn int QImage::height() const
  Returns the height of the image.
  \sa width(), size(), rect()
*/

/*!
  \fn QSize QImage::size() const
  Returns the size of the image.
  \sa width(), height(), rect()
*/

/*!
  \fn QRect QImage::rect() const
  Returns the enclosing rectangle (0,0,width(),height()) of the image.
  \sa width(), height(), size()
*/

/*!
  \fn int QImage::depth() const
  Returns the depth of the image.

  The image depth is the number of bits used to encode a single pixel, also
  called bits per pixel (bpp) or bit planes of an image.

  The supported depths are 1, 8, 16 and 32.
*/

/*!
  \fn int QImage::numColors() const
  Returns the size of the color table for the image.

  Notice that numColors() returns 0 for 16-bpp and 32-bpp images,
  since these images do not use color tables, but instead encode pixel
  values as RGB triplets.
*/

/*!
  \fn QImage::Endian QImage::bitOrder() const
  Returns the bit order for the image.

  If it is a 1-bpp image, this function returns either QImage::BigEndian or
  QImage::LittleEndian.

  If it is not a 1-bpp image, this function returns QImage::IgnoreEndian.

  \sa depth()
*/

/*!
  \fn uchar **QImage::jumpTable() const
  Returns a pointer to the scanline pointer table.

  This is the beginning of the data block for the image.
*/

/*!
  \fn QRgb *QImage::colorTable() const
  Returns a pointer to the color table.
*/

/*!
  \fn int QImage::numBytes() const
  Returns the number of bytes occupied by the image data.
  \sa bytesPerLine()
*/

/*!
  \fn int QImage::bytesPerLine() const
  Returns the number of bytes per image scanline.
  This is equivalent to numBytes()/height().
*/

/*!
  \fn QRgb QImage::color( int i ) const

  Returns the color in the color table at index \e i.

  A color value is an RGB triplet. Use the qRed(), qGreen() and qBlue()
  functions (defined in qcolor.h) to get the color value components.

  \sa setColor(), QColor
*/

/*!
  \fn void QImage::setColor( int i, QRgb c )

  Sets a color in the color table at index \e i to \e c.

  A color value is an RGB triplet.  Use the qRgb function (defined in qcolor.h)
  to make RGB triplets.

  \sa color()
*/

/*!
  \fn uchar *QImage::scanLine( int i ) const

  Returns a pointer to the pixel data at the \e i'th scanline.

  The scanline data is aligned on a 32-bit boundary.

  \warning If you are accessing 32-bpp image data, cast the returned
  pointer to \c QRgb* (QRgb has a 32 bit size) and use it to read/write
  the pixel value. You cannot use the \c uchar* pointer directly, because
  the pixel format depends on the byte order on the underlying
  platform. Hint: use \link ::qRed() qRed()\endlink and friends (qcolor.h)
  to access the pixels.

  \warning If you are accessing 16-bpp image data, you have to handle
  endianness yourself for now.

  \sa bits()
*/

/*!
  \fn uchar *QImage::bits() const
  Returns a pointer to the first pixel data. Equivalent to scanLine(0).
  \sa scanLine()
*/



void QImage::warningIndexRange( const char *func, int i )
{
#if defined(CHECK_RANGE)
    qWarning( "QImage::%s: Index %d out of range", func, i );
#else
    Q_UNUSED( func )
    Q_UNUSED( i )
#endif
}


/*!
  Resets all image parameters and deallocates the image data.
*/

void QImage::reset()
{
    freeBits();
    setNumColors( 0 );
#ifndef QT_NO_IMAGE_TEXT
    delete data->misc;
#endif    
    reinit();
}


/*!
  Fills the entire image with the pixel value \a pixel.

  If the \link depth() depth\endlink of this image is 1, only
  the lowest bit is used. If you say fill(0), fill(2) etc., the image
  is filled with 0s. If you say fill(1), fill(3) etc., the image
  is filled with 1s. If the depth is 8, the lowest 8 bits are used.

  If the depth is 32 and the image has no alpha buffer, the \a pixel
  value is written to each pixel in the image. If the image has an
  alpha buffer, only the 24 RGB bits are set and the upper 8 bits (alpha
  value) are left unchanged.
*/

void QImage::fill( uint pixel )
{
    if ( depth() == 1 || depth() == 8 ) {
	if ( depth() == 1 ) {
	    if ( pixel & 1 )
		pixel = 0xffffffff;
	    else
		pixel = 0;
	} else {
	    uint c = pixel & 0xff;
	    pixel = c | ((c << 8) & 0xff00) | ((c << 16) & 0xff0000) |
		    ((c << 24) & 0xff000000);
	}
	int bpl = bytesPerLine();
	for ( int i=0; i<height(); i++ )
	    memset( scanLine(i), pixel, bpl );
#ifndef QT_NO_IMAGE_16_BIT
    } else if ( depth() == 16 ) {
	for ( int i=0; i<height(); i++ ) {
	    //optimize with 32-bit writes, since image is always aligned
	    uint *p = (uint *)scanLine(i);
	    uint *end = (uint*)(((ushort*)p) + width());
	    uint fill;
	    ushort *f = (ushort*)&fill;
	    f[0]=pixel;
	    f[1]=pixel;
	    while ( p < end )
		*p++ = fill;
	}
#endif	// QT_NO_IMAGE_16_BIT
#ifndef QT_NO_IMAGE_TRUECOLOR	
    } else if ( depth() == 32 ) {
	if ( hasAlphaBuffer() ) {
	    pixel &= 0x00ffffff;
	    for ( int i=0; i<height(); i++ ) {
		uint *p = (uint *)scanLine(i);
		uint *end = p + width();
		while ( p < end ) {
		    *p = (*p & 0xff000000) | pixel;
		    p++;
		}
	    }
	} else {
	    for ( int i=0; i<height(); i++ ) {
		uint *p = (uint *)scanLine(i);
		uint *end = p + width();
		while ( p < end )
		    *p++ = pixel;
	    }
	}
#endif // QT_NO_IMAGE_TRUECOLOR	
    }
}


/*!
  Inverts all pixel values in the image.

  For 32 bit (24 RGB + alpha buffer) images, set \a invertAlpha to
  FALSE if you want the alpha bits to be unchanged, otherwise they are
  inverted too.
*/

void QImage::invertPixels( bool invertAlpha )
{
    Q_UINT32 n = numBytes();
    if ( n % 4 ) {
	Q_UINT8 *p = (Q_UINT8*)bits();
	Q_UINT8 *end = p + n;
	while ( p < end )
	    *p++ ^= 0xff;
    } else {
	Q_UINT32 *p = (Q_UINT32*)bits();
	Q_UINT32 *end = p + n/4;
	uint xorbits = invertAlpha && depth() == 32 ? 0x00ffffff : 0xffffffff;
	while ( p < end )
	    *p++ ^= xorbits;
    }
}


/*!
  Determines the host computer byte order.
  Returns QImage::LittleEndian (LSB first) or QImage::BigEndian (MSB first).
*/

QImage::Endian QImage::systemByteOrder()
{
    static Endian sbo = IgnoreEndian;
    if ( sbo == IgnoreEndian ) {		// initialize
	int  ws;
	bool be;
	qSysInfo( &ws, &be );
	sbo = be ? BigEndian : LittleEndian;
    }
    return sbo;
}


#if defined(_WS_X11_)
#define	 GC GC_QQQ
#include <X11/Xlib.h>				// needed for systemBitOrder
#include <X11/Xutil.h>
#include <X11/Xos.h>
#if defined(_OS_WIN32_)
#undef open					// kill utterly stupid #defines
#undef close
#undef read
#undef write
#endif
#endif

/*!
  Determines the bit order of the display hardware.
  Returns QImage::LittleEndian (LSB first) or QImage::BigEndian (MSB first).
*/

QImage::Endian QImage::systemBitOrder()
{
#if defined(_WS_X11_)
    return BitmapBitOrder(qt_xdisplay()) == MSBFirst ? BigEndian :LittleEndian;
#else
    return BigEndian;
#endif
}


/*!
  Resizes the color table to \e numColors colors.

  If the color table is expanded, then all new colors will be set to black
  (RGB 0,0,0).

  \sa color(), setColor()
*/

void QImage::setNumColors( int numColors )
{
    if ( numColors == data->ncols )
	return;
    if ( numColors == 0 ) {			// use no color table
	if ( data->ctbl ) {
	    if ( data->ctbl_mine )
		free( data->ctbl );
	    else
		data->ctbl_mine = TRUE;
	    data->ctbl = 0;
	}
	data->ncols = 0;
	return;
    }
    if ( data->ctbl && data->ctbl_mine ) {	// already has color table
	data->ctbl = (QRgb*)realloc( data->ctbl, numColors*sizeof(QRgb) );
	if ( data->ctbl && numColors > data->ncols )
	    memset( (char *)&data->ctbl[data->ncols], 0,
		    (numColors-data->ncols)*sizeof(QRgb) );
    } else {					// create new color table
	data->ctbl = (QRgb*)calloc( numColors*sizeof(QRgb), 1 );
	data->ctbl_mine = TRUE;
    }
    data->ncols = data->ctbl == 0 ? 0 : numColors;
}


/*!
  \fn bool QImage::hasAlphaBuffer() const

  Returns TRUE if alpha buffer mode is enabled, otherwise FALSE.

  \sa setAlphaBuffer()
*/

/*!
  Enables alpha buffer mode if \a enable is TRUE, otherwise disables it.
  The default setting is disabled.

  An 8-bpp image has 8 bit pixels. A pixel is an index into the \link
  color() color table\endlink, which contains 32-bit color values.
  In a 32-bpp image, the 32 bit pixels are the color values.

  This 32 bit value is encoded as follows: The lower 24 bits are used for
  the red, green and blue components. The upper 8 bits contain the alpha
  component.

  The alpha component specifies the transparency of a pixel. 0 means
  completely transparent and 255 means opaque. The alpha component is
  ignored if you do not enable alpha buffer mode.

  The alpha buffer is used to set a mask when a QImage is translated to a
  QPixmap.

  \sa hasAlphaBuffer(), createAlphaMask()
*/

void QImage::setAlphaBuffer( bool enable )
{
    data->alpha = enable;
}


/*!
  Sets the image width, height, depth, number of colors and bit order.
  Returns TRUE if successful, or FALSE if the parameters are incorrect or
  if memory cannot be allocated.

  The \e width and \e height is limited to 32767. \e depth must be 1, 8 or
  32. If \e depth is 1, then \e bitOrder must be set to either
  QImage::LittleEndian or QImage::BigEndian.  For other depths, \e
  bitOrder must be QImage::IgnoreEndian.

  This function allocates a color table and a buffer for the image data.
  The image data is not initialized.

  The image buffer is allocated as a single block that consists of a table
  of \link scanLine() scanline\endlink pointers (jumpTable()) and the
  image data (bits()).

  \sa width(), height(), depth(), numColors(), bitOrder(), jumpTable(),
  scanLine(), bits(), bytesPerLine(), numBytes()
*/

bool QImage::create( int width, int height, int depth, int numColors,
		    Endian bitOrder )
{
    reset();					// reset old data
    if ( width <= 0 || height <= 0 || depth <= 0 || numColors < 0 )
	return FALSE;				// invalid parameter(s)
    if ( depth == 1 && bitOrder == IgnoreEndian ) {
#if defined(CHECK_RANGE)
	qWarning( "QImage::create: Bit order is required for 1 bpp images" );
#endif
	return FALSE;
    }
    if ( depth != 1 )
	bitOrder = IgnoreEndian;

#if defined(CHECK_RANGE)
    if ( depth == 24 )
	qWarning( "QImage::create: 24-bpp images no longer supported, "
		  "use 32-bpp instead" );
#endif
    switch ( depth ) {
    case 1:
    case 8:
#ifndef QT_NO_IMAGE_16_BIT
    case 16:
#endif
#ifndef QT_NO_IMAGE_TRUECOLOR
    case 32:
#endif
	break;
    default:				// invalid depth
	return FALSE;
    }

    setNumColors( numColors );
    if ( data->ncols != numColors )		// could not alloc color table
	return FALSE;

// Qt/Embedded doesn't want to waste memory on un-necessary padding.
#ifdef _WS_QWS_
    const int bpl = (width*depth+7)/8;		// bytes per scanline
    const int pad = 0;
#else
    const int bpl = ((width*depth+31)/32)*4;	// bytes per scanline
    // #### WWA: shouldn't this be (width*depth+7)/8:
    const int pad = bpl - (width*depth)/8;	// pad with zeros
#endif
    int nbytes = bpl*height;			// image size
    int ptbl   = height*sizeof(uchar*);		// pointer table size
    int size   = nbytes + ptbl;			// total size of data block
    uchar **p  = (uchar **)malloc( size );	// alloc image bits
    if ( !p ) {					// no memory
	setNumColors( 0 );
	return FALSE;
    }
    data->w = width;
    data->h = height;
    data->d = depth;
    data->nbytes  = nbytes;
    data->bitordr = bitOrder;
    data->bits = p;				// set image pointer
    //uchar *d = (uchar*)p + ptbl;		// setup scanline pointers
    uchar *d = (uchar*)(p + height);		// setup scanline pointers
    while ( height-- ) {
	*p++ = d;
	if ( pad )
	    memset( d+bpl-pad, 0, pad );
	d += bpl;
    }
    return TRUE;
}

/*!
 \overload bool QImage::create( const QSize&, int depth, int numColors, Endian bitOrder )
*/
bool QImage::create( const QSize& size, int depth, int numColors,
		     QImage::Endian bitOrder )
{
    return create(size.width(), size.height(), depth, numColors, bitOrder);
}

/*!
  \internal
  Initializes the image data structure.
*/

void QImage::init()
{
    data = new QImageData;
    CHECK_PTR( data );
    reinit();
}

void QImage::reinit()
{
    data->w = data->h = data->d = data->ncols = 0;
    data->nbytes = 0;
    data->ctbl = 0;
    data->bits = 0;
    data->bitordr = QImage::IgnoreEndian;
    data->alpha = FALSE;
#ifndef QT_NO_IMAGE_TEXT
    data->misc = 0;
#endif    
    data->dpmx = 0;
    data->dpmy = 0;
    data->offset = QPoint(0,0);
}

/*!
  \internal
  Deallocates the image data and sets the bits pointer to 0.
*/

void QImage::freeBits()
{
    if ( data->bits ) {				// dealloc image bits
	free( data->bits );
	data->bits = 0;
	data->bits_mine = TRUE;
    }
}


/*****************************************************************************
  Internal routines for converting image depth.
 *****************************************************************************/

//
// convert_32_to_8:  Converts a 32 bits depth (true color) to an 8 bit
// image with a colormap.  If the 32 bit image has more than 256 colors,
// we convert the red,green and blue bytes into a single byte encoded
// as 6 shades of each of red, green and blue.
//
// if dithering is needed, only 1 color at most is available for alpha.
//
#ifndef QT_NO_IMAGE_TRUECOLOR
struct QRgbMap {
    QRgbMap() : rgb(0xffffffff) { }
    bool used() const { return rgb!=0xffffffff; }
    uchar  pix;
    QRgb  rgb;
};

static bool convert_32_to_8( const QImage *src, QImage *dst, int conversion_flags, QRgb* palette=0, int palette_count=0 )
{
    register QRgb *p;
    uchar  *b;
    bool    do_quant = FALSE;
    int	    y, x;

    if ( !dst->create(src->width(), src->height(), 8, 256) )
	return FALSE;

    const int tablesize = 997; // prime
    QRgbMap table[tablesize];
    int   pix=0;
    QRgb amask = src->hasAlphaBuffer() ? 0xffffffff : 0x00ffffff;
    if ( src->hasAlphaBuffer() )
	dst->setAlphaBuffer(TRUE);

    if ( palette ) {
	// Preload palette into table.

	p = palette;
	// Almost same code as pixel insertion below
	while ( palette_count-- > 0 ) {
	    // Find in table...
	    int hash = (*p & amask) % tablesize;
	    for (;;) {
		if ( table[hash].used() ) {
		    if ( table[hash].rgb == (*p & amask) ) {
			// Found previous insertion - use it
			break;
		    } else {
			// Keep searching...
			if (++hash == tablesize) hash = 0;
		    }
		} else {
		    // Cannot be in table
		    ASSERT ( pix != 256 );	// too many colors
		    // Insert into table at this unused position
		    dst->setColor( pix, (*p & amask) );
		    table[hash].pix = pix++;
		    table[hash].rgb = *p & amask;
		    break;
		}
	    }
	    p++;
	}
    }

    if ( (conversion_flags & Qt::DitherMode_Mask) == Qt::PreferDither ) {
	do_quant = TRUE;
    } else {
	for ( y=0; y<src->height(); y++ ) {	// check if <= 256 colors
	    p = (QRgb *)src->scanLine(y);
	    b = dst->scanLine(y);
	    x = src->width();
	    while ( x-- ) {
		// Find in table...
		int hash = (*p & amask) % tablesize;
		for (;;) {
		    if ( table[hash].used() ) {
			if ( table[hash].rgb == (*p & amask) ) {
			    // Found previous insertion - use it
			    break;
			} else {
			    // Keep searching...
			    if (++hash == tablesize) hash = 0;
			}
		    } else {
			// Cannot be in table
			if ( pix == 256 ) {	// too many colors
			    do_quant = TRUE;
			    // Break right out
			    x = 0;
			    y = src->height();
			} else {
			    // Insert into table at this unused position
			    dst->setColor( pix, (*p & amask) );
			    table[hash].pix = pix++;
			    table[hash].rgb = (*p & amask);
			}
			break;
		    }
		}
		*b++ = table[hash].pix;		// May occur once incorrectly
		p++;
	    }
	}
    }
    int ncols = do_quant ? 256 : pix;

    static uint bm[16][16];
    static int init=0;
    if (!init) {
	// Build a Bayer Matrix for dithering

	init = 1;
	int n, i, j;

	bm[0][0]=0;

	for (n=1; n<16; n*=2) {
	    for (i=0; i<n; i++) {
		for (j=0; j<n; j++) {
		    bm[i][j]*=4;
		    bm[i+n][j]=bm[i][j]+2;
		    bm[i][j+n]=bm[i][j]+3;
		    bm[i+n][j+n]=bm[i][j]+1;
		}
	    }
	}

	for (i=0; i<16; i++)
	    for (j=0; j<16; j++)
		bm[i][j]<<=8;
    }

    dst->setNumColors( ncols );

    if ( do_quant ) {				// quantization needed

#define MAX_R 5
#define MAX_G 5
#define MAX_B 5
#define INDEXOF(r,g,b) (((r)*(MAX_G+1)+(g))*(MAX_B+1)+(b))

	int rc, gc, bc;

	for ( rc=0; rc<=MAX_R; rc++ )		// build 6x6x6 color cube
	    for ( gc=0; gc<=MAX_G; gc++ )
		for ( bc=0; bc<=MAX_B; bc++ ) {
		    dst->setColor( INDEXOF(rc,gc,bc),
			(amask&0xff000000)
			| qRgb( rc*255/MAX_R, gc*255/MAX_G, bc*255/MAX_B ) );
		}

	int sw = src->width();

	int* line1[3];
	int* line2[3];
	int* pv[3];
	if ( ( conversion_flags & Qt::Dither_Mask ) == Qt::DiffuseDither ) {
	    line1[0] = new int[src->width()];
	    line2[0] = new int[src->width()];
	    line1[1] = new int[src->width()];
	    line2[1] = new int[src->width()];
	    line1[2] = new int[src->width()];
	    line2[2] = new int[src->width()];
	    pv[0] = new int[sw];
	    pv[1] = new int[sw];
	    pv[2] = new int[sw];
	}

	for ( y=0; y < src->height(); y++ ) {
	    p = (QRgb *)src->scanLine(y);
	    b = dst->scanLine(y);
	    QRgb *end = p + sw;

	    // perform quantization
	    if ( ( conversion_flags & Qt::Dither_Mask ) == Qt::ThresholdDither ) {
#define DITHER(p,m) ((uchar) ((p * (m) + 127) / 255))
		while ( p < end ) {
		    rc = qRed( *p );
		    gc = qGreen( *p );
		    bc = qBlue( *p );

		    *b++ =
			INDEXOF(
			    DITHER(rc, MAX_R),
			    DITHER(gc, MAX_G),
			    DITHER(bc, MAX_B)
			);

		    p++;
		}
#undef DITHER
	    } else if ( ( conversion_flags & Qt::Dither_Mask ) == Qt::OrderedDither ) {
#define DITHER(p,d,m) ((uchar) ((((256 * (m) + (m) + 1)) * (p) + (d)) / 65536 ))

		int x = 0;
		while ( p < end ) {
		    uint d = bm[y&15][x&15];

		    rc = qRed( *p );
		    gc = qGreen( *p );
		    bc = qBlue( *p );

		    *b++ =
			INDEXOF(
			    DITHER(rc, d, MAX_R),
			    DITHER(gc, d, MAX_G),
			    DITHER(bc, d, MAX_B)
			);

		    p++;
		    x++;
		}
#undef DITHER
	    } else { // Diffuse
		int endian = (QImage::systemByteOrder() == QImage::BigEndian);
		int x;
		uchar* q = src->scanLine(y);
		uchar* q2 = src->scanLine(y+1 < src->height() ? y + 1 : 0);
		for (int chan = 0; chan < 3; chan++) {
		    b = dst->scanLine(y);
		    int *l1 = (y&1) ? line2[chan] : line1[chan];
		    int *l2 = (y&1) ? line1[chan] : line2[chan];
		    if ( y == 0 ) {
			for (int i=0; i<sw; i++)
			    l1[i] = q[i*4+chan+endian];
		    }
		    if ( y+1 < src->height() ) {
			for (int i=0; i<sw; i++)
			    l2[i] = q2[i*4+chan+endian];
		    }
		    // Bi-directional error diffusion
		    if ( y&1 ) {
			for (x=0; x<sw; x++) {
			    int pix = QMAX(QMIN(5, (l1[x] * 5 + 128)/ 255), 0);
			    int err = l1[x] - pix * 255 / 5;
			    pv[chan][x] = pix;

			    // Spread the error around...
			    if ( x+1<sw ) {
				l1[x+1] += (err*7)>>4;
				l2[x+1] += err>>4;
			    }
			    l2[x]+=(err*5)>>4;
			    if (x>1)
				l2[x-1]+=(err*3)>>4;
			}
		    } else {
			for (x=sw; x-->0; ) {
			    int pix = QMAX(QMIN(5, (l1[x] * 5 + 128)/ 255), 0);
			    int err = l1[x] - pix * 255 / 5;
			    pv[chan][x] = pix;

			    // Spread the error around...
			    if ( x > 0 ) {
				l1[x-1] += (err*7)>>4;
				l2[x-1] += err>>4;
			    }
			    l2[x]+=(err*5)>>4;
			    if (x+1 < sw)
				l2[x+1]+=(err*3)>>4;
			}
		    }
		}
		if (endian) {
		    for (x=0; x<sw; x++) {
			*b++ = INDEXOF(pv[0][x],pv[1][x],pv[2][x]);
		    }
		} else {
		    for (x=0; x<sw; x++) {
			*b++ = INDEXOF(pv[2][x],pv[1][x],pv[0][x]);
		    }
		}
	    }
	}

	if ( src->hasAlphaBuffer() ) {
	    const int trans = 216;
	    dst->setColor(trans, 0x00000000);	// transparent
	    QImage mask = src->createAlphaMask(conversion_flags);
	    uchar* m;
	    for ( y=0; y < src->height(); y++ ) {
		uchar bit = 0x80;
		m = mask.scanLine(y);
		b = dst->scanLine(y);
		int w = src->width();
		for ( x = 0; x<w; x++ ) {
		    if ( !(*m&bit) )
			b[x] = trans;
		    if (!(bit >>= 1)) {
			bit = 0x80;
			while (*++m == 0xff && x<w) // skip chunks
			    x+=8;
		    }
		}
	    }
	}

	if ( ( conversion_flags & Qt::Dither_Mask ) == Qt::DiffuseDither ) {
	    delete [] line1[0];
	    delete [] line2[0];
	    delete [] line1[1];
	    delete [] line2[1];
	    delete [] line1[2];
	    delete [] line2[2];
	    delete [] pv[0];
	    delete [] pv[1];
	    delete [] pv[2];
	}

#undef MAX_R
#undef MAX_G
#undef MAX_B
#undef INDEXOF

    }

    return TRUE;
}


static bool convert_8_to_32( const QImage *src, QImage *dst )
{
    if ( !dst->create(src->width(), src->height(), 32) )
	return FALSE;				// create failed
    dst->setAlphaBuffer( src->hasAlphaBuffer() );
    for ( int y=0; y<dst->height(); y++ ) {	// for each scan line...
	register uint *p = (uint *)dst->scanLine(y);
	uchar  *b = src->scanLine(y);
	uint *end = p + dst->width();
	while ( p < end )
	    *p++ = src->color(*b++);
    }
    return TRUE;
}


static bool convert_1_to_32( const QImage *src, QImage *dst )
{
    if ( !dst->create(src->width(), src->height(), 32) )
	return FALSE;				// could not create
    dst->setAlphaBuffer( src->hasAlphaBuffer() );
    for ( int y=0; y<dst->height(); y++ ) {	// for each scan line...
	register uint *p = (uint *)dst->scanLine(y);
	uchar *b = src->scanLine(y);
	int x;
	if ( src->bitOrder() == QImage::BigEndian ) {
	    for ( x=0; x<dst->width(); x++ ) {
		*p++ = src->color( (*b >> (7 - (x & 7))) & 1 );
		if ( (x & 7) == 7 )
		    b++;
	    }
	} else {
	    for ( x=0; x<dst->width(); x++ ) {
		*p++ = src->color( (*b >> (x & 7)) & 1 );
		if ( (x & 7) == 7 )
		    b++;
	    }
	}
    }
    return TRUE;
}
#endif // QT_NO_IMAGE_TRUECOLOR

static bool convert_1_to_8( const QImage *src, QImage *dst )
{
    if ( !dst->create(src->width(), src->height(), 8, 2) )
	return FALSE;				// something failed
    dst->setAlphaBuffer( src->hasAlphaBuffer() );
    if (src->numColors() >= 2) {
	dst->setColor( 0, src->color(0) );	// copy color table
	dst->setColor( 1, src->color(1) );
    } else {
	// Unlikely, but they do exist
	if (src->numColors() >= 1)
	    dst->setColor( 0, src->color(0) );
	else
	    dst->setColor( 0, 0xffffffff );
	dst->setColor( 1, 0xff000000 );
    }
    for ( int y=0; y<dst->height(); y++ ) {	// for each scan line...
	register uchar *p = dst->scanLine(y);
	uchar *b = src->scanLine(y);
	int x;
	if ( src->bitOrder() == QImage::BigEndian ) {
	    for ( x=0; x<dst->width(); x++ ) {
		*p++ = (*b >> (7 - (x & 7))) & 1;
		if ( (x & 7) == 7 )
		    b++;
	    }
	} else {
	    for ( x=0; x<dst->width(); x++ ) {
		*p++ = (*b >> (x & 7)) & 1;
		if ( (x & 7) == 7 )
		    b++;
	    }
	}
    }
    return TRUE;
}


//
// dither_to_1:  Uses selected dithering algorithm.
//

static bool dither_to_1( const QImage *src, QImage *dst,
			 int conversion_flags, bool fromalpha )
{
    if ( !dst->create(src->width(), src->height(), 1, 2, QImage::BigEndian) )
	return FALSE;				// something failed

    enum { Threshold, Ordered, Diffuse } dithermode;

    if ( fromalpha ) {
	if ( ( conversion_flags & Qt::AlphaDither_Mask ) == Qt::DiffuseAlphaDither )
	    dithermode = Diffuse;
	else if ( ( conversion_flags & Qt::AlphaDither_Mask ) == Qt::OrderedAlphaDither )
	    dithermode = Ordered;
	else
	    dithermode = Threshold;
    } else {
	if ( ( conversion_flags & Qt::Dither_Mask ) == Qt::ThresholdDither )
	    dithermode = Threshold;
	else if ( ( conversion_flags & Qt::Dither_Mask ) == Qt::OrderedDither )
	    dithermode = Ordered;
	else
	    dithermode = Diffuse;
    }

    dst->setColor( 0, qRgb(255, 255, 255) );
    dst->setColor( 1, qRgb(  0,	  0,   0) );
    int	  w = src->width();
    int	  h = src->height();
    int	  d = src->depth();
    uchar gray[256];				// gray map for 8 bit images
    bool  use_gray = d == 8;
    if ( use_gray ) {				// make gray map
	if ( fromalpha ) {
	    // Alpha 0x00 -> 0 pixels (white)
	    // Alpha 0xFF -> 1 pixels (black)
	    for ( int i=0; i<src->numColors(); i++ )
		gray[i] = (255 - (src->color(i) >> 24));
	} else {
	    // Pixel 0x00 -> 1 pixels (black)
	    // Pixel 0xFF -> 0 pixels (white)
	    for ( int i=0; i<src->numColors(); i++ )
		gray[i] = qGray( src->color(i) & 0x00ffffff );
	}
    }

    switch ( dithermode ) {
      case Diffuse: {
	int *line1 = new int[w];
	int *line2 = new int[w];
	int bmwidth = (w+7)/8;
	if ( !(line1 && line2) )
	    return FALSE;
	register uchar *p;
	uchar *end;
	int *b1, *b2;
	int wbytes = w * (d/8);
	p = src->bits();
	end = p + wbytes;
	b2 = line2;
	if ( use_gray ) {			// 8 bit image
	    while ( p < end )
		*b2++ = gray[*p++];
#ifndef QT_NO_IMAGE_TRUECOLOR
	} else {				// 32 bit image
	    if ( fromalpha ) {
		while ( p < end ) {
		    *b2++ = 255 - (*(uint*)p >> 24);
		    p += 4;
		}
	    } else {
		while ( p < end ) {
		    *b2++ = qGray(*(uint*)p);
		    p += 4;
		}
	    }
#endif
	}
	int x, y;
	for ( y=0; y<h; y++ ) {			// for each scan line...
	    int *tmp = line1; line1 = line2; line2 = tmp;
	    bool not_last_line = y < h - 1;
	    if ( not_last_line ) {		// calc. grayvals for next line
		p = src->scanLine(y+1);
		end = p + wbytes;
		b2 = line2;
		if ( use_gray ) {		// 8 bit image
		    while ( p < end )
			*b2++ = gray[*p++];
#ifndef QT_NO_IMAGE_TRUECOLOR
		} else {			// 24 bit image
		    if ( fromalpha ) {
			while ( p < end ) {
			    *b2++ = 255 - (*(uint*)p >> 24);
			    p += 4;
			}
		    } else {
			while ( p < end ) {
			    *b2++ = qGray(*(uint*)p);
			    p += 4;
			}
		    }
#endif		    
		}
	    }

	    int err;
	    p = dst->scanLine( y );
	    memset( p, 0, bmwidth );
	    b1 = line1;
	    b2 = line2;
	    int bit = 7;
	    for ( x=1; x<=w; x++ ) {
		if ( *b1 < 128 ) {		// black pixel
		    err = *b1++;
		    *p |= 1 << bit;
		} else {			// white pixel
		    err = *b1++ - 255;
		}
		if ( bit == 0 ) {
		    p++;
		    bit = 7;
		} else {
		    bit--;
		}
		if ( x < w )
		    *b1 += (err*7)>>4;		// spread error to right pixel
		if ( not_last_line ) {
		    b2[0] += (err*5)>>4;	// pixel below
		    if ( x > 1 )
			b2[-1] += (err*3)>>4;	// pixel below left
		    if ( x < w )
			b2[1] += err>>4;	// pixel below right
		}
		b2++;
	    }
	}
	delete [] line1;
	delete [] line2;
      } break;
      case Ordered: {
	static uint bm[16][16];
	static int init=0;
	if (!init) {
	    // Build a Bayer Matrix for dithering

	    init = 1;
	    int n, i, j;

	    bm[0][0]=0;

	    for (n=1; n<16; n*=2) {
		for (i=0; i<n; i++) {
		    for (j=0; j<n; j++) {
			bm[i][j]*=4;
			bm[i+n][j]=bm[i][j]+2;
			bm[i][j+n]=bm[i][j]+3;
			bm[i+n][j+n]=bm[i][j]+1;
		    }
		}
	    }

	    // Force black to black
	    bm[0][0]=1;
	}

	dst->fill( 0 );
	uchar** mline = dst->jumpTable();
#ifndef QT_NO_IMAGE_TRUECOLOR
	if ( d == 32 ) {
	    uint** line = (uint**)src->jumpTable();
	    for ( int i=0; i<h; i++ ) {
		uint  *p = line[i];
		uint  *end = p + w;
		uchar *m = mline[i];
		int bit = 7;
		int j = 0;
		if ( fromalpha ) {
		    while ( p < end ) {
			if ( (*p++ >> 24) >= bm[j++&15][i&15] )
			    *m |= 1 << bit;
			if ( bit == 0 ) {
			    m++;
			    bit = 7;
			} else {
			    bit--;
			}
		    }
		} else {
		    while ( p < end ) {
			if ( (uint)qGray(*p++) < bm[j++&15][i&15] )
			    *m |= 1 << bit;
			if ( bit == 0 ) {
			    m++;
			    bit = 7;
			} else {
			    bit--;
			}
		    }
		}
	    }
	} else
#endif // QT_NO_IMAGE_TRUECOLOR
	    /* ( d == 8 ) */ {
	    uchar** line = src->jumpTable();
	    for ( int i=0; i<h; i++ ) {
		uchar *p = line[i];
		uchar *end = p + w;
		uchar *m = mline[i];
		int bit = 7;
		int j = 0;
		while ( p < end ) {
		    if ( (uint)gray[*p++] < bm[j++&15][i&15] )
			*m |= 1 << bit;
		    if ( bit == 0 ) {
			m++;
			bit = 7;
		    } else {
			bit--;
		    }
		}
	    }
	}
      } break;
      default: { // Threshold:
	dst->fill( 0 );
	uchar** mline = dst->jumpTable();
#ifndef QT_NO_IMAGE_TRUECOLOR
	if ( d == 32 ) {
	    uint** line = (uint**)src->jumpTable();
	    for ( int i=0; i<h; i++ ) {
		uint  *p = line[i];
		uint  *end = p + w;
		uchar *m = mline[i];
		int bit = 7;
		if ( fromalpha ) {
		    while ( p < end ) {
			if ( (*p++ >> 24) >= 128 )
			    *m |= 1 << bit;	// Set mask "on"
			if ( bit == 0 ) {
			    m++;
			    bit = 7;
			} else {
			    bit--;
			}
		    }
		} else {
		    while ( p < end ) {
			if ( qGray(*p++) < 128 )
			    *m |= 1 << bit;	// Set pixel "black"
			if ( bit == 0 ) {
			    m++;
			    bit = 7;
			} else {
			    bit--;
			}
		    }
		}
	    }
	} else 
#endif //QT_NO_IMAGE_TRUECOLOR
	    if ( d == 8 ) {
	    uchar** line = src->jumpTable();
	    for ( int i=0; i<h; i++ ) {
		uchar *p = line[i];
		uchar *end = p + w;
		uchar *m = mline[i];
		int bit = 7;
		while ( p < end ) {
		    if ( gray[*p++] < 128 )
			*m |= 1 << bit;		// Set mask "on"/ pixel "black"
		    if ( bit == 0 ) {
			m++;
			bit = 7;
		    } else {
			bit--;
		    }
		}
	    }
	}
      }
    }
    return TRUE;
}

#ifndef QT_NO_IMAGE_16_BIT
//###### Endianness issues!
static inline bool is16BitGray( ushort c )
{
    int r=(c & 0xf800) >> 11;
    int g=(c & 0x07e0) >> 6; //green/2
    int b=(c & 0x001f);
    return r == g && g == b;
}


static bool convert_16_to_32( const QImage *src, QImage *dst )
{
    if ( !dst->create(src->width(), src->height(), 32) )
	return FALSE;				// create failed
    dst->setAlphaBuffer( src->hasAlphaBuffer() );
    for ( int y=0; y<dst->height(); y++ ) {	// for each scan line...
	register uint *p = (uint *)dst->scanLine(y);
	ushort  *s = (ushort*)src->scanLine(y);
	uint *end = p + dst->width();
	while ( p < end )
	    *p++ = qt_conv16ToRgb( *s++ );
    }
    return TRUE;
}


static bool convert_32_to_16( const QImage *src, QImage *dst )
{
    if ( !dst->create(src->width(), src->height(), 16) )
	return FALSE;				// create failed
    dst->setAlphaBuffer( src->hasAlphaBuffer() );
    for ( int y=0; y<dst->height(); y++ ) {	// for each scan line...
	register ushort *p = (ushort *)dst->scanLine(y);
	uint  *s = (uint*)src->scanLine(y);
	ushort *end = p + dst->width();
	while ( p < end )
	    *p++ = qt_convRgbTo16( *s++ );
    }
    return TRUE;
}


#endif

/*!
  Converts the depth (bpp) of the image to \e depth and returns the
  converted image.  The original image is left undisturbed.

  The \e depth argument must be 1, 8, 16 or 32.

  See QPixmap::convertFromImage for a description of the \a
  conversion_flags argument.

  Returns \c *this if \e depth is equal to the image depth, or a null
  image if this image cannot be converted.

  \sa depth(), isNull()
*/

QImage QImage::convertDepth( int depth, int conversion_flags ) const
{
    QImage image;
    if ( (data->d == 8 || data->d == 32) && depth == 1 ) // dither
	dither_to_1( this, &image, conversion_flags, FALSE );
#ifndef QT_NO_IMAGE_TRUECOLOR
    else if ( data->d == 32 && depth == 8 )	// 32 -> 8
	convert_32_to_8( this, &image, conversion_flags );
    else if ( data->d == 8 && depth == 32 )	// 8 -> 32
	convert_8_to_32( this, &image );
#endif
    else if ( data->d == 1 && depth == 8 )	// 1 -> 8
	convert_1_to_8( this, &image );
#ifndef QT_NO_IMAGE_TRUECOLOR
    else if ( data->d == 1 && depth == 32 )	// 1 -> 32
	convert_1_to_32( this, &image );
#endif
#ifndef QT_NO_IMAGE_16_BIT
    else if ( data->d == 16 && depth != 16 ) {
	QImage tmp;
	convert_16_to_32( this, &tmp );
	image = tmp.convertDepth( depth, conversion_flags );
    } else if ( data->d != 16 && depth == 16 ) {
	QImage tmp = convertDepth( 32, conversion_flags );
	convert_32_to_16( &tmp, &image );
    }
#endif
    else if ( data->d == depth )
	image = *this;				// no conversion
    else {
#if defined(CHECK_RANGE)
	if ( isNull() )
	    qWarning( "QImage::convertDepth: Image is a null image" );
	else
	    qWarning( "QImage::convertDepth: Depth %d not supported", depth );
#endif
    }
    return image;
}

/*!
  \overload QImage QImage::convertDepth( int depth ) const
*/

QImage QImage::convertDepth( int depth ) const
{
    return convertDepth( depth, 0 );
}

/*!
  Tests if the ( \a x, \a y ) is a valid coordinate in the image.
*/

bool QImage::valid( int x, int y ) const
{
    return x >= 0 && x < width()
	&& y >= 0 && y < height();
}

/*!
  Returns the pixel index at the given coordinates.

  If (x,y) is not \link valid() valid\endlink, or if
  the image is not a paletted image (depth() \> 8), the results
  are undefined.
*/

int QImage::pixelIndex( int x, int y ) const
{
#if defined(CHECK_RANGE)
    if ( x < 0 || x > width() ) {
	qWarning( "QImage::pixel: x=%d out of range", x );
	return -12345;
    }
#endif
    uchar * s = scanLine( y );
    switch( depth() ) {
    case 1:
	if ( bitOrder() == QImage::LittleEndian )
	    return (*(s + (x >> 3)) >> (x & 7)) & 1;
	else
	    return (*(s + (x >> 3)) >> (7- (x & 7))) & 1;
    case 8:
	return (int)s[x];
#ifndef QT_NO_IMAGE_TRUECOLOR
#ifndef QT_NO_IMAGE_16_BIT
    case 16:
#endif	
    case 32:
#if defined(CHECK_RANGE)
	qWarning( "QImage::pixelIndex: Not applicable for %d-bpp images "
		 "(no palette)", depth() );
#endif
	return 0;
#endif //QT_NO_IMAGE_TRUECOLOR
    }
    return 0;
}


/*!
  Returns the actual color of the pixel at the given coordinates.

  If (x,y) is not \link valid() on the image\endlink, the results
  are undefined.

  \sa setPixel(), qRed(), qGreen(), qBlue()
*/

QRgb QImage::pixel( int x, int y ) const
{
#if defined(CHECK_RANGE)
    if ( x < 0 || x > width() ) {
	qWarning( "QImage::pixel: x=%d out of range", x );
	return 12345;
    }
#endif
    uchar * s = scanLine( y );
    switch( depth() ) {
    case 1:
	if ( bitOrder() == QImage::LittleEndian )
	    return color( (*(s + (x >> 3)) >> (x & 7)) & 1 );
	else
	    return color( (*(s + (x >> 3)) >> (7- (x & 7))) & 1 );
    case 8:
	return color( (int)s[x] );
#ifndef QT_NO_IMAGE_16_BIT
    case 16:
	return qt_conv16ToRgb(((ushort*)s)[x]);
#endif
#ifndef QT_NO_IMAGE_TRUECOLOR
    case 32:
	return ((QRgb*)s)[x];
#endif
    default:
	return 100367;
    }
}


/*!
  Sets the pixel index or color at the given coordinates.

  If (x,y) is not \link valid() valid\endlink, or if
  the image is a paletted image (depth() \<= 8) and \a index_or_rgb
  \>= numColors(), the results are undefined.

  \sa qRgb(), qRgba()
*/

void QImage::setPixel( int x, int y, uint index_or_rgb )
{
    if ( x < 0 || x > width() ) {
#if defined(CHECK_RANGE)
	qWarning( "QImage::setPixel: x=%d out of range", x );
#endif
	return;
    }
    if ( depth() == 1 ) {
	uchar * s = scanLine( y );
	if ( index_or_rgb > 1) {
#if defined(CHECK_RANGE)
	    qWarning( "QImage::setPixel: index=%d out of range",
		     index_or_rgb );
#endif
	} else if ( bitOrder() == QImage::LittleEndian ) {
	    if (index_or_rgb==0)
		*(s + (x >> 3)) &= ~(1 << (x & 7));
	    else
		*(s + (x >> 3)) |= (1 << (x & 7));
	} else {
	    if (index_or_rgb==0)
		*(s + (x >> 3)) &= ~(1 << (7-(x & 7)));
	    else
		*(s + (x >> 3)) |= (1 << (7-(x & 7)));
	}
    } else if ( depth() == 8 ) {
	if (index_or_rgb > (uint)numColors()) {
#if defined(CHECK_RANGE)
	    qWarning( "QImage::setPixel: index=%d out of range",
		     index_or_rgb );
#endif
	    return;
	}
	uchar * s = scanLine( y );
	s[x] = index_or_rgb;
#ifndef QT_NO_IMAGE_16_BIT
    } else if ( depth() == 16 ) {
	ushort * s = (ushort*)scanLine( y );
	s[x] = qt_convRgbTo16(index_or_rgb);
#endif
#ifndef QT_NO_IMAGE_TRUECOLOR
    } else if ( depth() == 32 ) {
	QRgb * s = (QRgb*)scanLine( y );
	s[x] = index_or_rgb;
#endif
    }
}


/*!
  Converts the bit order of the image to \e bitOrder and returns the converted
  image.

  Returns \c *this if the \e bitOrder is equal to the image bit order, or a
  null image if this image cannot be converted.

  \sa bitOrder(), systemBitOrder()
*/

QImage QImage::convertBitOrder( Endian bitOrder ) const
{
    if ( isNull() || data->d != 1 ||		// invalid argument(s)
	 !(bitOrder == BigEndian || bitOrder == LittleEndian) ) {
	QImage nullImage;
	return nullImage;
    }
    if ( data->bitordr == bitOrder )		// nothing to do
	return *this;

    QImage image( data->w, data->h, 1, data->ncols, bitOrder );
    register uchar *p;
    uchar *end;
    uchar *b;
    p = bits();
    b = image.bits();
    end = p + numBytes();
    while ( p < end )
	*b++ = bitflip[*p++];
    memcpy( image.colorTable(), colorTable(), numColors()*sizeof(QRgb) );
    return image;
}

// ### Candidate (renamed) for qcolor.h
static
bool isGray(QRgb c)
{
    return qRed(c) == qGreen(c)
	&& qRed(c) == qBlue(c);
}

/*!
  Returns TRUE if all the colors in the image are shades of
  gray, that is their R, G, and B components are equal.
  This function is slow for large 16-bit and 32-bit images.
*/
bool QImage::allGray() const
{
#ifndef QT_NO_IMAGE_TRUECOLOR    
    if (depth()==32) {
	int p = width()*height();
	QRgb* b = (QRgb*)bits();
	while (p--)
	    if (!isGray(*b++))
		return FALSE;
#ifndef QT_NO_IMAGE_16_BIT
    } else if (depth()==16) {
	int p = width()*height();
	ushort* b = (ushort*)bits();
	while (p--)
	    if (!is16BitGray(*b++))
		return FALSE;
#endif
    } else 
#endif //QT_NO_IMAGE_TRUECOLOR	
	{
	if (!data->ctbl) return TRUE;
	for (int i=0; i<numColors(); i++)
	    if (!isGray(data->ctbl[i]))
		return FALSE;
    }
    return TRUE;
}

/*!
  Returns TRUE if the image is allGray(), \e and if the image is 32-bpp
  or a 256-color 8-bpp image for which color(i) is QRgb(i,i,i).
*/
bool QImage::isGrayscale() const
{
    switch (depth()) {
#ifndef QT_NO_IMAGE_TRUECOLOR
    case 32:
#ifndef QT_NO_IMAGE_16_BIT
    case 16:
#endif
	return allGray();
#endif //QT_NO_IMAGE_TRUECOLOR
    case 8: {
	for (int i=0; i<numColors(); i++)
	    if (data->ctbl[i] != qRgb(i,i,i))
		return FALSE;
	return TRUE;
        }
    }
    return FALSE;
}

#ifndef QT_NO_IMAGE_SMOOTHSCALE
static
void pnmscale(const QImage& src, QImage& dst)
{
#define SCALE 4096
#define HALFSCALE 2048

    QRgb* xelrow = 0;
    QRgb* tempxelrow = 0;
    register QRgb* xP;
    register QRgb* nxP;
    int rows, cols, rowsread, newrows, newcols;
    register int row, col, needtoreadrow;
    const uchar maxval = 255;
    double xscale, yscale;
    long sxscale, syscale;
    register long fracrowtofill, fracrowleft;
    long* as;
    long* rs;
    long* gs;
    long* bs;
    int rowswritten = 0;

    cols = src.width();
    rows = src.height();
    newcols = dst.width();
    newrows = dst.height();

    xscale = (double) newcols / (double) cols;
    yscale = (double) newrows / (double) rows;

    sxscale = (long)(xscale * SCALE);
    syscale = (long)(yscale * SCALE);

    if ( newrows != rows )	/* shortcut Y scaling if possible */
	tempxelrow = new QRgb[cols];

    if ( src.hasAlphaBuffer() ) {
	dst.setAlphaBuffer(TRUE);
	as = new long[cols];
	for ( col = 0; col < cols; ++col )
	    as[col] = HALFSCALE;
    } else {
	as = 0;
    }
    rs = new long[cols];
    gs = new long[cols];
    bs = new long[cols];
    rowsread = 0;
    fracrowleft = syscale;
    needtoreadrow = 1;
    for ( col = 0; col < cols; ++col )
	rs[col] = gs[col] = bs[col] = HALFSCALE;
    fracrowtofill = SCALE;

    for ( row = 0; row < newrows; ++row ) {
	/* First scale Y from xelrow into tempxelrow. */
	if ( newrows == rows ) {
	    /* shortcut Y scaling if possible */
	    tempxelrow = xelrow = (QRgb*)src.scanLine(rowsread++);
	} else {
	    while ( fracrowleft < fracrowtofill ) {
		if ( needtoreadrow && rowsread < rows )
		    xelrow = (QRgb*)src.scanLine(rowsread++);
		for ( col = 0, xP = xelrow; col < cols; ++col, ++xP ) {
		    if (as) {
			as[col] += fracrowleft * qAlpha( *xP );
			rs[col] += fracrowleft * qRed( *xP ) * qAlpha( *xP ) / 255;
			gs[col] += fracrowleft * qGreen( *xP ) * qAlpha( *xP ) / 255;
			bs[col] += fracrowleft * qBlue( *xP ) * qAlpha( *xP ) / 255;
		    } else {
			rs[col] += fracrowleft * qRed( *xP );
			gs[col] += fracrowleft * qGreen( *xP );
			bs[col] += fracrowleft * qBlue( *xP );
		    }
		}
		fracrowtofill -= fracrowleft;
		fracrowleft = syscale;
		needtoreadrow = 1;
	    }
	    /* Now fracrowleft is >= fracrowtofill, so we can produce a row. */
	    if ( needtoreadrow && rowsread < rows ) {
		xelrow = (QRgb*)src.scanLine(rowsread++);
		needtoreadrow = 0;
	    }
	    for ( col = 0, xP = xelrow, nxP = tempxelrow;
		  col < cols; ++col, ++xP, ++nxP )
	    {
		register long a, r, g, b;

		if ( as ) {
		    r = rs[col] + fracrowtofill * qRed( *xP ) * qAlpha( *xP ) / 255;
		    g = gs[col] + fracrowtofill * qGreen( *xP ) * qAlpha( *xP ) / 255;
		    b = bs[col] + fracrowtofill * qBlue( *xP ) * qAlpha( *xP ) / 255;
		    a = as[col] + fracrowtofill * qAlpha( *xP );
		    if ( a ) {
			r = r * 255 / a * SCALE;
			g = g * 255 / a * SCALE;
			b = b * 255 / a * SCALE;
		    }
		} else {
		    r = rs[col] + fracrowtofill * qRed( *xP );
		    g = gs[col] + fracrowtofill * qGreen( *xP );
		    b = bs[col] + fracrowtofill * qBlue( *xP );
		}
		r /= SCALE;
		if ( r > maxval ) r = maxval;
		g /= SCALE;
		if ( g > maxval ) g = maxval;
		b /= SCALE;
		if ( b > maxval ) b = maxval;
		if ( as ) {
		    a /= SCALE;
		    if ( a > maxval ) a = maxval;
		    *nxP = qRgba( (int)r, (int)g, (int)b, (int)a );
		    as[col] = HALFSCALE;
		} else {
		    *nxP = qRgb( (int)r, (int)g, (int)b );
		}
		rs[col] = gs[col] = bs[col] = HALFSCALE;
	    }
	    fracrowleft -= fracrowtofill;
	    if ( fracrowleft == 0 ) {
		fracrowleft = syscale;
		needtoreadrow = 1;
	    }
	    fracrowtofill = SCALE;
	}

	/* Now scale X from tempxelrow into dst and write it out. */
	if ( newcols == cols ) {
	    /* shortcut X scaling if possible */
	    memcpy(dst.scanLine(rowswritten++), tempxelrow, newcols*4);
	} else {
	    register long a, r, g, b;
	    register long fraccoltofill, fraccolleft = 0;
	    register int needcol;

	    nxP = (QRgb*)dst.scanLine(rowswritten++);
	    fraccoltofill = SCALE;
	    a = r = g = b = HALFSCALE;
	    needcol = 0;
	    for ( col = 0, xP = tempxelrow; col < cols; ++col, ++xP ) {
		fraccolleft = sxscale;
		while ( fraccolleft >= fraccoltofill ) {
		    if ( needcol ) {
			++nxP;
			a = r = g = b = HALFSCALE;
		    }
		    if ( as ) {
			r += fraccoltofill * qRed( *xP ) * qAlpha( *xP ) / 255;
			g += fraccoltofill * qGreen( *xP ) * qAlpha( *xP ) / 255;
			b += fraccoltofill * qBlue( *xP ) * qAlpha( *xP ) / 255;
			a += fraccoltofill * qAlpha( *xP );
			if ( a ) {
			    r = r * 255 / a * SCALE;
			    g = g * 255 / a * SCALE;
			    b = b * 255 / a * SCALE;
			}
		    } else {
			r += fraccoltofill * qRed( *xP );
			g += fraccoltofill * qGreen( *xP );
			b += fraccoltofill * qBlue( *xP );
		    }
		    r /= SCALE;
		    if ( r > maxval ) r = maxval;
		    g /= SCALE;
		    if ( g > maxval ) g = maxval;
		    b /= SCALE;
		    if ( b > maxval ) b = maxval;
		    if (as) {
			a /= SCALE;
			if ( a > maxval ) a = maxval;
			*nxP = qRgba( (int)r, (int)g, (int)b, (int)a );
		    } else {
			*nxP = qRgb( (int)r, (int)g, (int)b );
		    }
		    fraccolleft -= fraccoltofill;
		    fraccoltofill = SCALE;
		    needcol = 1;
		}
		if ( fraccolleft > 0 ) {
		    if ( needcol ) {
			++nxP;
			a = r = g = b = HALFSCALE;
			needcol = 0;
		    }
		    if (as) {
			a += fraccolleft * qAlpha( *xP );
			r += fraccolleft * qRed( *xP ) * qAlpha( *xP ) / 255;
			g += fraccolleft * qGreen( *xP ) * qAlpha( *xP ) / 255;
			b += fraccolleft * qBlue( *xP ) * qAlpha( *xP ) / 255;
		    } else {
			r += fraccolleft * qRed( *xP );
			g += fraccolleft * qGreen( *xP );
			b += fraccolleft * qBlue( *xP );
		    }
		    fraccoltofill -= fraccolleft;
		}
	    }
	    if ( fraccoltofill > 0 ) {
		--xP;
		if (as) {
		    a += fraccolleft * qAlpha( *xP );
		    r += fraccoltofill * qRed( *xP ) * qAlpha( *xP ) / 255;
		    g += fraccoltofill * qGreen( *xP ) * qAlpha( *xP ) / 255;
		    b += fraccoltofill * qBlue( *xP ) * qAlpha( *xP ) / 255;
		    if ( a ) {
			r = r * 255 / a * SCALE;
			g = g * 255 / a * SCALE;
			b = b * 255 / a * SCALE;
		    }
		} else {
		    r += fraccoltofill * qRed( *xP );
		    g += fraccoltofill * qGreen( *xP );
		    b += fraccoltofill * qBlue( *xP );
		}
	    }
	    if ( ! needcol ) {
		r /= SCALE;
		if ( r > maxval ) r = maxval;
		g /= SCALE;
		if ( g > maxval ) g = maxval;
		b /= SCALE;
		if ( b > maxval ) b = maxval;
		if (as) {
		    a /= SCALE;
		    if ( a > maxval ) a = maxval;
		    *nxP = qRgba( (int)r, (int)g, (int)b, (int)a );
		} else {
		    *nxP = qRgb( (int)r, (int)g, (int)b );
		}
	    }
	}
    }

    if ( newrows != rows && tempxelrow )// Robust, tempxelrow might be 0 1 day
	delete [] tempxelrow;
    if ( as )				// Avoid purify complaint
	delete [] as;
    if ( rs )				// Robust, rs might be 0 one day
	delete [] rs;
    if ( gs )				// Robust, gs might be 0 one day
	delete [] gs;
    if ( bs )				// Robust, bs might be 0 one day
	delete [] bs;

#undef SCALE
#undef HALFSCALE
}
#endif
/*!
  \fn QImage QImage::smoothScale(int width, int height) const

  Returns a copy of the image smoothly scaled to \a width by \a height
  pixels.  For 32-bpp images, and 1-bpp/8-bpp color images, the result
  will be 32-bpp, while
  \link allGray() all-gray \endlink images (including black-and-white 1-bpp)
  will produce 8-bit
  \link isGrayscale() grayscale \endlink images with the palette spanning
  256 grays from black to white.

  This function uses code based on pnmscale.c by Jef Poskanzer.

  pnmscale.c - read a portable anymap and scale it

  \mustquote

  Copyright (C) 1989, 1991 by Jef Poskanzer.

  Permission to use, copy, modify, and distribute this software and its
  documentation for any purpose and without fee is hereby granted, provided
  that the above copyright notice appear in all copies and that both that
  copyright notice and this permission notice appear in supporting
  documentation.  This software is provided "as is" without express or
  implied warranty.

  \sa mirror()
*/
#ifndef QT_NO_IMAGE_SMOOTHSCALE
QImage QImage::smoothScale(int w, int h) const
{
    if ( isNull() ) {
#if defined(CHECK_RANGE)
	qWarning( "QImage::smoothScale: Image is a null image" );
#endif
	return *this;
    }

    if ( w == width() && h == height() )
	return *this; // nothing to do

    if (depth()==32) {
	QImage img(w, h, 32);
	// 32-bpp to 32-bpp
	pnmscale(*this,img);
	return img;
    } else if ( depth() != 16 && allGray() && !hasAlphaBuffer()) {
	// Inefficient
	return convertDepth(32).smoothScale(w,h).convertDepth(8);
    } else {
	// Inefficient
	return convertDepth(32).smoothScale(w,h);
    }
}
#endif
/*!
  Builds and returns a 1-bpp mask from the alpha buffer in this image.
  Returns a null image if \link setAlphaBuffer() alpha buffer mode\endlink
  is disabled.

  See QPixmap::convertFromImage for a description of the \a
  conversion_flags argument.

  The returned image has little-endian bit order, which you can
  convert to big-endianness using convertBitOrder().
*/

QImage QImage::createAlphaMask( int conversion_flags ) const
{
    if ( conversion_flags == 1 ) {
	// Old code is passing "TRUE".
	conversion_flags = Qt::DiffuseAlphaDither;
    }

    if ( isNull() || !hasAlphaBuffer() ) {
	QImage nullImage;
	return nullImage;
    }

    if ( depth() == 1 ) {
	// A monochrome pixmap, with alpha channels on those two colors.
	// Pretty unlikely, so use less efficient solution.
	return convertDepth(8, conversion_flags)
		.createAlphaMask( conversion_flags );
    }

    QImage mask1;
    dither_to_1( this, &mask1, conversion_flags, TRUE );
    return mask1;
}



/*!
  Creates and returns a 1-bpp heuristic mask for this image. It works by
  selecting a color from one of the corners, then chipping away pixels of
  that color, starting at all the edges.

  The four corners vote over which color is to be masked away. In
  case of a draw (this generally means that this function is not
  applicable to the image) the voting results are undocumented.

  The returned image has little-endian bit order, which you can
  convert to big-endianness using convertBitOrder().

  This function disregards the \link hasAlphaBuffer() alpha buffer. \endlink
*/

QImage QImage::createHeuristicMask( bool clipTight ) const
{
    if ( isNull() ) {
	QImage nullImage;
	return nullImage;
    }
    if ( depth() != 32 ) {
	QImage img32 = convertDepth(32);
	return img32.createHeuristicMask(clipTight);
    }

#define PIX(x,y)  (*((QRgb*)scanLine(y)+x) & 0x00ffffff)

    int w = width();
    int h = height();
    QImage m(w, h, 1, 2, QImage::LittleEndian);
    m.setColor( 0, 0xffffff );
    m.setColor( 1, 0 );
    m.fill( 0xff );

    QRgb background = PIX(0,0);
    if ( background != PIX(w-1,0) &&
	 background != PIX(0,h-1) &&
	 background != PIX(w-1,h-1) ) {
	background = PIX(w-1,0);
	if ( background != PIX(w-1,h-1) &&
	     background != PIX(0,h-1) &&
	     PIX(0,h-1) == PIX(w-1,h-1) ) {
	    background = PIX(w-1,h-1);
	}
    }

    int x,y;
    bool done = FALSE;
    uchar *ypp, *ypc, *ypn;
    while( !done ) {
	done = TRUE;
	ypn = m.scanLine(0);
	ypc = 0;
	for ( y = 0; y < h; y++ ) {
	    ypp = ypc;
	    ypc = ypn;
	    ypn = (y == h-1) ? 0 : m.scanLine(y+1);
	    QRgb *p = (QRgb *)scanLine(y);
	    for ( x = 0; x < w; x++ ) {
		// slowness here - it's possible to do six of these tests
		// together in one go.  oh well.
		if ( ( x == 0 || y == 0 || x == w-1 || y == h-1 ||
		       !(*(ypc + ((x-1) >> 3)) & (1 << ((x-1) & 7))) ||
		       !(*(ypc + ((x+1) >> 3)) & (1 << ((x+1) & 7))) ||
		       !(*(ypp + (x     >> 3)) & (1 << (x     & 7))) ||
		       !(*(ypn + (x     >> 3)) & (1 << (x     & 7))) ) &&
		     (	(*(ypc + (x     >> 3)) & (1 << (x     & 7))) ) &&
		     ( (*p & 0x00ffffff) == background ) ) {
		    done = FALSE;
		    *(ypc + (x >> 3)) &= ~(1 << (x & 7));
		}
		p++;
	    }
	}
    }

    if ( !clipTight ) {
	ypn = m.scanLine(0);
	ypc = 0;
	for ( y = 0; y < h; y++ ) {
	    ypp = ypc;
	    ypc = ypn;
	    ypn = (y == h-1) ? 0 : m.scanLine(y+1);
	    QRgb *p = (QRgb *)scanLine(y);
	    for ( x = 0; x < w; x++ ) {
		if ( (*p & 0x00ffffff) != background ) {
		    if ( x > 0 )
			*(ypc + ((x-1) >> 3)) |= (1 << ((x-1) & 7));
		    if ( x < w-1 )
			*(ypc + ((x+1) >> 3)) |= (1 << ((x+1) & 7));
		    if ( y > 0 )
			*(ypp + (x >> 3)) |= (1 << (x & 7));
		    if ( y < h-1 )
			*(ypn + (x >> 3)) |= (1 << (x & 7));
		}
		p++;
	    }
	}
    }

#undef PIX

    return m;
}

/*
  This code is contributed by Philipp Lang,
  GeneriCom Software Germany (www.generi.com)
  under the terms of the QPL, Version 1.0
*/

/*!
  Returns the image mirrored in the horizontal and/or vertical direction.

  \sa smoothScale()
*/
QImage QImage::mirror(bool horizontal, bool vertical) const
{
    int w = width();
    int h = height();
    if (w <= 1 && h <= 1 || (!horizontal && !vertical))
        return *this;
        
    // Create result image, copy colormap
    QImage result(w, h, depth(), numColors(), bitOrder());
    memcpy(result.colorTable(), colorTable(), numColors()*sizeof(QRgb));
    result.setAlphaBuffer(hasAlphaBuffer());
    
    if (depth() == 1)
        w = (w+7)/8;
    int dxi = horizontal ? -1 : 1;
    int dxs = horizontal ? w-1 : 0;
    int dyi = vertical ? -1 : 1;
    int dy = vertical ? h-1: 0;

    // 1 bit, 8 bit
    if (depth() == 1 || depth() == 8) {
        for (int sy = 0; sy < h; sy++, dy += dyi) {
            Q_UINT8* ssl = (Q_UINT8*)(data->bits[sy]);
            Q_UINT8* dsl = (Q_UINT8*)(result.data->bits[dy]);
            int dx = dxs;
            for (int sx = 0; sx < w; sx++, dx += dxi)
                dsl[dx] = ssl[sx];
        }
    }
#ifndef QT_NO_IMAGE_TRUECOLOR
#ifndef QT_NO_IMAGE_16_BIT
    // 16 bit
    else if (depth() == 16) {
        for (int sy = 0; sy < h; sy++, dy += dyi) {
            Q_UINT16* ssl = (Q_UINT16*)(data->bits[sy]);
            Q_UINT16* dsl = (Q_UINT16*)(result.data->bits[dy]);
            int dx = dxs;
            for (int sx = 0; sx < w; sx++, dx += dxi)
                dsl[dx] = ssl[sx];
        }
    }
#endif
    // 32 bit
    else if (depth() == 32) {
        for (int sy = 0; sy < h; sy++, dy += dyi) {
            Q_UINT32* ssl = (Q_UINT32*)(data->bits[sy]);
            Q_UINT32* dsl = (Q_UINT32*)(result.data->bits[dy]);
            int dx = dxs;
            for (int sx = 0; sx < w; sx++, dx += dxi)
                dsl[dx] = ssl[sx];
        }
    }
#endif
    
    // special handling of 1 bit images for horizontal mirroring
    if (horizontal && depth() == 1) {
        int shift = width() % 8;
        for (int y = h-1; y >= 0; y--) {
            Q_UINT8* a0 = (Q_UINT8*)(result.data->bits[y]);
            // Swap bytes
            Q_UINT8* a = a0+dxs;
            while (a >= a0) {
                *a = bitflip[*a];
                a--;
            }
            // Shift bits if unaligned
            if (shift != 0) {
                a = a0+dxs;
                Q_UINT8 c = 0;
                if (bitOrder() == QImage::LittleEndian) {
                    while (a >= a0) {
                        Q_UINT8 nc = *a << shift;
                        *a = (*a >> (8-shift)) | c;
			--a;
                        c = nc;
                    }
                } else {
                    while (a >= a0) {
                        Q_UINT8 nc = *a >> shift;
                        *a = (*a << (8-shift)) | c;
			--a;
                        c = nc;
                    }
                }
            }
        }
    }
    
    return result;    
}

/*!
  Returns a QImage which is a vertically mirrored copy of this
  image. The original QImage is left unchanged.
*/

QImage QImage::mirror() const
{
    return mirror(FALSE,TRUE);
}


/*!
  Returns a QImage where the values of the red and blue components of
  all pixels have been swapped, effectively converting a RGB image to
  a BGR image. The original QImage is left unchanged.
*/

QImage QImage::swapRGB() const
{
    QImage res = copy();
    if ( !isNull() ) {
#ifndef QT_NO_IMAGE_TRUECOLOR
	if ( depth() == 32 ) {
	    for ( int i=0; i < height(); i++ ) {
		uint *p = (uint*)scanLine( i );
		uint *q = (uint*)res.scanLine( i );
		uint *end = p + width();
		while ( p < end ) {
		    *q = ((*p << 16) & 0xff0000) | ((*p >> 16) & 0xff) |
			 (*p & 0xff00ff00);
		    p++;
		    q++;
		}
	    }
#ifndef QT_NO_IMAGE_16_BIT
	} else if ( depth() == 16 ) {
	    qWarning( "QImage::swapRGB not implemented for 16bpp" );
#endif
	} else 
#endif //QT_NO_IMAGE_TRUECOLOR
	    {
	    uint* p = (uint*)colorTable();
	    uint* q = (uint*)res.colorTable();
	    if ( p && q ) {
		for ( int i=0; i < numColors(); i++ ) {
		    *q = ((*p << 16) & 0xff0000) | ((*p >> 16) & 0xff) |
			 (*p & 0xff00ff00);
		    p++;
		    q++;
		}
	    }
	}
    }
    return res;
}


/*!
  Returns a string that specifies the image format of the file \e fileName,
  or null if the file cannot be read or if the format cannot be recognized.

  The QImageIO documentation lists the guaranteed supported image
  formats, or use the QImage::inputFormats() QImage::outputFormats()
  to get lists that include installed formats.

  \sa load(), save()
*/

const char* QImage::imageFormat( const QString &fileName )
{
    return QImageIO::imageFormat(fileName);
}

/*!
  Returns a list of image formats which are supported for image input.

  \sa inputFormatList()
*/
QStrList QImage::inputFormats()
{
    return QImageIO::inputFormats();
}
#ifndef QT_NO_STRINGLIST
/*!
  Returns a list of image formats which are supported for image input.
*/
QStringList QImage::inputFormatList()
{
    return QStringList::fromStrList(QImageIO::inputFormats());
}


/*!
  Returns a list of image formats which are supported for image output.
*/
QStringList QImage::outputFormatList()
{
    return QStringList::fromStrList(QImageIO::outputFormats());
}
#endif //QT_NO_STRINGLIST

/*!
  Returns a list of image formats which are supported for image output.

  \sa QImage::outputFormatList()
*/
QStrList QImage::outputFormats()
{
    return QImageIO::outputFormats();
}


/*!
  Loads an image from the file \a fileName.
  Returns TRUE if successful, or FALSE if the image could not be loaded.

  If \a format is specified, the loader attempts to read the image using the
  specified format. If \a format is not specified (default),
  the loader reads a few bytes from the header to guess the file format.

  The QImageIO documentation lists the supported image formats and
  explains how to add extra formats.

  \sa loadFromData(), save(), imageFormat(), QPixmap::load(), QImageIO
*/

bool QImage::load( const QString &fileName, const char* format )
{
    QImageIO io( fileName, format );
    bool result = io.read();
    if ( result )
	operator=( io.image() );
    return result;
}

/*!
  Loads an image from the binary data in \e buf (\e len bytes).
  Returns TRUE if successful, or FALSE if the image could not be loaded.

  If \e format is specified, the loader attempts to read the image using the
  specified format. If \e format is not specified (default),
  the loader reads a few bytes from the header to guess the file format.

  The QImageIO documentation lists the supported image formats and
  explains how to add extra formats.

  \sa load(), save(), imageFormat(), QPixmap::loadFromData(), QImageIO
*/

bool QImage::loadFromData( const uchar *buf, uint len, const char *format )
{
    QByteArray a;
    a.setRawData( (char *)buf, len );
    QBuffer b( a );
    b.open( IO_ReadOnly );
    QImageIO io( &b, format );
    bool result = io.read();
    b.close();
    a.resetRawData( (char *)buf, len );
    if ( result )
	operator=( io.image() );
    return result;
}

/*!
  \overload

  Loads from a QByteArray.
*/
bool QImage::loadFromData( QByteArray buf, const char *format )
{
    return loadFromData( (const uchar *)(buf.data()), buf.size(), format );
}

/*!
  Saves the image to the file \e fileName, using the image file format
  \e format and default quality settings.
  Returns TRUE if successful, or FALSE if the image could not be saved.

  \sa load(), loadFromData(), imageFormat(), QPixmap::save(), QImageIO
*/

bool QImage::save( const QString &fileName, const char* format ) const
{
    return save( fileName, format, -1 );
}

/*!
  Saves the image to the file \e fileName, using the image file format
  \e format and a quality factor \e quality.  \e quality must be in the
  range [0,100] or -1.  Specify 0 to obtain small compressed files, 100
  for large uncompressed files and -1 to use the default settings.
  Returns TRUE if successful, or FALSE if the image could not be saved.

  \sa load(), loadFromData(), imageFormat(), QPixmap::save(), QImageIO
*/

bool QImage::save( const QString &fileName, const char* format, int quality ) const
{
    if ( isNull() )
	return FALSE;				// nothing to save
    QImageIO io( fileName, format );
    io.setImage( *this );
    if ( quality > 100  || quality < -1 ) {
#if defined(CHECK_RANGE)
	qWarning( "QPixmap::save: quality out of range [-1,100]" );
#endif
        if ( quality > 100 )
	    quality = 100;
    }
    if ( quality >= 0 ) {
	QString s;
	s.setNum( quality );
	io.setParameters( s.latin1() );
    }
    return io.write();
}


/*****************************************************************************
  QImage stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
/*!
  \relates QImage
  Writes an image to the stream as a PNG image.
  \sa QImage::save()
  \link datastreamformat.html Format of the QDataStream operators \endlink
*/

QDataStream &operator<<( QDataStream &s, const QImage &image )
{
    QImageIO io;
    io.setIODevice( s.device() );
    if ( s.version() == 1 )
	io.setFormat( "BMP" );
    else
	io.setFormat( "PNG" );

    io.setImage( image );
    io.write();
    return s;
}

/*!
  \relates QImage
  Reads an image from the stream.
  \sa QImage::load()
  \link datastreamformat.html Format of the QDataStream operators \endlink
*/

QDataStream &operator>>( QDataStream &s, QImage &image )
{
    QImageIO io( s.device(), 0 );
    if ( io.read() )
	image = io.image();
    return s;
}
#endif

/*****************************************************************************
  Standard image io handlers (defined below)
 *****************************************************************************/

// standard image io handlers (defined below)
#ifndef QT_NO_IMAGEIO_BMP
static void read_bmp_image( QImageIO * );
static void write_bmp_image( QImageIO * );
#endif
#ifndef QT_NO_IMAGEIO_PPM
static void read_pbm_image( QImageIO * );
static void write_pbm_image( QImageIO * );
#endif
#ifndef QT_NO_IMAGEIO_XBM
static void read_xbm_image( QImageIO * );
static void write_xbm_image( QImageIO * );
#endif
#ifndef QT_NO_IMAGEIO_XPM
static void read_xpm_image( QImageIO * );
static void write_xpm_image( QImageIO * );
#endif

#ifndef QT_NO_ASYNC_IMAGE_IO
static void read_async_image( QImageIO * ); // Not in table of handlers
#endif

/*****************************************************************************
  Misc. utility functions
 *****************************************************************************/
#if !defined(QT_NO_IMAGEIO_XPM) || !defined(QT_NO_IMAGEIO_XBM)
static QString fbname( const QString &fileName )	// get file basename (sort of)
{
    QString s = fileName;
    if ( !s.isEmpty() ) {
	int i;
	if ( (i=s.findRev('/')) >= 0 )
	    s = s.mid(i);
	if ( (i=s.findRev('\\')) >= 0 )
	    s = s.mid(i);
	QRegExp r( QString::fromLatin1("[a-zA-Z][a-zA-Z0-9_]*") );
	int p = r.match( s, 0, &i );
	if ( p >= 0 )
	    s = s.mid(p);
	s.truncate(i);
    }
    if ( s.isEmpty() )
	s = QString::fromLatin1("dummy");
    return s;
}
#endif

#ifndef QT_NO_IMAGEIO_BMP
static void swapPixel01( QImage *image )	// 1-bpp: swap 0 and 1 pixels
{
    int i;
    if ( image->depth() == 1 && image->numColors() == 2 ) {
	register uint *p = (uint *)image->bits();
	int nbytes = image->numBytes();
	for ( i=0; i<nbytes/4; i++ ) {
	    *p = ~*p;
	    p++;
	}
	uchar *p2 = (uchar *)p;
	for ( i=0; i<(nbytes&3); i++ ) {
	    *p2 = ~*p2;
	    p2++;
	}
	QRgb t = image->color(0);		// swap color 0 and 1
	image->setColor( 0, image->color(1) );
	image->setColor( 1, t );
    }
}
#endif


/*****************************************************************************
  QImageIO member functions
 *****************************************************************************/

/*!
  \class QImageIO qimage.h

  \brief The QImageIO class contains parameters for loading
  and saving images.

  \ingroup images
  \ingroup io

  QImageIO contains a QIODevice object that is used for image data I/O.
  The programmer can install new image file formats in addition to those
  that Qt implements.

  Qt currently supports the following image file formats: PNG, BMP,
  XBM, XPM and PNM.  It may also support JPEG, MNG and GIF, if specially
  configured during compilation.  The different PNM formats are: PBM
  (P1 or P4), PGM (P2 or P5), PPM (P3 or P6).

  You will normally not need to use this class, QPixmap::load(),
  QPixmap::save() and QImage contain most of the needed functionality.

  For image files which contain sequences of images, only the first is
  read.  See the QMovie for loading multiple images.

  PBM, PGM, and PPM format \e output is always in the more condensed
  raw format. PPM and PGM files with more than 256 levels of intensity
  are scaled down when reading.

  \warning Unisys has changed its position regarding GIF.  If you are
  in a country where Unisys holds a patent on LZW compression and/or
  decompression and you want to use GIF, Unisys may require you to
  license that technology.  These countries include Canada, Japan, the
  USA, France, Germany, Italy and the UK.

  GIF support may be removed completely in a future version of Qt.  We
  recommend using the PNG format.

  \sa QImage, QPixmap, QFile, QMovie
*/

/*!
  Constructs a QImageIO object with all parameters set to zero.
*/

QImageIO::QImageIO()
{
    iostat = 0;
    iodev  = 0;
    params = 0;
}

/*!
  Constructs a QImageIO object with an I/O device and a format tag.
*/

QImageIO::QImageIO( QIODevice *ioDevice, const char *format )
    : frmt(format)
{
    iostat = 0;
    iodev  = ioDevice;
    params = 0;
}

/*!
  Constructs a QImageIO object with a file name and a format tag.
*/

QImageIO::QImageIO( const QString &fileName, const char* format )
    : frmt(format), fname(fileName)
{
    iostat = 0;
    iodev  = 0;
    params = 0;
}

/*!
  Destructs the object and all related data.
*/

QImageIO::~QImageIO()
{
    if ( params )
	delete [] params;
}


/*****************************************************************************
  QImageIO image handler functions
 *****************************************************************************/

class QImageHandler
{
public:
    QImageHandler( const char *f, const char *h, const QCString& fl,
		   image_io_handler r, image_io_handler w );
    QCString	      format;			// image format
    QRegExp	      header;			// image header pattern
    enum TMode { Untranslated=0, TranslateIn, TranslateInOut } text_mode;
    image_io_handler  read_image;		// image read function
    image_io_handler  write_image;		// image write function
    bool	      obsolete;			// support not "published"
};

QImageHandler::QImageHandler( const char *f, const char *h, const QCString& fl,
			      image_io_handler r, image_io_handler w )
    : format(f), header(QString::fromLatin1(h))
{
    text_mode = Untranslated;
    if ( fl.contains('t') )
	text_mode = TranslateIn;
    else if ( fl.contains('T') )
	text_mode = TranslateInOut;
    obsolete = fl.contains('O');
    read_image	= r;
    write_image = w;
}

typedef QList<QImageHandler> QIHList;// list of image handlers
static QIHList *imageHandlers = 0;

static void cleanup_image_handlers()		// cleanup image handler list
{
    delete imageHandlers;
    imageHandlers = 0;
}

void qt_init_image_handlers()		// initialize image handlers
{
    if ( !imageHandlers ) {
	imageHandlers = new QIHList;
	CHECK_PTR( imageHandlers );
	imageHandlers->setAutoDelete( TRUE );
	qAddPostRoutine( cleanup_image_handlers );
#ifndef QT_NO_IMAGEIO_BMP
	QImageIO::defineIOHandler( "BMP", "^BM", 0,
				   read_bmp_image, write_bmp_image );
#endif
#ifndef QT_NO_IMAGEIO_PPM
	QImageIO::defineIOHandler( "PBM", "^P1", "t",
				   read_pbm_image, write_pbm_image );
	QImageIO::defineIOHandler( "PBMRAW", "^P4", "O",
				   read_pbm_image, write_pbm_image );
	QImageIO::defineIOHandler( "PGM", "^P2", "t",
				   read_pbm_image, write_pbm_image );
	QImageIO::defineIOHandler( "PGMRAW", "^P5", "O",
				   read_pbm_image, write_pbm_image );
	QImageIO::defineIOHandler( "PPM", "^P3", "t",
				   read_pbm_image, write_pbm_image );
	QImageIO::defineIOHandler( "PPMRAW", "^P6", "O",
				   read_pbm_image, write_pbm_image );
#endif
#ifndef QT_NO_IMAGEIO_XBM
	QImageIO::defineIOHandler( "XBM", "^#define", "T",
				   read_xbm_image, write_xbm_image );
#endif
#ifndef QT_NO_IMAGEIO_XPM
	QImageIO::defineIOHandler( "XPM", "/\\*.XPM.\\*/", "T",
				   read_xpm_image, write_xpm_image );
#endif
#ifndef QT_NO_IMAGEIO_MNG
	qInitMngIO();
#endif
#ifndef QT_NO_IMAGEIO_PNG
	qInitPngIO();
#endif
#ifndef QT_NO_IMAGEIO_JPEG
	qInitJpegIO();
#endif
    }
}

static QImageHandler *get_image_handler( const char *format )
{						// get pointer to handler
    qt_init_image_handlers();
    register QImageHandler *p = imageHandlers->first();
    while ( p ) {				// traverse list
	if ( p->format == format )
	    return p;
	p = imageHandlers->next();
    }
    return 0;					// no such handler
}


struct QImageIOData //### use instead of QImageIO::params in 3.0
{
    const char *params;
    int quality;
};


/*!
  Defines a image IO handler for a specified image format.
  An image IO handler is responsible for reading and writing images.

  \arg \e format is the name of the format.
  \arg \e header is a regular expression that recognizes the image header.
  \arg \e flags is "T" for text formats like PBM; generally you will
	  want to use 0.
  \arg \e read_image is a function to read an image of this format.
  \arg \e write_image is a function to write an image of this format.

  Both read_image and write_image are of type image_io_handler, which is
  a function pointer.

  Example:
  \code
    void readGIF( QImageIO *image )
    {
      // read the image, using the image->ioDevice()
    }

    void writeGIF( QImageIO *image )
    {
      // write the image, using the image->ioDevice()
    }

    // add the GIF image handler

    QImageIO::defineIOHandler( "GIF",
			       "^GIF[0-9][0-9][a-z]",
			       0,
			       readGIF,
			       writeGIF );
  \endcode

  Prior to comparison with the regular expression, the file header is
  converted to change all 0 bytes into 1 bytes. This is done because 0
  is such a common header byte yet regular expressions cannot match it.

  For image formats supporting incremental display, such as sequences
  of animated frames, see the QImageFormatType class.
*/

void QImageIO::defineIOHandler( const char *format,
				const char *header,
				const char *flags,
				image_io_handler read_image,
				image_io_handler write_image )
{
    qt_init_image_handlers();
    QImageHandler *p;
    p = new QImageHandler( format, header, flags,
			   read_image, write_image );
    CHECK_PTR( p );
    imageHandlers->insert( 0, p );
}


/*****************************************************************************
  QImageIO normal member functions
 *****************************************************************************/

/*!
  \fn const QImage &QImageIO::image() const
  Returns the image currently set.
  \sa setImage()
*/

/*!
  \fn int QImageIO::status() const
  Returns the image IO status.	A non-zero value indicates an error, while 0
  means that the IO operation was successful.
  \sa setStatus()
*/

/*!
  \fn const char *QImageIO::format() const
  Returns the image format string, or 0 if no format has been explicitly set.
*/

/*!
  \fn QIODevice *QImageIO::ioDevice() const
  Returns the IO device currently set.
  \sa setIODevice()
*/

/*!
  \fn QString QImageIO::fileName() const
  Returns the file name currently set.
  \sa setFileName()
*/

/*!
  \fn const char *QImageIO::parameters() const
  Returns image parameters string.
  \sa setParameters()
*/

/*!
  \fn QString QImageIO::description() const
  Returns the image description string.
  \sa setDescription()
*/


/*!
  Sets the image.
  \sa image()
*/

void QImageIO::setImage( const QImage &image )
{
    im = image;
}

/*!
  Sets the image IO status.  A non-zero value indicates an error, while 0 means
  that the IO operation was successful.
  \sa status()
*/

void QImageIO::setStatus( int status )
{
    iostat = status;
}

/*!
  Sets the image format name of the image about to be read or written.

  It is necessary to specify a format before writing an image.

  It is not necessary to specify a format before reading an image.
  If not format has been set, Qt guesses the image format before reading
  it.  If a format is set, but the image has another (valid) format,
  the image will not be read.

  \sa read(), write(), format()
*/

void QImageIO::setFormat( const char *format )
{
    frmt = format;
}

/*!
  Sets the IO device to be used for reading or writing an image.

  Setting the IO device allows images to be read/written to any
  block-oriented QIODevice.

  If \e ioDevice is not null, this IO device will override file name
  settings.

  \sa setFileName()
*/

void QImageIO::setIODevice( QIODevice *ioDevice )
{
    iodev = ioDevice;
}

/*!
  Sets the name of the file to read or write an image.
  \sa setIODevice()
*/

void QImageIO::setFileName( const QString &fileName )
{
    fname = fileName;
}

/*!
  Sets the image parameters string for image handlers that require
  special parameters.

  Although all image formats supported by Qt ignore the parameters string,
  it will be useful for future extensions or contributions (like JPEG).
*/

void QImageIO::setParameters( const char *parameters )
{
    if ( params )
	delete [] params;
    params = qstrdup( parameters );
}

/*!
  Sets the image description string for image handlers that support image
  descriptions.

  Currently, no image format supported by Qt use the description string.
*/

void QImageIO::setDescription( const QString &description )
{
    descr = description;
}


/*!
  Returns a string that specifies the image format of the file \e fileName,
  or null if the file cannot not be read or if the format is not recognized.
*/

const char* QImageIO::imageFormat( const QString &fileName )
{
    QFile file( fileName );
    if ( !file.open(IO_ReadOnly) )
	return 0;
    const char* format = imageFormat( &file );
    file.close();
    return format;
}

/*!
  Returns a string that specifies the image format of the image read from
  \e d, or null if the file cannot be read or if the format is not recognized.
*/

const char *QImageIO::imageFormat( QIODevice *d )
{
    const int buflen = 14;
    char buf[buflen];
    qt_init_image_handlers();
    int pos   = d->at();			// save position
    int rdlen = d->readBlock( buf, buflen );	// read a few bytes

    if ( rdlen != buflen )
	return 0;

    const char* format = 0;
#ifndef QT_NO_ASYNC_IMAGE_IO
    // Try asynchronous loaders first (before we 0->1 the header),
    // but overwrite if found in IOHandlers.
    format = QImageDecoder::formatName((uchar*)buf, rdlen);
#endif

    for ( int n = 0; n<rdlen; n++ )
	if ( buf[n] == '\0' )
	    buf[n] = '\001';
    if ( d->status() == IO_Ok && rdlen > 0 ) {
	buf[rdlen-1] = '\0';
	QImageHandler *p = imageHandlers->first();
	while ( p ) {
	    if ( p->header.match(QString::fromLatin1(buf)) != -1 )
		// try match with headers
	    {
		format = p->format;
		break;
	    }
	    p = imageHandlers->next();
	}
    }
    d->at( pos );				// restore position
    return format;
}

/*!
  Returns a sorted list of image formats which are supported for image input.
*/
QStrList QImageIO::inputFormats()
{
    QStrList result;

    qt_init_image_handlers();

#ifndef QT_NO_ASYNC_IMAGE_IO
    // Include asynchronous loaders first.
    result = QImageDecoder::inputFormats();
#endif

    QImageHandler *p = imageHandlers->first();
    while ( p ) {
	if ( p->read_image
	    && !p->obsolete
	    && !result.contains(p->format) )
	{
	    result.inSort(p->format);
	}
	p = imageHandlers->next();
    }

    return result;
}

/*!
  Returns a sorted list of image formats which are supported for image output.
*/
QStrList QImageIO::outputFormats()
{
    QStrList result;

    qt_init_image_handlers();

    // Include asynchronous writers (!) first.
    // (None)

    QImageHandler *p = imageHandlers->first();
    while ( p ) {
	if ( p->write_image
	    && !p->obsolete
	    && !result.contains(p->format) )
	{
	    result.inSort(p->format);
	}
	p = imageHandlers->next();
    }

    return result;
}



/*!
  Reads an image into memory and returns TRUE if the image was successfully
  read.

  Before reading an image, you must set an IO device or a file name.
  If both an IO device and a file name has been set, then the IO device will
  be used.

  Setting the image file format string is optional.

  Note that this function does \e not set the \link format() format\endlink
  used to read the image.  If you need that information, use the imageFormat()
  static functions.

  Example:

  \code
    QImageIO iio;
    QPixmap  pixmap;
    iio.setFileName( "burger.bmp" );
    if ( image.read() )			// ok
	pixmap = iio.image();		// convert to pixmap
  \endcode

  \sa setIODevice(), setFileName(), setFormat(), write(), QPixmap::load()
*/

bool QImageIO::read()
{
    QFile	   file;
    const char	  *image_format;
    QImageHandler *h;

    if ( iodev ) {				// read from io device
	// ok, already open
    } else if ( !fname.isEmpty() ) {		// read from file
	file.setName( fname );
	if ( !file.open(IO_ReadOnly) )
	    return FALSE;			// cannot open file
	iodev = &file;
    } else {					// no file name or io device
	return FALSE;
    }
    if (frmt.isEmpty()) {
	// Try to guess format
	image_format = imageFormat( iodev );	// get image format

	if ( !image_format ) {
	    if ( file.isOpen() ) {			// unknown format
		file.close();
		iodev = 0;
	    }
	    return FALSE;
	}
    } else {
	image_format = frmt;
    }

    h = get_image_handler( image_format );
    if ( file.isOpen() ) {
#if !defined(_OS_UNIX_)
	if ( h && h->text_mode ) {		// reopen in translated mode
	    file.close();
	    file.open( IO_ReadOnly | IO_Translate );
	}
	else
#endif
	    file.at( 0 );			// position to start
    }
    iostat = 1;					// assume error

    if ( h && h->read_image ) {
	(*h->read_image)( this );
    }
#ifndef QT_NO_ASYNC_IMAGE_IO
    else {
	// Format name, but no handler - must be an asychronous reader
	read_async_image( this );
    }
#endif

    if ( file.isOpen() ) {			// image was read using file
	file.close();
	iodev = 0;
    }
    return iostat == 0;				// image successfully read?
}


/*!
  Writes an image to an IO device and returns TRUE if the image was
  successfully written.

  Before writing an image, you must set an IO device or a file name.
  If both an IO device and a file name has been set, then the IO
  device will be used.

  The image will be written using the specified image format.

  Example:
  \code
    QImageIO iio;
    QImage   im;
    im = pixmap;				// convert to image
    iio.setImage( im );
    iio.setFileName( "burger.bmp" );
    iio.setFormat( "BMP" );
    iio.write();				// TRUE if ok
  \endcode

  \sa setIODevice(), setFileName(), setFormat(), read(), QPixmap::save()
*/

bool QImageIO::write()
{
    if ( frmt.isEmpty() )
	return FALSE;
    QImageHandler *h = get_image_handler( frmt );
    if ( !h || !h->write_image ) {
#if defined(CHECK_RANGE)
	qWarning( "QImageIO::write: No such image format handler: %s",
		 format() );
#endif
	return FALSE;
    }
    QFile file;
    if ( !iodev && !fname.isEmpty() ) {
	file.setName( fname );
	bool translate = h->text_mode==QImageHandler::TranslateInOut;
	int fmode = translate ? IO_WriteOnly|IO_Translate : IO_WriteOnly;
	if ( !file.open(fmode) )		// couldn't create file
	    return FALSE;
	iodev = &file;
    }
    iostat = 1;
    (*h->write_image)( this );
    if ( file.isOpen() ) {			// image was written using file
	file.close();
	iodev = 0;
    }
    return iostat == 0;				// image successfully written?
}

#ifndef QT_NO_IMAGEIO_BMP

/*****************************************************************************
  BMP (DIB) image read/write functions
 *****************************************************************************/

const int BMP_FILEHDR_SIZE = 14;		// size of BMP_FILEHDR data

struct BMP_FILEHDR {				// BMP file header
    char   bfType[2];				// "BM"
    Q_INT32  bfSize;				// size of file
    Q_INT16  bfReserved1;
    Q_INT16  bfReserved2;
    Q_INT32  bfOffBits;				// pointer to the pixmap bits
};

QDataStream &operator>>( QDataStream &s, BMP_FILEHDR &bf )
{						// read file header
    s.readRawBytes( bf.bfType, 2 );
    s >> bf.bfSize >> bf.bfReserved1 >> bf.bfReserved2 >> bf.bfOffBits;
    return s;
}

QDataStream &operator<<( QDataStream &s, const BMP_FILEHDR &bf )
{						// write file header
    s.writeRawBytes( bf.bfType, 2 );
    s << bf.bfSize << bf.bfReserved1 << bf.bfReserved2 << bf.bfOffBits;
    return s;
}


const int BMP_OLD  = 12;			// old Windows/OS2 BMP size
const int BMP_WIN  = 40;			// new Windows BMP size
const int BMP_OS2  = 64;			// new OS/2 BMP size

const int BMP_RGB  = 0;				// no compression
const int BMP_RLE8 = 1;				// run-length encoded, 8 bits
const int BMP_RLE4 = 2;				// run-length encoded, 4 bits
const int BMP_BITFIELDS = 3;			// RGB values encoded in data as bit-fields

struct BMP_INFOHDR {				// BMP information header
    Q_INT32  biSize;				// size of this struct
    Q_INT32  biWidth;				// pixmap width
    Q_INT32  biHeight;				// pixmap height
    Q_INT16  biPlanes;				// should be 1
    Q_INT16  biBitCount;			// number of bits per pixel
    Q_INT32  biCompression;			// compression method
    Q_INT32  biSizeImage;				// size of image
    Q_INT32  biXPelsPerMeter;			// horizontal resolution
    Q_INT32  biYPelsPerMeter;			// vertical resolution
    Q_INT32  biClrUsed;				// number of colors used
    Q_INT32  biClrImportant;			// number of important colors
};


QDataStream &operator>>( QDataStream &s, BMP_INFOHDR &bi )
{
    s >> bi.biSize;
    if ( bi.biSize == BMP_WIN || bi.biSize == BMP_OS2 ) {
	s >> bi.biWidth >> bi.biHeight >> bi.biPlanes >> bi.biBitCount;
	s >> bi.biCompression >> bi.biSizeImage;
	s >> bi.biXPelsPerMeter >> bi.biYPelsPerMeter;
	s >> bi.biClrUsed >> bi.biClrImportant;
    }
    else {					// probably old Windows format
	Q_INT16 w, h;
	s >> w >> h >> bi.biPlanes >> bi.biBitCount;
	bi.biWidth  = w;
	bi.biHeight = h;
	bi.biCompression = BMP_RGB;		// no compression
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = bi.biYPelsPerMeter = 0;
	bi.biClrUsed = bi.biClrImportant = 0;
    }
    return s;
}

QDataStream &operator<<( QDataStream &s, const BMP_INFOHDR &bi )
{
    s << bi.biSize;
    s << bi.biWidth << bi.biHeight;
    s << bi.biPlanes;
    s << bi.biBitCount;
    s << bi.biCompression;
    s << bi.biSizeImage;
    s << bi.biXPelsPerMeter << bi.biYPelsPerMeter;
    s << bi.biClrUsed << bi.biClrImportant;
    return s;
}

static
int calc_shift(int mask)
{
    int result = 0;
    while (!(mask & 1)) {
	result++;
	mask >>= 1;
    }
    return result;
}

static
bool read_dib( QDataStream& s, int offset, int startpos, QImage& image )
{
    BMP_INFOHDR bi;
    QIODevice* d = s.device();

    s >> bi;					// read BMP info header
    if ( d->atEnd() )				// end of stream/file
	return FALSE;
#if 0
    qDebug( "offset...........%d", offset );
    qDebug( "startpos.........%d", startpos );
    qDebug( "biSize...........%d", bi.biSize );
    qDebug( "biWidth..........%d", bi.biWidth );
    qDebug( "biHeight.........%d", bi.biHeight );
    qDebug( "biPlanes.........%d", bi.biPlanes );
    qDebug( "biBitCount.......%d", bi.biBitCount );
    qDebug( "biCompression....%d", bi.biCompression );
    qDebug( "biSizeImage......%d", bi.biSizeImage );
    qDebug( "biXPelsPerMeter..%d", bi.biXPelsPerMeter );
    qDebug( "biYPelsPerMeter..%d", bi.biYPelsPerMeter );
    qDebug( "biClrUsed........%d", bi.biClrUsed );
    qDebug( "biClrImportant...%d", bi.biClrImportant );
#endif
    int w = bi.biWidth,	 h = bi.biHeight,  nbits = bi.biBitCount;
    int t = bi.biSize,	 comp = bi.biCompression;
    int red_mask, green_mask, blue_mask;
    int red_shift, green_shift, blue_shift;
    int red_scale, green_scale, blue_scale;

    if ( !(nbits == 1 || nbits == 4 || nbits == 8 || nbits == 24 || nbits == 32) ||
	bi.biPlanes != 1 || comp > BMP_BITFIELDS )
	return FALSE;					// weird BMP image
    if ( !(comp == BMP_RGB || (nbits == 4 && comp == BMP_RLE4) ||
	(nbits == 8 && comp == BMP_RLE8) || ((nbits == 16 || nbits == 32) && comp == BMP_BITFIELDS)) )
	 return FALSE;				// weird compression type

    int ncols;
    int depth;
    switch ( nbits ) {
	case 32:
	case 24:
	case 16:
	    depth = 32;
	    break;
	case 8:
	case 4:
	    depth = 8;
	    break;
	default:
	    depth = 1;
    }
    if ( depth == 32 )				// there's no colormap
	ncols = 0;
    else					// # colors used
	ncols = bi.biClrUsed ? bi.biClrUsed : 1 << nbits;

    image.create( w, h, depth, ncols, nbits == 1 ?
		  QImage::BigEndian : QImage::IgnoreEndian );
    if ( image.isNull() )			// could not create image
	return FALSE;

    d->at( startpos + BMP_FILEHDR_SIZE + bi.biSize ); // goto start of colormap

    if ( ncols > 0 ) {				// read color table
	uchar rgb[4];
	int   rgb_len = t == BMP_OLD ? 3 : 4;
	for ( int i=0; i<ncols; i++ ) {
	    if ( d->readBlock( (char *)rgb, rgb_len ) != rgb_len )
		return FALSE;
	    image.setColor( i, qRgb(rgb[2],rgb[1],rgb[0]) );
	    if ( d->atEnd() )			// truncated file
		return FALSE;
	}
    } else if (comp == BMP_BITFIELDS && (nbits == 16 || nbits == 32)) {
	if ( d->readBlock( (char *)&red_mask, sizeof(red_mask)) != sizeof(red_mask) )
	    return FALSE;
	if ( d->readBlock( (char *)&green_mask, sizeof(green_mask)) != sizeof(green_mask) )
	    return FALSE;
	if ( d->readBlock( (char *)&blue_mask, sizeof(blue_mask)) != sizeof(blue_mask) )
	    return FALSE;
	red_shift = calc_shift(red_mask);
	red_scale = 256 / ((red_mask >> red_shift) + 1);
	green_shift = calc_shift(green_mask);
	green_scale = 256 / ((green_mask >> green_shift) + 1);
	blue_shift = calc_shift(blue_mask);
	blue_scale = 256 / ((blue_mask >> blue_shift) + 1);
    } else if (comp == BMP_RGB && (nbits == 24 || nbits == 32)) {
	blue_mask = 0x000000ff;
	green_mask = 0x0000ff00;
	red_mask = 0x00ff0000;
	blue_shift = 0;
	green_shift = 8;
	red_shift = 16;
	blue_scale = green_scale = red_scale = 1;
    } else if (comp == BMP_RGB && nbits == 16)  // don't support RGB values for 15/16 bpp
	return FALSE; 

    // offset can be bogus, be careful
    if (offset>=0 && startpos + offset > d->at() )
	d->at( startpos + offset );		// start of image data

    int	     bpl = image.bytesPerLine();
    uchar **line = image.jumpTable();

    if ( nbits == 1 ) {				// 1 bit BMP image
	while ( --h >= 0 ) {
	    if ( d->readBlock((char*)line[h],bpl) != bpl )
		break;
	}
	if ( ncols == 2 && qGray(image.color(0)) < qGray(image.color(1)) )
	    swapPixel01( &image );		// pixel 0 is white!
    }

    else if ( nbits == 4 ) {			// 4 bit BMP image
	int    buflen = ((w+7)/8)*4;
	uchar *buf    = new uchar[buflen];
	CHECK_PTR( buf );
	if ( comp == BMP_RLE4 ) {		// run length compression
	    int x=0, y=0, b, c, i;
	    register uchar *p = line[h-1];
	    uchar *endp = line[h-1]+w;
	    while ( y < h ) {
		if ( (b=d->getch()) == EOF )
		    break;
		if ( b == 0 ) {			// escape code
		    switch ( (b=d->getch()) ) {
			case 0:			// end of line
			    x = 0;
			    y++;
			    p = line[h-y-1];
			    break;
			case 1:			// end of image
			case EOF:		// end of file
			    y = h;		// exit loop
			    break;
			case 2:			// delta (jump)
			    x += d->getch();
			    y += d->getch();

			    // Protection
			    if ( (uint)x >= (uint)w )
				x = w-1;
			    if ( (uint)y >= (uint)h )
				y = h-1;

			    p = line[h-y-1] + x;
			    break;
			default:		// absolute mode
			    // Protection
			    if ( p + b > endp )
				b = endp-p;

			    i = (c = b)/2;
			    while ( i-- ) {
				b = d->getch();
				*p++ = b >> 4;
				*p++ = b & 0x0f;
			    }
			    if ( c & 1 )
				*p++ = d->getch() >> 4;
			    if ( (((c & 3) + 1) & 2) == 2 )
				d->getch();	// align on word boundary
			    x += c;
		    }
		} else {			// encoded mode
		    // Protection
		    if ( p + b > endp )
			b = endp-p;

		    i = (c = b)/2;
		    b = d->getch();		// 2 pixels to be repeated
		    while ( i-- ) {
			*p++ = b >> 4;
			*p++ = b & 0x0f;
		    }
		    if ( c & 1 )
			*p++ = b >> 4;
		    x += c;
		}
	    }
	} else if ( comp == BMP_RGB ) {		// no compression
	    while ( --h >= 0 ) {
		if ( d->readBlock((char*)buf,buflen) != buflen )
		    break;
		register uchar *p = line[h];
		uchar *b = buf;
		for ( int i=0; i<w/2; i++ ) {	// convert nibbles to bytes
		    *p++ = *b >> 4;
		    *p++ = *b++ & 0x0f;
		}
		if ( w & 1 )			// the last nibble
		    *p = *b >> 4;
	    }
	}
	delete [] buf;
    }

    else if ( nbits == 8 ) {			// 8 bit BMP image
	if ( comp == BMP_RLE8 ) {		// run length compression
	    int x=0, y=0, b;
	    register uchar *p = line[h-1];
	    while ( y < h ) {
		if ( (b=d->getch()) == EOF )
		    break;
		if ( b == 0 ) {			// escape code
		    switch ( (b=d->getch()) ) {
			case 0:			// end of line
			    x = 0;
			    y++;
			    p = line[h-y-1];
			    break;
			case 1:			// end of image
			case EOF:		// end of file
			    y = h;		// exit loop
			    break;
			case 2:			// delta (jump)
			    x += d->getch();
			    y += d->getch();
			    p = line[h-y-1] + x;
			    break;
			default:		// absolute mode
			    if ( d->readBlock( (char *)p, b ) != b )
				return FALSE;
			    if ( (b & 1) == 1 )
				d->getch();	// align on word boundary
			    x += b;
			    p += b;
		    }
		} else {			// encoded mode
		    memset( p, d->getch(), b ); // repeat pixel
		    x += b;
		    p += b;
		}
	    }
	} else if ( comp == BMP_RGB ) {		// uncompressed
	    while ( --h >= 0 ) {
		if ( d->readBlock((char *)line[h],bpl) != bpl )
		    break;
	    }
	}
    }

    else if ( nbits == 16 || nbits == 24 || nbits == 32 ) { // 16,24,32 bit BMP image
	register QRgb *p;
	QRgb  *end;
	uchar *buf24 = new uchar[bpl];
	int    bpl24 = ((w*nbits+31)/32)*4;
	uchar *b;
	int c;

	while ( --h >= 0 ) {
	    p = (QRgb *)line[h];
	    end = p + w;
	    if ( d->readBlock( (char *)buf24,bpl24) != bpl24 )
		break;
	    b = buf24;
	    while ( p < end ) {
		c = *(uchar*)b | *(uchar*)(b+1)<<8;
		if (nbits != 16)
		    c |= *(uchar*)(b+2)<<16;
		*p++ = qRgb(((c & red_mask) >> red_shift) * red_scale,
					((c & green_mask) >> green_shift) * green_scale,
					((c & blue_mask) >> blue_shift)) * blue_scale;
		b += nbits/8;
	    }
	}
	delete[] buf24;
    }

    return TRUE;
}

bool qt_read_dib( QDataStream& s, QImage& image )
{
    return read_dib(s,-1,-BMP_FILEHDR_SIZE,image);
}


static void read_bmp_image( QImageIO *iio )
{
    QIODevice  *d = iio->ioDevice();
    QDataStream s( d );
    BMP_FILEHDR bf;
    int		startpos = d->at();

    s.setByteOrder( QDataStream::LittleEndian );// Intel byte order
    s >> bf;					// read BMP file header

    if ( qstrncmp(bf.bfType,"BM",2) != 0 )	// not a BMP image
	return;

    QImage image;
    if (read_dib( s, bf.bfOffBits, startpos, image )) {
	iio->setImage( image );
	iio->setStatus( 0 );			// image ok
    }
}

bool qt_write_dib( QDataStream& s, QImage image )
{
    int	nbits;
    int	bpl_bmp;
    int	bpl = image.bytesPerLine();

    QIODevice* d = s.device();

    if ( image.depth() == 8 && image.numColors() <= 16 ) {
	bpl_bmp = (((bpl+1)/2+3)/4)*4;
	nbits = 4;
    } else if ( image.depth() == 32 ) {
	bpl_bmp = ((image.width()*24+31)/32)*4;
	nbits = 24;
    } else {
	bpl_bmp = bpl;
	nbits = image.depth();
    }

    BMP_INFOHDR bi;
    bi.biSize	       = BMP_WIN;		// build info header
    bi.biWidth	       = image.width();
    bi.biHeight	       = image.height();
    bi.biPlanes	       = 1;
    bi.biBitCount      = nbits;
    bi.biCompression   = BMP_RGB;
    bi.biSizeImage     = bpl_bmp*image.height();
    bi.biXPelsPerMeter = 2834;			// 72 dpi
    bi.biYPelsPerMeter = 2834;
    bi.biClrUsed       = image.numColors();
    bi.biClrImportant  = image.numColors();
    s << bi;					// write info header

    if ( image.depth() != 32 ) {		// write color table
	uchar *color_table = new uchar[4*image.numColors()];
	uchar *rgb = color_table;
	QRgb *c = image.colorTable();
	for ( int i=0; i<image.numColors(); i++ ) {
	    *rgb++ = qBlue ( c[i] );
	    *rgb++ = qGreen( c[i] );
	    *rgb++ = qRed  ( c[i] );
	    *rgb++ = 0;
	}
	d->writeBlock( (char *)color_table, 4*image.numColors() );
	delete [] color_table;
    }

    if ( image.depth() == 1 && image.bitOrder() != QImage::BigEndian )
	image = image.convertBitOrder( QImage::BigEndian );

    int	 y;

    if ( nbits == 1 || nbits == 8 ) {		// direct output
	for ( y=image.height()-1; y>=0; y-- )
	    d->writeBlock( (char*)image.scanLine(y), bpl );
	return TRUE;
    }

    uchar *buf	= new uchar[bpl_bmp];
    uchar *b, *end;
    register uchar *p;

    memset( buf, 0, bpl_bmp );
    for ( y=image.height()-1; y>=0; y-- ) {	// write the image bits
	if ( nbits == 4 ) {			// convert 8 -> 4 bits
	    p = image.scanLine(y);
	    b = buf;
	    end = b + image.width()/2;
	    while ( b < end ) {
		*b++ = (*p << 4) | (*(p+1) & 0x0f);
		p += 2;
	    }
	    if ( image.width() & 1 )
		*b = *p << 4;
	} else {				// 32 bits
	    QRgb *p   = (QRgb *)image.scanLine( y );
	    QRgb *end = p + image.width();
	    b = buf;
	    while ( p < end ) {
		*b++ = qBlue(*p);
		*b++ = qGreen(*p);
		*b++ = qRed(*p);
		p++;
	    }
	}
	d->writeBlock( (char*)buf, bpl_bmp );
    }
    delete[] buf;
    return TRUE;
}


static void write_bmp_image( QImageIO *iio )
{
    QIODevice  *d = iio->ioDevice();
    QImage	image = iio->image();
    QDataStream s( d );
    BMP_FILEHDR bf;
    int		bpl_bmp;
    int		bpl = image.bytesPerLine();

    // Code partially repeated in qt_write_dib
    if ( image.depth() == 8 && image.numColors() <= 16 ) {
	bpl_bmp = (((bpl+1)/2+3)/4)*4;
    } else if ( image.depth() == 32 ) {
	bpl_bmp = ((image.width()*24+31)/32)*4;
    } else {
	bpl_bmp = bpl;
    }

    iio->setStatus( 0 );
    s.setByteOrder( QDataStream::LittleEndian );// Intel byte order
    strncpy( bf.bfType, "BM", 2 );		// build file header
    bf.bfReserved1 = bf.bfReserved2 = 0;	// reserved, should be zero
    bf.bfOffBits   = BMP_FILEHDR_SIZE + BMP_WIN + image.numColors()*4;
    bf.bfSize	   = bf.bfOffBits + bpl_bmp*image.height();
    s << bf;					// write file header

    qt_write_dib( s, image );
}

#endif // QT_NO_IMAGEIO_BMP

#ifndef QT_NO_IMAGEIO_PPM

/*****************************************************************************
  PBM/PGM/PPM (ASCII and RAW) image read/write functions
 *****************************************************************************/

static int read_pbm_int( QIODevice *d )
{
    int	  c;
    int	  val = -1;
    bool  digit;
    const int buflen = 100;
    char  buf[buflen];
    while ( TRUE ) {
	if ( (c=d->getch()) == EOF )		// end of file
	    break;
	digit = isdigit(c);
	if ( val != -1 ) {
	    if ( digit ) {
		val = 10*val + c - '0';
		continue;
	    } else {
		if ( c == '#' )			// comment
		    d->readLine( buf, buflen );
		break;
	    }
	}
	if ( digit )				// first digit
	    val = c - '0';
	else if ( isspace(c) )
	    continue;
	else if ( c == '#' )
	    d->readLine( buf, buflen );
	else
	    break;
    }
    return val;
}

static void read_pbm_image( QImageIO *iio )	// read PBM image data
{
    const int	buflen = 300;
    char	buf[buflen];
    QIODevice  *d = iio->ioDevice();
    int		w, h, nbits, mcc, y;
    int		pbm_bpl;
    char	type;
    bool	raw;
    QImage	image;

    if ( d->readBlock( buf, 3 ) != 3 )			// read P[1-6]<white-space>
	return;
    if ( !(buf[0] == 'P' && isdigit(buf[1]) && isspace(buf[2])) )
	return;
    switch ( (type=buf[1]) ) {
	case '1':				// ascii PBM
	case '4':				// raw PBM
	    nbits = 1;
	    break;
	case '2':				// ascii PGM
	case '5':				// raw PGM
	    nbits = 8;
	    break;
	case '3':				// ascii PPM
	case '6':				// raw PPM
	    nbits = 32;
	    break;
	default:
	    return;
    }
    raw = type >= '4';
    w = read_pbm_int( d );			// get image width
    h = read_pbm_int( d );			// get image height
    if ( nbits == 1 )
	mcc = 0;				// ignore max color component
    else
	mcc = read_pbm_int( d );		// get max color component
    if ( w <= 0 || w > 32767 || h <= 0 || h > 32767 || mcc < 0 || mcc > 32767 )
	return;					// weird P.M image

    int maxc = mcc;
    if ( maxc > 255 )
	maxc = 255;
    image.create( w, h, nbits, 0,
		  nbits == 1 ? QImage::BigEndian :  QImage::IgnoreEndian );
    if ( image.isNull() )
	return;

    pbm_bpl = (nbits*w+7)/8;			// bytes per scanline in PBM

    if ( raw ) {				// read raw data
	if ( nbits == 32 ) {			// type 6
	    pbm_bpl = 3*w;
	    uchar *buf24 = new uchar[pbm_bpl], *b;
	    QRgb  *p;
	    QRgb  *end;
	    for ( y=0; y<h; y++ ) {
		if ( d->readBlock( (char *)buf24, pbm_bpl ) != pbm_bpl ) {
		    delete[] buf24;
		    return;
		}
		p = (QRgb *)image.scanLine( y );
		end = p + w;
		b = buf24;
		while ( p < end ) {
		    *p++ = qRgb(b[0],b[1],b[2]);
		    b += 3;
		}
	    }
	    delete[] buf24;
	} else {				// type 4,5
	    for ( y=0; y<h; y++ ) {
		if ( d->readBlock( (char *)image.scanLine(y), pbm_bpl )
			!= pbm_bpl )
		    return;
	    }
	}
    } else {					// read ascii data
	register uchar *p;
	int n;
	for ( y=0; y<h; y++ ) {
	    p = image.scanLine( y );
	    n = pbm_bpl;
	    if ( nbits == 1 ) {
		int b;
		while ( n-- ) {
		    b = 0;
		    for ( int i=0; i<8; i++ )
			b = (b << 1) | (read_pbm_int(d) & 1);
		    *p++ = b;
		}
	    } else if ( nbits == 8 ) {
		if ( mcc == maxc ) {
		    while ( n-- ) {
			*p++ = read_pbm_int( d );
		    }
		} else {
		    while ( n-- ) {
			*p++ = read_pbm_int( d ) * maxc / mcc;
		    }
		}
	    } else {				// 32 bits
		n /= 4;
		int r, g, b;
		if ( mcc == maxc ) {
		    while ( n-- ) {
			r = read_pbm_int( d );
			g = read_pbm_int( d );
			b = read_pbm_int( d );
			*((QRgb*)p) = qRgb( r, g, b );
			p += 4;
		    }
		} else {
		    while ( n-- ) {
			r = read_pbm_int( d ) * maxc / mcc;
			g = read_pbm_int( d ) * maxc / mcc;
			b = read_pbm_int( d ) * maxc / mcc;
			*((QRgb*)p) = qRgb( r, g, b );
			p += 4;
		    }
		}
	    }
	}
    }

    if ( nbits == 1 ) {				// bitmap
	image.setNumColors( 2 );
	image.setColor( 0, qRgb(255,255,255) ); // white
	image.setColor( 1, qRgb(0,0,0) );	// black
    } else if ( nbits == 8 ) {			// graymap
	image.setNumColors( maxc+1 );
	for ( int i=0; i<=maxc; i++ )
	    image.setColor( i, qRgb(i*255/maxc,i*255/maxc,i*255/maxc) );
    }

    iio->setImage( image );
    iio->setStatus( 0 );			// image ok
}


static void write_pbm_image( QImageIO *iio )
{
    QIODevice* out = iio->ioDevice();
    QCString str;

    QImage  image  = iio->image();
    QCString format = iio->format();
    format = format.left(3);			// ignore RAW part
    bool gray = format == "PGM";

    if ( format == "PBM" ) {
	image = image.convertDepth(1);
    } else if ( image.depth() == 1 ) {
	image = image.convertDepth(8);
    }

    if ( image.depth() == 1 && image.numColors() == 2 ) {
	if ( qGray(image.color(0)) < qGray(image.color(1)) ) {
	    // 0=dark/black, 1=light/white - invert
	    image.detach();
	    for ( int y=0; y<image.height(); y++ ) {
		uchar *p = image.scanLine(y);
		uchar *end = p + image.bytesPerLine();
		while ( p < end )
		    *p++ ^= 0xff;
	    }
	}
    }

    uint w = image.width();
    uint h = image.height();

    str.sprintf("P\n%d %d\n", w, h);

    switch (image.depth()) {
	case 1: {
	    str.insert(1, '4');
	    if ((uint)out->writeBlock(str, str.length()) != str.length()) {
		iio->setStatus(1);
		return;
	    }
	    w = (w+7)/8;
	    for (uint y=0; y<h; y++) {
		uchar* line = image.scanLine(y);
		if ( w != (uint)out->writeBlock((char*)line, w) ) {
		    iio->setStatus(1);
		    return;
		}
	    }
	    }
	    break;

	case 8: {
	    str.insert(1, gray ? '5' : '6');
	    str.append("255\n");
	    if ((uint)out->writeBlock(str, str.length()) != str.length()) {
		iio->setStatus(1);
		return;
	    }
	    QRgb  *color = image.colorTable();
	    uint bpl = w*(gray ? 1 : 3);
	    uchar *buf   = new uchar[bpl];
	    for (uint y=0; y<h; y++) {
		uchar *b = image.scanLine(y);
		uchar *p = buf;
		uchar *end = buf+bpl;
		if ( gray ) {
		    while ( p < end ) {
			uchar g = (uchar)qGray(color[*b++]);
			*p++ = g;
		    }
		} else {
		    while ( p < end ) {
			QRgb rgb = color[*b++];
			*p++ = qRed(rgb);
			*p++ = qGreen(rgb);
			*p++ = qBlue(rgb);
		    }
		}
		if ( bpl != (uint)out->writeBlock((char*)buf, bpl) ) {
		    iio->setStatus(1);
		    return;
		}
	    }
	    delete [] buf;
	    }
	    break;

	case 32: {
	    str.insert(1, gray ? '5' : '6');
	    str.append("255\n");
	    if ((uint)out->writeBlock(str, str.length()) != str.length()) {
		iio->setStatus(1);
		return;
	    }
	    uint bpl = w*(gray ? 1 : 3);
	    uchar *buf = new uchar[bpl];
	    for (uint y=0; y<h; y++) {
		QRgb  *b = (QRgb*)image.scanLine(y);
		uchar *p = buf;
		uchar *end = buf+bpl;
		if ( gray ) {
		    while ( p < end ) {
			uchar g = (uchar)qGray(*b++);
			*p++ = g;
		    }
		} else {
		    while ( p < end ) {
			QRgb rgb = *b++;
			*p++ = qRed(rgb);
			*p++ = qGreen(rgb);
			*p++ = qBlue(rgb);
		    }
		}
		if ( bpl != (uint)out->writeBlock((char*)buf, bpl) ) {
		    iio->setStatus(1);
		    return;
		}
	    }
	    delete [] buf;
	    }
    }

    iio->setStatus(0);
}

#endif // QT_NO_IMAGEIO_PPM

#ifndef QT_NO_ASYNC_IMAGE_IO

class QImageIOFrameGrabber : public QImageConsumer {
public:
    QImageIOFrameGrabber() : framecount(0) { }

    QImageDecoder *decoder;
    int framecount;

    void changed(const QRect&) { }
    void end() { }
    void frameDone(const QPoint&, const QRect&) { framecount++; }
    void frameDone() { framecount++; }
    void setLooping(int) { }
    void setFramePeriod(int) { }
    void setSize(int, int) { }
};

static void read_async_image( QImageIO *iio )
{
    const int buf_len = 2048;
    uchar buffer[buf_len];
    QIODevice  *d = iio->ioDevice();
    QImageIOFrameGrabber* consumer = new QImageIOFrameGrabber();
    QImageDecoder decoder(consumer);
    consumer->decoder = &decoder;
    int startAt = d->at();
    int totLen = 0;

    for (;;) {
	int length = d->readBlock((char*)buffer, buf_len);
	if ( length <= 0 ) {
	    iio->setStatus(length);
	    break;
	}
	uchar* b = buffer;
	int r = -1;
	while (length > 0 && consumer->framecount==0) {
	    r = decoder.decode(b, length);
	    if ( r <= 0 ) break;
	    b += r;
	    totLen += r;
	    length -= r;
	}
	if ( consumer->framecount ) {
	    // Stopped after first frame
	    if ( d->isDirectAccess() )
		d->at( startAt + totLen );
	    else {
		// ### We have (probably) read too much from the stream into
		// the buffer, and there is no way to put it back!
	    }
	    iio->setImage(decoder.image());
	    iio->setStatus(0);
	    break;
	}
	if ( r <= 0 ) {
	    iio->setStatus(r);
	    break;
	}
    }

    delete consumer;
}

#endif // QT_NO_ASYNC_IMAGE_IO

#ifndef QT_NO_IMAGEIO_XBM

/*****************************************************************************
  X bitmap image read/write functions
 *****************************************************************************/

static inline int hex2byte( register char *p )
{
    return ((isdigit(*p)     ? *p     - '0' : toupper(*p)     - 'A' + 10)<< 4)|
	   ( isdigit(*(p+1)) ? *(p+1) - '0' : toupper(*(p+1)) - 'A' + 10);
}

static void read_xbm_image( QImageIO *iio )
{
    const int	buflen = 300;
    char	buf[buflen];
    QRegExp	r1, r2;
    QIODevice  *d = iio->ioDevice();
    int		i;
    int		w=-1, h=-1;
    QImage	image;

    r1 = QString::fromLatin1("^#define[\x20\t]+[a-zA-Z0-9_]+[\x20\t]+");
    r2 = QString::fromLatin1("[0-9]+");
    d->readLine( buf, buflen );			// "#define .._width <num>"
    QString sbuf;
    sbuf = QString::fromLatin1(buf);
    if ( r1.match(sbuf,0,&i)==0 && r2.match(sbuf,i)==i )
	w = atoi( &buf[i] );
    d->readLine( buf, buflen );			// "#define .._height <num>"
    sbuf = QString::fromLatin1(buf);
    if ( r1.match(sbuf,0,&i)==0 && r2.match(sbuf,i)==i )
	h = atoi( &buf[i] );
    if ( w <= 0 || w > 32767 || h <= 0 || h > 32767 )
	return;					// format error

    while ( TRUE ) {				// scan for data
	if ( d->readLine(buf, buflen) <= 0 )	// end of file
	    return;
	if ( strstr(buf,"0x") != 0 )		// does line contain data?
	    break;
    }

    image.create( w, h, 1, 2, QImage::LittleEndian );
    if ( image.isNull() )
	return;

    image.setColor( 0, qRgb(255,255,255) );	// white
    image.setColor( 1, qRgb(0,0,0) );		// black

    int	   x = 0, y = 0;
    uchar *b = image.scanLine(0);
    char  *p = strstr( buf, "0x" );
    w = (w+7)/8;				// byte width

    while ( y < h ) {				// for all encoded bytes...
	if ( p ) {				// p = "0x.."
	    *b++ = hex2byte(p+2);
	    p += 2;
	    if ( ++x == w && ++y < h ) {
		b = image.scanLine(y);
		x = 0;
	    }
	    p = strstr( p, "0x" );
	} else {				// read another line
	    if ( d->readLine(buf,buflen) <= 0 )	// EOF ==> truncated image
		break;
	    p = strstr( buf, "0x" );
	}
    }

    iio->setImage( image );
    iio->setStatus( 0 );			// image ok
}


static void write_xbm_image( QImageIO *iio )
{
    QIODevice *d = iio->ioDevice();
    QImage     image = iio->image();
    int	       w = image.width();
    int	       h = image.height();
    int	       i;
    QString    s = fbname(iio->fileName());	// get file base name
    char       buf[100];

    sprintf( buf, "#define %s_width %d\n", s.ascii(), w );
    d->writeBlock( buf, qstrlen(buf) );
    sprintf( buf, "#define %s_height %d\n", s.ascii(), h );
    d->writeBlock( buf, qstrlen(buf) );
    sprintf( buf, "static char %s_bits[] = {\n ", s.ascii() );
    d->writeBlock( buf, qstrlen(buf) );

    iio->setStatus( 0 );

    if ( image.depth() != 1 )
	image = image.convertDepth( 1 );	// dither
    if ( image.bitOrder() != QImage::LittleEndian )
	image = image.convertBitOrder( QImage::LittleEndian );

    bool invert = qGray(image.color(0)) < qGray(image.color(1));
    char hexrep[16];
    for ( i=0; i<10; i++ )
	hexrep[i] = '0' + i;
    for ( i=10; i<16; i++ )
	hexrep[i] = 'a' -10 + i;
    if ( invert ) {
	char t;
	for ( i=0; i<8; i++ ) {
	    t = hexrep[15-i];
	    hexrep[15-i] = hexrep[i];
	    hexrep[i] = t;
	}
    }
    int bcnt = 0;
    register char *p = buf;
    uchar *b = image.scanLine(0);
    int	 x=0, y=0;
    int nbytes = image.numBytes();
    w = (w+7)/8;
    while ( nbytes-- ) {			// write all bytes
	*p++ = '0';  *p++ = 'x';
	*p++ = hexrep[*b >> 4];
	*p++ = hexrep[*b++ & 0xf];
	if ( ++x == w && y < h-1 ) {
	    b = image.scanLine(++y);
	    x = 0;
	}
	if ( nbytes > 0 ) {
	    *p++ = ',';
	    if ( ++bcnt > 14 ) {
		*p++ = '\n';
		*p++ = ' ';
		*p   = '\0';
		d->writeBlock( buf, qstrlen(buf) );
		p = buf;
		bcnt = 0;
	    }
	}
    }
    strcpy( p, " };\n" );
    d->writeBlock( buf, qstrlen(buf) );
}

#endif // QT_NO_IMAGEIO_XBM


#ifndef QT_NO_IMAGEIO_XPM

/*****************************************************************************
  XPM image read/write functions
 *****************************************************************************/


// Skip until ", read until the next ", return the rest in *buf
// Returns FALSE on error, TRUE on success

static bool read_xpm_string( QCString &buf, QIODevice *d,
			     const char **source, int &index )
{
    if ( source ) {
	buf = source[index++];
	return TRUE;
    }

    if ( buf.size() < 69 )	    //# just an approximation
	buf.resize( 123 );

    buf[0] = '\0';
    int c;
    int i;
    while ( (c=d->getch()) != EOF && c != '"' ) { }
    if ( c == EOF ) {
	return FALSE;
    }
    i = 0;
    while ( (c=d->getch()) != EOF && c != '"' ) {
	if ( i == (int)buf.size() )
	    buf.resize( i*2+42 );
	buf[i++] = c;
    }
    if ( c == EOF ) {
	return FALSE;
    }

    if ( i == (int)buf.size() ) // always use a 0 terminator
	buf.resize( i+1 );
    buf[i] = '\0';
    return TRUE;
}


//
// INTERNAL
//
// Reads an .xpm from either the QImageIO or from the QString *.
// One of the two HAS to be 0, the other one is used.
//

static void read_xpm_image_or_array( QImageIO * iio, const char ** source,
				     QImage & image)
{
    QCString buf;
    QIODevice *d = 0;
    buf.resize( 200 );

    int i, cpp, ncols, w, h, index = 0;

    if ( iio ) {
	iio->setStatus( 1 );
	d = iio ? iio->ioDevice() : 0;
	d->readLine( buf.data(), buf.size() );	// "/* XPM */"
	QRegExp r (QString::fromLatin1("/\\*.XPM.\\*/"));
	if ( r.match(buf) < 0 )
	    return;					// bad magic
    } else if ( !source ) {
	return;
    }

    if ( !read_xpm_string( buf, d, source, index ) )
	return;

    if ( sscanf( buf, "%d %d %d %d", &w, &h, &ncols, &cpp ) < 4 )
	return;					// < 4 numbers parsed

    if ( cpp > 15 )
	return;

    if ( ncols > 256 ) {
	image.create( w, h, 32 );
    } else {
	image.create( w, h, 8, ncols );
    }

    QMap<QString, int> colorMap;
    int currentColor;

    for( currentColor=0; currentColor < ncols; ++currentColor ) {
	if ( !read_xpm_string( buf, d, source, index ) ) {
#if defined(CHECK_RANGE)
	    qWarning( "QImage: XPM color specification missing");
#endif
	    return;
	}
	QString index;
	index = buf.left( cpp );
	buf = buf.mid( cpp ).simplifyWhiteSpace().lower();
	buf.prepend( " " );
	i = buf.find( " c " );
	if ( i < 0 )
	    i = buf.find( " g " );
	if ( i < 0 )
	    i = buf.find( " g4 " );
	if ( i < 0 )
	    i = buf.find( " m " );
	if ( i < 0 ) {
#if defined(CHECK_RANGE)
	    qWarning( "QImage: XPM color specification is missing: %s", buf.data());
#endif
	    return;	// no c/g/g4/m specification at all
	}
	buf = buf.mid( i+3 );
	// Strip any other colorspec
	int end = buf.find(' ', 4);
	if ( end >= 0 )
	    buf.truncate(end);
	buf = buf.stripWhiteSpace();
	if ( buf == "none" ) {
	    image.setAlphaBuffer( TRUE );
	    int transparentColor = currentColor;
	    if ( image.depth() == 8 ) {
		image.setColor( transparentColor,
				RGB_MASK & qRgb(198,198,198) );
		colorMap.insert( index, transparentColor );
	    } else {
		QRgb rgb = RGB_MASK & qRgb(198,198,198);
		colorMap.insert( index, rgb );
	    }
	} else {
	    QColor c( buf.data() );
	    if ( image.depth() == 8 ) {
		image.setColor( currentColor, 0xff000000 | c.rgb() );
		colorMap.insert( index, currentColor );
	    } else {
		QRgb rgb = 0xff000000 | c.rgb();
		colorMap.insert( index, rgb );
	    }
	}
    }

    // Read pixels
    for( int y=0; y<h; y++ ) {
	if ( !read_xpm_string( buf, d, source, index ) ) {
#if defined(CHECK_RANGE)
	    qWarning( "QImage: XPM pixels missing on image line %d", y);
#endif
	    return;
	}
	if ( image.depth() == 8 ) {
	    uchar *p = image.scanLine(y);
	    uchar *d = (uchar *)buf.data();
	    uchar *end = d + buf.length();
	    int x;
	    if ( cpp == 1 ) {
		char b[2];
		b[1] = '\0';
		for ( x=0; x<w && d<end; x++ ) {
		    b[0] = *d++;
		    *p++ = (uchar)colorMap[b];
		}
	    } else {
		char b[16];
		b[cpp] = '\0';
		for ( x=0; x<w && d<end; x++ ) {
		    strncpy( b, (char *)d, cpp );
		    *p++ = (uchar)colorMap[b];
		    d += cpp;
		}
	    }
	} else {
	    QRgb *p = (QRgb*)image.scanLine(y);
	    uchar *d = (uchar *)buf.data();
	    uchar *end = d + buf.length();
	    int x;
	    char b[16];
	    b[cpp] = '\0';
	    for ( x=0; x<w && d<end; x++ ) {
		strncpy( b, (char *)d, cpp );
		*p++ = (QRgb)colorMap[b];
		d += cpp;
	    }
	}
    }
    if ( iio ) {
	iio->setImage( image );
	iio->setStatus( 0 );			// image ok
    }
}


static void read_xpm_image( QImageIO * iio )
{
    QImage i;
    (void)read_xpm_image_or_array( iio, 0, i );
    return;
}


static const char* xpm_color_name( int cpp, int index )
{
    static char returnable[5];
    static const char code[] = ".#abcdefghijklmnopqrstuvwxyzABCD"
			       "EFGHIJKLMNOPQRSTUVWXYZ0123456789";
    // cpp is limited to 4 and index is limited to 64^cpp
    if ( cpp > 1 ) {
	if ( cpp > 2 ) {
	    if ( cpp > 3 ) {
		returnable[3] = code[index % 64];
		index /= 64;
	    } else
		returnable[3] = '\0';
	    returnable[2] = code[index % 64];
	    index /= 64;
	} else
	    returnable[2] = '\0';
	// the following 4 lines are a joke!
	if ( index == 0 )
	    index = 64*44+21;
	else if ( index == 64*44+21 )
	    index = 0;
	returnable[1] = code[index % 64];
	index /= 64;
    } else
	returnable[1] = '\0';
    returnable[0] = code[index];

    return returnable;
}


// write XPM image data
static void write_xpm_image( QImageIO * iio )
{
    if ( iio )
	iio->setStatus( 1 );
    else
	return;

    // ### 8-bit case could be made faster
    QImage image;
    if ( iio->image().depth() != 32 )
	image = iio->image().convertDepth( 32 );
    else
	image = iio->image();

    QMap<QRgb, int> colorMap;

    int w = image.width(), h = image.height(), ncolors = 0;
    int x, y;

    // build color table
    for( y=0; y<h; y++ ) {
	QRgb * yp = (QRgb *)image.scanLine( y );
	for( x=0; x<w; x++ ) {
	    QRgb color = *(yp + x);
	    if ( !colorMap.contains(color) )
		colorMap.insert( color, ncolors++ );
	}
    }

    // number of 64-bit characters per pixel needed to encode all colors
    int cpp = 1;
    for ( int k = 64; ncolors > k; k *= 64 ) {
	++cpp;
	// limit to 4 characters per pixel
 	// 64^4 colors is enough for a 4096x4096 image
         if ( cpp > 4)
	    break;
    }

    QString line;

    // write header
    QTextStream s( iio->ioDevice() );
    s << "/* XPM */" << endl
      << "static char *" << fbname(iio->fileName()) << "[]={" << endl
      << "\"" << w << " " << h << " " << ncolors << " " << cpp << "\"";

    // write palette
    QMap<QRgb, int>::Iterator c = colorMap.begin();
    while ( c != colorMap.end() ) {
	QRgb color = c.key();
	if ( image.hasAlphaBuffer() && color == (color & RGB_MASK) )
	    line.sprintf( "\"%s c None\"",
			  xpm_color_name(cpp, *c) );
	else
	    line.sprintf( "\"%s c #%02x%02x%02x\"",
			  xpm_color_name(cpp, *c),
			  qRed(color),
			  qGreen(color),
			  qBlue(color) );
	++c;
	s << "," << endl << line;
    }

    // write pixels, limit to 4 characters per pixel
    line.truncate( cpp*w );
    for( y=0; y<h; y++ ) {
	QRgb * yp = (QRgb *) image.scanLine( y );
	int cc = 0;
	for( x=0; x<w; x++ ) {
	    int color = (int)(*(yp + x));
	    QCString chars = xpm_color_name( cpp, colorMap[color] );
	    line[cc++] = chars[0];
	    if ( cpp > 1 ) {
		line[cc++] = chars[1];
		if ( cpp > 2 ) {
		    line[cc++] = chars[2];
		    if ( cpp > 3 ) {
			line[cc++] = chars[3];
		    }
		}
	    }
	}
	s << "," << endl << "\"" << line << "\"";
    }
    s << "};" << endl;

    iio->setStatus( 0 );
}

#endif // QT_NO_IMAGEIO_XPM

/*!
  Note:  currently no closest-color search is made.  If colors are found that
  are not in the palette, the palette may not be used at all.  This result
  should not be considered valid, as it may change in future implementations.

  Currently inefficient for non 32-bit images.
*/
#ifndef QT_NO_IMAGE_TRUECOLOR
QImage QImage::convertDepthWithPalette( int d, QRgb* palette, int palette_count, int conversion_flags ) const
{
    if ( depth() == 1 ) {
	return convertDepth( 8, conversion_flags )
	       .convertDepthWithPalette( d, palette, palette_count, conversion_flags );
    } else if ( depth() == 8 ) {
	// ### this could be easily made more efficient
	return convertDepth( 32, conversion_flags )
	       .convertDepthWithPalette( d, palette, palette_count, conversion_flags );
    } else {
	QImage result;
	convert_32_to_8( this, &result,
	    (conversion_flags&~Qt::DitherMode_Mask) | Qt::AvoidDither,
	    palette, palette_count );
	return result.convertDepth( d );
    }
}
#endif
static
bool
haveSamePalette(const QImage& a, const QImage& b)
{
    if (a.depth() != b.depth()) return FALSE;
    if (a.numColors() != b.numColors()) return FALSE;
    QRgb* ca = a.colorTable();
    QRgb* cb = b.colorTable();
    for (int i=a.numColors(); i--; ) {
	if (*ca++ != *cb++) return FALSE;
    }
    return TRUE;
}

/*!
  Copies a \a sw by \a sh pixel area from \a src to position (\a dx, \a dy)
  in \a dst.  The pixels copied from source (src) are converted according
  to \a conversion_flags if it is incompatible with the destination (dst).

  The copying is clipped if areas outside \a src or \a dst are specified.

  If \a sw is -1, it is adjusted to src->width().
  Similarly, if \a sh is -1, it is adjusted to src->height().

  Currently inefficient for non 32-bit images.
*/
void bitBlt( QImage* dst, int dx, int dy, const QImage* src,
		int sx, int sy, int sw, int sh, int conversion_flags )
{
    // Parameter correction
    if ( sw < 0 ) sw = src->width();
    if ( sh < 0 ) sh = src->height();
    if ( sx < 0 ) { dx -= sx; sw += sx; sx = 0; }
    if ( sy < 0 ) { dy -= sy; sh += sy; sy = 0; }
    if ( dx < 0 ) { sx -= dx; sw += dx; dx = 0; }
    if ( dy < 0 ) { sy -= dy; sh += dy; dy = 0; }
    if ( sx + sw > src->width() ) sw = src->width() - sx;
    if ( sy + sh > src->height() ) sh = src->height() - sy;
    if ( dx + sw > dst->width() ) sw = dst->width() - dx;
    if ( dy + sh > dst->height() ) sh = dst->height() - dy;
    if ( sw <= 0 || sh <= 0 ) return; // Nothing left to copy
    if ( (dst->data == src->data) && dx==sx && dy==sy ) return; // Same pixels

    // "Easy" to copy if both same depth and one of:
    //   - 32 bit
    //   - 8 bit, identical palette
    //   - 1 bit, identical palette and byte-aligned area
    //
    if ( haveSamePalette(*dst,*src)
	 && ( dst->depth() != 1 ||
	      !( (dx&7) || (sx&7) ||
		 ((sw&7) && (sx+sw < src->width()) ||
		  (dx+sw < dst->width()) ) ) ) )
	{
	    // easy to copy
	} else if ( dst->depth() != 32 ) {
#ifndef QT_NO_IMAGE_TRUECOLOR

	    QImage dstconv = dst->convertDepth( 32 );
	    bitBlt( &dstconv, dx, dy, src, sx, sy, sw, sh,
		    (conversion_flags&~Qt::DitherMode_Mask) | Qt::AvoidDither );
	    *dst = dstconv.convertDepthWithPalette( dst->depth(),
						    dst->colorTable(), dst->numColors() );
#endif
	    return;
	}

    // Now assume palette can be ignored

    if ( dst->depth() != src->depth() ) {
	if ( sw == src->width() && sh == src->height() || dst->depth()==32 ) {
	    QImage srcconv = src->convertDepth( dst->depth(), conversion_flags );
	    bitBlt( dst, dx, dy, &srcconv, sx, sy, sw, sh, conversion_flags );
	} else {
	    QImage srcconv = src->copy( sx, sy, sw, sh ); // ie. bitBlt
	    bitBlt( dst, dx, dy, &srcconv, 0, 0, sw, sh, conversion_flags );
	}
	return;
    }

    // Now assume both are the same depth.

    // Now assume both are 32-bit or 8-bit with compatible palettes.

    // "Easy"

    switch ( dst->depth() ) {
    case 1:
	{
	    uchar* d = dst->scanLine(dy) + dx/8;
	    uchar* s = src->scanLine(sy) + sx/8;
	    const int bw = (sw+7)/8;
	    if ( bw < 64 ) {
		// Trust ourselves
		const int dd = dst->bytesPerLine() - bw;
		const int ds = src->bytesPerLine() - bw;
		while ( sh-- ) {
		    for ( int t=bw; t--; )
			*d++ = *s++;
		    d += dd;
		    s += ds;
		}
	    } else {
		// Trust libc
		const int dd = dst->bytesPerLine();
		const int ds = src->bytesPerLine();
		while ( sh-- ) {
		    memcpy( d, s, bw );
		    d += dd;
		    s += ds;
		}
	    }
	}
	break;
    case 8:
	{
	    uchar* d = dst->scanLine(dy) + dx;
	    uchar* s = src->scanLine(sy) + sx;
	    if ( sw < 64 ) {
		// Trust ourselves
		const int dd = dst->bytesPerLine() - sw;
		const int ds = src->bytesPerLine() - sw;
		while ( sh-- ) {
		    for ( int t=sw; t--; )
			*d++ = *s++;
		    d += dd;
		    s += ds;
		}
	    } else {
		// Trust libc
		const int dd = dst->bytesPerLine();
		const int ds = src->bytesPerLine();
		while ( sh-- ) {
		    memcpy( d, s, sw );
		    d += dd;
		    s += ds;
		}
	    }
	}
	break;
#ifndef QT_NO_IMAGE_TRUECOLOR
    case 32:
	{
	    QRgb* d = (QRgb*)dst->scanLine(dy) + dx;
	    QRgb* s = (QRgb*)src->scanLine(sy) + sx;
	    if ( sw < 64 ) {
		// Trust ourselves
		const int dd = dst->width() - sw;
		const int ds = src->width() - sw;
		while ( sh-- ) {
		    for ( int t=sw; t--; )
			*d++ = *s++;
		    d += dd;
		    s += ds;
		}
	    } else {
		// Trust libc
		const int dd = dst->width();
		const int ds = src->width();
		const int b = sw*sizeof(QRgb);
		while ( sh-- ) {
		    memcpy( d, s, b );
		    d += dd;
		    s += ds;
		}
	    }
	}
	break;
#endif // QT_NO_IMAGE_TRUECOLOR	
    }
}


/*!  Returns TRUE if this image and \a i have the same contents, and
  FALSE if they differ.  This can be slow.  Of course, this function
  returns quickly if e.g. the two images' widths are different.

  \sa operator=()
*/

bool QImage::operator==( const QImage & i ) const
{
    // same object, or shared?
    if ( i.data == data )
	return TRUE;
    // obviously different stuff?
    if ( i.data->h != data->h ||
	 i.data->w != data->w )
	return FALSE;
    // that was the fast bit...
    QImage i1 = convertDepth( 32 );
    QImage i2 = i.convertDepth( 32 );
    int l;
    for( l=0; l < data->h; l++ )
	if ( memcmp( i1.scanLine( l ), i2.scanLine( l ), 4*data->w ) )
	    return FALSE;
    return TRUE;
}


/*!  Returns TRUE if this image and \a i have different contents, and
  FALSE if they they have the same.  This can be slow.  Of course,
  this function returns quickly if e.g. the two images' widths are
  different.

  \sa operator=()
*/

bool QImage::operator!=( const QImage & i ) const
{
    return !(*this == i);
}




/*!
    \fn int QImage::dotsPerMeterX() const

    Returns the number of pixels that fit horizontally in a physical meter.
    This and dotsPerMeterY() define the intended scale and aspect ratio
    of the image.

    \sa setDotsPerMeterX()
*/

/*!
    \fn int QImage::dotsPerMeterY() const

    Returns the number of pixels that fit vertically in a physical meter.
    This and dotsPerMeterX() define the intended scale and aspect ratio
    of the image.

    \sa setDotsPerMeterY()
*/

/*!
    Sets the value returned by dotsPerMeterX() to \a x.
*/
void QImage::setDotsPerMeterX(int x)
{
    data->dpmx = x;
}

/*!
    Sets the value returned by dotsPerMeterY() to \a y.
*/
void QImage::setDotsPerMeterY(int y)
{
    data->dpmy = y;
}

/*!
    \fn QPoint QImage::offset() const

    Returns the number of pixels by which the image is intended to by
    offset by when positioning relative to other images.
*/

/*!
    Sets the value returned by offset() to \a p.
*/
void QImage::setOffset(const QPoint& p)
{
    data->offset = p;
}
#ifndef QT_NO_IMAGE_TEXT
/*!
    Returns the internal QImageDataMisc object, possibly creating it.
*/
QImageDataMisc& QImage::misc() const
{
    if ( !data->misc ) {
	QImage* that = (QImage*)this;
	that->data->misc = new QImageDataMisc;
	return *that->data->misc;
    }
    return *data->misc;
}

/*!
    Returns the string recorded for the keyword \a key in language \a lang,
    or in a default language if \a lang is 0.
*/
QString QImage::text(const char* key, const char* lang) const
{
    QImageTextKeyLang x(key,lang);
    return misc().text_lang[x];
}

/*!
    Returns the string recorded for the keyword and language \a kl,
    or in a default language if \a lang is 0.
*/
QString QImage::text(const QImageTextKeyLang& kl) const
{
    return misc().text_lang[kl];
}

/*!
    Returns the language identifiers for which some texts are recorded.

    \sa textList(), text(), setText(), textKeys()
*/
QStringList QImage::textLanguages() const
{
    if ( !data->misc )
	return QStringList();
    return misc().languages();
}

/*!
    Returns the keywords for which some texts are recorded.

    \sa textList(), text(), setText(), textLanguages()
*/
QStringList QImage::textKeys() const
{
    if ( !data->misc )
	return QStringList();
    return misc().keys();
}

/*!
    Returns a list of QImageTextKeyLang objects which enumerate
    all the texts key/languaage pairs set by setText() for this image.
*/
QValueList<QImageTextKeyLang> QImage::textList() const
{
    if ( !data->misc )
	return QValueList<QImageTextKeyLang>();
    return misc().list();
}

/*!
    Records \a s for the keyword \a key.  The \a key should be a portable
    keyword recognizable by other software - some suggested values can
    be found in
    <a href="http://www.cdrom.com/pub/png/spec/PNG-Chunks.html#C.tEXt">
    the PNG specification</a>.  \a s can be any text.  \a lang should
    specify the language code (see
    <a href="ftp://ftp.isi.edu/in-notes/1766">RFC 1766</a>) or 0.

    Note that no QImageIO handlers currently read or write this
    data.  In an upcoming Qt version, PNG I/O will do so.
*/
void QImage::setText(const char* key, const char* lang, const QString& s)
{
    QImageTextKeyLang x(key,lang);
    misc().text_lang.replace(x,s);
}

#endif // QT_NO_IMAGE_TEXT

#ifdef _WS_QWS_
QGfx * QImage::graphicsContext()
{
    QGfx * ret=0;
    if(depth()) {
	int w = qt_screen->mapToDevice( QSize(width(),height()) ).width();
	int h = qt_screen->mapToDevice( QSize(width(),height()) ).height();
	ret=QGfx::createGfx(depth(),bits(),w,h,bytesPerLine());
    } else {
	qDebug("Trying to create image for null depth");
	return 0;
    }
    if(depth()<=8) {
	QRgb * tmp=colorTable();
	int nc=numColors();
	if(tmp==0) {
	    static QRgb table[2] = { qRgb(255,255,255), qRgb(0,0,0) };
	    tmp=table;
	    nc=2;
	}
	ret->setClut(tmp,nc);
    }
    return ret;
}

#endif

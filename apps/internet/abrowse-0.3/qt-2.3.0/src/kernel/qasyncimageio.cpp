/****************************************************************************
** $Id$
**
** Implementation of asynchronous image/movie loading classes
**
** Created : 970617
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

#include "qasyncimageio.h"

#ifndef QT_NO_ASYNC_IMAGE_IO

#include "qpainter.h"
#include "qlist.h"
#include "qgif.h"
#include <stdlib.h>

extern void qt_init_image_handlers();


// NOT REVISED
/*!
  \class QImageConsumer qasyncimageio.h
  \brief An abstraction used by QImageDecoder.

  \ingroup images

  A QImageConsumer consumes information about changes to the QImage
  maintained by a QImageDecoder.  It represents the a view of the
  image which the decoder produces.

  \sa QImageDecoder
*/

/*!
  \fn void QImageConsumer::changed(const QRect&)

  Called when the given area of the image has changed.
*/

/*!
  \fn void QImageConsumer::end()

  Called when all data of all frames has been decoded and revealed
  as changed().
*/

/*!
  \fn void QImageConsumer::frameDone()

  One of the two frameDone() functions will be called
  when a frame of an animated image has ended and been revealed
  as changed().

  When this function is called, the current image should be displayed.

  The decoder will not make
  any further changes to the image until the next call to
  QImageFormat::decode().
*/

/*!
  \fn void QImageConsumer::frameDone( const QPoint& offset, const QRect& rect )

  One of the two frameDone() functions will be called
  when a frame of an animated image has ended and been revealed
  as changed().

  When this function is called, the area \a rect in the current image
  should be moved by \a offset and displayed.

  The decoder will not make
  any further changes to the image until the next call to
  QImageFormat::decode().
*/

/*!
  \fn void QImageConsumer::setLooping(int n)

  Called to indicate that the sequence of frames in the image
  should be repeated \a n times, including the sequence during
  decoding.

  <ul>
    <li> 0 = Forever
    <li> 1 = Only display frames the first time through
    <li> 2 = Repeat once after first pass through images
    <li> etc.
  </ul>

  To make the QImageDecoder
  do this just delete it and pass the information to it again
  for decoding (setLooping() will be called again of course, but
  that can be ignored), or keep copies of the
  changed areas at the ends of frames.
*/

/*!
  \fn void QImageConsumer::setFramePeriod(int milliseconds)

  Notes that the frame about to be decoded should not be displayed until
  the given number of \a milliseconds after the time that this function
  is called.  Of course, the image may not have been decoded by then, in
  which case the frame should not be displayed until it is complete.
  A value of -1 (the assumed default) indicates that the image should
  be displayed even while it is only partially loaded.
*/

/*!
  \fn void QImageConsumer::setSize(int, int)

  This function is called as soon as the size of the image has
  been determined.
*/


/*!
  \class QImageDecoder qasyncimageio.h
  \brief Incremental image decoder for all supported image formats.

  \ingroup images

  New formats are installed by creating objects of class
  QImageFormatType, and the QMovie class can be used for using
  all installed incremental image formats; QImageDecoder is only
  useful for creating new ways of feeding data to an QImageConsumer.
  
  \mustquote

  Qt supports GIF reading, if it is configured that way during
  installation (see qgif.h). If it is, we are required to state that
  "The Graphics Interchange Format(c) is the Copyright property of
  CompuServe Incorporated. GIF(sm) is a Service Mark property of
  CompuServe Incorporated."
*/

static const int max_header = 32;





// See qgif.h for important information regarding this option
#if defined(QT_BUILTIN_GIF_READER) && QT_BUILTIN_GIF_READER == 1
class Q_EXPORT QGIFFormat : public QImageFormat {
public:
    QGIFFormat();
    virtual ~QGIFFormat();

    int decode(QImage& img, QImageConsumer* consumer,
	    const uchar* buffer, int length);

private:
    void fillRect(QImage&, int x, int y, int w, int h, uchar col);

    // GIF specific stuff
    QRgb* globalcmap;
    QImage backingstore;
    unsigned char hold[16];
    bool gif89;
    int count;
    int ccount;
    int expectcount;
    enum State {
	Header,
	LogicalScreenDescriptor,
	GlobalColorMap,
	LocalColorMap,
	Introducer,
	ImageDescriptor,
	TableImageLZWSize,
	ImageDataBlockSize,
	ImageDataBlock,
	ExtensionLabel,
	GraphicControlExtension,
	ApplicationExtension,
	NetscapeExtensionBlockSize,
	NetscapeExtensionBlock,
	SkipBlockSize,
	SkipBlock,
	Done,
	Error
    } state;
    int gncols;
    int ncols;
    int lzwsize;
    bool lcmap;
    int swidth, sheight;
    int left, top, right, bottom;
    enum Disposal { NoDisposal, DoNotChange, RestoreBackground, RestoreImage };
    Disposal disposal;
    bool disposed;
    int trans;
    int trans_index;
    bool preserve_trans;
    bool gcmap;
    int bgcol;
    int interlace;
    int accum;
    int bitcount;

    enum { max_lzw_bits=12 }; // (poor-compiler's static const int)

    int code_size, clear_code, end_code, max_code_size, max_code;
    int firstcode, oldcode, incode;
    short table[2][1<< max_lzw_bits];
    short stack[(1<<(max_lzw_bits))*2];
    short *sp;
    bool needfirst;
    int x, y;
    int frame;
    bool out_of_bounds;
    bool digress;
    void nextY(QImage& img, QImageConsumer* consumer);
    void disposePrevious( QImage& img, QImageConsumer* consumer );
};

class Q_EXPORT QGIFFormatType : public QImageFormatType
{
    QImageFormat* decoderFor(const uchar* buffer, int length);
    const char* formatName() const;
};

#endif


struct QImageDecoderPrivate {
    QImageDecoderPrivate()
    {
	count = 0;
    }

    static void cleanup();

    static void ensureFactories()
    {
	if ( !factories ) {
	    factories = new QList<QImageFormatType>;
// See qgif.h for important information regarding this option
#if defined(QT_BUILTIN_GIF_READER) && QT_BUILTIN_GIF_READER == 1
	    gif_decoder_factory = new QGIFFormatType;
#endif
	    qAddPostRoutine( cleanup );
	}
    }

    static QList<QImageFormatType> * factories;

// See qgif.h for important information regarding this option
#if defined(QT_BUILTIN_GIF_READER) && QT_BUILTIN_GIF_READER == 1
    static QGIFFormatType * gif_decoder_factory;
#endif

    uchar header[max_header];
    int count;
};

QList<QImageFormatType> * QImageDecoderPrivate::factories = 0;
// See qgif.h for important information regarding this option
#if defined(QT_BUILTIN_GIF_READER) && QT_BUILTIN_GIF_READER == 1
QGIFFormatType * QImageDecoderPrivate::gif_decoder_factory = 0;
#endif


void QImageDecoderPrivate::cleanup()
{
    delete factories;
    factories = 0;
// See qgif.h for important information regarding this option
#if defined(QT_BUILTIN_GIF_READER) && QT_BUILTIN_GIF_READER == 1
    delete gif_decoder_factory;
    gif_decoder_factory = 0;
#endif
}


/*!
  Constructs a QImageDecoder which will send change information to
  a given QImageConsumer.
*/
QImageDecoder::QImageDecoder(QImageConsumer* c)
{
    qt_init_image_handlers();
    d = new QImageDecoderPrivate;
    CHECK_PTR(d);
    consumer = c;
    actual_decoder = 0;
}

/*!
  Destroys a QImageDecoder.  The image it built is destroyed.  The decoder
  built by the factory for the file format is destroyed. The consumer
  for which it decoded the image is \e not destroyed.
*/
QImageDecoder::~QImageDecoder()
{
    delete d;
    delete actual_decoder;
}

/*!
  \fn const QImage& QImageDecoder::image()

  Returns the image currently being decoded.
*/

/*!
  Call this function to decode some data into image changes.  The data
  will be decoded, sending change information to the QImageConsumer of
  this QImageDecoder, until one of the change functions of the consumer
  returns FALSE.

  Returns the number of bytes consumed, 0 if consumption is complete,
  and -1 if decoding fails due to invalid data.
*/
int QImageDecoder::decode(const uchar* buffer, int length)
{
    if (!actual_decoder) {
	int i=0;

	while (i < length && d->count < max_header)
	    d->header[d->count++] = buffer[i++];

	QImageDecoderPrivate::ensureFactories();

	for (QImageFormatType* f = QImageDecoderPrivate::factories->first();
	    f && !actual_decoder;
	    f = QImageDecoderPrivate::factories->next())
	{
	    actual_decoder = f->decoderFor(d->header, d->count);
	}

	if (!actual_decoder) {
	    if ( d->count < max_header ) {
		// not enough info yet
		return i;
	    } else {
		// failure - nothing matches max_header bytes
		return -1;
	    }
	}
    }
    return actual_decoder->decode(img, consumer, buffer, length);
}

/*!
  Returns a QImageFormatType by name. This might be used in cases where
  the user needs to force data to be interpreted as being in a certain
  format.  \a name is one of the formats listed by
  QImageDecoder::inputFormats(). Note that you will still need to supply
  decodable data to result->decoderFor() before you can begin decoding
  the data.
*/
QImageFormatType* QImageDecoder::format( const char* name )
{
    for (QImageFormatType* f = QImageDecoderPrivate::factories->first();
	f;
	f = QImageDecoderPrivate::factories->next())
    {
	if ( qstricmp(name,f->formatName())==0 )
	    return f;
    }
    return 0;
}

/*!
  Call this function to find the name of the format of the given header.
  The returned string is statically allocated.

  Returns 0 if the format is not recognized.
*/
const char* QImageDecoder::formatName(const uchar* buffer, int length)
{
    QImageDecoderPrivate::ensureFactories();

    const char* name = 0;
    for (QImageFormatType* f = QImageDecoderPrivate::factories->first();
	f && !name;
	f = QImageDecoderPrivate::factories->next())
    {
	QImageFormat *decoder = f->decoderFor(buffer, length);
	if (decoder) {
	    name = f->formatName();
	    delete decoder;
	}
    }
    return name;
}

/*!
  Returns a sorted list of formats for which asynchronous loading is supported.
*/
QStrList QImageDecoder::inputFormats()
{
    QImageDecoderPrivate::ensureFactories();

    QStrList result;

    for (QImageFormatType* f = QImageDecoderPrivate::factories->first();
	 f;
	 f = QImageDecoderPrivate::factories->next())
    {
	if ( !result.contains(  f->formatName() ) ) {
	    result.inSort(  f->formatName() );
	}
    }

    return result;
}

/*!
  Registers a new QImageFormatType.  This is not needed in
  application code as factories call this themselves.
*/
void QImageDecoder::registerDecoderFactory(QImageFormatType* f)
{
    QImageDecoderPrivate::ensureFactories();

    QImageDecoderPrivate::factories->insert(0,f);
}

/*!
  Unregisters a new QImageFormatType.  This is not needed in
  application code as factories call this themselves.
*/
void QImageDecoder::unregisterDecoderFactory(QImageFormatType* f)
{
    if ( !QImageDecoderPrivate::factories )
	return;

    QImageDecoderPrivate::factories->remove(f);
}

/*!
  \class QImageFormat qasyncimageio.h
  \brief Incremental image decoder for a specific image format.

  \ingroup images

  By making a derived classes of QImageFormatType which in turn
  creates objects that are a subclass of QImageFormat, you can add
  support for more incremental image formats, allowing such formats to
  be sources for a QMovie, or for the first frame of the image stream
  to be loaded as a QImage or QPixmap.

  Your new subclass must reimplement the decode() function in order to
  process your new format.

  New QImageFormat objects are generated by new QImageFormatType factories.
*/

/*!
  Destructs the object.

  \internal
  More importantly, destructs derived classes.
*/
QImageFormat::~QImageFormat()
{
}

/*!
  \fn int QImageFormat::decode(QImage& img, QImageConsumer* consumer,
	    const uchar* buffer, int length)

  New subclasses must reimplement this method.

  It should decode some or all of the bytes from \a buffer into
  \a img, calling the methods of \a consumer as the decoding proceeds to
  inform that consumer of changes to the image.
  The consumer may be 0, in which case the function should just process
  the data into \a img without telling any consumer about the changes.
  Note that the decoder must store enough state
  to be able to continue in subsequent calls to this method - this is
  the essence of the incremental image loading.

  The function should return without processing all the data if it
  reaches the end of a frame in the input.

  The function must return the number of bytes it has processed.
*/

/*!
  \class QImageFormatType qasyncimageio.h
  \brief Factory that makes QImageFormat objects.

  \ingroup images

  While the QImageIO class allows for \e complete loading of images,
  QImageFormatType allows for \e incremental loading of images.

  New image file formats are installed by creating objects of derived
  classes of QImageFormatType.  They must implement decoderFor()
  and formatName().

  QImageFormatType is a very simple class.  Its only task is to
  recognize image data in some format and make a new object, subclassed
  from QImageFormat, which can decode that format.

  The factories for formats built into Qt
  are automatically defined before any other factory is initialized.
  If two factories would recognize an image format, the factory created
  last will override the earlier one, thus you can override current
  and future built-in formats.
*/

/*!
  \fn virtual QImageFormat* QImageFormatType::decoderFor(const
	    uchar* buffer, int length)

  Returns a decoder for decoding an image which starts with the give bytes.
  This function should only return a decoder if it is definite that the
  decoder applies to data with the given header.  Returns 0 if there is
  insufficient data in the header to make a positive identification,
  or if the data is not recognized.
*/

/*!
  \fn virtual const char* QImageFormatType::formatName() const

  Returns the name of the format supported by decoders from this factory.
  The string is statically allocated.
*/

/*!
  Constructs a factory.  It automatically registers itself with QImageDecoder.
*/
QImageFormatType::QImageFormatType()
{
    QImageDecoder::registerDecoderFactory(this);
}

/*!
  Destroys a factory.  It automatically unregisters itself from QImageDecoder.
*/
QImageFormatType::~QImageFormatType()
{
    QImageDecoder::unregisterDecoderFactory(this);
}


/*!
  Returns TRUE if Qt was compiled with built-in GIF reading support,
  otherwise FALSE.
*/
bool qt_builtin_gif_reader()
{
#if defined(QT_BUILTIN_GIF_READER)
    return QT_BUILTIN_GIF_READER == 1;
#else
    return 0;
#endif
}

// See qgif.h for important information regarding this option
#if defined(QT_BUILTIN_GIF_READER) && QT_BUILTIN_GIF_READER == 1

/* -- NOTDOC
  \class QGIFFormat qasyncimageio.h
  \brief Incremental image decoder for GIF image format.

  \ingroup images

  This subclass of QImageFormat decodes GIF format images,
  including animated GIFs.  Internally in
*/

/*!
  Constructs a QGIFFormat.
*/
QGIFFormat::QGIFFormat()
{
    globalcmap = 0;
    disposal = NoDisposal;
    out_of_bounds = FALSE;
    disposed = TRUE;
    frame = -1;
    state = Header;
    count = 0;
    lcmap = FALSE;
}

/*!
  Destructs a QGIFFormat.
*/
QGIFFormat::~QGIFFormat()
{
    if (globalcmap) delete[] globalcmap;
}


/* -- NOTDOC
  \class QGIFFormatType qasyncimageio.h
  \brief Incremental image decoder for GIF image format.

  \ingroup images

  This subclass of QImageFormatType recognizes GIF
  format images, creating a QGIFFormat when required.  An instance
  of this class is created automatically before any other factories,
  so you should have no need for such objects.
*/

QImageFormat* QGIFFormatType::decoderFor(
    const uchar* buffer, int length)
{
    if (length < 6) return 0;
    if (buffer[0]=='G'
     && buffer[1]=='I'
     && buffer[2]=='F'
     && buffer[3]=='8'
     && (buffer[4]=='9' || buffer[4]=='7')
     && buffer[5]=='a')
	return new QGIFFormat;
    return 0;
}

const char* QGIFFormatType::formatName() const
{
    return "GIF";
}


void QGIFFormat::disposePrevious( QImage& img, QImageConsumer* consumer )
{
    if ( out_of_bounds ) // flush anything that survived
	consumer->changed(QRect(0,0,swidth,sheight));

    // Handle disposal of previous image before processing next one

    if ( disposed ) return;

    int l = QMIN(swidth-1,left);
    int r = QMIN(swidth-1,right);
    int t = QMIN(sheight-1,top);
    int b = QMIN(sheight-1,bottom);

    switch (disposal) {
      case NoDisposal:
	break;
      case DoNotChange:
	break;
      case RestoreBackground:
	preserve_trans = FALSE;
	if (trans_index>=0) {
	    // Easy:  we use the transparent color
	    fillRect(img, l, t, r-l+1, b-t+1, trans_index);
	} else if (bgcol>=0) {
	    // Easy:  we use the bgcol given
	    fillRect(img, l, t, r-l+1, b-t+1, bgcol);
	} else {
	    // Impossible:  We don't know of a bgcol - use pixel 0
	    uchar** line = img.jumpTable();
	    fillRect(img, l, t, r-l+1, b-t+1, line[0][0]);
	}
	if (consumer)
	    consumer->changed(QRect(l, t, r-l+1, b-t+1));
	break;
      case RestoreImage: {
	uchar** line = img.jumpTable();
	preserve_trans = FALSE;
	for (int ln=t; ln<=b; ln++) {
	    memcpy(line[ln]+l,
		backingstore.scanLine(ln-t),
		r-l+1);
	}
	consumer->changed(QRect(l, t, r-l+1, b-t+1));
      }
    }
    disposal = NoDisposal; // Until an extension says otherwise.

    disposed = TRUE;
}

/*!
  This function decodes some data into image changes.

  Returns the number of bytes consumed.
*/
int QGIFFormat::decode(QImage& img, QImageConsumer* consumer,
	const uchar* buffer, int length)
{
    // We are required to state that
    //    "The Graphics Interchange Format(c) is the Copyright property of
    //    CompuServe Incorporated. GIF(sm) is a Service Mark property of
    //    CompuServe Incorporated."

#define LM(l, m) (((m)<<8)|l)
    digress = FALSE;
    int initial = length;
    uchar** line = img.jumpTable();
    while (!digress && length) {
	length--;
	unsigned char ch=*buffer++;
	switch (state) {
	  case Header:
	    hold[count++]=ch;
	    if (count==6) {
		// Header
		gif89=(hold[3]!='8' || hold[4]!='7');
		state=LogicalScreenDescriptor;
		count=0;
	    }
	    break;
	  case LogicalScreenDescriptor:
	    hold[count++]=ch;
	    if (count==7) {
		// Logical Screen Descriptor
		swidth=LM(hold[0], hold[1]);
		sheight=LM(hold[2], hold[3]);
		gcmap=!!(hold[4]&0x80);
		//UNUSED: bpchan=(((hold[4]&0x70)>>3)+1);
		//UNUSED: gcmsortflag=!!(hold[4]&0x08);
		gncols=2<<(hold[4]&0x7);
		bgcol=(gcmap) ? hold[5] : -1;
		//aspect=hold[6] ? double(hold[6]+15)/64.0 : 1.0;

		trans = -1;
		trans_index = -1;
		preserve_trans = FALSE;
		count=0;
		ncols=gncols;
		if (gcmap) {
		    ccount=0;
		    state=GlobalColorMap;
		    globalcmap = new QRgb[gncols+1]; // +1 for trans_index
		    globalcmap[gncols] = 0x00000000;
		} else {
		    state=Introducer;
		}
	    }
	    break;
	  case GlobalColorMap: case LocalColorMap:
	    hold[count++]=ch;
	    if (count==3) {
		QRgb rgb = qRgb(hold[0], hold[1], hold[2]);
		if ( gncols >= 256 ) {
		    if (ccount==trans)
			rgb &= 0x00ffffff;
		}
		if ( state == LocalColorMap ) {
		    if ( ccount < img.numColors() )
			img.setColor(ccount, rgb);
		} else {
		    globalcmap[ccount] = rgb;
		}
		if (++ccount >= ncols) {
		    if ( ncols < img.numColors() && state == LocalColorMap )
			img.setColor(ncols,0x00000000);
		    if ( state == LocalColorMap )
			state=TableImageLZWSize;
		    else
			state=Introducer;
		}
		count=0;
	    }
	    break;
	  case Introducer:
	    hold[count++]=ch;
	    switch (ch) {
	      case ',':
		state=ImageDescriptor;
		break;
	      case '!':
		state=ExtensionLabel;
		break;
	      case ';':
		if (consumer) {
		    if ( out_of_bounds ) // flush anything that survived
			consumer->changed(QRect(0,0,swidth,sheight));
		    consumer->end();
		}
		state=Done;
		break;
	      default:
		digress=TRUE;
		// Unexpected Introducer - ignore block
		state=Error;
	    }
	    break;
	  case ImageDescriptor:
	    hold[count++]=ch;
	    if (count==10) {
		int newleft=LM(hold[1], hold[2]);
		int newtop=LM(hold[3], hold[4]);
		int width=LM(hold[5], hold[6]);
		int height=LM(hold[7], hold[8]);

		if ( swidth <= 0 )
		    swidth = newleft + width;
		if ( sheight <= 0 )
		    sheight = newtop + height;

		if (img.isNull()) {
		    img.create(swidth, sheight, 8,
			(gcmap && gncols < 256) ? gncols+1 : 256);
		    if (consumer) consumer->setSize(swidth, sheight);
		}
		img.setAlphaBuffer(trans >= 0);
		line = img.jumpTable();

		disposePrevious( img, consumer );
		disposed = FALSE;

		left = newleft;
		top = newtop;

		// Sanity check frame size - must fit on "screen".
		if (left >= swidth) left=QMAX(0, swidth-1);
		if (top >= sheight) top=QMAX(0, sheight-1);
		if (left+width >= swidth) {
		    if ( width <= swidth )
			left=swidth-width;
		    else
			width=swidth-left;
		}
		if (top+height >= sheight) {
		    if ( height <= sheight )
			top=sheight-height;
		    else
			height=sheight-top;
		}

		right=QMAX( 0, left+width-1);
		bottom=QMAX(0, top+height-1);
		lcmap=!!(hold[9]&0x80);
		interlace=!!(hold[9]&0x40);
		//bool lcmsortflag=!!(hold[9]&0x20);
		int lncols=lcmap ? (2<<(hold[9]&0x7)) : 0;
		if (lncols) {
		    if (lncols > ncols) img.setNumColors(lncols);
		    ncols = lncols;
		} else {
		    ncols = gncols;
		}
		frame++;
		if ( frame == 0 ) {
		    if ( left || top || width!=swidth || height!=sheight ) {
			// Not full-size image - erase with bg or transparent
			if ( bgcol>=0 ) {
			    fillRect(img, 0, 0, swidth, sheight, bgcol);
			    if (consumer) consumer->changed(QRect(0,0,swidth,sheight));
			} else if ( trans_index > 0 ) {
			    fillRect(img, 0, 0, swidth, sheight, trans_index);
			    if (consumer) consumer->changed(QRect(0,0,swidth,sheight));
			}
		    }
		}

		if ( disposal == RestoreImage ) {
		    int l = QMIN(swidth-1,left);
		    int r = QMIN(swidth-1,right);
		    int t = QMIN(sheight-1,top);
		    int b = QMIN(sheight-1,bottom);
		    int w = r-l+1;
		    int h = b-t+1;

		    if (backingstore.width() < w
			|| backingstore.height() < h) {
			// We just use the backing store as a byte array
			backingstore.create( QMAX(backingstore.width(),
						  w),
					     QMAX(backingstore.height(),
						  h),
					     8,1);
		    }
		    for (int ln=0; ln<h; ln++) {
			memcpy(backingstore.scanLine(ln),
			       line[t+ln]+l, w);
		    }
		}

		count=0;
		if (lcmap) {
		    ccount=0;
		    state=LocalColorMap;
		} else {
		    if (gcmap) {
			int n=ncols; if (n<256) n++; // +1 for trans_index
			if ( !img.colorTable() || img.numColors() < n )
			    img.setNumColors( n );
			memcpy(img.colorTable(), globalcmap,
			    n * sizeof(QRgb));
		    }
		    state=TableImageLZWSize;
		}
		x = left;
		y = top;
		accum = 0;
		bitcount = 0;
		sp = stack;
		needfirst = FALSE;
		out_of_bounds = FALSE;
	    }
	    break;
	  case TableImageLZWSize: {
	    lzwsize=ch;
	    if ( lzwsize > max_lzw_bits ) {
		state=Error;
	    } else {
		code_size=lzwsize+1;
		clear_code=1<<lzwsize;
		end_code=clear_code+1;
		max_code_size=2*clear_code;
		max_code=clear_code+2;
		int i;
		for (i=0; i<clear_code && i<(1<<max_lzw_bits); i++) {
		    table[0][i]=0;
		    table[1][i]=i;
		}
		for (i=clear_code; i<(1<<max_lzw_bits); i++) {
		    table[0][i]=table[1][i]=0;
		}
		state=ImageDataBlockSize;
	    }
	    count=0;
	    break;
	  } case ImageDataBlockSize:
	    expectcount=ch;
	    if (expectcount) {
		state=ImageDataBlock;
	    } else {
		if (consumer) {
		    consumer->frameDone();
		    digress = TRUE;
		}

		state=Introducer;
	    }
	    break;
	  case ImageDataBlock:
	    count++;
	    accum|=(ch<<bitcount);
	    bitcount+=8;
	    while (bitcount>=code_size && state==ImageDataBlock) {
		int code=accum&((1<<code_size)-1);
		bitcount-=code_size;
		accum>>=code_size;

		if (code==clear_code) {
		    if (!needfirst) {
			int i;
			code_size=lzwsize+1;
			max_code_size=2*clear_code;
			max_code=clear_code+2;
			for (i=0; i<clear_code; i++) {
			    table[0][i]=0;
			    table[1][i]=i;
			}
			for (i=clear_code; i<(1<<max_lzw_bits); i++) {
			    table[0][i]=table[1][i]=0;
			}
		    }
		    needfirst=TRUE;
		} else if (code==end_code) {
		    bitcount = -32768;
		    // Left the block end arrive
		} else {
		    if (needfirst) {
			firstcode=oldcode=code;
			if (!out_of_bounds) {
			    if (firstcode==trans) {
				if ( !preserve_trans && line )
				    line[y][x] = trans_index;
			    } else if ( line ) {
				line[y][x] = firstcode;
			    }
			}
			x++;
			if (x>=swidth) out_of_bounds = TRUE;
			needfirst=FALSE;
			if (x>right) {
			    x=left;
			    if (out_of_bounds)
				out_of_bounds = left>=swidth || y>=sheight;
			    nextY(img,consumer);
			}
		    } else {
			incode=code;
			if (code>=max_code) {
			    *sp++=firstcode;
			    code=oldcode;
			}
			while (code>=clear_code) {
			    *sp++=table[1][code];
			    if (code==table[0][code]) {
				state=Error;
				break;
			    }
			    if (sp-stack>=(1<<(max_lzw_bits))*2) {
				state=Error;
				break;
			    }
			    code=table[0][code];
			}
			*sp++=firstcode=table[1][code];
			code=max_code;
			if (code<(1<<max_lzw_bits)) {
			    table[0][code]=oldcode;
			    table[1][code]=firstcode;
			    max_code++;
			    if ((max_code>=max_code_size)
			     && (max_code_size<(1<<max_lzw_bits)))
			    {
				max_code_size*=2;
				code_size++;
			    }
			}
			oldcode=incode;
			while (sp>stack) {
			    --sp;
			    if (!out_of_bounds) {
				if (*sp==trans) {
				    if ( !preserve_trans )
					line[y][x] = trans_index;
				} else {
				    line[y][x] = *sp;
				}
			    }
			    x++;
			    if (x>=swidth) out_of_bounds = TRUE;
			    if (x>right) {
				x=left;
				if (out_of_bounds)
				    out_of_bounds = left>=swidth || y>=sheight;
				nextY(img,consumer);
			    }
			}
		    }
		}
	    }
	    if (count==expectcount) {
		count=0;
		state=ImageDataBlockSize;
	    }
	    break;
	  case ExtensionLabel:
	    switch (ch) {
	     case 0xf9:
		state=GraphicControlExtension;
		break;
	     case 0xff:
		state=ApplicationExtension;
		break;
/////////////////////////////////////////// Ignored at this time //////
//           case 0xfe:
//                state=CommentExtension;
//                break;
//           case 0x01:
//                break;
///////////////////////////////////////////////////////////////////////
	     default:
		state=SkipBlockSize;
	    }
	    count=0;
	    break;
	  case ApplicationExtension:
	    if (count<11) hold[count]=ch;
	    count++;
	    if (count==hold[0]+1) {
		if (qstrncmp((char*)(hold+1), "NETSCAPE", 8)==0) {
		    // Looping extension
		    state=NetscapeExtensionBlockSize;
		} else {
		    state=SkipBlockSize;
		}
		count=0;
	    }
	    break;
	  case NetscapeExtensionBlockSize:
	    expectcount=ch;
	    count=0;
	    if (expectcount) state=NetscapeExtensionBlock;
	    else state=Introducer;
	    break;
	  case NetscapeExtensionBlock:
	    if (count<3) hold[count]=ch;
	    count++;
	    if (count==expectcount) {
		int loop = hold[0]+hold[1]*256;

		// Why if the extension here, if it is supposed to only
		// play through once?  We assume that the creator meant
		// 0, which is infinite.
		if (loop == 1) loop = 0;

		if (consumer) consumer->setLooping(loop);
		state=SkipBlockSize; // Ignore further blocks
	    }
	    break;
	  case GraphicControlExtension:
	    if (count<5) hold[count]=ch;
	    count++;
	    if (count==hold[0]+1) {
		disposePrevious( img, consumer );
		disposal=Disposal((hold[1]>>2)&0x7);
		//UNUSED: waitforuser=!!((hold[1]>>1)&0x1);
		int delay=count>3 ? LM(hold[2], hold[3]) : 0;
		bool havetrans=hold[1]&0x1;
		int newtrans=havetrans ? hold[4] : -1;
		if (newtrans > ncols) {
		    // Ignore invalid transparency.
		    newtrans=-1;
		}
		if (newtrans >= 0 && frame>=0)
		    preserve_trans = TRUE;
		if (newtrans != trans) {
		    if (trans >= 0 && trans < gncols) {
			if (globalcmap && trans_index >=0 && trans_index < gncols)
			    globalcmap[trans_index]|=0xff000000;
		    }
		    trans = newtrans;
		    if ( ncols < 256 )
			trans_index = ncols;
		    else
			trans_index = newtrans;
		    if (trans >= 0 && trans < gncols) {
			if (globalcmap && trans_index >=0 && trans_index < gncols)
			    globalcmap[trans_index]&=0x00ffffff;
		    }
		}
		if (consumer) consumer->setFramePeriod(delay*10);
		count=0;
		state=SkipBlockSize;
	    }
	    break;
	  case SkipBlockSize:
	    expectcount=ch;
	    count=0;
	    if (expectcount) state=SkipBlock;
	    else state=Introducer;
	    break;
	  case SkipBlock:
	    count++;
	    if (count==expectcount) state=SkipBlockSize;
	    break;
	  case Done:
	    digress=TRUE;
	    /* Netscape ignores the junk, so we do too.
	    length++; // Unget
	    state=Error; // More calls to this is an error
	    */
	    break;
	  case Error:
	    return -1; // Called again after done.
	}
    }
    return initial-length;
}

void QGIFFormat::fillRect(QImage& img, int col, int row, int w, int h, uchar color)
{
    if (w>0) {
	uchar** line = img.jumpTable() + row;
	for (int j=0; j<h; j++) {
	    memset(line[j]+col, color, w);
	}
    }
}

void QGIFFormat::nextY(QImage& img, QImageConsumer* consumer)
{
    int my;
    switch (interlace) {
      case 0:
	// Non-interlaced
	if (consumer && !out_of_bounds)
	    consumer->changed(QRect(left, y, right-left+1, 1));
	y++;
	break;
      case 1:
	{
	    int i;
	    my = QMIN(7, bottom-y);
	    if ( trans<0 ) // Don't dup with transparency
		for (i=1; i<=my; i++)
		    memcpy(img.scanLine(y+i)+left, img.scanLine(y)+left,
			right-left+1);
	    if (consumer && !out_of_bounds)
		consumer->changed(QRect(left, y, right-left+1, my+1));
	    y+=8;
	    if (y>bottom) {
		interlace++; y=top+4;
		if (y > bottom) { // for really broken GIFs with bottom < 5
		    interlace=2;
		    y = top + 2;
		    if (y > bottom) { // for really broken GIF with bottom < 3
			interlace = 0;
			y = top + 1;
		    }
		}
	    }
	} break;
      case 2:
	{
	    int i;
	    my = QMIN(3, bottom-y);
	    if ( trans<0 ) // Don't dup with transparency
		for (i=1; i<=my; i++)
		    memcpy(img.scanLine(y+i)+left, img.scanLine(y)+left,
			right-left+1);
	    if (consumer && !out_of_bounds)
		consumer->changed(QRect(left, y, right-left+1, my+1));
	    y+=8;
	    if (y>bottom) {
		interlace++; y=top+2;
		if (y > bottom) { // for really broken GIF with bottom < 3
		    interlace = 3;
		    y = top + 1;
		}
	    }
  	} break;
      case 3:
	{
	    int i;
	    my = QMIN(1, bottom-y);
	    if ( trans<0 ) // Don't dup with transparency
		for (i=1; i<=my; i++)
		    memcpy(img.scanLine(y+i)+left, img.scanLine(y)+left,
			right-left+1);
	    if (consumer && !out_of_bounds)
		consumer->changed(QRect(left, y, right-left+1, my+1));
	    y+=4;
	    if (y>bottom) { interlace++; y=top+1; }
	} break;
      case 4:
	if (consumer && !out_of_bounds)
	    consumer->changed(QRect(left, y, right-left+1, 1));
	y+=2;
    }

    // Consume bogus extra lines
    if (y >= sheight) out_of_bounds=TRUE; //y=bottom;
}

#endif // QT_BUILTIN_GIF_READER

#endif // QT_NO_ASYNC_IMAGE_IO

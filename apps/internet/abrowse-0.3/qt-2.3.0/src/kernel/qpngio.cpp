/****************************************************************************
** $Id$
**
** Implementation of PNG QImage IOHandler
**
** Created : 970521
**
** Copyright (C) 1992-1998 Trolltech AS.  All rights reserved.
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

#include "qfeatures.h"

#ifndef QT_NO_IMAGEIO_PNG

#include "qimage.h"
#include "qasyncimageio.h"
#include "qiodevice.h"
#include "qpngio.h"

#include <png.h>

/*
  All PNG files load to the minimal QImage equivalent.

  All QImage formats output to reasonably efficient PNG equivalents.
  Never to greyscale.
*/

#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

static
void iod_read_fn(png_structp png_ptr, png_bytep data, png_size_t length)
{
    QImageIO* iio = (QImageIO*)png_get_io_ptr(png_ptr);
    QIODevice* in = iio->ioDevice();

    while (length) {
	int nr = in->readBlock((char*)data, length);
	if (nr <= 0) {
	    png_error(png_ptr, "Read Error");
	    return;
	}
	length -= nr;
    }
}


static
void qpiw_write_fn( png_structp png_ptr, png_bytep data, png_size_t length )
{
    QPNGImageWriter* qpiw = (QPNGImageWriter*)png_get_io_ptr( png_ptr );
    QIODevice* out = qpiw->device();

    uint nr = out->writeBlock( (char*)data, length );
    if ( nr != length ) {
	png_error( png_ptr, "Write Error" );
	return;
    }
}


static
void qpiw_flush_fn( png_structp png_ptr )
{
    QPNGImageWriter* qpiw = (QPNGImageWriter*)png_get_io_ptr( png_ptr );
    QIODevice* out = qpiw->device();

    out->flush();
}

#if defined(Q_C_CALLBACKS)
}
#endif

static
void setup_qt( QImage& image, png_structp png_ptr, png_infop info_ptr )
{
    if ( png_get_valid(png_ptr, info_ptr, PNG_INFO_gAMA) ) {
	double file_gamma;
	png_get_gAMA(png_ptr, info_ptr, &file_gamma);
#if defined(_OS_MAC_)
	// a good guess for Mac systems
	png_set_gamma( png_ptr, 1.7, file_gamma );
#else
	// a good guess for PC monitors in a bright office or a dim room
	png_set_gamma( png_ptr, 2.2, file_gamma );
#endif
    }

    png_uint_32 width;
    png_uint_32 height;
    int bit_depth;
    int color_type;
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
	0, 0, 0);

    if ( color_type == PNG_COLOR_TYPE_GRAY ) {
	// Black & White or 8-bit greyscale
	if ( bit_depth == 1 && info_ptr->channels == 1 ) {
	    png_set_invert_mono( png_ptr );
	    png_read_update_info( png_ptr, info_ptr );
	    image.create( width, height, 1, 2, QImage::BigEndian );
	    image.setColor( 1, qRgb(0,0,0) );
	    image.setColor( 0, qRgb(255,255,255) );
	} else {
	    if ( bit_depth == 16 )
		png_set_strip_16(png_ptr);
	    else if ( bit_depth < 8 )
		png_set_packing(png_ptr);
	    int ncols = bit_depth < 8 ? 1 << bit_depth : 256;
	    png_read_update_info(png_ptr, info_ptr);
	    image.create(width, height, 8, ncols);
	    for (int i=0; i<ncols; i++) {
		int c = i*255/(ncols-1);
		image.setColor( i, qRgba(c,c,c,0xff) );
	    }
	    if ( png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) ) {
		int g = info_ptr->trans_values.gray;
		if ( bit_depth > 8 ) {
		    // transparency support disabled for now
		} else {
		    image.setAlphaBuffer( TRUE );
		    image.setColor(g, RGB_MASK & image.color(g));
		}
	    }
	}
    } else if ( color_type == PNG_COLOR_TYPE_PALETTE
     && png_get_valid(png_ptr, info_ptr, PNG_INFO_PLTE)
     && info_ptr->num_palette <= 256 )
    {
	// 1-bit and 8-bit color
	if ( bit_depth != 1 )
	    png_set_packing( png_ptr );
	png_read_update_info( png_ptr, info_ptr );
	png_get_IHDR(png_ptr, info_ptr,
	    &width, &height, &bit_depth, &color_type, 0, 0, 0);
	image.create(width, height, bit_depth, info_ptr->num_palette,
	    QImage::BigEndian);
	int i = 0;
	if ( png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) ) {
	    image.setAlphaBuffer( TRUE );
	    while ( i < info_ptr->num_trans ) {
		image.setColor(i, qRgba(
		    info_ptr->palette[i].red,
		    info_ptr->palette[i].green,
		    info_ptr->palette[i].blue,
		    info_ptr->trans[i]
		    )
		);
		i++;
	    }
	}
	while ( i < info_ptr->num_palette ) {
	    image.setColor(i, qRgba(
		info_ptr->palette[i].red,
		info_ptr->palette[i].green,
		info_ptr->palette[i].blue,
		0xff
		)
	    );
	    i++;
	}
    } else {
	// 32-bit
	if ( bit_depth == 16 )
	    png_set_strip_16(png_ptr);

	png_set_expand(png_ptr);

	if ( color_type == PNG_COLOR_TYPE_GRAY_ALPHA )
	    png_set_gray_to_rgb(png_ptr);

	image.create(width, height, 32);

	// Only add filler if no alpha, or we can get 5 channel data.
	if (!(color_type & PNG_COLOR_MASK_ALPHA)
	   && !png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
	    png_set_filler(png_ptr, 0xff,
		QImage::systemByteOrder() == QImage::BigEndian ?
		    PNG_FILLER_BEFORE : PNG_FILLER_AFTER);
	    // We want 4 bytes, but it isn't an alpha channel
	} else {
	    image.setAlphaBuffer(TRUE);
	}

	if ( QImage::systemByteOrder() == QImage::BigEndian ) {
	    png_set_swap_alpha(png_ptr);
	}

	png_read_update_info(png_ptr, info_ptr);
    }

    // Qt==ARGB==Big(ARGB)==Little(BGRA)
    if ( QImage::systemByteOrder() == QImage::LittleEndian ) {
	png_set_bgr(png_ptr);
    }
}


#if defined(Q_C_CALLBACKS)
extern "C" {
#endif
static void qt_png_warning(png_structp /*png_ptr*/, png_const_charp message)
{
    qWarning("libpng warning: %s", message);
}

#if defined(Q_C_CALLBACKS)
}
#endif


static
void read_png_image(QImageIO* iio)
{
    png_structp png_ptr;
    png_infop info_ptr;
    png_infop end_info;
    png_bytep* row_pointers;

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,0,0,0);
    if (!png_ptr) {
	iio->setStatus(-1);
	return;
    }

    png_set_error_fn(png_ptr, 0, 0, &qt_png_warning);

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
	png_destroy_read_struct(&png_ptr, 0, 0);
	iio->setStatus(-2);
	return;
    }

    end_info = png_create_info_struct(png_ptr);
    if (!end_info) {
	png_destroy_read_struct(&png_ptr, &info_ptr, 0);
	iio->setStatus(-3);
	return;
    }

    if (setjmp(png_ptr->jmpbuf)) {
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	iio->setStatus(-4);
	return;
    }

    png_set_read_fn(png_ptr, (void*)iio, iod_read_fn);
    png_read_info(png_ptr, info_ptr);

    QImage image;
    setup_qt(image, png_ptr, info_ptr);

    png_uint_32 width;
    png_uint_32 height;
    int bit_depth;
    int color_type;
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
	0, 0, 0);

    uchar** jt = image.jumpTable();
    row_pointers=new png_bytep[height];

    for (uint y=0; y<height; y++) {
	row_pointers[y]=jt[y];
    }

    png_read_image(png_ptr, row_pointers);

#if 0 // libpng takes care of this.
png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)
    if (image.depth()==32 && png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
	QRgb trans = 0xFF000000 | qRgb(
	      (info_ptr->trans_values.red << 8 >> bit_depth)&0xff,
	      (info_ptr->trans_values.green << 8 >> bit_depth)&0xff,
	      (info_ptr->trans_values.blue << 8 >> bit_depth)&0xff);
	for (uint y=0; y<height; y++) {
	    for (uint x=0; x<info_ptr->width; x++) {
		if (((uint**)jt)[y][x] == trans) {
		    ((uint**)jt)[y][x] &= 0x00FFFFFF;
		} else {
		}
	    }
	}
    }
#endif

    image.setDotsPerMeterX(png_get_x_pixels_per_meter(png_ptr,info_ptr));
    image.setDotsPerMeterY(png_get_y_pixels_per_meter(png_ptr,info_ptr));

    delete [] row_pointers;

    iio->setImage(image);

    png_read_end(png_ptr, end_info);
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

    iio->setStatus(0);
}

QPNGImageWriter::QPNGImageWriter(QIODevice* iod) :
    dev(iod),
    frames_written(0),
    disposal(Unspecified),
    looping(-1),
    ms_delay(-1)
{
}

QPNGImageWriter::~QPNGImageWriter()
{
}

void QPNGImageWriter::setDisposalMethod(DisposalMethod dm)
{
    disposal = dm;
}

void QPNGImageWriter::setLooping(int loops)
{
    looping = loops;
}

void QPNGImageWriter::setFrameDelay(int msecs)
{
    ms_delay = msecs;
}


#ifndef QT_NO_IMAGE_TEXT
static void set_text(const QImage& image, png_structp png_ptr, png_infop info_ptr, bool short_not_long)
{
    QValueList<QImageTextKeyLang> keys = image.textList();
    if ( keys.count() ) {
	png_textp text_ptr = new png_text[keys.count()];
	int i=0;
	for (QValueList<QImageTextKeyLang>::Iterator it=keys.begin();
		it != keys.end(); ++it)
	{
	    QString t = image.text(*it);
	    if ( (t.length() <= 200) == short_not_long ) {
		if ( t.length() < 40 )
		    text_ptr[i].compression = PNG_TEXT_COMPRESSION_NONE;
		else
		    text_ptr[i].compression = PNG_TEXT_COMPRESSION_zTXt;
		text_ptr[i].key = (png_charp)(*it).key.data();
		text_ptr[i].text = (png_charp)t.latin1();
		//text_ptr[i].text = qstrdup(t.latin1());
		i++;
	    }
	}
	png_set_text(png_ptr, info_ptr, text_ptr, i);
	//for (int j=0; j<i; j++)
	    //free(text_ptr[i].text);
	delete [] text_ptr;
    }
}
#endif

bool QPNGImageWriter::writeImage(const QImage& image, int off_x, int off_y)
{
    return writeImage(image, -1, off_x, off_y);
}

bool QPNGImageWriter::writeImage(const QImage& image, int quality, int off_x, int off_y)
{
    QPoint offset = image.offset();
    off_x += offset.x();
    off_y += offset.y();

    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep* row_pointers;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,0,0,0);
    if (!png_ptr) {
	return FALSE;
    }

    png_set_error_fn(png_ptr, 0, 0, &qt_png_warning);

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
	png_destroy_write_struct(&png_ptr, 0);
	return FALSE;
    }

    if (setjmp(png_ptr->jmpbuf)) {
	png_destroy_write_struct(&png_ptr, &info_ptr);
	return FALSE;
    }

    if (quality >= 0) {
	if (quality > 9) {
#if defined(CHECK_RANGE)
	    qWarning( "PNG: Quality %d out of range", quality );
#endif
	    quality = 9;
	}
	png_set_compression_level(png_ptr, quality);
    }

    png_set_write_fn(png_ptr, (void*)this, qpiw_write_fn, qpiw_flush_fn);

    info_ptr->channels =
	(image.depth() == 32)
	    ? (image.hasAlphaBuffer() ? 4 : 3)
	    : 1;

    png_set_IHDR(png_ptr, info_ptr, image.width(), image.height(),
	image.depth() == 1 ? 1 : 8 /* per channel */,
	image.depth() == 32
	    ? image.hasAlphaBuffer()
		? PNG_COLOR_TYPE_RGB_ALPHA
		: PNG_COLOR_TYPE_RGB
	    : PNG_COLOR_TYPE_PALETTE, 0, 0, 0);


    //png_set_sBIT(png_ptr, info_ptr, 8);
    info_ptr->sig_bit.red = 8;
    info_ptr->sig_bit.green = 8;
    info_ptr->sig_bit.blue = 8;

#if 0 // libpng takes care of this.
    if (image.depth() == 1 && image.bitOrder() == QImage::BigEndian)
       png_set_packswap(png_ptr);
#endif

    if (image.numColors()) {
	// Paletted
        int num_palette = image.numColors();
	png_colorp palette = new png_color[num_palette];
	png_set_PLTE(png_ptr, info_ptr, palette, num_palette);
	int* trans = new int[num_palette];
	int num_trans = 0;
	for (int i=0; i<num_palette; i++) {
	    QRgb rgb=image.color(i);
	    info_ptr->palette[i].red = qRed(rgb);
	    info_ptr->palette[i].green = qGreen(rgb);
	    info_ptr->palette[i].blue = qBlue(rgb);
	    if (image.hasAlphaBuffer()) {
		trans[i] = rgb >> 24;
		if (trans[i] < 255) {
		    num_trans = i+1;
		}
	    }
	}
	if (num_trans) {
	    png_bytep copy_trans = new png_byte[num_trans];
	    for (int i=0; i<num_trans; i++)
		copy_trans[i] = trans[i];
	    png_set_tRNS(png_ptr, info_ptr, copy_trans, num_trans, 0);
	}
	delete trans;
    }

    if ( image.hasAlphaBuffer() ) {
	info_ptr->sig_bit.alpha = 8;
    }

    // Swap ARGB to RGBA (normal PNG format) before saving on
    // BigEndian machines
    if ( QImage::systemByteOrder() == QImage::BigEndian ) {
	png_set_swap_alpha(png_ptr);
    }

    // Qt==ARGB==Big(ARGB)==Little(BGRA)
    if ( QImage::systemByteOrder() == QImage::LittleEndian ) {
	png_set_bgr(png_ptr);
    }

    if (off_x || off_y) {
	png_set_oFFs(png_ptr, info_ptr, off_x, off_y, PNG_OFFSET_PIXEL);
    }

    if ( frames_written > 0 )
	png_set_sig_bytes(png_ptr, 8);

    if ( image.dotsPerMeterX() > 0 || image.dotsPerMeterY() > 0 ) {
	png_set_pHYs(png_ptr, info_ptr,
		image.dotsPerMeterX(), image.dotsPerMeterY(),
		PNG_RESOLUTION_METER);
    }

#ifndef QT_NO_IMAGE_TEXT
    // Write short texts early.
    set_text(image,png_ptr,info_ptr,TRUE);
#endif

    png_write_info(png_ptr, info_ptr);

#ifndef QT_NO_IMAGE_TEXT
    // Write long texts later.
    set_text(image,png_ptr,info_ptr,FALSE);
#endif

    if ( image.depth() != 1 )
	png_set_packing(png_ptr);

    if ( image.depth() == 32 && !image.hasAlphaBuffer() )
	png_set_filler(png_ptr, 0,
	    QImage::systemByteOrder() == QImage::BigEndian ?
		PNG_FILLER_BEFORE : PNG_FILLER_AFTER);

    if ( looping >= 0 && frames_written == 0 ) {
	uchar data[13] = "NETSCAPE2.0";
	//                0123456789aBC
	data[0xB] = looping%0x100;
	data[0xC] = looping/0x100;
	png_write_chunk(png_ptr, (png_byte*)"gIFx", data, 13);
    }
    if ( ms_delay >= 0 || disposal!=Unspecified ) {
	uchar data[4];
	data[0] = disposal;
	data[1] = 0;
	data[2] = (ms_delay/10)/0x100; // hundredths
	data[3] = (ms_delay/10)%0x100;
	png_write_chunk(png_ptr, (png_byte*)"gIFg", data, 4);
    }

    png_uint_32 width;
    png_uint_32 height;
    int bit_depth;
    int color_type;
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
	0, 0, 0);

    uchar** jt = image.jumpTable();
    row_pointers=new png_bytep[height];
    uint y;
    for (y=0; y<height; y++) {
	    row_pointers[y]=jt[y];
    }
    png_write_image(png_ptr, row_pointers);
    delete [] row_pointers;

    png_write_end(png_ptr, info_ptr);
    frames_written++;

    if (image.numColors())
	delete [] info_ptr->palette;
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
	delete [] info_ptr->trans;

    png_destroy_write_struct(&png_ptr, &info_ptr);

    return TRUE;
}

static
void write_png_image(QImageIO* iio)
{
    QPNGImageWriter writer(iio->ioDevice());
    int quality = -1;
    if ( iio->parameters() ) {
	bool ok = FALSE;
	int iq = QString::fromLatin1( iio->parameters() ).toInt( &ok );
	if ( ok && iq >= 0 ) {
	    if ( iq > 100) {
#if defined(CHECK_RANGE)
		qWarning( "PNG: Quality %d out of range", iq );
#endif
		iq = 100;
	    }
	    quality = (100-iq) * 9 / 91; // map [0,100] -> [9,0]
	}
    }
    if ( writer.writeImage(iio->image(), quality) )
	iio->setStatus(0);
    else
	iio->setStatus(-1);
}

// NOT REVISED
/*!
  \class QPNGImagePacker qpngio.h
  \brief Creates well-compressed PNG animations.

  By using transparency, QPNGImagePacker allows you to build a PNG image
  from a sequence of QImages.
*/


/*!
  Create an image packer that writes PNG data to \a iod, using a
  \a storage_depth bit encoding (use 8 or 32, depending on the
  desired quality and compression requirements).
*/
QPNGImagePacker::QPNGImagePacker(QIODevice* iod, int storage_depth,
	int conversionflags) :
    QPNGImageWriter(iod),
    depth(storage_depth),
    convflags(conversionflags),
    alignx(1)
{
}

/*!
  Align pixel differences to \a x pixels.  For example, using 8 can
  improve playback on certain hardware.  Normally the default of 1-pixel
  alignment (ie. no alignment) gives bets compression and performance.
*/
void QPNGImagePacker::setPixelAlignment(int x)
{
    alignx = x;
}

/*!
  Add the image \a img to the PNG animation, analyzing the differences
  between this and the previous image to improve compression.
*/
bool QPNGImagePacker::packImage(const QImage& img)
{
    QImage image = img.convertDepth(32);
    if ( previous.isNull() ) {
	// First image
	writeImage(image.convertDepth(depth,convflags));
    } else {
	bool done;
	int minx, maxx, miny, maxy;
	int w = image.width();
	int h = image.height();

	QRgb** jt = (QRgb**)image.jumpTable();
	QRgb** pjt = (QRgb**)previous.jumpTable();

	// Find left edge of change
	done = FALSE;
	for (minx = 0; minx < w && !done; minx++) {
	    for (int ty = 0; ty < h; ty++) {
		if ( jt[ty][minx] != pjt[ty][minx] ) {
		    done = TRUE;
		    break;
		}
	    }
	}
	minx--;

	// Find right edge of change
	done = FALSE;
	for (maxx = w-1; maxx >= 0 && !done; maxx--) {
	    for (int ty = 0; ty < h; ty++) {
		if ( jt[ty][maxx] != pjt[ty][maxx] ) {
		    done = TRUE;
		    break;
		}
	    }
	}
	maxx++;

	// Find top edge of change
	done = FALSE;
	for (miny = 0; miny < h && !done; miny++) {
	    for (int tx = 0; tx < w; tx++) {
		if ( jt[miny][tx] != pjt[miny][tx] ) {
		    done = TRUE;
		    break;
		}
	    }
	}
	miny--;

	// Find right edge of change
	done = FALSE;
	for (maxy = h-1; maxy >= 0 && !done; maxy--) {
	    for (int tx = 0; tx < w; tx++) {
		if ( jt[maxy][tx] != pjt[maxy][tx] ) {
		    done = TRUE;
		    break;
		}
	    }
	}
	maxy++;

	if ( minx > maxx ) minx=maxx=0;
	if ( miny > maxy ) miny=maxy=0;

	if ( alignx > 1 ) {
	    minx -= minx % alignx;
	    maxx = maxx - maxx % alignx + alignx - 1;
	}

	int dw = maxx-minx+1;
	int dh = maxy-miny+1;

	QImage diff(dw, dh, 32);

	diff.setAlphaBuffer(TRUE);
	int x, y;
	if ( alignx < 1 )
	    alignx = 1;
	for (y = 0; y < dh; y++) {
	    QRgb* li = (QRgb*)image.scanLine(y+miny)+minx;
	    QRgb* lp = (QRgb*)previous.scanLine(y+miny)+minx;
	    QRgb* ld = (QRgb*)diff.scanLine(y);
	    if ( alignx ) {
		for (x = 0; x < dw; x+=alignx) {
		    int i;
		    for (i=0; i<alignx; i++) {
			if ( li[x+i] != lp[x+i] )
			    break;
		    }
		    if ( i == alignx ) {
			// All the same
			for (i=0; i<alignx; i++) {
			    ld[x+i] = qRgba(0,0,0,0);
			}
		    } else {
			// Some different
			for (i=0; i<alignx; i++) {
			    ld[x+i] = 0xff000000 | li[x+i];
			}
		    }
		}
	    } else {
		for (x = 0; x < dw; x++) {
		    if ( li[x] != lp[x] )
			ld[x] = 0xff000000 | li[x];
		    else
			ld[x] = qRgba(0,0,0,0);
		}
	    }
	}

	diff = diff.convertDepth(depth,convflags);
	if ( !writeImage(diff, minx, miny) )
	    return FALSE;
    }
    previous = image;
    return TRUE;
}


#ifndef QT_NO_ASYNC_IMAGE_IO

class Q_EXPORT QPNGFormat : public QImageFormat {
public:
    QPNGFormat();
    virtual ~QPNGFormat();

    int decode(QImage& img, QImageConsumer* consumer,
	    const uchar* buffer, int length);

    void info(png_structp png_ptr, png_infop info);
    void row(png_structp png_ptr, png_bytep new_row,
		png_uint_32 row_num, int pass);
    void end(png_structp png_ptr, png_infop info);
#ifdef PNG_USER_CHUNK_SUPPORTED
    int user_chunk(png_structp png_ptr, png_infop info,
	    png_bytep data, png_uint_32 length);
#endif

private:
    // Animation-level information
    enum { MovieStart, FrameStart, Inside, End } state;
    int first_frame;
    int base_offx;
    int base_offy;

    // Image-level information
    png_structp png_ptr;
    png_infop info_ptr;

    // Temporary locals during single data-chunk processing
    QImageConsumer* consumer;
    QImage* image;
    int unused_data;
};

class Q_EXPORT QPNGFormatType : public QImageFormatType
{
    QImageFormat* decoderFor(const uchar* buffer, int length);
    const char* formatName() const;
};


/*!
  \class QPNGFormat qpngio.h
  \brief Incremental image decoder for PNG image format.

  \ingroup images

  This subclass of QImageFormat decodes PNG format images,
  including animated PNGs.

  Animated PNG images are standard PNG images.  The PNG standard
  defines two extension chunks that are useful for animations:

  <dl>
   <dt>gIFg - GIF-like Graphic Control Extension
    <dd>Includes frame disposal, user input flag (we ignore this),
	    and inter-frame delay.
   <dt>gIFx - GIF-like Application Extension
    <dd>Multi-purpose, but we just use the Netscape extension
	    which specifies looping.
  </dl>

  The subimages usually contain a offset chunk (oFFs) but need not.

  The first image defines the "screen" size.  Any subsequent image that
  doesn't fit is clipped.

TODO: decide on this point.  gIFg gives disposal types, so it can be done.
  All images paste (\e not composite, just place all-channel copying)
  over the previous image to produce a subsequent frame.
*/

/*!
  \class QPNGFormatType qasyncimageio.h
  \brief Incremental image decoder for PNG image format.

  \ingroup images
  \ingroup io

  This subclass of QImageFormatType recognizes PNG
  format images, creating a QPNGFormat when required.  An instance
  of this class is created automatically before any other factories,
  so you should have no need for such objects.
*/

QImageFormat* QPNGFormatType::decoderFor(
    const uchar* buffer, int length)
{
    if (length < 8) return 0;
    if (buffer[0]==137
     && buffer[1]=='P'
     && buffer[2]=='N'
     && buffer[3]=='G'
     && buffer[4]==13
     && buffer[5]==10
     && buffer[6]==26
     && buffer[7]==10)
	return new QPNGFormat;
    return 0;
}

const char* QPNGFormatType::formatName() const
{
    return "PNG";
}


#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

static void
info_callback(png_structp png_ptr, png_infop info)
{
    QPNGFormat* that = (QPNGFormat*)png_get_progressive_ptr(png_ptr);
    that->info(png_ptr,info);
}

static void
row_callback(png_structp png_ptr, png_bytep new_row,
   png_uint_32 row_num, int pass)
{
    QPNGFormat* that = (QPNGFormat*)png_get_progressive_ptr(png_ptr);
    that->row(png_ptr,new_row,row_num,pass);
}

static void
end_callback(png_structp png_ptr, png_infop info)
{
    QPNGFormat* that = (QPNGFormat*)png_get_progressive_ptr(png_ptr);
    that->end(png_ptr,info);
}

#ifdef PNG_USER_CHUNK_SUPPORTED
static int
user_chunk_callback(png_structp png_ptr, png_infop info,
	    png_bytep data, png_uint_32 length)
{
    QPNGFormat* that = (QPNGFormat*)png_get_progressive_ptr(png_ptr);
    return that->user_chunk(png_ptr,info,data,length);
}
#endif

#if defined(Q_C_CALLBACKS)
}
#endif


/*!
  Constructs a QPNGFormat.
*/
QPNGFormat::QPNGFormat()
{
    state = MovieStart;
    first_frame = 1;
    base_offx = 0;
    base_offy = 0;
    png_ptr = 0;
    info_ptr = 0;
}


/*!
  Destructs a QPNGFormat.
*/
QPNGFormat::~QPNGFormat()
{
    if ( png_ptr )
        png_destroy_read_struct(&png_ptr, &info_ptr, 0);
}


/*!
  This function decodes some data into image changes.

  Returns the number of bytes consumed.
*/
int QPNGFormat::decode(QImage& img, QImageConsumer* cons,
	const uchar* buffer, int length)
{
    consumer = cons;
    image = &img;

    if ( state != Inside ) {
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (!png_ptr) {
	    info_ptr = 0;
	    image = 0;
		return -1;
	}

	png_set_error_fn(png_ptr, 0, 0, &qt_png_warning);
	png_set_compression_level(png_ptr, 9);

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
	    png_destroy_read_struct(&png_ptr, &info_ptr, 0);
	    image = 0;
	    return -1;
	}

	if (setjmp((png_ptr)->jmpbuf)) {
	    png_destroy_read_struct(&png_ptr, &info_ptr, 0);
	    image = 0;
	    return -1;
	}

	png_set_progressive_read_fn(png_ptr, (void *)this,
	                            info_callback, row_callback, end_callback);

#ifdef PNG_USER_CHUNK_SUPPORTED
	png_set_user_chunk_fn(png_ptr, user_chunk_callback);
#endif

	if ( state != MovieStart && *buffer != 0211 ) {
	    // Good, no signature - the preferred way to concat PNG images.
	    // Skip them.
	    png_set_sig_bytes(png_ptr, 8);
	}

	state = Inside;
    }

    if ( !png_ptr ) return 0;
    
    if (setjmp(png_ptr->jmpbuf)) {
	png_destroy_read_struct(&png_ptr, &info_ptr, 0);
	image = 0;
	state = MovieStart;
	return -1;
    }
    unused_data = 0;
    png_process_data(png_ptr, info_ptr, (png_bytep)buffer, length);
    int l = length - unused_data;

    // TODO: send incremental stuff to consumer (optional)

    if ( state != Inside ) {
	if ( png_ptr )
	    png_destroy_read_struct(&png_ptr, &info_ptr, 0);
    }

    image = 0;
    return l;
}

void QPNGFormat::info(png_structp png, png_infop)
{
    png_set_interlace_handling(png);
    setup_qt(*image, png, info_ptr);
}

void QPNGFormat::row(png_structp png, png_bytep new_row,
    png_uint_32 row_num, int)
{
    uchar* old_row = image->scanLine(row_num);
    png_progressive_combine_row(png, old_row, new_row);
}


void QPNGFormat::end(png_structp png, png_infop info)
{
    int offx = png_get_x_offset_pixels(png,info) - base_offx;
    int offy = png_get_y_offset_pixels(png,info) - base_offy;
    if ( first_frame ) {
	base_offx = offx;
	base_offy = offy;
	first_frame = 0;
    }
    image->setOffset(QPoint(offx,offy));
    image->setDotsPerMeterX(png_get_x_pixels_per_meter(png,info));
    image->setDotsPerMeterY(png_get_y_pixels_per_meter(png,info));
#ifndef QT_NO_IMAGE_TEXT
    png_textp text_ptr;
    int num_text=0;
    png_get_text(png,info,&text_ptr,&num_text);
    while (num_text--) {
	image->setText(text_ptr->key,0,text_ptr->text);
	text_ptr++;
    }
#endif
    QRect r(0,0,image->width(),image->height());
    consumer->frameDone(QPoint(offx,offy),r);
    state = FrameStart;
    unused_data = png->buffer_size; // Since libpng doesn't tell us
}

#ifdef PNG_USER_CHUNK_SUPPORTED

#ifndef QT_NO_IMAGE_TEXT
static bool skip(png_uint_32& max, png_bytep& data)
{
    while (*data) {
	if ( !max ) return FALSE;
	max--;
	data++;
    }
    if ( !max ) return FALSE;
    max--;
    data++; // skip to after NUL
    return TRUE;
}
#endif

int QPNGFormat::user_chunk(png_structp png, png_infop,
	    png_bytep data, png_uint_32 length)
{
#if 0 // NOT SUPPORTED: experimental PNG animation.
    // qDebug("Got %ld-byte %s chunk", length, png->chunk_name);
    if ( 0==qstrcmp((char*)png->chunk_name, "gIFg")
	    && length == 4 ) {

	//QPNGImageWriter::DisposalMethod disposal =
	//  (QPNGImageWriter::DisposalMethod)data[0];
	// ### TODO: use the disposal method
	int ms_delay = ((data[2] << 8) | data[3])*10;
	consumer->setFramePeriod(ms_delay);
	return 1;
    } else if ( 0==qstrcmp((char*)png->chunk_name, "gIFx")
	    && length == 13 ) {
	if ( qstrncmp((char*)data,"NETSCAPE2.0",11)==0 ) {
	    int looping = (data[0xC]<<8)|data[0xB];
	    consumer->setLooping(looping);
	    return 1;
	}
    }
#endif

#ifndef QT_NO_IMAGE_TEXT
    if ( 0==qstrcmp((char*)png->chunk_name, "iTXt") && length>=6 ) {
	const char* keyword = (const char*)data;
	if ( !skip(length,data) ) return 0;
	if ( length >= 4 ) {
	    char compression_flag = *data++;
	    char compression_method = *data++;
	    if ( compression_flag == compression_method ) {
		// fool the compiler into thinking they're used
	    }
	    const char* lang = (const char*)data;
	    if ( !skip(length,data) ) return 0;
	    // const char* keyword_utf8 = (const char*)data;
	    if ( !skip(length,data) ) return 0;
	    const char* text_utf8 = (const char*)data;
	    if ( !skip(length,data) ) return 0;
	    QString text = QString::fromUtf8(text_utf8);
	    image->setText(keyword,lang[0] ? lang : 0,text);
	    return 1;
	}
    }
#endif

    return 0;
}
#endif


static QPNGFormatType* globalPngFormatTypeObject = 0;

#endif // QT_NO_ASYNC_IMAGE_IO

void qCleanupPngIO()
{
#ifndef QT_NO_ASYNC_IMAGE_IO
    if ( globalPngFormatTypeObject ) {
	delete globalPngFormatTypeObject;
	globalPngFormatTypeObject = 0;
    }
#endif
}

void qInitPngIO()
{
    static bool done = FALSE;
    if ( !done ) {
	done = TRUE;
	QImageIO::defineIOHandler( "PNG", "^.PNG\r", 0, read_png_image,
				   write_png_image);
#ifndef QT_NO_ASYNC_IMAGE_IO
	globalPngFormatTypeObject = new QPNGFormatType;
#endif
	qAddPostRoutine( qCleanupPngIO );
    }
}

void qt_zlib_compression_hack()
{
    compress(0,0,0,0);
    uncompress(0,0,0,0);
}

#endif // QT_NO_IMAGEIO_PNG

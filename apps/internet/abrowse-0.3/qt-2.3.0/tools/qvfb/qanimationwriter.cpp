/****************************************************************************
**
** Qt/Embedded virtual framebuffer
**
** Created : 20000605
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the Qt GUI Toolkit.
**
** Licensees holding valid Qt Professional Edition licenses may use this
** file in accordance with the Qt Professional Edition License Agreement
** provided with the Qt Professional Edition.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
** information about the Professional Edition licensing.
**
*****************************************************************************/

#define QT_CLEAN_NAMESPACE
#include "qanimationwriter.h"
#include <qfile.h>
#include <png.h>
#include <netinet/in.h> // for htonl

class QAnimationWriterData {
public:
    QAnimationWriterData(QIODevice* d) : dev(d)
    {
	framerate = 1000;
    }
    void setFrameRate(int d) { framerate = d; }
    virtual ~QAnimationWriterData() { }
    virtual void setImage(const QImage& src)=0;
    virtual bool canCompose() const { return FALSE; }
    virtual void composeImage(const QImage&, const QPoint& ) { }

protected:
    int framerate;
    QIODevice* dev;
};


class QAnimationWriterMNG : public QAnimationWriterData {
    bool first;
    png_structp png_ptr;
    png_infop info_ptr;
public:
    QAnimationWriterMNG(QIODevice* d) : QAnimationWriterData(d)
    {
	first = TRUE;
	begin_png();
    }

    ~QAnimationWriterMNG()
    {
	if ( first ) {
	    // Eh? Not images.
	    QImage dummy(1,1,32);
	    setImage(dummy);
	}
	writeMEND();
	end_png();
    }

    void begin_png()
    {
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,0,0,0);
	info_ptr = png_create_info_struct(png_ptr);
	png_set_compression_level(png_ptr,9);
	png_set_write_fn(png_ptr, (void*)this, write, 0);
    }

    void end_png()
    {
	png_destroy_write_struct(&png_ptr, &info_ptr);
    }


    static void write( png_structp png_ptr, png_bytep data, png_size_t length)
    {
	QAnimationWriterMNG* that = (QAnimationWriterMNG*)png_get_io_ptr(png_ptr);
	/*uint nw =*/ that->dev->writeBlock((const char*)data,length);
    }

    void writePNG(const QImage& image)
    {
	info_ptr->channels = 4;
	png_set_sig_bytes(png_ptr, 8); // Pretend we already wrote the sig
	png_set_IHDR(png_ptr, info_ptr, image.width(), image.height(),
	    8, image.hasAlphaBuffer()
		    ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB,
	    0, 0, 0);
	png_write_info(png_ptr, info_ptr);
	if ( !image.hasAlphaBuffer() )
	    png_set_filler(png_ptr, 0,
		QImage::systemByteOrder() == QImage::BigEndian ?
		    PNG_FILLER_BEFORE : PNG_FILLER_AFTER);
	//if ( QImage::systemByteOrder() == QImage::BigEndian ) {
            //png_set_swap_alpha(png_ptr);
        //}
	if ( QImage::systemByteOrder() == QImage::LittleEndian ) {
	    png_set_bgr(png_ptr);
	}

	png_bytep* row_pointers;
	uint height = image.height();
	uchar** jt = image.jumpTable();
	row_pointers=new png_bytep[height];
	uint y;
	for (y=0; y<height; y++) {
		row_pointers[y]=jt[y];
	}
	png_write_image(png_ptr, row_pointers);
	delete [] row_pointers;
	png_write_end(png_ptr, info_ptr);
	end_png();
	begin_png();
    }

    void writeMHDR( const QSize& size, int framerate )
    {
	dev->writeBlock("\212MNG\r\n\032\n", 8);

	struct {
	    int width;
	    int height;
	    int framerate;
	    int a,b,c;
	    int profile;
	} chunk;
	chunk.width = htonl(size.width());
	chunk.height = htonl(size.height());
	chunk.framerate = htonl(framerate);
	chunk.a=0;
	chunk.b=0;
	chunk.c=0;
	chunk.profile= htonl(0x00000003);

	png_write_chunk(png_ptr, (png_byte*)"MHDR", (png_byte*)&chunk, sizeof(chunk));
    }

    void writeMEND()
    {
	png_write_chunk(png_ptr, (png_byte*)"MEND", 0, 0);
    }

    void writeDEFI( const QPoint& offset, const QSize& size )
    {
	struct {
	    ushort o;
	    uchar s;
	    uchar concrete;
	    int x,y;
	    int lc,rc,tc,bc;
	} chunk;
	chunk.o=0;
	chunk.s=0;
	chunk.concrete=1;
	chunk.x=htonl(offset.x());
	chunk.y=htonl(offset.y());
	chunk.lc=0;
	chunk.rc=0;
	chunk.tc=htonl(INT_MAX);
	chunk.bc=htonl(INT_MAX);

	png_write_chunk(png_ptr, (png_byte*)"DEFI", (png_byte*)&chunk, sizeof(chunk));
    }

    void writeFRAM( const QSize& size )
    {
	struct {
	    uchar mode;
	    uchar n;
	    uchar nu;
	    uchar d;
	    uchar t;
	    uchar clip;
	    uchar s;
	    uchar deltatype;
	    uint left;
	    uint right;
	    uint top;
	    uint bottom;
	} chunk;
	chunk.mode=1;
	chunk.n='a';
	chunk.nu=0;
	chunk.d=0;
	chunk.clip=1;
	chunk.t=0;
	chunk.s=0;
	chunk.deltatype=0;
	chunk.left=0;
	chunk.right=htonl(size.width());
	chunk.top=0;
	chunk.bottom=htonl(size.height());

	png_write_chunk(png_ptr, (png_byte*)"FRAM", (png_byte*)&chunk, sizeof(chunk));
    }

    void writeMOVE( const QPoint& offset )
    {
	struct {
	    uchar filler[3];
	    uchar z[5];
	    int x,y;
	} chunk;
	memset(chunk.z,0,5);
	chunk.x=htonl(offset.x());
	chunk.y=htonl(offset.y());

	png_write_chunk(png_ptr, (png_byte*)"MOVE", ((png_byte*)&chunk)+3, sizeof(chunk)-3);
    }

    void setImage(const QImage& src)
    {
	if ( first ) {
	    first = FALSE;
	    writeMHDR(src.size(),framerate);
	}
	composeImage(src,QPoint(0,0));
    }

    bool canCompose() const { return TRUE; }

    void composeImage(const QImage& src, const QPoint& offset)
    {
	writeMOVE(offset);
	//writeFRAM(src.size());
	writePNG(src);
    }
};

QAnimationWriter::QAnimationWriter( const QString& filename, const char* format )
{
    if ( QCString(format) != "MNG" ) {
	qWarning("Format \"%s\" not supported, only MNG", format);
	dev = 0;
	d = 0;
    } else {
	QFile *f = new QFile(filename);
	f->open(IO_WriteOnly);
	dev = f;
	d = new QAnimationWriterMNG(dev);
    }
}

bool QAnimationWriter::okay() const
{
    return dev && dev->status() == IO_Ok;
}

QAnimationWriter::~QAnimationWriter()
{
    delete d;
    delete dev;
}

void QAnimationWriter::setFrameRate(int r)
{
    if (d) d->setFrameRate(r);
}

void QAnimationWriter::appendFrame(const QImage& frm, const QPoint& offset)
{
    QImage frame = frm.convertDepth(32);
    const int alignx = 1;
    if ( dev ) {
	if ( prev.isNull() || !d->canCompose() ) {
	    d->setImage(frame);
	} else {
	    bool done;
	    int minx, maxx, miny, maxy;
	    int w = frame.width();
	    int h = frame.height();

	    QRgb** jt = (QRgb**)frame.jumpTable();
	    QRgb** pjt = (QRgb**)prev.jumpTable() + offset.y();

	    // Find left edge of change
	    done = FALSE;
	    for (minx = 0; minx < w && !done; minx++) {
		for (int ty = 0; ty < h; ty++) {
		    if ( jt[ty][minx] != pjt[ty][minx+offset.x()] ) {
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
		    if ( jt[ty][maxx] != pjt[ty][maxx+offset.x()] ) {
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
		    if ( jt[miny][tx] != pjt[miny][tx+offset.x()] ) {
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
		    if ( jt[maxy][tx] != pjt[maxy][tx+offset.x()] ) {
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
	    for (y = 0; y < dh; y++) {
		QRgb* li = (QRgb*)frame.scanLine(y+miny)+minx;
		QRgb* lp = (QRgb*)prev.scanLine(y+miny+offset.y())+minx+offset.x();
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
qDebug("%d,%d  %d,%d",minx,miny,offset.x(),offset.y());
	    d->composeImage(diff,QPoint(minx,miny)+offset);
	}
	if ( prev.isNull() || prev.size() == frame.size() && offset == QPoint(0,0) ) {
	    prev = frame;
	} else {
	    bitBlt(&prev,offset.x(),offset.y(),&frame,0,0,frame.width(),frame.height());
	}
    }
}

void QAnimationWriter::appendFrame(const QImage& frm)
{
    appendFrame(frm,QPoint(0,0));
}

void QAnimationWriter::appendBlankFrame()
{
    QImage i(1,1,32);
    i.setAlphaBuffer(TRUE);
    i.fill(0);
    d->composeImage(i,QPoint(0,0));
}

/****************************************************************************
** $Id$
**
** Implementation of movie classes
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

// #define QT_SAVE_MOVIE_HACK

#include "qtimer.h"
#include "qpainter.h"
#include "qlist.h"
#include "qbitmap.h"
#include "qmovie.h"
#include "qfile.h"
#include "qbuffer.h"
#include "qshared.h"

#ifndef QT_NO_MOVIE

#ifdef _WS_QWS_
#include "qgfx_qws.h"
#endif

#include "qasyncio.h"
#include "qasyncimageio.h"

#include <stdlib.h>

// NOT REVISED
/*!
  \class QMovie qmovie.h
  \brief Incrementally loads an animation or image, signalling as it progresses.

  \ingroup images
  \ingroup drawing

  A QMovie provides a QPixmap as the framePixmap(), and connections can
  be made via connectResize() and connectUpdate() to receive notification
  of size and pixmap changes.  All decoding is driven by
  the normal event processing mechanisms.  The simplest way to display
  a QMovie, is to use a QLabel and QLabel::setMovie().

  The movie begins playing as soon as the QMovie is created (actually,
  once control returns to the event loop).  When the last frame in the
  movie has been played, it may loop back to the start if such looping
  is defined in the input source.

  QMovie objects are explicitly shared.  This means that a QMovie copied
  from another QMovie will be displaying the same frame at all times.
  If one shared movie pauses, all pause.  To make \e independent movies,
  they must be constructed separately.

  The set of data formats supported by QMovie is determined by the decoder
  factories which have been installed, and the format of the input is
  determined as the input is decoded.

  The supported formats are MNG (if Qt is built with MNG support enabled)
  and GIF (if Qt is built with GIF support enabled).  For MNG support, you
  need to have installed libmng from
  <a href="http://www.libmng.com/">http://www.libmng.com</a>.

  Archives of animated GIFs and tools for building them can be found at
  <a href="http://dir.yahoo.com/Arts/Visual_Arts/Animation/Computer_Animation/Animated_GIFs/">Yahoo</a>.

  We are required to state: The Graphics Interchange Format(c) is the
  Copyright property of CompuServe Incorporated. GIF(sm) is a Service
  Mark property of CompuServe Incorporated.

  \warning Unisys has changed its position regarding GIF.  If you are
  in a country where Unisys holds a patent on LZW compression and/or
  decompression and you want to use GIF, Unisys may require you to
  license that technology.  These countries include Canada, Japan, the
  USA, France, Germany, Italy and the UK.

  GIF support may be removed completely in a future version of Qt.  We
  recommend using the MNG or PNG format.

  <img src="qmovie.png">

  \sa QLabel::setMovie()
*/

class QMovieFilePrivate : public QObject, public QShared,
		      private QDataSink, private QImageConsumer
{
    Q_OBJECT

public: // for QMovie

    // Creates a null QMovieFilePrivate
    QMovieFilePrivate();

    // NOTE:  The ownership of the QDataSource is transferred to the QMovieFilePrivate
    QMovieFilePrivate(QDataSource* src, QMovie* movie, int bufsize);

    virtual ~QMovieFilePrivate();

    bool isNull() const;

    // Initialize, possibly to the null state
    void init(bool fully);
    void flushBuffer();
    void updatePixmapFromImage();
    void updatePixmapFromImage(const QPoint& off, const QRect& area);
    void showChanges();

    // This as QImageConsumer
    void changed(const QRect& rect);
    void end();
    void preFrameDone(); //util func
    void frameDone();
    void frameDone(const QPoint&, const QRect& rect);
    void restartTimer();
    void setLooping(int l);
    void setFramePeriod(int milliseconds);
    void setSize(int w, int h);

    // This as QDataSink
    int readyToReceive();
    void receive(const uchar* b, int bytecount);
    void eof();
    void pause();

signals:
    void sizeChanged(const QSize&);
    void areaChanged(const QRect&);
    void dataStatus(int);

public slots:
    void refresh();

public:
    QMovie *that;
    QWidget * display_widget;

    QImageDecoder *decoder;

    // Cyclic buffer
    int buf_size;
    uchar *buffer;
    int buf_r, buf_w, buf_usage;

    int framenumber;
    int frameperiod;
    int speed;
    QTimer *frametimer;
    int lasttimerinterval;
    int loop;
    bool movie_ended;

    bool waitingForFrameTick;
    int stepping;
    QRect changed_area;
    QRect valid_area;
    QDataPump *pump;
    QDataSource *source;
    QPixmap mypixmap;
    QBitmap mymask;
    QColor bg;

    int error;
    bool empty;

#ifdef QT_SAVE_MOVIE_HACK
  bool save_image;
  int image_number;
#endif
};


QMovieFilePrivate::QMovieFilePrivate()
{
    buffer = 0;
    pump = 0;
    source = 0;
    decoder = 0;
    display_widget=0;
    buf_size = 0;
    init(FALSE);
}

// NOTE:  The ownership of the QDataSource is transferred to the QMovieFilePrivate
QMovieFilePrivate::QMovieFilePrivate(QDataSource* src, QMovie* movie, int bufsize) :
    that(movie),
    buf_size(bufsize)
{
    frametimer = new QTimer(this);
    pump = src ? new QDataPump(src, this) : 0;
    QObject::connect(frametimer, SIGNAL(timeout()), this, SLOT(refresh()));
    source = src;
    buffer = 0;
    decoder = 0;
    speed = 100;
    display_widget=0;
    init(TRUE);
}

QMovieFilePrivate::~QMovieFilePrivate()
{
    if ( buffer )				// Avoid purify complaint
	delete [] buffer;
    delete pump;
    delete decoder;
    delete source;
}

bool QMovieFilePrivate::isNull() const
{
    return !buf_size;
}

// Initialize.  Only actually allocate any space if \a fully is TRUE,
// otherwise, just enough to be a valid null QMovieFilePrivate.
void QMovieFilePrivate::init(bool fully)
{
#ifdef QT_SAVE_MOVIE_HACK
    save_image = TRUE;
    image_number = 0;
#endif

    buf_usage = buf_r = buf_w = 0;
    if ( buffer )				// Avoid purify complaint
	delete [] buffer;
    buffer = fully ? new uchar[buf_size] : 0;
    if ( buffer )
	memset( buffer, 0, buf_size );

    delete decoder;
    decoder = fully ? new QImageDecoder(this) : 0;

#ifdef AVOID_OPEN_FDS
    if ( source && !source->isOpen() )
	source->open(IO_ReadOnly);
#endif

    waitingForFrameTick = FALSE;
    stepping = -1;
    framenumber = 0;
    frameperiod = -1;
    if (fully) frametimer->stop();
    lasttimerinterval = -1;
    changed_area.setRect(0,0,-1,-1);
    valid_area = changed_area;
    loop = -1;
    movie_ended = FALSE;
    error = 0;
    empty = TRUE;
}

void QMovieFilePrivate::flushBuffer()
{
    while (buf_usage && !waitingForFrameTick && stepping != 0 && !error) {
	int used = decoder->decode(buffer + buf_r,
			QMIN(buf_usage, buf_size - buf_r));
	if (used<=0) {
	    if ( used < 0 ) {
		error = 1;
		emit dataStatus(QMovie::UnrecognizedFormat);
	    }
	    break;
	}
	buf_r = (buf_r + used)%buf_size;
	buf_usage -= used;
    }

    // Some formats, like MNG can make stuff happen without any extra data.
    int used = decoder->decode(buffer + buf_r,0);
    if (used<=0) {
	if ( used < 0 ) {
	    error = 1;
	    emit dataStatus(QMovie::UnrecognizedFormat);
	}
    }

    if(error) frametimer->stop();
    maybeReady();
}

void QMovieFilePrivate::updatePixmapFromImage()
{
    if (changed_area.isEmpty()) return;
    updatePixmapFromImage(QPoint(0,0),changed_area);
}

void QMovieFilePrivate::updatePixmapFromImage(const QPoint& off,
						const QRect& area)
{
    // Create temporary QImage to hold the part we want
    const QImage& gimg = decoder->image();
    QImage img = gimg.copy(area);

#ifdef QT_SAVE_MOVIE_HACK
    if ( save_image ) {
	QString name;
	name.sprintf("movie%i.ppm",image_number++);
	gimg.save( name, "PPM" );
    }
#endif

    // Resize to size of image
    if (mypixmap.width() != gimg.width() || mypixmap.height() != gimg.height())
	mypixmap.resize(gimg.width(), gimg.height());

    if (bg.isValid()) {
	QPainter p;
	p.begin(&mypixmap);
	p.fillRect(area, bg);
	p.end();
    } else {
	if (gimg.hasAlphaBuffer()) {
	    // Resize to size of image
	    if (mymask.isNull()) {
		mymask.resize(gimg.width(), gimg.height());
		mymask.fill( Qt::color1 );
	    }
	}
	mypixmap.setMask(QBitmap()); // Remove reference to my mask
    }

    // Convert to pixmap and paste that onto myself
    QPixmap lines;
    lines.convertFromImage(img);
    bitBlt(&mypixmap, area.left(), area.top(),
	   &lines, off.x(), off.x(), area.width(), area.height(),
	   CopyROP, !bg.isValid());

    if (!bg.isValid() && gimg.hasAlphaBuffer()) {
	bitBlt(&mymask, area.left(), area.top(),
	       lines.mask(), 0, 0, area.width(),
	       area.height(),
	       CopyROP, TRUE);
	mypixmap.setMask(mymask);
    }

#ifdef _WS_QWS_
    if(display_widget) {
	QGfx * mygfx=display_widget->graphicsContext();
	if(mygfx) {
	    double xscale,yscale;
	    xscale=display_widget->width();
	    yscale=display_widget->height();
	    xscale=xscale/((double)mypixmap.width());
	    yscale=yscale/((double)mypixmap.height());
	    double xh,yh;
	    xh=xscale*((double)area.left());
	    yh=yscale*((double)area.top());
	    mygfx->setSource(&mypixmap);
	    mygfx->setAlphaType(QGfx::IgnoreAlpha);
	    mygfx->stretchBlt(0,0,display_widget->width(),
			      display_widget->height(),mypixmap.width(),
			      mypixmap.height());
	    delete mygfx;
	}
    }
#endif
}

void QMovieFilePrivate::showChanges()
{
    if (changed_area.isValid()) {
	updatePixmapFromImage();

	valid_area = valid_area.unite(changed_area);
	emit areaChanged(changed_area);

	changed_area.setWidth(-1); // make empty
    }
}

// QMovieFilePrivate as QImageConsumer
void QMovieFilePrivate::changed(const QRect& rect)
{
    if (!frametimer->isActive())
	frametimer->start(0);
    changed_area = changed_area.unite(rect);
}

void QMovieFilePrivate::end()
{
    movie_ended = TRUE;
}

void QMovieFilePrivate::preFrameDone()
{
    if (stepping > 0) {
	stepping--;
	if (!stepping) {
	    frametimer->stop();
	    emit dataStatus( QMovie::Paused );
	}
    } else {
	waitingForFrameTick = TRUE;
	restartTimer();
    }
}
void QMovieFilePrivate::frameDone()
{
    preFrameDone();
    showChanges();
    emit dataStatus(QMovie::EndOfFrame);
    framenumber++;
}
void QMovieFilePrivate::frameDone(const QPoint& p,
				const QRect& rect)
{
    preFrameDone();
    const QImage& gimg = decoder->image();
    if (framenumber==0)
	emit sizeChanged(gimg.size());
    valid_area = valid_area.unite(QRect(p,rect.size()));
    updatePixmapFromImage(p,rect);
    emit areaChanged(QRect(p,rect.size()));
    emit dataStatus(QMovie::EndOfFrame);
    framenumber++;
}

void QMovieFilePrivate::restartTimer()
{
    if (speed > 0) {
	int i = frameperiod >= 0 ? frameperiod * 100/speed : 0;
	if ( i != lasttimerinterval || !frametimer->isActive() ) {
	    lasttimerinterval = i;
	    frametimer->start( i );
	}
    } else {
	frametimer->stop();
    }
}

void QMovieFilePrivate::setLooping(int nloops)
{
    if (loop == -1) { // Only if we don't already know how many loops!
	if (source->rewindable()) {
	    source->enableRewind(TRUE);
	    loop = nloops;
	} else {
	    // Cannot loop from this source
	    loop = -2;
	}
    }
}

void QMovieFilePrivate::setFramePeriod(int milliseconds)
{
    // Animation:  only show complete frame
    frameperiod = milliseconds;
    if (stepping<0 && frameperiod >= 0) restartTimer();
}

void QMovieFilePrivate::setSize(int w, int h)
{
    if (mypixmap.width() != w || mypixmap.height() != h) {
	mypixmap.resize(w, h);
	emit sizeChanged(QSize(w, h));
    }
}


// QMovieFilePrivate as QDataSink

int QMovieFilePrivate::readyToReceive()
{
    // Could pre-fill buffer, but more efficient to just leave the
    // data back at the source.
    return (waitingForFrameTick || !stepping || buf_usage || error)
	? 0 : buf_size;
}

void QMovieFilePrivate::receive(const uchar* b, int bytecount)
{
    if ( bytecount ) empty = FALSE;

    while (bytecount && !waitingForFrameTick && stepping != 0) {
	int used = decoder->decode(b, bytecount);
	if (used<=0) {
	    if ( used < 0 ) {
		error = 1;
		emit dataStatus(QMovie::UnrecognizedFormat);
	    }
	    break;
	}
	b+=used;
	bytecount-=used;
    }

    // Append unused to buffer
    while (bytecount--) {
	buffer[buf_w] = *b++;
	buf_w = (buf_w+1)%buf_size;
	buf_usage++;
    }
}

void QMovieFilePrivate::eof()
{
    if ( !movie_ended )
	return;

    if ( empty )
	emit dataStatus(QMovie::SourceEmpty);

#ifdef QT_SAVE_MOVIE_HACK
    save_image = FALSE;
#endif

    emit dataStatus(QMovie::EndOfLoop);

    if (loop >= 0) {
	if (loop) {
	    loop--;
	    if (!loop) return;
	}
	delete decoder;
	decoder = new QImageDecoder(this);
	source->rewind();
	framenumber = 0;
    } else {
	delete decoder;
	decoder = 0;
	if ( buffer )				// Avoid purify complaint
	    delete [] buffer;
	buffer = 0;
	emit dataStatus(QMovie::EndOfMovie);
#ifdef AVOID_OPEN_FDS
	if ( source )
	    source->close();
#endif
    }
}

void QMovieFilePrivate::pause()
{
    if ( stepping ) {
	stepping = 0;
	frametimer->stop();
	emit dataStatus( QMovie::Paused );
    }
}

void QMovieFilePrivate::refresh()
{
    if (!decoder) {
	frametimer->stop();
	return;
    }

    if (frameperiod < 0 && loop == -1) {
	// Only show changes if probably not an animation
	showChanges();
    }

    if (!buf_usage) {
	frametimer->stop();
    }

    waitingForFrameTick = FALSE;
    flushBuffer();
}

///////////////// End of QMovieFilePrivate /////////////////





/*!
  Constructs a null QMovie.  The only interesting thing to do to such
  a movie is to assign another movie to it.

  \sa isNull()
*/
QMovie::QMovie()
{
    d = new QMovieFilePrivate();
}

/*!
  Constructs a QMovie with an external data source.
  You should later call pushData() to send incoming animation data to
  the movie.

  \sa pushData()
*/
QMovie::QMovie(int bufsize)
{
    d = new QMovieFilePrivate(0, this, bufsize);
}

/*!
  Returns the maximum amount of data that can currently be pushed
  into the movie by a call to pushData().  This is affected by the
  initial buffer size, but varies as the movie plays and data is consumed.
*/
int QMovie::pushSpace() const
{
    return d->readyToReceive();
}

/*!
  Pushes \a length bytes from \a data into the movie.  \a length must
  be no more than the amount returned by pushSpace() since the previous
  call to pushData().
*/
void QMovie::pushData(const uchar* data, int length)
{
    d->receive(data,length);
}

#ifdef _WS_QWS_ // ##### Temporary performance experiment
void QMovie::setDisplayWidget(QWidget * w)
{
    d->display_widget=w;
}
#endif

/*!
  Constructs a QMovie which reads an image sequence from the given
  QDataSource.  The source must be allocated dynamically,
  as it becomes owned by the QMovie, and will be destroyed
  when the movie is destroyed.
  The movie starts playing as soon as event processing continues.

  The \a bufsize argument sets the maximum amount of data the movie
  will transfer from the data source per event loop.  The lower this
  value, the better interleaved the movie playback will be with other
  event processing, but the slower the overall processing.
*/
QMovie::QMovie(QDataSource* src, int bufsize)
{
    d = new QMovieFilePrivate(src, this, bufsize);
}

/*!
  Constructs a QMovie which reads an image sequence from the named file.
*/
QMovie::QMovie(const QString &fileName, int bufsize)
{
    QFile* file = new QFile(fileName);
    file->open(IO_ReadOnly);
    d = new QMovieFilePrivate(new QIODeviceSource(file), this, bufsize);
}

/*!
  Constructs a QMovie which reads an image sequence from given data.
*/
QMovie::QMovie(QByteArray data, int bufsize)
{
    QBuffer* buffer = new QBuffer(data);
    buffer->open(IO_ReadOnly);
    d = new QMovieFilePrivate(new QIODeviceSource(buffer), this, bufsize);
}

/*!
  Constructs a movie that uses the same data as another movie.
  QMovies use explicit sharing, so operations on the copy will
  effect the same operations on the original.
*/
QMovie::QMovie(const QMovie& movie)
{
    d = movie.d;
    d->ref();
}

/*!
  Destroys the QMovie.  If this is the last reference to the data of the
  movie, that will also be destroyed.
*/
QMovie::~QMovie()
{
    if (d->deref()) delete d;
}

/*!
  Returns TRUE if the movie is null.
*/
bool QMovie::isNull() const
{
    return d->isNull();
}

/*!
  Makes this movie use the same data as another movie.
  QMovies use explicit sharing.
*/
QMovie& QMovie::operator=(const QMovie& movie)
{
    movie.d->ref();
    if (d->deref()) delete d;
    d = movie.d;
    return *this;
}


/*!
  Set the background color of the pixmap.  If the background color
  isValid(), the pixmap will never have a mask, as the background
  color will be used in transparent regions of the image.

  \sa backgroundColor()
*/
void QMovie::setBackgroundColor(const QColor& c)
{
    d->bg = c;
}

/*!
  Returns the background color of the movie set by setBackgroundColor().
*/
const QColor& QMovie::backgroundColor() const
{
    return d->bg;
}

/*!
  Returns the area of the pixmap for which pixels have been generated.
*/
const QRect& QMovie::getValidRect() const
{
    return d->valid_area;
}

/*!
  Returns the current frame of the movie, as a QPixmap.
  It is not generally useful to
  keep a copy of this pixmap.  Better to keep a copy of the QMovie and
  get the framePixmap() only when needed for drawing.

  \sa frameImage()
*/
const QPixmap& QMovie::framePixmap() const
{
    return d->mypixmap;
}

/*!
  Returns the current frame of the movie, as a QImage.
  It is not generally useful to keep a copy of this image.
  Also note that you must not call this function if the
  movie is finished(), as the image is not them available.

  \sa framePixmap()
*/
const QImage& QMovie::frameImage() const
{
    return d->decoder->image();
}

/*!
  Returns the number of steps remaining after a call to step(), 0 if paused,
  or a negative value if the movie is running normally or is finished.
*/
int QMovie::steps() const
{
    return d->stepping;
}

/*!
  Returns the number of times EndOfFrame has been emitted since the
  start of the current loop of the movie.  Thus, before
  any EndOfFrame has been emitted, the value will be 0,
  within slots processing the first signal, frameNumber() will be 1, and
  so on.
*/
int QMovie::frameNumber() const
{
    return d->framenumber;
}

/*!
  Returns TRUE if the image is paused.
*/
bool QMovie::paused() const
{
    return d->stepping == 0;
}

/*!
  Returns TRUE if the image is no longer playing - this happens when all
  loops of all frames is complete.
*/
bool QMovie::finished() const
{
    return !d->decoder;
}

/*!
  Returns TRUE if the image is not single-stepping, not paused,
  and not finished.
*/
bool QMovie::running() const
{
    return d->stepping<0 && d->decoder;
}

/*!
  Pauses the progress of the animation.

  \sa unpause()
*/
void QMovie::pause()
{
    d->pause();
}

/*!
  Unpauses the progress of the animation.

  \sa pause()
*/
void QMovie::unpause()
{
    if ( d->stepping >= 0 )	{
	if (d->isNull())
	    return;
	d->stepping = -1;
	d->restartTimer();
    }
}

/*!
  Steps forward, showing the given number of frames, then pauses.
*/
void QMovie::step(int steps)
{
    if (d->isNull())
	return;
    d->stepping = steps;
    d->frametimer->start(0);
    d->waitingForFrameTick = FALSE; // Full speed ahead!
}

/*!
  Steps forward 1 frame, then pauses.
*/
void QMovie::step()
{
    step(1);
}

/*!
  Rewinds the movie to the beginning.  If the movie has not been paused,
  it begins playing again.
*/
void QMovie::restart()
{
    if (d->isNull())
	return;
    if (d->source->rewindable()) {
	d->source->enableRewind(TRUE);
	d->source->rewind();
	int s = d->stepping;
	d->init(TRUE);
	if ( !s ) s = 1; // Don't pause or we'll not get to the FIRST frame
	if (s>0) step(s);
    }
}

/*!
  Returns the speed-up factor of the movie.  The default is 100 percent.
  \sa setSpeed()
*/
int QMovie::speed() const
{
    return d->speed;
}

/*!
  Sets the speed-up factor of the movie.  This is a percentage of the
  speed dictated by the input data format.  The default is 100 percent.
*/
void QMovie::setSpeed(int percent)
{
    int oldspeed = d->speed;
    if ( oldspeed != percent && percent >= 0 ) {
	d->speed = percent;
	// Restart timer only if really needed
	if (d->stepping < 0) {
	    if ( !percent || !oldspeed    // To or from zero
		 || oldspeed*4 / percent > 4   // More than 20% slower
		 || percent*4 / oldspeed > 4   // More than 20% faster
		 )
		d->restartTimer();
	}
    }
}

/*!
  Connects the given member, of type \code void member(const QSize&) \endcode
  such that it is signalled when the movie changes size.

  Note that due to the explicit sharing of QMovie objects, these connections
  persist until they are explicitly disconnected with disconnectResize(), or
  until \e every shared copy of the movie is deleted.
*/
void QMovie::connectResize(QObject* receiver, const char *member)
{
    QObject::connect(d, SIGNAL(sizeChanged(const QSize&)), receiver, member);
}

/*!
  Disconnects the given member, or all members if member is zero,
  previously connected by
  connectResize().
*/
void QMovie::disconnectResize(QObject* receiver, const char *member)
{
    QObject::disconnect(d, SIGNAL(sizeChanged(const QSize&)), receiver, member);
}

/*!
  Connects the given member, of type \code void member(const QRect&) \endcode
  such that it is signalled when an area of the framePixmap() has
  changed since the previous frame.

  Note that due to the explicit sharing of QMovie objects, these connections
  persist until they are explicitly disconnected with disconnectUpdate(), or
  until \e every shared copy of the movie is deleted.
*/
void QMovie::connectUpdate(QObject* receiver, const char *member)
{
    QObject::connect(d, SIGNAL(areaChanged(const QRect&)), receiver, member);
}

/*!
  Disconnects the given member, or all members if member is zero,
  previously connected by
  connectUpdate().
*/
void QMovie::disconnectUpdate(QObject* receiver, const char *member)
{
    QObject::disconnect(d, SIGNAL(areaChanged(const QRect&)), receiver, member);
}

/*!
  Connects the given member, of type \code void member(int) \endcode
  such that it is signalled when the movie changes status.  The status
  code are negative for errors and positive for information, and they
  are currently:

  <ul>
   <li> \c QMovie::SourceEmpty - signalled if the input cannot be read.
   <li> \c QMovie::UnrecognizedFormat - signalled if the input data is unrecognized.
   <li> \c QMovie::Paused - signalled when the movie is paused by a call to paused(),
			or by after \link step() stepping \endlink pauses.
   <li> \c QMovie::EndOfFrame - signalled at end-of-frame, after any update and Paused signals.
   <li> \c QMovie::EndOfLoop - signalled at end-of-loop, after any update signals,
				EndOfFrame, but before EndOfMovie.
   <li> \c QMovie::EndOfMovie - signalled when the movie completes and is not about
				 to loop.
  </ul>

  More status messages may be added in the future, so a general test for
  error would test for negative.

  Note that due to the explicit sharing of QMovie objects, these connections
  persist until they are explicitly disconnected with disconnectStatus(), or
  until \e every shared copy of the movie is deleted.
*/
void QMovie::connectStatus(QObject* receiver, const char *member)
{
    QObject::connect(d, SIGNAL(dataStatus(int)), receiver, member);
}

/*!
  Disconnects the given member, or all members if member is zero,
  previously connected by
  connectStatus().
*/
void QMovie::disconnectStatus(QObject* receiver, const char *member)
{
    QObject::disconnect(d, SIGNAL(dataStatus(int)), receiver, member);
}


#include "qmovie.moc"

#endif	// QT_NO_MOVIE

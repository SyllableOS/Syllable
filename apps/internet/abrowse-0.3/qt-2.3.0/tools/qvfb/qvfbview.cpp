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

#include "qvfbview.h"
#include "qvfbhdr.h"

#define QTE_PIPE "QtEmbedded-%1"

#include <qapplication.h>
#include <qimage.h>
#include <qbitmap.h>
#include <qtimer.h>
#include <qwmatrix.h>
#include "qanimationwriter.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <errno.h>

QVFbView::QVFbView( int display_id, int w, int h, int d, QWidget *parent,
		    const char *name, uint flags )
    : QScrollView( parent, name, flags ), lockId(-1)
{
    displayid = display_id;
    viewport()->setMouseTracking( TRUE );
    viewport()->setFocusPolicy( StrongFocus );
    zoom = 1;
    animation = 0;

    switch ( d ) {
	case 1:
	case 4:
	case 8:
	case 16:
	case 32:
	    break;
	
	default:
	    qFatal( "Unsupported bit depth %d\n", d );
    }

    mousePipe = QString(QT_VFB_MOUSE_PIPE).arg(display_id);
    keyboardPipe = QString(QT_VFB_KEYBOARD_PIPE).arg(display_id);

    unlink( mousePipe.latin1() );
    mknod( mousePipe.latin1(), S_IFIFO | 0666, 0 );
    mouseFd = open( mousePipe.latin1(), O_RDWR | O_NDELAY );
    if ( mouseFd == -1 ) {
	qFatal( "Cannot open mouse pipe" );
    }

    unlink( keyboardPipe );
    mknod( keyboardPipe, S_IFIFO | 0666, 0 );
    keyboardFd = open( keyboardPipe, O_RDWR | O_NDELAY );
    if ( keyboardFd == -1 ) {
	qFatal( "Cannot open keyboard pipe" );
    }

    key_t key = ftok( mousePipe.latin1(), 'b' );

    int bpl;
    if ( d == 1 )
	bpl = (w*d+7)/8;
    else
	bpl = ((w*d+31)/32)*4;
    
    int dataSize = bpl * h + 1024;
    shmId = shmget( key, dataSize, IPC_CREAT|0666);
    if ( shmId != -1 )
	data = (unsigned char *)shmat( shmId, 0, 0 );
    else {
	struct shmid_ds shm;
	shmctl( shmId, IPC_RMID, &shm );
	shmId = shmget( key, dataSize, IPC_CREAT|0666);
	data = (unsigned char *)shmat( shmId, 0, 0 );
    }

    if ( (int)data == -1 )
	qFatal( "Cannot attach to shared memory" );

    hdr = (QVFbHeader *)data;
    hdr->width = w;
    hdr->height = h;
    hdr->depth = d;
    hdr->linestep = bpl;
    hdr->numcols = 0;
    hdr->dataoffset = 1024;
    hdr->update = QRect();

    resizeContents( w, h );

    timer = new QTimer( this );
    connect( timer, SIGNAL(timeout()), this, SLOT(timeout()) );

    setRate( 30 );
}

QVFbView::~QVFbView()
{
    stopAnimation();
    sendKeyboardData( 0, 0, 0, TRUE, FALSE ); // magic die key
    struct shmid_ds shm;
    shmdt( (char*)data );
    shmctl( shmId, IPC_RMID, &shm );
    ::close( mouseFd );
    ::close( keyboardFd );
    unlink( mousePipe );
    unlink( keyboardPipe );
}

int QVFbView::displayId() const
{
    return displayid;
}

int QVFbView::displayWidth() const
{
    return hdr->width;
}

int QVFbView::displayHeight() const
{
    return hdr->height;
}

int QVFbView::displayDepth() const
{
    return hdr->depth;
}



void QVFbView::setZoom( double z )
{
    if ( zoom != z ) {
	zoom = z;
	setDirty(QRect(0,0,hdr->width,hdr->height));
	resizeContents( int(hdr->width*z), int(hdr->height*z) );
	updateGeometry();
	qApp->sendPostedEvents();
	topLevelWidget()->adjustSize();
	drawScreen();
    }
}

void QVFbView::setRate( int r )
{
    refreshRate = r;
    timer->start( 1000/r );
}

void QVFbView::initLock()
{
    QString username = "unknown";
    const char *logname = getenv("LOGNAME");
    if ( logname )
	username = logname;
    QString dataDir = "/tmp/qtembedded-" + username;

    QString pipe = dataDir + "/" + QString( QTE_PIPE ).arg( displayid );
    int semkey = ftok( pipe.latin1(), 'd' );
    lockId = semget( semkey, 0, 0 );
}

void QVFbView::lock()
{
    if ( lockId == -1 )
	initLock();

    sembuf sops;
    sops.sem_num = 0;
    sops.sem_flg = SEM_UNDO;
    sops.sem_op = -1;
    int rv;
    do {
	rv = semop(lockId,&sops,1);
    } while ( rv == -1 && errno == EINTR );

    if ( rv == -1 )
	lockId = -1;
}

void QVFbView::unlock()
{
    if ( lockId >= 0 ) {
	sembuf sops;
	sops.sem_num = 0;
	sops.sem_op = 1;
	sops.sem_flg = SEM_UNDO;
	int rv;
	do {
	    rv = semop(lockId,&sops,1);
	} while ( rv == -1 && errno == EINTR );
    }
}

void QVFbView::sendMouseData( const QPoint &pos, int buttons )
{
    write( mouseFd, &pos, sizeof( QPoint ) );
    write( mouseFd, &buttons, sizeof( int ) );
}

void QVFbView::sendKeyboardData( int unicode, int keycode, int modifiers,
				 bool press, bool repeat )
{
    QVFbKeyData kd;

    kd.unicode = unicode | (keycode << 16);
    kd.modifiers = modifiers;
    kd.press = press;
    kd.repeat = repeat;
    write( keyboardFd, &kd, sizeof( QVFbKeyData ) );
}

void QVFbView::timeout()
{
    lock();
    if ( animation ) {
	    QRect r( hdr->update );
	    r = r.intersect( QRect(0, 0, hdr->width, hdr->height ) );
	    if ( r.isEmpty() ) {
		animation->appendBlankFrame();
	    } else {
		int l;
		QImage img = getBuffer( r, l );
		animation->appendFrame(img,QPoint(r.x(),r.y()));
	    }
    }
    if ( hdr->dirty ) {
	drawScreen();
    }
    unlock();
}

QImage QVFbView::getBuffer( const QRect &r, int &leading ) const
{
    switch ( hdr->depth ) {
      case 16: {
	static unsigned char *imgData = 0;
	if ( !imgData ) {
	    int bpl = ((hdr->width*32+31)/32)*4;
	    imgData = new unsigned char [ bpl * hdr->height ];
	}
	QImage img( imgData, r.width(), r.height(), 32, 0, 0, QImage::IgnoreEndian );
	for ( int row = 0; row < r.height(); row++ ) {
	    QRgb *dptr = (QRgb*)img.scanLine( row );
	    ushort *sptr = (ushort*)(data + hdr->dataoffset + (r.y()+row)*hdr->linestep);
	    sptr += r.x();
	    for ( int col=0; col < r.width(); col++ ) {
		ushort s = *sptr++;
		*dptr++ = qRgb((s>>11)*255/31,((s>>5)&0x3f)*255/63,(s&0x1f)*255/31);
	    }
	}
	leading = 0;
	return img;
      }
      case 4: {
	static unsigned char *imgData = 0;
	if ( !imgData ) {
	    int bpl = ((hdr->width*8+31)/32)*4;
	    imgData = new unsigned char [ bpl * hdr->height ];
	}
	QImage img( imgData, r.width(), r.height(), 8, hdr->clut, 16,
		    QImage::IgnoreEndian );
	for ( int row = 0; row < r.height(); row++ ) {
	    unsigned char *dptr = img.scanLine( row );
	    unsigned char *sptr = data + hdr->dataoffset + (r.y()+row)*hdr->linestep;
	    sptr += r.x()/2;
	    int col = 0;
	    if ( r.x() & 1 ) {
		*dptr++ = *sptr++ >> 4;
		col++;
	    }
	    for ( ; col < r.width()-1; col+=2 ) {
		unsigned char s = *sptr++;
		*dptr++ = s & 0x0f;
		*dptr++ = s >> 4;
	    }
	    if ( !(r.right() & 1) )
		*dptr = *sptr & 0x0f;
	}
	leading = 0;
	return img;
      }
      case 32: {
	leading = r.x();
	return QImage( data + hdr->dataoffset + r.y() * hdr->linestep,
		    hdr->width, r.height(), hdr->depth, 0,
		    0, QImage::LittleEndian );
      }
      case 8: {
	leading = r.x();
	return QImage( data + hdr->dataoffset + r.y() * hdr->linestep,
		    hdr->width, r.height(), hdr->depth, hdr->clut,
		    256, QImage::LittleEndian );
      }
      case 1: {
	leading = r.x();
	return QImage( data + hdr->dataoffset + r.y() * hdr->linestep,
		    hdr->width, r.height(), hdr->depth, hdr->clut,
		    0, QImage::LittleEndian );
      }
    }
    return QImage();
}

void QVFbView::drawScreen()
{
    QPainter p( viewport() );

    p.translate( -contentsX(), -contentsY() );

    lock();
    QRect r( hdr->update );
    hdr->dirty = FALSE;
    hdr->update = QRect();
    // qDebug( "update %d, %d, %dx%d", r.y(), r.x(), r.width(), r.height() );
    r = r.intersect( QRect(0, 0, hdr->width, hdr->height ) );
    if ( !r.isEmpty() )  {
	if ( int(zoom) != zoom ) {
	    r.rLeft() = int(int(r.left()*zoom)/zoom);
	    r.rTop() = int(int(r.top()*zoom)/zoom);
	    r.rRight() = int(int(r.right()*zoom+zoom+0.0000001)/zoom+1.9999);
	    r.rBottom() = int(int(r.bottom()*zoom+zoom+0.0000001)/zoom+1.9999);
	    r.rRight() = QMIN(r.right(),hdr->width-1);
	    r.rBottom() = QMIN(r.bottom(),hdr->height-1);
	}
	int leading;
	QImage img( getBuffer( r, leading ) );
	QPixmap pm;
	if ( zoom == 1 ) {
	    pm.convertFromImage( img );
	} else if ( int(zoom) == zoom ) {
	    QWMatrix m;
	    m.scale(zoom,zoom);
	    pm.convertFromImage( img );
	    pm = pm.xForm(m);
	} else {
	    pm.convertFromImage( img.smoothScale(int(img.width()*zoom),int(img.height()*zoom)) );
	}
	unlock();
	p.setPen( black );
	p.setBrush( white );
	p.drawPixmap( int(r.x()*zoom), int(r.y()*zoom), pm,
			int(leading*zoom), 0, pm.width(), pm.height() );
    } else {
	unlock();
    }
}

bool QVFbView::eventFilter( QObject *obj, QEvent *e )
{
    if ( obj == viewport() &&
	 (e->type() == QEvent::FocusIn || e->type() == QEvent::FocusOut) )
	return TRUE;

    return QScrollView::eventFilter( obj, e );
}

void QVFbView::viewportPaintEvent( QPaintEvent *pe )
{
    QRect r( pe->rect() );
    r.moveBy( contentsX(), contentsY() );
    r = QRect(int(r.x()/zoom),int(r.y()/zoom),
	    int(r.width()/zoom)+1,int(r.height()/zoom)+1);
    setDirty(r);
    drawScreen();
}

void QVFbView::setDirty( const QRect& r )
{
    lock();
    hdr->update |= r;
    hdr->dirty = TRUE;
    unlock();
}

void QVFbView::contentsMousePressEvent( QMouseEvent *e )
{
    sendMouseData( e->pos()/zoom, e->stateAfter() );
}

void QVFbView::contentsMouseDoubleClickEvent( QMouseEvent *e )
{
    sendMouseData( e->pos()/zoom, e->stateAfter() );
}

void QVFbView::contentsMouseReleaseEvent( QMouseEvent *e )
{
    sendMouseData( e->pos()/zoom, e->stateAfter() );
}

void QVFbView::contentsMouseMoveEvent( QMouseEvent *e )
{
    sendMouseData( e->pos()/zoom, e->state() );
}



void QVFbView::keyPressEvent( QKeyEvent *e )
{
    sendKeyboardData(e->text()[0].unicode(), e->key(), 
		     e->state()&(ShiftButton|ControlButton|AltButton),
		     TRUE, e->isAutoRepeat());
}

void QVFbView::keyReleaseEvent( QKeyEvent *e )
{
    sendKeyboardData(e->ascii(), e->key(), 
		     e->state()&(ShiftButton|ControlButton|AltButton),
		     FALSE, e->isAutoRepeat());
}


QImage QVFbView::image() const
{
    ((QVFbView*)this)->lock();
    int l;
    QImage r = getBuffer( QRect(0, 0, hdr->width, hdr->height), l ).copy();
    ((QVFbView*)this)->unlock();
    return r;
}

void QVFbView::startAnimation( const QString& filename )
{
    delete animation;
    animation = new QAnimationWriter(filename,"MNG");
    animation->setFrameRate(refreshRate);
    animation->appendFrame(QImage(data + hdr->dataoffset,
                hdr->width, hdr->height, hdr->depth, hdr->clut,
                256, QImage::LittleEndian));
}

void QVFbView::stopAnimation()
{
    delete animation;
    animation = 0;
}


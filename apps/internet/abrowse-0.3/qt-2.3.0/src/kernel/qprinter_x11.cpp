/****************************************************************************
** $Id$
**
** Implementation of QPrinter class for X11
**
** Created : 950810
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
** licenses for Unix/X11 may use this file in accordance with the Qt Commercial
** License Agreement provided with the Software.
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

#include "qprinter.h"

#ifndef QT_NO_PRINTER

#include "qpaintdevicemetrics.h"
#include "qfile.h"
#include "qfileinfo.h"
#include "qdir.h"
#include "qpsprinter_p.h"
#include "qprintdialog.h"
#include "qapplication.h"
#include <stdlib.h>

#if defined(_OS_WIN32_)
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#endif

#if defined(_WS_X11_)
#include <X11/Xlib.h>
#endif

#if defined(_OS_OS2EMX_)
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#include <os2.h>
#endif

#if defined(_OS_QNX_)
#include <process.h>
#endif

// NOT REVISED

/*****************************************************************************
  QPrinter member functions
 *****************************************************************************/

// QPrinter states

#define PST_IDLE	0
#define PST_ACTIVE	1
#define PST_ERROR	2
#define PST_ABORTED	3


/*!
  Constructs a printer paint device.
*/

QPrinter::QPrinter()
    : QPaintDevice( QInternal::Printer | QInternal::ExternalDevice )
{
    pdrv = 0;
    pid = 0;
    orient = Portrait;
    page_size = A4;
    ncopies = 1;
    from_pg = to_pg = min_pg = max_pg = 0;
    state = PST_IDLE;
    output_file = FALSE;
    to_edge	= FALSE;
}

/*!
  Destructs the printer paint device and cleans up.
*/

QPrinter::~QPrinter()
{
    delete pdrv;
    if ( pid ) {
	(void)::kill( pid, 6 );
	(void)::wait( 0 );
	pid = 0;
    }
}


/*!
  Advances to a new page on the printer.
  Returns TRUE if successful, otherwise FALSE.
*/

bool QPrinter::newPage()
{
    if ( state == PST_ACTIVE && pdrv )
	return ((QPSPrinter*)pdrv)->cmd( QPSPrinter::NewPage, 0, 0 );
    return FALSE;
}


/*!
  Aborts the print job.
  Returns TRUE if successful, otherwise FALSE.
  \sa aborted()
*/

bool QPrinter::abort()
{
    if ( state == PST_ACTIVE && pdrv ) {
	((QPSPrinter*)pdrv)->cmd( QPSPrinter::AbortPrinting, 0, 0 );
	state = PST_ABORTED;
	if ( pid ) {
	    (void)::kill( pid, 6 );
	    (void)::wait( 0 );
	    pid = 0;
	}
    }
    return state == PST_ABORTED;
}

/*!
  Returns TRUE is the printer job was aborted, otherwise FALSE.
  \sa abort()
*/

bool QPrinter::aborted() const
{
    return state == PST_ABORTED;
}


/*!
  Opens a printer setup dialog and asks the user to specify what printer
  to use and miscellaneous printer settings.

  Returns TRUE if the user pressed "OK" to print, or FALSE if the
  user cancelled the operation.
*/

bool QPrinter::setup( QWidget * /*parent*/ )
{
    return QPrintDialog::getPrinterSetup( this );
}


/*!
  \internal
  Handles painter commands to the printer.
*/

bool QPrinter::cmd( int c, QPainter *paint, QPDevCmdParam *p )
{
    if ( c ==  PdcBegin ) {
	if ( state == PST_IDLE ) {
	    if ( output_file ) {
		int fd = 0;
#if defined(_OS_WIN32_)
		if ( qt_winver & Qt::WV_NT_based )
		    fd = _topen( qt_winTchar(output_filename,TRUE),
				_O_CREAT | _O_BINARY | _O_TRUNC | _O_WRONLY );
		else
		    fd = _open( output_filename.ascii(),
				_O_CREAT | _O_BINARY | _O_TRUNC | _O_WRONLY );
#else
		fd = ::open( output_filename.local8Bit(),
			     O_CREAT | O_NOCTTY | O_TRUNC | O_WRONLY,
			     0666 );
#endif
		if ( fd >= 0 ) {
		    pdrv = new QPSPrinter( this, fd );
		    state = PST_ACTIVE;
		}
	    } else {
		QString pr;
		if ( printer_name )
		    pr = printer_name;
#if defined(_OS_WIN32_)
		// Not implemented
		// lpr needs -Sserver argument
#else
		QApplication::flushX();
		int fds[2];
		if ( pipe( fds ) != 0 ) {
		    qWarning( "QPSPrinter: could not open pipe to print" );
		    state = PST_ERROR;
		    return FALSE;
		}
#if 0 && defined(_OS_OS2EMX_)
		// this code is still not used, and maybe it's not
		// usable either, any more.  if you want to use it,
		// you may need to fix it first.

		// old comment:

		// this code is usable but not in use.  spawn() is
		// preferable to fork()/exec() for very large
		// programs.  if fork()/exec() is a problem and you
		// use OS/2, remove '0 && ' from the #if.
		int tmp;
		tmp = dup(0);
		dup2( fds[0], 0 );
		::close( fds[0] );
		fcntl(tmp, F_SETFD, FD_CLOEXEC);
		fcntl(fds[1], F_SETFD, FD_CLOEXEC);
		pr.prepend( option_string ? option_string : "-P" ); // ###
		if ( spawnlp(P_NOWAIT,print_prog.data(), print_prog.data(),
			     pr.data(), output->name(), 0) == -1 ) {
		    ;			// couldn't exec, ignored
		}
		dup2( tmp, 0 );
		::close( tmp );
		pdrv = new QPSPrinter( this, fds[1] );
		state = PST_ACTIVE;
#else
		pid = fork();
		if ( pid == 0 ) {	// child process
		    // if possible, exit quickly, so the actual lp/lpr
		    // becomes a child of init, and ::waitpid() is
		    // guaranteed not to wait.
		    if ( fork() > 0 )
			exit( 0 );
		    dup2( fds[0], 0 );
#if defined(_WS_X11_)
		    // hack time... getting the maximum number of open
		    // files, if possible.  if not we assume it's the
		    // larger of 256 and the fd we got
		    int i;
#if defined(_OS_OS2EMX_)
		    LONG req_count = 0;
		    ULONG rc, handle_count;
		    rc = DosSetRelMaxFH (&req_count, &handle_count);
		    /* if (rc != NO_ERROR) ... */
		    i = (int)handle_count;
#elif defined(_SC_OPEN_MAX)
		    i = (int)sysconf( _SC_OPEN_MAX );
#elif defined(_POSIX_OPEN_MAX)
		    i = (int)_POSIX_OPEN_MAX;
#elif defined(OPEN_MAX)
		    i = (int)OPEN_MAX;
#else
		    i = QMAX( 256, fds[0] );
#endif // ways-to-set i
		    while( --i > 0 )
			::close( i );
#endif // _WS_X11_
		    if ( print_prog ) {
			pr.prepend( option_string ? option_string :
				    QString::fromLatin1( "-P" ) );
			(void)execlp( print_prog.ascii(), print_prog.ascii(),
				      pr.ascii(), 0 );
		    } else {
			// if no print program has been specified, be smart
			// about the option string too.
			const char * lprarg = 0;
			QString lprhack;
			const char * lparg = 0;
			QString lphack;
			if ( pr || option_string ) {
			    lprhack = pr;
			    lprhack.prepend( option_string ? option_string :
					     QString::fromLatin1( "-P" ) );
			    lprarg = lprhack.ascii();
			    lphack = pr;
			    lphack.prepend( option_string ? option_string :
					    QString::fromLatin1( "-d" ) );
			    lparg = lphack.ascii();
			}
			(void)execlp( "lp", "lp", lparg, 0 );
			(void)execlp( "lpr", "lpr", lprarg, 0 );
			(void)execl( "/bin/lp", "lp", lparg, 0 );
			(void)execl( "/bin/lpr", "lpr", lprarg, 0 );
			(void)execl( "/usr/bin/lp", "lp", lparg, 0 );
			(void)execl( "/usr/bin/lpr", "lpr", lprarg, 0 );
		    }
		    // if we couldn't exec anything, close the fd,
		    // wait for a second so the parent process (the
		    // child of the GUI process) has exited.  then
		    // exit.
		    ::close( 0 );
		    (void)::sleep( 1 );
		    ::exit( 0 );
		} else {		// parent process
		    ::close( fds[0] );
		    pdrv = new QPSPrinter( this, fds[1] );
		    state = PST_ACTIVE;
		}
#endif // else part of _OS_OS2EMX_
#endif // else part for #if _OS_WIN32_
	    }
	    if ( state == PST_ACTIVE && pdrv )
		return ((QPSPrinter*)pdrv)->cmd( c, paint, p );
	} else {
	    // ignore it?  I don't know
	}
    } else {
	bool r = FALSE;
	if ( state == PST_ACTIVE && pdrv ) {
	    r = ((QPSPrinter*)pdrv)->cmd( c, paint, p );
	    if ( c == PdcEnd ) {
		state = PST_IDLE;
		delete pdrv;
		pdrv = 0;
		if ( pid ) {
		    (void)::waitpid( pid, 0, 0 );
		    pid = 0;
		}
	    }
	}
	return r;
    }
    return TRUE;
}


/*!
  Internal implementation of the virtual QPaintDevice::metric() function.

  Use the QPaintDeviceMetrics class instead.

  \internal
  Hard coded return values for PostScript under X.
*/

int QPrinter::metric( int m ) const
{
    int val;
    PageSize s = pageSize();
#if defined(CHECK_RANGE)
    ASSERT( (uint)s < (uint)NPageSize );
#endif
    static int widths[]	 = { 595, 516, 612, 612, 541,
			     2384, 1684, 1191, 842, 420, 297, 210, 148, 105,
			     2920, 2064, 91, 1460, 1032, 729, 516, 363, 258,
			     181, 127, 461, 297, 312, 595, 1224, 792 };

    static int heights[] = { 842, 729, 792, 1009, 720,
			     3370, 2384, 1684, 1191, 595, 420, 297, 210, 148,
			     4127, 2920, 127, 2064, 1460, 1032, 729, 516, 363,
			     258, 181, 648, 684, 624, 935, 792, 1224 };
    switch ( m ) {
	case QPaintDeviceMetrics::PdmWidth:
	    val = orient == Portrait ? widths[ s ] : heights[ s ];
	    if ( !fullPage() )
		val -= 2*margins().width();
	    break;
	case QPaintDeviceMetrics::PdmHeight:
	    val = orient == Portrait ? heights[ s ] : widths[ s ];
	    if ( !fullPage() )
		val -= 2*margins().height();
	    break;
	case QPaintDeviceMetrics::PdmDpiX:
	    val = 72;
	    break;
	case QPaintDeviceMetrics::PdmDpiY:
	    val = 72;
	    break;
	case QPaintDeviceMetrics::PdmWidthMM:
	    val = metric( QPaintDeviceMetrics::PdmWidth );
	    val = (val * 254 + 360) / 720; // +360 to get the right rounding
	    break;
	case QPaintDeviceMetrics::PdmHeightMM:
	    val = metric( QPaintDeviceMetrics::PdmHeight );
	    val = (val * 254 + 360) / 720;
	    break;
	case QPaintDeviceMetrics::PdmNumColors:
	    val = 16777216;
	    break;
	case QPaintDeviceMetrics::PdmDepth:
	    val = 24;
	    break;
	default:
	    val = 0;
#if defined(CHECK_RANGE)
	    qWarning( "QPixmap::metric: Invalid metric command" );
#endif
    }
    return val;
}


/*!  Returns the width of the left/right and top/bottom margins of the
printer.  This is a best-effort guess, not based on perfect knowledge.

If you have called setFullPage( TRUE ) (this is recommended for
high-quality printing), margins().width() may be treated as the
smallest sane left/right margin you can use, and margins().height() as
the smallest sane top/bottom margins you can use.

If you have called setFullPage( FALSE ) (this is the default),
margins() is automatically subtracted from the pageSize() by QPrinter.

\sa setFullPage() QPaintDeviceMetrics PageSize
*/

QSize QPrinter::margins() const
{
    return (orient == Portrait) ? QSize( 36, 22 ) : QSize( 22, 36 );
}

#endif

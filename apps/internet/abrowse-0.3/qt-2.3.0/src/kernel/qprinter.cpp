/**********************************************************************
** $Id$
**
** Implementation of QPrinter class
**
** Created : 941003
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

#include "qprinter.h"

#ifndef QT_NO_PRINTER

// NOT REVISED
/*!
  \class QPrinter qprinter.h
  \brief The QPrinter class is a paint device that paint on a printer.

  \ingroup drawing

  On Windows, it uses the built-in printer drivers.  On X11, it
  generates postscript and sends that to lpr, lp or another print
  command.

  QPrinter is used much the same way that QWidget and QPixmap are
  used.  The big difference is that you must keep track of the pages.

  QPrinter supports a number of settable parameters, mostly related to
  pages, and most can be changed by the end user in when the
  application calls QPrinter::setup().

  The most important ones are: <ul>
  <li> setOrientation() tells QPrinter to turn the page (virtual).
  <li> setPageSize() tells QPrinter what page size to expect from the
  printer.
  <li> setFullPage() tells QPrinter whether you want to deal with the
  full page (so you can have accurate margins etc.) or with just with
  the part the printer can draw on.  The default is FALSE: You can
  probably paint on (0,0) but the document's margins are unknown.
  <li> setNumCopies() tells QPrinter how many copies of the document
  it should print.
  <li> setMinMax() tells QPrinter and QPrintDialog what the allowed
  range for fromPage() and toPage() are.
  </ul>

  There are also some settings that the user sets (through the printer
  dialog) and that applications are expected to obey: <ul>

  <li> pageOrder() tells the application program whether to print
  first-page-first or last-page-first.

  <li> colorMode() tells the application program whether to print
  in color or grayscale.  (If you print in color and the printer does
  not support color, Qt will try to approximate.  The document may
  take longer to print, but the quality should not be made visibly
  poorer.)

  <li> fromPage() and toPage() indicate what pages the application
  program should print.

  </ul>

  You can of course call e.g. setPageOrder() to establish a default
  before you ask the user through setup().

  Once you've started printing, newPage() is essential.  You will
  probably also need to look at the QPaintDeviceMetrics for the
  printer (see the <a href="simple-application.html#printer">simple
  print function</a> in the Application walk-through). Note that the
  paint device metrics are only valid after the QPrinter has been set
  up; i.e. after setup() has returned successfully. If you want
  high-quality printing with accurate margins, setFullPage( TRUE ) is
  a must.

  If you want to abort the print job, abort() will make a best effort.
  It may cancel the entire job, or just some of it.

  \internal Need a function to setup() without a dialog (ie. use defaults).
*/

/*! \enum QPrinter::Orientation

  This enum type (not to be confused with Qt::Orientation) is used to
  decide how Qt should print on each sheet.  <ul>

  <li> \c Portrait - (the default) means to print such that the page's
  height is greater than its width.

  <li> \c Landscape - means to print such that the page's width is
  greater than its height.

  </ul>

  This type interacts with QPrinter::PageSize and QPrinter::setFullPage()
  to determine the final size of the page available to the
  application.
*/


/*! \enum QPrinter::PageSize

  This enum type decides what paper size QPrinter is to use.  QPrinter
  does not check that the paper size is available; it just uses this
  information together with Orientation and QPrinter::setFullPage() to
  determine the printable area (see QPaintDeviceMetrics).

  The defined sizes (with setFullPage( TRUE )) are <ul>
  <li>\c QPrinter::A0 (841 x 1189 mm)
  <li>\c QPrinter::A1 (594 x 841 mm)
  <li>\c QPrinter::A2 (420 x 594 mm)
  <li>\c QPrinter::A3 (297 x 420 mm)
  <li>\c QPrinter::A4 (210x297 mm, 8.26x11.7 inches)
  <li>\c QPrinter::A5 (148 x 210 mm)
  <li>\c QPrinter::A6 (105 x 148 mm)
  <li>\c QPrinter::A7 (74 x 105 mm)
  <li>\c QPrinter::A8 (52 x 74 mm)
  <li>\c QPrinter::A9 (37 x 52 mm)
  <li>\c QPrinter::B0 (1030 x 1456 mm)
  <li>\c QPrinter::B1 (728 x 1030 mm)
  <li>\c QPrinter::B10 (32 x 45 mm)
  <li>\c QPrinter::B2 (515 x 728 mm)
  <li>\c QPrinter::B3 (364 x 515 mm)
  <li>\c QPrinter::B4 (257 x 364 mm)
  <li>\c QPrinter::B5 (182x257 mm, 7.17x10.13 inches)
  <li>\c QPrinter::B6 (128 x 182 mm)
  <li>\c QPrinter::B7 (91 x 128 mm)
  <li>\c QPrinter::B8 (64 x 91 mm)
  <li>\c QPrinter::B9 (45 x 64 mm)
  <li>\c QPrinter::C5E (163 x 229 mm)
  <li>\c QPrinter::Comm10E (105 x 241 mm, US Common #10 Envelope)
  <li>\c QPrinter::DLE (110 x 220 mm)
  <li>\c QPrinter::Executive (7.5x10 inches, 191x254 mm)
  <li>\c QPrinter::Folio (210 x 330 mm)
  <li>\c QPrinter::Ledger (432 x 279 mm)
  <li>\c QPrinter::Legal (8.5x14 inches, 216x356 mm)
  <li>\c QPrinter::Letter (8.5x11 inches, 216x279 mm)
  <li>\c QPrinter::Tabloid (279 x 432 mm)
  </ul>

  With setFullPage( FALSE ) (the default), the metrics will be a bit
  smaller.  How much depends on the printer in use.
*/


/*! \enum QPrinter::PageOrder

  This enum type is used by QPrinter/QPrintDialog to tell the
  application program how to print.  The possible values are <ul>

  <li> \c QPrinter::FirstPageFirst - the lowest-numbered page should
  be printed first.

  <li> \c QPrinter::LastPageFirst - the highest-numbered page should
  be printed first.

  </ul>
*/

/*! \enum QPrinter::ColorMode

  This enum type is used to indicate whether QPrinter should print in
  color or not.  The possible values are: <ul>

  <li> \c Color - print in color if available, else in grayscale.  This
  is the default.

  <li> \c GrayScale - print in grayscale, even on color printers.
  Might be a little faster than \c Color.

  </ul>
*/


/*!
  \fn QString QPrinter::printerName() const

  Returns the printer name.  This value is initially set to the name of the
  default printer.

  \sa setPrinterName()
*/

/*!
  Sets the printer name.

  The default printer will be used if no printer name is set.

  Under X11, the PRINTER environment variable defines the
  default printer.  Under any other window system, the window
  system defines the default printer.

  \sa printerName()
*/

void QPrinter::setPrinterName( const QString &name )
{
    if ( state != 0 ) {
#if defined(CHECK_STATE)
	qWarning( "QPrinter::setPrinterName: Cannot do this during printing" );
#endif
	return;
    }
    printer_name = name;
}


/*!
  \fn bool QPrinter::outputToFile() const
  Returns TRUE if the output should be written to a file, or FALSE if the
  output should be sent directly to the printer.
  The default setting is FALSE.

  This function is currently only supported under X11.

  \sa setOutputToFile(), setOutputFileName()
*/

/*!
  Specifies whether the output should be written to a file or sent
  directly to the printer.

  Will output to a file if \e enable is TRUE, or will output directly
  to the printer if \e enable is FALSE.

  This function is currently only supported under X11.

  \sa outputToFile(), setOutputFileName()
*/

void QPrinter::setOutputToFile( bool enable )
{
    if ( state != 0 ) {
#if defined(CHECK_STATE)
	qWarning( "QPrinter::setOutputToFile: Cannot do this during printing" );
#endif
	return;
    }
    output_file = enable;
}


/*!
  \fn QString QPrinter::outputFileName() const
  Returns the name of the output file.	There is no default file name.
  \sa setOutputFileName(), setOutputToFile()
*/

/*!
  Sets the name of the output file.

  Setting a null name (0 or "") disables output to a file, i.e.
  calls setOutputToFile(FALSE);
  Setting non-null name enables output to a file, i.e.  calls
  setOutputToFile(TRUE).

  This function is currently only supported under X11.

  \sa outputFileName(), setOutputToFile()
*/

void QPrinter::setOutputFileName( const QString &fileName )
{
    if ( state != 0 ) {
#if defined(CHECK_STATE)
	qWarning("QPrinter::setOutputFileName: Cannot do this during printing");
#endif
	return;
    }
    output_filename = fileName;
    output_file = !output_filename.isEmpty();
}


/*!
  \fn QString QPrinter::printProgram() const

  Returns the name of the program that sends the print output to the printer.

  The default is to return a null string; meaning that QPrinter will
  try to be smart in a system-dependent way.  On X11 only, you can set
  it to something different to use a specific print program.

  On Windows, this function returns the name of the printer device driver.

  \sa setPrintProgram() setPrinterSelectionOption()
*/

/*!
  Sets the name of the program that should do the print job.

  On X11, this function sets the program to call with the PostScript
  output.  On other platforms, it has no effect.

  \sa printProgram()
*/

void QPrinter::setPrintProgram( const QString &printProg )
{
    print_prog = printProg;
}


/*!
  \fn QString QPrinter::docName() const
  Returns the document name.
  \sa setDocName()
*/

/*!
  Sets the document name.
*/

void QPrinter::setDocName( const QString &name )
{
    if ( state != 0 ) {
#if defined(CHECK_STATE)
	qWarning( "QPrinter::setDocName: Cannot do this during printing" );
#endif
	return;
    }
    doc_name = name;
}


/*!
  \fn QString QPrinter::creator() const
  Returns the creator name.
  \sa setCreator()
*/

/*!
  Sets the creator name.

  Calling this function only has effect for the X11 version of Qt.
  The creator name is the name of the application that created the
  document.  If no creator name is specified, then the creator will be
  set to "Qt" with some version number.

  \sa creator()
*/

void QPrinter::setCreator( const QString &creator )
{
    creator_name = creator;
}


/*!
  \fn Orientation QPrinter::orientation() const

  Returns the orientation setting.  The default value is \c
  QPrinter::Portrait.
  \sa setOrientation()
*/

/*!
  Sets the print orientation.

  The orientation can be either \c QPrinter::Portrait or
  \c QPrinter::Landscape.

  The printer driver reads this setting and prints using the specified
  orientation.  On Windows however, this setting won't take effect until 
  the printer dialog is shown (using QPrinter::setup() ).

  \sa orientation()
*/

void QPrinter::setOrientation( Orientation orientation )
{
    orient = orientation;
}


/*!
  \fn PageSize QPrinter::pageSize() const

  Returns the printer page size.  The default value is system-dependent.

  \sa setPageSize()
*/


static QPrinter::PageSize makepagesize( QPrinter::PageSize ps,
					QPrinter::PageOrder po,
					QPrinter::ColorMode cm )
{
    return (QPrinter::PageSize)( ((int)ps & 255) +
				 ((po == QPrinter::LastPageFirst) ? 256 : 0) +
				 ((cm == QPrinter::GrayScale) ? 512 : 0) );
}



/*!
  Sets the printer page size to \a newPageSize.

  The default page size is system-dependent.

  This function is useful mostly for setting a default value that the
  user can override in the print dialog when you call setup().

  \sa pageSize() PageSize setFullPage()
*/

void QPrinter::setPageSize( PageSize newPageSize )
{
    if ( newPageSize > NPageSize ) {
#if defined(CHECK_STATE)
	qWarning("QPrinter::SetPageSize: illegal page size %d", newPageSize );
#endif
	return;
    }
    page_size = makepagesize( newPageSize, pageOrder(), colorMode() );
}


/*!  Sets the page order to \a newPageOrder.

  The page order can be \c QPrinter::FirstPageFirst or \c
  QPrinter::LastPageFirst.  The application programmer is responsible
  for reading the page order and printing accordingly.

  This function is useful mostly for setting a default value that the
  user can override in the print dialog when you call setup().
*/

void QPrinter::setPageOrder( PageOrder newPageOrder )
{
    page_size = makepagesize( pageSize(), newPageOrder, colorMode() );
}


/*!  Returns the current page order.

  The default page order is \a FirstPageFirst.
*/

QPrinter::PageOrder QPrinter::pageOrder() const
{
    if ( ((int)page_size) & 256 )
	return QPrinter::LastPageFirst;
    else
	return QPrinter::FirstPageFirst;
}


/*!  Sets the printer's color mode to \a newColorMode, which can be
  one of \c Color (the default) and \c GrayScale.

  A future version of Qt will modify its printing accordingly.  At
  present, QPrinter behaves as if \c Color is selected.

  \sa colorMode()
*/

void QPrinter::setColorMode( ColorMode newColorMode )
{
    page_size = makepagesize( pageSize(), pageOrder(), newColorMode );
}


/*!  Returns the current color mode.  The default color mode is \c
  Color.

  \sa setColorMode()
*/

QPrinter::ColorMode QPrinter::colorMode() const
{
    if ( ((int)page_size) & 512 )
	return QPrinter::GrayScale;
    else
	return QPrinter::Color;


}


/*!
  \fn int QPrinter::fromPage() const
  Returns the from-page setting.  The default value is 0.

  The programmer is responsible for reading this setting and print
  accordingly.

  \sa setFromTo(), toPage()
*/

/*!
  \fn int QPrinter::toPage() const
  Returns the to-page setting.  The default value is 0.

  The programmer is responsible for reading this setting and print
  accordingly.

  \sa setFromTo(), fromPage()
*/

/*!
  Sets the from page and to page.

  The from-page and to-page settings specify what pages to print.

  This function is useful mostly for setting a default value that the
  user can override in the print dialog when you call setup().

  \sa fromPage(), toPage(), setMinMax(), setup()
*/

void QPrinter::setFromTo( int fromPage, int toPage )
{
    if ( state != 0 ) {
#if defined(CHECK_STATE)
	qWarning( "QPrinter::setFromTo: Cannot do this during printing" );
#endif
	return;
    }
    from_pg = fromPage;
    to_pg = toPage;
}


/*!
  \fn int QPrinter::minPage() const
  Returns the min-page setting.	 The default value is 0.
  \sa maxPage(), setMinMax()
*/

/*!
  \fn int QPrinter::maxPage() const
  Returns the max-page setting.	 The default value is 0.
  \sa minPage(), setMinMax()
*/

/*!
  Sets the min page and max page.

  The min-page and max-page restrict the from-page and to-page settings.
  When the printer setup dialog comes up, the user cannot select
  from and to that are outside the range specified by min and max pages.

  \sa minPage(), maxPage(), setFromTo(), setup()
*/

void QPrinter::setMinMax( int minPage, int maxPage )
{
    min_pg = minPage;
    max_pg = maxPage;
}


/*!
  \fn int QPrinter::numCopies() const
  Returns the number of copies to be printed.  The default value is 1.
  \sa setNumCopies()
*/

/*!
  Sets the number of pages to be printed.

  The printer driver reads this setting and prints the specified number of
  copies.

  \sa numCopies(), setup()
*/

void QPrinter::setNumCopies( int numCopies )
{
    ncopies = numCopies;
}


/*!  Returns the printer options selection string.  This is only
useful if the print command has been explicitly set.

The default value (a null string) implies to select printer in a
system-dependent manner.

Any other value implies to use that value.

\sa setPrinterSelectionOption()
*/

QString QPrinter::printerSelectionOption() const
{
    return option_string;
}


/*!  Sets the printer to use \a option to select printer.  \a option
is null by default, meaning to be a little smart, but can be set to
other values to use a specific printer selection option.

If the printer selection option is changed while the printer is
active, the current print job may or may not be affected.
*/

void QPrinter::setPrinterSelectionOption( const QString & option )
{
    option_string = option;
}


/*!  Sets QPrinter to have the origin of the coordinate system at the
top left corner of the paper if \a fp is TRUE, or where it thinks the
top left corner of the printable area is if \a fp is FALSE.

The default is FALSE: You can (probably) print on (0,0), and
QPaintDeviceMetrics will report something smaller than the size
indicated by PageSize.  (Note that QPrinter may be wrong - it does not
have perfect knowledge of the physical printer.)

If you set it to TRUE, QPaintDeviceMetrics will report the exact same
size as indicated by PageSize, but you cannot print on all of that -
you have to take care of the output margins yourself.

If the page-size mode is changed while the printer is active, the
current print job may or may not be affected.

\sa PageSize setPageSize() QPaintDeviceMetrics fullPage()
*/

void QPrinter::setFullPage( bool fp )
{
    to_edge = fp;
}


/*!  Returns TRUE if the origin of the printer's coordinate system is
at the corner of the sheet, and FALSE if it is at the edge of the
printable area.

See setFullPage() for more detail and some warnings.

\sa setFullPage() PageSize QPaintDeviceMetrics
*/

bool QPrinter::fullPage() const
{
    return to_edge;
}

#endif // QT_NO_PRINTER

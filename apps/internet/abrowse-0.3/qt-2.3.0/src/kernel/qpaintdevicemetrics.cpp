/****************************************************************************
** $Id$
**
** Implementation of QPaintDeviceMetrics class
**
** Created : 941109
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

#include "qpaintdevicemetrics.h"

// NOT REVISED
/*!
  \class QPaintDeviceMetrics qpaintdevicemetrics.h
  \brief The QPaintDeviceMetrics class provides information about a
  paint device.

  \ingroup drawing

  Sometimes it is necessary to obtain information about the physical
  characteristics of a paint device when drawing graphics.  This class
  provides just that.  For example, to compute the aspect ratio of a
  paint device:

  \code
    QPaintDeviceMetrics pdm( myWidget );
    double aspect = (double)pdm.widthMM() / (double)pdm.heightMM();
  \endcode

  QPaintDeviceMetrics contains methods to provide the width and height
  of a device in both pixels and millimeters, the number of colors
  the device supports, the number of bit planes, and finally the
  resolution of the device.

  Note that it is not always possible for QPaintDeviceMetrics to
  compute the values you ask for, particularly for external devices.
  The ultimate example is asking for the resolution of a QPrinter
  that is set to "print to file" - who knows what printer that file
  will end up?

*/

/*!
  Constructs a metric for the paint device \e pd.
*/
QPaintDeviceMetrics::QPaintDeviceMetrics( const QPaintDevice *pd )
{
    pdev = (QPaintDevice *)pd;
}


/*!
  \fn int QPaintDeviceMetrics::width() const

  Returns the width of the paint device, in default coordinate system
  units (e.g. pixels for QPixmap and QWidget).
*/

/*!
  \fn int QPaintDeviceMetrics::height() const

  Returns the height of the paint device, in default coordinate system
  units (e.g. pixels for QPixmap and QWidget).
*/

/*!
  \fn int QPaintDeviceMetrics::widthMM() const
  Returns the width of the paint device, measured in millimeters.
*/

/*!
  \fn int QPaintDeviceMetrics::heightMM() const
  Returns the height of the paint device, measured in millimeters.
*/

/*!
  \fn int QPaintDeviceMetrics::numColors() const
  Returns the number of different colors available for the paint device.
*/

/*!
  \fn int QPaintDeviceMetrics::depth() const
  Returns the bit depth (number of bit planes) of the paint device.
*/

/*!
  \fn int QPaintDeviceMetrics::logicalDpiX() const

  Returns the horizontal resolution of the device, in dots per inch, that is
  used when computing font sizes.
  For X this is usually the same as could be computed
  from widthMM(), but on Windows this varies.
*/

/*!
  \fn int QPaintDeviceMetrics::logicalDpiY() const

  Returns the vertical resolution of the device, in dots per inch, that is
  used when computing font sizes.
  For X this is usually the same as could be computed
  from heightMM(), but on Windows this varies.
*/

/****************************************************************************
** $Id$
**
** Implementation of QColorGroup and QPalette classes
**
** Created : 950323
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

#include "qpalette.h"

#ifndef QT_NO_PALETTE
#ifndef __ATHEOS__
#include "qdatastream.h"
#endif
#include "qpixmap.h"

/*****************************************************************************
  QColorGroup member functions
 *****************************************************************************/

// REVISED - arnt
/*!
  \class QColorGroup qpalette.h
  \brief The QColorGroup class contains a group of widget colors.

  \ingroup appearance
  \ingroup drawing

  A color group contains a group of colors used by widgets for drawing
  themselves.  Widgets should not use colors like "red" and "turqoise"
  but rather "foreground" and "base", where possible.  The color roles
  are enumerated and defined in the ColorRole documentation.

  The most common usage of QColorGroup is like this:

  \code
    QPainter p;
    ...
    p.setPen( colorGroup().foreground() );
    p.drawLine( ... )
  \endcode

  See the \l ColorRole documentation below for more details on roles.

  It's also possible to modify color groups or create them from scratch.

  The color group class can be created using three different
  constructors, or by modifying one supplied by the system.  The
  default constructor creates an all-black color group, which can then
  be modified using set functions.  There are two functions that take
  long lists of arguments (slightly different lists - beware!).  And
  there is the copy constructor.

  We strongly recommend using a system-supplied color group, and
  modifying that as necessary.

  You modify a color group by calling the access functions setColor()
  and setBrush(), depending on whether you want a pure color or e.g. a
  pixmap pattern.

  There are also corresponding color() and brush() getters, and a
  commonly used convenience function to get each ColorRole:
  background(), foreground(), base() and so on.

  \sa QColor QPalette QWidget::colorGroup()
*/

/*! \enum QColorGroup::ColorRole

  The ColorRole enum defines the different symbolic color roles used
  in current GUIs.  The central roles are:

  <ul>
  <li> \c Background - general background color.

  <li> \c Foreground - general foreground color.

  <li> \c Base - used as background color for e.g. text entry widgets,
  usually white or another light color.

  <li> \c Text - the foreground color used with \c Base. Usually this
  is the same as the \c Foreground, in what case it must provide good
  contrast both with \c Background and \c Base.

  <li> \c Button - general button background color, where buttons need a
  background different from \c Background, as in the Macintosh style.

  <li> \c ButtonText - a foreground color used with the \c Button color.

  </ul> There are some color roles used mostly for 3D bevel and shadow
  effects: <ul>

  <li> \c Light - lighter than \c Button color.

  <li> \c Midlight - between \c Button and \c Light.

  <li> \c Dark - darker than \c Button.

  <li> \c Mid - between \c Button and \c Dark.

  <li> \c Shadow - a very dark color.

  </ul> All of these are normally derived from \c Background, and used
  in ways that depend on that relationship.  For example, buttons
  depend on it to make the bevels look good, and Motif scroll bars
  depend on \c Mid to be slightly different from \c Background.

  Selected (marked) items have two roles: <ul>

  <li> \c Highlight  - a color to indicate a selected or highlighted item.

  <li> \c HighlightedText - a text color that contrasts to \c Highlight.

  </ul> Finally, there is a special role for text that needs to be
  drawn where \c Text or \c Foreground would provide bad contrast,
  such as on pressed push buttons: <ul>

  <li> \c BrightText - a text color that is very different from \c
  Foreground and contrasts well with e.g. \c Dark.

  </ul>

  Note that text colors can be used for other things than just words:
  text colors are \e usually used for text, but it's quite common to
  have lines, icons and so on that belong with a text color logically.

  This image shows most of the color roles in use:
  <img src="palette.png" width="603" height="166" alt="">
*/




/*!
  Constructs a color group with all colors set to black.
*/

QColorGroup::QColorGroup()
{
    br = new QBrush[NColorRoles];	// all colors become black

    // The d pointer may allow sharing in the future.  The br pointer
    // then will be a redundant pointer that makes possible the inlines
    // in the header file.  Don't forget to add delete d in the destructor.
    // QPalette, the main QColorGroup user, is already shared though,
    // so perhaps not much is to be gained.
    d = 0;
}

/*!
  Constructs a color group that is an independent copy of \a other.
*/
QColorGroup::QColorGroup( const QColorGroup& other )
{
    br = new QBrush[NColorRoles];
    for (int i=0; i<NColorRoles; i++)
	br[i] = other.br[i];
    d = 0;
}

/*!
  Copies the colors of \a other to this color group.
*/
QColorGroup& QColorGroup::operator =(const QColorGroup& other)
{
    for (int i=0; i<NColorRoles; i++)
	br[i] = other.br[i];
    return *this;
}

static QColor qt_mix_colors( QColor a, QColor b)
{
    return QColor( (a.red() + b.red()) / 2, (a.green() + b.green()) / 2, (a.blue() + b.blue()) / 2 );
}


/*!
Constructs a color group. You can pass either brushes, pixmaps or
plain colors for each parameter.

This constructor can be very handy sometimes, but don't overuse it:
Such long lists of arguments are rather error-prone.

\sa QBrush
*/
 QColorGroup::QColorGroup( const QBrush &foreground, const QBrush &button,
			   const QBrush &light, const QBrush &dark,
			   const QBrush &mid, const QBrush &text,
			   const QBrush &bright_text, const QBrush &base,
			   const QBrush &background)
{
    br = new QBrush[NColorRoles];
    br[Foreground]      = foreground;
    br[Button] 	 	= button;
    br[Light] 		= light;
    br[Dark] 		= dark;
    br[Mid] 		= mid;
    br[Text] 		= text;
    br[BrightText] 	= bright_text;
    br[ButtonText] 	= text;
    br[Base] 		= base;
    br[Background] 	= background;
    br[Midlight] 	= qt_mix_colors( br[Button].color(), br[Light].color() );
    br[Shadow]          = Qt::black;
    br[Highlight]       = Qt::darkBlue;
    br[HighlightedText] = Qt::white;
}


/*!\obsolete

  Constructs a color group with the specified colors. The button
  color will be set to the background color.
*/

QColorGroup::QColorGroup( const QColor &foreground, const QColor &background,
			  const QColor &light, const QColor &dark,
			  const QColor &mid,
			  const QColor &text, const QColor &base )
{
    br = new QBrush[NColorRoles];
    br[Foreground]      = QBrush(foreground);
    br[Button]          = QBrush(background);
    br[Light]           = QBrush(light);
    br[Dark]            = QBrush(dark);
    br[Mid]             = QBrush(mid);
    br[Text]            = QBrush(text);
    br[BrightText]      = br[Light];
    br[ButtonText]      = br[Text];
    br[Base]            = QBrush(base);
    br[Background]      = QBrush(background);
    br[Midlight] 	= qt_mix_colors( br[Button].color(), br[Light].color() );
    br[Shadow]          = Qt::black;
    br[Highlight]       = Qt::darkBlue;
    br[HighlightedText] = Qt::white;
}

/*!
  Destructs the color group.
*/

QColorGroup::~QColorGroup()
{
    delete [] br;
}

/*!
  Returns the color that has been set for color role \a r.
  \sa brush() ColorRole
 */
const QColor &QColorGroup::color( ColorRole r ) const
{
    return br[r].color();
}

/*!
  Returns the brush that has been set for color role \a r.

  \sa color() setBrush() ColorRole
*/
const QBrush &QColorGroup::brush( ColorRole r ) const
{
    return br[r];
}

/*!
  Sets the brush used for color role \a r to a solid color \a c.

  \sa brush() setColor() ColorRole
*/
void QColorGroup::setColor( ColorRole r, const QColor &c )
{
    setBrush( r, QBrush(c) );
}

/*!
  Sets the brush used for color role \a r to \a b.

  \sa brush() setColor() ColorRole
*/
void QColorGroup::setBrush( ColorRole r, const QBrush &b )
{
    br[r] = b;
}


/*!
  \fn const QColor & QColorGroup::foreground() const

  Returns the foreground color of the color group.

  \sa ColorRole
*/

/*!
  \fn const QColor & QColorGroup::button() const
  Returns the button color of the color group.

  \sa ColorRole
*/

/*!
  \fn const QColor & QColorGroup::light() const
  Returns the light color of the color group.

  \sa ColorRole
*/

/*!
  \fn const QColor& QColorGroup::midlight() const
  Returns the midlight color of the color group.

  \sa ColorRole
*/

/*!
  \fn const QColor & QColorGroup::dark() const
  Returns the dark color of the color group.

  \sa ColorRole
*/

/*!
  \fn const QColor & QColorGroup::mid() const
  Returns the medium color of the color group.

  \sa ColorRole
*/

/*!
  \fn const QColor & QColorGroup::text() const
  Returns the text foreground color of the color group.

  \sa ColorRole
*/

/*!
  \fn const QColor & QColorGroup::brightText() const
  Returns the bright text foreground color of the color group.

  \sa ColorRole
*/

/*!
  \fn const QColor & QColorGroup::buttonText() const
  Returns the button text foreground color of the color group.

  \sa ColorRole
*/

/*!
  \fn const QColor & QColorGroup::base() const
  Returns the base color of the color group.

  \sa ColorRole
*/

/*!
  \fn const QColor & QColorGroup::background() const
  Returns the background color of the color group.

  \sa ColorRole
*/

/*!
  \fn const QColor & QColorGroup::shadow() const
  Returns the shadow color of the color group.

  \sa ColorRole
*/

/*!
  \fn const QColor & QColorGroup::highlight() const
  Returns the highlight color of the color group.

  \sa ColorRole
*/

/*!
  \fn const QColor & QColorGroup::highlightedText() const
  Returns the highlighted text color of the color group.

  \sa ColorRole
*/

/*!
  \fn bool QColorGroup::operator!=( const QColorGroup &g ) const
  Returns TRUE if this color group is different from \e g, or FALSE if
  it is equal to \e g.
  \sa operator!=()
*/

/*!
  Returns TRUE if this color group is equal to \e g, or FALSE if
  it is different from \e g.
  \sa operator==()
*/

bool QColorGroup::operator==( const QColorGroup &g ) const
{
    for( int r = 0 ; r < NColorRoles ; r++ )
	if ( br[r] != g.br[r] )
	    return FALSE;
    return TRUE;
}


/*****************************************************************************
  QPalette member functions
 *****************************************************************************/

/*!
  \class QPalette qpalette.h

  \brief The QPalette class contains color groups for each widget state.

  \ingroup appearance
  \ingroup shared
  \ingroup drawing

  A palette consists of three color groups: a \e active, a \e disabled
  and an \e inactive color group.  All widgets contain a palette, and
  all the widgets in Qt use their palette to draw themselves.  This
  makes the user interface consistent and easily configurable.

  If you make a new widget you are strongly advised to use the colors in
  the palette rather than hard-coding specific colors.

  The color groups are: <ul> <li> The active() group is used for the
  window that has keyboard focus. <li> The inactive() group is used
  for other windows. <li> The disabled() group is used for widgets
  (not windows) that are disabled for some reason. </ul>

  Of course, both active and inactive windows can contain disabled
  widgets.  (Disabled widgets are often called \e inaccessible or \e
  grayed \e out.)

  In Motif style, active() and inactive() look precisely the same.  In
  Windows 2000 style and Macintosh Platinum style, the two styles look
  slightly different.

  There are setActive(), setInactive() and setDisabled() functions to
  modify the palette.  Qt also supports a normal() group; this is an
  obsolete alias for active(), supported for backward compatibility.

  (The split between normal() and active() prior to Qt 2.1 did not
  work except in the simplest of cases, hence the change to the
  current, more powerful design.)

  \sa QApplication::setPalette(), QWidget::setPalette(), QColorGroup, QColor
*/ // ### should mention the constructors, copy stuff and isCopyOf()


static int palette_count = 1;

/*!
  Constructs a palette that consists of color groups with only black colors.
*/

QPalette::QPalette()
{
    data = new QPalData;
    CHECK_PTR( data );
    data->ser_no = palette_count++;
}

/*!\obsolete
  Constructs a palette from the \e button color. The other colors are
  automatically calculated, based on this color. Background will be
  the button color as well.
*/

QPalette::QPalette( const QColor &button )
{
    data = new QPalData;
    CHECK_PTR( data );
    data->ser_no = palette_count++;
    QColor bg = button, btn = button, fg, base, disfg;
    int h, s, v;
    bg.hsv( &h, &s, &v );
    if ( v > 128 ) {				// light background
	fg   = Qt::black;
	base = Qt::white;
	disfg = Qt::darkGray;
    } else {					// dark background
	fg   = Qt::white;
	base = Qt::black;
	disfg = Qt::darkGray;
    }
    data->normal   = QColorGroup( fg, btn, btn.light(150), btn.dark(),
				  btn.dark(150), fg, Qt::white, base, bg );
    data->active   = data->normal;
    data->disabled = QColorGroup( disfg, btn, btn.light(150), btn.dark(),
				  btn.dark(150), disfg, Qt::white, base, bg );
    data->inactive = data->active;
}

/*!
  Constructs a palette from a \e button color and a background. The other colors are
  automatically calculated, based on these colors.
*/

QPalette::QPalette( const QColor &button, const QColor &background )
{
    data = new QPalData;
    CHECK_PTR( data );
    data->ser_no = palette_count++;
    QColor bg = background, btn = button, fg, base, disfg;
    int h, s, v;
    bg.hsv( &h, &s, &v );
    if ( v > 128 ) {				// light background
	fg   = Qt::black;
	base = Qt::white;
	disfg = Qt::darkGray;
    } else {					// dark background
	fg   = Qt::white;
	base = Qt::black;
	disfg = Qt::darkGray;
    }
    data->normal   = QColorGroup( fg, btn, btn.light(150), btn.dark(),
				  btn.dark(150), fg, Qt::white, base, bg );
    data->active   = data->normal;
    data->disabled = QColorGroup( disfg, btn, btn.light(150), btn.dark(),
				  btn.dark(150), disfg, Qt::white, base, bg );
    data->inactive = data->active;
}

/*!
  Constructs a palette that consists of the three color groups \e
  active, \e disabled and \e inactive.  See QPalette for definitions
  of the color groups and QColorGroup::ColorRole for definitions of
  each color role in the three groups.

  \sa QColorGroup QColorGroup::ColorRole QPalette
*/

QPalette::QPalette( const QColorGroup &active, const QColorGroup &disabled,
		    const QColorGroup &inactive )
{
    data = new QPalData;
    CHECK_PTR( data );
    data->ser_no = palette_count++;
    data->active = active;
    data->normal = data->active;
    data->disabled = disabled;
    data->inactive = inactive;
}

/*!
  Constructs a copy of \a p.

  This constructor is fast (it uses copy-on-write).
*/

QPalette::QPalette( const QPalette &p )
{
    data = p.data;
    data->ref();
}

/*!
  Destructs the palette.
*/

QPalette::~QPalette()
{
    if ( data->deref() )
	delete data;
}

/*!
  Assigns \e p to this palette and returns a reference to this
  palette.

  This is fast (it uses copy-on-write).

  \sa copy()
*/

QPalette &QPalette::operator=( const QPalette &p )
{
    p.data->ref();
    if ( data->deref() )
	delete data;
    data = p.data;
    return *this;
}


/*!
  Returns the color in \a gr used for color role \a r.

  \sa brush() setColor() QColorGroup::ColorRole
*/
const QColor &QPalette::color( ColorGroup gr, QColorGroup::ColorRole r ) const
{
    return directBrush( gr, r ).color();
}

/*!
  Returns the brush in \a gr used for color role \a r.

  \sa color() setBrush() QColorGroup::ColorRole
*/
const QBrush &QPalette::brush( ColorGroup gr, QColorGroup::ColorRole r ) const
{
    return directBrush( gr, r );
}

/*!
  Sets the brush in \a gr used for color role \a r to the solid color \a c.

  \sa setBrush() color() QColorGroup::ColorRole
*/
void QPalette::setColor( ColorGroup gr, QColorGroup::ColorRole r,
			 const QColor &c)
{
    setBrush( gr, r, QBrush(c) );
}

/*!
  Sets the brush in \a gr used for color role \a r to \a b.

  \sa brush() setColor() QColorGroup::ColorRole
*/
void QPalette::setBrush( ColorGroup gr, QColorGroup::ColorRole r,
			 const QBrush &b)
{
    detach();
    data->ser_no = palette_count++;
    if ( gr == Normal )
	gr = Active; // #### remove 3.0
    directBrush( gr, r ) = b;
    if ( gr == Active )
	data->normal = data->active; // ##### remove 3.0
}

/*!
  Sets the color of the brush in \a gr used for color role \a r to \a c.

  \sa color() setBrush() QColorGroup::ColorRole
*/
void QPalette::setColor( QColorGroup::ColorRole r, const QColor &c )
{
    setBrush( r, QBrush(c) );
}

/*!
  Sets the brush in for color role \a r in all three color groups to \a b.

  \sa brush() setColor() QColorGroup::ColorRole active() inactive() disabled()
*/
void QPalette::setBrush( QColorGroup::ColorRole r, const QBrush &b )
{
    detach();
    data->ser_no = palette_count++;
    directBrush( Active,   r ) = b;
    directBrush( Disabled, r ) = b;
    directBrush( Inactive,   r ) = b;
    data->normal = data->active; // #### remove 3.0
}


/*! Return a deep copy of this palette.  This is slower than the copy
constructor and assignment operator and offers no advantages any more.
*/

QPalette QPalette::copy() const
{
    QPalette p( data->active, data->disabled, data->inactive );
    return p;
}


/*!
  Detaches this palette from any other QPalette objects with which it
  might implicitly share QColorGroup objects.  In essence, does the
  copy bit of copy-on-write.

  Calling this should generally not be necessary; QPalette calls it
  itself when necessary.
*/

void QPalette::detach()
{
    if ( data->count != 1 )
	*this = copy();
}


/*! \fn const QColorGroup & QPalette::normal() const

  \obsolete

  Use active() instead.
*/

/*!\obsolete

  Use setActive() instead.
*/

void QPalette::setNormal( const QColorGroup &g )
{
    setActive( g );
}

/*!
  \fn const QColorGroup & QPalette::disabled() const

  Returns the disabled color group of this palette.

  \sa QColorGroup, setDisabled(), active(), inactive()
*/

/*!
  Sets the \c Disabled color group to \e g.
  \sa disabled() setActive() setInactive()
*/

void QPalette::setDisabled( const QColorGroup &g )
{
    detach();
    data->ser_no = palette_count++;
    data->disabled = g;
}

/*!
  \fn const QColorGroup & QPalette::active() const
  Returns the active color group of this palette.
  \sa QColorGroup, setActive(), inactive(), disabled()
*/

/*!
  Sets the \c Active color group to \e g.
  \sa active() setDisabled() setInactive() QColorGroup
*/

void QPalette::setActive( const QColorGroup &g )
{
    detach();
    data->ser_no = palette_count++;
    data->active = g;
    data->normal = data->active; //#### alias
}

/*!
  \fn const QColorGroup & QPalette::inactive() const
  Returns the inactive color group of this palette.
  \sa QColorGroup,  setInactive(), active(), disabled()
*/

/*!
  Sets the \c Inactive color group to \e g.
  \sa active() setDisabled() setActive() QColorGroup
*/

void QPalette::setInactive( const QColorGroup &g )
{
    detach();
    data->ser_no = palette_count++;
    data->inactive = g;
}


/*!
  \fn bool QPalette::operator!=( const QPalette &p ) const

  Returns TRUE (slowly) if this palette is different from \e p, or
  FALSE (usually quickly) if they are equal.
*/

/*!  Returns TRUE (usually quickly) if this palette is equal to \e p,
or FALSE (slowly) if they are different.
*/

bool QPalette::operator==( const QPalette &p ) const
{
    return data->active == p.data->active &&
	   data->disabled == p.data->disabled &&
	   data->inactive == p.data->inactive;
}


/*!
  \fn int QPalette::serialNumber() const

  Returns a number that uniquely identifies this QPalette object.  The
  serial number is intended for caching.  Its value may not be used
  for anything other than equality testing.

  Note that QPalette uses copy-on-write, and the serial number changes
  during the lazy copy operation (detach()), not during a shallow
  copy (copy constructor or assignment).

  \sa QPixmap QPixmapCache QCache
*/


/*****************************************************************************
  QColorGroup/QPalette stream functions
 *****************************************************************************/

#ifndef QT_NO_DATASTREAM
#ifndef __ATHEOS__
/*!
  \relates QColorGroup
  Writes a color group to the stream.

  \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

QDataStream &operator<<( QDataStream &s, const QColorGroup &g )
{
    if ( s.version() == 1 ) {
	s << g.foreground()
	  << g.background()
	  << g.light()
	  << g.dark()
	  << g.mid()
	  << g.text()
	  << g.base();
    } else {
	for( int r = 0 ; r < QColorGroup::NColorRoles ; r++ )
	    s << g.brush( (QColorGroup::ColorRole)r);
    }
    return s;
}

/*!
  \related QColorGroup
  Reads a color group from the stream.

  \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

QDataStream &operator>>( QDataStream &s, QColorGroup &g )
{
    if ( s.version() == 1 ) {
	QColor fg, bg, light, dark, mid, text, base;
	s >> fg >> bg >> light >> dark >> mid >> text >> base;
	QPalette p( bg );
	QColorGroup n( p.normal() );
	n.setColor( QColorGroup::Foreground, fg );
	n.setColor( QColorGroup::Light, light );
	n.setColor( QColorGroup::Dark, dark );
	n.setColor( QColorGroup::Mid, mid );
	n.setColor( QColorGroup::Text, text );
	n.setColor( QColorGroup::Base, base );
	g = n;
    } else {
	QBrush tmp;
	for( int r = 0 ; r < QColorGroup::NColorRoles; r++ ) {
	    s >> tmp;
	    g.setBrush( (QColorGroup::ColorRole)r, tmp);
	}
    }
    return s;
}


/*!
  \relates QPalette
  Writes a palette to the stream and returns a reference to the stream.

  \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

QDataStream &operator<<( QDataStream &s, const QPalette &p )
{
    return s << p.active()
	     << p.disabled()
	     << p.inactive();
}


void readV1ColorGroup( QDataStream &s, QColorGroup &g,
		       QPalette::ColorGroup r )
{
    QColor fg, bg, light, dark, mid, text, base;
    s >> fg >> bg >> light >> dark >> mid >> text >> base;
    QPalette p( bg );
    QColorGroup n;
    switch ( r ) {
	case QPalette::Disabled:
	    n = p.disabled();
	    break;
	case QPalette::Inactive:
	    n = p.inactive();
	    break;
	default:
	    n = p.active();
	    break;
    }
    n.setColor( QColorGroup::Foreground, fg );
    n.setColor( QColorGroup::Light, light );
    n.setColor( QColorGroup::Dark, dark );
    n.setColor( QColorGroup::Mid, mid );
    n.setColor( QColorGroup::Text, text );
    n.setColor( QColorGroup::Base, base );
    g = n;
}


/*!
  \relates QPalette
  Reads a palette from the stream and returns a reference to the stream.

  \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

QDataStream &operator>>( QDataStream &s, QPalette &p )
{
    QColorGroup active, disabled, inactive;
    if ( s.version() == 1 ) {
	readV1ColorGroup( s, active, QPalette::Active );
	readV1ColorGroup( s, disabled, QPalette::Disabled );
	readV1ColorGroup( s, inactive, QPalette::Inactive );
    } else {
	s >> active >> disabled >> inactive;
    }
    QPalette newpal( active, disabled, inactive );
    p = newpal;
    return s;
}
#endif // __ATHEOS__
#endif //QT_NO_DATASTREAM

/*!  Returns TRUE if this palette and \a p are copies of each other,
  ie. one of them was created as a copy of the other and neither was
  subsequently modified.  This is much stricter than equality.

  \sa operator=, operator==
*/

bool QPalette::isCopyOf( const QPalette & p )
{
    return data && data == p.data;
}

QBrush &QPalette::directBrush( ColorGroup gr, QColorGroup::ColorRole r ) const
{
    if ( (uint)gr > (uint)QPalette::NColorGroups ) {
#if defined(CHECK_RANGE)
	qWarning( "QPalette::directBrush: colorGroup(%i) out of range", gr );
#endif
	return data->normal.br[QColorGroup::Foreground];
    }
    if ( (uint)r >= (uint)QColorGroup::NColorRoles ) {
#if defined(CHECK_RANGE)
	qWarning( "QPalette::directBrush: colorRole(%i) out of range", r );
#endif
	return data->normal.br[QColorGroup::Foreground];
    }
    switch( gr ) {
    case Normal:
    case Active:
	return data->active.br[r];
	//break;
    case Disabled:
	return data->disabled.br[r];
	//break;
    case Inactive:
	return data->inactive.br[r];
	//break;
    default:
	break;
    }
#if defined(CHECK_RANGE)
    qWarning( "QPalette::directBrush: colorGroup(%i) internal error", gr );
#endif
    return data->normal.br[QColorGroup::Foreground]; // Satisfy compiler
}


/*! \base64 palette.png

iVBORw0KGgoAAAANSUhEUgAAAlsAAACmCAMAAADaiLqtAAADAFBMVEUAAAAAAD8AAH8AAMAA
AP8AKgAAKj8AKn8AKsAAKv8AVQAAVT8AVX8AVcAAVf8AfwAAfz8Af38Af8AAf/8AqgAAqj8A
qn8AqsAAqv8A1QAA1T8A1X8A1cAA1f8A/wAA/z8A/38A/8AA//8qAAAqAD8qAH8qAMAqAP8q
KgAqKj8qKn8qKsAqKv8qVQAqVT8qVX8qVcAqVf8qfwAqfz8qf38qf8Aqf/8qqgAqqj8qqn8q
qsAqqv8q1QAq1T8q1X8q1cAq1f8q/wAq/z8q/38q/8Aq//9VAABVAD9VAH9VAMBVAP9VKgBV
Kj9VKn9VKsBVKv9VVQBVVT9VVX9VVcBVVf9VfwBVfz9Vf39Vf8BVf/9VqgBVqj9Vqn9VqsBV
qv9V1QBV1T9V1X9V1cBV1f9V/wBV/z9V/39V/8BV//9/AAB/AD9/AH9/AMB/AP9/KgB/Kj9/
Kn9/KsB/Kv9/VQB/VT9/VX9/VcB/Vf9/fwB/fz9/f39/f8B/f/9/qgB/qj9/qn9/qsB/qv9/
1QB/1T9/1X9/1cB/1f9//wB//z9//39//8B///+qAACqAD+qAH+qAMCqAP+qKgCqKj+qKn+q
KsCqKv+qVQCqVT+qVX+qVcCqVf+qfwCqfz+qf3+qf8Cqf/+qqgCqqj+qqn+qqsCqqv+q1QCq
1T+q1X+q1cCq1f+q/wCq/z+q/3+q/8Cq///VAADVAD/VAH/VAMDVAP/VKgDVKj/VKn/VKsDV
Kv/VVQDVVT/VVX/VVcDVVf/VfwDVfz/Vf3/Vf8DVf//VqgDVqj/Vqn/VqsDVqv/V1QDV1T/V
1X/V1cDV1f/V/wDV/z/V/3/V/8DV////AAD/AD//AH//AMD/AP//KgD/Kj//Kn//KsD/Kv//
VQD/VT//VX//VcD/Vf//fwD/fz//f3//f8D/f///qgD/qj//qn//qsD/qv//1QD/1T//1X//
1cD/1f///wD//z///3///8D/////AAD/AD//AH//AMAAAAAqKipVVVV/f3+qqqrV1dX////b
393zAAAACXBIWXMAAAxNAAAMTQHSzq1OAAAAHXRFWHRTb2Z0d2FyZQBHTlUgR2hvc3RzY3Jp
cHQgNS4xMIGXTpAAAA4zSURBVHic7Z3tduMoDIb3zP3fXO9k/upMGraNQUggvgyyE1vv7NYN
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
7EZC5QCtyPjYZbUIJCVwNG5ePL1qjctLF91HsVfn3m4gHXiNx7xgCWRdWpc8dglbcX5mzha2
2PvYKj4cyTMS9rGpsTWi7m9D5dMa1diSDNM+Y0XUubcbAYqfE9nC59IlbOFTGLNPb6XGFia6
025JNGVshbs6mK28Fo9mS+whSGEj6t/bjbKVV2Jqt0SjlRmsoTYRgk3Z2ybmbMVAStbx7xNn
a7FLp67FyNhijzc80yalxhb2fgb8rfgN2d+C5362xFaQHaglvsW7anV19BOf0eHN96+S2KL9
RObOZ/1ErDeI56gvm+VxlC3sJ4oOIyWNPSvG1gr1v/PJKoCJjMuPGPhCTB68ZHwLwqaH9MGJ
nRd+Z8jWrjJ9U70jW8Qn6WBrzHPQZ6tL6X0hW/9+tKdY31HvyBYp/BZb4z6pGD8J1GYrH7gn
baIztvZq1VqMRxhf0MhjMn91MVtir9df0tia0Kq1GJqdaEi0jq3c09rk2XIPY2tCK+bLv0bL
9y6E2KNVbOVDwF4bW+5hbM1oP1uBKl+xh0rM0DBbwusFrxdbv/t5GVsTGmcrtEyj31PX6N5u
220UzgW0LsxWHJEEHtR8hYIv3DNXqDIuWdPbMhWUumVT2tC6tC9PR7tZUDuMvNuTrunVZCuW
9rXUuqUXWpduE5ndoqtB4luUGAZk9gGZZuscnZOQ2LESW1dFClW/s81qXdvfomz5mRj1dT4+
Ks7a4HAmTanL99+6OlJ9Cmhdmy10mPrmy6dTH8N8FxcZFNgCQ4qJoHVltvBnL1sII59LBSCw
ZUSJimjdxJcfsFsMK4ffdRlbbu2+gRcRQevabSL+BOJXRbaEsJq/la/4Nra43L8sQLfGj9P8
Oh/sJ1K2CJnJ8jZji+tGbKWCzrBuff0zUeUo3YItbOcaYUPaPXB9UTkBt4vo8HU+ptvo8HfV
ptvI2DJpydgyacnYMmnJ2DJpydgyacnYMmnJ2DJpydgyacnYMmnJ2DJpydgyacnYMmnJ2DJp
ydgyacnYMmnJ2DJpydgyacnYMmnJ2DJpydgyacnYMmnJ2DJpydgyacnYMmnJ2DJpydgyacnY
MmnJ2DJpydgyacnYMmnJ2DJpydgyacnYMmnJ2DJp6X+ZTGjxOxEPlAAAAABJRU5ErkJggg==

*/
#endif // QT_NO_PALETTE

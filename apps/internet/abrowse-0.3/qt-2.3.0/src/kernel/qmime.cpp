/****************************************************************************
** $Id$
**
** Implementation of MIME support
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

#include "qmime.h"

#ifndef QT_NO_MIME

#include "qmap.h"
#include "qstringlist.h"
#include "qfileinfo.h"
#include "qdir.h"
#include "qdragobject.h"
#include "qapplication.h" // ### for now
#include "qclipboard.h" // ### for now

// NOT REVISED
/*!
  \class QMimeSource qmime.h
  \brief An abstract piece of formatted data.
  \ingroup misc

  \link dnd.html Drag-and-drop\endlink and
  \link QClipboard clipboard\endlink use this abstraction.

  \sa <a href="http://www.isi.edu/in-notes/iana/assignments/media-types/">
	  IANA list of MIME media types</a>
*/


/*!
  Provided to ensure subclasses destruct correctly.
*/
QMimeSource::~QMimeSource()
{
    if (QApplication::closingDown()) return;
    
#ifndef QT_NO_MIMECLIPBOARD
    if (QApplication::clipboard()->data() == this) {
#ifdef CHECK_RANGE
	qWarning("QMimeSource::~QMimeSource: clipboard data deleted!");
#endif
#if defined(_WS_X11_)	
	QApplication::clipboard()->clobber();
#endif
    }
#endif // QT_NO_CLIPBOARD
}

/*!
  \fn QByteArray QMimeSource::encodedData(const char*) const

  Returns the encoded payload of this object, in the specified
  MIME format.

  Subclasses must reimplement this function.
*/



/*!
  Returns TRUE if the object can provide the data
  in format \a mimeType.  The default implementation
  iterates over format().

  Note that it is often better to use the more-abstract
  canDecode() functions such as QTextDrag::canDecode()
  and QImageDrag::canDecode().
*/
bool QMimeSource::provides(const char* mimeType) const
{
    const char* fmt;
    for (int i=0; (fmt = format(i)); i++) {
	if ( !qstricmp(mimeType,fmt) )
	    return TRUE;
    }
    return FALSE;
}


/*!
  \fn const char * QMimeSource::format(int i) const

  Returns the \e ith format, or NULL.
*/



class QMimeSourceFactoryData {
public:
    QMimeSourceFactoryData() :
	last(0)
    {
    }

    ~QMimeSourceFactoryData()
    {
	QMap<QString, QMimeSource*>::Iterator it = stored.begin();
	while ( it != stored.end() ) {
	    delete *it;
	    ++it;
	}
	delete last;
    }

    QMap<QString, QMimeSource*> stored;
    QMap<QString, QString> extensions;
    QStringList path;
    QMimeSource* last;
};


/*!
  \class QMimeSourceFactory qmime.h
  \brief An extensible supply of MIME-typed data.
  \ingroup environment

  A QMimeSourceFactory provides an abstract interface to a collection of
  information.  Each piece of information is represented by a QMimeSource
  object, which can be examined and converted to concrete data types by
  functions like QImageDrag::canDecode() and QImageDrag::decode().

  The base QMimeSourceFactory can be used in two ways: as an abstraction of
  a collection of files, or as specifically stored data.  For it to access
  files, call setFilePath() before accessing data.  For stored data, call
  setData() for each item (there are also convenience functions setText(),
  setImage(), and setPixmap() that simply call setData() with massaged
  parameters).

  The rich text widgets QTextView and QTextBrowser use
  QMimeSourceFactory to resolve references such as images or links
  within rich text documents. They either access the default factory (
  see defaultFactory() ) or their own ( see
  QTextView::setMimeSourceFactory() ). Other classes that are capable
  of displaying rich text like QLabel, QWhatsThis or QMessageBox
  always use the default factory.

  As mentioned earlier, a factory can also be used as container to
  store data associated with a name. This technique is useful whenever
  rich text contains images that are stored in the program itself, not
  loaded from the hard disk. Your program may for example define some
  image data as

  \code
  static const char* myimage_xpm[]={
  "...",
  ...
  "..."};
  \endcode

  To be able to use this image within some rich text, for example inside a
  QLabel, you have to create a QImage from the raw data and insert it
  into the factory with a unique name:

  \code
  QMimeSourceFactory::defaultFactory()->setImage( "myimage", QImage(myimage_data) );
  \endcode

  Now you can create a rich text QLabel with

  \code
  QLabel* label = new QLabel(
      "Rich text with embedded image:<img source=\"myimage\">"
      "Isn't that <em>cute</em>?" );
  \endcode
*/


/*!
  Constructs a QMimeSourceFactory which has no file path and no stored
  content.
*/
QMimeSourceFactory::QMimeSourceFactory() :
    d(new QMimeSourceFactoryData)
{
    // add some reasonable defaults
    setExtensionType("htm", "text/html;charset=iso8859-1");
    setExtensionType("html", "text/html;charset=iso8859-1");
    setExtensionType("txt", "text/plain");
    setExtensionType("xml", "text/xml;charset=UTF-8");
    setExtensionType("jpg", "image/jpeg"); // support misspelled jpeg files
}

/*!
  Destructs the QMimeSourceFactory, deleting all stored content.
*/
QMimeSourceFactory::~QMimeSourceFactory()
{
    delete d;
}

#ifndef __ATHEOS__
static QMimeSource* data_internal(const QString& abs_name,
				  const QMap<QString, QString> extensions )
{
    QMimeSource* r = 0;
    QFileInfo fi(abs_name);
    if ( fi.isReadable() ) {

	// get the right mimetype
	QString e = fi.extension(FALSE);
	QCString mimetype = "application/octet-stream";
	const char* imgfmt;
	if ( extensions.contains(e) )
	    mimetype = extensions[e].latin1();
	else if ( ( imgfmt = QImage::imageFormat( abs_name ) ) )
	    mimetype = QCString("image/")+QCString(imgfmt).lower();

	QFile f(abs_name);
	if ( f.open(IO_ReadOnly) ) {
	    QByteArray ba(f.size());
	    f.readBlock(ba.data(), ba.size());
	    QStoredDrag* sr = new QStoredDrag( mimetype );
	    sr->setEncodedData( ba );
	    r = sr;
	}
    }
    return r;
}
#endif // __ATHEOS__


/*!
  Returns a reference to the data associated with \a abs_name.  The return
  value only remains valid until a subsequent call to this function for
  the same object, and only if setData() is not called to modify the data,
  so you should immediately decode the result.

  If there is no data associated with \a abs_name in the factory's
  store, the factory tries to access the local filesystem. If \a
  abs_name isn't an absolute filename, the factory will search for it
  on all defined paths ( see setFilePath() ).

  The factory understands all image formats supported by
  QImageIO. Any other mime types are determined by the filename
  extension. The default settings are
  \code
  setExtensionType("html", "text/html;charset=iso8859-1");
  setExtensionType("htm", "text/html;charset=iso8859-1");
  setExtensionType("txt", "text/plain");
  setExtensionType("xml", "text/xml;charset=UTF-8");
  \endcode
  The effect of these is that filenames ending in "html" or "htm" will
  be treated as text encoded in the iso8859-1 encoding, those ending in "txt"
  will be treated as text encoded in the local encoding; those ending in "xml"
  will be treated as text encoded in UTF8 encoding.  The text subtype ("html",
  "plain", or "xml") does not affect the factory, but users of the factory
  may behave differently. We recommend creating "xml" files where practical
  as such files can be viewed regardless of the run-time encoding, and can
  encode any Unicode characters without resorting to encoding definitions
  inside the file.

  Any file data that is not recognized will be retrieved as a QMimeSource
  providing the "application/octet-stream" MIME type, which is just
  uninterpreted binary data.
  You can add further extensions or change existing ones with
  subsequent calls to setExtensionType(). If the extension mechanism
  is not sufficient for you problem domain, you may inherit
  QMimeSourceFactory and reimplement this function to perform some
  more clever mime type detection. The same applies if you want to use
  the mime source factory for accessing URL referenced data over a
  network.
*/
#ifndef __ATHEOS__
const QMimeSource* QMimeSourceFactory::data(const QString& abs_name) const
{
    if ( d->stored.contains(abs_name) )
	return d->stored[abs_name];

    QMimeSource* r = 0;
    QStringList::Iterator it;
    if ( abs_name[0] == '/'
#ifdef _WS_WIN_
	    || abs_name[0] && abs_name[1] == ':'
#endif
    )
    {
	// handle absolute file names directly
	r = data_internal( abs_name, d->extensions );
    }
    else { // check list of paths
	for ( it = d->path.begin(); !r && it != d->path.end(); ++it ) {
	    QString filename = *it;
	    if ( filename[(int)filename.length()-1] != '/' )
		filename += '/';
	    filename += abs_name;
	    r = data_internal( filename, d->extensions );
	}
    }
    delete d->last;
    d->last = r;
    return r;
}
#endif // __ATHEOS__

/*!
  Sets a list of directories which will be searched when named data
  is requested.

  \sa filePath()
*/
void QMimeSourceFactory::setFilePath( const QStringList& path )
{
    d->path = path;
}

/*!
  Returns the currently set search paths.
*/
QStringList QMimeSourceFactory::filePath() const
{
  return d->path;
}

/*!
  Adds another search path.

  \sa setFilePath()
*/
void QMimeSourceFactory::addFilePath( const QString& p )
{
  d->path += p;
}

/*!
  Sets the MIME-type to be associated with a filename extension.  This
  determines the MIME-type for files found via a path set by setFilePath().
*/
void QMimeSourceFactory::setExtensionType( const QString& ext, const char* mimetype )
{
    d->extensions.replace(ext, mimetype);
}

/*!
  Converts the absolute or relative data item name \a abs_or_rel_name
  to an absolute name, interpreted within the context of the data
  item named \a context (this must be an absolute name).
*/
QString QMimeSourceFactory::makeAbsolute(const QString& abs_or_rel_name, const QString& context) const
{
    if ( context.isNull() ||
	 !(context[0] == '/'
#ifdef _WS_WIN_
	 || ( context[0] && context[1] == ':')
#endif
	   ))
	return abs_or_rel_name;
    if ( abs_or_rel_name.isEmpty() )
	return context;
    QFileInfo c( context );
    QFileInfo r( c.dir(TRUE), abs_or_rel_name );
    return r.absFilePath();
}

/*!
  A convenience function. See data(const QString& abs_name).
*/
const QMimeSource* QMimeSourceFactory::data(const QString& abs_or_rel_name, const QString& context) const
{
    const QMimeSource* r = data(makeAbsolute(abs_or_rel_name,context));
    if ( !r && !d->path.isEmpty() )
	r = data(abs_or_rel_name);
    return r;
}


/*!
  Sets \a text to be the data item associated with
  the absolute name \a abs_name.

  Equivalent to setData(abs_name, new QTextDrag(text)).
*/
void QMimeSourceFactory::setText( const QString& abs_name, const QString& text )
{
    setData(abs_name, new QTextDrag(text));
}

/*!
  Sets \a image to be the data item associated with
  the absolute name \a abs_name.

  Equivalent to setData(abs_name, new QImageDrag(image)).
*/
void QMimeSourceFactory::setImage( const QString& abs_name, const QImage& image )
{
    setData(abs_name, new QImageDrag(image));
}

/*!
  Sets \a pixmap to be the data item associated with
  the absolute name \a abs_name.
*/
void QMimeSourceFactory::setPixmap( const QString& abs_name, const QPixmap& pixmap )
{
    setData(abs_name, new QImageDrag(pixmap.convertToImage()));
}

/*!
  Sets \a data to be the data item associated with
  the absolute name \a abs_name. Note that the ownership of \a data is
  transferred to the factory - do not delete or access the pointer after
  passing it to this function.
*/
void QMimeSourceFactory::setData( const QString& abs_name, QMimeSource* data )
{
    if ( d->stored.contains(abs_name) )
	delete d->stored[abs_name];
    d->stored.replace(abs_name,data);
}



static QMimeSourceFactory* defaultfactory = 0;
void qt_cleanup_defaultfactory()
{
    delete defaultfactory;
}

/*!
  Returns the application-wide default mime source factory. This
  factory is used by rich text rendering classes such as
  QSimpleRichText, QWhatsThis and also QMessageBox to resolve named
  references within rich text documents. It serves also as initial
  factory for the more complex render widgets QTextView and
  QTextBrowser.

  \sa setDefaultFactory()
 */
QMimeSourceFactory* QMimeSourceFactory::defaultFactory()
{
    if (!defaultfactory) {
	defaultfactory = new QMimeSourceFactory();
	qAddPostRoutine( qt_cleanup_defaultfactory );
    }
    return defaultfactory;
}

/*!
  Sets the default \a factory, destroying any previously set mime source
  provider. The ownership of the factory is transferred.

  \sa defaultFactory()
 */
void QMimeSourceFactory::setDefaultFactory( QMimeSourceFactory* factory)
{
    if ( defaultfactory != factory )
	delete defaultfactory;
    defaultfactory = factory;
}

#endif // QT_NO_MIME

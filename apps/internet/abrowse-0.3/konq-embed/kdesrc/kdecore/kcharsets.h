/* This file is part of the KDE libraries
    Copyright (C) 1999 Lars Knoll (knoll@kde.org)
    $Id$

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/
#ifndef KCHARSETS_H
#define KCHARSETS_H

#include <qstring.h>
#include <qfont.h>
#include <qstringlist.h>
#include <qlist.h>

class KGlobal;
class KCharsetsPrivate;

class QTextCodec;

/**
 * Charset font and encoder/decoder handling.
 *
 * This is needed, because Qt's font matching algorithm gives the font
 * family a higher priority than the charset.  For many applications
 * this is not acceptable, since it can totally obscure the output,
 * in languages which use non iso-8859-1 charsets.
 *
 * @author Lars Knoll <knoll@kde.org>
 * @version $Id$
 */
class KCharsets
{
    friend class KGlobal;

protected:
    /** protected constructor. If you need the kcharsets object, use
        @ref KGlobal::charsets() instead.
    */
    KCharsets();

public:

    /** destructor */
    virtual ~KCharsets();

    /**
     * provided for compitibility.
     */
    QTextCodec *codecForName(const QString &name) const;
    
    /**
     * tries to find a QTextCodec to convert the given encoding from and to
     * Unicode. If no codec could be found the latin1 codec will be returned an
     * ok will be set to false.
     */
    QTextCodec *codecForName(const QString &n, bool &ok) const;
    /**
     * @returns the charset that fits a given encoding best (that can display a
     * file in the given encoding)
     */
    QFont::CharSet charsetForEncoding(const QString &encoding) const;

    /**
     * converts an entity to a character. The string must contain only the
     * entity without the trailing ';'.
     *  @returns QChar::null if the entity could not be decoded.
     */
    QChar fromEntity(const QString &str) const;
    /**
     * Overloaded member function. Tries to find an entity in the
     * @ref QString str.
     * @returns a decoded entity if one could be found, @ref QChar::null
     * otherwise
     * @param len is a return value, that gives the length of the decoded
     * entity.
     */
    QChar fromEntity(const QString &str, int &len) const;

    /**
     * converts a QChar to an entity. The returned string does already
     * contain the leading '&' and the trailing ';'.
     */
    QString toEntity(const QChar &ch) const;

    /**
     * lists all available charsets for a given family.
     * if family is omitted, it will return all charsets available.
     *
     * It is the resonsability of the caller to delete the items in the list.
     */
    QList<QFont::CharSet> availableCharsets(QString family = QString::null);

    /**
     * as above, but returns the names of the charsets
     */
    QStringList availableCharsetNames(QString family = QString::null);

        /**
         * Lists all available encodings as names.
         */
        QStringList availableEncodingNames();

    /**
     * @returns a QFont, which can print the character given, and is closest
     * to the font given. if no mathing font could be found, the returned font
     * will have the charset @ref QFont::Any.
     */
    QFont fontForChar( const QChar &ch, const QFont &f ) const;

    //FIXME: setQfont without ecnod arg should return charsetforlocale
    // or unicode...

    /**
     * @returns all families that have a font in the give charset
     */
    QStringList availableFamilies( QFont::CharSet ch );
    /**
     * sets the @ref QFont f to the charset given in charset.
     * Opposed to @ref QFont's setCharset() function, this function will do
     * it's best to find a font which can display the given charset. It might
     * change the font's family for this purpose, but care is taken to find
     * a family which is as close as possible to the font given.
     */
    void setQFont(QFont &f, QFont::CharSet charset = QFont::Unicode) const;

    /**
     * overloaded member function. Provided for convenience.
     */
    void setQFont(QFont &f, QString charset) const;


    /**
     * @returns the name of the charset f is set to.
     */
    QString name(const QFont &f);

    /**
     * @returns the name of the Charset c.
     */
    QString name(QFont::CharSet c);

    /**
     * is a font with the given charset available?
     */
    bool isAvailable(QFont::CharSet charset);
    /**
     * overloaded member function. Provided for convenience.
     */
    bool isAvailable(const QString &charset);

    /**
     * @returns the charset for the locale.
     */
    QFont::CharSet charsetForLocale();

    /**
     * does the given font family have a unicode encoding?
     */
    bool hasUnicode(QString family) const;
    /**
     * does given font exist with a unicode encoding?
     */
    bool hasUnicode(QFont &font) const;

    enum FixedType { FixedUnknown, Fixed, Proportional };
    enum WeightType { WeightUnknown, Medium, Bold };
    enum SlantType { SlantUnknown, Normal, Italic };

public:
    QString xCharsetName(QFont::CharSet) const;
    QFont::CharSet nameToID(QString name) const;
    QFont::CharSet xNameToID(QString name) const;

private:
    KCharsetsPrivate *d;
};

#endif

/* This file is part of the KDE project
   Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
             (C) 1999 David Faure <faure@kde.org>

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
#ifndef __kparts_factory_h__
#define __kparts_factory_h__

#include <klibloader.h>

class QWidget;

namespace KParts
{

class Part;

/**
 * A generic factory object to create a Part.
 *
 * Factory is an abstract class. Reimplement the @ref
 * createPartObject() method to give it functionality.
 *
 * @see KLibFactory.
 */
class Factory : public KLibFactory
{
  Q_OBJECT
public:
  Factory( QObject *parent = 0, const char *name = 0 );
  virtual ~Factory();

    /**
     * Creates a part.
     *
     * The QStringList can be used to pass additional arguments to the part.
     * If the part needs additional arguments, it should take them as
     * name="value" pairs. This is the way additional arguments will get passed
     * to the part from eg. khtml. You can for example emebed the part into HTML
     * by using the following code:
     * <pre>
     *    <object type="my_mimetype" data="url_to_my_data">
     *        <param name="name1" value="value1">
     *        <param name="name2" value="value2">
     *    </object>
     * </pre>
     * This could result in a call to
     * <pre>
     *     createPart( parentWidget, name, parentObject, parentName, "Kparts::Part",
     *                 QStringList("name1="value1"", "name2="value2") );
     * </pre>
     *
     * @returns the newly created part.
     *
     * Never reimplement this function. Instead, reimplement @ref
     * createPartObject().
     *
     * createPart() automatically emits a signal @ref objectCreated to tell
     * the library about its newly created object.  This is very
     * important for reference counting, and allows unloading the
     * library automatically once all its objects have been destroyed.
     *
     * This function is virtual for compatibility reasons only.
     *
     */
    virtual Part *createPart( QWidget *parentWidget = 0, const char *widgetName = 0, QObject *parent = 0, const char *name = 0, const char *classname = "KParts::Part", const QStringList &args = QStringList() );

protected:

    /**
     * Reimplement this method in your implementation to create the Part.
     *
     * The QStringList can be used to pass additional arguments to the part.
     * If the part needs additional arguments, it should take them as
     * name="value" pairs. This is the way additional arguments will get passed
     * to the part from eg. khtml. You can for example emebed the part into HTML
     * by using the following code:
     * <pre>
     *    <object type="my_mimetype" data="url_to_my_data">
     *        <param name="name1" value="value1">
     *        <param name="name2" value="value2">
     *    </object>
     * </pre>
     * This could result in a call to
     * <pre>
     *     createPart( parentWidget, name, parentObject, parentName, "Kparts::Part",
     *                 QStringList("name1="value1"", "name2="value2") );
     * </pre>
     *
     * @returns the newly created part.
     */
    virtual Part *createPartObject( QWidget *parentWidget = 0, const char *widgetName = 0, QObject *parent = 0, const char *name = 0, const char *classname = "KParts::Part", const QStringList &args = QStringList() );
    
    /**
     * Reimplemented from KLibFactory. Calls @ref createPart()
     */
    virtual QObject *createObject( QObject *parent = 0, const char *name = 0, const char *classname = "QObject", const QStringList &args = QStringList() );
};

};

#endif

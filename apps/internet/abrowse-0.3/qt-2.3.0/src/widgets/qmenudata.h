/****************************************************************************
** $Id$
**
** Definition of QMenuData class
**
** Created : 941128
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the widgets module of the Qt GUI Toolkit.
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

#ifndef QMENUDATA_H
#define QMENUDATA_H

#ifndef QT_H
#include "qglobal.h"

// if we did not include QIconSet, we would break code due to the missing
// conversion QPixmap->QIconset
#include "qiconset.h"

#endif // QT_H

#ifndef QT_NO_MENUDATA

class QPopupMenu;
class QMenuDataData;
class QObject;

#if defined(INCLUDE_MENUITEM_DEF)

#include "qstring.h"
#include "qsignal.h"
#include "qfont.h"

class QCustomMenuItem;
class QMenuItemData;

class Q_EXPORT QMenuItem			// internal menu item class
{
friend class QMenuData;
public:
    QMenuItem();
   ~QMenuItem();

    int		id()		const	{ return ident; }
    QIconSet   *iconSet()	const	{ return iconset_data; }
    QString	text()		const	{ return text_data; }
    QString	whatsThis()	const	{ return whatsthis_data; }
    QPixmap    *pixmap()	const	{ return pixmap_data; }
    QPopupMenu *popup()		const	{ return popup_menu; }
    QWidget *widget()		const	{ return widget_item; }
    QCustomMenuItem *custom()	const;
    int		key()		const	{ return accel_key; }
    QSignal    *signal()	const	{ return signal_data; }
    bool	isSeparator()	const	{ return is_separator; }
    bool	isEnabled()	const	{ return is_enabled; }
    bool	isChecked()	const	{ return is_checked; }
    bool	isDirty()	const	{ return is_dirty; }

    void	setText( const QString &text ) { text_data = text; }
    void	setDirty( bool dirty )	       { is_dirty = dirty; }
    void	setWhatsThis( const QString &text ) { whatsthis_data = text; }

private:
    int		ident;				// item identifier
    QIconSet   *iconset_data;			// icons
    QString	text_data;			// item text
    QString	whatsthis_data;			// item Whats This help text
    QPixmap    *pixmap_data;			// item pixmap
    QPopupMenu *popup_menu;			// item popup menu
    QWidget    *widget_item;				// widget menu item
    int		accel_key;			// accelerator key (state|ascii)
    QSignal    *signal_data;			// connection
    uint	is_separator : 1;		// separator flag
    uint	is_enabled   : 1;		// disabled flag
    uint	is_checked   : 1;		// checked flag
    uint	is_dirty     : 1;		// dirty (update) flag
    QMenuItemData* d;

    QMenuItemData* extra();

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QMenuItem( const QMenuItem & );
    QMenuItem &operator=( const QMenuItem & );
#endif
};

#include "qlist.h"
typedef QList<QMenuItem>	 QMenuItemList;
typedef QListIterator<QMenuItem> QMenuItemListIt;

#else

class QMenuItem;
class QMenuItemList;
class QPixmap;

#endif // INCLUDE_MENUITEM_TEXT



class Q_EXPORT QCustomMenuItem : public Qt
{
public:
    QCustomMenuItem();
    virtual ~QCustomMenuItem();
    virtual bool fullSpan() const;
    virtual bool isSeparator() const;
    virtual void setFont( const QFont& font );
    virtual void paint( QPainter* p, const QColorGroup& cg, bool act,
			bool enabled, int x, int y, int w, int h ) = 0;
    virtual QSize sizeHint() = 0;
};


class Q_EXPORT QMenuData			// menu data class
{
friend class QMenuBar;
friend class QPopupMenu;
public:
    QMenuData();
    virtual ~QMenuData();

    uint	count() const;


    int		insertItem( const QString &text,
			    const QObject *receiver, const char* member,
			    int accel = 0, int id = -1, int index = -1 );
    int		insertItem( const QIconSet& icon,
			    const QString &text,
			    const QObject *receiver, const char* member,
			    int accel = 0, int id = -1, int index = -1 );
    int		insertItem( const QPixmap &pixmap,
			    const QObject *receiver, const char* member,
			    int accel = 0, int id = -1, int index = -1 );
    int		insertItem( const QIconSet& icon,
			    const QPixmap &pixmap,
			    const QObject *receiver, const char* member,
			    int accel = 0, int id = -1, int index = -1 );




    int		insertItem( const QString &text, int id=-1, int index=-1 );
    int		insertItem( const QIconSet& icon,
			    const QString &text, int id=-1, int index=-1 );

    int		insertItem( const QString &text, QPopupMenu *popup,
			    int id=-1, int index=-1 );
    int		insertItem( const QIconSet& icon,
			    const QString &text, QPopupMenu *popup,
			    int id=-1, int index=-1 );


    int		insertItem( const QPixmap &pixmap, int id=-1, int index=-1 );
    int		insertItem( const QIconSet& icon,
			    const QPixmap &pixmap, int id=-1, int index=-1 );
    int		insertItem( const QPixmap &pixmap, QPopupMenu *popup,
			    int id=-1, int index=-1 );
    int		insertItem( const QIconSet& icon,
			    const QPixmap &pixmap, QPopupMenu *popup,
			    int id=-1, int index=-1 );

    int		insertItem( QWidget* widget, int id=-1, int index=-1 );

    int		insertItem( const QIconSet& icon, QCustomMenuItem* custom, int id=-1, int index=-1 );
    int		insertItem( QCustomMenuItem* custom, int id=-1, int index=-1 );


    int		insertSeparator( int index=-1 );

    void	removeItem( int id )		{ removeItemAt(indexOf(id)); }
    void	removeItemAt( int index );
    void	clear();

    int		accel( int id )		const;
    void	setAccel( int key, int id );

    QIconSet    *iconSet( int id )	const;
    QString text( int id )		const;
    QPixmap    *pixmap( int id )	const;

    void setWhatsThis( int id, const QString& );
    QString whatsThis( int id ) const;


    void	changeItem( int id, const QString &text );
    void	changeItem( int id, const QPixmap &pixmap );
    void	changeItem( int id, const QIconSet &icon, const QString &text );
    void	changeItem( int id, const QIconSet &icon, const QPixmap &pixmap );

    void	changeItem( const QString &text, int id ); // obsolete
    void	changeItem( const QPixmap &pixmap, int id ); // obsolete
    void	changeItem( const QIconSet &icon, const QString &text, int id ); // obsolete


    bool	isItemEnabled( int id ) const;
    void	setItemEnabled( int id, bool enable );

    bool	isItemChecked( int id ) const;
    void	setItemChecked( int id, bool check );

    virtual void updateItem( int id );

    int		indexOf( int id )	const;
    int		idAt( int index )	const;
    virtual void	setId( int index, int id );

    bool	connectItem( int id,
			     const QObject *receiver, const char* member );
    bool	disconnectItem( int id,
				const QObject *receiver, const char* member );

    bool	setItemParameter( int id, int param );
    int	itemParameter( int id ) const;

    QMenuItem  *findItem( int id )	const;
    QMenuItem  *findItem( int id, QMenuData ** parent )	const;

    void activateItemAt( int index );

protected:
    int		   actItem;
    QMenuItemList *mitems;
    QMenuData	  *parentMenu;
    uint	   isPopupMenu	: 1;
    uint	   isMenuBar	: 1;
    uint	   badSize	: 1;
    uint	   mouseBtDn	: 1;
    uint	avoid_circularity : 1;
    uint 	actItemDown : 1;
    virtual void   menuContentsChanged();
    virtual void   menuStateChanged();
    virtual void   menuInsPopup( QPopupMenu * );
    virtual void   menuDelPopup( QPopupMenu * );

    QMenuItem * findPopup( QPopupMenu *, int *index = 0 );

private:
    int		insertAny( const QString *, const QPixmap *, QPopupMenu *,
			   const QIconSet*, int, int, QWidget* = 0, QCustomMenuItem* = 0);
    void	removePopup( QPopupMenu * );
    virtual void	setAllDirty( bool );
    void	changeItemIconSet( int id, const QIconSet &icon );

    QMenuDataData *d;

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QMenuData( const QMenuData & );
    QMenuData &operator=( const QMenuData & );
#endif
};


#endif // QT_NO_MENUDATA

#endif // QMENUDATA_H

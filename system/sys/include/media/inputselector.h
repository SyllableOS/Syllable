/*  Media Input Selector class
 *  Copyright (C) 2003 Arno Klenke
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */
 
#ifndef __F_MEDIA_INPUT_SELECTOR_H_
#define __F_MEDIA_INPUT_SELECTOR_H_

#include <gui/window.h>
#include <gui/view.h>
#include <gui/point.h>
#include <gui/dropdownmenu.h>
#include <gui/stringview.h>
#include <gui/textview.h>
#include <gui/button.h>
#include <gui/filerequester.h>
#include <atheos/types.h>
#include <util/string.h>
#include <util/message.h>
#include <util/messenger.h>
#include <media/manager.h>
#include <media/input.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent


/** Media Input selector.
 * \ingroup media
 * \par Description:
 * The Media Input Selector opens a new window where the user can select
 * the file / device he wants to open. The selector will make sure
 * that the file / device can be openend by the system.
 *
 * \author	Arno Klenke
 *****************************************************************************/

class MediaInputSelector : public Window
{
public:
	MediaInputSelector( const Point cPosition, const String zTitle, Messenger* pcTarget, Message* pcOpenMessage, 
						Message* pcCancelMessage, bool bAutoDetect = true );
	virtual ~MediaInputSelector();
	virtual bool OkToQuit();
	virtual void HandleMessage( Message* pcMessage );
	
	virtual void _reserved1();
    virtual void _reserved2();
    virtual void _reserved3();
    virtual void _reserved4();
    virtual void _reserved5();
    virtual void _reserved6();
    virtual void _reserved7();
    virtual void _reserved8();
    virtual void _reserved9();
    virtual void _reserved10();
private:
	class Private;
	Private* m;
};

}

#endif













/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 2005 Syllable Team
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

#ifndef __F_GUI_STATUSBAR_H__
#define __F_GUI_STATUSBAR_H__

#include <gui/stringview.h>
#include <gui/layoutview.h>
#include <util/string.h>

namespace os
{
#if 0 // Fool Emacs auto-indent
}
#endif

/** Base class for StatusBar panels.
 * \ingroup gui
 * \par Description:
 *	
 * \sa StatusBar
 * \author	Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
class StatusPanel : public StringView
{
public:
	enum {
		F_FIXED			= 0x0001
	};
	StatusPanel();
	StatusPanel( const String& cName, const String& cText, double vSize, uint32 nFlags );
	virtual ~StatusPanel();

	uint32 GetFlags() const;
	void SetFlags( uint32 nFlags );

	double GetSize() const;
	void SetSize( double vSize );

private:
    StatusPanel& operator=( const StatusPanel& );
    StatusPanel( const StatusPanel& );
    
    void _UpdateSize();

	class Private;
	Private *m;
};

/** Status bar
 * \ingroup gui
 * \par Description:
 * \par Example:
 * \code
 *	pcPanel->AddPanel( "statuspanel", "Status: OK" );
 *	pcPanel->SetText( "statuspanel", "Status: ERROR" );
 * \endcode
 * \sa StatusPanel
 * \author	Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
class StatusBar : public LayoutView
{
public:
    StatusBar( const Rect& cFrame, const String& cTitle,
                    uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP,
                    uint32 nFlags = WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );
    virtual ~StatusBar();

	LayoutNode* AddPanel( const String& cName, const String& cText, double vSize = 0.0, uint32 nFlags = 0 );
	LayoutNode* AddPanel( StatusPanel* pcPanel );

	void SetText( const String& cName, const String& cText );

private:
	virtual void __SB_reserved1__();
	virtual void __SB_reserved2__();
	virtual void __SB_reserved3__();
	virtual void __SB_reserved4__();
	virtual void __SB_reserved5__();

private:
    StatusBar& operator=( const StatusBar& );
    StatusBar( const StatusBar& );

	class Private;
	Private *m;
};

}

#endif		// __F_GUI_STATUSBAR_H__

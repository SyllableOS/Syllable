/*  Media Server output plugin
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

#ifndef _PREFS_H_
#define _PREFS_H_

#include <gui/layoutview.h>
#include <gui/dropdownmenu.h>
#include <gui/button.h>
#include <gui/stringview.h>

class PrefsView : public os::View
{
	public:
		PrefsView( os::Rect cFrame );
		~PrefsView();	
		void AttachedToWindow();
		bool OkToQuit( void );
		void HandleMessage( os::Message* pcMessage );
		virtual os::Point GetPreferredSize( bool bLargest ) const { if( bLargest ) return( os::Point( 10000, 10000 ) ); return( os::Point( 200, 200 ) ); }
	private:
		os::LayoutView *m_pcLRoot;
		os::VLayoutNode *m_pcVLRoot;
		os::HLayoutNode *m_pcControls;
		os::HLayoutNode *m_pcButtons;
		os::Button* m_pcApply;
		os::DropdownMenu *m_pcSoundcard;
		uint32 m_nWidth;
		uint32 m_hCurrentSoundcard;
};


#endif

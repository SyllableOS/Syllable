/*
 *  Login Details Preferences for Syllable
 *  Copyright (C) 2002 William Rose
 *  Copyright (C) 2005 Kristian Van Der Vliet
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <pwd.h>

#include <gui/rect.h>
#include <gui/window.h>
#include <gui/tabview.h>
#include <gui/button.h>
#include <gui/layoutview.h>
#include <gui/imageview.h>
#include <gui/view.h>
#include <gui/frameview.h>
#include <gui/imagebutton.h>
#include <gui/textview.h>
#include <gui/filerequester.h>
#include <util/message.h>

#ifndef LOGIN_SYLLABLE__LOGIN_WINDOW_H_
#define LOGIN_SYLLABLE__LOGIN_WINDOW_H_

enum window_messages
{
	ID_WINDOW_OK,
	ID_WINDOW_CANCEL,
	ID_CHANGE_ICON
};

class LoginWindow : public os::Window
{
	public:
		LoginWindow( const os::Rect cFrame );
		~LoginWindow();

		void HandleMessage( os::Message *pcMessage );
		virtual bool OkToQuit();

		status_t Validate( std::string cOld );
		status_t SavePassword( const std::string cNew );

	private:
		os::BitmapImage * GetUserIcon( void );
		status_t SetUserIcon( void );

		os::FrameView *m_pcInfoFrame, *m_pcIconFrame, *m_pcPasswordFrame;
		os::LayoutView *m_pcButtonLayout;
		os::TextView *m_pcCurrentPassword, *m_pcNew1Password, *m_pcNew2Password;

		os::ImageView* m_pcLockIconView;

		os::ImageButton *m_pcIconButton;
		os::FileRequester *m_pcIconRequester;

		int m_nUid;
		os::String m_cLogin;

		os::String m_cIcon;
};

#endif

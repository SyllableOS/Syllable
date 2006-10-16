/*
 *  Users and Groups Preferences for Syllable
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

#include <stdlib.h>
#include <gui/rect.h>
#include <util/application.h>

#include <users_window.h>

using namespace os;

class UsersApplication : public Application
{
	public:
		UsersApplication();

	private:
		UsersWindow *m_pcUsersWindow;
};

UsersApplication::UsersApplication() : Application( "application/x-vnd-UsersPreferences" )
{
	SetCatalog("UsersAndGroups.catalog");

	m_pcUsersWindow = new UsersWindow( Rect( 0, 0, 399, 449 ) );

	m_pcUsersWindow->CenterInScreen();
	m_pcUsersWindow->Show();
	m_pcUsersWindow->MakeFocus();
}

int main( int argc, char **argv )
{
	UsersApplication *pcApplication = new UsersApplication();
	pcApplication->Run();

	return EXIT_SUCCESS;
}


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

#include <pwd.h>
#include <grp.h>

#include <vector>

#include <gui/rect.h>
#include <gui/window.h>
#include <gui/tabview.h>
#include <gui/button.h>
#include <gui/layoutview.h>
#include <gui/view.h>
#include <gui/listview.h>
#include <util/message.h>

#ifndef USERS_SYLLABLE__USERS_WINDOW_H_
#define USERS_SYLLABLE__USERS_WINDOW_H_

enum users_messages
{
	ID_USERS_SELECT,
	ID_USERS_ADD,
	ID_USERS_POST_ADD,
	ID_USERS_EDIT,
	ID_USERS_POST_EDIT,
	ID_USERS_DELETE,
	ID_USERS_SET_PASSWD,
	ID_USERS_NEW_PASSWORD
};

class UsersView : public os::View
{
	public:
		UsersView( const os::Rect cFrame );
		~UsersView();

		virtual void AllAttached( void );
		virtual void HandleMessage( os::Message *pcMessage );

		status_t SaveChanges( void );

	private:
		status_t Validate( const int32 nUid, const std::string cOld );
		void DisplayProperties( const std::string cTitle, const struct passwd *psPasswd, os::Message *pcMessage );
		status_t UpdateList( const int nSelected );

		os::LayoutView *m_pcLayoutView;
		os::ListView *m_pcUsersList;
		os::Button *m_pcAdd, *m_pcEdit, *m_pcDelete, *m_pcPassword;

		struct passwd *m_psSelected;

		std::vector<std::string>m_vNewHomes;

		bool m_bModified;
};

enum groups_messages
{
	ID_GROUPS_SELECT,
	ID_GROUPS_ADD,
	ID_GROUPS_POST_ADD,
	ID_GROUPS_EDIT,
	ID_GROUPS_POST_EDIT,
	ID_GROUPS_DELETE
};

class GroupsView : public os::View
{
	public:
		GroupsView( const os::Rect cFrame );
		~GroupsView();

		virtual void AllAttached( void );
		virtual void HandleMessage( os::Message *pcMessage );

		status_t SaveChanges( void );

	private:
		void DisplayProperties( const std::string cTitle, const struct group *psGroup, os::Message *pcMessage );
		status_t UpdateList( const int nSelected );

		os::LayoutView *m_pcLayoutView;
		os::ListView *m_pcGroupsList;
		os::Button *m_pcAdd, *m_pcEdit, *m_pcDelete;

		struct group *m_psSelected;

		bool m_bModified;
};

enum window_messages
{
	ID_WINDOW_OK,
	ID_WINDOW_CANCEL
};

class UsersWindow : public os::Window
{
	public:
		UsersWindow( const os::Rect cFrame );
		~UsersWindow();

		void HandleMessage( os::Message *pcMessage );
		virtual bool OkToQuit();

	private:
		os::TabView *m_pcTabView;
		UsersView *m_pcUsersView;
		GroupsView *m_pcGroupsView;
		os::LayoutView *m_pcLayoutView;
};

#endif

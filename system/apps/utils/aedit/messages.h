// AEdit 0.6 -:-  (C)opyright 2000-2002 Kristian Van Der Vliet
//                (C)opyright 2004 Jonas Jarvoll
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef __MESSAGES_H_
#define __MESSAGES_H_

enum Messages
{
	M_MENU_APP_QUIT,
	M_MENU_APP_ABOUT,
	M_MENU_FILE_NEW,
	M_MENU_FILE_OPEN,
	M_MENU_FILE_CLOSE,
	M_MENU_FILE_SAVE,
	M_MENU_FILE_SAVE_AS,
	M_MENU_FILE_SAVE_ALL,
	M_MENU_FILE_NEXT_TAB,
	M_MENU_FILE_PREV_TAB,
	M_MENU_EDIT_CUT,
	M_MENU_EDIT_COPY,
	M_MENU_EDIT_PASTE,
	M_MENU_EDIT_SELECT_ALL,
	M_MENU_EDIT_UNDO,
	M_MENU_EDIT_REDO,
	M_MENU_EDIT_ANCHOR,
	M_MENU_EDIT_UP,
	M_MENU_EDIT_DOWN,
	M_MENU_VIEW_TOOLBAR,
	M_MENU_VIEW_STATUSBAR,
	M_MENU_VIEW_SETFONT,
	M_MENU_VIEW_TABSHOW,
	M_MENU_VIEW_TABHIDE,
	M_MENU_VIEW_TABAUTO,
	M_MENU_FIND_FIND,
	M_MENU_FIND_REPLACE,
	M_MENU_FIND_GOTO,
	M_MENU_SETTINGS_SAVE_ON_CLOSE,
	M_MENU_SETTINGS_SAVE_NOW,
	M_MENU_SETTINGS_RESET,
	M_MENU_FONT_SIZE,
	M_MENU_FONT_FAMILY_AND_STYLE,
	M_BUT_FILE_NEW,
	M_BUT_FILE_OPEN,
	M_BUT_FILE_SAVE,
	M_BUT_EDIT_CUT,
	M_BUT_EDIT_COPY,
	M_BUT_EDIT_PASTE,
	M_BUT_FIND_FIND,
	M_BUT_EDIT_UNDO,
	M_BUT_EDIT_REDO,
	M_EDITOR_INVOKED,
	M_BUT_GOTO_GOTO,
	M_BUT_GOTO_CLOSE,
	M_BUT_GOTO_TEXTVIEW,
	M_BUT_FIND_GO,
	M_BUT_FIND_NEXT,
	M_BUT_FIND_CLOSE,
	M_BUT_FIND_TEXTVIEW,
	M_BUT_REPLACE_DO,
	M_BUT_REPLACE_CLOSE,
	M_BUT_REPLACE_FIND_TEXTVIEW,
	M_BUT_REPLACE_REPLACE_TEXTVIEW,
	M_INVOKED_TAB_CHANGED,
	M_INVOKED_CLOSE_ALERT,
	M_INVOKED_SAVEAS_ALERT,
	M_INVOKED_QUIT_ALERT,
	M_BUT_ABOUT_CLOSE,
	M_VOID					// Used where no message is required
};

#endif


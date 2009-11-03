/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2000 Kurt Skauen
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

#ifndef __F_SYLLABLE_MESSAGES_H__
#define __F_SYLLABLE_MESSAGES_H__

#ifdef __cplusplus
extern "C" {
#endif

enum {
	/* Attention! */
	/* Be careful not to change the sequence of message codes here, */
	/* as doing so would break binary compatibility! */
	
    M_LAST_USER_MSG   = 9999999,	
    M_ABOUT_REQUESTED = 10000000,
    M_WINDOW_ACTIVATED,
    M_APP_ACTIVATED,
    M_ARGV_RECEIVED,
    M_QUIT_REQUESTED,
    M_CLOSE_REQUESTED,

    M_KEY_DOWN,
    M_KEY_UP,
    M_MINIMIZE,
    M_MOUSE_DOWN,
    M_MOUSE_UP,
    M_MOUSE_MOVED,
    M_WHEEL_MOVED,
    M_READY_TO_RUN,
    M_PATHS_RECEIVED,
    M_SCREEN_CHANGED,
    M_VALUE_CHANGED,
    M_VIEW_MOVED,
    M_VIEW_RESIZED,

    M_WINDOW_MOVED,
    M_WINDOW_RESIZED,
    M_DESKTOP_ACTIVATED,
    M_SCREENMODE_CHANGED,
    M_ZOOM,
    M_PAINT,
    M_COLOR_CONFIG_CHANGED,
    M_FONT_CHANGED,
    M_MENU_EVENT,
    M_WINDOWS_CHANGED,
    _M_UNUSED3,
    M_QUIT,			/* Quit if you want	*/
    M_TERMINATE,			/* Quit NOW	*/
    _M_UNUSED4,
    _M_UNUSED5,
    _M_UNUSED6,
    _M_UNUSED7,
    _M_UNUSED8,
    _M_UNUSED9,
    M_MOVE_WINDOW,
    M_RESIZE_WINDOW,
    M_WINDOW_FRAME_CHANGED,

	/* Add new appserver message codes here. */

    M_SET_PROPERTY = 20000000,
    M_GET_PROPERTY,
    M_CREATE_PROPERTY,
    M_DELETE_PROPERTY,
    M_GET_SUPPORTED_SUITES,
    M_CUT,
    M_COPY,
    M_PASTE,
    M_LOAD_REQUESTED,
    M_SAVE_REQUESTED,
    M_FILE_REQUESTER_CANCELED,

    M_MESSAGE_NOT_UNDERSTOOD,
    M_NO_REPLY,
    M_REPLY,
    M_SIMPLE_DATA,
    M_MIME_DATA,
    M_ARCHIVED_OBJECT,
    M_UPDATE_STATUS_BAR,
    M_RESET_STATUS_BAR,
    M_NODE_MONITOR,

   	M_FONT_REQUESTED,
	M_FONT_REQUESTER_CANCELED,

	M_BOX_REQUESTED,
	M_BOX_REQUESTER_CANCELED,

	M_COLOR_REQUESTER_CANCELED,
	M_COLOR_REQUESTED,
	M_COLOR_REQUESTER_CHANGED,
	
	M_CLIPBOARD_CHANGED,
    
    /* Add new libsyllable message codes here. */
    
    M_FIRST_EVENT = 1000000000,
};

#ifdef __cplusplus
}
#endif

#endif	/* __F_SYLLABLE_MESSAGES_H__ */

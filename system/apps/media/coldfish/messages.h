/*  ColdFish Music Player
 *  Copyright (C) 2003 Kristian Van Der Vliet
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
 
#ifndef _MESSAGES_H_
#define _MESSAGES_H_


/* Messages */
enum
{
	/* Button messages */
	CF_GUI_PLAY,
	CF_GUI_PAUSE,
	CF_GUI_STOP,
	CF_GUI_SEEK,
	CF_GUI_SELECT_LIST,
	CF_GUI_SHOW_LIST,
	CF_GUI_REMOVE_FILE,
	
	CF_GUI_LIST_INVOKED,
	CF_GUI_LIST_SELECTED,

	/* App messages */
	CF_STATE_CHANGED,
	CF_ADD_FILE,
	CF_PLAY_NEXT
};

#endif


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

/* Play states */
enum
{
	CF_STATE_STOPPED,
	CF_STATE_PLAYING,
	CF_STATE_PAUSED,
};


/* Messages */
enum
{
	/* Button / menu messages */
	CF_GUI_PLAY = 0,
	CF_GUI_PAUSE,
	CF_GUI_STOP,
	CF_GUI_SEEK,
	CF_GUI_SELECT_LIST,
	CF_GUI_OPEN_INPUT,
	CF_GUI_SHOW_LIST,
	CF_GUI_ADD_FILE,
	CF_GUI_REMOVE_FILE,
	CF_GUI_QUIT,
	CF_GUI_ABOUT,
	
	CF_GUI_LIST_INVOKED,
	CF_GUI_LIST_SELECTED,
	
	CF_GUI_VIEW_LIST,
	
	CF_IS_CANCEL,
	CF_IS_OPEN,
	
	/* App messages */
	CF_STATE_CHANGED = 100,
	CF_ADD_FILE = 101,
	CF_PLAY_NEXT = 102,
	CF_PLAY_PREVIOUS = 103,
	CF_SET_VIS_PLUGIN = 104,
	CF_GET_SONG=105,
	CF_GET_PLAYSTATE=106
};

#endif



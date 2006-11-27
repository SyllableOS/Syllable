#ifndef __F_DESKTOP_H__
#define __F_DESKTOP_H__

/*
 *  The AtheOS application server
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#include "swindow.h"


bool init_desktops();

void set_desktop_config( int nDesktop, const os::screen_mode& sMode, const std::string& cBackDropPath );
int  get_desktop_config( int* pnActiveDesktop, os::screen_mode* psMode, std::string* pcBackdropPath );
void set_desktop_screenmode( int nDesktop, const os::screen_mode& sMode );

int get_active_desktop();
int get_prev_desktop();
os::Rect get_desktop_max_window_frame( int nDesktop );
void set_desktop_max_window_frame( int nDesktop, os::Rect cWinSize );

SrvWindow* get_first_window( int nDesktop );
SrvWindow* get_active_window( bool bIgnoreSystemWindows );
void set_active_window( SrvWindow* pcWindow, bool bNotifyPrevious = true );

void add_window_to_desktop( SrvWindow* pcWindow );
void remove_window_from_desktop( SrvWindow* pcWindow );
void remove_from_focusstack( SrvWindow* pcWindow );
void set_desktop( int nNum, bool bNotifyActiveWnd = true );

//void set_desktop_screenmode( int nDesktop, int nWidth, int nHeight, os::color_space eColorSpace, float vRefreshRate,
//			     float vHPos, float vVPos, float vHSize, float vVSize );

#endif // __F_DESKTOP_H__



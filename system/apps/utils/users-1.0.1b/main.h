/*
 *  Users and Passwords Manager for AtheOS
 *  Copyright (C) 2002 William Rose
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
#ifndef WR_USERS_MAIN_H
#define WR_USERS_MAIN_H

#include <gui/image.h>
#include <gui/window.h>

bool compare_pwd_to_pwd( const char *pzCmp, const char *pzPwd );
bool compare_pwd_to_txt( const char *pzCmp, const char *pzTxt );
os::BitmapImage *LoadImage(const std::string&);
void errno_alert( const char *pzTitle, const char *pzAction, os::Window* pcWin=NULL );


#endif




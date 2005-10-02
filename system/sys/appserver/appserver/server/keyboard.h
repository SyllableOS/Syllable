/*
 *  The AtheOS application server
 *  Copyright (C) 1999  Kurt Skauen
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

#include <stdio.h>
#include <appserver/keymap.h>
#include <util/locker.h>

extern keymap g_sDefaultKeyMap;
extern keymap* g_psKeymap;
extern os::Gate g_cKeymapGate;

keymap* load_keymap( FILE* hFile );

uint32	GetQualifiers();
int		convert_key_code( char* pzDst, int nRawKey, int nQual, int *nDeadKeyState );
void	HandleKeyboard( bool bKeyEvent, int nKeyCode );
void	InitKeyboard( void );
bool	SetKeymap( const std::string& cKeymapPath );







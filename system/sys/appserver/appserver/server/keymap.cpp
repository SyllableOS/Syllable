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
#include <assert.h>
#include <ctype.h>
#include <malloc.h>
#include <string.h>

#include <atheos/kernel.h>

#include <appserver/keymap.h>

int load_keymap( FILE* hFile, keymap* psKeymap )
{
    uint32 anHeader[2];

    if ( fread( anHeader, sizeof(uint32), 2, hFile ) != 2 ) {
	dbprintf( "Error: Failed to read keymap header\n" );
	return( -1 );
    }
    if ( anHeader[0] != KEYMAP_MAGIC ) {
	dbprintf( "Error: Keymap have bad magic number (%08lx) should have been %08x\n", anHeader[0], KEYMAP_MAGIC );
	return( -1 );
    }
    if ( anHeader[1] != CURRENT_KEYMAP_VERSION ) {
	dbprintf( "Error: Unknown keymap version %ld\n", anHeader[1] );
	return( -1 );
    }
    if ( fread( psKeymap, sizeof(*psKeymap), 1, hFile ) == 1 ) {
	return( 0 );
    } else {
	dbprintf( "Error: Failed to read keymap\n" );
	return( -1 );
    }
}


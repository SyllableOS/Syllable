
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

keymap *load_keymap( FILE *hFile )
{
	keymap_header sHeader;

	if( fread( &sHeader, sizeof( sHeader ), 1, hFile ) != 1 )
	{
		dbprintf( "Error: Failed to read keymap header\n" );
		return ( NULL );
	}
	if( sHeader.m_nMagic != KEYMAP_MAGIC )
	{
		dbprintf( "Error: Keymap have bad magic number (%08x) should have been %08x\n", sHeader.m_nMagic, KEYMAP_MAGIC );
		return ( NULL );
	}
	if( sHeader.m_nVersion != CURRENT_KEYMAP_VERSION )
	{
		dbprintf( "Error: Unknown keymap version %d\n", sHeader.m_nVersion );
		return ( NULL );
	}

	keymap *psKeymap = ( keymap * ) malloc( sHeader.m_nSize );

	if( !psKeymap )
	{
		dbprintf( "Error: Could not allocate memory for keymap (Size: %d)\n", sHeader.m_nSize );
		return ( NULL );
	}

	if( fread( psKeymap, sHeader.m_nSize, 1, hFile ) == 1 )
	{
		return ( psKeymap );
	}
	else
	{
		dbprintf( "Error: Failed to read keymap\n" );
		return ( NULL );
	}
}

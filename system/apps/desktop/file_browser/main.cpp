/*  Syllable FileBrowser
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
 
#include <sys/stat.h>
#include "application.h"

int main( int argc, char *argv[] )
{
	const char *pzHome = getenv( "HOME" );
	App* pcApp = NULL;
	struct stat sStat;
	
	if ( ( argc > 1 ) && ( lstat( argv[1], &sStat ) >= 0 ) )
	{
		pcApp = new App( argv[1] );
	}
	else
	{
		pcApp = new App( pzHome );
	}
	pcApp->Run();
	return( 0 );
}












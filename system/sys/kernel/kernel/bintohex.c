
/*
 *  bintohex - outputs a binary file as a comma-separated list of hex numbers
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
#include <atheos/types.h>


static const size_t BUFFER_SIZE = 1024;

int main( int argc, char **argv )
{
	FILE *hInFile;
	FILE *hOutFile;
	uint8 anBuffer[BUFFER_SIZE];
	int nSize;
	int i;

	if ( argc != 3 )
	{
		printf( "Error: Invalid argument count.\n" );
		return ( 1 );
	}
	hInFile = fopen( argv[1], "r" );
	if ( NULL == hInFile )
	{
		printf( "Error: failed to open input file %s\n", argv[1] );
		return ( 1 );
	}
	hOutFile = fopen( argv[2], "w" );
	if ( NULL == hOutFile )
	{
		printf( "Error: failed to open output file %s\n", argv[2] );
		return ( 1 );
	}
	for ( ;; )
	{
		nSize = fread( anBuffer, 1, BUFFER_SIZE, hInFile );

		for ( i = 0; i < nSize; ++i )
		{
			fprintf( hOutFile, "0x%02x,\n", anBuffer[i] );
		}
		if ( nSize != BUFFER_SIZE )
		{
			break;
		}
	}
	fclose( hInFile );
	fclose( hOutFile );
	return ( 0 );
}

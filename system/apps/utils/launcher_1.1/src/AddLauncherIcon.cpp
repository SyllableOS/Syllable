/*
    Launcher - A program launcher for AtheOS
    Copyright (C) 2002 Andrew Kennan (bewilder@stresscow.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <storage/fsnode.h>
#include <stdio.h>
#include <string>

using namespace os;

int main( int argc, char** argv )
{

	if( argc == 3 ) {
		string zTarget = argv[1];
		string zValue  = argv[2];

		FSNode *pcNode = new FSNode( zTarget );
		
		int n = pcNode->WriteAttr( "LAUNCHER_ICON", O_TRUNC, ATTR_TYPE_STRING, zValue.c_str(), 0, zValue.length() );

		if( n > -1 ) {
			printf( "Done\n" );
		} else {
			printf( "Failed %d\n", errno );
		}
	} else {
		printf("AddLauncherIcon v1.0.0 - Add Launcher Icons to files and directories.
		
Copyright 2002 Andrew Kennan

This program is licenced under the GNU GPL. For more information see the file
/Applications/Launcher/COPYING

Usage:
	AddLauncherIcon <path> <png file>

" );
	}
}
		

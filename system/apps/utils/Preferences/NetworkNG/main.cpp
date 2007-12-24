// Syllable Network Preferences - Copyright 2006 Andrew Kennan
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "App.h"

#include <util/optionparser.h>

Action GetAction( int argc, char **argv )
{
	OptionParser cOpts( argc, argv );

	OptionParser::argmap asArgs[] =
	{
		{
		"detect", 'd', "Detect changes to the configuration of network hardware."},
		{
		NULL, 0, NULL}
	};
	cOpts.AddArgMap( asArgs );
	cOpts.ParseOptions( "d" );

	return cOpts.FindOption( 'd' ) == NULL ? Normal : Detect;
}

int main( int argc, char **argv )
{

	App *pcApp = new App( GetAction( argc, argv ) );

	pcApp->Run();
	return ( 0 );
}

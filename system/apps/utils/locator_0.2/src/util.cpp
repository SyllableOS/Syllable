/*
    Locator - A fast file finder for Syllable
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


#include "util.h"

using namespace std;

os::Rect center_frame( const os::Rect &cFrame )
{
	Rect cNewFrame;
	Desktop *pcDesktop = new Desktop( );
	IPoint *pcRes;
	pcRes = new IPoint( pcDesktop->GetResolution( ) );
	
	float vHalfWidth = cFrame.Width() / 2;
	float vHalfHeight = cFrame.Height() / 2;
	
	cNewFrame.left =   (pcRes->x / 2) - vHalfWidth;
	cNewFrame.top =    (pcRes->y / 2) - vHalfHeight;
	cNewFrame.right =  (pcRes->x / 2) + vHalfWidth;
	cNewFrame.bottom = (pcRes->y / 2) + vHalfHeight;
	
	return cNewFrame;
}


	// Convert to an absolute path (resolve ~/^)
std::string convert_path( std::string zPath )
{
	os::String zTmpPath = "";	
	
	if( zPath.length() > 0 ) {
		int n = 0;
		while( zPath.find( "/", n ) != string::npos ) {
			n = zPath.find( "/", n ) + 1;
		}	
		zTmpPath = zPath.substr( 0, n );
	
		Directory *pcDir = new Directory( );
		if( pcDir->SetTo( zTmpPath.c_str() ) == 0 ) {
			pcDir->GetPath( &zTmpPath );
			zTmpPath = zTmpPath + (os::String)"/" + zPath.substr( n );
		}
			
		delete pcDir;
	}
		
	return zTmpPath.str();
}


std::string app_path( void )
{
	static string zLauncherPath;
	
	if( zLauncherPath == "" ) {
		zLauncherPath = convert_path( "^/" );
		zLauncherPath = zLauncherPath.substr( 0, zLauncherPath.length()-4 );
	}
	
	return zLauncherPath;
}

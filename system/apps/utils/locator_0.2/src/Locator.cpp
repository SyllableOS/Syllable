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

#include "locator_window.h"


class Locator : public Application
{

    LocatorWindow* m_pcMainWindow;

  public:
    Locator( );
    ~Locator( );

};


Locator::Locator( ) : Application( "application/x-vnd.ADK-Locator" )
{
  
  m_pcMainWindow = new LocatorWindow( );
  m_pcMainWindow->Show( );

}


Locator::~Locator( )
{

  m_pcMainWindow->Close( );

}


int main( int argc, char** argv )
{

  Locator* pcMyLocator = new Locator( );

  pcMyLocator->Run( );
  return( 0 );

}

//  Coldfish 1.0 -:-  (C)opyright 2003 Kristian Van Der Vliet
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

#include "seekslider.h"

using namespace os;

SeekSlider::SeekSlider( const Rect & cFrame, const std::string & cName, Message * pcMsg ):Slider( cFrame, cName, pcMsg )
{
	m_bMouseUp = false;
}

SeekSlider::~SeekSlider( void )
{

}

bool SeekSlider::Invoked( Message * pcMessage )
{
	return ( m_bMouseUp );
}

void SeekSlider::MouseDown( const Point & cPosition, uint32 nButtons )
{
	m_bMouseUp = false;
	Slider::MouseDown( cPosition, nButtons );
}

void SeekSlider::MouseUp( const Point & cPosition, uint32 nButtons, Message * pcData )
{
	m_bMouseUp = true;
	Slider::MouseUp( cPosition, nButtons, pcData );
}
